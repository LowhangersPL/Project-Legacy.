/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "World.h"
#include "Player.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "SystemConfig.h"
#include "revision.h"
#include "Util.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

bool ChatHandler::HandleGameObjectTargetCommand(char* args)
{
    Player* pl = m_session->GetPlayer();
    QueryResult* result;
    GameEventMgr::ActiveEvents const& activeEventsList = sGameEventMgr.GetActiveEventList();
    if (*args)
    {
        // number or [name] Shift-click form |color|Hgameobject_entry:go_id|h[name]|h|r
        char* cId = ExtractKeyFromLink(&args, "Hgameobject_entry");
        if (!cId)
            return false;

        uint32 id;
        if (ExtractUInt32(&cId, id))
        {
            result = WorldDatabase.PQuery("SELECT guid, id, position_x, position_y, position_z, orientation, map, (POW(position_x - '%f', 2) + POW(position_y - '%f', 2) + POW(position_z - '%f', 2)) AS order_ FROM gameobject WHERE map = '%i' AND id = '%u' ORDER BY order_ ASC LIMIT 1",
                pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(), pl->GetMapId(), id);
        }
        else
        {
            std::string name = cId;
            WorldDatabase.escape_string(name);
            result = WorldDatabase.PQuery(
                "SELECT guid, id, position_x, position_y, position_z, orientation, map, (POW(position_x - %f, 2) + POW(position_y - %f, 2) + POW(position_z - %f, 2)) AS order_ "
                "FROM gameobject,gameobject_template WHERE gameobject_template.entry = gameobject.id AND map = %i AND name " _LIKE_ " " _CONCAT3_("'%%'", "'%s'", "'%%'")" ORDER BY order_ ASC LIMIT 1",
                pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(), pl->GetMapId(), name.c_str());
        }
    }
    else
    {
        std::ostringstream eventFilter;
        eventFilter << " AND (event IS NULL ";
        bool initString = true;

        for (GameEventMgr::ActiveEvents::const_iterator itr = activeEventsList.begin(); itr != activeEventsList.end(); ++itr)
        {
            if (initString)
            {
                eventFilter << "OR event IN (" << *itr;
                initString = false;
            }
            else
                eventFilter << "," << *itr;
        }

        if (!initString)
            eventFilter << "))";
        else
            eventFilter << ")";

        result = WorldDatabase.PQuery("SELECT gameobject.guid, id, position_x, position_y, position_z, orientation, map, "
            "(POW(position_x - %f, 2) + POW(position_y - %f, 2) + POW(position_z - %f, 2)) AS order_ FROM gameobject "
            "LEFT OUTER JOIN game_event_gameobject on gameobject.guid=game_event_gameobject.guid WHERE map = '%i' %s ORDER BY order_ ASC LIMIT 10",
            pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(), pl->GetMapId(), eventFilter.str().c_str());
    }

    if (!result)
    {
        SendSysMessage(LANG_COMMAND_TARGETOBJNOTFOUND);
        return true;
    }

    bool found = false;
    float x, y, z, o;
    uint32 lowguid, id;
    uint16 mapid, pool_id;

    do
    {
        Field* fields = result->Fetch();
        lowguid = fields[0].GetUInt32();
        id = fields[1].GetUInt32();
        x = fields[2].GetFloat();
        y = fields[3].GetFloat();
        z = fields[4].GetFloat();
        o = fields[5].GetFloat();
        mapid = fields[6].GetUInt16();
        pool_id = sPoolMgr.IsPartOfAPool<GameObject>(lowguid);
        if (!pool_id || pl->GetMap()->GetPersistentState()->IsSpawnedPoolObject<GameObject>(lowguid))
            found = true;
    } while (result->NextRow() && (!found));

    delete result;

    if (!found)
    {
        PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, id);
        return false;
    }

    GameObjectInfo const* goI = ObjectMgr::GetGameObjectInfo(id);

    if (!goI)
    {
        PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, id);
        return false;
    }

    GameObject* target = m_session->GetPlayer()->GetMap()->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, id, lowguid));

    PSendSysMessage(LANG_GAMEOBJECT_DETAIL, lowguid, goI->name, lowguid, id, x, y, z, mapid, o);

    if (target)
    {
        time_t curRespawnDelay = target->GetRespawnTimeEx() - time(nullptr);
        if (curRespawnDelay < 0)
            curRespawnDelay = 0;

        std::string curRespawnDelayStr = secsToTimeString(curRespawnDelay, true);
        std::string defRespawnDelayStr = secsToTimeString(target->GetRespawnDelay(), true);

        PSendSysMessage(LANG_COMMAND_RAWPAWNTIMES, defRespawnDelayStr.c_str(), curRespawnDelayStr.c_str());

        ShowNpcOrGoSpawnInformation<GameObject>(target->GetGUIDLow());
    }
    return true;
}

