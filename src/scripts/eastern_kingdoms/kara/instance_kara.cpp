#include "scriptPCH.h"
#include "kara.h"

#include <cmath>
#include <sstream>

/*
 * Pauper's Walk gauntlet
 *
 * Behaviour:
 * - Entering the start entrance begins the event once per instance.
 * - Paupers continually spawn from two spawn points.
 * - Reaching the middle trigger disables spawn point 2.
 * - Reaching either exit trigger permanently completes the event.
 * - Completion is saved to the instance ID and cannot be started again.
 */

enum
{
    NPC_RESTLESS_PAUPER = 1000047,
    NPC_CRYPT_OVERSEER_MORHOLT = 1000021,
    NPC_CRYPT_CAPTAIN_BARTHOL = 1000022,
    NPC_CRYPT_CAPTAIN_ALDREN = 1000023,
    NPC_CRYPT_CAPTAIN_RENWALD = 1000024,

    DATA_PAUPERS_WALK = 1,

    PAUPERS_NOT_STARTED = 0,
    PAUPERS_IN_PROGRESS = 1,
    PAUPERS_DONE        = 2,

    PAUPER_SPAWN_INTERVAL = 6000,
    PAUPER_MAX_ALIVE      = 30


};

struct PositionData
{
    float x;
    float y;
    float z;
    float o;
};

/*
 * Coordinates supplied from the in-game GPS.
 */
static PositionData const PauperSpawnPoint1 =
{
    -1334.109619f, -549.690491f, 75.558723f, 4.507328f
};

static PositionData const PauperSpawnPoint2 =
{
    -1250.995972f, -518.033875f, 51.910641f, 3.929703f
};

static PositionData const MorholtSpawnPosition =
{
    -1322.140625f,
    -660.460327f,
    73.966751f,
    0.054691f
};

/*
 * Middle trigger:
 * crossing this part of the hallway stops spawn point 2,
 * while spawn point 1 continues.
 */
static PositionData const PauperMiddleTrigger =
{
    -1355.864868f, -486.918701f, 77.328293f, 0.0f
};

/*
 * Either exit completes the event permanently for this instance ID.
 */
static PositionData const PauperExitTrigger1 =
{
    -1297.910156f, -553.876587f, 75.162331f, 0.0f
};

static PositionData const PauperExitTrigger2 =
{
    -1254.586060f, -519.779724f, 51.109329f, 0.0f
};

static bool IsWithinRadius2D(Player* pPlayer, PositionData const& position, float radius)
{
    if (!pPlayer)
        return false;

    float const xDifference = pPlayer->GetPositionX() - position.x;
    float const yDifference = pPlayer->GetPositionY() - position.y;

    return xDifference * xDifference + yDifference * yDifference <= radius * radius;
}

/*
 * This is deliberately a box rather than one tiny point.
 * It covers the width and depth of the entrance.
 */
static bool IsInsidePauperStartEntrance(Player* pPlayer)
{
    if (!pPlayer)
        return false;

    float const x = pPlayer->GetPositionX();
    float const y = pPlayer->GetPositionY();
    float const z = pPlayer->GetPositionZ();

    return x >= -1298.0f && x <= -1281.0f &&
        y >= -453.0f && y <= -449.0f &&
        z >= 88.0f && z <= 112.0f;
}

struct instance_kara : public ScriptedInstance
{
    instance_kara(Map* pMap) : ScriptedInstance(pMap)
    {
        Initialize();
    }

    uint32 m_uiPauperEventState;
    uint32 m_uiPauperSpawnTimer;
    uint32 m_uiPositionCheckTimer;

    bool m_bSpawnPoint1Active;
    bool m_bSpawnPoint2Active;

    bool m_bBartholDead;
    bool m_bAldrenDead;
    bool m_bRenwaldDead;

    bool m_bMorholtSpawned;
    bool m_bMorholtDefeated;

    std::string m_strInstanceData;

    void Initialize()
    {
        m_uiPauperEventState = PAUPERS_NOT_STARTED;
        m_uiPauperSpawnTimer = PAUPER_SPAWN_INTERVAL;
        m_uiPositionCheckTimer = 500;

        m_bSpawnPoint1Active = false;
        m_bSpawnPoint2Active = false;

        m_bBartholDead = false;
        m_bAldrenDead = false;
        m_bRenwaldDead = false;

        m_bMorholtSpawned = false;
        m_bMorholtDefeated = false;

        m_strInstanceData.clear();
    }

    Player* GetFirstAlivePlayer()
    {
        Map::PlayerList const& playerList = instance->GetPlayers();

        for (Map::PlayerList::const_iterator itr = playerList.begin();
             itr != playerList.end(); ++itr)
        {
            Player* pPlayer = itr->getSource();

            if (pPlayer && pPlayer->IsAlive())
                return pPlayer;
        }

        return NULL;
    }

