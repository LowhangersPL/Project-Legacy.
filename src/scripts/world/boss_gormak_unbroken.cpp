#include "scriptPCH.h"

enum
{
    SPELL_MORTAL_STRIKE = 12294,
    SPELL_ARCING_SMASH = 28168,
    SPELL_REND = 21949
};

struct boss_gormak_unbrokenAI : public ScriptedAI
{
    boss_gormak_unbrokenAI(Creature* pCreature)
        : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiMortalStrikeTimer;
    uint32 m_uiArcingSmashTimer;
    uint32 m_uiRendTimer;

    bool m_bHalfHealthLine;

    void Reset() override
    {
        m_uiMortalStrikeTimer = urand(9000, 12000);
        m_uiArcingSmashTimer = urand(6000, 9000);
        m_uiRendTimer = urand(12000, 16000);

        m_bHalfHealthLine = false;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        m_creature->MonsterSay(
            "Gormak smash little ones now!",
            LANG_UNIVERSAL,
            0);
    }

    void KilledUnit(Unit* pVictim) override
    {
        if (!pVictim || !pVictim->IsPlayer())
            return;

        switch (urand(0, 2))
        {
        case 0:
            m_creature->MonsterSay(
                "HA! Gormak hit good!",
                LANG_UNIVERSAL,
                0);
            break;

        case 1:
            m_creature->MonsterSay(
                "Pretty ladies see that?",
                LANG_UNIVERSAL,
                0);
            break;

        case 2:
            m_creature->MonsterSay(
                "Gormak very strong!",
                LANG_UNIVERSAL,
                0);
            break;
        }
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        m_creature->MonsterSay(
            "Gormak... still handsome...",
            LANG_UNIVERSAL,
            0);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() ||
            !m_creature->GetVictim())
        {
            return;
        }

        Unit* pVictim =
            m_creature->GetVictim();

        // One flavour line at 50%.
        if (!m_bHalfHealthLine &&
            m_creature->GetHealthPercent() <= 50.0f)
        {
            m_bHalfHealthLine = true;

            m_creature->MonsterSay(
                "Little ones make Gormak angry!",
                LANG_UNIVERSAL,
                0);
        }

        // =====================================================
        // MORTAL STRIKE
        // =====================================================

        if (m_uiMortalStrikeTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                pVictim,
                SPELL_MORTAL_STRIKE) ==
                CAST_OK)
            {
                m_uiMortalStrikeTimer =
                    urand(12000, 15000);

                return;
            }

            m_uiMortalStrikeTimer = 1000;
        }
        else
        {
            m_uiMortalStrikeTimer -= uiDiff;
        }

        // =====================================================
        // ARCING SMASH
        // =====================================================

        if (m_uiArcingSmashTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                pVictim,
                SPELL_ARCING_SMASH) ==
                CAST_OK)
            {
                m_uiArcingSmashTimer =
                    urand(8000, 12000);

                return;
            }

            m_uiArcingSmashTimer = 1000;
        }
        else
        {
            m_uiArcingSmashTimer -= uiDiff;
        }

        // =====================================================
        // REND
        // =====================================================

        if (m_uiRendTimer <= uiDiff)
        {
            if (!pVictim->HasAura(
                SPELL_REND))
            {
                if (DoCastSpellIfCan(
                    pVictim,
                    SPELL_REND) ==
                    CAST_OK)
                {
                    m_uiRendTimer =
                        urand(15000, 20000);

                    return;
                }

                m_uiRendTimer = 1000;
            }
            else
            {
                // Don't refresh Rend while it's already active.
                m_uiRendTimer = 3000;
            }
        }
        else
        {
            m_uiRendTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_gormak_unbroken(
    Creature* pCreature)
{
    return new boss_gormak_unbrokenAI(
        pCreature);
}

void AddSC_boss_gormak_unbroken()
{
    Script* pNewScript =
        new Script;

    pNewScript->Name =
        "boss_gormak_unbroken";

    pNewScript->GetAI =
        &GetAI_boss_gormak_unbroken;

    pNewScript->RegisterSelf();
}