bool ChatHandler::HandleGameObjectInfoCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_id|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
        return false;

    if (!lowguid)
        return false;

    GameObject* pGameObject = nullptr;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
        pGameObject = GetGameObjectWithGuid(lowguid, go_data->id);

    if (!pGameObject)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage("Entry: %u, GUID: %u\nName: %s\nType: %u, Display Id: %u\nGO State: %u, Loot State: %u", pGameObject->GetEntry(), pGameObject->GetGUIDLow(), pGameObject->GetGOInfo()->name, pGameObject->GetGoType(), pGameObject->GetDisplayId(), pGameObject->GetGoState(), pGameObject->getLootState());
    SendSysMessage(pGameObject->isSpawned() ? "Object is spawned." : "Not spawned.");

    return true;
}

bool ChatHandler::HandleGameObjectTurnCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_id|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
        return false;

    if (!lowguid)
        return false;

    GameObject* obj = nullptr;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
        obj = GetGameObjectWithGuid(lowguid, go_data->id);

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    float o;
    if (!ExtractOptFloat(&args, o, m_session->GetPlayer()->GetOrientation()))
        return false;

    Map* map = obj->GetMap();
    map->Remove(obj, false);

    obj->Relocate(obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ(), o);
    obj->UpdateRotationFields();

    map->Add(obj);

    obj->SaveToDB();
    obj->Refresh();

    PSendSysMessage(LANG_COMMAND_TURNOBJMESSAGE, obj->GetGUIDLow(), obj->GetGOInfo()->name, obj->GetGUIDLow());

    return true;
}

bool ChatHandler::HandleGameObjectMoveCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_guid|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
        return false;

    if (!lowguid)
        return false;

    GameObject* obj = nullptr;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
        obj = GetGameObjectWithGuid(lowguid, go_data->id);

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    if (!args)
    {
        Player* chr = m_session->GetPlayer();

        Map* map = obj->GetMap();
        map->Remove(obj, false);

        obj->Relocate(chr->GetPositionX(), chr->GetPositionY(), chr->GetPositionZ(), obj->GetOrientation());
        obj->SetFloatValue(GAMEOBJECT_POS_X, chr->GetPositionX());
        obj->SetFloatValue(GAMEOBJECT_POS_Y, chr->GetPositionY());
        obj->SetFloatValue(GAMEOBJECT_POS_Z, chr->GetPositionZ());

        map->Add(obj);
    }
    else
    {
        float x;
        if (!ExtractFloat(&args, x))
            return false;

        float y;
        if (!ExtractFloat(&args, y))
            return false;

        float z;
        if (!ExtractFloat(&args, z))
            return false;

        if (!MapManager::IsValidMapCoord(obj->GetMapId(), x, y, z))
        {
            PSendSysMessage(LANG_INVALID_TARGET_COORD, x, y, obj->GetMapId());
            SetSentErrorMessage(true);
            return false;
        }

        Map* map = obj->GetMap();
        map->Remove(obj, false);

        obj->Relocate(x, y, z, obj->GetOrientation());
        obj->SetFloatValue(GAMEOBJECT_POS_X, x);
        obj->SetFloatValue(GAMEOBJECT_POS_Y, y);
        obj->SetFloatValue(GAMEOBJECT_POS_Z, z);

        map->Add(obj);
    }

    obj->SaveToDB();
    obj->Refresh();

    PSendSysMessage(LANG_COMMAND_MOVEOBJMESSAGE, obj->GetGUIDLow(), obj->GetGOInfo()->name, obj->GetGUIDLow());

    return true;
}

bool ChatHandler::HandleGameObjectDeleteCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_guid|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
        return false;

    if (!lowguid)
        return false;

    GameObject* obj = nullptr;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
        obj = GetGameObjectWithGuid(lowguid, go_data->id);

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    if (ObjectGuid ownerGuid = obj->GetOwnerGuid())
    {
        Unit* owner = ObjectAccessor::GetUnit(*m_session->GetPlayer(), ownerGuid);
        if (!owner || !ownerGuid.IsPlayer())
        {
            PSendSysMessage(LANG_COMMAND_DELOBJREFERCREATURE, obj->GetGUIDLow(), ownerGuid.GetString().c_str());
            SetSentErrorMessage(true);
            return false;
        }

        owner->RemoveGameObject(obj, false);
    }

    obj->SetRespawnTime(0);                                 // not save respawn time
    obj->Delete();
    obj->DeleteFromDB();

    PSendSysMessage(LANG_COMMAND_DELOBJMESSAGE, obj->GetGUIDLow());

    return true;
}

