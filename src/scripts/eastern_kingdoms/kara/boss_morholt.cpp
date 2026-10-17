#include "scriptPCH.h"
#include "kara.h"

enum
{
    NPC_CAPTAIN_ALDREN = 1000022,
    NPC_CAPTAIN_BARTHOL = 1000023,
    NPC_CAPTAIN_RENWALD = 1000024,

    SPELL_SHADOW_BOLT = 11661,
    SPELL_ANTI_MAGIC_SHIELD = 7121,
    SPELL_DRAIN_LIFE = 11700,
    SPELL_UNHOLY_FRENZY = 8699
};

/*
 * These are the database spawn GUIDs of the creatures inside
 * the small crypts surrounding Morholt.
 *
 * Any creature in this list that is still alive when Morholt
 * is engaged will immediately join the fight.
 */
static uint32 const aMorholtCryptTrashGuids[] =
{
    2530879,
    2530877,
    2530875,
    2583078,
    2583076,
    2530874,
    2530870,
    2530872,
    2530873,
    2530871,
    2530869,
    2530866,
    2530864,
    2530867,
    2530865,
    2530844,
    2530847,
    2530846,
    2530845,
    2530840,
    2530837,
    2530839,
    2530838,
    2530849,
    2530848,
    2530841,
    2530843,
    2530842
};

struct boss_morholtAI : public ScriptedAI
{
    boss_morholtAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiShadowBoltTimer;
    uint32 m_uiDrainLifeTimer;
    uint32 m_uiAntiMagicShieldTimer;

    bool m_bAldrenRevived;
    bool m_bBartholRevived;
    bool m_bRenwaldRevived;

    void Reset() override
    {
        m_uiShadowBoltTimer = urand(2000, 4000);

        // Drain Life lasts five seconds, so keep plenty of time
        // between casts.
        m_uiDrainLifeTimer = urand(14000, 18000);

        // Anti-Magic Shield lasts ten seconds. Its cooldown is
        // deliberately long so Morholt is not constantly protected.
        m_uiAntiMagicShieldTimer = urand(25000, 32000);

        m_bAldrenRevived = false;
        m_bBartholRevived = false;
        m_bRenwaldRevived = false;

        m_creature->SetCasterChaseDistance(25.0f);
    }

    void Aggro(Unit* pWho) override
    {
        PullSurvivingCryptTrash(pWho);
    }

    /*
     * Pull every listed crypt creature that is currently alive.
     *
     * Dead or despawned creatures are ignored automatically.
     */
    void PullSurvivingCryptTrash(Unit* pTarget)
    {
        if (!pTarget)
            return;

        for (uint32 i = 0;
            i < sizeof(aMorholtCryptTrashGuids) /
            sizeof(aMorholtCryptTrashGuids[0]);
            ++i)
        {
            uint32 const uiSpawnGuid = aMorholtCryptTrashGuids[i];

            /*
             * Creature ObjectGuids on this MaNGOS branch contain both
             * the creature entry and its database spawn GUID, so obtain
             * the entry from the creature spawn data first.
             */
            CreatureData const* pData = sObjectMgr.GetCreatureData(uiSpawnGuid);

            if (!pData)
                continue;

            Creature* pTrash =
                m_creature->GetMap()->GetCreature(
                    ObjectGuid(HIGHGUID_UNIT, pData->creature_id[0], uiSpawnGuid)
                );

            if (!pTrash || !pTrash->IsAlive())
                continue;

            pTrash->SetInCombatWithZone();

            if (pTrash->AI())
                pTrash->AI()->AttackStart(pTarget);
        }
    }

    /*
     * Summon one of the three fallen captains beside Morholt.
     *
     * The captain is a fresh summoned copy because the original
     * database corpse may already have despawned before the raid
     * begins the Morholt encounter.
     */
    Creature* SummonCaptain(
        uint32 uiEntry,
        float fOffsetX,
        float fOffsetY
    )
    {
        Creature* pCaptain = m_creature->SummonCreature(
            uiEntry,
            m_creature->GetPositionX() + fOffsetX,
            m_creature->GetPositionY() + fOffsetY,
            m_creature->GetPositionZ(),
            m_creature->GetOrientation(),
            TEMPSUMMON_DEAD_DESPAWN,
            0
        );

        if (!pCaptain)
            return nullptr;

        pCaptain->SetInCombatWithZone();

        Unit* pTarget = m_creature->GetVictim();

        if (!pTarget)
        {
            pTarget = m_creature->SelectAttackingTarget(
                ATTACKING_TARGET_RANDOM,
                0
            );
        }

        if (pTarget && pCaptain->AI())
            pCaptain->AI()->AttackStart(pTarget);

        /*
         * Each captain receives Unholy Frenzy once when brought
         * back. The effect lasts twenty seconds.
         */
        m_creature->CastSpell(
            pCaptain,
            SPELL_UNHOLY_FRENZY,
            true
        );

        return pCaptain;
    }

