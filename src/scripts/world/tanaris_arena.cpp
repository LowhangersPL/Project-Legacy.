#include "scriptPCH.h"

enum
{
    QUEST_ARENA_ROUND_1 = 900250,
    QUEST_ARENA_ROUND_2 = 900251,
    QUEST_ARENA_ROUND_3 = 900252,
    QUEST_ARENA_ROUND_4 = 900253,
    QUEST_ARENA_ROUND_5 = 900254,

    NPC_KRIXEL_COPPERFIST = 1000050,
    NPC_RIPPER_RAZEL = 1000051,
    NPC_SKARRA_BROODMOTHER = 1000052,
    NPC_SKARRA_BROODLING = 1000053,
    NPC_ARENA_SPECTATOR_FEMALE = 1000059,

    NPC_GAZZIK_BOOMWRENCH = 1000060,
    NPC_THE_PULVERIZER = 1000061,
    NPC_ZALJIRA_HEXWEAVER = 1000062,
    NPC_GORMAK_UNBROKEN = 1000063,

    GUID_FEMALE_SPECTATOR_1 = 2531096,
    GUID_FEMALE_SPECTATOR_2 = 2531097,

    FACTION_GADGETZAN = 475,
    FACTION_ARENA_FIGHTER = 900
};

enum ArenaState
{
    ARENA_IDLE = 0,
    ARENA_INTRO,
    ARENA_ACTIVE,
    ARENA_VICTORY
};

static float const ARENA_CENTER_X = -7137.246582f;
static float const ARENA_CENTER_Y = -3787.229980f;
static float const ARENA_CENTER_Z = 9.014814f;

static float const ARENA_RADIUS = 18.5f;

// Gives grouped players time to accept the shared quest and enter the cage
// before the round introduction begins. No arena boundary failure occurs
// during this grace period because the round is still in ARENA_INTRO.
static uint32 const ARENA_GROUP_ACCEPT_GRACE_MS = 15000;

// Larger than the actual player boundary so nearby guards,
// bruisers and other Gadgetzan NPCs do not interfere.
static float const ARENA_NPC_SUPPRESSION_RADIUS = 45.0f;

// Round 1
static float const RIPPER_SPAWN_X = -7137.246582f;
static float const RIPPER_SPAWN_Y = -3787.229980f;
static float const RIPPER_SPAWN_Z = 9.014814f;
static float const RIPPER_SPAWN_O = 0.0f;

// Round 2
static float const SKARRA_SPAWN_X = -7137.246582f;
static float const SKARRA_SPAWN_Y = -3787.229980f;
static float const SKARRA_SPAWN_Z = 9.014814f;
static float const SKARRA_SPAWN_O = 0.0f;

// Round 3
//
// Spawn them apart so the goblin and robot do not appear
// directly inside one another.
static float const GAZZIK_SPAWN_X = -7133.5f;
static float const GAZZIK_SPAWN_Y = -3787.0f;
static float const GAZZIK_SPAWN_Z = 9.014814f;
static float const GAZZIK_SPAWN_O = 3.14f;

static float const PULVERIZER_SPAWN_X = -7140.5f;
static float const PULVERIZER_SPAWN_Y = -3787.0f;
static float const PULVERIZER_SPAWN_Z = 9.014814f;
static float const PULVERIZER_SPAWN_O = 0.0f;

// Round 4
static float const ZALJIRA_SPAWN_X = -7137.246582f;
static float const ZALJIRA_SPAWN_Y = -3787.229980f;
static float const ZALJIRA_SPAWN_Z = 9.014814f;
static float const ZALJIRA_SPAWN_O = 0.0f;

// Round 5
static float const GORMAK_SPAWN_X = -7137.246582f;
static float const GORMAK_SPAWN_Y = -3787.229980f;
static float const GORMAK_SPAWN_Z = 9.014814f;
static float const GORMAK_SPAWN_O = 0.0f;

struct ArenaSuppressedNpc
{
    ObjectGuid guid;
    ReactStates reactState;
    bool hadPassiveFlag;

    ArenaSuppressedNpc(
        ObjectGuid const& npcGuid,
        ReactStates state,
        bool passiveFlag)
        : guid(npcGuid),
        reactState(state),
        hadPassiveFlag(passiveFlag)
    {
    }
};

struct npc_krixel_copperfistAI : public ScriptedAI
{
    npc_krixel_copperfistAI(Creature* pCreature)
        : ScriptedAI(pCreature)
    {
        Reset();
    }

    ArenaState m_eArenaState;

    uint32 m_uiActiveQuest;
    uint32 m_uiIntroTimer;
    uint32 m_uiBoundaryTimer;
    uint32 m_uiVictoryTimer;

    uint8 m_uiIntroStep;

    ObjectGuid m_ownerGuid;

    // Primary arena opponent.
    //
    // Round 1 = Ripper
    // Round 2 = Skarra
    // Round 3 = Gazzik
    // Round 4 = Zal'jira
    // Round 5 = Gormak
    ObjectGuid m_bossGuid;

    // Used only by Round 3 for The Pulverizer.
    ObjectGuid m_secondBossGuid;

    bool m_bRound3GazzikDefeated;
    bool m_bRound3PulverizerDefeated;

    std::vector<ArenaSuppressedNpc> m_suppressedNpcs;

    void Reset() override
    {
        RestoreArenaNpcs();

        m_eArenaState = ARENA_IDLE;

        m_uiActiveQuest = 0;
        m_uiIntroTimer = 0;
        m_uiBoundaryTimer = 1000;
        m_uiVictoryTimer = 0;

        m_uiIntroStep = 0;

        m_ownerGuid.Clear();
        m_bossGuid.Clear();
        m_secondBossGuid.Clear();

        m_bRound3GazzikDefeated = false;
        m_bRound3PulverizerDefeated = false;
    }

    void AttackStart(Unit* /*pWho*/) override
    {
        // Krixel is an arena promoter and quest giver.
        // He should never initiate combat.
    }

    void KeepKrixelOutOfCombat()
    {
        m_creature->SetReactState(
            REACT_PASSIVE);

        if (!m_creature->IsInCombat())
            return;

        m_creature->AttackStop();
        m_creature->DeleteThreatList();
        m_creature->CombatStop(true);

        m_creature->GetMotionMaster()->Clear();
        m_creature->GetMotionMaster()->MoveIdle();
    }

    Player* GetArenaOwner()
    {
        if (m_ownerGuid.IsEmpty())
            return nullptr;

        return m_creature->GetMap()->GetPlayer(
            m_ownerGuid);
    }

    Creature* GetArenaBoss()
    {
        if (m_bossGuid.IsEmpty())
            return nullptr;

        return m_creature->GetMap()->GetCreature(
            m_bossGuid);
    }

    Creature* GetSecondArenaBoss()
    {
        if (m_secondBossGuid.IsEmpty())
            return nullptr;

        return m_creature->GetMap()->GetCreature(
            m_secondBossGuid);
    }

    bool IsArenaOpponent(Creature* pCreature)
    {
        if (!pCreature)
            return false;

        ObjectGuid guid =
            pCreature->GetObjectGuid();

        if (!m_bossGuid.IsEmpty() &&
            guid == m_bossGuid)
        {
            return true;
        }

        if (!m_secondBossGuid.IsEmpty() &&
            guid == m_secondBossGuid)
        {
            return true;
        }

        return false;
    }