bool ChatHandler::HandleGameObjectAddCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject_entry:go_id|h[name]|h|r
    uint32 id;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject_entry", id))
        return false;

    if (!id)
        return false;

    int32 spawntimeSecs;
    if (!ExtractOptInt32(&args, spawntimeSecs, 0))
        return false;

    const GameObjectInfo* gInfo = ObjectMgr::GetGameObjectInfo(id);

    if (!gInfo)
    {
        PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, id);
        SetSentErrorMessage(true);
        return false;
    }

    if (gInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->displayId))
    {
        // report to DB errors log as in loading case
        sLog.outErrorDb("Gameobject (Entry %u GoType: %u) have invalid displayId (%u), not spawned.", id, gInfo->type, gInfo->displayId);
        PSendSysMessage(LANG_GAMEOBJECT_HAVE_INVALID_DATA, id);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = m_session->GetPlayer();
    float x = float(chr->GetPositionX());
    float y = float(chr->GetPositionY());
    float z = float(chr->GetPositionZ());
    float o = float(chr->GetOrientation());
    Map* map = chr->GetMap();

    GameObject* pGameObj = new GameObject;

    // used guids from specially reserved range (can be 0 if no free values)
    uint32 db_lowGUID = sObjectMgr.GenerateStaticGameObjectLowGuid();
    if (!db_lowGUID)
    {
        SendSysMessage(LANG_NO_FREE_STATIC_GUID_FOR_SPAWN);
        SetSentErrorMessage(true);
        return false;
    }

    if (!pGameObj->Create(db_lowGUID, gInfo->id, map, x, y, z, o, 0.0f, 0.0f, 0.0f, 0.0f, GO_ANIMPROGRESS_DEFAULT, GO_STATE_READY))
    {
        delete pGameObj;
        return false;
    }

    if (spawntimeSecs)
        pGameObj->SetRespawnTime(spawntimeSecs);

    // fill the gameobject data and save to the db
    pGameObj->SaveToDB(map->GetId());

    // this will generate a new guid if the object is in an instance
    if (!pGameObj->LoadFromDB(db_lowGUID, map))
    {
        delete pGameObj;
        return false;
    }

    DEBUG_LOG(GetMangosString(LANG_GAMEOBJECT_CURRENT), gInfo->name, db_lowGUID, x, y, z, o);

    map->Add(pGameObj);

    sObjectMgr.AddGameobjectToGrid(db_lowGUID, sObjectMgr.GetGOData(db_lowGUID));

    PSendSysMessage(LANG_GAMEOBJECT_ADD, id, gInfo->name, db_lowGUID, x, y, z);
    return true;
}

bool ChatHandler::HandleHousePlaceCommand(char* args)
{
    uint32 id;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject_entry", id) || !id)
        return false;

    Player* chr = m_session->GetPlayer();
    Map* map = chr->GetMap();

    float x = chr->GetPositionX();
    float y = chr->GetPositionY();
    float z = chr->GetPositionZ();
    float o = chr->GetOrientation();

    QueryResult* plotResult = WorldDatabase.PQuery(
        "SELECT plot_id FROM housing_plots "
        "WHERE enabled = 1 AND map = %u "
        "AND %f BETWEEN min_x AND max_x "
        "AND %f BETWEEN min_y AND max_y "
        "AND %f BETWEEN min_z AND max_z "
        "LIMIT 1",
        map->GetId(), x, y, z);

    if (!plotResult)
    {
        SendSysMessage("You must stand inside an enabled housing plot.");
        SetSentErrorMessage(true);
        return false;
    }

    uint32 plotId = plotResult->Fetch()[0].GetUInt32();
    delete plotResult;

    GameObjectInfo const* gInfo = ObjectMgr::GetGameObjectInfo(id);
    if (!gInfo)
    {
        PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, id);
        SetSentErrorMessage(true);
        return false;
    }

    if (gInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->displayId))
    {
        PSendSysMessage(LANG_GAMEOBJECT_HAVE_INVALID_DATA, id);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 dbLowGuid = sObjectMgr.GenerateStaticGameObjectLowGuid();
    if (!dbLowGuid)
    {
        SendSysMessage(LANG_NO_FREE_STATIC_GUID_FOR_SPAWN);
        SetSentErrorMessage(true);
        return false;
    }

    GameObject* gameObject = new GameObject;

    if (!gameObject->Create(dbLowGuid, gInfo->id, map, x, y, z, o,
        0.0f, 0.0f, 0.0f, 0.0f, GO_ANIMPROGRESS_DEFAULT, GO_STATE_READY))
    {
        delete gameObject;
        return false;
    }

    gameObject->SaveToDB(map->GetId());

    if (!gameObject->LoadFromDB(dbLowGuid, map))
    {
        delete gameObject;
        return false;
    }

    map->Add(gameObject);
    sObjectMgr.AddGameobjectToGrid(dbLowGuid, sObjectMgr.GetGOData(dbLowGuid));

    CharacterDatabase.PExecute(
        "INSERT INTO guild_housing_objects "
        "(gameobject_guid, guildid, plot_id, gameobject_entry, map, "
        "position_x, position_y, position_z, orientation, placed_by) "
        "VALUES (%u, 0, %u, %u, %u, %f, %f, %f, %f, %u)",
        dbLowGuid, plotId, id, map->GetId(), x, y, z, o, chr->GetGUIDLow());

    PSendSysMessage(
        "Housing object placed: entry %u, GUID %u, plot %u.",
        id, dbLowGuid, plotId);

    return true;
}