    void ReviveAldren()
    {
        m_bAldrenRevived = true;

        m_creature->MonsterYell(
            "Aldren... your soul is mine. Rise once more!",
            LANG_UNIVERSAL,
            0
        );

        SummonCaptain(
            NPC_CAPTAIN_ALDREN,
            3.0f,
            2.0f
        );
    }

    void ReviveBarthol()
    {
        m_bBartholRevived = true;

        m_creature->MonsterYell(
            "Barthol... death does not release you from my service!",
            LANG_UNIVERSAL,
            0
        );

        SummonCaptain(
            NPC_CAPTAIN_BARTHOL,
            -3.0f,
            2.0f
        );
    }

    void ReviveRenwald()
    {
        m_bRenwaldRevived = true;

        m_creature->MonsterYell(
            "Renwald... your oath binds you still. Rise!",
            LANG_UNIVERSAL,
            0
        );

        SummonCaptain(
            NPC_CAPTAIN_RENWALD,
            0.0f,
            -3.0f
        );
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() ||
            !m_creature->GetVictim())
        {
            return;
        }

        /*
         * Captain resurrection thresholds.
         *
         * These checks occur before normal spells so the resurrection
         * cannot be delayed by another successful cast.
         */
        float const fHealthPercent =
            m_creature->GetHealthPercent();

        if (!m_bAldrenRevived &&
            fHealthPercent <= 75.0f)
        {
            ReviveAldren();
            return;
        }

        if (!m_bBartholRevived &&
            fHealthPercent <= 50.0f)
        {
            ReviveBarthol();
            return;
        }

        if (!m_bRenwaldRevived &&
            fHealthPercent <= 25.0f)
        {
            ReviveRenwald();
            return;
        }

        /*
         * Anti-Magic Shield.
         *
         * This lasts ten seconds and is used approximately once every
         * 35–45 seconds after its first cast.
         */
        if (m_uiAntiMagicShieldTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                m_creature,
                SPELL_ANTI_MAGIC_SHIELD
            ) == CAST_OK)
            {
                m_uiAntiMagicShieldTimer =
                    urand(35000, 45000);

                return;
            }

            m_uiAntiMagicShieldTimer = 2000;
        }
        else
            m_uiAntiMagicShieldTimer -= uiDiff;

        /*
         * Drain Life on a random threat-list target.
         *
         * It is spaced out because the channel lasts five seconds.
         */
        if (m_uiDrainLifeTimer <= uiDiff)
        {
            Unit* pDrainTarget =
                m_creature->SelectAttackingTarget(
                    ATTACKING_TARGET_RANDOM,
                    0
                );

            if (pDrainTarget)
            {
                if (DoCastSpellIfCan(
                    pDrainTarget,
                    SPELL_DRAIN_LIFE
                ) == CAST_OK)
                {
                    m_uiDrainLifeTimer =
                        urand(22000, 30000);

                    return;
                }
            }

            m_uiDrainLifeTimer = 2000;
        }
        else
            m_uiDrainLifeTimer -= uiDiff;

        /*
         * Shadow Bolt filler.
         */
        if (m_uiShadowBoltTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                m_creature->GetVictim(),
                SPELL_SHADOW_BOLT
            ) == CAST_OK)
            {
                m_uiShadowBoltTimer =
                    urand(3500, 5500);

                return;
            }
        }
        else
            m_uiShadowBoltTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_morholt(Creature* pCreature)
{
    return new boss_morholtAI(pCreature);
}

void AddSC_boss_morholt()
{
    Script* pNewScript = new Script;

    pNewScript->Name = "boss_morholt";
    pNewScript->GetAI = &GetAI_boss_morholt;
    pNewScript->RegisterSelf();
}