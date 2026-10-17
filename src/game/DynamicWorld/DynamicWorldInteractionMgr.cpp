#include "DynamicWorldInteractionMgr.h"

#include "Database/DatabaseEnv.h"
#include "GameObject.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "Policies/SingletonImp.h"
#include "Util.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include <algorithm>
#include <ctime>
#include "PoolManager.h"
#include "Conditions.h"
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "Item.h"

#include <sstream>

INSTANTIATE_SINGLETON_1(DynamicWorldInteractionMgr);

DynamicWorldInteractionMgr::DynamicWorldInteractionMgr()
    : m_rotationCheckTimer(10000)
{}

void DynamicWorldInteractionMgr::Load()
{
    m_interactions.clear();
    m_gameObjectInteractions.clear();

    sLog.outString("Loading Dynamic World Interactions...");

    QueryResult* result = WorldDatabase.Query(
        "SELECT "
        "id, name, interaction_type, condition_entry, repeat_type, "
        "cooldown_seconds, active_duration_seconds, flags, enabled, "
        "script_name, comment "
        "FROM dynamic_world_interaction "
        "WHERE enabled = 1");

    uint32 interactionCount = 0;

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            DynamicWorldInteraction interaction;

            interaction.id = fields[0].GetUInt32();
            interaction.name = fields[1].GetCppString();
            interaction.interactionType = fields[2].GetUInt8();
            interaction.conditionEntry = fields[3].GetUInt32();
            interaction.repeatType = fields[4].GetUInt8();
            interaction.cooldownSeconds = fields[5].GetUInt32();
            interaction.activeDurationSeconds = fields[6].GetUInt32();
            interaction.flags = fields[7].GetUInt32();
            interaction.enabled = fields[8].GetBool();
            interaction.scriptName = fields[9].GetCppString();
            interaction.comment = fields[10].GetCppString();
            if (!interaction.id)
            {
                sLog.outErrorDb(
                    "Dynamic World Interaction with id 0 was skipped.");
                continue;
            }

            m_interactions[interaction.id] = interaction;
            ++interactionCount;
        } while (result->NextRow());

        delete result;
    }

    sLog.outString(
        ">> Loaded %u Dynamic World Interaction definitions",
        interactionCount);

    result = WorldDatabase.Query(
        "SELECT interaction_id, guid "
        "FROM dynamic_world_interaction_gameobject");

    uint32 gameObjectLinkCount = 0;

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 interactionId = fields[0].GetUInt32();
            uint32 guid = fields[1].GetUInt32();

            if (m_interactions.find(interactionId) == m_interactions.end())
            {
                sLog.outErrorDb(
                    "Dynamic World GameObject GUID %u references "
                    "missing or disabled interaction %u.",
                    guid,
                    interactionId);
                continue;
            }

            m_gameObjectInteractions[guid] = interactionId;
            ++gameObjectLinkCount;
        } while (result->NextRow());

        delete result;
    }

    sLog.outString(
        ">> Loaded %u Dynamic World GameObject links",
        gameObjectLinkCount);

    result = WorldDatabase.Query(
        "SELECT "
        "interaction_id, action_order, action_type, target_type, "
        "value1, value2, value3, value4, chance, delay_ms, comment "
        "FROM dynamic_world_interaction_action "
        "ORDER BY interaction_id, action_order");

    uint32 actionCount = 0;

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 interactionId = fields[0].GetUInt32();

            InteractionMap::iterator interactionItr =
                m_interactions.find(interactionId);

            if (interactionItr == m_interactions.end())
            {
                sLog.outErrorDb(
                    "Dynamic World action references missing or disabled "
                    "interaction %u.",
                    interactionId);
                continue;
            }

            DynamicWorldAction action;

            action.actionOrder = fields[1].GetUInt8();
            action.actionType = fields[2].GetUInt8();
            action.targetType = fields[3].GetUInt8();
            action.value1 = fields[4].GetInt32();
            action.value2 = fields[5].GetInt32();
            action.value3 = fields[6].GetInt32();
            action.value4 = fields[7].GetInt32();
            action.chance = fields[8].GetFloat();
            action.delayMs = fields[9].GetUInt32();
            action.comment = fields[10].GetCppString();

            interactionItr->second.actions.push_back(action);
            ++actionCount;
        } while (result->NextRow());

        delete result;
    }

    for (InteractionMap::iterator itr = m_interactions.begin();
        itr != m_interactions.end();
        ++itr)
    {
        std::sort(
            itr->second.actions.begin(),
            itr->second.actions.end(),
            [](DynamicWorldAction const& left, DynamicWorldAction const& right)
            {
                return left.actionOrder < right.actionOrder;
            });
    }

    sLog.outString(
        ">> Loaded %u Dynamic World Interaction actions",
        actionCount);

    sLog.outString(
        ">> Dynamic World system loaded: %u definitions, %u links, %u actions",
        interactionCount,
        gameObjectLinkCount,
        actionCount);

    sLog.outString();
}