bool ChatHandler::HandleHouseDeleteCommand(char* args)
{
    uint32 gameObjectGuid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", gameObjectGuid) || !gameObjectGuid)
        return false;

    QueryResult* housingResult = CharacterDatabase.PQuery(
        "SELECT gameobject_guid FROM guild_housing_objects "
        "WHERE gameobject_guid = %u LIMIT 1",
        gameObjectGuid);

    if (!housingResult)
    {
        SendSysMessage("That is not a housing object.");
        SetSentErrorMessage(true);
        return false;
    }

    delete housingResult;

    GameObjectData const* goData = sObjectMgr.GetGOData(gameObjectGuid);
    if (!goData)
    {
        SendSysMessage("Housing object spawn data was not found.");
        SetSentErrorMessage(true);
        return false;
    }

    GameObject* gameObject = GetGameObjectWithGuid(gameObjectGuid, goData->id);
    if (!gameObject)
    {
        SendSysMessage("Stand near the housing object before deleting it.");
        SetSentErrorMessage(true);
        return false;
    }

    gameObject->SetRespawnTime(0);
    gameObject->Delete();
    gameObject->DeleteFromDB();

    CharacterDatabase.PExecute(
        "DELETE FROM guild_housing_objects WHERE gameobject_guid = %u",
        gameObjectGuid);

    PSendSysMessage("Housing object %u deleted.", gameObjectGuid);
    return true;
}

bool ChatHandler::HandleHouseMoveCommand(char* args)
{
    uint32 gameObjectGuid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", gameObjectGuid) || !gameObjectGuid)
        return false;

    QueryResult* housingResult = CharacterDatabase.PQuery(
        "SELECT plot_id FROM guild_housing_objects WHERE gameobject_guid = %u LIMIT 1",
        gameObjectGuid);

    if (!housingResult)
    {
        SendSysMessage("That is not a housing object.");
        SetSentErrorMessage(true);
        return false;
    }

    uint32 plotId = housingResult->Fetch()[0].GetUInt32();
    delete housingResult;

    GameObjectData const* goData = sObjectMgr.GetGOData(gameObjectGuid);
    GameObject* gameObject = goData ? GetGameObjectWithGuid(gameObjectGuid, goData->id) : nullptr;

    if (!gameObject)
    {
        SendSysMessage("Stand near the housing object before moving it.");
        SetSentErrorMessage(true);
        return false;
    }

    float x;
    float y;
    float z;

    if (!args || !*args)
    {
        Player* chr = m_session->GetPlayer();
        x = chr->GetPositionX();
        y = chr->GetPositionY();
        z = chr->GetPositionZ();
    }
    else
    {
        if (!ExtractFloat(&args, x) || !ExtractFloat(&args, y) || !ExtractFloat(&args, z))
            return false;
    }

    if (!MapManager::IsValidMapCoord(gameObject->GetMapId(), x, y, z))
    {
        PSendSysMessage(LANG_INVALID_TARGET_COORD, x, y, gameObject->GetMapId());
        SetSentErrorMessage(true);
        return false;
    }

    QueryResult* plotCheck = WorldDatabase.PQuery(
        "SELECT plot_id FROM housing_plots "
        "WHERE enabled = 1 AND plot_id = %u AND map = %u "
        "AND %f BETWEEN min_x AND max_x "
        "AND %f BETWEEN min_y AND max_y "
        "AND %f BETWEEN min_z AND max_z LIMIT 1",
        plotId, gameObject->GetMapId(), x, y, z);

    if (!plotCheck)
    {
        SendSysMessage("That position is outside this housing plot.");
        SetSentErrorMessage(true);
        return false;
    }

    delete plotCheck;

    Map* map = gameObject->GetMap();
    map->Remove(gameObject, false);

    gameObject->Relocate(x, y, z, gameObject->GetOrientation());
    gameObject->SetFloatValue(GAMEOBJECT_POS_X, x);
    gameObject->SetFloatValue(GAMEOBJECT_POS_Y, y);
    gameObject->SetFloatValue(GAMEOBJECT_POS_Z, z);

    map->Add(gameObject);
    gameObject->SaveToDB();
    gameObject->Refresh();

    CharacterDatabase.PExecute(
        "UPDATE guild_housing_objects "
        "SET position_x = %f, position_y = %f, position_z = %f "
        "WHERE gameobject_guid = %u",
        x, y, z, gameObjectGuid);

    PSendSysMessage("Housing object %u moved.", gameObjectGuid);
    return true;
}

