#include "scriptPCH.h"

enum
{
    SPELL_BLIND = 2094,
    SPELL_CLEAVE = 11609,
    SPELL_KNOCKDOWN = 5164,
    SPELL_FRENZIED_RAGE = 3490
};

static float const RIPPER_ESCAPE_X = -7114.933594f;
static float const RIPPER_ESCAPE_Y = -3797.127686f;
static float const RIPPER_ESCAPE_Z = 8.369942f;

struct boss_ripper_razelAI : public ScriptedAI
{
    boss_ripper_razelAI(Creature* pCreature)
        : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiBlindTimer;
    uint32 m_uiCleaveTimer;
    uint32 m_uiKnockdownTimer;
    uint32 m_uiFrenzyTimer;

    uint32 m_uiSurrenderTimer;
    uint8 m_uiSurrenderStep;

    bool m_bLowHealthPhase;
    bool m_bSurrendered;

    ObjectGuid m_surrenderTargetGuid;

    void Reset() override
    {
        m_uiBlindTimer = urand(28000, 35000);
        m_uiCleaveTimer = 7000;
        m_uiKnockdownTimer = 14000;
        m_uiFrenzyTimer = 0;

        m_uiSurrenderTimer = 0;
        m_uiSurrenderStep = 0;

        m_bLowHealthPhase = false;
        m_bSurrendered = false;

        m_surrenderTargetGuid.Clear();

        m_creature->SetReactState(
            REACT_PASSIVE);

        m_creature->SetStandState(
            UNIT_STAND_STATE_STAND);

        m_creature->SetSheath(
            SHEATH_STATE_MELEE);
    }

    Player* GetCurrentPlayerVictim()
    {
        Unit* pVictim =
            m_creature->GetVictim();

        if (!pVictim ||
            pVictim->GetTypeId() != TYPEID_PLAYER)
        {
            return nullptr;
        }

        return (Player*)pVictim;
    }

    Player* SelectRandomArenaPlayer()
    {
        std::vector<Player*> vTargets;

        Player* pVictim =
            GetCurrentPlayerVictim();

        if (!pVictim)
            return nullptr;

        Group* pGroup =
            pVictim->GetGroup();

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

            if (!pGroup)
            {
                if (pPlayer->GetObjectGuid() !=
                    pVictim->GetObjectGuid())
                {
                    continue;
                }
            }
            else
            {
                if (pPlayer->GetGroup() !=
                    pGroup)
                {
                    continue;
                }
            }

            vTargets.push_back(
                pPlayer);
        }

        if (vTargets.empty())
            return nullptr;

