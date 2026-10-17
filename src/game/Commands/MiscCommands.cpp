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
#include "Opcodes.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "SystemConfig.h"
#include "revision.h"
#include "Util.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Map.h"
#include "Mail.h"
#include "MassMailMgr.h"
#include "InstanceData.h"
#include "BattleGroundMgr.h"
#include <sstream>

static uint32 GetGuildBankDay()
{
    return uint32(time(nullptr) / DAY);
}

static std::string FormatGuildBankMoney(uint64 copper)
{
    uint64 gold = copper / 10000;
    uint64 silver = (copper % 10000) / 100;
    uint64 copperLeft = copper % 100;

    std::ostringstream ss;

    if (gold > 0)
        ss << gold << "g ";

    if (silver > 0 || gold > 0)
        ss << silver << "s ";

    ss << copperLeft << "c";

    return ss.str();
}

static bool GuildBankHasTabPermission(uint32 guildId, uint32 rankId, uint32 tabId, const char* column)
{
    // Guild Master always has full access.
    if (rankId == GR_GUILDMASTER)
        return true;

    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT %s FROM guild_bank_right "
        "WHERE guildid='%u' AND rankid='%u' AND tabid='%u'",
        column,
        guildId,
        rankId,
        tabId
    );

    if (!result)
        return false;

    bool allowed = result->Fetch()[0].GetUInt8() != 0;
    delete result;

    return allowed;
}

static void GuildBankWriteLog(
    uint32 guildId,
    uint32 playerGuid,
    uint32 action,
    uint32 tabId,
    uint32 slot,
    uint32 itemGuid,
    uint32 itemEntry,
    uint32 itemCount,
    uint64 money)
{
    CharacterDatabase.PExecute(
        "INSERT INTO guild_bank_log "
        "(guildid,player_guid,action,tabid,slot,item_guid,item_entry,item_count,money,timestamp) "
        "VALUES ('%u','%u','%u','%u','%u','%u','%u','%u','" UI64FMTD "','%u')",
        guildId,
        playerGuid,
        action,
        tabId,
        slot,
        itemGuid,
        itemEntry,
        itemCount,
        money,
        uint32(time(nullptr))
    );
}

bool ChatHandler::HandleGuildBankSetPermCommand(char* args)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player || !player->GetGuildId())
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    if (player->GetRank() != 0)
    {
        SendSysMessage("Only the Guild Master can change guild bank permissions.");
        return true;
    }

    uint32 rankId = 0;
    uint32 tabId = 0;
    uint32 view = 0;
    uint32 deposit = 0;
    uint32 withdraw = 0;
    uint32 dailyLimit = 0;

    if (!args || sscanf(
        args,
        "%u %u %u %u %u %u",
        &rankId,
        &tabId,
        &view,
        &deposit,
        &withdraw,
        &dailyLimit) != 6)
    {
        SendSysMessage(
            "Usage: .guild bank setperm <rank> <tab> "
            "<view 0/1> <deposit 0/1> <withdraw 0/1> <daily limit>"
        );
        return true;
    }

    if (rankId >= 10 || tabId >= 6 || view > 1 || deposit > 1 || withdraw > 1)
    {
        SendSysMessage("Invalid permission values.");
        return true;
    }

    uint32 guildId = player->GetGuildId();

    QueryResult* rankResult = CharacterDatabase.PQuery(
        "SELECT 1 FROM guild_rank WHERE guildid='%u' AND rid='%u'",
        guildId,
        rankId
    );

    if (!rankResult)
    {
        SendSysMessage("That guild rank does not exist.");
        return true;
    }

    delete rankResult;

    QueryResult* bankResult = CharacterDatabase.PQuery(
        "SELECT purchased_tabs FROM guild_bank WHERE guildid='%u'",
        guildId
    );

    if (!bankResult)
    {
        SendSysMessage("Guild bank does not exist.");
        return true;
    }

    uint32 purchasedTabs = bankResult->Fetch()[0].GetUInt32();
    delete bankResult;

    if (tabId >= purchasedTabs)
    {
        SendSysMessage("That guild bank tab has not been purchased.");
        return true;
    }

    CharacterDatabase.PExecute(
        "REPLACE INTO guild_bank_right "
        "(guildid,rankid,tabid,can_view,can_deposit,can_withdraw,daily_withdraw_limit) "
        "VALUES ('%u','%u','%u','%u','%u','%u','%u')",
        guildId,
        rankId,
        tabId,
        view,
        deposit,
        withdraw,
        dailyLimit
    );

    PSendSysMessage(
        "Bank permissions updated: rank %u tab %u view=%u deposit=%u withdraw=%u daily=%u.",
        rankId,
        tabId,
        view,
        deposit,
        withdraw,
        dailyLimit
    );

    return true;
}

bool ChatHandler::HandleGuildBankSetMoneyPermCommand(char* args)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player || !player->GetGuildId())
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    if (player->GetRank() != 0)
    {
        SendSysMessage("Only the Guild Master can change guild bank permissions.");
        return true;
    }

    uint32 rankId = 0;
    uint32 deposit = 0;
    uint32 withdraw = 0;
    unsigned long long dailyLimit = 0;

    if (!args || sscanf(
        args,
        "%u %u %u %llu",
        &rankId,
        &deposit,
        &withdraw,
        &dailyLimit) != 4)
    {
        SendSysMessage(
            "Usage: .guild bank setmoneyperm <rank> "
            "<deposit 0/1> <withdraw 0/1> <daily copper limit>"
        );
        return true;
    }

    if (rankId >= 10 || deposit > 1 || withdraw > 1)
    {
        SendSysMessage("Invalid permission values.");
        return true;
    }

    uint32 guildId = player->GetGuildId();

    QueryResult* rankResult = CharacterDatabase.PQuery(
        "SELECT 1 FROM guild_rank WHERE guildid='%u' AND rid='%u'",
        guildId,
        rankId
    );

    if (!rankResult)
    {
        SendSysMessage("That guild rank does not exist.");
        return true;
    }

    delete rankResult;

    CharacterDatabase.PExecute(
        "REPLACE INTO guild_bank_money_right "
        "(guildid,rankid,can_deposit_money,can_withdraw_money,daily_money_limit) "
        "VALUES ('%u','%u','%u','%u','" UI64FMTD "')",
        guildId,
        rankId,
        deposit,
        withdraw,
        uint64(dailyLimit)
    );

    PSendSysMessage(
        "Guild bank money permissions updated for rank %u.",
        rankId
    );

    return true;
}

bool ChatHandler::HandleGuildBankDepositCommand(char* args)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player)
        return false;

    uint32 guildId = player->GetGuildId();
    uint32 rankId = player->GetRank();

    if (!guildId)
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    uint32 tabId = 0;
    uint32 bag = 0;
    uint32 slot = 0;

    // 98 = no specific bank slot requested.
    uint32 requestedBankSlot = 98;

    if (!args || !*args)
    {
        SendSysMessage(
            "Usage: .guild bank deposit <tab> <bag> <slot> [bank slot]"
        );
        return true;
    }

    int parsed = sscanf(
        args,
        "%u %u %u %u",
        &tabId,
        &bag,
        &slot,
        &requestedBankSlot
    );

    if (parsed < 3)
    {
        SendSysMessage(
            "Usage: .guild bank deposit <tab> <bag> <slot> [bank slot]"
        );
        return true;
    }

    if (bag > 255 || slot > 255)
    {
        SendSysMessage("Invalid bag or slot.");
        return true;
    }

    if (requestedBankSlot > 98)
    {
        SendSysMessage("Invalid guild bank slot.");
        return true;
    }

    CharacterDatabase.PExecute(
        "INSERT IGNORE INTO guild_bank "
        "(guildid, money, purchased_tabs) "
        "VALUES ('%u', '0', '1')",
        guildId
    );

    CharacterDatabase.PExecute(
        "INSERT IGNORE INTO guild_bank_tab "
        "(guildid, tabid, name, icon) "
        "VALUES ('%u', '0', 'Tab 1', '')",
        guildId
    );

    QueryResult* tabResult = CharacterDatabase.PQuery(
        "SELECT purchased_tabs "
        "FROM guild_bank "
        "WHERE guildid='%u'",
        guildId
    );

    if (!tabResult)
    {
        SendSysMessage("Unable to load guild bank.");
        return true;
    }

    uint32 purchasedTabs =
        tabResult->Fetch()[0].GetUInt32();

    delete tabResult;

    if (tabId >= purchasedTabs)
    {
        SendSysMessage(
            "That guild bank tab has not been purchased."
        );
        return true;
    }

    if (!GuildBankHasTabPermission(
        guildId,
        rankId,
        tabId,
        "can_deposit"
    ))
    {
        SendSysMessage(
            "Your guild rank cannot deposit items into that tab."
        );
        return true;
    }

    Item* item =
        player->GetItemByPos(
            uint8(bag),
            uint8(slot)
        );

    if (!item)
    {
        SendSysMessage(
            "No item found in that bag/slot."
        );
        return true;
    }

    if (!item->CanBeTraded())
    {
        SendSysMessage(
            "That item cannot be deposited."
        );
        return true;
    }

    bool usedSlots[98] = { false };

    QueryResult* result =
        CharacterDatabase.PQuery(
            "SELECT slot "
            "FROM guild_bank_item "
            "WHERE guildid='%u' "
            "AND tabid='%u'",
            guildId,
            tabId
        );

    if (result)
    {
        do
        {
            uint32 bankSlot =
                result->Fetch()[0].GetUInt32();

            if (bankSlot < 98)
                usedSlots[bankSlot] = true;

        } while (result->NextRow());

        delete result;
    }

    uint32 freeSlot = 98;

    // Addon supplied a specific destination slot.
    if (requestedBankSlot < 98)
    {
        if (usedSlots[requestedBankSlot])
        {
            SendSysMessage(
                "That guild bank slot is already occupied."
            );
            return true;
        }

        freeSlot = requestedBankSlot;
    }
    else
    {
        // Old command behavior:
        // automatically choose the first free slot.
        for (uint32 i = 0; i < 98; ++i)
        {
            if (!usedSlots[i])
            {
                freeSlot = i;
                break;
            }
        }
    }

    if (freeSlot >= 98)
    {
        SendSysMessage(
            "That guild bank tab is full."
        );
        return true;
    }

    uint32 itemGuid =
        item->GetGUIDLow();

    uint32 itemEntry =
        item->GetEntry();

    uint32 itemCount =
        item->GetCount();

    player->MoveItemFromInventory(
        uint8(bag),
        uint8(slot),
        true
    );

    CharacterDatabase.BeginTransaction(
        player->GetGUIDLow()
    );

    item->DeleteFromInventoryDB();
    item->SaveToDB();

    CharacterDatabase.PExecute(
        "UPDATE item_instance "
        "SET owner_guid='0' "
        "WHERE guid='%u'",
        itemGuid
    );

    CharacterDatabase.PExecute(
        "INSERT INTO guild_bank_item "
        "(guildid, tabid, slot, item_guid) "
        "VALUES ('%u', '%u', '%u', '%u')",
        guildId,
        tabId,
        freeSlot,
        itemGuid
    );

    GuildBankWriteLog(
        guildId,
        player->GetGUIDLow(),
        1,
        tabId,
        freeSlot,
        itemGuid,
        itemEntry,
        itemCount,
        0
    );

    CharacterDatabase.CommitTransaction();

    PSendSysMessage(
        "Deposited entry %u, GUID %u, count %u into tab %u slot %u.",
        itemEntry,
        itemGuid,
        itemCount,
        tabId,
        freeSlot
    );

    delete item;

    return true;
}