bool ChatHandler::HandleHouseTurnCommand(char* args)
{
    uint32 gameObjectGuid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", gameObjectGuid) || !gameObjectGuid)
        return false;

    float orientation;
    if (!ExtractFloat(&args, orientation))
        return false;

    QueryResult* housingResult = CharacterDatabase.PQuery(
        "SELECT gameobject_guid FROM guild_housing_objects WHERE gameobject_guid = %u LIMIT 1",
        gameObjectGuid);

    if (!housingResult)
    {
        SendSysMessage("That is not a housing object.");
        SetSentErrorMessage(true);
        return false;
    }

    delete housingResult;

    GameObjectData const* goData = sObjectMgr.GetGOData(gameObjectGuid);
    GameObject* gameObject = goData ? GetGameObjectWithGuid(gameObjectGuid, goData->id) : nullptr;

    if (!gameObject)
    {
        SendSysMessage("Stand near the housing object before rotating it.");
        SetSentErrorMessage(true);
        return false;
    }

    Map* map = gameObject->GetMap();
    map->Remove(gameObject, false);

    gameObject->Relocate(
        gameObject->GetPositionX(),
        gameObject->GetPositionY(),
        gameObject->GetPositionZ(),
        orientation);

    gameObject->UpdateRotationFields();

    map->Add(gameObject);
    gameObject->SaveToDB();
    gameObject->Refresh();

    CharacterDatabase.PExecute(
        "UPDATE guild_housing_objects SET orientation = %f WHERE gameobject_guid = %u",
        orientation, gameObjectGuid);

    PSendSysMessage("Housing object %u rotated.", gameObjectGuid);
    return true;
}

bool ChatHandler::HandleHouseScaleCommand(char* args)
{
    uint32 gameObjectGuid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", gameObjectGuid) || !gameObjectGuid)
        return false;

    float scale;
    if (!ExtractFloat(&args, scale))
        return false;

    if (scale < 0.25f || scale > 3.0f)
    {
        SendSysMessage("Housing scale must be between 0.25 and 3.0.");
        SetSentErrorMessage(true);
        return false;
    }

    QueryResult* housingResult = CharacterDatabase.PQuery(
        "SELECT gameobject_guid FROM guild_housing_objects WHERE gameobject_guid = %u LIMIT 1",
        gameObjectGuid);

    if (!housingResult)
    {
        SendSysMessage("That is not a housing object.");
        SetSentErrorMessage(true);
        return false;
    }

    delete housingResult;

    GameObjectData const* goData = sObjectMgr.GetGOData(gameObjectGuid);
    GameObject* gameObject = goData ? GetGameObjectWithGuid(gameObjectGuid, goData->id) : nullptr;

    if (!gameObject)
    {
        SendSysMessage("Stand near the housing object before scaling it.");
        SetSentErrorMessage(true);
        return false;
    }

    gameObject->SetObjectScale(scale);
    gameObject->SaveToDB();
    gameObject->Refresh();

    CharacterDatabase.PExecute(
        "UPDATE guild_housing_objects SET scale = %f WHERE gameobject_guid = %u",
        scale, gameObjectGuid);

    PSendSysMessage("Housing object %u scaled to %.2f.", gameObjectGuid, scale);
    return true;
}

bool ChatHandler::HandleHouseListCommand(char* /*args*/)
{
    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT gameobject_guid, gameobject_entry, position_x, position_y, position_z, orientation, scale "
        "FROM guild_housing_objects WHERE plot_id = 1 ORDER BY id LIMIT 100");

    if (!result)
    {
        SendSysMessage("No housing objects have been placed in Plot 1.");
        return true;
    }

    SendSysMessage("Housing objects in Plot 1:");

    do
    {
        Field* fields = result->Fetch();

        PSendSysMessage(
            "GUID %u | Entry %u | X %.2f Y %.2f Z %.2f | Rotation %.2f | Scale %.2f",
            fields[0].GetUInt32(),
            fields[1].GetUInt32(),
            fields[2].GetFloat(),
            fields[3].GetFloat(),
            fields[4].GetFloat(),
            fields[5].GetFloat(),
            fields[6].GetFloat());
    } while (result->NextRow());

    delete result;
    return true;
}