        return vTargets[
            urand(0, vTargets.size() - 1)];
    }

    Player* SelectAnyNearbyPlayer()
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

            vTargets.push_back(
                pPlayer);
        }

        if (vTargets.empty())
            return nullptr;

        return vTargets[
            urand(0, vTargets.size() - 1)];
    }

    Player* SelectRandomNonVictimPlayer()
    {
        std::vector<Player*> vTargets;

        Player* pVictim =
            GetCurrentPlayerVictim();

        if (!pVictim)
            return nullptr;

        Group* pGroup =
            pVictim->GetGroup();

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

            if (pPlayer->GetObjectGuid() ==
                pVictim->GetObjectGuid())
            {
                continue;
            }

            if (!pGroup ||
                pPlayer->GetGroup() !=
                pGroup)
            {
                continue;
            }

            vTargets.push_back(
                pPlayer);
        }

        if (vTargets.empty())
            return nullptr;

        return vTargets[
            urand(0, vTargets.size() - 1)];
    }

    void StopSurrenderCombat()
    {
        m_creature->InterruptNonMeleeSpells(
            false);

        m_creature->AttackStop();

        m_creature->DeleteThreatList();

        m_creature->SetReactState(
            REACT_PASSIVE);
    }

    void BeginSurrender()
    {
        if (m_bSurrendered)
            return;

        // Pick the cheap-shot target before combat is cleared.
        // Prefer somebody from the active arena group. If the core has
        // already dropped our victim for any reason, fall back to any
        // nearby living player so the cheap shot still happens.
        Player* pTarget =
            SelectRandomArenaPlayer();

        if (!pTarget)
            pTarget = SelectAnyNearbyPlayer();

        if (pTarget)
        {
            m_surrenderTargetGuid =
                pTarget->GetObjectGuid();
        }

        m_bSurrendered = true;

        // Players may no longer attack him.
        m_creature->SetFlag(
            UNIT_FIELD_FLAGS,
            UNIT_FLAG_NON_ATTACKABLE);

        // Immediately kill his attack/threat state.
        StopSurrenderCombat();

        // Most importantly, remove the old chase movement.
        m_creature->GetMotionMaster()->Clear();
        m_creature->GetMotionMaster()->MoveIdle();

        // Put the weapon away during the surrender.
        m_creature->SetSheath(
            SHEATH_STATE_UNARMED);

        m_creature->MonsterSay(
            "Alright! ALRIGHT! I yield!",
            LANG_UNIVERSAL,
            0);

        // Use an actual stand state rather than an emote.
        // This reliably keeps him kneeling.
        m_creature->SetStandState(
            UNIT_STAND_STATE_KNEEL);

        m_uiSurrenderStep = 0;
        m_uiSurrenderTimer = 3000;
    }

    void UpdateSurrender(
        const uint32 uiDiff)
    {
        // While kneeling / preparing the cheap shot,
        // repeatedly make sure the core cannot restart
        // his old attack or chase.
        if (m_uiSurrenderStep < 2)
        {
            StopSurrenderCombat();

            m_creature->GetMotionMaster()->Clear();
            m_creature->GetMotionMaster()->MoveIdle();
        }

        if (m_uiSurrenderTimer > uiDiff)
        {
            m_uiSurrenderTimer -= uiDiff;
            return;
        }

        switch (m_uiSurrenderStep)
        {
        case 0:
        {
            // Stand back up but remain completely passive.
            m_creature->SetStandState(
                UNIT_STAND_STATE_STAND);

            m_creature->MonsterSay(
                "You have bested me... for now.",
                LANG_UNIVERSAL,
                0);

            m_uiSurrenderTimer = 2000;
            ++m_uiSurrenderStep;
            break;
        }

        case 1:
        {
            // One last cheap shot before he bolts.
            Player* pTarget = nullptr;

            if (!m_surrenderTargetGuid.IsEmpty())
            {
                pTarget =
                    m_creature->GetMap()->GetPlayer(
                        m_surrenderTargetGuid);
            }

            // If the original target vanished, grab another nearby player.
            if (!pTarget ||
                !pTarget->IsAlive() ||
                !pTarget->IsWithinDistInMap(
                    m_creature,
                    40.0f))
            {
                pTarget =
                    SelectAnyNearbyPlayer();

                if (pTarget)
                {
                    m_surrenderTargetGuid =
                        pTarget->GetObjectGuid();
                }
            }

            if (pTarget)
            {
                m_creature->SetFacingToObject(
                    pTarget);

                m_creature->MonsterSay(
                    "One more thing...",
                    LANG_UNIVERSAL,
                    0);

                // Triggered cast so surrender/combat state cannot block it.
                m_creature->CastSpell(
                    pTarget,
                    SPELL_KNOCKDOWN,
                    true);
            }

            // Give the knockdown a moment to visibly land before he runs.
            m_uiSurrenderTimer = 700;
            ++m_uiSurrenderStep;
            break;
        }

        case 2:
        {
            // Cheap shot is finished. The encounter is now
            // completely over.

            StopSurrenderCombat();

            // Do NOT change faction here.
            // The arena controller owns Ripper's faction exactly like
            // every other arena boss. Changing him to Gadgetzan inside
            // this boss script can immediately flag players At War with
            // Gadgetzan when combat begins or while combat state lingers.
            m_creature->SetReactState(
                REACT_PASSIVE);

            // Keep him non-attackable while escaping.
            m_creature->SetFlag(
                UNIT_FIELD_FLAGS,
                UNIT_FLAG_NON_ATTACKABLE);

            // Make absolutely sure he is standing.
            m_creature->SetStandState(
                UNIT_STAND_STATE_STAND);

            // Put his weapon away so he no longer looks
            // like he is trying to continue the fight.
            m_creature->SetSheath(
                SHEATH_STATE_UNARMED);

            // Throw away any old chase/idle movement before
            // issuing the escape movement.
            m_creature->GetMotionMaster()->Clear();

            // He has just cheated, so he should BOLT rather than casually
            // stroll away. Force run movement and give him a noticeable
            // run-speed boost for the escape.
            m_creature->SetWalk(false);
            m_creature->SetSpeedRate(
                MOVE_RUN,
                1.6f);

            m_creature->GetMotionMaster()->MovePoint(
                1,
                RIPPER_ESCAPE_X,
                RIPPER_ESCAPE_Y,
                RIPPER_ESCAPE_Z);

            // Give him plenty of time to physically run away.
            m_uiSurrenderTimer = 10000;
            ++m_uiSurrenderStep;
            break;
        }

        case 3:
        {
            m_creature->AddObjectToRemoveList();

            ++m_uiSurrenderStep;
            break;
        }

        default:
            break;
        }
    }

    void DamageTaken(
        Unit* /*pDealer*/,
        uint32& uiDamage) override
    {
        // Once the surrender sequence begins,
        // he cannot take any more damage.
        if (m_bSurrendered)
        {
            uiDamage = 0;
            return;
        }

        uint32 uiSurrenderHealth =
            uint32(
                m_creature->GetMaxHealth() *
                0.05f);

        if (uiSurrenderHealth < 1)
            uiSurrenderHealth = 1;

        uint32 uiCurrentHealth =
            m_creature->GetHealth();

        if (uiCurrentHealth <=
            uiSurrenderHealth)
        {
            uiDamage = 0;

            m_creature->SetHealth(
                uiSurrenderHealth);

            BeginSurrender();
            return;
        }

        // Stop the incoming hit from pushing him through
        // the 5% threshold or killing him.
        if (uiDamage >=
            (uiCurrentHealth -
                uiSurrenderHealth))
        {
            uiDamage = 0;

            m_creature->SetHealth(
                uiSurrenderHealth);

            BeginSurrender();
            return;
        }
    }

    void UpdateAI(
        const uint32 uiDiff) override
    {
        // Surrender completely replaces normal combat AI.
        if (m_bSurrendered)
        {
            UpdateSurrender(
                uiDiff);

            return;
        }

        Unit* pVictim =
            m_creature->GetVictim();

        if (!pVictim ||
            !pVictim->IsAlive() ||
            pVictim->GetTypeId() != TYPEID_PLAYER)
        {
            return;
        }

        // =====================================================
        // PHASE 2 - 30%
        //
        // No more Blind, Cleave or normal Knockdown.
        // Only Frenzied Rage approximately every 12 seconds
        // plus normal melee attacks.
        // =====================================================

        if (!m_bLowHealthPhase &&
            m_creature->GetHealthPercent() <= 30.0f)
        {
            m_bLowHealthPhase = true;

            m_creature->MonsterYell(
                "Alright! No more playin' fair!",
                LANG_UNIVERSAL,
                0);

            m_creature->CastSpell(
                m_creature,
                SPELL_FRENZIED_RAGE,
                true);

            m_uiFrenzyTimer =
                urand(11000, 14000);
        }

        if (m_bLowHealthPhase)
        {
            if (m_uiFrenzyTimer <= uiDiff)
            {
                m_creature->CastSpell(
                    m_creature,
                    SPELL_FRENZIED_RAGE,
                    true);

                m_uiFrenzyTimer =
                    urand(11000, 14000);
            }
            else
            {
                m_uiFrenzyTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
            return;
        }

        // =====================================================
        // PHASE 1 - ABOVE 30%
        // =====================================================

        // Pocket Sand / Blind.
        if (m_uiBlindTimer <= uiDiff)
        {
            Player* pTarget =
                SelectRandomNonVictimPlayer();

            if (!pTarget)
            {
                pTarget =
                    GetCurrentPlayerVictim();
            }

            if (pTarget)
            {
                m_creature->MonsterSay(
                    "Pocket sand! Ha-sha!",
                    LANG_UNIVERSAL,
                    0);

                m_creature->CastSpell(
                    pTarget,
                    SPELL_BLIND,
                    false);
            }

            m_uiBlindTimer =
                urand(30000, 40000);
        }
        else
        {
            m_uiBlindTimer -= uiDiff;
        }

        // Cleave.
        if (m_uiCleaveTimer <= uiDiff)
        {
            if (Unit* pCurrentVictim =
                m_creature->GetVictim())
            {
                m_creature->CastSpell(
                    pCurrentVictim,
                    SPELL_CLEAVE,
                    false);
            }

            m_uiCleaveTimer =
                urand(7000, 10000);
        }
        else
        {
            m_uiCleaveTimer -= uiDiff;
        }

        // Normal combat Knockdown.
        if (m_uiKnockdownTimer <= uiDiff)
        {
            if (Unit* pCurrentVictim =
                m_creature->GetVictim())
            {
                m_creature->CastSpell(
                    pCurrentVictim,
                    SPELL_KNOCKDOWN,
                    false);
            }

            m_uiKnockdownTimer =
                urand(14000, 18000);
        }
        else
        {
            m_uiKnockdownTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_ripper_razel(
    Creature* pCreature)
{
    return new boss_ripper_razelAI(
        pCreature);
}

void AddSC_boss_ripper_razel()
{
    Script* pNewScript;

    pNewScript =
        new Script;

    pNewScript->Name =
        "boss_ripper_razel";

    pNewScript->GetAI =
        &GetAI_boss_ripper_razel;

    pNewScript->RegisterSelf();
}