DynamicWorldInteraction const*
DynamicWorldInteractionMgr::GetInteraction(uint32 interactionId) const
{
    InteractionMap::const_iterator itr =
        m_interactions.find(interactionId);

    if (itr == m_interactions.end())
        return nullptr;

    return &itr->second;
}

DynamicWorldInteraction const*
DynamicWorldInteractionMgr::GetInteractionForGameObject(uint32 guid) const
{
    GameObjectInteractionMap::const_iterator guidItr =
        m_gameObjectInteractions.find(guid);

    if (guidItr == m_gameObjectInteractions.end())
        return nullptr;

    return GetInteraction(guidItr->second);
}

bool DynamicWorldInteractionMgr::IsDynamicWorldGameObject(uint32 guid) const
{
    return m_gameObjectInteractions.find(guid) != m_gameObjectInteractions.end();
}

bool DynamicWorldInteractionMgr::GetCharacterProgress(
    uint32 characterGuid,
    uint32 interactionId,
    DynamicWorldCharacterProgress& progress) const
{
    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT completion_count, first_completed, last_completed "
        "FROM character_dynamic_world_interaction "
        "WHERE guid = '%u' AND interaction_id = '%u' "
        "LIMIT 1",
        characterGuid,
        interactionId);

    if (!result)
        return false;

    Field* fields = result->Fetch();

    progress.completionCount = fields[0].GetUInt32();
    progress.firstCompleted = fields[1].GetUInt64();
    progress.lastCompleted = fields[2].GetUInt64();

    delete result;
    return true;
}

void DynamicWorldInteractionMgr::LoadRotations()
{
    m_rotations.clear();
    m_rotationCheckTimer = 10000;

    sLog.outString("Loading Dynamic World Rotations...");

    QueryResult* result = WorldDatabase.Query(
        "SELECT "
        "id, name, pool_entry, rotation_seconds, "
        "last_rotation, next_rotation, enabled, flags, comment "
        "FROM dynamic_world_rotation "
        "WHERE enabled = 1");

    uint32 count = 0;
    uint64 currentTime = uint64(std::time(nullptr));

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            DynamicWorldRotation rotation;

            rotation.id = fields[0].GetUInt32();
            rotation.name = fields[1].GetCppString();

            uint32 poolEntry = fields[2].GetUInt32();

            rotation.rotationSeconds = fields[3].GetUInt32();
            rotation.lastRotation = fields[4].GetUInt64();
            rotation.nextRotation = fields[5].GetUInt64();
            rotation.enabled = fields[6].GetBool();
            rotation.flags = fields[7].GetUInt32();
            rotation.comment = fields[8].GetCppString();

            if (!rotation.id)
            {
                sLog.outErrorDb(
                    "Dynamic World Rotation with id 0 was skipped.");
                continue;
            }

            if (!poolEntry || poolEntry > 65535)
            {
                sLog.outErrorDb(
                    "Dynamic World Rotation %u has invalid pool_entry %u. "
                    "This core supports pool IDs from 1 to 65535.",
                    rotation.id,
                    poolEntry);
                continue;
            }

            rotation.poolEntry = uint16(poolEntry);

            if (rotation.poolEntry > sPoolMgr.GetMaxPoolId())
            {
                sLog.outErrorDb(
                    "Dynamic World Rotation %u references missing pool %u.",
                    rotation.id,
                    uint32(rotation.poolEntry));
                continue;
            }

            if (!rotation.rotationSeconds)
            {
                sLog.outErrorDb(
                    "Dynamic World Rotation %u has rotation_seconds 0.",
                    rotation.id);
                continue;
            }

            // A newly-created rotation receives its first future deadline.
            if (!rotation.nextRotation)
            {
                rotation.nextRotation =
                    currentTime + rotation.rotationSeconds;

                WorldDatabase.PExecute(
                    "UPDATE dynamic_world_rotation "
                    "SET next_rotation = " UI64FMTD " "
                    "WHERE id = '%u'",
                    rotation.nextRotation,
                    rotation.id);
            }

            m_rotations[rotation.id] = rotation;
            ++count;

        } while (result->NextRow());

        delete result;
    }

    sLog.outString(
        ">> Loaded %u Dynamic World Rotation definitions",
        count);

    sLog.outString();
}

