#include "scriptPCH.h"

enum
{
    SPELL_SCORPID_POISON = 24586,
    SPELL_LOW_SWIPE = 8716,

    NPC_SKARRA_BROODLING = 1000053
};

struct boss_skarra_broodmotherAI : public ScriptedAI
{
    boss_skarra_broodmotherAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiPoisonTimer;
    uint32 m_uiSwipeTimer;

    bool m_bFirstBroodWave;
    bool m_bSecondBroodWave;

    void Reset() override
    {
        std::list<Creature*> broodlings;
        GetCreatureListWithEntryInGrid(
            broodlings,
            m_creature,
            NPC_SKARRA_BROODLING,
            100.0f);

        for (std::list<Creature*>::iterator itr = broodlings.begin();
            itr != broodlings.end();
            ++itr)
        {
            (*itr)->ForcedDespawn();
        }

        m_uiPoisonTimer = 5000;
        m_uiSwipeTimer = 14000;

        m_bFirstBroodWave = false;
        m_bSecondBroodWave = false;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        m_creature->SetInCombatWithZone();
    }

    void SummonBroodlings()
    {
        static float const offsets[3][2] =
        {
            {  3.0f,  2.0f },
            { -3.0f,  2.0f },
            {  0.0f, -3.0f }
        };

        for (uint8 i = 0; i < 3; ++i)
        {
            float x = m_creature->GetPositionX() + offsets[i][0];
            float y = m_creature->GetPositionY() + offsets[i][1];
            float z = m_creature->GetPositionZ();

            if (Creature* pBroodling = m_creature->SummonCreature(
                NPC_SKARRA_BROODLING,
                x,
                y,
                z,
                m_creature->GetOrientation(),
                TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
                5 * MINUTE * IN_MILLISECONDS))
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(
                    ATTACKING_TARGET_RANDOM,
                    0))
                {
                    pBroodling->AI()->AttackStart(pTarget);
                }
                else if (Unit* pVictim = m_creature->GetVictim())
                {
                    pBroodling->AI()->AttackStart(pVictim);
                }
            }
        }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        // First brood wave at 65%
        if (!m_bFirstBroodWave &&
            m_creature->GetHealthPercent() <= 65.0f)
        {
            m_bFirstBroodWave = true;
            SummonBroodlings();
        }

        // Second brood wave at 35%
        if (!m_bSecondBroodWave &&
            m_creature->GetHealthPercent() <= 35.0f)
        {
            m_bSecondBroodWave = true;
            SummonBroodlings();
        }

        // Stacking scorpid poison on current tank
        if (m_uiPoisonTimer <= uiDiff)
        {
            if (Unit* pVictim = m_creature->GetVictim())
                m_creature->CastSpell(
                    pVictim,
                    SPELL_SCORPID_POISON,
                    false);

            m_uiPoisonTimer = urand(6000, 8000);
        }
        else
            m_uiPoisonTimer -= uiDiff;

        // Low Swipe on current tank
        if (m_uiSwipeTimer <= uiDiff)
        {
            if (Unit* pVictim = m_creature->GetVictim())
                m_creature->CastSpell(
                    pVictim,
                    SPELL_LOW_SWIPE,
                    false);

            m_uiSwipeTimer = urand(18000, 22000);
        }
        else
            m_uiSwipeTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_skarra_broodmother(Creature* pCreature)
{
    return new boss_skarra_broodmotherAI(pCreature);
}

void AddSC_boss_skarra_broodmother()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_skarra_broodmother";
    pNewScript->GetAI = &GetAI_boss_skarra_broodmother;
    pNewScript->RegisterSelf();
}