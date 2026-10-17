#include "scriptPCH.h"
#include "kara.h"

enum
{
    SPELL_MORTAL_CLEAVE = 22859,
    SPELL_CHARGE = 22911,
    SPELL_KNOCK_AWAY = 10101,
};

struct boss_warden_harrowAI : public ScriptedAI
{
    boss_warden_harrowAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiMortalCleaveTimer;
    uint32 m_uiChargeTimer;
    uint32 m_uiKnockAwayTimer;

    void Reset() override
    {
        m_uiMortalCleaveTimer = urand(6000, 9000);
        m_uiChargeTimer = urand(12000, 16000);
        m_uiKnockAwayTimer = urand(18000, 23000);
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
         * Charge a random raid member other than the current tank.
         */
        if (m_uiChargeTimer <= uiDiff)
        {
            Unit* pTarget = SelectRandomNonTankTarget();

            if (pTarget)
            {
                if (DoCastSpellIfCan(pTarget, SPELL_CHARGE) == CAST_OK)
                {
                    m_uiChargeTimer = urand(18000, 24000);
                    return;
                }
            }

            m_uiChargeTimer = 2000;
        }
        else
            m_uiChargeTimer -= uiDiff;

        /*
         * Knock the current tank away.
         */
        if (m_uiKnockAwayTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                m_creature->GetVictim(),
                SPELL_KNOCK_AWAY
            ) == CAST_OK)
            {
                m_uiKnockAwayTimer = urand(22000, 28000);
                return;
            }

            m_uiKnockAwayTimer = 2000;
        }
        else
            m_uiKnockAwayTimer -= uiDiff;

        /*
         * Main heavy melee attack.
         */
        if (m_uiMortalCleaveTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                m_creature->GetVictim(),
                SPELL_MORTAL_CLEAVE
            ) == CAST_OK)
            {
                m_uiMortalCleaveTimer = urand(9000, 13000);
                return;
            }

            m_uiMortalCleaveTimer = 2000;
        }
        else
            m_uiMortalCleaveTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_warden_harrow(Creature* pCreature)
{
    return new boss_warden_harrowAI(pCreature);
}

void AddSC_boss_warden_harrow()
{
    Script* pNewScript = new Script;

    pNewScript->Name = "boss_warden_harrow";
    pNewScript->GetAI = &GetAI_boss_warden_harrow;
    pNewScript->RegisterSelf();
}