    Player* GetFirstPlayer()
    {
        Map::PlayerList const& playerList = instance->GetPlayers();

        for (Map::PlayerList::const_iterator itr = playerList.begin();
            itr != playerList.end(); ++itr)
        {
            Player* pPlayer = itr->getSource();

            if (pPlayer)
                return pPlayer;
        }

        return NULL;
    }

    Player* GetNearestAlivePlayer(float x, float y)
    {
        Player* pNearestPlayer = NULL;
        float nearestDistanceSquared = 0.0f;

        Map::PlayerList const& playerList = instance->GetPlayers();

        for (Map::PlayerList::const_iterator itr = playerList.begin();
             itr != playerList.end(); ++itr)
        {
            Player* pPlayer = itr->getSource();

            if (!pPlayer || !pPlayer->IsAlive())
                continue;

            float const xDifference = pPlayer->GetPositionX() - x;
            float const yDifference = pPlayer->GetPositionY() - y;
            float const distanceSquared =
                xDifference * xDifference + yDifference * yDifference;

            if (!pNearestPlayer || distanceSquared < nearestDistanceSquared)
            {
                pNearestPlayer = pPlayer;
                nearestDistanceSquared = distanceSquared;
            }
        }

        return pNearestPlayer;
    }

    uint32 CountLivingPaupers()
    {
        Player* pAnchor = GetFirstAlivePlayer();

        if (!pAnchor)
            return 0;

        std::list<Creature*> pauperList;
        GetCreatureListWithEntryInGrid(
            pauperList,
            pAnchor,
            NPC_RESTLESS_PAUPER,
            160.0f);

        uint32 livingCount = 0;

        for (std::list<Creature*>::iterator itr = pauperList.begin();
             itr != pauperList.end(); ++itr)
        {
            Creature* pPauper = *itr;

            if (pPauper && pPauper->IsAlive())
                ++livingCount;
        }

        return livingCount;
    }

    void DespawnAllPaupers()
    {
        Player* pAnchor = GetFirstPlayer();

        if (!pAnchor)
            return;

        std::list<Creature*> pauperList;

        GetCreatureListWithEntryInGrid(
            pauperList,
            pAnchor,
            NPC_RESTLESS_PAUPER,
            200.0f);

        for (std::list<Creature*>::iterator itr = pauperList.begin();
            itr != pauperList.end(); ++itr)
        {
            Creature* pPauper = *itr;

            if (pPauper)
                pPauper->ForcedDespawn();
        }
    }

    void ResetPauperEventAfterWipe()
    {
        if (m_uiPauperEventState != PAUPERS_IN_PROGRESS)
            return;

        DespawnAllPaupers();

        m_uiPauperEventState = PAUPERS_NOT_STARTED;
        m_bSpawnPoint1Active = false;
        m_bSpawnPoint2Active = false;
        m_uiPauperSpawnTimer = PAUPER_SPAWN_INTERVAL;

        SavePauperState();
    }

    void SavePauperState()
    {
        std::ostringstream saveStream;

        saveStream << m_uiPauperEventState << ' '
            << (m_bSpawnPoint1Active ? 1 : 0) << ' '
            << (m_bSpawnPoint2Active ? 1 : 0) << ' '
            << (m_bBartholDead ? 1 : 0) << ' '
            << (m_bAldrenDead ? 1 : 0) << ' '
            << (m_bRenwaldDead ? 1 : 0) << ' '
            << (m_bMorholtDefeated ? 1 : 0);

        m_strInstanceData = saveStream.str();
        SaveToDB();
    }

    void StartPauperEvent()
    {
        if (m_uiPauperEventState != PAUPERS_NOT_STARTED)
            return;

        m_uiPauperEventState = PAUPERS_IN_PROGRESS;
        m_bSpawnPoint1Active = true;
        m_bSpawnPoint2Active = true;
        m_uiPauperSpawnTimer = 1000;

        Map::PlayerList const& playerList = instance->GetPlayers();

        for (Map::PlayerList::const_iterator itr = playerList.begin();
            itr != playerList.end(); ++itr)
        {
            if (Player* pPlayer = itr->getSource())
            {
                pPlayer->MonsterTextEmote(
                    "The sound of rattling bones echoes through the crypt...",
                    0,
                    true);
            }
        }

        SavePauperState();
    }

    void DisableSecondPauperSpawn()
    {
        if (m_uiPauperEventState != PAUPERS_IN_PROGRESS)
            return;

        if (!m_bSpawnPoint2Active)
            return;

        m_bSpawnPoint2Active = false;
        SavePauperState();
    }