bool ChatHandler::HandleGuildBankWithdrawCommand(char* args)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player)
        return false;

    uint32 guildId = player->GetGuildId();
    uint32 rankId = player->GetRank();

    if (!guildId)
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    uint32 tabId = 0;
    uint32 bankSlot = 0;

    if (!args || !*args || sscanf(args, "%u %u", &tabId, &bankSlot) != 2)
    {
        SendSysMessage("Usage: .guild bank withdraw <tab> <slot>");
        return true;
    }

    if (bankSlot >= 98)
    {
        SendSysMessage("Invalid guild bank slot.");
        return true;
    }

    QueryResult* bankResult = CharacterDatabase.PQuery(
        "SELECT purchased_tabs FROM guild_bank WHERE guildid='%u'",
        guildId
    );

    if (!bankResult)
    {
        SendSysMessage("Guild bank does not exist.");
        return true;
    }

    uint32 purchasedTabs = bankResult->Fetch()[0].GetUInt32();
    delete bankResult;

    if (tabId >= purchasedTabs)
    {
        SendSysMessage("That guild bank tab has not been purchased.");
        return true;
    }

    if (!GuildBankHasTabPermission(guildId, rankId, tabId, "can_view") ||
        !GuildBankHasTabPermission(guildId, rankId, tabId, "can_withdraw"))
    {
        SendSysMessage("Your guild rank cannot withdraw items from that tab.");
        return true;
    }

    if (rankId != GR_GUILDMASTER)
    {
        QueryResult* permResult = CharacterDatabase.PQuery(
            "SELECT daily_withdraw_limit FROM guild_bank_right "
            "WHERE guildid='%u' AND rankid='%u' AND tabid='%u'",
            guildId,
            rankId,
            tabId
        );

        if (!permResult)
        {
            SendSysMessage("Your guild rank has no permissions for this tab.");
            return true;
        }

        uint32 dailyLimit = permResult->Fetch()[0].GetUInt32();
        delete permResult;

        if (dailyLimit > 0)
        {
            uint32 today = GetGuildBankDay();
            uint32 used = 0;

            QueryResult* usageResult = CharacterDatabase.PQuery(
                "SELECT items_withdrawn, reset_day "
                "FROM guild_bank_daily_usage "
                "WHERE guildid='%u' AND player_guid='%u' AND tabid='%u'",
                guildId,
                player->GetGUIDLow(),
                tabId
            );

            if (usageResult)
            {
                Field* usageFields = usageResult->Fetch();

                if (usageFields[1].GetUInt32() == today)
                    used = usageFields[0].GetUInt32();

                delete usageResult;
            }

            if (used >= dailyLimit)
            {
                SendSysMessage(
                    "You have reached your daily guild bank withdrawal limit for this tab."
                );
                return true;
            }
        }
    }

    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT "
        "ii.creatorGuid, "
        "ii.giftCreatorGuid, "
        "ii.count, "
        "ii.duration, "
        "ii.charges, "
        "ii.flags, "
        "ii.enchantments, "
        "ii.randomPropertyId, "
        "ii.durability, "
        "ii.text, "
        "gbi.item_guid, "
        "ii.itemEntry, "
        "ii.generated_loot "
        "FROM guild_bank_item gbi "
        "JOIN item_instance ii ON ii.guid = gbi.item_guid "
        "WHERE gbi.guildid='%u' "
        "AND gbi.tabid='%u' "
        "AND gbi.slot='%u'",
        guildId,
        tabId,
        bankSlot
    );

    if (!result)
    {
        SendSysMessage("That guild bank slot is empty.");
        return true;
    }

    Field* fields = result->Fetch();

    uint32 itemGuid = fields[10].GetUInt32();
    uint32 itemEntry = fields[11].GetUInt32();

    ItemPrototype const* proto = ObjectMgr::GetItemPrototype(itemEntry);

    if (!proto)
    {
        delete result;
        SendSysMessage("Guild bank item has an invalid item entry.");
        return true;
    }

    Item* item = NewItemOrBag(proto);
    item->SetGeneratedLoot(fields[12].GetBool());

    if (!item->LoadFromDB(itemGuid, ObjectGuid(), fields, itemEntry))
    {
        delete item;
        delete result;

        SendSysMessage("Failed to load guild bank item.");
        return true;
    }

    delete result;

    ItemPosCountVec dest;

    uint8 msg = player->CanStoreItem(
        NULL_BAG,
        NULL_SLOT,
        dest,
        item,
        false
    );

    if (msg != EQUIP_ERR_OK)
    {
        delete item;

        SendSysMessage(
            "You do not have enough inventory space for that item."
        );
        return true;
    }

    uint32 itemCount = item->GetCount();

    item->SetState(ITEM_UNCHANGED);
    player->MoveItemToInventory(dest, item, true);

    CharacterDatabase.BeginTransaction(player->GetGUIDLow());

    CharacterDatabase.PExecute(
        "DELETE FROM guild_bank_item "
        "WHERE guildid='%u' AND tabid='%u' AND slot='%u'",
        guildId,
        tabId,
        bankSlot
    );

    CharacterDatabase.PExecute(
        "UPDATE item_instance SET owner_guid='%u' WHERE guid='%u'",
        player->GetGUIDLow(),
        itemGuid
    );

    if (rankId != GR_GUILDMASTER)
    {
        uint32 today = GetGuildBankDay();

        CharacterDatabase.PExecute(
            "INSERT INTO guild_bank_daily_usage "
            "(guildid,player_guid,tabid,items_withdrawn,money_withdrawn,reset_day) "
            "VALUES ('%u','%u','%u','1','0','%u') "
            "ON DUPLICATE KEY UPDATE "
            "items_withdrawn = IF(reset_day='%u', items_withdrawn + 1, 1), "
            "money_withdrawn = IF(reset_day='%u', money_withdrawn, 0), "
            "reset_day='%u'",
            guildId,
            player->GetGUIDLow(),
            tabId,
            today,
            today,
            today,
            today
        );
    }

    GuildBankWriteLog(
        guildId,
        player->GetGUIDLow(),
        2,
        tabId,
        bankSlot,
        itemGuid,
        itemEntry,
        itemCount,
        0
    );

    player->SaveInventoryAndGoldToDB();

    CharacterDatabase.CommitTransaction();

    PSendSysMessage(
        "Withdrew entry %u, GUID %u, count %u from tab %u slot %u.",
        itemEntry,
        itemGuid,
        itemCount,
        tabId,
        bankSlot
    );

    return true;
}

bool ChatHandler::HandleGuildBankMoveCommand(char* args)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player)
        return false;

    uint32 guildId = player->GetGuildId();
    uint32 rankId = player->GetRank();

    if (!guildId)
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    uint32 tabId = 0;
    uint32 fromSlot = 0;
    uint32 toSlot = 0;

    if (!args ||
        sscanf(
            args,
            "%u %u %u",
            &tabId,
            &fromSlot,
            &toSlot
        ) != 3)
    {
        SendSysMessage(
            "Usage: .guild bank move <tab> <from slot> <to slot>"
        );

        return true;
    }

    if (fromSlot >= 98 ||
        toSlot >= 98)
    {
        SendSysMessage(
            "Invalid guild bank slot."
        );
        return true;
    }

    if (fromSlot == toSlot)
        return true;

    QueryResult* bankResult =
        CharacterDatabase.PQuery(
            "SELECT purchased_tabs "
            "FROM guild_bank "
            "WHERE guildid='%u'",
            guildId
        );

    if (!bankResult)
    {
        SendSysMessage(
            "Guild bank does not exist."
        );
        return true;
    }

    uint32 purchasedTabs =
        bankResult->Fetch()[0].GetUInt32();

    delete bankResult;

    if (tabId >= purchasedTabs)
    {
        SendSysMessage(
            "That guild bank tab has not been purchased."
        );
        return true;
    }

    if (rankId != GR_GUILDMASTER)
    {
        if (!GuildBankHasTabPermission(
            guildId,
            rankId,
            tabId,
            "can_deposit"
        ) ||
            !GuildBankHasTabPermission(
                guildId,
                rankId,
                tabId,
                "can_withdraw"
            ))
        {
            SendSysMessage(
                "You do not have permission to move items in that guild bank tab."
            );

            return true;
        }
    }

    QueryResult* sourceResult =
        CharacterDatabase.PQuery(
            "SELECT item_guid "
            "FROM guild_bank_item "
            "WHERE guildid='%u' "
            "AND tabid='%u' "
            "AND slot='%u'",
            guildId,
            tabId,
            fromSlot
        );

    if (!sourceResult)
    {
        SendSysMessage(
            "The source guild bank slot is empty."
        );
        return true;
    }

    uint32 sourceGuid =
        sourceResult->Fetch()[0].GetUInt32();

    delete sourceResult;

    QueryResult* targetResult =
        CharacterDatabase.PQuery(
            "SELECT item_guid "
            "FROM guild_bank_item "
            "WHERE guildid='%u' "
            "AND tabid='%u' "
            "AND slot='%u'",
            guildId,
            tabId,
            toSlot
        );

    // Target empty: simple move.
    if (!targetResult)
    {
        CharacterDatabase.PExecute(
            "UPDATE guild_bank_item "
            "SET slot='%u' "
            "WHERE guildid='%u' "
            "AND tabid='%u' "
            "AND slot='%u'",
            toSlot,
            guildId,
            tabId,
            fromSlot
        );

        return true;
    }

    uint32 targetGuid =
        targetResult->Fetch()[0].GetUInt32();

    delete targetResult;

    // Target occupied: swap item GUIDs.
    CharacterDatabase.BeginTransaction(
        player->GetGUIDLow()
    );

    CharacterDatabase.PExecute(
        "UPDATE guild_bank_item "
        "SET item_guid='%u' "
        "WHERE guildid='%u' "
        "AND tabid='%u' "
        "AND slot='%u'",
        targetGuid,
        guildId,
        tabId,
        fromSlot
    );

    CharacterDatabase.PExecute(
        "UPDATE guild_bank_item "
        "SET item_guid='%u' "
        "WHERE guildid='%u' "
        "AND tabid='%u' "
        "AND slot='%u'",
        sourceGuid,
        guildId,
        tabId,
        toSlot
    );

    CharacterDatabase.CommitTransaction();

    return true;
}

bool ChatHandler::HandleGuildBankListCommand(char* args)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player)
        return false;

    uint32 guildId = player->GetGuildId();
    uint32 rankId = player->GetRank();

    if (!guildId)
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    uint32 tabId = 0;

    if (!args || !*args || sscanf(args, "%u", &tabId) != 1)
    {
        SendSysMessage("Usage: .guild bank list <tab>");
        return true;
    }

    QueryResult* bankResult = CharacterDatabase.PQuery(
        "SELECT purchased_tabs FROM guild_bank WHERE guildid='%u'",
        guildId
    );

    if (!bankResult)
    {
        SendSysMessage("Guild bank does not exist.");
        return true;
    }

    uint32 purchasedTabs = bankResult->Fetch()[0].GetUInt32();
    delete bankResult;

    if (tabId >= purchasedTabs)
    {
        SendSysMessage("That guild bank tab has not been purchased.");
        return true;
    }

    if (!GuildBankHasTabPermission(guildId, rankId, tabId, "can_view"))
    {
        SendSysMessage("Your guild rank cannot view that tab.");
        return true;
    }

    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT "
        "gbi.slot, "
        "gbi.item_guid, "
        "ii.itemEntry, "
        "ii.count "
        "FROM guild_bank_item gbi "
        "JOIN item_instance ii ON ii.guid = gbi.item_guid "
        "WHERE gbi.guildid='%u' AND gbi.tabid='%u' "
        "ORDER BY gbi.slot",
        guildId,
        tabId
    );

    if (!result)
    {
        PSendSysMessage("Guild bank tab %u is empty.", tabId);
        return true;
    }

    PSendSysMessage("Guild bank tab %u:", tabId);

    do
    {
        Field* fields = result->Fetch();

        PSendSysMessage(
            "Slot %u: entry %u, GUID %u, count %u",
            fields[0].GetUInt32(),
            fields[2].GetUInt32(),
            fields[1].GetUInt32(),
            fields[3].GetUInt32()
        );
    } while (result->NextRow());

    delete result;

    return true;
}

bool ChatHandler::HandleGuildBankDepositMoneyCommand(char* args)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player)
        return false;

    uint32 guildId = player->GetGuildId();
    uint32 rankId = player->GetRank();

    if (!guildId)
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    unsigned long long amountRaw = 0;

    if (!args || sscanf(args, "%llu", &amountRaw) != 1)
    {
        SendSysMessage("Usage: .guild bank depositmoney <copper>");
        return true;
    }

    if (amountRaw == 0 || amountRaw > 2147483647ULL)
    {
        SendSysMessage("Invalid money amount.");
        return true;
    }

    uint32 amount = uint32(amountRaw);

    if (rankId != GR_GUILDMASTER)
    {
        QueryResult* permissionResult = CharacterDatabase.PQuery(
            "SELECT can_deposit_money "
            "FROM guild_bank_money_right "
            "WHERE guildid='%u' AND rankid='%u'",
            guildId,
            rankId
        );

        if (!permissionResult)
        {
            SendSysMessage(
                "Your guild rank cannot deposit guild bank money."
            );

            return true;
        }

        bool allowed =
            permissionResult->Fetch()[0].GetUInt8() != 0;

        delete permissionResult;

        if (!allowed)
        {
            SendSysMessage(
                "Your guild rank cannot deposit guild bank money."
            );

            return true;
        }
    }

    if (player->GetMoney() < amount)
    {
        SendSysMessage(
            "You do not have that much money."
        );

        return true;
    }

    CharacterDatabase.PExecute(
        "INSERT IGNORE INTO guild_bank "
        "(guildid,money,purchased_tabs) "
        "VALUES ('%u','0','1')",
        guildId
    );

    CharacterDatabase.BeginTransaction(
        player->GetGUIDLow()
    );

    player->ModifyMoney(
        -int32(amount)
    );

    CharacterDatabase.PExecute(
        "UPDATE guild_bank "
        "SET money = money + '%u' "
        "WHERE guildid='%u'",
        amount,
        guildId
    );

    GuildBankWriteLog(
        guildId,
        player->GetGUIDLow(),
        3,
        0,
        0,
        0,
        0,
        0,
        amount
    );

    player->SaveInventoryAndGoldToDB();

    CharacterDatabase.CommitTransaction();

    std::string moneyText =
        FormatGuildBankMoney(amount);

    PSendSysMessage(
        "Deposited %s into the guild bank.",
        moneyText.c_str()
    );

    return true;
}

