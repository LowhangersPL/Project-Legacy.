
#include "scriptPCH.h"
#include "kara.h"

// The Gullet is a durable melee boss built around tank pressure.
// At 15% health he enrages with Unholy Frenzy and finishes the fight faster.

enum
{
    SPELL_MORTAL_WOUND = 25646,
    SPELL_KNOCK_AWAY = 10101,
    SPELL_UNHOLY_FRENZY = 27995
};

struct boss_the_gulletAI : public ScriptedAI
{
    boss_the_gulletAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    // Main ability cooldowns.
    uint32 m_uiMortalWoundTimer;
    uint32 m_uiKnockAwayTimer;

    // Prevents the 15% enrage from triggering more than once.
    bool m_bEnraged;

    void Reset() override
    {
        // Initial timings give the tank a few seconds before both mechanics begin.
        m_uiMortalWoundTimer = urand(8000, 12000);
        m_uiKnockAwayTimer = urand(15000, 20000);

        m_bEnraged = false;
    }

    void UpdateAI(uint32 const diff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        // At 15% health he yells and casts Unholy Frenzy on himself once.
        if (!m_bEnraged && m_creature->GetHealthPercent() <= 15.0f)
        {
            m_bEnraged = true;

            m_creature->MonsterYell("FEEEED!", LANG_UNIVERSAL, 0);
            DoCastSpellIfCan(m_creature, SPELL_UNHOLY_FRENZY);
        }

        // Mortal Wound keeps steady healing pressure on the current tank.
        if (m_uiMortalWoundTimer <= diff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_MORTAL_WOUND) == CAST_OK)
            {
                m_uiMortalWoundTimer = urand(12000, 16000);
                return;
            }

            m_uiMortalWoundTimer = 2000;
        }
        else
            m_uiMortalWoundTimer -= diff;

        // Knock Away periodically throws back the current tank.
        if (m_uiKnockAwayTimer <= diff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_KNOCK_AWAY) == CAST_OK)
            {
                m_uiKnockAwayTimer = urand(18000, 24000);
                return;
            }

            m_uiKnockAwayTimer = 2000;
        }
        else
            m_uiKnockAwayTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_the_gullet(Creature* pCreature)
{
    return new boss_the_gulletAI(pCreature);
}

// Registers the script name used by creature_template.script_name.
void AddSC_boss_the_gullet()
{
    Script* pNewScript = new Script;

    pNewScript->Name = "boss_the_gullet";
    pNewScript->GetAI = &GetAI_boss_the_gullet;
    pNewScript->RegisterSelf();
}