bool ChatHandler::HandleHouseNudgeCommand(char* args)
{
    float deltaX;
    float deltaY;
    float deltaZ;

    if (!ExtractFloat(&args, deltaX) || !ExtractFloat(&args, deltaY) || !ExtractFloat(&args, deltaZ))
        return false;

    GameObject* gameObject = getSelectedGameObject();
    if (!gameObject)
    {
        SendSysMessage("Select a nearby housing object first.");
        SetSentErrorMessage(true);
        return false;
    }

    uint32 gameObjectGuid = gameObject->GetDBTableGUIDLow();
    QueryResult* housingResult = CharacterDatabase.PQuery(
        "SELECT plot_id FROM guild_housing_objects WHERE gameobject_guid = %u LIMIT 1",
        gameObjectGuid);

    if (!housingResult)
    {
        SendSysMessage("The selected object is not a housing object.");
        SetSentErrorMessage(true);
        return false;
    }

    uint32 plotId = housingResult->Fetch()[0].GetUInt32();
    delete housingResult;

    float x = gameObject->GetPositionX() + deltaX;
    float y = gameObject->GetPositionY() + deltaY;
    float z = gameObject->GetPositionZ() + deltaZ;

    QueryResult* plotCheck = WorldDatabase.PQuery(
        "SELECT plot_id FROM housing_plots "
        "WHERE enabled = 1 AND plot_id = %u AND map = %u "
        "AND %f BETWEEN min_x AND max_x "
        "AND %f BETWEEN min_y AND max_y "
        "AND %f BETWEEN min_z AND max_z LIMIT 1",
        plotId, gameObject->GetMapId(), x, y, z);

    if (!plotCheck)
    {
        SendSysMessage("That move would leave this housing plot.");
        SetSentErrorMessage(true);
        return false;
    }

    delete plotCheck;

    Map* map = gameObject->GetMap();
    map->Remove(gameObject, false);

    gameObject->Relocate(x, y, z, gameObject->GetOrientation());
    gameObject->SetFloatValue(GAMEOBJECT_POS_X, x);
    gameObject->SetFloatValue(GAMEOBJECT_POS_Y, y);
    gameObject->SetFloatValue(GAMEOBJECT_POS_Z, z);

    map->Add(gameObject);
    gameObject->SaveToDB();
    gameObject->Refresh();

    CharacterDatabase.PExecute(
        "UPDATE guild_housing_objects "
        "SET position_x = %f, position_y = %f, position_z = %f "
        "WHERE gameobject_guid = %u",
        x, y, z, gameObjectGuid);

    return true;
}

bool ChatHandler::HandleHouseRotateCommand(char* args)
{
    float amount;
    if (!ExtractFloat(&args, amount))
        return false;

    GameObject* gameObject = getSelectedGameObject();
    if (!gameObject)
    {
        SendSysMessage("Select a nearby housing object first.");
        SetSentErrorMessage(true);
        return false;
    }

    uint32 gameObjectGuid = gameObject->GetDBTableGUIDLow();
    QueryResult* housingResult = CharacterDatabase.PQuery(
        "SELECT gameobject_guid FROM guild_housing_objects WHERE gameobject_guid = %u LIMIT 1",
        gameObjectGuid);

    if (!housingResult)
    {
        SendSysMessage("The selected object is not a housing object.");
        SetSentErrorMessage(true);
        return false;
    }

    delete housingResult;

    float orientation = gameObject->GetOrientation() + amount;
    Map* map = gameObject->GetMap();
    map->Remove(gameObject, false);

    gameObject->Relocate(
        gameObject->GetPositionX(),
        gameObject->GetPositionY(),
        gameObject->GetPositionZ(),
        orientation);

    gameObject->UpdateRotationFields();
    map->Add(gameObject);
    gameObject->SaveToDB();
    gameObject->Refresh();

    CharacterDatabase.PExecute(
        "UPDATE guild_housing_objects SET orientation = %f WHERE gameobject_guid = %u",
        orientation, gameObjectGuid);

    return true;
}

bool ChatHandler::HandleHouseScaleShiftCommand(char* args)
{
    float amount;
    if (!ExtractFloat(&args, amount))
        return false;

    GameObject* gameObject = getSelectedGameObject();
    if (!gameObject)
    {
        SendSysMessage("Select a nearby housing object first.");
        SetSentErrorMessage(true);
        return false;
    }

    uint32 gameObjectGuid = gameObject->GetDBTableGUIDLow();
    QueryResult* housingResult = CharacterDatabase.PQuery(
        "SELECT gameobject_guid FROM guild_housing_objects WHERE gameobject_guid = %u LIMIT 1",
        gameObjectGuid);

    if (!housingResult)
    {
        SendSysMessage("The selected object is not a housing object.");
        SetSentErrorMessage(true);
        return false;
    }

    delete housingResult;

    float scale = gameObject->GetObjectScale() + amount;
    if (scale < 0.25f || scale > 3.0f)
    {
        SendSysMessage("Housing scale must stay between 0.25 and 3.0.");
        SetSentErrorMessage(true);
        return false;
    }

    gameObject->SetObjectScale(scale);
    gameObject->SaveToDB();
    gameObject->Refresh();

    CharacterDatabase.PExecute(
        "UPDATE guild_housing_objects SET scale = %f WHERE gameobject_guid = %u",
        scale, gameObjectGuid);

    return true;
}

bool ChatHandler::HandleHouseRemoveSelectedCommand(char* /*args*/)
{
    GameObject* gameObject = getSelectedGameObject();
    if (!gameObject)
    {
        SendSysMessage("Select a nearby housing object first.");
        SetSentErrorMessage(true);
        return false;
    }

    uint32 gameObjectGuid = gameObject->GetDBTableGUIDLow();
    QueryResult* housingResult = CharacterDatabase.PQuery(
        "SELECT gameobject_guid FROM guild_housing_objects WHERE gameobject_guid = %u LIMIT 1",
        gameObjectGuid);

    if (!housingResult)
    {
        SendSysMessage("The selected object is not a housing object.");
        SetSentErrorMessage(true);
        return false;
    }

    delete housingResult;

    gameObject->SetRespawnTime(0);
    gameObject->Delete();
    gameObject->DeleteFromDB();

    CharacterDatabase.PExecute(
        "DELETE FROM guild_housing_objects WHERE gameobject_guid = %u",
        gameObjectGuid);

    return true;
}