void DynamicWorldInteractionMgr::HandleQuestAccepted(
    Player* player,
    GameObject* gameObject,
    uint32 questId)
{
    if (!player || !gameObject || !questId)
        return;

    DynamicWorldInteraction const* interaction =
        GetInteractionForGameObject(gameObject->GetGUIDLow());

    if (!interaction)
        return;

    bool startsAcceptedQuest = false;

    for (DynamicWorldActionList::const_iterator itr =
        interaction->actions.begin();
        itr != interaction->actions.end();
        ++itr)
    {
        DynamicWorldAction const& action = *itr;

        if (action.actionType == DYNAMIC_WORLD_ACTION_START_QUEST &&
            action.value1 > 0 &&
            uint32(action.value1) == questId)
        {
            startsAcceptedQuest = true;
            break;
        }
    }

    if (!startsAcceptedQuest)
        return;

    DynamicWorldCharacterProgress progress;

    if (interaction->repeatType == 0 &&
        GetCharacterProgress(
            player->GetGUIDLow(),
            interaction->id,
            progress))
    {
        return;
    }

    SaveCharacterCompletion(
        player->GetGUIDLow(),
        interaction->id);
}

bool DynamicWorldInteractionMgr::IsDynamicWorldQuestStarter(
    GameObject* gameObject,
    uint32 questId) const
{
    if (!gameObject || !questId)
        return false;

    DynamicWorldInteraction const* interaction =
        GetInteractionForGameObject(gameObject->GetGUIDLow());

    if (!interaction)
        return false;

    for (DynamicWorldActionList::const_iterator itr = interaction->actions.begin();
        itr != interaction->actions.end();
        ++itr)
    {
        DynamicWorldAction const& action = *itr;

        if (action.actionType == DYNAMIC_WORLD_ACTION_START_QUEST &&
            action.value1 > 0 &&
            uint32(action.value1) == questId)
        {
            return true;
        }
    }

    return false;
}

void DynamicWorldInteractionMgr::RotatePool(
    DynamicWorldRotation& rotation)
{
    uint64 currentTime = uint64(std::time(nullptr));

    sLog.outString(
        "Rotating Dynamic World pool %u: %s",
        uint32(rotation.poolEntry),
        rotation.name.c_str());

    // Remove the currently selected members from every applicable map state.
    sPoolMgr.DespawnPoolInMaps(rotation.poolEntry);

    // Immediately select and spawn a new set according to pool max_limit
    // and member chances.
    sPoolMgr.SpawnPoolInMaps(rotation.poolEntry, true);

    rotation.lastRotation = currentTime;
    rotation.nextRotation =
        currentTime + rotation.rotationSeconds;

    WorldDatabase.PExecute(
        "UPDATE dynamic_world_rotation "
        "SET last_rotation = " UI64FMTD ", "
        "next_rotation = " UI64FMTD " "
        "WHERE id = '%u'",
        rotation.lastRotation,
        rotation.nextRotation,
        rotation.id);

    sLog.outString(
        ">> Dynamic World pool %u rotated. Next rotation: " UI64FMTD,
        uint32(rotation.poolEntry),
        rotation.nextRotation);
}

