#include "scriptPCH.h"

enum
{
    SPELL_SHADOW_BOLT = 11659,
    SPELL_HEX = 17172,
    SPELL_CURSE_TUTENKASH = 12255
};

struct boss_zaljira_hexweaverAI : public ScriptedAI
{
    boss_zaljira_hexweaverAI(Creature* pCreature)
        : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiShadowBoltTimer;
    uint32 m_uiHexTimer;
    uint32 m_uiCurseTimer;

    void Reset() override
    {
        m_uiShadowBoltTimer = urand(2500, 4000);

        // First Hex fairly early.
        m_uiHexTimer = urand(12000, 16000);

        // First curse after a few seconds so the mechanic
        // is noticeable during shorter fights.
        m_uiCurseTimer = urand(7000, 10000);
    }

    Player* SelectHexTarget()
    {
        std::vector<Player*> vTargets;

        Unit* pVictim =
            m_creature->GetVictim();

        Map::PlayerList const& players =
            m_creature->GetMap()->GetPlayers();

        for (Map::PlayerList::const_iterator itr =
            players.begin();
            itr != players.end();
            ++itr)
        {
            Player* pPlayer =
                itr->getSource();

            if (!pPlayer ||
                !pPlayer->IsAlive())
            {
                continue;
            }

            if (!pPlayer->IsWithinDistInMap(
                m_creature,
                40.0f))
            {
                continue;
            }

            if (!m_creature->IsHostileTo(
                pPlayer))
            {
                continue;
            }

            // In a group, prefer someone other than
            // the current victim/tank.
            if (pVictim &&
                pPlayer->GetObjectGuid() ==
                pVictim->GetObjectGuid())
            {
                continue;
            }

            vTargets.push_back(
                pPlayer);
        }

        if (!vTargets.empty())
        {
            return vTargets[
                urand(
                    0,
                    vTargets.size() - 1)];
        }

        // Solo fallback:
        // if there is nobody else, Hex the current player.
        if (pVictim &&
            pVictim->IsPlayer() &&
            pVictim->IsAlive())
        {
            return static_cast<Player*>(
                pVictim);
        }

        return nullptr;
    }

    Player* SelectRandomPlayer()
    {
        std::vector<Player*> vTargets;

        Map::PlayerList const& players =
            m_creature->GetMap()->GetPlayers();

        for (Map::PlayerList::const_iterator itr =
            players.begin();
            itr != players.end();
            ++itr)
        {
            Player* pPlayer =
                itr->getSource();

            if (!pPlayer ||
                !pPlayer->IsAlive())
            {
                continue;
            }

            if (!pPlayer->IsWithinDistInMap(
                m_creature,
                40.0f))
            {
                continue;
            }

            if (!m_creature->IsHostileTo(
                pPlayer))
            {
                continue;
            }

            vTargets.push_back(
                pPlayer);
        }

        if (vTargets.empty())
            return nullptr;

        return vTargets[
            urand(
                0,
                vTargets.size() - 1)];
    }

    void UpdateAI(
        const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() ||
            !m_creature->GetVictim())
        {
            return;
        }

        // =====================================================
        // HEX
        // =====================================================

        if (m_uiHexTimer <= uiDiff)
        {
            if (Player* pTarget =
                SelectHexTarget())
            {
                if (DoCastSpellIfCan(
                    pTarget,
                    SPELL_HEX) ==
                    CAST_OK)
                {
                    m_uiHexTimer =
                        urand(18000, 24000);

                    return;
                }
            }

            // If she could not cast for whatever reason,
            // retry shortly instead of losing the cooldown.
            m_uiHexTimer = 1000;
        }
        else
        {
            m_uiHexTimer -= uiDiff;
        }

        // =====================================================
        // CURSE OF TUTEN'KASH
        // =====================================================

        if (m_uiCurseTimer <= uiDiff)
        {
            // Use a random player so this isn't always
            // just another tank debuff.
            if (Player* pTarget =
                SelectRandomPlayer())
            {
                if (DoCastSpellIfCan(
                    pTarget,
                    SPELL_CURSE_TUTENKASH) ==
                    CAST_OK)
                {
                    m_uiCurseTimer =
                        urand(22000, 28000);

                    return;
                }
            }

            m_uiCurseTimer = 1000;
        }
        else
        {
            m_uiCurseTimer -= uiDiff;
        }

        // =====================================================
        // SHADOW BOLT FILLER
        // =====================================================

        if (m_uiShadowBoltTimer <= uiDiff)
        {
            Unit* pVictim =
                m_creature->GetVictim();

            if (pVictim)
            {
                if (DoCastSpellIfCan(
                    pVictim,
                    SPELL_SHADOW_BOLT) ==
                    CAST_OK)
                {
                    m_uiShadowBoltTimer =
                        urand(3500, 5000);

                    return;
                }
            }

            m_uiShadowBoltTimer = 500;
        }
        else
        {
            m_uiShadowBoltTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_zaljira_hexweaver(
    Creature* pCreature)
{
    return new boss_zaljira_hexweaverAI(
        pCreature);
}

void AddSC_boss_zaljira_hexweaver()
{
    Script* pNewScript =
        new Script;

    pNewScript->Name =
        "boss_zaljira_hexweaver";

    pNewScript->GetAI =
        &GetAI_boss_zaljira_hexweaver;

    pNewScript->RegisterSelf();
}