bool ChatHandler::HandleGameObjectTempAddCommand(char* args)
{
    uint32 id;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject_entry", id))
        return false;

    if (!id)
        return false;

    Player* chr = m_session->GetPlayer();

    char* spawntime = strtok(nullptr, " ");
    uint32 spawntm = 300;

    if (spawntime)
        spawntm = atoi((char*)spawntime);

    float x = chr->GetPositionX();
    float y = chr->GetPositionY();
    float z = chr->GetPositionZ();
    float ang = chr->GetOrientation();

    float rot2 = sin(ang / 2);
    float rot3 = cos(ang / 2);

    if (!chr->SummonGameObject(id, x, y, z, ang, 0, 0, rot2, rot3, spawntm))
        return false;
    return true;
}

bool ChatHandler::HandleGameObjectNearCommand(char* args)
{
    float distance;
    if (!ExtractOptFloat(&args, distance, 10.0f))
        return false;

    uint32 count = 0;

    Player* pl = m_session->GetPlayer();
    QueryResult* result = WorldDatabase.PQuery("SELECT guid, id, position_x, position_y, position_z, map, "
        "(POW(position_x - '%f', 2) + POW(position_y - '%f', 2) + POW(position_z - '%f', 2)) AS order_ "
        "FROM gameobject WHERE map='%u' AND (POW(position_x - '%f', 2) + POW(position_y - '%f', 2) + POW(position_z - '%f', 2)) <= '%f' ORDER BY order_",
        pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(),
        pl->GetMapId(), pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(), distance * distance);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 guid = fields[0].GetUInt32();
            uint32 entry = fields[1].GetUInt32();
            float x = fields[2].GetFloat();
            float y = fields[3].GetFloat();
            float z = fields[4].GetFloat();
            int mapid = fields[5].GetUInt16();

            GameObjectInfo const* gInfo = ObjectMgr::GetGameObjectInfo(entry);

            if (!gInfo)
                continue;

            PSendSysMessage(LANG_GO_MIXED_LIST_CHAT, guid, PrepareStringNpcOrGoSpawnInformation<GameObject>(guid).c_str(), entry, guid, gInfo->name, x, y, z, mapid);

            ++count;
        } while (result->NextRow());

        delete result;
    }

    PSendSysMessage(LANG_COMMAND_NEAROBJMESSAGE, distance, count);
    return true;
}

class NearestGameObjectInObjectRangeCheck
{
public:
    NearestGameObjectInObjectRangeCheck(WorldObject const& obj, float range) : i_obj(obj), i_range(range) {}
    WorldObject const& GetFocusObject() const { return i_obj; }
    bool operator()(GameObject* go)
    {
        if (i_obj.IsWithinDistInMap(go, i_range))
        {
            i_range = i_obj.GetDistance(go);        // use found GO range as new range limit for next check
            return true;
        }
        return false;
    }
    float GetLastRange() const { return i_range; }
private:
    WorldObject const& i_obj;
    float  i_range;
};


bool ChatHandler::HandleGameObjectSelectCommand(char*)
{
    const float dist = 10.0f;
    Player* player = m_session->GetPlayer();

    GameObject* go = nullptr;
    CellPair pair(MaNGOS::ComputeCellPair(player->GetPositionX(), player->GetPositionY()));
    Cell cell(pair);
    cell.SetNoCreate();

    NearestGameObjectInObjectRangeCheck go_check(*player, dist);
    MaNGOS::GameObjectLastSearcher<NearestGameObjectInObjectRangeCheck> searcher(go, go_check);

    TypeContainerVisitor<MaNGOS::GameObjectLastSearcher<NearestGameObjectInObjectRangeCheck>, GridTypeMapContainer> go_searcher(searcher);
    cell.Visit(pair, go_searcher, *(player->GetMap()), *player, dist);
    if (go)
    {
        player->SetSelectedGobj(go->GetObjectGuid());
        GameObjectInfo const* data = go->GetGOInfo();
        PSendSysMessage(LANG_GO_MIXED_LIST_CHAT, go->GetDBTableGUIDLow(), PrepareStringNpcOrGoSpawnInformation<GameObject>(go->GetObjectGuid()).c_str(), go->GetEntry(), go->GetDBTableGUIDLow(), data ? data->name : go->GetName(), go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), go->GetMapId());
    }
    else
        SendSysMessage(LANG_COMMAND_NOGAMEOBJECTFOUND);
    return true;
}

GameObject* ChatHandler::getSelectedGameObject()
{
    Player* player = m_session->GetPlayer();
    return player->GetMap()->GetGameObject(player->GetSelectedGobj());
}