bool ChatHandler::HandleGuildBankWithdrawMoneyCommand(char* args)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player)
        return false;

    uint32 guildId = player->GetGuildId();
    uint32 rankId = player->GetRank();

    if (!guildId)
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    unsigned long long amountRaw = 0;

    if (!args || sscanf(args, "%llu", &amountRaw) != 1)
    {
        SendSysMessage("Usage: .guild bank withdrawmoney <copper>");
        return true;
    }

    if (amountRaw == 0 || amountRaw > 2147483647ULL)
    {
        SendSysMessage("Invalid money amount.");
        return true;
    }

    uint32 amount = uint32(amountRaw);

    QueryResult* bankResult = CharacterDatabase.PQuery(
        "SELECT money "
        "FROM guild_bank "
        "WHERE guildid='%u'",
        guildId
    );

    if (!bankResult)
    {
        SendSysMessage(
            "Guild bank does not exist."
        );

        return true;
    }

    uint64 bankMoney =
        bankResult->Fetch()[0].GetUInt64();

    delete bankResult;

    if (bankMoney < amount)
    {
        SendSysMessage(
            "The guild bank does not have that much money."
        );

        return true;
    }

    if (rankId != GR_GUILDMASTER)
    {
        QueryResult* permissionResult =
            CharacterDatabase.PQuery(
                "SELECT can_withdraw_money,daily_money_limit "
                "FROM guild_bank_money_right "
                "WHERE guildid='%u' AND rankid='%u'",
                guildId,
                rankId
            );

        if (!permissionResult)
        {
            SendSysMessage(
                "Your guild rank cannot withdraw guild bank money."
            );

            return true;
        }

        Field* permissionFields =
            permissionResult->Fetch();

        bool allowed =
            permissionFields[0].GetUInt8() != 0;

        uint64 dailyLimit =
            permissionFields[1].GetUInt64();

        delete permissionResult;

        if (!allowed)
        {
            SendSysMessage(
                "Your guild rank cannot withdraw guild bank money."
            );

            return true;
        }

        if (dailyLimit > 0)
        {
            uint32 dayStart =
                GetGuildBankDay() * DAY;

            QueryResult* usageResult =
                CharacterDatabase.PQuery(
                    "SELECT COALESCE(SUM(money),0) "
                    "FROM guild_bank_log "
                    "WHERE guildid='%u' "
                    "AND player_guid='%u' "
                    "AND action='4' "
                    "AND timestamp >= '%u'",
                    guildId,
                    player->GetGUIDLow(),
                    dayStart
                );

            uint64 alreadyWithdrawn = 0;

            if (usageResult)
            {
                alreadyWithdrawn =
                    usageResult->Fetch()[0].GetUInt64();

                delete usageResult;
            }

            if (alreadyWithdrawn + amount >
                dailyLimit)
            {
                SendSysMessage(
                    "That would exceed your daily guild bank money withdrawal limit."
                );

                return true;
            }
        }
    }

    if (uint64(player->GetMoney()) + amount >
        MAX_MONEY_AMOUNT)
    {
        SendSysMessage(
            "You cannot carry that much money."
        );

        return true;
    }

    CharacterDatabase.BeginTransaction(
        player->GetGUIDLow()
    );

    CharacterDatabase.PExecute(
        "UPDATE guild_bank "
        "SET money = money - '%u' "
        "WHERE guildid='%u'",
        amount,
        guildId
    );

    player->ModifyMoney(
        int32(amount)
    );

    GuildBankWriteLog(
        guildId,
        player->GetGUIDLow(),
        4,
        0,
        0,
        0,
        0,
        0,
        amount
    );

    player->SaveInventoryAndGoldToDB();

    CharacterDatabase.CommitTransaction();

    std::string moneyText =
        FormatGuildBankMoney(amount);

    PSendSysMessage(
        "Withdrew %s from the guild bank.",
        moneyText.c_str()
    );

    return true;
}

bool ChatHandler::HandleGuildBankTabsCommand(char* /*args*/)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player)
        return false;

    uint32 guildId = player->GetGuildId();

    if (!guildId)
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    CharacterDatabase.PExecute(
        "INSERT IGNORE INTO guild_bank "
        "(guildid, money, purchased_tabs) "
        "VALUES ('%u', '0', '1')",
        guildId
    );

    CharacterDatabase.PExecute(
        "INSERT IGNORE INTO guild_bank_tab "
        "(guildid, tabid, name, icon) "
        "VALUES ('%u', '0', 'Tab 1', '')",
        guildId
    );

    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT gb.purchased_tabs, gbt.tabid, gbt.name "
        "FROM guild_bank gb "
        "LEFT JOIN guild_bank_tab gbt ON gbt.guildid = gb.guildid "
        "WHERE gb.guildid='%u' "
        "ORDER BY gbt.tabid",
        guildId
    );

    if (!result)
    {
        SendSysMessage("Unable to load guild bank tabs.");
        return true;
    }

    uint32 purchasedTabs = result->Fetch()[0].GetUInt32();

    PSendSysMessage(
        "Purchased guild bank tabs: %u/6",
        purchasedTabs
    );

    do
    {
        Field* fields = result->Fetch();

        if (!fields[1].IsNULL())
        {
            PSendSysMessage(
                "Tab %u: %s",
                fields[1].GetUInt32(),
                fields[2].GetString()
            );
        }
    } while (result->NextRow());

    delete result;

    return true;
}

bool ChatHandler::HandleGuildBankAddTabCommand(char* /*args*/)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player)
        return false;

    uint32 guildId = player->GetGuildId();

    if (!guildId)
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    if (player->GetRank() != 0)
    {
        SendSysMessage(
            "Only the Guild Master can purchase guild bank tabs."
        );

        return true;
    }

    CharacterDatabase.PExecute(
        "INSERT IGNORE INTO guild_bank "
        "(guildid,money,purchased_tabs) "
        "VALUES ('%u','0','1')",
        guildId
    );

    CharacterDatabase.PExecute(
        "INSERT IGNORE INTO guild_bank_tab "
        "(guildid,tabid,name,icon) "
        "VALUES ('%u','0','Tab 1','')",
        guildId
    );

    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT purchased_tabs,money "
        "FROM guild_bank "
        "WHERE guildid='%u'",
        guildId
    );

    if (!result)
    {
        SendSysMessage(
            "Unable to load guild bank."
        );

        return true;
    }

    Field* fields = result->Fetch();

    uint32 purchasedTabs =
        fields[0].GetUInt32();

    uint64 bankMoney =
        fields[1].GetUInt64();

    delete result;

    if (purchasedTabs >= 6)
    {
        SendSysMessage(
            "Guild bank already has the maximum number of tabs."
        );

        return true;
    }

    // Tab 2 = 50g
    // Tab 3 = 100g
    // Tab 4 = 200g
    // Tab 5 = 400g
    // Tab 6 = 800g

    uint64 tabPrice = 0;

    switch (purchasedTabs)
    {
    case 1:
        tabPrice = 50ULL * 10000ULL;
        break;

    case 2:
        tabPrice = 100ULL * 10000ULL;
        break;

    case 3:
        tabPrice = 200ULL * 10000ULL;
        break;

    case 4:
        tabPrice = 400ULL * 10000ULL;
        break;

    case 5:
        tabPrice = 800ULL * 10000ULL;
        break;

    default:
        SendSysMessage(
            "Unable to determine guild bank tab price."
        );

        return true;
    }

    if (bankMoney < tabPrice)
    {
        std::string priceText =
            FormatGuildBankMoney(tabPrice);

        PSendSysMessage(
            "The guild bank needs %s to purchase the next tab.",
            priceText.c_str()
        );

        return true;
    }

    uint32 newTabId =
        purchasedTabs;

    CharacterDatabase.BeginTransaction(
        player->GetGUIDLow()
    );

    CharacterDatabase.PExecute(
        "UPDATE guild_bank "
        "SET purchased_tabs='%u', "
        "money = money - '" UI64FMTD "' "
        "WHERE guildid='%u'",
        purchasedTabs + 1,
        tabPrice,
        guildId
    );

    CharacterDatabase.PExecute(
        "INSERT INTO guild_bank_tab "
        "(guildid,tabid,name,icon) "
        "VALUES ('%u','%u','Tab %u','')",
        guildId,
        newTabId,
        newTabId + 1
    );

    GuildBankWriteLog(
        guildId,
        player->GetGUIDLow(),
        5,
        newTabId,
        0,
        0,
        0,
        0,
        tabPrice
    );

    CharacterDatabase.CommitTransaction();

    std::string priceText =
        FormatGuildBankMoney(tabPrice);

    PSendSysMessage(
        "Purchased Guild Bank Tab %u for %s.",
        newTabId + 1,
        priceText.c_str()
    );

    return true;
}


bool ChatHandler::HandleGuildBankMoneyCommand(char* /*args*/)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player || !player->GetGuildId())
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT money FROM guild_bank WHERE guildid='%u'",
        player->GetGuildId()
    );

    if (!result)
    {
        SendSysMessage("Guild bank does not exist.");
        return true;
    }

    uint64 money = result->Fetch()[0].GetUInt64();
    delete result;

    PSendSysMessage(
        "Guild bank funds: " UI64FMTD " copper.",
        money
    );

    return true;
}

bool ChatHandler::HandleGuildBankLogCommand(char* /*args*/)
{
    if (!m_session)
        return false;

    Player* player = m_session->GetPlayer();

    if (!player || !player->GetGuildId())
    {
        SendSysMessage("You are not in a guild.");
        return true;
    }

    uint32 guildId = player->GetGuildId();

    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT "
        "gbl.action, "
        "gbl.player_guid, "
        "c.name, "
        "gbl.tabid, "
        "gbl.slot, "
        "gbl.item_entry, "
        "gbl.item_count, "
        "gbl.money, "
        "gbl.timestamp "
        "FROM guild_bank_log gbl "
        "LEFT JOIN characters c ON c.guid = gbl.player_guid "
        "WHERE gbl.guildid='%u' "
        "ORDER BY gbl.id DESC "
        "LIMIT 30",
        guildId
    );

    if (!result)
    {
        SendSysMessage("Guild bank log is empty.");
        return true;
    }

    SendSysMessage("Recent guild bank activity:");

    do
    {
        Field* fields = result->Fetch();

        uint32 action = fields[0].GetUInt32();
        uint32 playerGuid = fields[1].GetUInt32();

        std::string playerName;

        if (!fields[2].IsNULL())
            playerName = fields[2].GetString();
        else
            playerName = "Unknown";

        switch (action)
        {
        case 1:
            PSendSysMessage(
                "DEPOSIT: %s deposited entry %u x%u into tab %u slot %u.",
                playerName.c_str(),
                fields[5].GetUInt32(),
                fields[6].GetUInt32(),
                fields[3].GetUInt32(),
                fields[4].GetUInt32()
            );
            break;

        case 2:
            PSendSysMessage(
                "WITHDRAW: %s withdrew entry %u x%u from tab %u slot %u.",
                playerName.c_str(),
                fields[5].GetUInt32(),
                fields[6].GetUInt32(),
                fields[3].GetUInt32(),
                fields[4].GetUInt32()
            );
            break;

        case 3:
            PSendSysMessage(
                "MONEY DEPOSIT: %s deposited " UI64FMTD " copper.",
                playerName.c_str(),
                fields[7].GetUInt64()
            );
            break;

        case 4:
            PSendSysMessage(
                "MONEY WITHDRAW: %s withdrew " UI64FMTD " copper.",
                playerName.c_str(),
                fields[7].GetUInt64()
            );
            break;

        default:
            PSendSysMessage(
                "UNKNOWN BANK ACTION %u by %s (GUID %u).",
                action,
                playerName.c_str(),
                playerGuid
            );
            break;
        }
    } while (result->NextRow());

    delete result;

    return true;
}

bool ChatHandler::HandleHelpCommand(char* args)
{
    if (!*args)
    {
        ShowHelpForCommand(getCommandTable(), "help");
        ShowHelpForCommand(getCommandTable(), "");
    }
    else
    {
        if (!ShowHelpForCommand(getCommandTable(), args))
            SendSysMessage(LANG_NO_CMD);
    }

    return true;
}

bool ChatHandler::HandleCommandsCommand(char* /*args*/)
{
    ShowHelpForCommand(getCommandTable(), "");
    return true;
}

bool ChatHandler::HandleAuctionAllianceCommand(char* /*args*/)
{
    m_session->GetPlayer()->SetAuctionAccessMode(m_session->GetPlayer()->GetTeam() != ALLIANCE ? -1 : 0);
    m_session->SendAuctionHello(m_session->GetPlayer());
    return true;
}

bool ChatHandler::HandleAuctionHordeCommand(char* /*args*/)
{
    m_session->GetPlayer()->SetAuctionAccessMode(m_session->GetPlayer()->GetTeam() != HORDE ? -1 : 0);
    m_session->SendAuctionHello(m_session->GetPlayer());
    return true;
}

bool ChatHandler::HandleAuctionGoblinCommand(char* /*args*/)
{
    m_session->GetPlayer()->SetAuctionAccessMode(1);
    m_session->SendAuctionHello(m_session->GetPlayer());
    return true;
}

bool ChatHandler::HandleAuctionCommand(char* /*args*/)
{
    m_session->GetPlayer()->SetAuctionAccessMode(0);
    m_session->SendAuctionHello(m_session->GetPlayer());

    return true;
}

bool ChatHandler::HandleBankCommand(char* /*args*/)
{
    m_session->SendShowBank(m_session->GetPlayer()->GetObjectGuid());

    return true;
}

bool ChatHandler::HandleStableCommand(char* /*args*/)
{
    m_session->SendStablePet(m_session->GetPlayer()->GetObjectGuid());

    return true;
}

//Enable\Dissable GM Mode
bool ChatHandler::HandleGMCommand(char* args)
{
    if (!*args)
    {
        if (m_session->GetPlayer()->IsGameMaster())
            m_session->SendNotification(LANG_GM_ON);
        else
            m_session->SendNotification(LANG_GM_OFF);
        return true;
    }

    bool value;
    if (!ExtractOnOff(&args, value))
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    m_session->GetPlayer()->SetGameMaster(value, true);

    return true;
}

