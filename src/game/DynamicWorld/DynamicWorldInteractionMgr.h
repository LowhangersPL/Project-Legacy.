#pragma once

#include "Common.h"
#include "Policies/Singleton.h"

#include <string>
#include <unordered_map>
#include <vector>

class GameObject;
class Player;

enum DynamicWorldActionType
{
    DYNAMIC_WORLD_ACTION_GIVE_ITEM = 0,
    DYNAMIC_WORLD_ACTION_GIVE_MONEY = 1,
    DYNAMIC_WORLD_ACTION_CAST_SPELL = 2,
    DYNAMIC_WORLD_ACTION_START_QUEST = 3,
    DYNAMIC_WORLD_ACTION_SPAWN_CREATURE = 4,
    DYNAMIC_WORLD_ACTION_SPAWN_GAMEOBJECT = 5,
    DYNAMIC_WORLD_ACTION_SET_GO_STATE = 6,
    DYNAMIC_WORLD_ACTION_DESPAWN_GO = 7,
    DYNAMIC_WORLD_ACTION_SEND_TEXT = 8,
    DYNAMIC_WORLD_ACTION_OPEN_LOOT = 9,
    DYNAMIC_WORLD_ACTION_MARK_COMPLETE = 10,
    DYNAMIC_WORLD_ACTION_GIVE_XP = 11
};

struct DynamicWorldRotation
{
    uint32 id;
    std::string name;
    uint16 poolEntry;
    uint32 rotationSeconds;
    uint64 lastRotation;
    uint64 nextRotation;
    bool enabled;
    uint32 flags;
    std::string comment;
};

struct DynamicWorldAction
{
    uint8 actionOrder;
    uint8 actionType;
    uint8 targetType;
    int32 value1;
    int32 value2;
    int32 value3;
    int32 value4;
    float chance;
    uint32 delayMs;
    std::string comment;
};

typedef std::vector<DynamicWorldAction> DynamicWorldActionList;
typedef std::unordered_map<uint32, DynamicWorldRotation> DynamicWorldRotationMap;

struct DynamicWorldInteraction
{
    uint32 id;
    std::string name;
    uint8 interactionType;
    uint32 conditionEntry;
    uint8 repeatType;
    uint32 cooldownSeconds;
    uint32 activeDurationSeconds;
    uint32 flags;
    bool enabled;
    std::string scriptName;
    std::string comment;

    DynamicWorldActionList actions;
};

struct DynamicWorldCharacterProgress
{
    uint32 completionCount;
    uint64 firstCompleted;
    uint64 lastCompleted;

    DynamicWorldCharacterProgress()
        : completionCount(0),
        firstCompleted(0),
        lastCompleted(0)
    {}
};

class DynamicWorldInteractionMgr
{
public:
    DynamicWorldInteractionMgr();

    void Load();
    void LoadRotations();
    void Update(uint32 diff);

    DynamicWorldInteraction const* GetInteraction(uint32 interactionId) const;
    DynamicWorldInteraction const* GetInteractionForGameObject(uint32 guid) const;

    bool IsDynamicWorldGameObject(uint32 guid) const;

    // Returns true when this GameObject belongs to the dynamic system,
    // even if use is denied because it was already completed.
    bool HandleInteraction(Player* player, GameObject* gameObject);
    bool IsDynamicWorldQuestStarter(GameObject* gameObject, uint32 questId) const;

    void HandleQuestAccepted(
        Player* player,
        GameObject* gameObject,
        uint32 questId);

private:
    typedef std::unordered_map<uint32, DynamicWorldInteraction> InteractionMap;
    typedef std::unordered_map<uint32, uint32> GameObjectInteractionMap;

    std::string GetConditionFailureText(
        uint32 conditionId,
        Player* player,
        GameObject* gameObject) const;

    std::string DescribeCondition(uint32 conditionId) const;

    std::string FormatCooldownRemaining(uint64 seconds) const;


    bool GetCharacterProgress(
        uint32 characterGuid,
        uint32 interactionId,
        DynamicWorldCharacterProgress& progress) const;
    void SaveCharacterCompletion(uint32 characterGuid, uint32 interactionId);
    void SendPlayerMessage(Player* player, std::string const& message) const;

    InteractionMap m_interactions;
    GameObjectInteractionMap m_gameObjectInteractions;

    void RotatePool(DynamicWorldRotation& rotation);

    DynamicWorldRotationMap m_rotations;
    uint32 m_rotationCheckTimer;
};

#define sDynamicWorldInteractionMgr MaNGOS::Singleton<DynamicWorldInteractionMgr>::Instance()