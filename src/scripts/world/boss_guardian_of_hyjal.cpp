#include "scriptPCH.h"

enum
{
    SPELL_GRASPING_ROOTS_MARK = 33542,
    SPELL_ROOT_VISUAL = 339,
    SPELL_SPLINTERING_BARK = 33543,

    NPC_ANCIENT_TREANT = 900501
};

struct boss_guardian_of_hyjalAI : public ScriptedAI
{
    boss_guardian_of_hyjalAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiRootsTimer;
    uint32 m_uiRootDetonateTimer;
    uint32 m_uiBarkTimer;
    uint32 m_uiEventCounter;

    ObjectGuid m_rootTargetGuid;
    bool m_bRootMarked;

    void Reset() override
    {
        std::list<Creature*> treants;
        GetCreatureListWithEntryInGrid(treants, m_creature, NPC_ANCIENT_TREANT, 200.0f);

        for (std::list<Creature*>::iterator itr = treants.begin(); itr != treants.end(); ++itr)
            (*itr)->ForcedDespawn();

        m_uiRootsTimer = 15000;
        m_uiRootDetonateTimer = 0;
        m_uiBarkTimer = 12000;
        m_uiEventCounter = 1;

        m_rootTargetGuid.Clear();
        m_bRootMarked = false;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        m_creature->SetInCombatWithZone();
    }

    Player* GetTankPlayer()
    {
        Unit* pVictim = m_creature->GetVictim();

        if (pVictim && pVictim->GetTypeId() == TYPEID_PLAYER)
            return (Player*)pVictim;

        return nullptr;
    }

    bool IsInTankRaid(Player* pPlayer)
    {
        if (!pPlayer)
            return false;

        Player* pTank = GetTankPlayer();

        if (!pTank)
            return false;

        Group* pTankGroup = pTank->GetGroup();

        // If the tank is not in a group/raid, only the tank counts.
        if (!pTankGroup)
            return pPlayer->GetObjectGuid() == pTank->GetObjectGuid();

        return pPlayer->GetGroup() == pTankGroup;
    }

    void SummonTreants()
    {
        m_creature->MonsterYell("Children of the ancient grove, rise!", LANG_UNIVERSAL, 0);

        float x = 4549.740723f;
        float y = -3964.096191f;
        float z = 943.331543f;
        float o = 4.356172f;

        for (uint8 i = 0; i < 3; ++i)
        {
            float sx = x;
            float sy = y;

            switch (i)
            {
            case 0:
                sx -= 3.0f;
                break;
            case 1:
                sx += 3.0f;
                break;
            case 2:
                sy += 3.0f;
                break;
            }

            if (Creature* pTreant = m_creature->SummonCreature(
                NPC_ANCIENT_TREANT,
                sx, sy, z, o,
                TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
                5 * MINUTE * IN_MILLISECONDS))
            {
                if (Unit* pVictim = m_creature->GetVictim())
                    pTreant->AI()->AttackStart(pVictim);
            }
        }
    }

    Player* SelectRootTarget()
    {
        std::vector<Player*> vTargets;

        Player* pTank = GetTankPlayer();
        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();

        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* pPlayer = itr->getSource();

            if (!pPlayer || !pPlayer->IsAlive())
                continue;

            if (!pPlayer->IsWithinDistInMap(m_creature, 80.0f))
                continue;

            // Only players in the tank's group/raid can be targeted.
            if (!IsInTankRaid(pPlayer))
                continue;

            // Never target the current tank.
            if (pTank && pPlayer->GetObjectGuid() == pTank->GetObjectGuid())
                continue;

            vTargets.push_back(pPlayer);
        }

        if (!vTargets.empty())
            return vTargets[urand(0, vTargets.size() - 1)];

        return nullptr;
    }

    void DetonateRoots(Player* pMarkedPlayer)
    {
        if (!pMarkedPlayer || !pMarkedPlayer->IsAlive())
            return;

        std::vector<Player*> vRootedPlayers;
        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();

        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* pPlayer = itr->getSource();

            if (!pPlayer || !pPlayer->IsAlive())
                continue;

            // Only the tank's group/raid can be hit by this mechanic.
            if (!IsInTankRaid(pPlayer))
                continue;

            if (pPlayer->IsWithinDistInMap(pMarkedPlayer, 10.0f))
                vRootedPlayers.push_back(pPlayer);
        }

        if (vRootedPlayers.empty())
            return;

        uint32 count = vRootedPlayers.size();
        uint32 damage = 800 + ((count - 1) * 800);

        for (std::vector<Player*>::iterator itr = vRootedPlayers.begin(); itr != vRootedPlayers.end(); ++itr)
        {
            Player* pPlayer = *itr;

            m_creature->DealDamage(pPlayer, damage, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NATURE, nullptr, false);
            pPlayer->CastSpell(pPlayer, SPELL_ROOT_VISUAL, true);
        }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_creature->GetHealthPercent() < 100.0f - m_uiEventCounter * 25.0f)
        {
            SummonTreants();
            ++m_uiEventCounter;
        }

        if (m_uiBarkTimer <= uiDiff)
        {
            Unit* pVictim = m_creature->GetVictim();

            if (pVictim)
                m_creature->CastSpell(pVictim, SPELL_SPLINTERING_BARK, true);

            m_uiBarkTimer = 18000;
        }
        else
            m_uiBarkTimer -= uiDiff;

        if (!m_bRootMarked)
        {
            if (m_uiRootsTimer <= uiDiff)
            {
                Player* pTarget = SelectRootTarget();

                if (pTarget)
                {
                    m_rootTargetGuid = pTarget->GetObjectGuid();
                    m_bRootMarked = true;
                    m_uiRootDetonateTimer = 10000;

                    m_creature->CastSpell(pTarget, SPELL_GRASPING_ROOTS_MARK, true);
                    m_creature->MonsterSay("The forest claims another!", LANG_UNIVERSAL, 0);
                }

                m_uiRootsTimer = 30000;
            }
            else
                m_uiRootsTimer -= uiDiff;
        }

        if (m_bRootMarked)
        {
            if (m_uiRootDetonateTimer <= uiDiff)
            {
                if (Player* pMarkedPlayer = m_creature->GetMap()->GetPlayer(m_rootTargetGuid))
                    DetonateRoots(pMarkedPlayer);

                m_rootTargetGuid.Clear();
                m_bRootMarked = false;
            }
            else
                m_uiRootDetonateTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_guardian_of_hyjal(Creature* pCreature)
{
    return new boss_guardian_of_hyjalAI(pCreature);
}

void AddSC_boss_guardian_of_hyjal()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_guardian_of_hyjal";
    pNewScript->GetAI = &GetAI_boss_guardian_of_hyjal;
    pNewScript->RegisterSelf();
}