#include "scriptPCH.h"

enum
{
    SPELL_EZ_THRO_DYNAMITE = 23000,
    SPELL_WAR_STOMP = 15593,
    SPELL_FRENZIED_RAGE = 3490
};

struct boss_gazzik_boomwrenchAI : public ScriptedAI
{
    boss_gazzik_boomwrenchAI(Creature* pCreature)
        : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiDynamiteTimer;

    void Reset() override
    {
        m_uiDynamiteTimer = urand(6000, 9000);
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
            Player* pPlayer = itr->getSource();

            if (!pPlayer || !pPlayer->IsAlive())
                continue;

            if (!pPlayer->IsWithinDistInMap(
                m_creature,
                40.0f))
            {
                continue;
            }

            if (!m_creature->IsHostileTo(pPlayer))
                continue;

            vTargets.push_back(pPlayer);
        }

        if (vTargets.empty())
            return nullptr;

        return vTargets[
            urand(0, vTargets.size() - 1)];
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->GetVictim())
            return;

        if (m_uiDynamiteTimer <= uiDiff)
        {
            if (Player* pTarget = SelectRandomPlayer())
            {
                m_creature->CastSpell(
                    pTarget,
                    SPELL_EZ_THRO_DYNAMITE,
                    false);
            }

            m_uiDynamiteTimer =
                urand(10000, 14000);
        }
        else
        {
            m_uiDynamiteTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

struct boss_the_pulverizerAI : public ScriptedAI
{
    boss_the_pulverizerAI(Creature* pCreature)
        : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiWarStompTimer;
    uint32 m_uiFrenziedRageTimer;

    void Reset() override
    {
        m_uiWarStompTimer =
            urand(18000, 22000);

        m_uiFrenziedRageTimer =
            urand(10000, 12000);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->GetVictim())
            return;

        if (m_uiWarStompTimer <= uiDiff)
        {
            m_creature->CastSpell(
                m_creature,
                SPELL_WAR_STOMP,
                false);

            m_uiWarStompTimer =
                urand(25000, 30000);
        }
        else
        {
            m_uiWarStompTimer -= uiDiff;
        }

        if (m_uiFrenziedRageTimer <= uiDiff)
        {
            m_creature->CastSpell(
                m_creature,
                SPELL_FRENZIED_RAGE,
                true);

            m_uiFrenziedRageTimer =
                urand(15000, 18000);
        }
        else
        {
            m_uiFrenziedRageTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_gazzik_boomwrench(
    Creature* pCreature)
{
    return new boss_gazzik_boomwrenchAI(
        pCreature);
}

CreatureAI* GetAI_boss_the_pulverizer(
    Creature* pCreature)
{
    return new boss_the_pulverizerAI(
        pCreature);
}

void AddSC_boss_gazzik_and_pulverizer()
{
    Script* pNewScript;

    pNewScript = new Script;

    pNewScript->Name =
        "boss_gazzik_boomwrench";

    pNewScript->GetAI =
        &GetAI_boss_gazzik_boomwrench;

    pNewScript->RegisterSelf();

    pNewScript = new Script;

    pNewScript->Name =
        "boss_the_pulverizer";

    pNewScript->GetAI =
        &GetAI_boss_the_pulverizer;

    pNewScript->RegisterSelf();
}