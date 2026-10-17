#include "scriptPCH.h"
#include "kara.h"

enum
{
    SPELL_FROSTBOLT = 25304,
    SPELL_BLIZZARD = 27618,
    SPELL_CONE_OF_COLD = 10161,

    NPC_HALLWELL_CRYPTGUARD = 1000020,
    NPC_HALLWELL_CARETAKER = 1000025,
    NPC_HALLWELL_FORGOTTEN = 1000026,
    NPC_HALLWELL_GHOUL = 1000027,
    NPC_HALLWELL_MAGGOT = 1000028,
    NPC_HALLWELL_SPIRIT = 1000037,
    NPC_HALLWELL_SHADE = 1000039,
};

static float const ADD_SPAWN_X = -1141.281250f;
static float const ADD_SPAWN_Y = -472.093567f;
static float const ADD_SPAWN_Z = 31.240137f;
static float const ADD_SPAWN_O = 4.721414f;

struct boss_keeper_hallwellAI : public ScriptedAI
{
    boss_keeper_hallwellAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiFrostboltTimer;
    uint32 m_uiBlizzardTimer;
    uint32 m_uiConeOfColdTimer;
    uint32 m_uiSummonTimer;

    std::list<ObjectGuid> m_lSummonedUndeadGuids;

    void DespawnSummonedUndead()
    {
        for (std::list<ObjectGuid>::iterator itr =
            m_lSummonedUndeadGuids.begin();
            itr != m_lSummonedUndeadGuids.end();
            ++itr)
        {
            Creature* pSummon = m_creature->GetMap()->GetCreature(*itr);

            if (pSummon)
                pSummon->ForcedDespawn();
        }

        m_lSummonedUndeadGuids.clear();
    }

    void Reset() override
    {
        /*
         * Reset is called when Hallwell evades after the party wipes.
         * Remove every surviving add from the previous attempt.
         */
        DespawnSummonedUndead();

        m_uiFrostboltTimer = urand(2000, 3500);
        m_uiBlizzardTimer = urand(12000, 16000);
        m_uiConeOfColdTimer = urand(8000, 12000);
        m_uiSummonTimer = 30000;

        m_creature->SetCasterChaseDistance(25.0f);
    }

    void Aggro(Unit* /*pWho*/) override
    {
        m_creature->MonsterYell(
            "So... you are the ones who have been disturbing my crypts.",
            LANG_UNIVERSAL,
            0
        );
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        m_creature->MonsterYell(
            "This vessel... can be replaced...",
            LANG_UNIVERSAL,
            0
        );

        /*
         * Also clean up surviving adds when Hallwell is defeated.
         */
        DespawnSummonedUndead();
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

    Unit* SelectRandomRaidTarget()
    {
        Unit* pTarget = m_creature->SelectAttackingTarget(
            ATTACKING_TARGET_RANDOM,
            0
        );

        if (!pTarget)
            pTarget = m_creature->GetVictim();

        return pTarget;
    }

    uint32 SelectRandomUndeadEntry()
    {
        switch (urand(0, 9))
        {
        case 0:
        case 1:
            return NPC_HALLWELL_CRYPTGUARD;

        case 2:
        case 3:
            return NPC_HALLWELL_FORGOTTEN;

        case 4:
            return NPC_HALLWELL_GHOUL;

        case 5:
            return NPC_HALLWELL_MAGGOT;

        case 6:
            return NPC_HALLWELL_SPIRIT;

        case 7:
            return NPC_HALLWELL_SHADE;

        case 8:
            return NPC_HALLWELL_CARETAKER;

        default:
            return NPC_HALLWELL_GHOUL;
        }
    }

    void SummonRandomUndead()
    {
        Creature* pSummon = m_creature->SummonCreature(
            SelectRandomUndeadEntry(),
            ADD_SPAWN_X,
            ADD_SPAWN_Y,
            ADD_SPAWN_Z,
            ADD_SPAWN_O,
            TEMPSUMMON_CORPSE_TIMED_DESPAWN,
            3000
        );

        if (!pSummon)
            return;

        /*
         * Store the summon so Reset() can remove it after a party wipe.
         */
        m_lSummonedUndeadGuids.push_back(
            pSummon->GetObjectGuid()
        );

        /*
         * Immediately place the add in combat with the raid.
         */
        pSummon->SetInCombatWithZone();

        Unit* pTarget = SelectRandomRaidTarget();

        if (!pTarget)
            return;

        pSummon->AddThreat(pTarget, 1000.0f);

        if (pSummon->AI())
            pSummon->AI()->AttackStart(pTarget);
    }

    void SummonUndeadWave()
    {
        m_creature->MonsterYell(
            "Come forth, servants of the crypt!",
            LANG_UNIVERSAL,
            0
        );

        /*
         * Each add is selected independently, so duplicate types are possible.
         */
        SummonRandomUndead();
        SummonRandomUndead();
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() ||
            !m_creature->GetVictim())
        {
            return;
        }

        /*
         * Summon two random dungeon undead every 30 seconds.
         */
        if (m_uiSummonTimer <= uiDiff)
        {
            SummonUndeadWave();
            m_uiSummonTimer = 30000;
        }
        else
            m_uiSummonTimer -= uiDiff;

        /*
         * Cast Blizzard at a random non-tank raid member.
         */
        if (m_uiBlizzardTimer <= uiDiff)
        {
            Unit* pTarget = SelectRandomNonTankTarget();

            if (!pTarget)
                pTarget = m_creature->GetVictim();

            if (DoCastSpellIfCan(
                pTarget,
                SPELL_BLIZZARD
            ) == CAST_OK)
            {
                m_uiBlizzardTimer = urand(18000, 24000);
                return;
            }

            m_uiBlizzardTimer = 2000;
        }
        else
            m_uiBlizzardTimer -= uiDiff;

        /*
         * Use Cone of Cold when the current target is nearby.
         */
        if (m_uiConeOfColdTimer <= uiDiff)
        {
            Unit* pVictim = m_creature->GetVictim();

            if (pVictim &&
                m_creature->IsWithinDistInMap(pVictim, 10.0f))
            {
                if (DoCastSpellIfCan(
                    pVictim,
                    SPELL_CONE_OF_COLD
                ) == CAST_OK)
                {
                    m_uiConeOfColdTimer = urand(12000, 17000);
                    return;
                }
            }

            m_uiConeOfColdTimer = 2000;
        }
        else
            m_uiConeOfColdTimer -= uiDiff;

        /*
         * Frostbolt is Hallwell's normal filler attack.
         */
        if (m_uiFrostboltTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                m_creature->GetVictim(),
                SPELL_FROSTBOLT
            ) == CAST_OK)
            {
                m_uiFrostboltTimer = urand(3000, 4500);
                return;
            }

            m_uiFrostboltTimer = 1000;
        }
        else
            m_uiFrostboltTimer -= uiDiff;
    }
};

CreatureAI* GetAI_boss_keeper_hallwell(Creature* pCreature)
{
    return new boss_keeper_hallwellAI(pCreature);
}

void AddSC_boss_keeper_hallwell()
{
    Script* pNewScript = new Script;

    pNewScript->Name = "boss_keeper_hallwell";
    pNewScript->GetAI = &GetAI_boss_keeper_hallwell;
    pNewScript->RegisterSelf();
}