void DynamicWorldInteractionMgr::Update(uint32 diff)
{
    if (diff >= m_rotationCheckTimer)
        m_rotationCheckTimer = 0;
    else
    {
        m_rotationCheckTimer -= diff;
        return;
    }

    m_rotationCheckTimer = 10000;

    if (m_rotations.empty())
        return;

    uint64 currentTime = uint64(std::time(nullptr));

    for (DynamicWorldRotationMap::iterator itr = m_rotations.begin();
        itr != m_rotations.end();
        ++itr)
    {
        DynamicWorldRotation& rotation = itr->second;

        if (!rotation.enabled)
            continue;

        if (rotation.nextRotation &&
            currentTime >= rotation.nextRotation)
        {
            RotatePool(rotation);
        }
    }
}

void DynamicWorldInteractionMgr::SaveCharacterCompletion(
    uint32 characterGuid,
    uint32 interactionId)
{
    uint64 currentTime = uint64(std::time(nullptr));

    CharacterDatabase.PExecute(
        "INSERT INTO character_dynamic_world_interaction "
        "(guid, interaction_id, completion_count, "
        "first_completed, last_completed) "
        "VALUES ('%u', '%u', 1, " UI64FMTD ", " UI64FMTD ") "
        "ON DUPLICATE KEY UPDATE "
        "completion_count = completion_count + 1, "
        "last_completed = VALUES(last_completed)",
        characterGuid,
        interactionId,
        currentTime,
        currentTime);
}

void DynamicWorldInteractionMgr::SendPlayerMessage(
    Player* player,
    std::string const& message) const
{
    if (!player || !player->GetSession())
        return;

    WorldPacket data(SMSG_NOTIFICATION, message.size() + 1);
    data << message;

    player->GetSession()->SendPacket(&data);
}

std::string DynamicWorldInteractionMgr::DescribeCondition(
    uint32 conditionId) const
{
    ConditionEntry const* condition =
        sConditionStorage.LookupEntry<ConditionEntry>(conditionId);

    if (!condition)
        return "an unknown requirement";

    std::ostringstream message;

    switch (condition->GetConditionType())
    {
    case CONDITION_SKILL:
    {
        SkillLineEntry const* skill =
            sSkillLineStore.LookupEntry(condition->GetValue1());

        char const* skillName = nullptr;

        if (skill)
            skillName = skill->name[sWorld.GetDefaultDbcLocale()];

        if (skillName && skillName[0] != '\0')
            message << skillName;
        else
            message << "skill " << condition->GetValue1();

        message << " (" << condition->GetValue2() << ")";
        return message.str();
    }

    case CONDITION_SKILL_BELOW:
    {
        SkillLineEntry const* skill =
            sSkillLineStore.LookupEntry(condition->GetValue1());

        char const* skillName = nullptr;

        if (skill)
            skillName = skill->name[sWorld.GetDefaultDbcLocale()];

        if (skillName && skillName[0] != '\0')
            message << skillName;
        else
            message << "skill " << condition->GetValue1();

        message << " below " << condition->GetValue2();
        return message.str();
    }

    case CONDITION_LEVEL:
    {
        switch (condition->GetValue2())
        {
        case 0:
            message << "level " << condition->GetValue1();
            break;
        case 1:
            message << "level " << condition->GetValue1() << " or higher";
            break;
        case 2:
            message << "level " << condition->GetValue1() << " or lower";
            break;
        default:
            message << "the required level";
            break;
        }

        return message.str();
    }

    case CONDITION_ITEM:
    {
        message << "item " << condition->GetValue1();

        if (condition->GetValue2() > 1)
            message << " x" << condition->GetValue2();

        return message.str();
    }

    case CONDITION_ITEM_EQUIPPED:
    {
        message << "item " << condition->GetValue1() << " equipped";
        return message.str();
    }

    case CONDITION_QUESTREWARDED:
    {
        message << "quest " << condition->GetValue1() << " completed";
        return message.str();
    }

    case CONDITION_QUESTTAKEN:
    {
        message << "quest " << condition->GetValue1() << " active";
        return message.str();
    }

    case CONDITION_QUESTAVAILABLE:
    {
        message << "quest " << condition->GetValue1() << " available";
        return message.str();
    }

    case CONDITION_SPELL:
    {
        if (condition->GetValue2() == 0)
            message << "spell " << condition->GetValue1() << " learned";
        else
            message << "spell " << condition->GetValue1() << " not learned";

        return message.str();
    }

    case CONDITION_REPUTATION_RANK_MIN:
    {
        static char const* reputationRanks[] =
        {
            "Hated",
            "Hostile",
            "Unfriendly",
            "Neutral",
            "Friendly",
            "Honored",
            "Revered",
            "Exalted"
        };

        if (condition->GetValue2() < 8)
            message << reputationRanks[condition->GetValue2()];
        else
            message << "the required reputation";

        message << " with faction " << condition->GetValue1();
        return message.str();
    }

    case CONDITION_AND:
    {
        return DescribeCondition(condition->GetValue1()) +
            " and " +
            DescribeCondition(condition->GetValue2());
    }

    case CONDITION_OR:
    {
        return DescribeCondition(condition->GetValue1()) +
            " or " +
            DescribeCondition(condition->GetValue2());
    }

    case CONDITION_NOT:
    {
        return "not " + DescribeCondition(condition->GetValue1());
    }

    default:
        break;
    }

    return "the required conditions";
}