// Enables or disables hiding of the staff badge
bool ChatHandler::HandleGMChatCommand(char* args)
{
    if (!*args)
    {
        if (m_session->GetPlayer()->IsGMChat())
            m_session->SendNotification(LANG_GM_CHAT_ON);
        else
            m_session->SendNotification(LANG_GM_CHAT_OFF);
        return true;
    }

    bool value;
    if (!ExtractOnOff(&args, value))
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    m_session->GetPlayer()->SetGMChat(value, true);

    return true;
}

//Enable\Disable Invisible mode
bool ChatHandler::HandleGMVisibleCommand(char* args)
{
    if (!*args)
    {
        bool visible = GetSession()->GetPlayer()->IsGMVisible();
        uint32 visibilityLevel = visible ? 0 : GetSession()->GetPlayer()->GetGMInvisibilityLevel();
        PSendSysMessage(LANG_YOU_ARE, visible ? GetMangosString(LANG_VISIBLE) : GetMangosString(LANG_INVISIBLE), visibilityLevel);
        return true;
    }

    bool value;
    uint8 accessLevel = GetAccessLevel();
    uint32 visibilityLevel = accessLevel + 1;

    if (ExtractUInt32(&args, visibilityLevel))
        value = (visibilityLevel == 0); // Make visible if level = 0 only
    else if (ExtractOnOff(&args, value))
        visibilityLevel = accessLevel;

    if (visibilityLevel > accessLevel)
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    if (!value)
        m_session->GetPlayer()->SetGMInvisibilityLevel(visibilityLevel);

    m_session->GetPlayer()->SetGMVisible(value, true);

    return true;
}