    Creature* GetArenaGoblinFemale(uint32 uiDbGuid)
    {
        std::list<Creature*> creatures;

        MaNGOS::AnyUnitInObjectRangeCheck check(
            m_creature,
            100.0f);

        MaNGOS::CreatureListSearcher<
            MaNGOS::AnyUnitInObjectRangeCheck> searcher(
                creatures,
                check);

        Cell::VisitGridObjects(
            m_creature,
            searcher,
            100.0f);

        for (std::list<Creature*>::iterator itr =
            creatures.begin();
            itr != creatures.end();
            ++itr)
        {
            Creature* pCreature = *itr;

            if (!pCreature)
                continue;

            // These are the two specific goblin women placed
            // beside the arena. We match their DB GUIDs directly
            // so no random spectator can ever be selected.
            if (pCreature->GetGUIDLow() == uiDbGuid)
                return pCreature;
        }

        return nullptr;
    }

    bool IsInsideArena(Player* pPlayer)
    {
        if (!pPlayer)
            return false;

        float dx =
            pPlayer->GetPositionX() -
            ARENA_CENTER_X;

        float dy =
            pPlayer->GetPositionY() -
            ARENA_CENTER_Y;

        return (dx * dx + dy * dy) <=
            (ARENA_RADIUS * ARENA_RADIUS);
    }

    bool IsArenaParticipant(Player* pPlayer)
    {
        if (!pPlayer)
            return false;

        Player* pOwner = GetArenaOwner();

        if (!pOwner)
            return false;

        if (pPlayer->GetObjectGuid() ==
            pOwner->GetObjectGuid())
        {
            return true;
        }

        Group* pOwnerGroup =
            pOwner->GetGroup();

        if (!pOwnerGroup)
            return false;

        return pPlayer->GetGroup() ==
            pOwnerGroup;
    }

    bool IsSameArenaGroup(Player* pPlayer)
    {
        return IsArenaParticipant(pPlayer);
    }

    Player* GetRandomArenaParticipant()
    {
        std::vector<Player*> participants;

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
                !pPlayer->IsAlive() ||
                !IsArenaParticipant(pPlayer))
            {
                continue;
            }