    void CompletePauperEvent()
    {

        if (m_uiPauperEventState == PAUPERS_DONE)
            return;

        m_uiPauperEventState = PAUPERS_DONE;
        m_bSpawnPoint1Active = false;
        m_bSpawnPoint2Active = false;
        m_uiPauperSpawnTimer = PAUPER_SPAWN_INTERVAL;

        SavePauperState();
    }

    void SpawnPauper(PositionData const& spawnPosition)
    {
        Player* pTarget =
            GetNearestAlivePlayer(spawnPosition.x, spawnPosition.y);

        if (!pTarget)
            return;

        Creature* pPauper = instance->SummonCreature(
            NPC_RESTLESS_PAUPER,
            spawnPosition.x,
            spawnPosition.y,
            spawnPosition.z,
            spawnPosition.o,
            TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,
            20000);

        if (!pPauper)
        {
            return;
        }

        if (pPauper->AI())
            pPauper->AI()->AttackStart(pTarget);
    }

    void SpawnActivePaupers()
    {
        uint32 livingPaupers = CountLivingPaupers();

        if (livingPaupers >= PAUPER_MAX_ALIVE)
            return;

        if (m_bSpawnPoint1Active)
        {
            for (uint8 i = 0; i < 3 && livingPaupers < PAUPER_MAX_ALIVE; ++i)
            {
                SpawnPauper(PauperSpawnPoint1);
                ++livingPaupers;
            }
        }

        if (m_bSpawnPoint2Active)
        {
            for (uint8 i = 0; i < 3 && livingPaupers < PAUPER_MAX_ALIVE; ++i)
            {
                SpawnPauper(PauperSpawnPoint2);
                ++livingPaupers;
            }
        }
    }

    void OnCreatureDeath(Creature* pCreature)
    {
        if (!pCreature)
            return;

        switch (pCreature->GetEntry())
        {
        case NPC_CRYPT_CAPTAIN_BARTHOL:
            m_bBartholDead = true;
            break;

        case NPC_CRYPT_CAPTAIN_ALDREN:
            m_bAldrenDead = true;
            break;

        case NPC_CRYPT_CAPTAIN_RENWALD:
            m_bRenwaldDead = true;
            break;

        case NPC_CRYPT_OVERSEER_MORHOLT:
            m_bMorholtDefeated = true;
            m_bMorholtSpawned = false;
            SavePauperState();
            return;

        default:
            return;
        }

        SavePauperState();
        TrySpawnMorholt();
    }

    void TrySpawnMorholt()
    {
        if (!m_bBartholDead ||
            !m_bAldrenDead ||
            !m_bRenwaldDead)
        {
            return;
        }

        if (m_bMorholtSpawned || m_bMorholtDefeated)
            return;

        Creature* pMorholt = instance->SummonCreature(
            NPC_CRYPT_OVERSEER_MORHOLT,
            MorholtSpawnPosition.x,
            MorholtSpawnPosition.y,
            MorholtSpawnPosition.z,
            MorholtSpawnPosition.o,
            TEMPSUMMON_DEAD_DESPAWN,
            0);

        if (!pMorholt)
            return;

        m_bMorholtSpawned = true;

        pMorholt->MonsterYell(
            "You have slain my captains. Now you will answer to their master!",
            LANG_UNIVERSAL,
            0);

        SavePauperState();
    }

    void CheckPlayerPositions()
    {
        Map::PlayerList const& playerList = instance->GetPlayers();

        for (Map::PlayerList::const_iterator itr = playerList.begin();
             itr != playerList.end(); ++itr)
        {
            Player* pPlayer = itr->getSource();

            if (!pPlayer || !pPlayer->IsAlive())
                continue;

            /*
             * Once done, no position can reactivate it.
             */
            if (m_uiPauperEventState == PAUPERS_DONE)
                return;

            /*
             * Either escape route permanently ends the event.
             * The radii are intentionally broad enough to cover
             * the whole passage rather than one exact coordinate.
             */
            if (m_uiPauperEventState == PAUPERS_IN_PROGRESS)
            {
                if (IsWithinRadius2D(pPlayer, PauperExitTrigger1, 8.0f) ||
                    IsWithinRadius2D(pPlayer, PauperExitTrigger2, 22.0f))
                {
                    CompletePauperEvent();
                    return;
                }

                /*
                 * Reaching the middle passage disables spawn point 2.
                 * Spawn point 1 continues until an exit is reached.
                 */
                if (m_bSpawnPoint2Active &&
                    IsWithinRadius2D(pPlayer, PauperMiddleTrigger, 20.0f))
                {
                    DisableSecondPauperSpawn();
                }
            }

            /*
             * The event may only start from NOT_STARTED.
             * It can therefore never restart after completion.
             */
            if (m_uiPauperEventState == PAUPERS_NOT_STARTED &&
                IsInsidePauperStartEntrance(pPlayer))
            {
                StartPauperEvent();
                return;
            }
        }
    }