bool ChatHandler::HandleSetViewCommand(char* /*args*/)
{
    if (Unit* unit = GetSelectedUnit())
        m_session->GetPlayer()->GetCamera().SetView(unit);
    else
    {
        PSendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

bool ChatHandler::HandleGMOptionsCommand(char* args)
{
    if (!args)
        return false;

    std::string sArgs(args);
    bool enable = true;
    uint32 flags = 0;
    if (sArgs.find("off") != std::string::npos)
        enable = false;
    if (sArgs.find("OFF") != std::string::npos)
        enable = false;
    if (sArgs.find("GOD") != std::string::npos || sArgs.find("god") != std::string::npos)
        flags |= PLAYER_CHEAT_GOD;
    if (sArgs.find("CD") != std::string::npos || sArgs.find("cd") != std::string::npos || sArgs.find("cooldown") != std::string::npos || sArgs.find("COOLDOWN") != std::string::npos)
        flags |= PLAYER_CHEAT_NO_COOLDOWN;
    if (sArgs.find("CAST") != std::string::npos || sArgs.find("cast") != std::string::npos)
        flags |= PLAYER_CHEAT_NO_CAST_TIME;
    if (sArgs.find("SPEED") != std::string::npos || sArgs.find("speed") != std::string::npos)
        flags |= PLAYER_CHEAT_NO_MOD_SPEED;
    if (sArgs.find("power") != std::string::npos || sArgs.find("POWER") != std::string::npos || sArgs.find("mana") != std::string::npos || sArgs.find("MANA") != std::string::npos)
        flags |= PLAYER_CHEAT_NO_POWER;
    if (sArgs.find("crit") != std::string::npos || sArgs.find("CRIT") != std::string::npos)
        flags |= PLAYER_CHEAT_ALWAYS_CRIT;
    if (sArgs.find("checkcast") != std::string::npos || sArgs.find("CHECKCAST") != std::string::npos || sArgs.find("check") != std::string::npos || sArgs.find("CHECK") != std::string::npos)
        flags |= PLAYER_CHEAT_NO_CHECK_CAST;
    if (sArgs.find("proc") != std::string::npos || sArgs.find("PROC") != std::string::npos)
        flags |= PLAYER_CHEAT_ALWAYS_PROC;
    if (sArgs.find("video") != std::string::npos || sArgs.find("VIDEO") != std::string::npos)
        flags |= PLAYER_VIDEO_MODE;

    Player* pTarget = GetSelectedPlayer();
    if (!pTarget)
        pTarget = m_session->GetPlayer();
    PSendSysMessage("%s flags 0x%x for `%s`.", enable ? "Adding" : "Removing", flags, pTarget->GetName());
    if (enable)
        pTarget->EnableOption(flags);
    else
        pTarget->RemoveOption(flags);

    if (flags & PLAYER_VIDEO_MODE)
    {
        UpdateMask m;
        m.SetCount(UNIT_END);
        m.SetBit(UNIT_FIELD_FLAGS);
        pTarget->RefreshBitsForVisibleUnits(&m, TYPEMASK_UNIT);
        m.SetCount(PLAYER_END);
        m.SetBit(UNIT_FIELD_FLAGS);
        pTarget->RefreshBitsForVisibleUnits(&m, TYPEMASK_PLAYER);
    }
    return true;
}

/// Display the list of GMs
bool ChatHandler::HandleGMListFullCommand(char* /*args*/)
{
    ///- Get the accounts with GM Level >0
    QueryResult *result = LoginDatabase.PQuery("SELECT username, account_access.gmlevel FROM account, account_access "
        "WHERE account_access.id = account.id AND account_access.gmlevel > 0 AND RealmID=%u", realmID);
    if (result)
    {
        SendSysMessage(LANG_GMLIST);
        SendSysMessage("========================");
        SendSysMessage(LANG_GMLIST_HEADER);
        SendSysMessage("========================");

        ///- Circle through them. Display username and GM level
        do
        {
            Field *fields = result->Fetch();
            PSendSysMessage("|%15s|%6s|", fields[0].GetString(), fields[1].GetString());
        } while (result->NextRow());

        PSendSysMessage("========================");
        delete result;
    }
    else
        PSendSysMessage(LANG_GMLIST_EMPTY);
    return true;
}

bool ChatHandler::HandleGMListIngameCommand(char* /*args*/)
{
    std::list< std::pair<std::string, bool> > names;

    {
        HashMapHolder<Player>::ReadGuard g(HashMapHolder<Player>::GetLock());
        HashMapHolder<Player>::MapType &m = sObjectAccessor.GetPlayers();
        for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            AccountTypes itr_sec = itr->second->GetSession()->GetSecurity();
            if ((itr->second->IsGameMaster() || (itr_sec > SEC_PLAYER && itr_sec <= (AccountTypes)sWorld.getConfig(CONFIG_UINT32_GM_LEVEL_IN_GM_LIST))) &&
                (!m_session || itr->second->IsVisibleGloballyFor(m_session->GetPlayer())))
                names.push_back(std::make_pair<std::string, bool>(GetNameLink(itr->second), itr->second->IsAcceptWhispers()));
        }
    }

    if (!names.empty())
    {
        SendSysMessage(LANG_GMS_ON_SRV);

        char const* accepts = GetMangosString(LANG_GM_ACCEPTS_WHISPER);
        char const* not_accept = GetMangosString(LANG_GM_NO_WHISPER);
        for (std::list<std::pair< std::string, bool> >::const_iterator iter = names.begin(); iter != names.end(); ++iter)
            PSendSysMessage("%s - %s", iter->first.c_str(), iter->second ? accepts : not_accept);
    }
    else
        SendSysMessage(LANG_GMS_NOT_LOGGED);

    return true;
}

bool ChatHandler::HandleGMFlyCommand(char* args)
{
    bool value;
    if (!ExtractOnOff(&args, value))
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    Player *target = GetSelectedPlayer();
    if (!target)
        target = m_session->GetPlayer();

    target->SetFly(value);

    if (value)
        SendSysMessage("WARNING: Do not jump or flying mode will be removed.");

    if (m_session->IsReplaying())
    {
        MovementInfo movementInfo = m_session->GetPlayer()->m_movementInfo;
        movementInfo.UpdateTime(WorldTimer::getMSTime());
        WorldPacket data(MSG_MOVE_HEARTBEAT, 31);
        data << m_session->GetRecorderGuid().WriteAsPacked();
        data << movementInfo;
        m_session->SendPacket(&data);
    }

    PSendSysMessage(LANG_COMMAND_FLYMODE_STATUS, GetNameLink(target).c_str(), args);
    return true;
}

bool RegisterPlayerToBG(WorldSession * sess, BattleGroundTypeId bgid)
{
    Player* pPlayer = sess->GetPlayer();
    if (!pPlayer->GetBGAccessByLevel(bgid))
        return false;
    pPlayer->SetBattleGroundEntryPoint(pPlayer->GetMapId(), pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ(), pPlayer->GetOrientation());
    sess->SendBattlegGroundList(pPlayer->GetObjectGuid(), bgid);
    return true;
}

bool ChatHandler::HandleGoWarsongCommand(char * args)
{
    return RegisterPlayerToBG(m_session, BattleGroundTypeId(BATTLEGROUND_WS));
}
bool ChatHandler::HandleGoArathiCommand(char *args)
{
    return RegisterPlayerToBG(m_session, BattleGroundTypeId(BATTLEGROUND_AB));
}
bool ChatHandler::HandleGoAlteracCommand(char *args)
{
    return RegisterPlayerToBG(m_session, BattleGroundTypeId(BATTLEGROUND_AV));
}

/** \brief GM command level 3 - Create a guild.
 *
 * This command allows a GM (level 3) to create a guild.
 *
 * The "args" parameter contains the name of the guild leader
 * and then the name of the guild.
 *
 */
bool ChatHandler::HandleGuildCreateCommand(char* args)
{
    // guildmaster name optional
    char* guildMasterStr = ExtractOptNotLastArg(&args);

    Player* target;
    if (!ExtractPlayerTarget(&guildMasterStr, &target))
        return false;

    char* guildStr = ExtractQuotedArg(&args);
    if (!guildStr)
        return false;

    std::string guildname = guildStr;

    if (target->GetGuildId())
    {
        SendSysMessage(LANG_PLAYER_IN_GUILD);
        return true;
    }

    Guild* guild = new Guild;
    if (!guild->Create(target, guildname))
    {
        delete guild;
        SendSysMessage(LANG_GUILD_NOT_CREATED);
        SetSentErrorMessage(true);
        return false;
    }

    sGuildMgr.AddGuild(guild);
    return true;
}

bool ChatHandler::HandleGuildInviteCommand(char *args)
{
    // player name optional
    char* nameStr = ExtractOptNotLastArg(&args);

    // if not guild name only (in "") then player name
    ObjectGuid target_guid;
    if (!ExtractPlayerTarget(&nameStr, nullptr, &target_guid))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    char* guildStr = ExtractQuotedArg(&args);
    if (!guildStr)
        return false;

    std::string glName = guildStr;
    Guild* targetGuild = sGuildMgr.GetGuildByName(glName);
    if (!targetGuild)
    {
        SendSysMessage(LANG_GUILD_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    // player's guild membership checked in AddMember before add
    auto status = targetGuild->AddMember(target_guid, targetGuild->GetLowestRank());

    if (status != GuildAddStatus::OK)
    {
        std::string error;

        switch (status) // bad
        {
            case GuildAddStatus::ALREADY_IN_GUILD:
                error = "Player is already in a guild.";
                break;
            case GuildAddStatus::GUILD_FULL:
                error = "The target guild is full.";
                break;
            case GuildAddStatus::PLAYER_DATA_ERROR:
                error = "Player data appears to be corrupt - tell an administrator.";
                break;
            case GuildAddStatus::UNKNOWN_PLAYER:
                error = "Unable to find target player.";
                break;
            default:
                error = "Unhandled guild invite error.";
        }

        SendSysMessage(error.c_str());
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage("Player added to %s.", glName.c_str());
    return true;
}

bool ChatHandler::HandleGuildUninviteCommand(char *args)
{
    Player* target;
    ObjectGuid target_guid;
    if (!ExtractPlayerTarget(&args, &target, &target_guid))
        return false;

    uint32 glId = target ? target->GetGuildId() : Player::GetGuildIdFromDB(target_guid);

    if (!glId)
        return false;

    Guild* targetGuild = sGuildMgr.GetGuildById(glId);
    if (!targetGuild)
    {
        SendSysMessage(LANG_GUILD_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    if (targetGuild->DelMember(target_guid))
    {
        targetGuild->Disband();
        delete targetGuild;
    }

    return true;
}

bool ChatHandler::HandleGuildRankCommand(char *args)
{
    char* nameStr = ExtractOptNotLastArg(&args);

    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&nameStr, &target, &target_guid, &target_name))
        return false;

    uint32 glId = target ? target->GetGuildId() : Player::GetGuildIdFromDB(target_guid);

    if (!glId)
    {
        SendSysMessage(LANG_GUILD_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    Guild* targetGuild = sGuildMgr.GetGuildById(glId);
    if (!targetGuild)
        return false;

    uint32 newrank;
    if (!ExtractUInt32(&args, newrank))
        return false;

    if (newrank > targetGuild->GetLowestRank())
        return false;

    MemberSlot* slot = targetGuild->GetMemberSlot(target_guid);
    if (!slot)
        return false;

    slot->ChangeRank(newrank);
    return true;
}

bool ChatHandler::HandleGuildDeleteCommand(char* args)
{
    if (!*args)
        return false;

    char* guildStr = ExtractQuotedArg(&args);
    if (!guildStr)
        return false;

    std::string gld = guildStr;

    Guild* targetGuild = sGuildMgr.GetGuildByName(gld);

    if (!targetGuild)
    {
        SendSysMessage(LANG_GUILD_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    targetGuild->Disband();
    delete targetGuild;

    return true;
}

/**
 * Renames a guild if a guild could be found with the specified name.
 * Usage: .guild rename "name" "new name"
 * It is not possible to rename a guild to a name that is already in use.
 */
bool ChatHandler::HandleGuildRenameCommand(char* args)
{
    if (!args || !*args)
        return false;

    char* currentName = ExtractQuotedArg(&args);
    if (!currentName)
        return false;

    std::string current(currentName);

    char* newName = ExtractQuotedArg(&args);
    if (!newName)
        return false;

    std::string newn(newName);

    Guild* target = sGuildMgr.GetGuildByName(currentName);
    if (!target)
    {
        SendSysMessage(LANG_GUILD_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    if (Guild* existing = sGuildMgr.GetGuildByName(newn))
    {
        PSendSysMessage("A guild with the name '%s' already exists", newName);
        SetSentErrorMessage(true);
        return false;
    }

    target->Rename(newn);
    PSendSysMessage("Guild '%s' successfully renamed to '%s'. Players must relog to see the changes", currentName, newName);
    return true;
}

bool ChatHandler::HandleInstanceBindingMode(char* args)
{
    Player* player = GetSession()->GetPlayer();

    if (strcmp(args, "on") == 0)
    {
        player->SetSmartInstanceBindingMode(true);
        PSendSysMessage("Smart rebinding has been enabled.");
        return true;
    }
    else if (strcmp(args, "off") == 0)
    {
        player->SetSmartInstanceBindingMode(false);
        PSendSysMessage("Smart rebinding has been disabled.");
        return true;
    }

    PSendSysMessage("[Error] Invalid arguments - only 'on' or 'off' are accepted");
    return false;
}

bool ChatHandler::HandleInstanceSwitchCommand(char* args)
{
    uint32 newInstanceId = 0;
    Player* target = GetSelectedPlayer();
    if (!target)
        return false;
    if (!ExtractUInt32(&args, newInstanceId))
        return false;
    uint32 oldInstance = target->GetInstanceId();
    PSendSysMessage("Instance switch from #%u to %u: %s", oldInstance, newInstanceId, target->SwitchInstance(newInstanceId) ? "OK" : "FAILED");
    target->SetAutoInstanceSwitch(false);
    return true;
}

bool ChatHandler::HandleInstanceContinentsCommand(char*)
{
    if (Player* target = GetSelectedPlayer())
        PSendSysMessage("Target: %s, map %u instance %u", target->GetName(), target->GetMapId(), target->GetInstanceId());

    for (int mapId = 0; mapId < 2; ++mapId)
    {
        PSendSysMessage("MAP %u", mapId);
        for (int i = 0; i < 20; ++i)
            if (Map* m = sMapMgr.FindMap(mapId, i))
            {
                Map::PlayerList const& players = m->GetPlayers();
                if (players.begin() == players.end())
                    continue;
                Map::PlayerList::const_iterator it = players.begin();
                int count = 0;
                while (it != players.end())
                {
                    ++count;
                    ++it;
                }
                PSendSysMessage("[Instance%2u] %u players, dist visible:%.1f activate:%.1f", i, count, m->GetVisibilityDistance(), m->GetGridActivationDistance());
            }
    }
    return true;
}

bool ChatHandler::HandleInstancePerfInfosCommand(char* args)
{
    Player* player = GetSession()->GetPlayer();
    Map* map = player->FindMap();
    if (!map)
        return false;
    map->PrintInfos(*this);
    uint32 playersInClient = 0, gobjsInClient = 0, unitsInClient = 0, corpsesInClient = 0;
    for (ObjectGuidSet::const_iterator it = player->m_visibleGUIDs.begin(); it != player->m_visibleGUIDs.end(); ++it)
    {
        switch (it->GetHigh())
        {
            case HIGHGUID_PLAYER: ++playersInClient; break;
            case HIGHGUID_GAMEOBJECT: ++gobjsInClient; break;
            case HIGHGUID_UNIT: ++unitsInClient; break;
            case HIGHGUID_CORPSE: ++corpsesInClient; break;
        }
    }
    PSendSysMessage("Units in client: %u pl, %u gobj, %u crea, %u corpses", playersInClient, gobjsInClient, unitsInClient, corpsesInClient);
    return true;
}

bool ChatHandler::HandleInstanceListBindsCommand(char* /*args*/)
{
    Player* player = GetSelectedPlayer();
    if (!player) player = m_session->GetPlayer();
    uint32 counter = 0;

    Player::BoundInstancesMap &binds = player->GetBoundInstances();
    for (Player::BoundInstancesMap::const_iterator itr = binds.begin(); itr != binds.end(); ++itr)
    {
        DungeonPersistentState *state = itr->second.state;
        std::string timeleft = "";
        if (!itr->second.perm)
            timeleft = secsToTimeString(state->GetResetTime() - time(nullptr), true);
        else
            timeleft = secsToTimeString(sMapPersistentStateMgr.GetScheduler().GetResetTimeFor(itr->first) - time(nullptr));

        if (const MapEntry* entry = sMapStorage.LookupEntry<MapEntry>(itr->first))
        {
            PSendSysMessage("map: %d (%s) inst: %d perm: %s canReset: %s TTR: %s",
                            itr->first, entry->name, state->GetInstanceId(), itr->second.perm ? "yes" : "no",
                            state->CanReset() ? "yes" : "no", timeleft.c_str());
        }
        else
            PSendSysMessage("bound for a nonexistent map %u", itr->first);
        counter++;
    }

    PSendSysMessage("player binds: %d", counter);
    counter = 0;

    if (Group *group = player->GetGroup())
    {
        Group::BoundInstancesMap &binds = group->GetBoundInstances();
        for (Group::BoundInstancesMap::const_iterator itr = binds.begin(); itr != binds.end(); ++itr)
        {
            DungeonPersistentState *state = itr->second.state;
            std::string timeleft = "";
            if (!itr->second.perm)
                timeleft = secsToTimeString(state->GetResetTime() - time(nullptr), true);
            else
                timeleft = secsToTimeString(sMapPersistentStateMgr.GetScheduler().GetResetTimeFor(itr->first) - time(nullptr));

            if (const MapEntry* entry = sMapStorage.LookupEntry<MapEntry>(itr->first))
            {
                PSendSysMessage("map: %d (%s) inst: %d perm: %s canReset: %s TTR: %s",
                                itr->first, entry->name, state->GetInstanceId(), itr->second.perm ? "yes" : "no",
                                state->CanReset() ? "yes" : "no", timeleft.c_str());
            }
            else
                PSendSysMessage("bound for a nonexistent map %u", itr->first);
            counter++;
        }
    }
    PSendSysMessage("group binds: %d", counter);

    return true;
}

bool ChatHandler::HandleInstanceUnbindCommand(char* args)
{
    if (!*args)
        return false;

    Player* player = GetSelectedPlayer();
    if (!player || GetAccessLevel() < SEC_BASIC_ADMIN)
        player = m_session->GetPlayer();
    uint32 counter = 0;
    uint32 mapid = 0;
    bool got_map = false;

    if (strncmp(args, "all", strlen(args)) != 0)
    {
        if (!isNumeric(args[0]))
            return false;

        got_map = true;
        mapid = atoi(args);
    }

    Player::BoundInstancesMap &binds = player->GetBoundInstances();
    for (Player::BoundInstancesMap::iterator itr = binds.begin(); itr != binds.end();)
    {
        if (got_map && mapid != itr->first)
        {
            ++itr;
            continue;
        }
        if (itr->first != player->GetMapId())
        {
            DungeonPersistentState *save = itr->second.state;
            std::string timeleft = secsToTimeString(save->GetResetTime() - time(nullptr), true);

            if (const MapEntry* entry = sMapStorage.LookupEntry<MapEntry>(itr->first))
            {
                PSendSysMessage("unbinding map: %d (%s) inst: %d perm: %s canReset: %s TTR: %s",
                                itr->first, entry->name, save->GetInstanceId(), itr->second.perm ? "yes" : "no",
                                save->CanReset() ? "yes" : "no", timeleft.c_str());
            }
            else
                PSendSysMessage("bound for a nonexistent map %u", itr->first);
            player->UnbindInstance(itr);
            counter++;
        }
        else
            ++itr;
    }
    PSendSysMessage("instances unbound: %d", counter);

    return true;
}

bool ChatHandler::HandleInstanceStatsCommand(char* /*args*/)
{
    PSendSysMessage("instances loaded: %d", sMapMgr.GetNumInstances());
    PSendSysMessage("players in instances: %d", sMapMgr.GetNumPlayersInInstances());

    uint32 numSaves, numBoundPlayers, numBoundGroups;
    sMapPersistentStateMgr.GetStatistics(numSaves, numBoundPlayers, numBoundGroups);
    PSendSysMessage("instance saves: %d", numSaves);
    PSendSysMessage("players bound: %d", numBoundPlayers);
    PSendSysMessage("groups bound: %d", numBoundGroups);
    return true;
}

bool ChatHandler::HandleInstanceSaveDataCommand(char* /*args*/)
{
    Player* pl = m_session->GetPlayer();

    Map* map = pl->GetMap();

    InstanceData* iData = map->GetInstanceData();
    if (!iData)
    {
        PSendSysMessage("Map has no instance data.");
        SetSentErrorMessage(true);
        return false;
    }

    iData->SaveToDB();
    return true;
}

bool ChatHandler::HandleSendMailHelper(MailDraft& draft, char* args)
{
    // format: "subject text" "mail text"
    char* msgSubject = ExtractQuotedArg(&args);
    if (!msgSubject)
        return false;

    char* msgText = ExtractQuotedArg(&args);
    if (!msgText)
        return false;

    // msgSubject, msgText isn't NUL after prev. check
    draft.SetSubjectAndBody(msgSubject, msgText);

    return true;
}

bool ChatHandler::HandleSendMassMailCommand(char* args)
{
    // format: raceMask "subject text" "mail text"
    uint32 raceMask = 0;
    char const* name = nullptr;

    if (!ExtractRaceMask(&args, raceMask, &name))
        return false;

    // need dynamic object because it trasfered to mass mailer
    MailDraft* draft = new MailDraft;

    // fill mail
    if (!HandleSendMailHelper(*draft, args))
    {
        delete draft;
        return false;
    }

    // from console show nonexistent sender
    MailSender sender(MAIL_NORMAL, m_session ? m_session->GetPlayer()->GetObjectGuid().GetCounter() : 0, MAIL_STATIONERY_GM);

    sMassMailMgr.AddMassMailTask(draft, sender, raceMask);

    PSendSysMessage(LANG_MAIL_SENT, name);
    return true;
}

bool ChatHandler::HandleSendItemsHelper(MailDraft& draft, char* args)
{
    // format: "subject text" "mail text" item1[:count1] item2[:count2] ... item12[:count12]
    char* msgSubject = ExtractQuotedArg(&args);
    if (!msgSubject)
        return false;

    char* msgText = ExtractQuotedArg(&args);
    if (!msgText)
        return false;

    // extract items
    typedef std::pair<uint32, uint32> ItemPair;
    typedef std::list< ItemPair > ItemPairs;
    ItemPairs items;

    // get from tail next item str
    while (char* itemStr = ExtractArg(&args))
    {
        // parse item str
        uint32 item_id = 0;
        uint32 item_count = 1;
        if (sscanf(itemStr, "%u:%u", &item_id, &item_count) != 2)
            if (sscanf(itemStr, "%u", &item_id) != 1)
                return false;

        if (!item_id)
        {
            PSendSysMessage(LANG_COMMAND_ITEMIDINVALID, item_id);
            SetSentErrorMessage(true);
            return false;
        }

        ItemPrototype const* item_proto = ObjectMgr::GetItemPrototype(item_id);
        if (!item_proto)
        {
            PSendSysMessage(LANG_COMMAND_ITEMIDINVALID, item_id);
            SetSentErrorMessage(true);
            return false;
        }

        if (item_count < 1 || (item_proto->MaxCount > 0 && item_count > uint32(item_proto->MaxCount)))
        {
            PSendSysMessage(LANG_COMMAND_INVALID_ITEM_COUNT, item_count, item_id);
            SetSentErrorMessage(true);
            return false;
        }

        while (item_count > item_proto->GetMaxStackSize())
        {
            items.push_back(ItemPair(item_id, item_proto->GetMaxStackSize()));
            item_count -= item_proto->GetMaxStackSize();
        }

        items.push_back(ItemPair(item_id, item_count));

        if (items.size() > MAX_MAIL_ITEMS)
        {
            PSendSysMessage(LANG_COMMAND_MAIL_ITEMS_LIMIT, MAX_MAIL_ITEMS);
            SetSentErrorMessage(true);
            return false;
        }
    }

    // fill mail
    draft.SetSubjectAndBody(msgSubject, msgText);

    for (ItemPairs::const_iterator itr = items.begin(); itr != items.end(); ++itr)
    {
        if (Item* item = Item::CreateItem(itr->first, itr->second, m_session ? m_session->GetPlayer() : 0))
        {
            item->SaveToDB();                               // save for prevent lost at next mail load, if send fail then item will deleted
            draft.AddItem(item);
        }
    }

    return true;
}

bool ChatHandler::HandleSendItemsCommand(char* args)
{
    // format: name "subject text" "mail text" item1[:count1] item2[:count2] ... item12[:count12]
    Player* receiver;
    ObjectGuid receiver_guid;
    std::string receiver_name;
    if (!ExtractPlayerTarget(&args, &receiver, &receiver_guid, &receiver_name))
        return false;

    MailDraft draft;

    // fill mail
    if (!HandleSendItemsHelper(draft, args))
        return false;

    // from console show nonexistent sender
    MailSender sender(MAIL_NORMAL, m_session ? m_session->GetPlayer()->GetObjectGuid().GetCounter() : 0, MAIL_STATIONERY_GM);

    draft.SendMailTo(MailReceiver(receiver, receiver_guid), sender);

    std::string nameLink = playerLink(receiver_name);
    PSendSysMessage(LANG_MAIL_SENT, nameLink.c_str());
    return true;
}

bool ChatHandler::HandleSendMassItemsCommand(char* args)
{
    // format: racemask "subject text" "mail text" item1[:count1] item2[:count2] ... item12[:count12]

    uint32 raceMask = 0;
    char const* name = nullptr;

    if (!ExtractRaceMask(&args, raceMask, &name))
        return false;

    // need dynamic object because it trasfered to mass mailer
    MailDraft* draft = new MailDraft;


    // fill mail
    if (!HandleSendItemsHelper(*draft, args))
    {
        delete draft;
        return false;
    }

    // from console show nonexistent sender
    MailSender sender(MAIL_NORMAL, m_session ? m_session->GetPlayer()->GetObjectGuid().GetCounter() : 0, MAIL_STATIONERY_GM);

    sMassMailMgr.AddMassMailTask(draft, sender, raceMask);

    PSendSysMessage(LANG_MAIL_SENT, name);
    return true;
}

bool ChatHandler::HandleSendMoneyHelper(MailDraft& draft, char* args)
{
    /// format: "subject text" "mail text" money

    char* msgSubject = ExtractQuotedArg(&args);
    if (!msgSubject)
        return false;

    char* msgText = ExtractQuotedArg(&args);
    if (!msgText)
        return false;

    uint32 money;
    if (!ExtractUInt32(&args, money))
        return false;

    if (money <= 0)
        return false;

    // msgSubject, msgText isn't NUL after prev. check
    draft.SetSubjectAndBody(msgSubject, msgText).SetMoney(money);

    return true;
}

bool ChatHandler::HandleSendMoneyCommand(char* args)
{
    /// format: name "subject text" "mail text" money

    Player* receiver;
    ObjectGuid receiver_guid;
    std::string receiver_name;
    if (!ExtractPlayerTarget(&args, &receiver, &receiver_guid, &receiver_name))
        return false;

    MailDraft draft;

    // fill mail
    if (!HandleSendMoneyHelper(draft, args))
        return false;

    // from console show nonexistent sender
    MailSender sender(MAIL_NORMAL, m_session ? m_session->GetPlayer()->GetObjectGuid().GetCounter() : 0, MAIL_STATIONERY_GM);

    draft.SendMailTo(MailReceiver(receiver, receiver_guid), sender);

    std::string nameLink = playerLink(receiver_name);
    PSendSysMessage(LANG_MAIL_SENT, nameLink.c_str());
    return true;
}

bool ChatHandler::HandleSendMassMoneyCommand(char* args)
{
    /// format: raceMask "subject text" "mail text" money

    uint32 raceMask = 0;
    char const* name = nullptr;

    if (!ExtractRaceMask(&args, raceMask, &name))
        return false;

    // need dynamic object because it trasfered to mass mailer
    MailDraft* draft = new MailDraft;

    // fill mail
    if (!HandleSendMoneyHelper(*draft, args))
    {
        delete draft;
        return false;
    }

    // from console show nonexistent sender
    MailSender sender(MAIL_NORMAL, m_session ? m_session->GetPlayer()->GetObjectGuid().GetCounter() : 0, MAIL_STATIONERY_GM);

    sMassMailMgr.AddMassMailTask(draft, sender, raceMask);

    PSendSysMessage(LANG_MAIL_SENT, name);
    return true;
}

// Send mail by command
bool ChatHandler::HandleSendMailCommand(char* args)
{
    // format: name "subject text" "mail text"
    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&args, &target, &target_guid, &target_name))
        return false;

    MailDraft draft;

    // fill draft
    if (!HandleSendMailHelper(draft, args))
        return false;

    // from console show nonexistent sender
    MailSender sender(MAIL_NORMAL, m_session ? m_session->GetPlayer()->GetObjectGuid().GetCounter() : 0, MAIL_STATIONERY_GM);

    draft.SendMailTo(MailReceiver(target, target_guid), sender);

    std::string nameLink = playerLink(target_name);
    PSendSysMessage(LANG_MAIL_SENT, nameLink.c_str());
    return true;
}

/// Send a message to a player in game
bool ChatHandler::HandleSendMessageCommand(char* args)
{
    ///- Find the player
    Player *rPlayer;
    if (!ExtractPlayerTarget(&args, &rPlayer))
        return false;

    ///- message
    if (!*args)
        return false;

    ///- Check that he is not logging out.
    if (rPlayer->GetSession()->isLogingOut())
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    ///- Send the message
    //Use SendAreaTriggerMessage for fastest delivery.
    rPlayer->GetSession()->SendAreaTriggerMessage("%s", args);
    rPlayer->GetSession()->SendAreaTriggerMessage("|cffff0000[Message from administrator]:|r");

    //Confirmation message
    std::string nameLink = GetNameLink(rPlayer);
    PSendSysMessage(LANG_SENDMESSAGE, nameLink.c_str(), args);
    return true;
}

bool ChatHandler::HandlePoolUpdateCommand(char* args)
{
    Player* player = m_session->GetPlayer();

    MapPersistentState* mapState = player->GetMap()->GetPersistentState();
    SpawnedPoolData& spawns = mapState->GetSpawnedPoolData();

    // shared continent pools data expected too big for show
    uint32 pool_id = 0;
    if (!ExtractUint32KeyFromLink(&args, "Hpool", pool_id))
        return false;

    PoolTemplateData const& pool_template = sPoolMgr.GetPoolTemplate(pool_id);

    PSendSysMessage("Pool #%u: %u objects spawned [limit = %u]", pool_id, spawns.GetSpawnedObjects(pool_id), pool_template.MaxLimit);
    sPoolMgr.UpdatePool<GameObject>(*mapState, pool_id);
    sPoolMgr.UpdatePool<Creature>(*mapState, pool_id);
    return true;
}

bool ChatHandler::HandlePoolSpawnsCommand(char* args)
{
    Player* player = m_session->GetPlayer();

    MapPersistentState* mapState = player->GetMap()->GetPersistentState();

    // shared continent pools data expected too big for show
    uint32 pool_id = 0;
    if (!ExtractUint32KeyFromLink(&args, "Hpool", pool_id) && !mapState->GetMapEntry()->Instanceable())
    {
        PSendSysMessage(LANG_POOL_SPAWNS_NON_INSTANCE, mapState->GetMapEntry()->name, mapState->GetMapId());
        SetSentErrorMessage(false);
        return false;
    }

    SpawnedPoolData const& spawns = mapState->GetSpawnedPoolData();

    SpawnedPoolObjects const& crSpawns = spawns.GetSpawnedCreatures();
    for (SpawnedPoolObjects::const_iterator itr = crSpawns.begin(); itr != crSpawns.end(); ++itr)
        if (!pool_id || pool_id == sPoolMgr.IsPartOfAPool<Creature>(*itr))
            if (CreatureData const* data = sObjectMgr.GetCreatureData(*itr))
                if (CreatureInfo const* info = ObjectMgr::GetCreatureTemplate(data->creature_id[0]))
                    PSendSysMessage(LANG_CREATURE_LIST_CHAT, *itr, PrepareStringNpcOrGoSpawnInformation<Creature>(*itr).c_str(),
                                    *itr, info->name, data->posX, data->posY, data->posZ, data->mapid);

    SpawnedPoolObjects const& goSpawns = spawns.GetSpawnedGameobjects();
    for (SpawnedPoolObjects::const_iterator itr = goSpawns.begin(); itr != goSpawns.end(); ++itr)
        if (!pool_id || pool_id == sPoolMgr.IsPartOfAPool<GameObject>(*itr))
            if (GameObjectData const* data = sObjectMgr.GetGOData(*itr))
                if (GameObjectInfo const* info = ObjectMgr::GetGameObjectInfo(data->id))
                    PSendSysMessage(LANG_GO_LIST_CHAT, *itr, PrepareStringNpcOrGoSpawnInformation<GameObject>(*itr).c_str(),
                                    *itr, info->name, data->posX, data->posY, data->posZ, data->mapid);

    return true;
}

bool ChatHandler::HandlePoolInfoCommand(char* args)
{
    // id or [name] Shift-click form |color|Hpool:id|h[name]|h|r
    uint32 pool_id;
    if (!ExtractUint32KeyFromLink(&args, "Hpool", pool_id))
        return false;

    Player* player = m_session ? m_session->GetPlayer() : nullptr;

    MapPersistentState* mapState = player ? player->GetMap()->GetPersistentState() : nullptr;
    SpawnedPoolData const* spawns = mapState ? &mapState->GetSpawnedPoolData() : nullptr;

    std::string active_str = GetMangosString(LANG_ACTIVE);

    PoolTemplateData const& pool_template = sPoolMgr.GetPoolTemplate(pool_id);
    uint32 mother_pool_id = sPoolMgr.IsPartOfAPool<Pool>(pool_id);
    if (!mother_pool_id)
        PSendSysMessage(LANG_POOL_INFO_HEADER, pool_id, pool_template.IsAutoSpawn(), pool_template.MaxLimit);
    else
    {
        PoolTemplateData const& mother_template = sPoolMgr.GetPoolTemplate(mother_pool_id);
        if (m_session)
            PSendSysMessage(LANG_POOL_INFO_HEADER_CHAT, pool_id, mother_pool_id, mother_pool_id, mother_template.description.c_str(),
                            pool_template.IsAutoSpawn(), pool_template.MaxLimit);
        else
            PSendSysMessage(LANG_POOL_INFO_HEADER_CONSOLE, pool_id, mother_pool_id, mother_template.description.c_str(),
                            pool_template.IsAutoSpawn(), pool_template.MaxLimit);
    }

    PoolGroup<Creature> const& poolCreatures = sPoolMgr.GetPoolCreatures(pool_id);
    SpawnedPoolObjects const* crSpawns = spawns ? &spawns->GetSpawnedCreatures() : nullptr;

    PoolObjectList const& poolCreaturesEx = poolCreatures.GetExplicitlyChanced();
    if (!poolCreaturesEx.empty())
    {
        SendSysMessage(LANG_POOL_CHANCE_CREATURE_LIST_HEADER);
        for (PoolObjectList::const_iterator itr = poolCreaturesEx.begin(); itr != poolCreaturesEx.end(); ++itr)
        {
            if (CreatureData const* data = sObjectMgr.GetCreatureData(itr->guid))
            {
                if (CreatureInfo const* info = ObjectMgr::GetCreatureTemplate(data->creature_id[0]))
                {
                    char const* active = crSpawns && crSpawns->find(itr->guid) != crSpawns->end() ? active_str.c_str() : "";
                    if (m_session)
                        PSendSysMessage(LANG_POOL_CHANCE_CREATURE_LIST_CHAT, itr->guid, PrepareStringNpcOrGoSpawnInformation<Creature>(itr->guid).c_str(),
                                        itr->guid, info->name, data->posX, data->posY, data->posZ, data->mapid, itr->chance, active);
                    else
                        PSendSysMessage(LANG_POOL_CHANCE_CREATURE_LIST_CONSOLE, itr->guid, PrepareStringNpcOrGoSpawnInformation<Creature>(itr->guid).c_str(),
                                        info->name, data->posX, data->posY, data->posZ, data->mapid, itr->chance, active);
                }
            }
        }
    }

    PoolObjectList const& poolCreaturesEq = poolCreatures.GetEqualChanced();
    if (!poolCreaturesEq.empty())
    {
        SendSysMessage(LANG_POOL_CREATURE_LIST_HEADER);
        for (PoolObjectList::const_iterator itr = poolCreaturesEq.begin(); itr != poolCreaturesEq.end(); ++itr)
        {
            if (CreatureData const* data = sObjectMgr.GetCreatureData(itr->guid))
            {
                if (CreatureInfo const* info = ObjectMgr::GetCreatureTemplate(data->creature_id[0]))
                {
                    char const* active = crSpawns && crSpawns->find(itr->guid) != crSpawns->end() ? active_str.c_str() : "";
                    if (m_session)
                        PSendSysMessage(LANG_POOL_CREATURE_LIST_CHAT, itr->guid, PrepareStringNpcOrGoSpawnInformation<Creature>(itr->guid).c_str(),
                                        itr->guid, info->name, data->posX, data->posY, data->posZ, data->mapid, active);
                    else
                        PSendSysMessage(LANG_POOL_CREATURE_LIST_CONSOLE, itr->guid, PrepareStringNpcOrGoSpawnInformation<Creature>(itr->guid).c_str(),
                                        info->name, data->posX, data->posY, data->posZ, data->mapid, active);
                }
            }
        }
    }

    PoolGroup<GameObject> const& poolGameObjects = sPoolMgr.GetPoolGameObjects(pool_id);
    SpawnedPoolObjects const* goSpawns = spawns ? &spawns->GetSpawnedGameobjects() : nullptr;

    PoolObjectList const& poolGameObjectsEx = poolGameObjects.GetExplicitlyChanced();
    if (!poolGameObjectsEx.empty())
    {
        SendSysMessage(LANG_POOL_CHANCE_GO_LIST_HEADER);
        for (PoolObjectList::const_iterator itr = poolGameObjectsEx.begin(); itr != poolGameObjectsEx.end(); ++itr)
        {
            if (GameObjectData const* data = sObjectMgr.GetGOData(itr->guid))
            {
                if (GameObjectInfo const* info = ObjectMgr::GetGameObjectInfo(data->id))
                {
                    char const* active = goSpawns && goSpawns->find(itr->guid) != goSpawns->end() ? active_str.c_str() : "";
                    if (m_session)
                        PSendSysMessage(LANG_POOL_CHANCE_GO_LIST_CHAT, itr->guid, PrepareStringNpcOrGoSpawnInformation<GameObject>(itr->guid).c_str(),
                                        itr->guid, info->name, data->posX, data->posY, data->posZ, data->mapid, itr->chance, active);
                    else
                        PSendSysMessage(LANG_POOL_CHANCE_GO_LIST_CONSOLE, itr->guid, PrepareStringNpcOrGoSpawnInformation<GameObject>(itr->guid).c_str(),
                                        info->name, data->posX, data->posY, data->posZ, data->mapid, itr->chance, active);
                }
            }
        }
    }

    PoolObjectList const& poolGameObjectsEq = poolGameObjects.GetEqualChanced();
    if (!poolGameObjectsEq.empty())
    {
        SendSysMessage(LANG_POOL_GO_LIST_HEADER);
        for (PoolObjectList::const_iterator itr = poolGameObjectsEq.begin(); itr != poolGameObjectsEq.end(); ++itr)
        {
            if (GameObjectData const* data = sObjectMgr.GetGOData(itr->guid))
            {
                if (GameObjectInfo const* info = ObjectMgr::GetGameObjectInfo(data->id))
                {
                    char const* active = goSpawns && goSpawns->find(itr->guid) != goSpawns->end() ? active_str.c_str() : "";
                    if (m_session)
                        PSendSysMessage(LANG_POOL_GO_LIST_CHAT, itr->guid, PrepareStringNpcOrGoSpawnInformation<GameObject>(itr->guid).c_str(),
                                        itr->guid, info->name, data->posX, data->posY, data->posZ, data->mapid, active);
                    else
                        PSendSysMessage(LANG_POOL_GO_LIST_CONSOLE, itr->guid, PrepareStringNpcOrGoSpawnInformation<GameObject>(itr->guid).c_str(),
                                        info->name, data->posX, data->posY, data->posZ, data->mapid, active);
                }
            }
        }
    }

    PoolGroup<Pool> const& poolPools = sPoolMgr.GetPoolPools(pool_id);
    SpawnedPoolPools const* poolSpawns = spawns ? &spawns->GetSpawnedPools() : nullptr;

    PoolObjectList const& poolPoolsEx = poolPools.GetExplicitlyChanced();
    if (!poolPoolsEx.empty())
    {
        SendSysMessage(LANG_POOL_CHANCE_POOL_LIST_HEADER);
        for (PoolObjectList::const_iterator itr = poolPoolsEx.begin(); itr != poolPoolsEx.end(); ++itr)
        {
            PoolTemplateData const& itr_template = sPoolMgr.GetPoolTemplate(itr->guid);
            char const* active = poolSpawns && poolSpawns->find(itr->guid) != poolSpawns->end() ? active_str.c_str() : "";
            if (m_session)
                PSendSysMessage(LANG_POOL_CHANCE_POOL_LIST_CHAT, itr->guid,
                                itr->guid, itr_template.description.c_str(), itr_template.IsAutoSpawn() ? 1 : 0, itr_template.MaxLimit,
                                sPoolMgr.GetPoolCreatures(itr->guid).size(), sPoolMgr.GetPoolGameObjects(itr->guid).size(), sPoolMgr.GetPoolPools(itr->guid).size(),
                                itr->chance, active);
            else
                PSendSysMessage(LANG_POOL_CHANCE_POOL_LIST_CONSOLE, itr->guid,
                                itr_template.description.c_str(), itr_template.IsAutoSpawn() ? 1 : 0, itr_template.MaxLimit,
                                sPoolMgr.GetPoolCreatures(itr->guid).size(), sPoolMgr.GetPoolGameObjects(itr->guid).size(), sPoolMgr.GetPoolPools(itr->guid).size(),
                                itr->chance, active);
        }
    }

    PoolObjectList const& poolPoolsEq = poolPools.GetEqualChanced();
    if (!poolPoolsEq.empty())
    {
        SendSysMessage(LANG_POOL_POOL_LIST_HEADER);
        for (PoolObjectList::const_iterator itr = poolPoolsEq.begin(); itr != poolPoolsEq.end(); ++itr)
        {
            PoolTemplateData const& itr_template = sPoolMgr.GetPoolTemplate(itr->guid);
            char const* active = poolSpawns && poolSpawns->find(itr->guid) != poolSpawns->end() ? active_str.c_str() : "";
            if (m_session)
                PSendSysMessage(LANG_POOL_POOL_LIST_CHAT, itr->guid,
                                itr->guid, itr_template.description.c_str(), itr_template.IsAutoSpawn() ? 1 : 0, itr_template.MaxLimit,
                                sPoolMgr.GetPoolCreatures(itr->guid).size(), sPoolMgr.GetPoolGameObjects(itr->guid).size(), sPoolMgr.GetPoolPools(itr->guid).size(),
                                active);
            else
                PSendSysMessage(LANG_POOL_POOL_LIST_CONSOLE, itr->guid,
                                itr_template.description.c_str(), itr_template.IsAutoSpawn() ? 1 : 0, itr_template.MaxLimit,
                                sPoolMgr.GetPoolCreatures(itr->guid).size(), sPoolMgr.GetPoolGameObjects(itr->guid).size(), sPoolMgr.GetPoolPools(itr->guid).size(),
                                active);
        }
    }
    return true;
}

void ChatHandler::ShowTriggerTargetListHelper(uint32 id, AreaTriggerTeleport const* at, bool subpart /*= false*/)
{
    if (m_session)
    {
        char dist_buf[50];
        if (!subpart)
        {
            float dist = m_session->GetPlayer()->GetDistance2d(at->target_X, at->target_Y);
            snprintf(dist_buf, 50, GetMangosString(LANG_TRIGGER_DIST), dist);
        }
        else
            dist_buf[0] = '\0';

        PSendSysMessage(LANG_TRIGGER_TARGET_LIST_CHAT,
                        subpart ? " -> " : "", id, id, at->target_mapId, at->target_X, at->target_Y, at->target_Z, dist_buf);
    }
    else
        PSendSysMessage(LANG_TRIGGER_TARGET_LIST_CONSOLE,
                        subpart ? " -> " : "", id, at->target_mapId, at->target_X, at->target_Y, at->target_Z);
}

void ChatHandler::ShowTriggerListHelper(AreaTriggerEntry const * atEntry)
{

    char const* tavern = sObjectMgr.IsTavernAreaTrigger(atEntry->id) ? GetMangosString(LANG_TRIGGER_TAVERN) : "";
    char const* quest = sObjectMgr.GetQuestForAreaTrigger(atEntry->id) ? GetMangosString(LANG_TRIGGER_QUEST) : "";

    if (m_session)
    {
        float dist = m_session->GetPlayer()->GetDistance2d(atEntry->x, atEntry->y);
        char dist_buf[50];
        snprintf(dist_buf, 50, GetMangosString(LANG_TRIGGER_DIST), dist);

        PSendSysMessage(LANG_TRIGGER_LIST_CHAT,
                        atEntry->id, atEntry->id, atEntry->mapid, atEntry->x, atEntry->y, atEntry->z, dist_buf, tavern, quest);
    }
    else
        PSendSysMessage(LANG_TRIGGER_LIST_CONSOLE,
                        atEntry->id, atEntry->mapid, atEntry->x, atEntry->y, atEntry->z, tavern, quest);

    if (AreaTriggerTeleport const* at = sObjectMgr.GetAreaTriggerTeleport(atEntry->id))
        ShowTriggerTargetListHelper(atEntry->id, at, true);
}

bool ChatHandler::HandleTriggerCommand(char* args)
{
    AreaTriggerEntry const* atEntry = nullptr;

    Player* pl = m_session ? m_session->GetPlayer() : nullptr;

    // select by args
    if (*args)
    {
        uint32 atId;
        if (!ExtractUint32KeyFromLink(&args, "Hareatrigger", atId))
            return false;

        if (!atId)
            return false;

        atEntry = sObjectMgr.GetAreaTrigger(atId);

        if (!atEntry)
        {
            PSendSysMessage(LANG_COMMAND_GOAREATRNOTFOUND, atId);
            SetSentErrorMessage(true);
            return false;
        }
    }
    // find nearest
    else
    {
        if (!m_session)
            return false;

        float dist2 = MAP_SIZE * MAP_SIZE;

        Player* pl = m_session->GetPlayer();

        // Search triggers
        for (auto const itr : sObjectMgr.GetAreaTriggersMap())
        {
            AreaTriggerEntry const *atTestEntry = &itr.second;
            if (!atTestEntry)
                continue;

            if (atTestEntry->mapid != m_session->GetPlayer()->GetMapId())
                continue;

            float dx = atTestEntry->x - pl->GetPositionX();
            float dy = atTestEntry->y - pl->GetPositionY();

            float test_dist2 = dx * dx + dy * dy;

            if (test_dist2 >= dist2)
                continue;

            dist2 = test_dist2;
            atEntry = atTestEntry;
        }

        if (!atEntry)
        {
            SendSysMessage(LANG_COMMAND_NOTRIGGERFOUND);
            SetSentErrorMessage(true);
            return false;
        }
    }

    ShowTriggerListHelper(atEntry);

    int loc_idx = GetSessionDbLocaleIndex();

    AreaTriggerTeleport const* at = sObjectMgr.GetAreaTriggerTeleport(atEntry->id);
    if (at)
        PSendSysMessage(LANG_TRIGGER_REQ_LEVEL, at->requiredLevel);

    if (uint32 quest_id = sObjectMgr.GetQuestForAreaTrigger(atEntry->id))
    {
        SendSysMessage(LANG_TRIGGER_EXPLORE_QUEST);
        ShowQuestListHelper(quest_id, loc_idx, pl);
    }

    if (at)
    {
        if (at->requiredItem || at->requiredItem2)
        {
            SendSysMessage(LANG_TRIGGER_REQ_ITEMS);

            if (at->requiredItem)
                ShowItemListHelper(at->requiredItem, loc_idx, pl);
            if (at->requiredItem2)
                ShowItemListHelper(at->requiredItem2, loc_idx, pl);
        }

        if (at->requiredQuest)
        {
            SendSysMessage(LANG_TRIGGER_REQ_QUEST);
            ShowQuestListHelper(at->requiredQuest, loc_idx, pl);
        }
    }

    return true;
}

bool ChatHandler::HandleTriggerActiveCommand(char* /*args*/)
{
    uint32 counter = 0;                                     // Counter for figure out that we found smth.

    Player* pl = m_session->GetPlayer();

    // Search in AreaTable.dbc
    for (auto const itr : sObjectMgr.GetAreaTriggersMap())
    {
        AreaTriggerEntry const *atEntry = &itr.second;
        if (!atEntry)
            continue;

        if (!IsPointInAreaTriggerZone(atEntry, pl->GetMapId(), pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ()))
            continue;

        ShowTriggerListHelper(atEntry);

        ++counter;
    }

    if (counter == 0)                                      // if counter == 0 then we found nth
        SendSysMessage(LANG_COMMAND_NOTRIGGERFOUND);

    return true;
}

bool ChatHandler::HandleTriggerNearCommand(char* args)
{
    float distance = (!*args) ? 10.0f : (float)atof(args);
    float dist2 =  distance * distance;
    uint32 counter = 0;                                     // Counter for figure out that we found smth.

    Player* pl = m_session->GetPlayer();

    // Search triggers
    for (auto const itr : sObjectMgr.GetAreaTriggersMap())
    {
        AreaTriggerEntry const *atEntry = &itr.second;
        if (!atEntry)
            continue;

        if (atEntry->mapid != m_session->GetPlayer()->GetMapId())
            continue;

        float dx = atEntry->x - pl->GetPositionX();
        float dy = atEntry->y - pl->GetPositionY();

        if (dx * dx + dy * dy > dist2)
            continue;

        ShowTriggerListHelper(atEntry);

        ++counter;
    }

    // Search trigger targets
    for (auto const itr : sObjectMgr.GetAreaTriggersMap())
    {
        AreaTriggerEntry const *atEntry = &itr.second;
        if (!atEntry)
            continue;

        AreaTriggerTeleport const* at = sObjectMgr.GetAreaTriggerTeleport(atEntry->id);
        if (!at)
            continue;

        if (at->target_mapId != m_session->GetPlayer()->GetMapId())
            continue;

        float dx = at->target_X - pl->GetPositionX();
        float dy = at->target_Y - pl->GetPositionY();

        if (dx * dx + dy * dy > dist2)
            continue;

        ShowTriggerTargetListHelper(atEntry->id, at);

        ++counter;
    }

    if (counter == 0)                                      // if counter == 0 then we found nth
        SendSysMessage(LANG_COMMAND_NOTRIGGERFOUND);

    return true;
}

bool ChatHandler::HandleCinematicAddWpCommand(char *args)
{
    uint32 cinematic_id = 0;
    uint32 timer = 0;
    char comment[100];

    sscanf(args, "%u %u %s", &cinematic_id, &timer, comment);

    Player* me = m_session->GetPlayer();
    WorldDatabase.PExecute(
        "INSERT INTO `cinematic_waypoints` (`cinematic`, `timer`, `position_x`, `position_y`, `position_z`, `comment`) VALUES "
        "(%u, %u, %f, %f, %f, '%s')",
        cinematic_id, timer,
        ceil(me->GetPositionX()), ceil(me->GetPositionY()), ceil(me->GetPositionZ()), comment
    );

    PSendSysMessage("Added new waypoint at %u ms for cinematic %u.", timer, cinematic_id);
    sObjectMgr.LoadCinematicsWaypoints();
    return true;
}

bool ChatHandler::HandleCinematicGoTimeCommand(char *args)
{
    uint32 cinematic_id = 0;
    uint32 time = 0;

    sscanf(args, "%u %u", &cinematic_id, &time);

    Player* me = m_session->GetPlayer();
    Position const* tpPosition = sObjectMgr.GetCinematicPosition(cinematic_id, time);
    if (!tpPosition)
    {
        SendSysMessage("Cannot find location.");
        SetSentErrorMessage(true);
        return false;
    }

    if (me)
    {
        me->TeleportTo(me->GetMapId(), tpPosition->x, tpPosition->y, tpPosition->z, 0.0f);
        PSendSysMessage("You are now at position (%f %f %f)", tpPosition->x, tpPosition->y, tpPosition->z);
    }
    else
        PSendSysMessage("Position found (%f %f %f)", tpPosition->x, tpPosition->y, tpPosition->z);

    return true;
}

bool ChatHandler::HandleCinematicListWpCommand(char *args)
{
    // TODO
    // Donne une liste des WayPoints en fonction de la cinematique envoyee
    //
    // Exemple :
    // .cine listwp 41
    //
    return true;
}

bool ChatHandler::HandleReplayPlayCommand(char* c)
{
    if (!c || !*c || strchr(c, '/') != nullptr || strchr(c, '.') != nullptr)
        return false;
    WorldSession* sess = m_session;
    if (Player* player = GetSelectedPlayer())
        sess = player->GetSession();
    std::string filename = "replays/";
    filename += c;
    sess->SetReadPacket(filename.c_str());
    if (m_session->IsReplaying())
        PSendSysMessage("Starting replay %s for %s", c, playerLink(sess->GetPlayerName()).c_str());
    else
        PSendSysMessage("Could not start replay %s", c);
    return true;
}

bool ChatHandler::HandleReplayForwardCommand(char* c)
{
    if (!m_session->IsReplaying())
    {
        SendSysMessage("Not replaying currently");
        SetSentErrorMessage(true);
        return false;
    }
    int32 secsToSkip = 0;
    ExtractInt32(&c, secsToSkip);
    m_session->ReplaySkipTime(secsToSkip);
    PSendSysMessage("Skipping %i ms", secsToSkip);
    return true;
}

bool ChatHandler::HandleReplaySpeedCommand(char* c)
{
    if (!m_session->IsReplaying())
    {
        SendSysMessage("Not currently replaying");
        SetSentErrorMessage(true);
        return false;
    }
    float newRate = 1.0f;
    ExtractFloat(&c, newRate);
    m_session->SetReplaySpeedRate(newRate);
    PSendSysMessage("Read speed rate changed to %f", newRate);
    return true;
}

bool ChatHandler::HandleReplayStopCommand(char* c)
{
    if (!m_session->IsReplaying())
    {
        SendSysMessage("Not replaying currently");
        SetSentErrorMessage(true);
        return false;
    }
    m_session->SetReadPacket(nullptr);
    SendSysMessage("Replay stopped");
    return true;
}

bool ChatHandler::HandleReplayRecordCommand(char* c)
{
    WorldSession* sess = m_session;
    if (Player* player = GetSelectedPlayer())
        sess = player->GetSession();
    PSendSysMessage("Starting replay recording for %s", playerLink(sess->GetPlayerName()).c_str());
    sess->SetDumpPacket(c);
    return true;
}

// BG
#define COLOR_HORDE      "FF3300"
#define COLOR_ALLIANCE   "0066B3"
#define COLOR_BG         "D580FE"
#define COLOR_INFO       "FF9900"
#define COLOR_STATUS     "FECCBF"
#define DO_COLOR(a, b)   "|cff" a "" b "|r"

typedef std::map<ObjectGuid, BattleGroundPlayer> BattleGroundPlayerMap;
bool ChatHandler::HandleBGStatusCommand(char *args)
{
    Player *chr = m_session->GetPlayer();
    ASSERT(chr);
    SendSysMessage(DO_COLOR(COLOR_INFO, "-- Currently running BGs"));
    uint8 i = 0;
    uint8 uiAllianceCount, uiHordeCount;
    for (int8 bgTypeId = BATTLEGROUND_AB; bgTypeId >= BATTLEGROUND_AV; --bgTypeId)
    {
        for (BattleGroundSet::const_iterator it = sBattleGroundMgr.GetBattleGroundsBegin(BattleGroundTypeId(bgTypeId)); it != sBattleGroundMgr.GetBattleGroundsEnd(BattleGroundTypeId(bgTypeId)); ++it)
        {
            // Pas un "vrai" BG, mais un "modele" de BG.
            if (!it->first)
                continue;

            ++i;
            uiAllianceCount = 0;
            uiHordeCount    = 0;
            BattleGroundPlayerMap const& pPlayers = it->second->GetPlayers();
            std::string playerName = "";

            for (BattleGroundPlayerMap::const_iterator itr = pPlayers.begin(); itr != pPlayers.end(); ++itr)
            {
                if (itr->second.PlayerTeam == HORDE)
                    uiHordeCount++;
                else
                    uiAllianceCount++;
                if (playerName == "")
                    if (sObjectMgr.GetPlayerNameByGUID(itr->first, playerName))
                        playerName = playerLink(playerName);
            }

            std::string statusName;
            BattleGroundStatus status = it->second->GetStatus();
            switch (status)
            {
                case STATUS_WAIT_JOIN:
                    statusName = "WaitJoin";
                    break;
                case STATUS_IN_PROGRESS:
                    statusName = "InProgress";
                    break;
                case STATUS_WAIT_LEAVE:
                    statusName = "WaitLeave";
                    break;
            }

            PSendSysMessage(DO_COLOR(COLOR_BG, "[%s %2u]") " [%2u-%2u] "
                    DO_COLOR(COLOR_STATUS, "[%s]")
                    DO_COLOR(COLOR_ALLIANCE, "%2u") "vs" DO_COLOR(COLOR_HORDE, "%2u")
                    " Player:%s %s",
                    it->second->GetName(), it->first, it->second->GetMinLevel(), it->second->GetMaxLevel(), statusName.c_str(),
                    uiAllianceCount, uiHordeCount,
                    playerName.c_str(), secsToTimeString(it->second->GetStartTime() / 1000, true).c_str());
        }
    }
    if (!i)
        PSendSysMessage(DO_COLOR(COLOR_INFO, "(No battleground started)"));

    PSendSysMessage(DO_COLOR(COLOR_INFO, "-- Queues for your bracket"));
    i = 0;

    for (uint8 bgTypeId = BATTLEGROUND_AV; bgTypeId <= BATTLEGROUND_AB; ++bgTypeId)
    {
        ++i;
        uiAllianceCount = 0;
        uiHordeCount    = 0;

        BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(BattleGroundTypeId(bgTypeId));
        // Doit etre une référence (&), sinon crash par la suite ...
        BattleGroundQueue& queue = sBattleGroundMgr.m_BattleGroundQueues[bgQueueTypeId];
        for (BattleGroundQueue::QueuedPlayersMap::const_iterator it = queue.m_QueuedPlayers.begin(); it != queue.m_QueuedPlayers.end(); ++it)
        {
            if (it->second.GroupInfo->GroupTeam == HORDE)
                uiHordeCount++;
            else
                uiAllianceCount++;
        }

        BattleGround *bg_template = sBattleGroundMgr.GetBattleGroundTemplate(BattleGroundTypeId(bgTypeId));
        ASSERT(bg_template);

        PSendSysMessage(DO_COLOR(COLOR_BG, "[%s]" "   " DO_COLOR(COLOR_ALLIANCE, "[Alliance] : %2u") " - " DO_COLOR(COLOR_HORDE, "[Horde] : %2u")),
                        bg_template->GetName(), uiAllianceCount, uiHordeCount);
    }
    if (!i)
        PSendSysMessage(DO_COLOR(COLOR_INFO, "(No player queued)"));
    return true;
}

bool ChatHandler::HandleBGStartCommand(char *args)
{
    Player *chr = m_session->GetPlayer();
    ASSERT(chr);
    BattleGround* pBg = chr->GetBattleGround();
    if (!pBg)
    {
        SendSysMessage("Vous devez etre dans un champs de bataille pour utiliser cette commande.");
        SetSentErrorMessage(true);
        return false;
    }
    pBg->SetStartDelayTime(0);
    PSendSysMessage("BG commence [%s][%u]", pBg->GetName(), pBg->GetInstanceID());
    return true;
}

bool ChatHandler::HandleBGStopCommand(char *args)
{
    Player *chr = m_session->GetPlayer();
    ASSERT(chr);
    BattleGround* pBg = chr->GetBattleGround();
    if (!pBg)
    {
        SendSysMessage("You are not in a battleground");
        SetSentErrorMessage(true);
        return false;
    }
    pBg->StopBattleGround();
    PSendSysMessage("Battleground stopped [%s][%u]", pBg->GetName(), pBg->GetInstanceID());
    return true;
}

bool ChatHandler::HandleBGCustomCommand(char *args)
{
    Player *chr = m_session->GetPlayer();
    ASSERT(chr);
    BattleGround* pBg = chr->GetBattleGround();
    if (!pBg)
    {
        SendSysMessage("You are not in a battleground");
        SetSentErrorMessage(true);
        return false;
    }
    pBg->HandleCommand(chr, this, args);
    return true;

}

bool ChatHandler::HandleLinkGraveCommand(char* args)
{
    uint32 g_id;
    if (!ExtractUInt32(&args, g_id))
        return false;

    char* teamStr = ExtractLiteralArg(&args);

    Team g_team;
    if (!teamStr)
        g_team = TEAM_NONE;
    else if (strncmp(teamStr, "horde", strlen(teamStr)) == 0)
        g_team = HORDE;
    else if (strncmp(teamStr, "alliance", strlen(teamStr)) == 0)
        g_team = ALLIANCE;
    else
        return false;

    WorldSafeLocsEntry const* graveyard =  sWorldSafeLocsStore.LookupEntry(g_id);

    if (!graveyard)
    {
        PSendSysMessage(LANG_COMMAND_GRAVEYARDNOEXIST, g_id);
        SetSentErrorMessage(true);
        return false;
    }

    Player* player = m_session->GetPlayer();

    uint32 zoneId = player->GetZoneId();

    const auto *areaEntry = AreaEntry::GetById(zoneId);
    if (!areaEntry || !areaEntry->IsZone())
    {
        PSendSysMessage(LANG_COMMAND_GRAVEYARDWRONGZONE, g_id, zoneId);
        SetSentErrorMessage(true);
        return false;
    }

    if (sObjectMgr.AddGraveYardLink(g_id, zoneId, g_team))
        PSendSysMessage(LANG_COMMAND_GRAVEYARDLINKED, g_id, zoneId);
    else
        PSendSysMessage(LANG_COMMAND_GRAVEYARDALRLINKED, g_id, zoneId);

    return true;
}

bool ChatHandler::HandleNearGraveCommand(char* args)
{
    Team g_team;

    size_t argslen = strlen(args);

    if (!*args)
        g_team = TEAM_NONE;
    else if (strncmp(args, "horde", argslen) == 0)
        g_team = HORDE;
    else if (strncmp(args, "alliance", argslen) == 0)
        g_team = ALLIANCE;
    else
        return false;

    Player* player = m_session->GetPlayer();
    uint32 zone_id = player->GetZoneId();

    WorldSafeLocsEntry const* graveyard = sObjectMgr.GetClosestGraveYard(
            player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetMapId(), g_team);

    if (graveyard)
    {
        uint32 g_id = graveyard->ID;

        GraveYardData const* data = sObjectMgr.FindGraveYardData(g_id, zone_id);
        if (!data)
        {
            PSendSysMessage(LANG_COMMAND_GRAVEYARDERROR, g_id);
            SetSentErrorMessage(true);
            return false;
        }

        g_team = data->team;

        std::string team_name = GetMangosString(LANG_COMMAND_GRAVEYARD_NOTEAM);

        if (g_team == 0)
            team_name = GetMangosString(LANG_COMMAND_GRAVEYARD_ANY);
        else if (g_team == HORDE)
            team_name = GetMangosString(LANG_COMMAND_GRAVEYARD_HORDE);
        else if (g_team == ALLIANCE)
            team_name = GetMangosString(LANG_COMMAND_GRAVEYARD_ALLIANCE);

        PSendSysMessage(LANG_COMMAND_GRAVEYARDNEAREST, g_id, team_name.c_str(), zone_id);
    }
    else
    {
        std::string team_name;

        if (g_team == 0)
            team_name = GetMangosString(LANG_COMMAND_GRAVEYARD_ANY);
        else if (g_team == HORDE)
            team_name = GetMangosString(LANG_COMMAND_GRAVEYARD_HORDE);
        else if (g_team == ALLIANCE)
            team_name = GetMangosString(LANG_COMMAND_GRAVEYARD_ALLIANCE);

        if (g_team == ~uint32(0))
            PSendSysMessage(LANG_COMMAND_ZONENOGRAVEYARDS, zone_id);
        else
            PSendSysMessage(LANG_COMMAND_ZONENOGRAFACTION, zone_id, team_name.c_str());
    }

    return true;
}