            participants.push_back(
                pPlayer);
        }

        if (participants.empty())
            return nullptr;

        return participants[
            urand(0, participants.size() - 1)];
    }

    Creature* GetRandomArenaSpectator()
    {
        std::list<Creature*> spectators;

        GetCreatureListWithEntryInGrid(
            spectators,
            m_creature,
            NPC_ARENA_SPECTATOR_FEMALE,
            100.0f);

        if (spectators.empty())
            return nullptr;

        std::vector<Creature*> validSpectators;

        for (std::list<Creature*>::iterator itr =
            spectators.begin();
            itr != spectators.end();
            ++itr)
        {
            Creature* pSpectator = *itr;

            if (!pSpectator ||
                !pSpectator->IsAlive())
            {
                continue;
            }

            // The two named goblin women have their own scripted reactions.
            // Never use either of them for generic crowd taunts.
            uint32 uiDbGuid = pSpectator->GetGUIDLow();

            if (uiDbGuid == GUID_FEMALE_SPECTATOR_1 ||
                uiDbGuid == GUID_FEMALE_SPECTATOR_2)
            {
                continue;
            }

            validSpectators.push_back(
                pSpectator);
        }

        if (validSpectators.empty())
            return nullptr;

        return validSpectators[
            urand(0, validSpectators.size() - 1)];
    }

    void DoRandomCrowdTaunt(
        char const* pText)
    {
        Creature* pSpectator =
            GetRandomArenaSpectator();

        if (!pSpectator)
            return;

        if (Player* pTarget =
            GetRandomArenaParticipant())
        {
            pSpectator->SetFacingToObject(
                pTarget);
        }

        pSpectator->MonsterSay(
            pText,
            LANG_UNIVERSAL,
            0);
    }

    void PurgeAllArenaOpponents()
    {
        // Every arena boss is a TEMPSUMMON_MANUAL_DESPAWN.
        // Only Krixel owns their lifetime. There is no automatic
        // corpse/timer respawn path left for any round.

        Creature* pBoss =
            GetArenaBoss();

        Creature* pSecondBoss =
            GetSecondArenaBoss();

        if (pBoss)
        {
            pBoss->CombatStop(true);
            pBoss->DeleteThreatList();
            pBoss->AttackStop();
            pBoss->AddObjectToRemoveList();
        }

        if (pSecondBoss)
        {
            pSecondBoss->CombatStop(true);
            pSecondBoss->DeleteThreatList();
            pSecondBoss->AttackStop();
            pSecondBoss->AddObjectToRemoveList();
        }

        // Broodlings are also temporary arena creatures.
        DespawnSkarraBroodlings();

        // Clear controller references only AFTER the summons have been removed.
        m_bossGuid.Clear();
        m_secondBossGuid.Clear();

        m_bRound3GazzikDefeated = false;
        m_bRound3PulverizerDefeated = false;
    }

    void DespawnSkarraBroodlings()
    {
        std::list<Creature*> broodlings;

        GetCreatureListWithEntryInGrid(
            broodlings,
            m_creature,
            NPC_SKARRA_BROODLING,
            100.0f);

        for (std::list<Creature*>::iterator itr =
            broodlings.begin();
            itr != broodlings.end();
            ++itr)
        {
            Creature* pBroodling = *itr;

            if (!pBroodling)
                continue;

            pBroodling->AddObjectToRemoveList();
        }
    }

    void DespawnArenaBoss()
    {
        PurgeAllArenaOpponents();
    }

    void SuppressArenaNpcs()
    {
        if (!m_suppressedNpcs.empty())
            return;

        std::list<Creature*> creatures;

        MaNGOS::AnyUnitInObjectRangeCheck check(
            m_creature,
            ARENA_NPC_SUPPRESSION_RADIUS);

        MaNGOS::CreatureListSearcher<
            MaNGOS::AnyUnitInObjectRangeCheck> searcher(
                creatures,
                check);

        Cell::VisitGridObjects(
            m_creature,
            searcher,
            ARENA_NPC_SUPPRESSION_RADIUS);

        for (std::list<Creature*>::iterator itr =
            creatures.begin();
            itr != creatures.end();
            ++itr)
        {
            Creature* pCreature = *itr;

            if (!pCreature)
                continue;

            // Never suppress Krixel himself.
            if (pCreature == m_creature)
                continue;

            // Never suppress an arena opponent.
            //
            // This matters for Round 3 because there are two.
            if (IsArenaOpponent(pCreature))
                continue;

            // Only Gadgetzan NPCs.
            if (pCreature->GetFactionTemplateId() !=
                FACTION_GADGETZAN)
            {
                continue;
            }

            bool hadPassiveFlag =
                pCreature->HasFlag(
                    UNIT_FIELD_FLAGS,
                    UNIT_FLAG_PASSIVE);

            m_suppressedNpcs.push_back(
                ArenaSuppressedNpc(
                    pCreature->GetObjectGuid(),
                    pCreature->GetReactState(),
                    hadPassiveFlag));

            pCreature->CombatStop(true);
            pCreature->DeleteThreatList();

            pCreature->SetReactState(
                REACT_PASSIVE);

            pCreature->SetFlag(
                UNIT_FIELD_FLAGS,
                UNIT_FLAG_PASSIVE);
        }
    }

    void KeepArenaNpcsOutOfCombat()
    {
        for (std::vector<ArenaSuppressedNpc>::iterator itr =
            m_suppressedNpcs.begin();
            itr != m_suppressedNpcs.end();
            ++itr)
        {
            Creature* pCreature =
                m_creature->GetMap()->GetCreature(
                    itr->guid);

            if (!pCreature)
                continue;

            pCreature->SetReactState(
                REACT_PASSIVE);

            pCreature->SetFlag(
                UNIT_FIELD_FLAGS,
                UNIT_FLAG_PASSIVE);

            if (pCreature->IsInCombat())
            {
                pCreature->CombatStop(true);
                pCreature->DeleteThreatList();
            }
        }
    }

    void RestoreArenaNpcs()
    {
        if (m_suppressedNpcs.empty())
            return;

        for (std::vector<ArenaSuppressedNpc>::iterator itr =
            m_suppressedNpcs.begin();
            itr != m_suppressedNpcs.end();
            ++itr)
        {
            Creature* pCreature =
                m_creature->GetMap()->GetCreature(
                    itr->guid);

            if (!pCreature)
                continue;

            pCreature->CombatStop(true);
            pCreature->DeleteThreatList();

            if (!itr->hadPassiveFlag)
            {
                pCreature->RemoveFlag(
                    UNIT_FIELD_FLAGS,
                    UNIT_FLAG_PASSIVE);
            }
            else
            {
                pCreature->SetFlag(
                    UNIT_FIELD_FLAGS,
                    UNIT_FLAG_PASSIVE);
            }

            pCreature->SetReactState(
                itr->reactState);
        }

        m_suppressedNpcs.clear();
    }

    void ResetArena()
    {
        // Clear players first so no lingering hostile relationship can
        // carry over when Gadgetzan NPCs are restored.
        StopArenaParticipantCombat();

        RestoreArenaNpcs();

        // Only remove the arena creatures we actually summoned.
        PurgeAllArenaOpponents();

        m_eArenaState = ARENA_IDLE;

        m_uiActiveQuest = 0;
        m_uiIntroTimer = 0;
        m_uiBoundaryTimer = 1000;
        m_uiVictoryTimer = 0;

        m_uiIntroStep = 0;

        m_ownerGuid.Clear();

        m_bRound3GazzikDefeated = false;
        m_bRound3PulverizerDefeated = false;
    }

    void FailAllActiveParticipants()
    {
        if (!m_uiActiveQuest)
            return;

        Map::PlayerList const& players =
            m_creature->GetMap()->GetPlayers();

        for (Map::PlayerList::const_iterator itr =
            players.begin();
            itr != players.end();
            ++itr)
        {
            Player* pPlayer =
                itr->getSource();

            if (!pPlayer)
                continue;

            if (!IsArenaParticipant(pPlayer))
                continue;

            if (pPlayer->GetQuestStatus(
                m_uiActiveQuest) ==
                QUEST_STATUS_INCOMPLETE)
            {
                pPlayer->FailQuest(
                    m_uiActiveQuest);
            }
        }
    }

    void StopArenaParticipantCombat()
    {
        if (m_ownerGuid.IsEmpty())
            return;

        Map::PlayerList const& players =
            m_creature->GetMap()->GetPlayers();

        for (Map::PlayerList::const_iterator itr =
            players.begin();
            itr != players.end();
            ++itr)
        {
            Player* pPlayer =
                itr->getSource();

            if (!pPlayer)
                continue;

            if (!IsArenaParticipant(pPlayer))
                continue;

            // Break every remaining combat link before an arena opponent
            // becomes friendly, despawns, or the Gadgetzan NPCs are restored.
            pPlayer->CombatStop(true);
        }
    }

    bool CheckArenaParticipants()
    {
        uint32 uiLivingParticipants = 0;

        Map::PlayerList const& players =
            m_creature->GetMap()->GetPlayers();

        for (Map::PlayerList::const_iterator itr =
            players.begin();
            itr != players.end();
            ++itr)
        {
            Player* pPlayer =
                itr->getSource();

            if (!pPlayer)
                continue;

            if (!IsArenaParticipant(pPlayer))
                continue;

            if (pPlayer->GetQuestStatus(
                m_uiActiveQuest) !=
                QUEST_STATUS_INCOMPLETE)
            {
                continue;
            }

            if (!IsInsideArena(pPlayer))
            {
                pPlayer->FailQuest(
                    m_uiActiveQuest);

                continue;
            }

            if (pPlayer->IsAlive())
                ++uiLivingParticipants;
        }

        return uiLivingParticipants > 0;
    }

    // =========================================================
    // ROUND 1 - RIPPER RAZEL
    // =========================================================

    void StartRoundOne(Player* pPlayer)
    {
        if (!pPlayer)
            return;

        if (m_eArenaState != ARENA_IDLE)
            return;

        // Hard cleanup before every new bout. This prevents any
        // opponent from an earlier round appearing in this one.
        PurgeAllArenaOpponents();

        m_ownerGuid =
            pPlayer->GetObjectGuid();

        m_uiActiveQuest =
            QUEST_ARENA_ROUND_1;

        m_secondBossGuid.Clear();

        Creature* pRipper =
            m_creature->SummonCreature(
                NPC_RIPPER_RAZEL,
                RIPPER_SPAWN_X,
                RIPPER_SPAWN_Y,
                RIPPER_SPAWN_Z,
                RIPPER_SPAWN_O,
                TEMPSUMMON_MANUAL_DESPAWN,
                0);

        if (!pRipper)
        {
            m_ownerGuid.Clear();
            m_uiActiveQuest = 0;
            return;
        }

        m_bossGuid =
            pRipper->GetObjectGuid();

        // Never place an arena opponent on Gadgetzan's reputation faction.
        // It stays non-attackable during the intro, so faction 900 is safe
        // and cannot toggle players At War with Gadgetzan.
        pRipper->SetFactionTemplateId(
            FACTION_ARENA_FIGHTER);

        pRipper->SetReactState(
            REACT_PASSIVE);

        pRipper->SetFlag(
            UNIT_FIELD_FLAGS,
            UNIT_FLAG_NON_ATTACKABLE);

        pRipper->CombatStop(true);

        m_eArenaState = ARENA_INTRO;
        m_uiIntroStep = 0;
        m_uiIntroTimer = ARENA_GROUP_ACCEPT_GRACE_MS;
    }

    void BeginRoundOneCombat()
    {
        Player* pOwner =
            GetArenaOwner();

        Creature* pRipper =
            GetArenaBoss();

        if (!pOwner || !pRipper)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        if (!IsInsideArena(pOwner))
        {
            if (pOwner->GetQuestStatus(
                QUEST_ARENA_ROUND_1) ==
                QUEST_STATUS_INCOMPLETE)
            {
                pOwner->FailQuest(
                    QUEST_ARENA_ROUND_1);
            }

            ResetArena();
            return;
        }

        SuppressArenaNpcs();

        pRipper->SetFactionTemplateId(
            FACTION_ARENA_FIGHTER);

        pRipper->SetReactState(
            REACT_PASSIVE);

        pRipper->RemoveFlag(
            UNIT_FIELD_FLAGS,
            UNIT_FLAG_NON_ATTACKABLE);

        pRipper->SetInCombatWith(
            pOwner);

        pOwner->SetInCombatWith(
            pRipper);

        pRipper->Attack(
            pOwner,
            true);

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
                !pPlayer->IsAlive() ||
                !IsArenaParticipant(pPlayer))
            {
                continue;
            }

            pRipper->SetInCombatWith(
                pPlayer);

            pPlayer->SetInCombatWith(
                pRipper);
        }

        m_eArenaState = ARENA_ACTIVE;
        m_uiBoundaryTimer = 1000;
    }

    void UpdateRoundOneIntro(
        const uint32 uiDiff)
    {
        if (m_uiIntroTimer > uiDiff)
        {
            m_uiIntroTimer -= uiDiff;
            return;
        }

        Player* pOwner =
            GetArenaOwner();

        Creature* pRipper =
            GetArenaBoss();

        if (!pOwner || !pRipper)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        Creature* pFemaleOne =
            GetArenaGoblinFemale(
                GUID_FEMALE_SPECTATOR_1);

        Creature* pFemaleTwo =
            GetArenaGoblinFemale(
                GUID_FEMALE_SPECTATOR_2);

        switch (m_uiIntroStep)
        {
        case 0:
        {
            m_creature->MonsterSay(
                "First into the cage, straight outta Lost Rigger Cove... \"Ripper\" Razel!",
                LANG_UNIVERSAL,
                0);

            m_uiIntroTimer = 2500;
            ++m_uiIntroStep;
            break;
        }

        case 1:
        {
            if (pFemaleOne)
            {
                pFemaleOne->MonsterSay(
                    "RIIIIIPPER!",
                    LANG_UNIVERSAL,
                    0);

                pFemaleOne->HandleEmote(
                    EMOTE_ONESHOT_CHEER);
            }

            m_uiIntroTimer = 1500;
            ++m_uiIntroStep;
            break;
        }

        case 2:
        {
            if (pFemaleTwo)
            {
                pFemaleTwo->MonsterSay(
                    "WE LOVE YOU RIPPER!",
                    LANG_UNIVERSAL,
                    0);

                pFemaleTwo->HandleEmote(
                    EMOTE_ONESHOT_CHEER);
            }

            m_uiIntroTimer = 1500;
            ++m_uiIntroStep;
            break;
        }

        case 3:
        {
            if (pFemaleOne)
            {
                pRipper->SetFacingToObject(
                    pFemaleOne);
            }

            pRipper->HandleEmote(
                EMOTE_ONESHOT_FLEX);

            m_uiIntroTimer = 2000;
            ++m_uiIntroStep;
            break;
        }

        case 4:
        {
            if (pFemaleOne)
            {
                pRipper->SetFacingToObject(
                    pFemaleOne);
            }

            pRipper->HandleEmote(
                EMOTE_ONESHOT_KISS);

            m_uiIntroTimer = 1500;
            ++m_uiIntroStep;
            break;
        }

        case 5:
        {
            if (pFemaleOne)
            {
                pFemaleOne->MonsterSay(
                    "HE JUST BLEW ME A KISS!",
                    LANG_UNIVERSAL,
                    0);
            }

            m_uiIntroTimer = 1200;
            ++m_uiIntroStep;
            break;
        }

        case 6:
        {
            if (pFemaleTwo)
            {
                pFemaleTwo->MonsterSay(
                    "No! He just blew ME a kiss!",
                    LANG_UNIVERSAL,
                    0);
            }

            m_uiIntroTimer = 1800;
            ++m_uiIntroStep;
            break;
        }

        case 7:
        {
            pRipper->SetFacingToObject(
                pOwner);

            pRipper->MonsterSay(
                "Right... where were we?",
                LANG_UNIVERSAL,
                0);

            m_uiIntroTimer = 1500;
            ++m_uiIntroStep;
            break;
        }

        case 8:
        {
            BeginRoundOneCombat();
            break;
        }

        default:
            break;
        }
    }

    void HandleRoundOneVictory()
    {
        if (m_eArenaState != ARENA_ACTIVE)
            return;

        Player* pOwner =
            GetArenaOwner();

        Creature* pRipper =
            GetArenaBoss();

        if (!pOwner || !pRipper)
        {
            ResetArena();
            return;
        }

        m_eArenaState =
            ARENA_VICTORY;

        // Ripper later returns to Gadgetzan faction for his escape.
        // Drop all player combat links BEFORE that happens so the
        // encounter can never toggle Gadgetzan to At War.
        StopArenaParticipantCombat();

        pOwner->RewardPlayerAndGroupAtEvent(
            NPC_RIPPER_RAZEL,
            pRipper);

        // Give the surrender, kneel, cheap shot and escape
        // enough time to finish.
        m_uiVictoryTimer = 16000;
    }

    void UpdateActiveRoundOne(
        const uint32 uiDiff)
    {
        Player* pOwner =
            GetArenaOwner();

        Creature* pRipper =
            GetArenaBoss();

        if (!pOwner)
        {
            ResetArena();
            return;
        }

        if (!pRipper)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        KeepArenaNpcsOutOfCombat();

        if (pRipper->GetHealthPercent() <=
            5.0f)
        {
            HandleRoundOneVictory();
            return;
        }

        if (m_uiBoundaryTimer > uiDiff)
        {
            m_uiBoundaryTimer -= uiDiff;
            return;
        }

        m_uiBoundaryTimer = 1000;

        if (!CheckArenaParticipants())
        {
            FailAllActiveParticipants();
            ResetArena();
        }
    }

    // =========================================================
    // ROUND 2 - SKARRA THE BROODMOTHER
    // =========================================================

    void StartRoundTwo(Player* pPlayer)
    {
        if (!pPlayer)
            return;

        if (m_eArenaState != ARENA_IDLE)
            return;

        // Hard cleanup before every new bout. This prevents any
        // opponent from an earlier round appearing in this one.
        PurgeAllArenaOpponents();

        m_ownerGuid =
            pPlayer->GetObjectGuid();

        m_uiActiveQuest =
            QUEST_ARENA_ROUND_2;

        m_secondBossGuid.Clear();

        // Clean up anything left from a previous Skarra attempt.
        DespawnSkarraBroodlings();

        Creature* pSkarra =
            m_creature->SummonCreature(
                NPC_SKARRA_BROODMOTHER,
                SKARRA_SPAWN_X,
                SKARRA_SPAWN_Y,
                SKARRA_SPAWN_Z,
                SKARRA_SPAWN_O,
                TEMPSUMMON_MANUAL_DESPAWN,
                0);

        if (!pSkarra)
        {
            m_ownerGuid.Clear();
            m_uiActiveQuest = 0;
            return;
        }

        m_bossGuid =
            pSkarra->GetObjectGuid();

        // Friendly/non-attackable while Krixel introduces her.
        pSkarra->SetFactionTemplateId(
            FACTION_ARENA_FIGHTER);

        pSkarra->SetReactState(
            REACT_PASSIVE);

        pSkarra->SetFlag(
            UNIT_FIELD_FLAGS,
            UNIT_FLAG_NON_ATTACKABLE);

        pSkarra->CombatStop(true);

        m_eArenaState = ARENA_INTRO;
        m_uiIntroStep = 0;
        m_uiIntroTimer = ARENA_GROUP_ACCEPT_GRACE_MS;
    }

    void BeginRoundTwoCombat()
    {
        Player* pOwner =
            GetArenaOwner();

        Creature* pSkarra =
            GetArenaBoss();

        if (!pOwner || !pSkarra)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        if (!IsInsideArena(pOwner))
        {
            if (pOwner->GetQuestStatus(
                QUEST_ARENA_ROUND_2) ==
                QUEST_STATUS_INCOMPLETE)
            {
                pOwner->FailQuest(
                    QUEST_ARENA_ROUND_2);
            }

            ResetArena();
            return;
        }

        // Keep Gadgetzan out of the sanctioned arena fight.
        SuppressArenaNpcs();

        pSkarra->SetFactionTemplateId(
            FACTION_ARENA_FIGHTER);

        pSkarra->SetReactState(
            REACT_PASSIVE);

        pSkarra->RemoveFlag(
            UNIT_FIELD_FLAGS,
            UNIT_FLAG_NON_ATTACKABLE);

        pSkarra->SetInCombatWith(
            pOwner);

        pOwner->SetInCombatWith(
            pSkarra);

        pSkarra->Attack(
            pOwner,
            true);

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
                !pPlayer->IsAlive() ||
                !IsArenaParticipant(pPlayer))
            {
                continue;
            }

            pSkarra->SetInCombatWith(
                pPlayer);

            pPlayer->SetInCombatWith(
                pSkarra);
        }

        m_eArenaState = ARENA_ACTIVE;
        m_uiBoundaryTimer = 1000;
    }

    void UpdateRoundTwoIntro(
        const uint32 uiDiff)
    {
        if (m_uiIntroTimer > uiDiff)
        {
            m_uiIntroTimer -= uiDiff;
            return;
        }

        Player* pOwner =
            GetArenaOwner();

        Creature* pSkarra =
            GetArenaBoss();

        if (!pOwner || !pSkarra)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        switch (m_uiIntroStep)
        {
        case 0:
        {
            m_creature->MonsterSay(
                "Round two! From the sands of Tanaris... Skarra the Broodmother!",
                LANG_UNIVERSAL,
                0);

            m_uiIntroTimer = 3000;
            ++m_uiIntroStep;
            break;
        }

        case 1:
        {
            pSkarra->SetFacingToObject(
                pOwner);

            m_uiIntroTimer = 1000;
            ++m_uiIntroStep;
            break;
        }

        case 2:
        {
            BeginRoundTwoCombat();
            break;
        }

        default:
            break;
        }
    }

    void HandleRoundTwoVictory()
    {
        if (m_eArenaState != ARENA_ACTIVE)
            return;

        Player* pOwner =
            GetArenaOwner();

        Creature* pSkarra =
            GetArenaBoss();

        if (!pOwner)
        {
            ResetArena();
            return;
        }

        m_eArenaState =
            ARENA_VICTORY;

        StopArenaParticipantCombat();

        // Remove any broodlings immediately when their mother dies.
        DespawnSkarraBroodlings();

        if (pSkarra)
        {
            pOwner->RewardPlayerAndGroupAtEvent(
                NPC_SKARRA_BROODMOTHER,
                pSkarra);

            pSkarra->CombatStop(true);
            pSkarra->DeleteThreatList();
            pSkarra->AddObjectToRemoveList();

            m_bossGuid.Clear();
        }

        m_uiVictoryTimer = 3000;
    }

    void UpdateActiveRoundTwo(
        const uint32 uiDiff)
    {
        Player* pOwner =
            GetArenaOwner();

        Creature* pSkarra =
            GetArenaBoss();

        if (!pOwner)
        {
            ResetArena();
            return;
        }

        if (!pSkarra)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        KeepArenaNpcsOutOfCombat();

        // Unlike Ripper, Skarra dies normally.
        if (!pSkarra->IsAlive())
        {
            HandleRoundTwoVictory();
            return;
        }

        if (m_uiBoundaryTimer > uiDiff)
        {
            m_uiBoundaryTimer -= uiDiff;
            return;
        }

        m_uiBoundaryTimer = 1000;

        if (!CheckArenaParticipants())
        {
            FailAllActiveParticipants();
            ResetArena();
        }
    }

    // =========================================================
    // ROUND 3 - GAZZIK BOOMWRENCH & THE PULVERIZER
    // =========================================================

    void StartRoundThree(Player* pPlayer)
    {
        if (!pPlayer)
            return;

        if (m_eArenaState != ARENA_IDLE)
            return;

        // Hard cleanup before every new bout. This prevents any
        // opponent from an earlier round appearing in this one.
        PurgeAllArenaOpponents();

        m_ownerGuid =
            pPlayer->GetObjectGuid();

        m_uiActiveQuest =
            QUEST_ARENA_ROUND_3;

        m_bRound3GazzikDefeated = false;
        m_bRound3PulverizerDefeated = false;

        Creature* pGazzik =
            m_creature->SummonCreature(
                NPC_GAZZIK_BOOMWRENCH,
                GAZZIK_SPAWN_X,
                GAZZIK_SPAWN_Y,
                GAZZIK_SPAWN_Z,
                GAZZIK_SPAWN_O,
                TEMPSUMMON_MANUAL_DESPAWN,
                0);

        if (!pGazzik)
        {
            m_ownerGuid.Clear();
            m_uiActiveQuest = 0;
            return;
        }

        Creature* pPulverizer =
            m_creature->SummonCreature(
                NPC_THE_PULVERIZER,
                PULVERIZER_SPAWN_X,
                PULVERIZER_SPAWN_Y,
                PULVERIZER_SPAWN_Z,
                PULVERIZER_SPAWN_O,
                TEMPSUMMON_MANUAL_DESPAWN,
                0);

        if (!pPulverizer)
        {
            pGazzik->AddObjectToRemoveList();

            m_ownerGuid.Clear();
            m_uiActiveQuest = 0;
            return;
        }

        m_bossGuid =
            pGazzik->GetObjectGuid();

        m_secondBossGuid =
            pPulverizer->GetObjectGuid();

        // Both are friendly/non-attackable during the intro.

        pGazzik->SetFactionTemplateId(
            FACTION_ARENA_FIGHTER);

        pGazzik->SetReactState(
            REACT_PASSIVE);

        pGazzik->SetFlag(
            UNIT_FIELD_FLAGS,
            UNIT_FLAG_NON_ATTACKABLE);

        pGazzik->CombatStop(true);

        pPulverizer->SetFactionTemplateId(
            FACTION_ARENA_FIGHTER);

        pPulverizer->SetReactState(
            REACT_PASSIVE);

        pPulverizer->SetFlag(
            UNIT_FIELD_FLAGS,
            UNIT_FLAG_NON_ATTACKABLE);

        pPulverizer->CombatStop(true);

        m_eArenaState = ARENA_INTRO;
        m_uiIntroStep = 0;
        m_uiIntroTimer = ARENA_GROUP_ACCEPT_GRACE_MS;
    }

    void EngageRoundThreeOpponent(
        Creature* pOpponent,
        Player* pOwner)
    {
        if (!pOpponent || !pOwner)
            return;

        pOpponent->SetFactionTemplateId(
            FACTION_ARENA_FIGHTER);

        pOpponent->SetReactState(
            REACT_PASSIVE);

        pOpponent->RemoveFlag(
            UNIT_FIELD_FLAGS,
            UNIT_FLAG_NON_ATTACKABLE);

        pOpponent->SetInCombatWith(
            pOwner);

        pOwner->SetInCombatWith(
            pOpponent);

        pOpponent->Attack(
            pOwner,
            true);

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
                !pPlayer->IsAlive() ||
                !IsArenaParticipant(pPlayer))
            {
                continue;
            }

            pOpponent->SetInCombatWith(
                pPlayer);

            pPlayer->SetInCombatWith(
                pOpponent);
        }
    }

    void BeginRoundThreeCombat()
    {
        Player* pOwner =
            GetArenaOwner();

        Creature* pGazzik =
            GetArenaBoss();

        Creature* pPulverizer =
            GetSecondArenaBoss();

        if (!pOwner ||
            !pGazzik ||
            !pPulverizer)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        if (!IsInsideArena(pOwner))
        {
            if (pOwner->GetQuestStatus(
                QUEST_ARENA_ROUND_3) ==
                QUEST_STATUS_INCOMPLETE)
            {
                pOwner->FailQuest(
                    QUEST_ARENA_ROUND_3);
            }

            ResetArena();
            return;
        }

        // Both opponent GUIDs are already stored, so the
        // suppression system knows not to suppress either one.
        SuppressArenaNpcs();

        EngageRoundThreeOpponent(
            pGazzik,
            pOwner);

        EngageRoundThreeOpponent(
            pPulverizer,
            pOwner);

        m_eArenaState = ARENA_ACTIVE;
        m_uiBoundaryTimer = 1000;
    }

    void UpdateRoundThreeIntro(
        const uint32 uiDiff)
    {
        if (m_uiIntroTimer > uiDiff)
        {
            m_uiIntroTimer -= uiDiff;
            return;
        }

        Player* pOwner =
            GetArenaOwner();

        Creature* pGazzik =
            GetArenaBoss();

        Creature* pPulverizer =
            GetSecondArenaBoss();

        if (!pOwner ||
            !pGazzik ||
            !pPulverizer)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        switch (m_uiIntroStep)
        {
        case 0:
        {
            m_creature->MonsterSay(
                "Round three! Gazzik Boomwrench and his latest creation... The Pulverizer!",
                LANG_UNIVERSAL,
                0);

            m_uiIntroTimer = 3000;
            ++m_uiIntroStep;
            break;
        }

        case 1:
        {
            pGazzik->SetFacingToObject(
                pOwner);

            pPulverizer->SetFacingToObject(
                pOwner);

            m_uiIntroTimer = 1500;
            ++m_uiIntroStep;
            break;
        }

        case 2:
        {
            BeginRoundThreeCombat();
            break;
        }

        default:
            break;
        }
    }

    void HandleRoundThreeVictory()
    {
        if (m_eArenaState != ARENA_ACTIVE)
            return;

        Player* pOwner =
            GetArenaOwner();

        if (!pOwner)
        {
            ResetArena();
            return;
        }

        m_eArenaState =
            ARENA_VICTORY;

        StopArenaParticipantCombat();

        m_uiVictoryTimer = 3000;
    }

    void UpdateActiveRoundThree(
        const uint32 uiDiff)
    {
        Player* pOwner =
            GetArenaOwner();

        if (!pOwner)
        {
            ResetArena();
            return;
        }

        Creature* pGazzik =
            GetArenaBoss();

        Creature* pPulverizer =
            GetSecondArenaBoss();

        KeepArenaNpcsOutOfCombat();

        // ---------------------------------------------
        // GAZZIK DEATH
        // ---------------------------------------------

        if (!m_bRound3GazzikDefeated)
        {
            if (!pGazzik)
            {
                // He vanished without us ever seeing a death.
                // Treat that as an invalid/failed attempt.
                FailAllActiveParticipants();
                ResetArena();
                return;
            }

            if (!pGazzik->IsAlive())
            {
                m_bRound3GazzikDefeated = true;

                pOwner->RewardPlayerAndGroupAtEvent(
                    NPC_GAZZIK_BOOMWRENCH,
                    pGazzik);

                pGazzik->CombatStop(true);
                pGazzik->DeleteThreatList();
                pGazzik->AddObjectToRemoveList();

                m_bossGuid.Clear();
            }
        }

        // ---------------------------------------------
        // PULVERIZER DEATH
        // ---------------------------------------------

        if (!m_bRound3PulverizerDefeated)
        {
            if (!pPulverizer)
            {
                FailAllActiveParticipants();
                ResetArena();
                return;
            }

            if (!pPulverizer->IsAlive())
            {
                m_bRound3PulverizerDefeated = true;

                pOwner->RewardPlayerAndGroupAtEvent(
                    NPC_THE_PULVERIZER,
                    pPulverizer);

                pPulverizer->CombatStop(true);
                pPulverizer->DeleteThreatList();
                pPulverizer->AddObjectToRemoveList();

                m_secondBossGuid.Clear();
            }
        }

        // Both objectives complete.
        if (m_bRound3GazzikDefeated &&
            m_bRound3PulverizerDefeated)
        {
            HandleRoundThreeVictory();
            return;
        }

        if (m_uiBoundaryTimer > uiDiff)
        {
            m_uiBoundaryTimer -= uiDiff;
            return;
        }

        m_uiBoundaryTimer = 1000;

        if (!CheckArenaParticipants())
        {
            FailAllActiveParticipants();
            ResetArena();
        }
    }

    // =========================================================
    // ROUND 4 - ZAL'JIRA THE HEXWEAVER
    // =========================================================

    void StartRoundFour(Player* pPlayer)
    {
        if (!pPlayer)
            return;

        if (m_eArenaState != ARENA_IDLE)
            return;

        // Hard cleanup before every new bout. This prevents any
        // opponent from an earlier round appearing in this one.
        PurgeAllArenaOpponents();

        m_ownerGuid = pPlayer->GetObjectGuid();
        m_uiActiveQuest = QUEST_ARENA_ROUND_4;
        m_secondBossGuid.Clear();

        Creature* pZaljira = m_creature->SummonCreature(
            NPC_ZALJIRA_HEXWEAVER,
            ZALJIRA_SPAWN_X,
            ZALJIRA_SPAWN_Y,
            ZALJIRA_SPAWN_Z,
            ZALJIRA_SPAWN_O,
            TEMPSUMMON_MANUAL_DESPAWN,
            0);

        if (!pZaljira)
        {
            m_ownerGuid.Clear();
            m_uiActiveQuest = 0;
            return;
        }

        m_bossGuid = pZaljira->GetObjectGuid();

        pZaljira->SetFactionTemplateId(FACTION_ARENA_FIGHTER);
        pZaljira->SetReactState(REACT_PASSIVE);
        pZaljira->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        pZaljira->CombatStop(true);

        m_eArenaState = ARENA_INTRO;
        m_uiIntroStep = 0;
        m_uiIntroTimer = ARENA_GROUP_ACCEPT_GRACE_MS;
    }

    void BeginRoundFourCombat()
    {
        Player* pOwner = GetArenaOwner();
        Creature* pZaljira = GetArenaBoss();

        if (!pOwner || !pZaljira)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        if (!IsInsideArena(pOwner))
        {
            if (pOwner->GetQuestStatus(QUEST_ARENA_ROUND_4) == QUEST_STATUS_INCOMPLETE)
                pOwner->FailQuest(QUEST_ARENA_ROUND_4);

            ResetArena();
            return;
        }

        SuppressArenaNpcs();

        pZaljira->SetFactionTemplateId(FACTION_ARENA_FIGHTER);
        pZaljira->SetReactState(REACT_PASSIVE);
        pZaljira->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        pZaljira->SetInCombatWith(pOwner);
        pOwner->SetInCombatWith(pZaljira);
        pZaljira->Attack(pOwner, true);

        Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();

        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* pPlayer = itr->getSource();

            if (!pPlayer || !pPlayer->IsAlive() || !IsArenaParticipant(pPlayer))
                continue;

            pZaljira->SetInCombatWith(pPlayer);
            pPlayer->SetInCombatWith(pZaljira);
        }

        m_eArenaState = ARENA_ACTIVE;
        m_uiBoundaryTimer = 1000;
    }

    void UpdateRoundFourIntro(const uint32 uiDiff)
    {
        if (m_uiIntroTimer > uiDiff)
        {
            m_uiIntroTimer -= uiDiff;
            return;
        }

        Player* pOwner = GetArenaOwner();
        Creature* pZaljira = GetArenaBoss();

        if (!pOwner || !pZaljira)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        switch (m_uiIntroStep)
        {
        case 0:
            m_creature->MonsterSay(
                "Round four! From the shadows comes Zal'jira the Hexweaver!",
                LANG_UNIVERSAL,
                0);
            m_uiIntroTimer = 3000;
            ++m_uiIntroStep;
            break;

        case 1:
            pZaljira->SetFacingToObject(pOwner);
            pZaljira->MonsterSay(
                "Heh heh... let's see what kinda creature ya make.",
                LANG_UNIVERSAL,
                0);
            m_uiIntroTimer = 2500;
            ++m_uiIntroStep;
            break;

        case 2:
            BeginRoundFourCombat();
            break;

        default:
            break;
        }
    }

    void HandleRoundFourVictory()
    {
        if (m_eArenaState != ARENA_ACTIVE)
            return;

        Player* pOwner = GetArenaOwner();
        Creature* pZaljira = GetArenaBoss();

        if (!pOwner)
        {
            ResetArena();
            return;
        }

        m_eArenaState = ARENA_VICTORY;

        StopArenaParticipantCombat();

        if (pZaljira)
        {
            pOwner->RewardPlayerAndGroupAtEvent(
                NPC_ZALJIRA_HEXWEAVER,
                pZaljira);

            pZaljira->CombatStop(true);
            pZaljira->DeleteThreatList();
            pZaljira->AddObjectToRemoveList();

            m_bossGuid.Clear();
        }

        m_uiVictoryTimer = 3000;
    }

    void UpdateActiveRoundFour(const uint32 uiDiff)
    {
        Player* pOwner = GetArenaOwner();

        if (!pOwner)
        {
            ResetArena();
            return;
        }

        Creature* pZaljira = GetArenaBoss();

        if (!pZaljira)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        KeepArenaNpcsOutOfCombat();

        if (!pZaljira->IsAlive())
        {
            HandleRoundFourVictory();
            return;
        }

        if (m_uiBoundaryTimer > uiDiff)
        {
            m_uiBoundaryTimer -= uiDiff;
            return;
        }

        m_uiBoundaryTimer = 1000;

        if (!CheckArenaParticipants())
        {
            FailAllActiveParticipants();
            ResetArena();
        }
    }

    // =========================================================
    // ROUND 5 - GORMAK THE UNBROKEN
    // =========================================================

    void StartRoundFive(Player* pPlayer)
    {
        if (!pPlayer)
            return;

        if (m_eArenaState != ARENA_IDLE)
            return;

        // Hard cleanup before every new bout. This prevents any
        // opponent from an earlier round appearing in this one.
        PurgeAllArenaOpponents();

        m_ownerGuid = pPlayer->GetObjectGuid();
        m_uiActiveQuest = QUEST_ARENA_ROUND_5;
        m_secondBossGuid.Clear();

        Creature* pGormak = m_creature->SummonCreature(
            NPC_GORMAK_UNBROKEN,
            GORMAK_SPAWN_X,
            GORMAK_SPAWN_Y,
            GORMAK_SPAWN_Z,
            GORMAK_SPAWN_O,
            TEMPSUMMON_MANUAL_DESPAWN,
            0);

        if (!pGormak)
        {
            m_ownerGuid.Clear();
            m_uiActiveQuest = 0;
            return;
        }

        m_bossGuid = pGormak->GetObjectGuid();

        pGormak->SetFactionTemplateId(FACTION_ARENA_FIGHTER);
        pGormak->SetReactState(REACT_PASSIVE);
        pGormak->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        pGormak->CombatStop(true);

        m_eArenaState = ARENA_INTRO;
        m_uiIntroStep = 0;
        m_uiIntroTimer = ARENA_GROUP_ACCEPT_GRACE_MS;
    }

    void BeginRoundFiveCombat()
    {
        Player* pOwner = GetArenaOwner();
        Creature* pGormak = GetArenaBoss();

        if (!pOwner || !pGormak)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        if (!IsInsideArena(pOwner))
        {
            if (pOwner->GetQuestStatus(
                QUEST_ARENA_ROUND_5) ==
                QUEST_STATUS_INCOMPLETE)
            {
                pOwner->FailQuest(
                    QUEST_ARENA_ROUND_5);
            }

            ResetArena();
            return;
        }

        SuppressArenaNpcs();

        // =====================================================
        // COMPLETELY END GORMAK'S INTRO STATE
        // =====================================================

        pGormak->HandleEmote(
            EMOTE_STATE_NONE);

        pGormak->SetStandState(
            UNIT_STAND_STATE_STAND);

        pGormak->SetFactionTemplateId(
            FACTION_ARENA_FIGHTER);

        pGormak->SetReactState(
            REACT_PASSIVE);

        // Explicitly clear every flag that could prevent
        // players from attacking him.
        pGormak->RemoveFlag(
            UNIT_FIELD_FLAGS,
            UNIT_FLAG_NON_ATTACKABLE);

        pGormak->RemoveFlag(
            UNIT_FIELD_FLAGS,
            UNIT_FLAG_NOT_SELECTABLE);

        // Clear any stale intro/combat state before the real fight begins.
        pGormak->CombatStop(true);
        pGormak->DeleteThreatList();
        pGormak->AttackStop();

        // =====================================================
        // START THE ACTUAL FIGHT
        // =====================================================

        pGormak->SetInCombatWith(
            pOwner);

        pOwner->SetInCombatWith(
            pGormak);

        pGormak->Attack(
            pOwner,
            true);

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
                !pPlayer->IsAlive() ||
                !IsArenaParticipant(pPlayer))
            {
                continue;
            }

            pGormak->SetInCombatWith(
                pPlayer);

            pPlayer->SetInCombatWith(
                pGormak);
        }

        m_eArenaState =
            ARENA_ACTIVE;

        m_uiBoundaryTimer = 1000;
    }

    void UpdateRoundFiveIntro(const uint32 uiDiff)
    {
        if (m_uiIntroTimer > uiDiff)
        {
            m_uiIntroTimer -= uiDiff;
            return;
        }

        Player* pOwner = GetArenaOwner();
        Creature* pGormak = GetArenaBoss();

        if (!pOwner || !pGormak)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        Creature* pFemaleOne =
            GetArenaGoblinFemale(
                GUID_FEMALE_SPECTATOR_1);

        Creature* pFemaleTwo =
            GetArenaGoblinFemale(
                GUID_FEMALE_SPECTATOR_2);

        switch (m_uiIntroStep)
        {
        case 0:
            m_creature->MonsterSay(
                "Final round! The biggest, meanest brute to step into this cage... Gormak the Unbroken!",
                LANG_UNIVERSAL,
                0);

            m_uiIntroTimer = 2500;
            ++m_uiIntroStep;
            break;

        case 1:
            if (pFemaleOne)
                pGormak->SetFacingToObject(
                    pFemaleOne);
            else if (pFemaleTwo)
                pGormak->SetFacingToObject(
                    pFemaleTwo);

            pGormak->MonsterSay(
                "Pretty goblin ladies watch Gormak, yes?",
                LANG_UNIVERSAL,
                0);

            m_uiIntroTimer = 1500;
            ++m_uiIntroStep;
            break;

        case 2:
            pGormak->HandleEmote(
                EMOTE_STATE_DANCE);

            m_uiIntroTimer = 3000;
            ++m_uiIntroStep;
            break;

        case 3:
            if (pFemaleOne)
            {
                pFemaleOne->MonsterSay(
                    "Is he still looking at us?",
                    LANG_UNIVERSAL,
                    0);
            }

            m_uiIntroTimer = 1800;
            ++m_uiIntroStep;
            break;

        case 4:
            if (pFemaleTwo)
            {
                pFemaleTwo->MonsterSay(
                    "Yep. Don't make eye contact.",
                    LANG_UNIVERSAL,
                    0);
            }

            m_uiIntroTimer = 2200;
            ++m_uiIntroStep;
            break;

        case 5:
            pGormak->MonsterSay(
                "They shy. Gormak understand.",
                LANG_UNIVERSAL,
                0);

            m_uiIntroTimer = 1800;
            ++m_uiIntroStep;
            break;

            // Crowd starts laying into the challengers before the bell.
        case 6:
            DoRandomCrowdTaunt(
                "Hope you said your goodbyes! Gormak's gonna pound you into the sand!");

            m_uiIntroTimer = 1700;
            ++m_uiIntroStep;
            break;

        case 7:
            DoRandomCrowdTaunt(
                "Look at these little ones! Gormak's gonna pick his teeth with your bones!");

            m_uiIntroTimer = 1700;
            ++m_uiIntroStep;
            break;

        case 8:
            DoRandomCrowdTaunt(
                "Five rounds just to get flattened by an ogre! That's rough!");

            m_uiIntroTimer = 1700;
            ++m_uiIntroStep;
            break;

        case 9:
            DoRandomCrowdTaunt(
                "Run while you've still got legs!");

            m_uiIntroTimer = 1800;
            ++m_uiIntroStep;
            break;

        case 10:
            pGormak->HandleEmote(
                EMOTE_STATE_NONE);

            pGormak->SetFacingToObject(
                pOwner);

            pGormak->MonsterSay(
                "Ladies watch! Gormak smash little ones good!",
                LANG_UNIVERSAL,
                0);

            m_uiIntroTimer = 1800;
            ++m_uiIntroStep;
            break;

        case 11:
            BeginRoundFiveCombat();
            break;

        default:
            break;
        }
    }

    void HandleRoundFiveVictory()
    {
        if (m_eArenaState != ARENA_ACTIVE)
            return;

        Player* pOwner = GetArenaOwner();
        Creature* pGormak = GetArenaBoss();

        if (!pOwner)
        {
            ResetArena();
            return;
        }

        m_eArenaState = ARENA_VICTORY;

        StopArenaParticipantCombat();

        // Announce the champion exactly once, when Gormak dies.
        // This is deliberately NOT tied to quest turn-in because every
        // group member may turn in the quest separately.
        m_creature->MonsterYell(
            "Hear this, Tanaris! Gormak the Unbroken has fallen! A new champion stands in the Gadgetzan Arena!",
            LANG_UNIVERSAL,
            0);

        if (pGormak)
        {
            pOwner->RewardPlayerAndGroupAtEvent(
                NPC_GORMAK_UNBROKEN,
                pGormak);

            pGormak->CombatStop(true);
            pGormak->DeleteThreatList();
            pGormak->AddObjectToRemoveList();

            m_bossGuid.Clear();
        }

        m_uiVictoryTimer = 3000;
    }

    void UpdateActiveRoundFive(const uint32 uiDiff)
    {
        Player* pOwner = GetArenaOwner();

        if (!pOwner)
        {
            ResetArena();
            return;
        }

        Creature* pGormak = GetArenaBoss();

        if (!pGormak)
        {
            FailAllActiveParticipants();
            ResetArena();
            return;
        }

        KeepArenaNpcsOutOfCombat();

        if (!pGormak->IsAlive())
        {
            HandleRoundFiveVictory();
            return;
        }

        if (m_uiBoundaryTimer > uiDiff)
        {
            m_uiBoundaryTimer -= uiDiff;
            return;
        }

        m_uiBoundaryTimer = 1000;

        if (!CheckArenaParticipants())
        {
            FailAllActiveParticipants();
            ResetArena();
        }
    }

    // =========================================================
    // SHARED VICTORY / UPDATE
    // =========================================================

    void UpdateVictory(
        const uint32 uiDiff)
    {
        KeepArenaNpcsOutOfCombat();

        if (m_uiVictoryTimer > uiDiff)
        {
            m_uiVictoryTimer -= uiDiff;
            return;
        }

        ResetArena();
    }

    void UpdateAI(
        const uint32 uiDiff) override
    {
        KeepKrixelOutOfCombat();

        switch (m_eArenaState)
        {
        case ARENA_IDLE:
            break;

        case ARENA_INTRO:
        {
            if (m_uiActiveQuest ==
                QUEST_ARENA_ROUND_1)
            {
                UpdateRoundOneIntro(
                    uiDiff);
            }
            else if (m_uiActiveQuest ==
                QUEST_ARENA_ROUND_2)
            {
                UpdateRoundTwoIntro(
                    uiDiff);
            }
            else if (m_uiActiveQuest ==
                QUEST_ARENA_ROUND_3)
            {
                UpdateRoundThreeIntro(
                    uiDiff);
            }
            else if (m_uiActiveQuest ==
                QUEST_ARENA_ROUND_4)
            {
                UpdateRoundFourIntro(
                    uiDiff);
            }
            else if (m_uiActiveQuest ==
                QUEST_ARENA_ROUND_5)
            {
                UpdateRoundFiveIntro(
                    uiDiff);
            }
            else
            {
                ResetArena();
            }

            break;
        }

        case ARENA_ACTIVE:
        {
            if (m_uiActiveQuest ==
                QUEST_ARENA_ROUND_1)
            {
                UpdateActiveRoundOne(
                    uiDiff);
            }
            else if (m_uiActiveQuest ==
                QUEST_ARENA_ROUND_2)
            {
                UpdateActiveRoundTwo(
                    uiDiff);
            }
            else if (m_uiActiveQuest ==
                QUEST_ARENA_ROUND_3)
            {
                UpdateActiveRoundThree(
                    uiDiff);
            }
            else if (m_uiActiveQuest ==
                QUEST_ARENA_ROUND_4)
            {
                UpdateActiveRoundFour(
                    uiDiff);
            }
            else if (m_uiActiveQuest ==
                QUEST_ARENA_ROUND_5)
            {
                UpdateActiveRoundFive(
                    uiDiff);
            }
            else
            {
                ResetArena();
            }

            break;
        }

        case ARENA_VICTORY:
        {
            UpdateVictory(
                uiDiff);
            break;
        }
        }
    }
};