bool ChatHandler::HandleHouseSelectCommand(char* /*args*/)
{
    const float distance = 10.0f;
    Player* player = m_session->GetPlayer();

    GameObject* gameObject = nullptr;
    CellPair pair(MaNGOS::ComputeCellPair(player->GetPositionX(), player->GetPositionY()));
    Cell cell(pair);
    cell.SetNoCreate();

    NearestGameObjectInObjectRangeCheck gameObjectCheck(*player, distance);
    MaNGOS::GameObjectLastSearcher<NearestGameObjectInObjectRangeCheck> searcher(gameObject, gameObjectCheck);
    TypeContainerVisitor<MaNGOS::GameObjectLastSearcher<NearestGameObjectInObjectRangeCheck>, GridTypeMapContainer> visitor(searcher);
    cell.Visit(pair, visitor, *(player->GetMap()), *player, distance);

    if (!gameObject)
    {
        SendSysMessage("No nearby gameobject was found.");
        SetSentErrorMessage(true);
        return false;
    }

    uint32 gameObjectGuid = gameObject->GetDBTableGUIDLow();
    QueryResult* housingResult = CharacterDatabase.PQuery(
        "SELECT gameobject_guid FROM guild_housing_objects WHERE gameobject_guid = %u LIMIT 1",
        gameObjectGuid);

    if (!housingResult)
    {
        SendSysMessage("The nearest gameobject is not a housing object. Stand closer to the decoration you want to edit.");
        SetSentErrorMessage(true);
        return false;
    }

    delete housingResult;
    player->SetSelectedGobj(gameObject->GetObjectGuid());

    PSendSysMessage("Housing object selected: %s (GUID %u).", gameObject->GetName(), gameObjectGuid);
    return true;
}

bool ChatHandler::HandleGameObjectDespawnCommand(char*)
{
    GameObject* go = getSelectedGameObject();
    if (!go)
    {
        SendSysMessage(LANG_COMMAND_NOGAMEOBJECTFOUND);
        return false;
    }
    go->Despawn();
    return true;
}

bool ChatHandler::HandleGameObjectRespawnCommand(char*)
{
    GameObject* go = getSelectedGameObject();
    if (!go)
    {
        SendSysMessage(LANG_COMMAND_NOGAMEOBJECTFOUND);
        return false;
    }
    go->Respawn();
    return true;
}

bool ChatHandler::HandleGameObjectToggleCommand(char* args)
{
    uint32 lowguid;
    GameObject* obj = nullptr;

    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
    {
        // no args passed, search for already selected GO
        obj = getSelectedGameObject();
        if (!obj)
        {
            PSendSysMessage(LANG_COMMAND_NOGAMEOBJECTFOUND);
            SetSentErrorMessage(true);
            return false;
        }
    }
    else
    {
        // lowguid arg found, search DB for GO
        if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
            obj = GetGameObjectWithGuid(lowguid, go_data->id);

        if (!obj)
        {
            PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }
    }

    if (obj->getLootState() == GO_READY || obj->getLootState() == GO_JUST_DEACTIVATED)
        obj->UseDoorOrButton();
    else
        obj->ResetDoorOrButton();

    return true;
}

bool ChatHandler::HandleGameObjectResetCommand(char*)
{
    GameObject* go = getSelectedGameObject();
    if (!go)
    {
        SendSysMessage(LANG_COMMAND_NOGAMEOBJECTFOUND);
        return false;
    }
    go->ResetDoorOrButton();
    return true;
}

bool ChatHandler::HandleGameObjectSetGoStateCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_id|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
        return false;

    if (!lowguid)
        return false;

    GameObject* pGameObject = nullptr;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
        pGameObject = GetGameObjectWithGuid(lowguid, go_data->id);

    if (!pGameObject)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 uiGoState;
    if (!ExtractUInt32(&args, uiGoState))
        return false;

    if (uiGoState > GO_STATE_ACTIVE_ALTERNATIVE)
        return false;

    pGameObject->SetGoState(GOState(uiGoState));
    PSendSysMessage("You changed GO State of %s (GUID %u) to %u.", pGameObject->GetName(), pGameObject->GetGUIDLow(), uiGoState);

    return true;
}

bool ChatHandler::HandleGameObjectSetLootStateCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_id|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
        return false;

    if (!lowguid)
        return false;

    GameObject* pGameObject = nullptr;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
        pGameObject = GetGameObjectWithGuid(lowguid, go_data->id);

    if (!pGameObject)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 uiLootState;
    if (!ExtractUInt32(&args, uiLootState))
        return false;

    if (uiLootState > GO_JUST_DEACTIVATED)
        return false;

    pGameObject->SetLootState(LootState(uiLootState));
    PSendSysMessage("You changed Loot State of %s (GUID %u) to %u.", pGameObject->GetName(), pGameObject->GetGUIDLow(), uiLootState);

    return true;
}