std::string DynamicWorldInteractionMgr::GetConditionFailureText(
    uint32 conditionId,
    Player* player,
    GameObject* gameObject) const
{
    ConditionEntry const* condition =
        sConditionStorage.LookupEntry<ConditionEntry>(conditionId);

    if (!condition)
        return "This interaction has an invalid requirement.";

    // For an AND condition, report the first requirement that failed.
    if (condition->GetConditionType() == CONDITION_AND)
    {
        if (!IsConditionSatisfied(
            condition->GetValue1(),
            player,
            player->GetMap(),
            gameObject,
            CONDITION_FROM_HARDCODED))
        {
            return GetConditionFailureText(
                condition->GetValue1(),
                player,
                gameObject);
        }

        if (!IsConditionSatisfied(
            condition->GetValue2(),
            player,
            player->GetMap(),
            gameObject,
            CONDITION_FROM_HARDCODED))
        {
            return GetConditionFailureText(
                condition->GetValue2(),
                player,
                gameObject);
        }
    }

    // Both sides of an OR failed, so display both alternatives.
    if (condition->GetConditionType() == CONDITION_OR)
    {
        return "Requires " +
            DescribeCondition(condition->GetValue1()) +
            " or " +
            DescribeCondition(condition->GetValue2()) +
            ".";
    }

    return "Requires " + DescribeCondition(conditionId) + ".";
}

std::string DynamicWorldInteractionMgr::FormatCooldownRemaining(
    uint64 seconds) const
{
    std::ostringstream message;

    uint64 days = seconds / DAY;
    seconds %= DAY;

    uint64 hours = seconds / HOUR;
    seconds %= HOUR;

    uint64 minutes = seconds / MINUTE;
    seconds %= MINUTE;

    if (days)
    {
        message << days << " day";

        if (days != 1)
            message << "s";

        if (hours)
            message << " " << hours << " hour" << (hours != 1 ? "s" : "");
    }
    else if (hours)
    {
        message << hours << " hour";

        if (hours != 1)
            message << "s";

        if (minutes)
            message << " " << minutes << " minute" << (minutes != 1 ? "s" : "");
    }
    else if (minutes)
    {
        message << minutes << " minute";

        if (minutes != 1)
            message << "s";

        if (seconds)
            message << " " << seconds << " second" << (seconds != 1 ? "s" : "");
    }
    else
    {
        message << seconds << " second";

        if (seconds != 1)
            message << "s";
    }

    return message.str();
}