    void SetData(uint32 uiType, uint32 uiData)
    {
        if (uiType != DATA_PAUPERS_WALK)
            return;

        switch (uiData)
        {
            case PAUPERS_NOT_STARTED:
                /*
                 * Do not allow outside code to reset a completed event.
                 */
                if (m_uiPauperEventState != PAUPERS_DONE)
                {
                    m_uiPauperEventState = PAUPERS_NOT_STARTED;
                    m_bSpawnPoint1Active = false;
                    m_bSpawnPoint2Active = false;
                    SavePauperState();
                }
                break;

            case PAUPERS_IN_PROGRESS:
                StartPauperEvent();
                break;

            case PAUPERS_DONE:
                CompletePauperEvent();
                break;
        }
    }

    uint32 GetData(uint32 uiType)
    {
        if (uiType == DATA_PAUPERS_WALK)
            return m_uiPauperEventState;

        return 0;
    }

    const char* Save()
    {
        return m_strInstanceData.c_str();
    }

    void Load(const char* pData)
    {
        if (!pData)
        {
            Initialize();
            return;
        }

        std::istringstream loadStream(pData);

        uint32 spawnPoint1Active = 0;
        uint32 spawnPoint2Active = 0;

        uint32 bartholDead = 0;
        uint32 aldrenDead = 0;
        uint32 renwaldDead = 0;
        uint32 morholtDefeated = 0;

        loadStream >> m_uiPauperEventState
            >> spawnPoint1Active
            >> spawnPoint2Active
            >> bartholDead
            >> aldrenDead
            >> renwaldDead
            >> morholtDefeated;

        if (loadStream.fail())
        {
            Initialize();
            return;
        }

        m_bSpawnPoint1Active = spawnPoint1Active != 0;
        m_bSpawnPoint2Active = spawnPoint2Active != 0;

        m_bBartholDead = bartholDead != 0;
        m_bAldrenDead = aldrenDead != 0;
        m_bRenwaldDead = renwaldDead != 0;

        m_bMorholtDefeated = morholtDefeated != 0;
        m_bMorholtSpawned = false;

        /*
         * DONE always remains fully disabled after an instance reload.
         */
        if (m_uiPauperEventState == PAUPERS_DONE)
        {
            m_bSpawnPoint1Active = false;
            m_bSpawnPoint2Active = false;
        }
        /*
         * A saved in-progress event resumes spawning after reload.
         */
        else if (m_uiPauperEventState == PAUPERS_IN_PROGRESS)
        {
            m_uiPauperSpawnTimer = 1000;
        }
        else
        {
            m_uiPauperEventState = PAUPERS_NOT_STARTED;
            m_bSpawnPoint1Active = false;
            m_bSpawnPoint2Active = false;
        }

        m_strInstanceData = pData;
    }

    void Update(uint32 uiDiff)
    {
        if (m_uiPauperSpawnTimer <= uiDiff)
        {

            SpawnActivePaupers();
            m_uiPauperSpawnTimer = PAUPER_SPAWN_INTERVAL;
        }

        if (m_uiPositionCheckTimer <= uiDiff)
        {
            /*
             * If the event is running and nobody is alive,
             * the group has wiped.
             */
            if (m_uiPauperEventState == PAUPERS_IN_PROGRESS &&
                !GetFirstAlivePlayer())
            {
                ResetPauperEventAfterWipe();
                m_uiPositionCheckTimer = 500;
                return;
            }

            CheckPlayerPositions();
            m_uiPositionCheckTimer = 500;
        }
        else
            m_uiPositionCheckTimer -= uiDiff;

        if (!m_bMorholtSpawned && !m_bMorholtDefeated)
            TrySpawnMorholt();

        if (m_uiPauperEventState != PAUPERS_IN_PROGRESS)
            return;

        if (m_uiPauperSpawnTimer <= uiDiff)
        {
            SpawnActivePaupers();
            m_uiPauperSpawnTimer = PAUPER_SPAWN_INTERVAL;
        }
        else
            m_uiPauperSpawnTimer -= uiDiff;


    }
};

InstanceData* GetInstanceData_instance_kara(Map* pMap)
{
    return new instance_kara(pMap);
}

void AddSC_instance_kara()
{
    Script* pNewScript = new Script;

    pNewScript->Name = "instance_kara";
    pNewScript->GetInstanceData = &GetInstanceData_instance_kara;
    pNewScript->RegisterSelf();
}
