
#include "scriptPCH.h"
#include "kara.h"

// Lady Elenora is a ranged caster boss.
// At 20% health she restores her mana and enters a faster final phase.

enum
{
    SPELL_SHADOW_BOLT = 11661,
    SPELL_CURSE_OF_AGONY = 11713,
    SPELL_SILENCE = 18278,
    SPELL_BANSHEE_CURSE = 17105
};

struct boss_lady_elenoraAI : public ScriptedAI
{
    boss_lady_elenoraAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    // Individual cooldowns for each ability.
    uint32 m_uiShadowBoltTimer;
    uint32 m_uiCurseTimer;
    uint32 m_uiSilenceTimer;
    uint32 m_uiBansheeCurseTimer;

    // Prevents the 20% phase from triggering more than once.
    bool m_bFinalPhase;

    void Reset() override
    {
        // Initial ability timings.
        m_uiShadowBoltTimer = urand(2000, 4000);
        m_uiCurseTimer = urand(7000, 10000);
        m_uiSilenceTimer = urand(18000, 22000);
        m_uiBansheeCurseTimer = urand(12000, 16000);

        m_bFinalPhase = false;

        // Keeps her at casting range instead of constantly moving into melee.
        m_creature->SetCasterChaseDistance(25.0f);
    }

    // Final phase begins at 20% health.
    // She restores all mana, delivers her yell and shortens every spell timer.
    void EnterFinalPhase()
    {
        m_bFinalPhase = true;

        m_creature->MonsterYell(
            "You will not take him from me... I will join him in death!",
            LANG_UNIVERSAL,
            0);

        m_creature->SetPower(POWER_MANA, m_creature->GetMaxPower(POWER_MANA));

        m_uiShadowBoltTimer = 1000;
        m_uiCurseTimer = 5000;
        m_uiSilenceTimer = 12000;
        m_uiBansheeCurseTimer = 8000;
    }

    void UpdateAI(uint32 const diff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        // Trigger the faster final phase once she reaches 20% health.
        if (!m_bFinalPhase && m_creature->GetHealthPercent() <= 20.0f)
        {
            EnterFinalPhase();
            return;
        }

        // Banshee Curse is kept on the current tank to reduce melee hit chance.
        if (m_uiBansheeCurseTimer <= diff)
        {
            Unit* pTarget = m_creature->GetVictim();

            if (pTarget &&
                DoCastSpellIfCan(pTarget, SPELL_BANSHEE_CURSE) == CAST_OK)
            {
                m_uiBansheeCurseTimer = m_bFinalPhase ? urand(12000, 15000)
                    : urand(18000, 22000);
                return;
            }

            m_uiBansheeCurseTimer = 2000;
        }
        else
            m_uiBansheeCurseTimer -= diff;

        // Curse of Agony is applied to a random player.
        if (m_uiCurseTimer <= diff)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);

            if (pTarget &&
                DoCastSpellIfCan(pTarget, SPELL_CURSE_OF_AGONY) == CAST_OK)
            {
                m_uiCurseTimer = m_bFinalPhase ? urand(7000, 9000)
                    : urand(12000, 16000);
                return;
            }

            m_uiCurseTimer = 2000;
        }
        else
            m_uiCurseTimer -= diff;

        // Silence targets a random player to pressure the group's casters.
        if (m_uiSilenceTimer <= diff)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);

            if (pTarget &&
                DoCastSpellIfCan(pTarget, SPELL_SILENCE) == CAST_OK)
            {
                m_uiSilenceTimer = m_bFinalPhase ? urand(14000, 17000)
                    : urand(22000, 28000);
                return;
            }

            m_uiSilenceTimer = 2000;
        }
        else
            m_uiSilenceTimer -= diff;

        // Shadow Bolt is her main filler spell and becomes much faster below 20%.
        if (m_uiShadowBoltTimer <= diff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SHADOW_BOLT) == CAST_OK)
            {
                m_uiShadowBoltTimer = m_bFinalPhase ? urand(1500, 2500)
                    : urand(3500, 5000);
                return;
            }

            m_uiShadowBoltTimer = 1000;
        }
        else
            m_uiShadowBoltTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_lady_elenora(Creature* pCreature)
{
    return new boss_lady_elenoraAI(pCreature);
}

// Registers the script name used by creature_template.script_name.
void AddSC_boss_lady_elenora()
{
    Script* pNewScript = new Script;

    pNewScript->Name = "boss_lady_elenora";
    pNewScript->GetAI = &GetAI_boss_lady_elenora;
    pNewScript->RegisterSelf();
}
