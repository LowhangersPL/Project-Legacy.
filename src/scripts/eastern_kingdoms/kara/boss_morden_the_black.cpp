#include "scriptPCH.h"
#include "kara.h"

enum
{
    SPELL_BONE_SHIELD = 27688,
    SPELL_SHADOW_BOLT = 11661,
    SPELL_SHADOW_BOLT_VOLLEY = 20741,
    SPELL_DRAIN_LIFE = 24618,
    SPELL_DARK_MENDING = 19775,
};

struct boss_morden_the_blackAI : public ScriptedAI
{
    boss_morden_the_blackAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiShadowBoltTimer;
    uint32 m_uiShadowBoltVolleyTimer;
    uint32 m_uiDrainLifeTimer;
    uint32 m_uiDarkMendingRetryTimer;

    bool m_bBoneShieldCast;
    bool m_bDarkMendingAnnounced;
    bool m_bDarkMendingUsed;

    void Reset() override
    {
        m_uiShadowBoltTimer = urand(2000, 4000);
        m_uiShadowBoltVolleyTimer = urand(8000, 12000);
        m_uiDrainLifeTimer = urand(12000, 18000);
        m_uiDarkMendingRetryTimer = 0;

        m_bBoneShieldCast = false;
        m_bDarkMendingAnnounced = false;
        m_bDarkMendingUsed = false;

        m_creature->SetCasterChaseDistance(25.0f);
    }

    void Aggro(Unit* /*pWho*/) override
    {
        if (DoCastSpellIfCan(m_creature, SPELL_BONE_SHIELD) == CAST_OK)
            m_bBoneShieldCast = true;
    }

    Unit* SelectRandomNonTankTarget()
    {
        Unit* pVictim = m_creature->GetVictim();

        for (uint8 i = 0; i < 10; ++i)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(
                ATTACKING_TARGET_RANDOM,
                1
            );

            if (pTarget && pTarget != pVictim)
                return pTarget;
        }

        return nullptr;
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        /*
         * Fallback in case Bone Shield failed to cast during Aggro.
         */
        if (!m_bBoneShieldCast)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BONE_SHIELD) == CAST_OK)
            {
                m_bBoneShieldCast = true;
                return;
            }
        }

        /*
         * At 10% health, Morden makes one last attempt to heal himself.
         *
         * The flag is set when the cast successfully begins. If players
         * interrupt it, he will not attempt Dark Mending again.
         */
        if (!m_bDarkMendingUsed &&
            m_creature->GetHealthPercent() <= 10.0f)
        {
            if (!m_bDarkMendingAnnounced)
            {
                m_creature->MonsterYell(
                    "The grave has not finished with me!",
                    LANG_UNIVERSAL,
                    0
                );

                m_bDarkMendingAnnounced = true;
                m_uiDarkMendingRetryTimer = 500;
            }

            if (m_uiDarkMendingRetryTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(
                    m_creature,
                    SPELL_DARK_MENDING
                ) == CAST_OK)
                {
                    m_bDarkMendingUsed = true;
                    return;
                }

                m_uiDarkMendingRetryTimer = 500;
            }
            else
                m_uiDarkMendingRetryTimer -= uiDiff;

            /*
             * Prioritize the emergency heal over all normal abilities.
             */
            return;
        }

        /*
         * Shadow Bolt Volley applies raid-wide pressure.
         */
        if (m_uiShadowBoltVolleyTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                m_creature->GetVictim(),
                SPELL_SHADOW_BOLT_VOLLEY
            ) == CAST_OK)
            {
                m_uiShadowBoltVolleyTimer = urand(15000, 20000);
                return;
            }

            m_uiShadowBoltVolleyTimer = 2000;
        }
        else
            m_uiShadowBoltVolleyTimer -= uiDiff;

        /*
         * Drain Life targets a random player other than the tank.
         */
        if (m_uiDrainLifeTimer <= uiDiff)
        {
            Unit* pTarget = SelectRandomNonTankTarget();

            if (!pTarget)
                pTarget = m_creature->GetVictim();

            if (DoCastSpellIfCan(
                pTarget,
                SPELL_DRAIN_LIFE
            ) == CAST_OK)
            {
                m_uiDrainLifeTimer = urand(20000, 25000);
                return;
            }

            m_uiDrainLifeTimer = 2000;
        }
        else
            m_uiDrainLifeTimer -= uiDiff;

        /*
         * Shadow Bolt is Morden's normal filler attack.
         */
        if (m_uiShadowBoltTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                m_creature->GetVictim(),
                SPELL_SHADOW_BOLT
            ) == CAST_OK)
            {
                m_uiShadowBoltTimer = urand(3000, 5000);
                return;
            }

            m_uiShadowBoltTimer = 1000;
        }
        else
            m_uiShadowBoltTimer -= uiDiff;
    }
};

CreatureAI* GetAI_boss_morden_the_black(Creature* pCreature)
{
    return new boss_morden_the_blackAI(pCreature);
}

void AddSC_boss_morden_the_black()
{
    Script* pNewScript = new Script;

    pNewScript->Name = "boss_morden_the_black";
    pNewScript->GetAI = &GetAI_boss_morden_the_black;
    pNewScript->RegisterSelf();
}