bool DynamicWorldInteractionMgr::HandleInteraction(
    Player* player,
    GameObject* gameObject)
{
    if (!player || !gameObject)
        return false;

    DynamicWorldInteraction const* interaction =
        GetInteractionForGameObject(gameObject->GetGUIDLow());

    // Not one of our GameObjects: allow normal MaNGOS handling.
    if (!interaction)
        return false;

    // Check the optional MaNGOS condition before granting any rewards.
    if (interaction->conditionEntry &&
        !IsConditionSatisfied(
            interaction->conditionEntry,
            player,
            player->GetMap(),
            gameObject,
            CONDITION_FROM_HARDCODED))
    {
        SendPlayerMessage(
            player,
            GetConditionFailureText(
                interaction->conditionEntry,
                player,
                gameObject));

        // Ensure the underlying GO remains available.
        gameObject->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
        gameObject->SetGoState(GO_STATE_READY);
        gameObject->SetLootState(GO_READY);
        gameObject->SetRespawnTime(0);
        gameObject->Refresh();

        return true;
    }

    uint32 characterGuid = player->GetGUIDLow();

    DynamicWorldCharacterProgress progress;
    bool hasProgress = GetCharacterProgress(
        characterGuid,
        interaction->id,
        progress);

    switch (interaction->repeatType)
    {
    case 0:
    {
        // Once per character.
        if (hasProgress)
        {
            SendPlayerMessage(
                player,
                "You have already discovered this.");

            gameObject->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
            gameObject->SetGoState(GO_STATE_READY);
            gameObject->SetLootState(GO_READY);
            gameObject->SetRespawnTime(0);
            gameObject->Refresh();

            return true;
        }

        break;
    }

    case 1:
    {
        // Always repeatable.
        break;
    }

    case 2:
    {
        // Repeatable after cooldown_seconds.
        if (!interaction->cooldownSeconds)
        {
            sLog.outErrorDb(
                "Dynamic World Interaction %u uses repeat_type 2 "
                "but cooldown_seconds is 0.",
                interaction->id);

            SendPlayerMessage(
                player,
                "This interaction is temporarily unavailable.");

            gameObject->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
            gameObject->SetGoState(GO_STATE_READY);
            gameObject->SetLootState(GO_READY);
            gameObject->SetRespawnTime(0);
            gameObject->Refresh();

            return true;
        }

        if (hasProgress)
        {
            uint64 currentTime = uint64(std::time(nullptr));
            uint64 availableTime =
                progress.lastCompleted + interaction->cooldownSeconds;

            if (currentTime < availableTime)
            {
                uint64 remaining = availableTime - currentTime;

                SendPlayerMessage(
                    player,
                    "You can use this again in " +
                    FormatCooldownRemaining(remaining) +
                    ".");

                gameObject->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
                gameObject->SetGoState(GO_STATE_READY);
                gameObject->SetLootState(GO_READY);
                gameObject->SetRespawnTime(0);
                gameObject->Refresh();

                return true;
            }
        }

        break;
    }

    default:
    {
        sLog.outErrorDb(
            "Dynamic World Interaction %u has invalid repeat_type %u.",
            interaction->id,
            uint32(interaction->repeatType));

        SendPlayerMessage(
            player,
            "This interaction is not configured correctly.");

        gameObject->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
        gameObject->SetGoState(GO_STATE_READY);
        gameObject->SetLootState(GO_READY);
        gameObject->SetRespawnTime(0);
        gameObject->Refresh();

        return true;
    }
    }

    bool markComplete = false;
    bool actionFailed = false;
    bool gameObjectDespawned = false;

    for (DynamicWorldActionList::const_iterator itr =
        interaction->actions.begin();
        itr != interaction->actions.end();
        ++itr)
    {
        DynamicWorldAction const& action = *itr;

        if (action.chance < 100.0f && !roll_chance_f(action.chance))
            continue;

        // Delayed actions will be added later.
        // For now they execute immediately.
        switch (action.actionType)
        {

        case DYNAMIC_WORLD_ACTION_DESPAWN_GO:
        {
            uint32 respawnSeconds = 0;

            // value1 can override the interaction-wide active duration.
            if (action.value1 > 0)
                respawnSeconds = uint32(action.value1);
            else
                respawnSeconds = interaction->activeDurationSeconds;

            if (!respawnSeconds)
            {
                sLog.outErrorDb(
                    "Dynamic World Interaction %u DESPAWN_GO action "
                    "has no respawn duration.",
                    interaction->id);

                actionFailed = true;
                break;
            }

            // The blocker must be a normal database-spawned GameObject.
            if (!gameObject->GetGOData())
            {
                sLog.outErrorDb(
                    "Dynamic World Interaction %u attempted to despawn "
                    "a non-database GameObject GUID %u.",
                    interaction->id,
                    gameObject->GetGUIDLow());

                actionFailed = true;
                break;
            }

            // Use the requested duration instead of the spawn's normal timer.
            gameObject->SetRespawnDelay(respawnSeconds);
            gameObject->Despawn();
            gameObject->SaveRespawnTime();

            gameObjectDespawned = true;
            break;
        }

        case DYNAMIC_WORLD_ACTION_START_QUEST:
        {
            if (action.value1 <= 0)
                break;

            uint32 questId = uint32(action.value1);

            Quest const* quest =
                sObjectMgr.GetQuestTemplate(questId);

            if (!quest)
            {
                sLog.outErrorDb(
                    "Dynamic World Interaction %u references missing quest %u.",
                    interaction->id,
                    questId);

                actionFailed = true;
                break;
            }

            if (!player->CanTakeQuest(quest, true))
            {
                actionFailed = true;
                break;
            }

            if (!player->CanAddQuest(quest, true))
            {
                actionFailed = true;
                break;
            }

            // Open the normal quest details window.
            // The player must choose Accept or Decline.
            player->PlayerTalkClass->SendQuestGiverQuestDetails(
                quest,
                gameObject->GetObjectGuid(),
                true);

            break;
        }


        case DYNAMIC_WORLD_ACTION_CAST_SPELL:
        {
            if (action.value1 <= 0)
                break;

            player->CastSpell(
                player,
                uint32(action.value1),
                true);

            break;
        }
        case DYNAMIC_WORLD_ACTION_SEND_TEXT:
        {
            // During the first framework test, the action comment
            // is used as the player-facing message.
            if (!action.comment.empty())
                SendPlayerMessage(player, action.comment);

            break;
        }

        case DYNAMIC_WORLD_ACTION_GIVE_MONEY:
        {
            if (action.value1 <= 0)
                break;

            uint32 money = uint32(action.value1);

            if (player->GetMoney() > MAX_MONEY_AMOUNT - money)
            {
                SendPlayerMessage(
                    player,
                    "You cannot carry any more money.");

                break;
            }

            player->ModifyMoney(int32(money));
            break;
        }

        case DYNAMIC_WORLD_ACTION_GIVE_ITEM:
        {
            if (action.value1 <= 0 || action.value2 <= 0)
                break;

            uint32 itemEntry = uint32(action.value1);
            uint32 itemCount = uint32(action.value2);

            ItemPrototype const* itemProto =
                ObjectMgr::GetItemPrototype(itemEntry);

            if (!itemProto)
            {
                sLog.outErrorDb(
                    "Dynamic World Interaction %u GIVE_ITEM action "
                    "references missing item %u.",
                    interaction->id,
                    itemEntry);

                actionFailed = true;
                break;
            }

            ItemPosCountVec destination;
            InventoryResult result = player->CanStoreNewItem(
                NULL_BAG,
                NULL_SLOT,
                destination,
                itemEntry,
                itemCount);
            if (result != EQUIP_ERR_OK)
            {
                player->SendEquipError(
                    result,
                    nullptr,
                    nullptr,
                    itemEntry);

                actionFailed = true;
                break;
            }

            Item* item = player->StoreNewItem(
                destination,
                itemEntry,
                true,
                Item::GenerateItemRandomPropertyId(itemEntry));

            if (item)
                player->SendNewItem(item, itemCount, true, false);

            break;
        }

        case DYNAMIC_WORLD_ACTION_GIVE_XP:
        {
            if (action.value1 <= 0)
                break;

            if (player->GetLevel() >=
                sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
            {
                break;
            }

            player->GiveXP(uint32(action.value1), nullptr);
            break;
        }

        case DYNAMIC_WORLD_ACTION_MARK_COMPLETE:
        {
            markComplete = true;
            break;
        }

        default:
        {
            sLog.outErrorDb(
                "Dynamic World Interaction %u attempted unsupported "
                "action type %u.",
                interaction->id,
                uint32(action.actionType));

            break;
        }
        }
    }

    if (markComplete && !actionFailed)
    {
        SaveCharacterCompletion(characterGuid, interaction->id);
    }
    else if (actionFailed)
    {
        SendPlayerMessage(
            player,
            "The interaction could not be completed.");
    }

    // Dynamic World interactions must not consume or deactivate the
    // underlying GameObject. Keep it available for other players and
    // for repeat attempts by the same character.
    if (!gameObjectDespawned)
    {
        gameObject->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
        gameObject->SetGoState(GO_STATE_READY);
        gameObject->SetLootState(GO_READY);
        gameObject->SetRespawnTime(0);
        gameObject->Refresh();
    }
    // Registered dynamic objects do not continue into normal GO handling.
    return true;
}