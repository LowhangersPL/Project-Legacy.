/*
 * Karazhan Crypts - Gravemother
 *
 * Mechanics:
 *  - Venom Spit (25497) on a random non-tank target every 20-25 seconds.
 *  - Web II (4168) on a random non-tank target every 25-30 seconds.
 *  - Summons four Gravemother Spiderlings (1000048) every 30 seconds.
 *  - Below 25% health, spiderlings are summoned every 15-18 seconds.
 */

#include "scriptPCH.h"
#include "kara.h"
#include <cmath>

enum
{
    SPELL_VENOM_SPIT = 25497,
    SPELL_WEB_II = 4168,

    NPC_GRAVEMOTHER_SPIDERLING = 1000048,

    SPIDERLING_COUNT = 4
};

struct boss_gravemotherAI : public ScriptedAI
{
    boss_gravemotherAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiVenomSpitTimer;
    uint32 m_uiWebTimer;
    uint32 m_uiSpiderlingTimer;

    bool m_bBroodAwakened;

    void DespawnSpiderlings()
    {
        std::list<Creature*> spiderlings;

        GetCreatureListWithEntryInGrid(
            spiderlings,
            m_creature,
            NPC_GRAVEMOTHER_SPIDERLING,
            200.0f
        );

        for (std::list<Creature*>::iterator itr = spiderlings.begin();
            itr != spiderlings.end();
            ++itr)
        {
            (*itr)->ForcedDespawn();
        }
    }

    void Reset() override
    {
        DespawnSpiderlings();

        // Initial delays are staggered so every mechanic does not occur at once.
        m_uiVenomSpitTimer = urand(10000, 14000);
        m_uiWebTimer = urand(16000, 20000);
        m_uiSpiderlingTimer = urand(22000, 26000);

        m_bBroodAwakened = false;
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DespawnSpiderlings();
    }

    void SummonSpiderlings()
    {
        static float const fTwoPi = 6.28318530718f;
        static float const fSpawnDistance = 4.0f;

        for (uint8 i = 0; i < SPIDERLING_COUNT; ++i)
        {
            float fAngle = m_creature->GetOrientation() +
                (fTwoPi * float(i) / float(SPIDERLING_COUNT));

            float fX = m_creature->GetPositionX() + std::cos(fAngle) * fSpawnDistance;
            float fY = m_creature->GetPositionY() + std::sin(fAngle) * fSpawnDistance;
            float fZ = m_creature->GetPositionZ();

            Creature* pSpiderling = m_creature->SummonCreature(
                NPC_GRAVEMOTHER_SPIDERLING,
                fX,
                fY,
                fZ,
                fAngle,
                TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
                60000
            );

            if (!pSpiderling)
                continue;

            pSpiderling->SetInCombatWithZone();

            Unit* pTarget = m_creature->SelectAttackingTarget(
                ATTACKING_TARGET_RANDOM,
                0
            );

            if (!pTarget)
                pTarget = m_creature->GetVictim();

            if (pTarget && pSpiderling->AI())
                pSpiderling->AI()->AttackStart(pTarget);
        }
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        // At 25% health the brood begins spawning much faster.
        if (!m_bBroodAwakened && m_creature->GetHealthPercent() < 25.0f)
        {
            m_bBroodAwakened = true;

            // Ensure the first accelerated wave does not take too long to arrive.
            if (m_uiSpiderlingTimer > 10000)
                m_uiSpiderlingTimer = 10000;
        }

        // Venom Spit: prefer a random non-tank target.
        if (m_uiVenomSpitTimer <= uiDiff)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(
                ATTACKING_TARGET_RANDOM,
                1
            );

            if (!pTarget)
                pTarget = m_creature->GetVictim();

            if (DoCastSpellIfCan(pTarget, SPELL_VENOM_SPIT) == CAST_OK)
                m_uiVenomSpitTimer = urand(20000, 25000);
        }
        else
            m_uiVenomSpitTimer -= uiDiff;

        // Web II: prefer a random non-tank target.
        if (m_uiWebTimer <= uiDiff)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(
                ATTACKING_TARGET_RANDOM,
                1
            );

            if (!pTarget)
                pTarget = m_creature->GetVictim();

            if (DoCastSpellIfCan(pTarget, SPELL_WEB_II) == CAST_OK)
                m_uiWebTimer = urand(25000, 30000);
        }
        else
            m_uiWebTimer -= uiDiff;

        // Summon four weak spiderlings around Gravemother.
        if (m_uiSpiderlingTimer <= uiDiff)
        {
            SummonSpiderlings();

            if (m_bBroodAwakened)
                m_uiSpiderlingTimer = urand(15000, 18000);
            else
                m_uiSpiderlingTimer = 30000;
        }
        else
            m_uiSpiderlingTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_gravemother(Creature* pCreature)
{
    return new boss_gravemotherAI(pCreature);
}

void AddSC_boss_gravemother()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_gravemother";
    pNewScript->GetAI = &GetAI_boss_gravemother;
    pNewScript->RegisterSelf();
}