CreatureAI* GetAI_npc_krixel_copperfist(
    Creature* pCreature)
{
    return new npc_krixel_copperfistAI(
        pCreature);
}

bool QuestAccept_npc_krixel_copperfist(
    Player* pPlayer,
    Creature* pCreature,
    Quest const* pQuest)
{
    if (!pPlayer ||
        !pCreature ||
        !pQuest)
    {
        return true;
    }

    uint32 uiQuestId =
        pQuest->GetQuestId();

    if (uiQuestId != QUEST_ARENA_ROUND_1 &&
        uiQuestId != QUEST_ARENA_ROUND_2 &&
        uiQuestId != QUEST_ARENA_ROUND_3 &&
        uiQuestId != QUEST_ARENA_ROUND_4 &&
        uiQuestId != QUEST_ARENA_ROUND_5)
    {
        return true;
    }

    npc_krixel_copperfistAI* pAI =
        dynamic_cast<npc_krixel_copperfistAI*>(
            pCreature->AI());

    if (!pAI)
        return true;

    if (pAI->m_eArenaState !=
        ARENA_IDLE)
    {
        if (pAI->IsSameArenaGroup(
            pPlayer) &&
            uiQuestId == pAI->m_uiActiveQuest)
        {
            return true;
        }

        pCreature->MonsterSay(
            "Hold it! Cage is occupied. Wait your turn!",
            LANG_UNIVERSAL,
            0);

        return false;
    }

    switch (uiQuestId)
    {
    case QUEST_ARENA_ROUND_1:
    {
        pAI->StartRoundOne(
            pPlayer);
        break;
    }

    case QUEST_ARENA_ROUND_2:
    {
        pAI->StartRoundTwo(
            pPlayer);
        break;
    }

    case QUEST_ARENA_ROUND_3:
    {
        pAI->StartRoundThree(
            pPlayer);
        break;
    }

    case QUEST_ARENA_ROUND_4:
    {
        pAI->StartRoundFour(
            pPlayer);
        break;
    }

    case QUEST_ARENA_ROUND_5:
    {
        pAI->StartRoundFive(
            pPlayer);
        break;
    }

    default:
        break;
    }

    return true;
}


void AddSC_tanaris_arena()
{
    Script* pNewScript;

    pNewScript =
        new Script;

    pNewScript->Name =
        "npc_krixel_copperfist";

    pNewScript->GetAI =
        &GetAI_npc_krixel_copperfist;

    pNewScript->pQuestAcceptNPC =
        &QuestAccept_npc_krixel_copperfist;

    pNewScript->RegisterSelf();
}