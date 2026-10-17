/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
 *
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
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "Opcodes.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "Database/DatabaseEnv.h"
#include "ChannelMgr.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Player.h"
#include "SpellAuras.h"
#include "Language.h"
#include "Util.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Anticheat.h"
#include "AccountMgr.h"
#include "Database/DBCStores.h"
#include "SpellMgr.h"
#include "BattleGroundMgr.h"
#include "BattleGround.h"
#include "Map.h"

bool WorldSession::ProcessChatMessageAfterSecurityCheck(std::string& msg, uint32 lang, uint32 msgType)
{
    if (!IsLanguageAllowedForChatType(lang, msgType))
        return false;

    if (lang != LANG_ADDON)
    {
        // strip invisible characters for non-addon messages
        if (sWorld.getConfig(CONFIG_BOOL_CHAT_FAKE_MESSAGE_PREVENTING))
            stripLineInvisibleChars(msg);

        if (sWorld.getConfig(CONFIG_UINT32_CHAT_STRICT_LINK_CHECKING_SEVERITY) && GetSecurity() < SEC_MODERATOR
                && !ChatHandler(this).isValidChatMessage(msg.c_str()))
        {
            sLog.outError("Player %s (GUID: %u) sent a chatmessage with an invalid link: %s", GetPlayer()->GetName(),
                          GetPlayer()->GetGUIDLow(), msg.c_str());
            if (sWorld.getConfig(CONFIG_UINT32_CHAT_STRICT_LINK_CHECKING_KICK))
                KickPlayer();
            return false;
        }
    }

    ChatHandler handler(this);

    if (handler.ParseCommands(msg.c_str()))
        return false;

    return true;
}

bool WorldSession::IsLanguageAllowedForChatType(uint32 lang, uint32 msgType)
{
    // Right now we'll just restrict addon language to the appropriate chat types
    // Anything else is OK (default previous behaviour)
    switch (lang)
    {
        case LANG_ADDON:
        {
            switch (msgType)
            {
                case CHAT_MSG_PARTY:
                case CHAT_MSG_GUILD:
                case CHAT_MSG_OFFICER:
                case CHAT_MSG_RAID:
                case CHAT_MSG_RAID_LEADER:
                case CHAT_MSG_RAID_WARNING:
                case CHAT_MSG_BATTLEGROUND:
                case CHAT_MSG_BATTLEGROUND_LEADER:
                case CHAT_MSG_CHANNEL:
                    return true;
                default:
                    return false;
            }
        }
        default:
            return true;
    }

    return true;
}

uint32_t WorldSession::ChatCooldown()
{
    ASSERT(GetPlayer());

    auto cooldown = sWorld.getConfig(CONFIG_UINT32_WORLD_CHAN_CD);
    const auto minLevel = sWorld.getConfig(CONFIG_UINT32_WORLD_CHAN_MIN_LEVEL);
    const auto cooldownMaxLvl = sWorld.getConfig(CONFIG_UINT32_WORLD_CHAN_CD_MAX_LEVEL);
    const auto cooldownScaling = sWorld.getConfig(CONFIG_UINT32_WORLD_CHAN_CD_SCALING);
    const auto cooldownUseAcctLvl = sWorld.getConfig(CONFIG_UINT32_WORLD_CHAN_CD_USE_ACCOUNT_MAX_LEVEL);
    const auto playerLevel = cooldownUseAcctLvl? GetAccountMaxLevel() : GetPlayer()->GetLevel();

    if (cooldown && cooldownMaxLvl > playerLevel)
    {
        const auto currTime = time(nullptr);
        const auto delta = currTime - GetLastPubChanMsgTime();

        if (cooldownScaling)
        {
            auto factor = static_cast<double>((cooldownMaxLvl - playerLevel)) / (cooldownMaxLvl - minLevel);
            cooldown *= factor;
        }

        if (delta < cooldown)
        {
            return cooldown - delta;
        }
    }

    return 0;
}

void WorldSession::HandleMessagechatOpcode(WorldPacket & recv_data)
{
    uint32 type;
    uint32 lang;

    recv_data >> type;
    recv_data >> lang;

    if (type >= MAX_CHAT_MSG_TYPE)
    {
        sLog.outError("CHAT: Wrong message type received: %u", type);
        return;
    }

    DEBUG_LOG("CHAT: packet received. type %u, lang %u", type, lang);

    // prevent talking at unknown language (cheating)
    LanguageDesc const* langDesc = GetLanguageDescByID(lang);
    if (!langDesc)
    {
        SendNotification(LANG_UNKNOWN_LANGUAGE);
        return;
    }
    if (_player && !IsReplaying() && langDesc->skill_id != 0 && !_player->HasSkill(langDesc->skill_id))
    {
        SendNotification(LANG_NOT_LEARNED_LANGUAGE);
        return;
    }

    if (lang == LANG_ADDON)
    {
        // Disabled addon channel?
        if (!sWorld.getConfig(CONFIG_BOOL_ADDON_CHANNEL))
            return;
    }
    // LANG_ADDON should not be changed nor be affected by flood control
    else
    {
        // send in universal language if player in .gmon mode (ignore spell effects)
        if (_player && _player->IsGameMaster())
            lang = LANG_UNIVERSAL;
        else
        {
            // send in universal language in two side iteration allowed mode
            if (sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CHAT))
                lang = LANG_UNIVERSAL;
            else
            {
                switch (type)
                {
                    case CHAT_MSG_PARTY:
                    case CHAT_MSG_RAID:
                    case CHAT_MSG_RAID_LEADER:
                    case CHAT_MSG_RAID_WARNING:
                        // allow two side chat at group channel if two side group allowed
                        if (sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_GROUP))
                            lang = LANG_UNIVERSAL;
                        break;
                    case CHAT_MSG_GUILD:
                    case CHAT_MSG_OFFICER:
                        // allow two side chat at guild channel if two side guild allowed
                        if (sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_GUILD))
                            lang = LANG_UNIVERSAL;
                        break;
                }
            }

            // but overwrite it by SPELL_AURA_MOD_LANGUAGE auras (only single case used)
            if (_player)
            {
                Unit::AuraList const& ModLangAuras = _player->GetAurasByType(SPELL_AURA_MOD_LANGUAGE);
                if (!ModLangAuras.empty())
                    lang = ModLangAuras.front()->GetModifier()->m_miscvalue;
            }
        }

        if (type != CHAT_MSG_AFK && type != CHAT_MSG_DND)
        {
            if (type != CHAT_MSG_WHISPER) // whisper checked later
            {
                auto currTime = time(nullptr);

                if (m_muteTime > currTime) // Muted
                {
                    std::string timeStr = secsToTimeString(m_muteTime - currTime);
                    SendNotification(GetMangosString(LANG_WAIT_BEFORE_SPEAKING), timeStr.c_str());
                    return;
                }
            }
            
            if (lang != LANG_ADDON && GetMasterPlayer())
                GetMasterPlayer()->UpdateSpeakTime(); // Anti chat flood
        }
    }

    std::string msg, channel, to;
    // Message parsing
    switch (type)
    {
        case CHAT_MSG_CHANNEL:
            recv_data >> channel;
            recv_data >> msg;

            if (!ProcessChatMessageAfterSecurityCheck(msg, lang, type))
                return;

            if (msg.empty())
                return;
            break;
        case CHAT_MSG_WHISPER:
            recv_data >> to;
            // no break
        case CHAT_MSG_SAY:
        case CHAT_MSG_EMOTE:
        case CHAT_MSG_YELL:
        case CHAT_MSG_PARTY:
        case CHAT_MSG_GUILD:
        case CHAT_MSG_OFFICER:
        case CHAT_MSG_RAID:
        case CHAT_MSG_RAID_LEADER:
        case CHAT_MSG_RAID_WARNING:
        case CHAT_MSG_BATTLEGROUND:
        case CHAT_MSG_BATTLEGROUND_LEADER:
            recv_data >> msg;
            if (!ProcessChatMessageAfterSecurityCheck(msg, lang, type))
                return;
            if (msg.empty())
                return;
        break;

        case CHAT_MSG_AFK:
        case CHAT_MSG_DND:
            recv_data >> msg;
        break;

        default:
            sLog.outError("CHAT: unknown message type %u, lang: %u", type, lang);
            return;
    }

    /** Enable various spam chat detections */
    if (lang != LANG_ADDON)
    {
        if (AntispamInterface *a = sAnticheatLib->GetAntispam())
            if (a->isMuted(GetAccountId(), true, type))
                return;
    }

    // Project Legacy mixed-faction WSG scoreboard bridge.
    if (lang == LANG_ADDON &&
        type == CHAT_MSG_BATTLEGROUND &&
        msg == "PLBGSCORE\tREQUEST")
    {
        Player* requester = _player;
        BattleGround* bg = requester ? requester->GetBattleGround() : nullptr;

        if (!bg || bg->GetTypeID() != BATTLEGROUND_WS)
            return;

        auto SendScoreAddonMessage = [&](std::string const& text)
            {
                WorldPacket data;
                ChatHandler::BuildChatPacket(
                    data,
                    CHAT_MSG_BATTLEGROUND,
                    text.c_str(),
                    LANG_ADDON,
                    CHAT_TAG_NONE,
                    requester->GetObjectGuid(),
                    requester->GetName());

                requester->GetSession()->SendPacket(&data);
            };

        SendScoreAddonMessage("PLBGSCORE\tCLEAR");

        Map::PlayerList const& playerList = requester->GetMap()->GetPlayers();

        for (Map::PlayerList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
        {
            Player* member = itr->getSource();

            if (!member || member->GetBattleGround() != bg)
                continue;

            std::string message = "PLBGSCORE\tP:";
            message += member->GetName();
            message += ":";
            message += member->GetBGTeam() == ALLIANCE ? "A" : "H";

            SendScoreAddonMessage(message);
        }

        SendScoreAddonMessage("PLBGSCORE\tDONE");
        return;
    }

    // Project Legacy remote battleground queue bridge
    if (type == CHAT_MSG_WHISPER &&
        to == _player->GetName() &&
        msg.compare(0, 6, "PLBGQ:") == 0)
    {
        char* end = nullptr;
        uint32 mapId = uint32(strtoul(msg.c_str() + 6, &end, 10));

        if (!end || *end != '\0')
            return;

        BattleGroundTypeId bgTypeId = GetBattleGroundTypeIdByMapId(mapId);

        if (bgTypeId == BATTLEGROUND_TYPE_NONE)
            return;

        if (!_player->GetBGAccessByLevel(bgTypeId))
        {
            SendNotification(LANG_YOUR_BG_LEVEL_REQ_ERROR);
            return;
        }

        SendBattlegGroundList(_player->GetObjectGuid(), bgTypeId);
        return;
    }


    // Project Legacy Guild Bank addon bridge
 if (lang == LANG_ADDON && type == CHAT_MSG_GUILD)
 {
     const std::string prefix = "PL_GBANK";

     if (msg.compare(0, prefix.length(), prefix) == 0)
     {
         std::string command;

         if (msg.length() > prefix.length())
         {
             size_t pos = prefix.length();

             while (pos < msg.length() &&
                 (msg[pos] == '\t' || msg[pos] == ':' || msg[pos] == ' '))
             {
                 ++pos;
             }

             command = msg.substr(pos);
         }

         // ----------------------------------------
         // HELLO
         // ----------------------------------------
         if (command == "HELLO")
         {
             ChatHandler(GetPlayer()).SendSysMessage(
                 "PL_GBANK:HELLO_ACK"
             );

             return;
         }

         // ----------------------------------------
        // RANGE
        // ----------------------------------------
         if (command == "RANGE")
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             if (!player->FindNearestGameObject(
                 900700,
                 INTERACTION_DISTANCE))
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:CLOSE"
                 );
             }

             return;
         }

         // ----------------------------------------
         // OPEN
         // ----------------------------------------
         if (command == "OPEN")
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             // Guild Bank may only be opened while standing at a Guild Vault.
             if (!player->FindNearestGameObject(900700, INTERACTION_DISTANCE))
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:CLOSE"
                 );

                 return;
             }

             uint32 guildId = player->GetGuildId();

             if (!guildId)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_IN_GUILD"
                 );

                 return;
             }
QueryResult* bankResult = CharacterDatabase.PQuery(
                 "SELECT money,purchased_tabs "
                 "FROM guild_bank "
                 "WHERE guildid='%u'",
                 guildId
             );

             if (!bankResult)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BANK_NOT_PURCHASED"
                 );

                 return;
             }

             Field* bankFields = bankResult->Fetch();

             uint64 money = bankFields[0].GetUInt64();
             uint32 purchasedTabs = bankFields[1].GetUInt32();
             uint32 rankId = player->GetRank();

             delete bankResult;

             

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:OPEN:%u:" UI64FMTD ":%u",
                 purchasedTabs,
                 money,
                 rankId
             );

             QueryResult* tabResult = CharacterDatabase.PQuery(
                 "SELECT tabid,name "
                 "FROM guild_bank_tab "
                 "WHERE guildid='%u' "
                 "ORDER BY tabid",
                 guildId
             );

             if (tabResult)
             {
                 do
                 {
                     Field* fields = tabResult->Fetch();

                     ChatHandler(player).PSendSysMessage(
                         "PL_GBANK:TAB:%u:%s",
                         fields[0].GetUInt32(),
                         fields[1].GetString()
                     );

                 } while (tabResult->NextRow());

                 delete tabResult;
             }

             ChatHandler(player).SendSysMessage(
                 "PL_GBANK:OPEN_DONE"
             );

             return;
         }

         // ----------------------------------------
         // TAB <id>
         // ----------------------------------------
         if (command.compare(0, 4, "TAB ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 guildId = player->GetGuildId();

             if (!guildId)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_IN_GUILD"
                 );

                 return;
             }

             uint32 tabId = 0;

             if (sscanf(
                 command.c_str(),
                 "TAB %u",
                 &tabId
             ) != 1)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_TAB_REQUEST"
                 );

                 return;
             }

             QueryResult* bankResult = CharacterDatabase.PQuery(
                 "SELECT purchased_tabs "
                 "FROM guild_bank "
                 "WHERE guildid='%u'",
                 guildId
             );

             if (!bankResult)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BANK_LOAD_FAILED"
                 );

                 return;
             }

             uint32 purchasedTabs =
                 bankResult->Fetch()[0].GetUInt32();

             delete bankResult;

             if (tabId >= purchasedTabs)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:TAB_NOT_PURCHASED"
                 );

                 return;
             }

             uint32 rankId = player->GetRank();

             if (rankId != GR_GUILDMASTER)
             {
                 QueryResult* permResult =
                     CharacterDatabase.PQuery(
                         "SELECT can_view "
                         "FROM guild_bank_right "
                         "WHERE guildid='%u' "
                         "AND rankid='%u' "
                         "AND tabid='%u'",
                         guildId,
                         rankId,
                         tabId
                     );

                 if (!permResult)
                 {
                     ChatHandler(player).SendSysMessage(
                         "PL_GBANK:ERROR:NO_VIEW_PERMISSION"
                     );

                     return;
                 }

                 bool canView =
                     permResult->Fetch()[0].GetUInt8() != 0;

                 delete permResult;

                 if (!canView)
                 {
                     ChatHandler(player).SendSysMessage(
                         "PL_GBANK:ERROR:NO_VIEW_PERMISSION"
                     );

                     return;
                 }
             }

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:TAB_BEGIN:%u",
                 tabId
             );

             QueryResult* itemResult =
                 CharacterDatabase.PQuery(
                     "SELECT "
                     "gbi.slot, "
                     "gbi.item_guid, "
                     "ii.itemEntry, "
                     "ii.count, "
                     "ii.randomPropertyId, "
                     "ii.enchantments "
                     "FROM guild_bank_item gbi "
                     "JOIN item_instance ii ON ii.guid = gbi.item_guid "
                     "WHERE gbi.guildid='%u' "
                     "AND gbi.tabid='%u' "
                     "ORDER BY gbi.slot",
                     guildId,
                     tabId
                 );

             if (itemResult)
             {
                 do
                 {
                     Field* fields =
                         itemResult->Fetch();

                     uint32 itemEntry =
                         fields[2].GetUInt32();

                     ItemPrototype const* proto =
                         ObjectMgr::GetItemPrototype(
                             itemEntry
                         );

                     const char* iconName = "";

                     if (proto)
                     {
                         ItemDisplayInfoEntry const* displayInfo =
                             sItemDisplayInfoStore.LookupEntry(
                                 proto->DisplayInfoID
                             );

                         if (displayInfo &&
                             displayInfo->inventoryIcon)
                         {
                             iconName =
                                 displayInfo->inventoryIcon;
                         }
                     }
                     int32 randomPropertyId =
                         fields[4].GetInt16();

                     const char* suffixName = "";

                     uint32 statTypes[3] =
                     {
                         0,
                         0,
                         0
                     };

                     uint32 statAmounts[3] =
                     {
                         0,
                         0,
                         0
                     };

                     if (randomPropertyId > 0)
                     {
                         ItemRandomPropertiesEntry const*
                             randomProperty =
                             sItemRandomPropertiesStore.LookupEntry(
                                 uint32(randomPropertyId)
                             );

                         if (randomProperty &&
                             randomProperty->nameSuffix[0])
                         {
                             suffixName =
                                 randomProperty->nameSuffix[0];
                         }
                     }

                     Tokenizer enchantTokens(
                         fields[5].GetString(),
                         ' ',
                         64
                     );

                     uint32 statIndex = 0;

                     for (uint32 tokenIndex = 0;
                         tokenIndex + 2 < enchantTokens.size() &&
                         statIndex < 3;
                         tokenIndex += 3)
                     {
                         uint32 enchantmentId =
                             uint32(atoi(enchantTokens[tokenIndex]));

                         if (!enchantmentId)
                             continue;

                         SpellItemEnchantmentEntry const*
                             enchantment =
                             sSpellItemEnchantmentStore.LookupEntry(
                                 enchantmentId
                             );

                         if (!enchantment)
                             continue;

                         for (uint32 effectIndex = 0;
                             effectIndex < 3 &&
                             statIndex < 3;
                             ++effectIndex)
                         {

                             if (!enchantment->type[effectIndex])
                                 continue;

                             uint32 statType = 0;
                             uint32 statAmount = 0;
                             bool hasStat = false;

                             if (enchantment->type[effectIndex] ==
                                 ITEM_ENCHANTMENT_TYPE_STAT)
                             {
                                 statType =
                                     enchantment->spellid[effectIndex];

                                 statAmount =
                                     enchantment->amount[effectIndex];

                                 hasStat = true;
                             }
                             else if (
                                 enchantment->type[effectIndex] ==
                                 ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL
                                 )
                             {
                                 SpellEntry const* spell =
                                     sSpellMgr.GetSpellEntry(
                                         enchantment->spellid[effectIndex]
                                     );

                                 if (spell)
                                 {
                                     for (uint32 spellEffect = 0;
                                         spellEffect < 3;
                                         ++spellEffect)
                                     {
                                         if (
                                             spell->EffectApplyAuraName[
                                                 spellEffect
                                             ] != SPELL_AURA_MOD_STAT
                                             )
                                         {
                                             continue;
                                         }

                                         statType =
                                             spell->EffectMiscValue[
                                                 spellEffect
                                             ];

                                         statAmount =
                                             uint32(
                                                 spell->EffectBasePoints[
                                                     spellEffect
                                                 ] + 1
                                             );

                                         hasStat = true;
                                         break;
                                     }
                                 }
                             }

                             if (!hasStat || !statAmount)
                                 continue;

                             statTypes[statIndex] =
                                 statType;

                             statAmounts[statIndex] =
                                 statAmount;

                             ++statIndex;
                         }
                     }

                     ChatHandler(player).PSendSysMessage(
                         "PL_GBANK:ITEM:%u:%u:%u:%u:%u:%s:%i:%s:%u:%u:%u:%u:%u:%u",
                         tabId,
                         fields[0].GetUInt32(),
                         fields[1].GetUInt32(),
                         itemEntry,
                         fields[3].GetUInt32(),
                         iconName,
                         randomPropertyId,
                         suffixName,
                         statTypes[0],
                         statAmounts[0],
                         statTypes[1],
                         statAmounts[1],
                         statTypes[2],
                         statAmounts[2]
                     );

                 } while (itemResult->NextRow());

                 delete itemResult;
             }

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:TAB_DONE:%u",
                 tabId
             );

             return;
         }

         // ----------------------------------------
         // DEPOSIT <tab> <bag> <slot> <bankSlot>
         // ----------------------------------------
         if (command.compare(0, 8, "DEPOSIT ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 tabId = 0;
             uint32 bag = 0;
             uint32 slot = 0;
             uint32 bankSlot = 0;

             if (sscanf(
                 command.c_str(),
                 "DEPOSIT %u %u %u %u",
                 &tabId,
                 &bag,
                 &slot,
                 &bankSlot
             ) != 4)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_DEPOSIT_REQUEST"
                 );

                 return;
             }

             if (bankSlot >= 98)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_BANK_SLOT"
                 );

                 return;
             }

             char args[80];

             snprintf(
                 args,
                 sizeof(args),
                 "%u %u %u %u",
                 tabId,
                 bag,
                 slot,
                 bankSlot
             );

             ChatHandler handler(player);

             handler.HandleGuildBankDepositCommand(args);

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:REFRESH:%u",
                 tabId
             );

             return;
         }

         // ----------------------------------------
         // WITHDRAW <tab> <bankSlot>
         // ----------------------------------------
         if (command.compare(0, 9, "WITHDRAW ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 tabId = 0;
             uint32 bankSlot = 0;

             if (sscanf(
                 command.c_str(),
                 "WITHDRAW %u %u",
                 &tabId,
                 &bankSlot
             ) != 2)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_WITHDRAW_REQUEST"
                 );

                 return;
             }

             if (bankSlot >= 98)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_BANK_SLOT"
                 );

                 return;
             }

             char args[64];

             snprintf(
                 args,
                 sizeof(args),
                 "%u %u",
                 tabId,
                 bankSlot
             );

             ChatHandler handler(player);

             handler.HandleGuildBankWithdrawCommand(args);

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:REFRESH:%u",
                 tabId
             );

             return;
         }

         // ----------------------------------------
         // MOVE <tab> <fromSlot> <toSlot>
         // ----------------------------------------
         if (command.compare(0, 5, "MOVE ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 tabId = 0;
             uint32 fromSlot = 0;
             uint32 toSlot = 0;

             if (sscanf(
                 command.c_str(),
                 "MOVE %u %u %u",
                 &tabId,
                 &fromSlot,
                 &toSlot
             ) != 3)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_MOVE_REQUEST"
                 );

                 return;
             }

             if (fromSlot >= 98 ||
                 toSlot >= 98)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_BANK_SLOT"
                 );

                 return;
             }

             char args[64];

             snprintf(
                 args,
                 sizeof(args),
                 "%u %u %u",
                 tabId,
                 fromSlot,
                 toSlot
             );

             ChatHandler handler(player);

             handler.HandleGuildBankMoveCommand(args);

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:REFRESH:%u",
                 tabId
             );

             return;
         }

         // ----------------------------------------
         // DEPOSIT_MONEY <copper>
         // ----------------------------------------
         if (command.compare(0, 14, "DEPOSIT_MONEY ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             unsigned long long amount = 0;

             if (sscanf(
                 command.c_str(),
                 "DEPOSIT_MONEY %llu",
                 &amount
             ) != 1)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_DEPOSIT_MONEY_REQUEST"
                 );

                 return;
             }

             if (amount == 0 ||
                 amount > 2147483647ULL)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_MONEY_AMOUNT"
                 );

                 return;
             }

             char args[64];

             snprintf(
                 args,
                 sizeof(args),
                 "%llu",
                 amount
             );

             ChatHandler handler(player);

             handler.HandleGuildBankDepositMoneyCommand(args);

             ChatHandler(player).SendSysMessage(
                 "PL_GBANK:REFRESH_MONEY"
             );

             return;
         }

         // ----------------------------------------
         // WITHDRAW_MONEY <copper>
         // ----------------------------------------
         if (command.compare(0, 15, "WITHDRAW_MONEY ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             unsigned long long amount = 0;

             if (sscanf(
                 command.c_str(),
                 "WITHDRAW_MONEY %llu",
                 &amount
             ) != 1)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_WITHDRAW_MONEY_REQUEST"
                 );

                 return;
             }

             if (amount == 0 ||
                 amount > 2147483647ULL)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_MONEY_AMOUNT"
                 );

                 return;
             }

             char args[64];

             snprintf(
                 args,
                 sizeof(args),
                 "%llu",
                 amount
             );

             ChatHandler handler(player);

             handler.HandleGuildBankWithdrawMoneyCommand(args);

             ChatHandler(player).SendSysMessage(
                 "PL_GBANK:REFRESH_MONEY"
             );

             return;
         }

         // ----------------------------------------
         // LOG <tab>
         // Item deposits + withdrawals
         // ----------------------------------------
         if (command.compare(0, 4, "LOG ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 guildId =
                 player->GetGuildId();

             uint32 tabId = 0;

             if (!guildId)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_IN_GUILD"
                 );

                 return;
             }

             if (sscanf(
                 command.c_str(),
                 "LOG %u",
                 &tabId
             ) != 1)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_LOG_REQUEST"
                 );

                 return;
             }

             QueryResult* bankResult =
                 CharacterDatabase.PQuery(
                     "SELECT purchased_tabs "
                     "FROM guild_bank "
                     "WHERE guildid='%u'",
                     guildId
                 );

             if (!bankResult)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BANK_LOAD_FAILED"
                 );

                 return;
             }

             uint32 purchasedTabs =
                 bankResult->Fetch()[0].GetUInt32();

             delete bankResult;

             if (tabId >= purchasedTabs)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:TAB_NOT_PURCHASED"
                 );

                 return;
             }

             uint32 rankId =
                 player->GetRank();

             if (rankId != GR_GUILDMASTER)
             {
                 QueryResult* permResult =
                     CharacterDatabase.PQuery(
                         "SELECT can_view "
                         "FROM guild_bank_right "
                         "WHERE guildid='%u' "
                         "AND rankid='%u' "
                         "AND tabid='%u'",
                         guildId,
                         rankId,
                         tabId
                     );

                 if (!permResult)
                 {
                     ChatHandler(player).SendSysMessage(
                         "PL_GBANK:ERROR:NO_VIEW_PERMISSION"
                     );

                     return;
                 }

                 bool canView =
                     permResult->Fetch()[0].GetUInt8() != 0;

                 delete permResult;

                 if (!canView)
                 {
                     ChatHandler(player).SendSysMessage(
                         "PL_GBANK:ERROR:NO_VIEW_PERMISSION"
                     );

                     return;
                 }
             }

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:LOG_BEGIN:%u",
                 tabId
             );

             QueryResult* result =
                 CharacterDatabase.PQuery(
                     "SELECT "
                     "gbl.player_guid, "
                     "COALESCE(c.name,'Unknown'), "
                     "gbl.action, "
                     "gbl.slot, "
                     "gbl.item_entry, "
                     "gbl.item_count, "
                     "gbl.timestamp "
                     "FROM guild_bank_log gbl "
                     "LEFT JOIN characters c "
                     "ON c.guid = gbl.player_guid "
                     "WHERE gbl.guildid='%u' "
                     "AND gbl.tabid='%u' "
                     "AND gbl.action IN (1,2) "
                     "ORDER BY gbl.timestamp DESC "
                     "LIMIT 50",
                     guildId,
                     tabId
                 );

             if (result)
             {
                 do
                 {
                     Field* fields =
                         result->Fetch();

                     ChatHandler(player).PSendSysMessage(
                         "PL_GBANK:LOG_ENTRY:%u:%s:%u:%u:%u:%u:%u",
                         fields[0].GetUInt32(),
                         fields[1].GetString(),
                         fields[2].GetUInt32(),
                         fields[3].GetUInt32(),
                         fields[4].GetUInt32(),
                         fields[5].GetUInt32(),
                         fields[6].GetUInt32()
                     );

                 } while (result->NextRow());

                 delete result;
             }

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:LOG_DONE:%u",
                 tabId
             );

             return;
         }

         // ----------------------------------------
         // MONEY_LOG
         // ----------------------------------------
         if (command == "MONEY_LOG")
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 guildId =
                 player->GetGuildId();

             if (!guildId)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_IN_GUILD"
                 );

                 return;
             }

             ChatHandler(player).SendSysMessage(
                 "PL_GBANK:MONEY_LOG_BEGIN"
             );

             QueryResult* result =
                 CharacterDatabase.PQuery(
                     "SELECT "
                     "gbl.player_guid, "
                     "COALESCE(c.name,'Unknown'), "
                     "gbl.action, "
                     "gbl.money, "
                     "gbl.timestamp "
                     "FROM guild_bank_log gbl "
                     "LEFT JOIN characters c "
                     "ON c.guid = gbl.player_guid "
                     "WHERE gbl.guildid='%u' "
                     "AND gbl.action IN (3,4,5) "
                     "ORDER BY gbl.timestamp DESC "
                     "LIMIT 50",
                     guildId
                 );

             if (result)
             {
                 do
                 {
                     Field* fields =
                         result->Fetch();

                     ChatHandler(player).PSendSysMessage(
                         "PL_GBANK:MONEY_LOG_ENTRY:%u:%s:%u:"
                         UI64FMTD ":%u",
                         fields[0].GetUInt32(),
                         fields[1].GetString(),
                         fields[2].GetUInt32(),
                         fields[3].GetUInt64(),
                         fields[4].GetUInt32()
                     );

                 } while (result->NextRow());

                 delete result;
             }

             ChatHandler(player).SendSysMessage(
                 "PL_GBANK:MONEY_LOG_DONE"
             );

             return;
         }

         // ----------------------------------------
         // PURCHASE
         // Initial guild bank purchase
         // ----------------------------------------
         if (command == "PURCHASE")
        {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 guildId = player->GetGuildId();

             if (!guildId)
            {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_IN_GUILD"
                );

                 return;
            }

             if (player->GetRank() != 0)
            {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_GUILD_MASTER"
                );

                 return;
            }

             // Never charge a guild twice.
             QueryResult* existingBank =
                 CharacterDatabase.PQuery(
                     "SELECT 1 FROM guild_bank "
                     "WHERE guildid='%u'",
                     guildId
                );

             if (existingBank)
            {
                 delete existingBank;

                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:OPEN_TRIGGER"
                );

                 return;
            }

             // Initial Guild Bank + Tab 1 price: 25 gold.
             const uint32 purchasePrice =
                 25 * 10000;

             if (player->GetMoney() < purchasePrice)
            {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_ENOUGH_MONEY"
                );

                 return;
            }

             CharacterDatabase.BeginTransaction(
                 player->GetGUIDLow()
            );

             player->ModifyMoney(
                 -int32(purchasePrice)
            );

             CharacterDatabase.PExecute(
                 "INSERT INTO guild_bank "
                 "(guildid,money,purchased_tabs) "
                 "VALUES ('%u','0','1')",
                 guildId
            );

             CharacterDatabase.PExecute(
                 "INSERT INTO guild_bank_tab "
                 "(guildid,tabid,name,icon) "
                 "VALUES ('%u','0','Tab 1','')",
                 guildId
            );

             player->SaveInventoryAndGoldToDB();

             CharacterDatabase.CommitTransaction();

             ChatHandler(player).SendSysMessage(
                 "PL_GBANK:PURCHASED"
            );

             ChatHandler(player).SendSysMessage(
                 "PL_GBANK:OPEN_TRIGGER"
            );

             return;
        }

         // ----------------------------------------
         // ADDTAB
         // ----------------------------------------
         if (command == "ADDTAB")
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             ChatHandler handler(player);

             handler.HandleGuildBankAddTabCommand(nullptr);

             // Reload tabs + bank balance after purchase.
             ChatHandler(player).SendSysMessage(
                 "PL_GBANK:REFRESH_MONEY"
             );

             return;
         }

         // ----------------------------------------
         // INFO <tab>
         // ----------------------------------------
         if (command.compare(0, 5, "INFO ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 guildId =
                 player->GetGuildId();

             uint32 rankId =
                 player->GetRank();

             uint32 tabId = 0;

             if (!guildId)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_IN_GUILD"
                 );

                 return;
             }

             if (sscanf(
                 command.c_str(),
                 "INFO %u",
                 &tabId
             ) != 1)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_INFO_REQUEST"
                 );

                 return;
             }

             QueryResult* bankResult =
                 CharacterDatabase.PQuery(
                     "SELECT money,purchased_tabs "
                     "FROM guild_bank "
                     "WHERE guildid='%u'",
                     guildId
                 );

             if (!bankResult)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BANK_LOAD_FAILED"
                 );

                 return;
             }

             Field* bankFields =
                 bankResult->Fetch();

             uint64 money =
                 bankFields[0].GetUInt64();

             uint32 purchasedTabs =
                 bankFields[1].GetUInt32();

             delete bankResult;

             if (tabId >= purchasedTabs)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:TAB_NOT_PURCHASED"
                 );

                 return;
             }

             uint32 canView = 0;
             uint32 canDeposit = 0;
             uint32 canWithdraw = 0;
             uint32 dailyLimit = 0;

             uint32 canDepositMoney = 0;
             uint32 canWithdrawMoney = 0;
             uint64 dailyMoneyLimit = 0;

             // Guild Master always has full rights.
             if (rankId == GR_GUILDMASTER)
             {
                 canView = 1;
                 canDeposit = 1;
                 canWithdraw = 1;

                 canDepositMoney = 1;
                 canWithdrawMoney = 1;
             }
             else
             {
                 QueryResult* tabPermResult =
                     CharacterDatabase.PQuery(
                         "SELECT "
                         "can_view, "
                         "can_deposit, "
                         "can_withdraw, "
                         "daily_withdraw_limit "
                         "FROM guild_bank_right "
                         "WHERE guildid='%u' "
                         "AND rankid='%u' "
                         "AND tabid='%u'",
                         guildId,
                         rankId,
                         tabId
                     );

                 if (tabPermResult)
                 {
                     Field* fields =
                         tabPermResult->Fetch();

                     canView =
                         fields[0].GetUInt32();

                     canDeposit =
                         fields[1].GetUInt32();

                     canWithdraw =
                         fields[2].GetUInt32();

                     dailyLimit =
                         fields[3].GetUInt32();

                     delete tabPermResult;
                 }

                 QueryResult* moneyPermResult =
                     CharacterDatabase.PQuery(
                         "SELECT "
                         "can_deposit_money, "
                         "can_withdraw_money, "
                         "daily_money_limit "
                         "FROM guild_bank_money_right "
                         "WHERE guildid='%u' "
                         "AND rankid='%u'",
                         guildId,
                         rankId
                     );

                 if (moneyPermResult)
                 {
                     Field* fields =
                         moneyPermResult->Fetch();

                     canDepositMoney =
                         fields[0].GetUInt32();

                     canWithdrawMoney =
                         fields[1].GetUInt32();

                     dailyMoneyLimit =
                         fields[2].GetUInt64();

                     delete moneyPermResult;
                 }
             }

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:INFO:%u:%u:%u:%u:%u:%u:%u:%u:"
                 UI64FMTD ":" UI64FMTD,
                 tabId,
                 rankId,
                 canView,
                 canDeposit,
                 canWithdraw,
                 dailyLimit,
                 canDepositMoney,
                 canWithdrawMoney,
                 dailyMoneyLimit,
                 money
             );

             return;
         }

         // ----------------------------------------
         // GET_PERMS <rankId>
         // Guild Master only
         // ----------------------------------------
         if (command.compare(0, 10, "GET_PERMS ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 guildId = player->GetGuildId();
             uint32 requestedRankId = 0;

             if (!guildId)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_IN_GUILD"
                 );

                 return;
             }

             // Only the Guild Master may inspect arbitrary
             // guild-bank rank permissions.
             if (player->GetRank() != 0)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_GUILD_MASTER"
                 );

                 return;
             }

             if (sscanf(
                 command.c_str(),
                 "GET_PERMS %u",
                 &requestedRankId
             ) != 1)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_PERMS_REQUEST"
                 );

                 return;
             }

             // Vanilla supports at most 10 guild ranks.
             if (requestedRankId >= 10)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_RANK"
                 );

                 return;
             }
QueryResult* bankResult =
                 CharacterDatabase.PQuery(
                     "SELECT purchased_tabs "
                     "FROM guild_bank "
                     "WHERE guildid='%u'",
                     guildId
                 );

             if (!bankResult)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BANK_NOT_PURCHASED"
                 );

                 return;
             }

             uint32 purchasedTabs =
                 bankResult->Fetch()[0].GetUInt32();

             delete bankResult;

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:PERMS_BEGIN:%u:%u",
                 requestedRankId,
                 purchasedTabs
             );

             // ------------------------------------
             // TAB PERMISSIONS
             // ------------------------------------
             for (uint32 tabId = 0;
                 tabId < purchasedTabs;
                 ++tabId)
             {
                 uint32 canView = 0;
                 uint32 canDeposit = 0;
                 uint32 canWithdraw = 0;
                 uint32 dailyLimit = 0;

                 // Guild Master always has full access.
                 if (requestedRankId == GR_GUILDMASTER)
                 {
                     canView = 1;
                     canDeposit = 1;
                     canWithdraw = 1;
                     dailyLimit = 0;
                 }
                 else
                 {
                     QueryResult* tabPermResult =
                         CharacterDatabase.PQuery(
                             "SELECT "
                             "can_view, "
                             "can_deposit, "
                             "can_withdraw, "
                             "daily_withdraw_limit "
                             "FROM guild_bank_right "
                             "WHERE guildid='%u' "
                             "AND rankid='%u' "
                             "AND tabid='%u'",
                             guildId,
                             requestedRankId,
                             tabId
                         );

                     if (tabPermResult)
                     {
                         Field* fields =
                             tabPermResult->Fetch();

                         canView =
                             fields[0].GetUInt32();

                         canDeposit =
                             fields[1].GetUInt32();

                         canWithdraw =
                             fields[2].GetUInt32();

                         dailyLimit =
                             fields[3].GetUInt32();

                         delete tabPermResult;
                     }
                 }

                 ChatHandler(player).PSendSysMessage(
                     "PL_GBANK:PERMS_TAB:%u:%u:%u:%u:%u:%u",
                     requestedRankId,
                     tabId,
                     canView,
                     canDeposit,
                     canWithdraw,
                     dailyLimit
                 );
             }

             // ------------------------------------
             // MONEY PERMISSIONS
             // ------------------------------------
             uint32 canDepositMoney = 0;
             uint32 canWithdrawMoney = 0;
             uint64 dailyMoneyLimit = 0;

             if (requestedRankId == GR_GUILDMASTER)
             {
                 canDepositMoney = 1;
                 canWithdrawMoney = 1;
                 dailyMoneyLimit = 0;
             }
             else
             {
                 QueryResult* moneyPermResult =
                     CharacterDatabase.PQuery(
                         "SELECT "
                         "can_deposit_money, "
                         "can_withdraw_money, "
                         "daily_money_limit "
                         "FROM guild_bank_money_right "
                         "WHERE guildid='%u' "
                         "AND rankid='%u'",
                         guildId,
                         requestedRankId
                     );

                 if (moneyPermResult)
                 {
                     Field* fields =
                         moneyPermResult->Fetch();

                     canDepositMoney =
                         fields[0].GetUInt32();

                     canWithdrawMoney =
                         fields[1].GetUInt32();

                     dailyMoneyLimit =
                         fields[2].GetUInt64();

                     delete moneyPermResult;
                 }
             }

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:PERMS_MONEY:%u:%u:%u:"
                 UI64FMTD,
                 requestedRankId,
                 canDepositMoney,
                 canWithdrawMoney,
                 dailyMoneyLimit
             );

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:PERMS_DONE:%u",
                 requestedRankId
             );

             return;
         }

         // ----------------------------------------
         // SET_TAB_PERMS
         // <rankId> <tabId> <view> <deposit>
         // <withdraw> <dailyLimit>
         // Guild Master only
         // ----------------------------------------
         if (command.compare(0, 14, "SET_TAB_PERMS ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 guildId = player->GetGuildId();

             if (!guildId)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_IN_GUILD"
                 );

                 return;
             }

             if (player->GetRank() != 0)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_GUILD_MASTER"
                 );

                 return;
             }

             uint32 requestedRankId = 0;
             uint32 tabId = 0;
             uint32 canView = 0;
             uint32 canDeposit = 0;
             uint32 canWithdraw = 0;
             uint32 dailyLimit = 0;

             if (sscanf(
                 command.c_str(),
                 "SET_TAB_PERMS %u %u %u %u %u %u",
                 &requestedRankId,
                 &tabId,
                 &canView,
                 &canDeposit,
                 &canWithdraw,
                 &dailyLimit
             ) != 6)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_SET_TAB_PERMS"
                 );

                 return;
             }

             if (requestedRankId >= 10)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_RANK"
                 );

                 return;
             }

             // Guild Master rights are hard-coded to full access
             // and cannot be edited.
             if (requestedRankId == GR_GUILDMASTER)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:CANNOT_EDIT_GUILD_MASTER"
                 );

                 return;
             }
QueryResult* bankResult =
                 CharacterDatabase.PQuery(
                     "SELECT purchased_tabs "
                     "FROM guild_bank "
                     "WHERE guildid='%u'",
                     guildId
                 );

             if (!bankResult)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BANK_NOT_PURCHASED"
                 );

                 return;
             }

             uint32 purchasedTabs =
                 bankResult->Fetch()[0].GetUInt32();

             delete bankResult;

             if (tabId >= purchasedTabs)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:TAB_NOT_PURCHASED"
                 );

                 return;
             }

             canView = canView ? 1 : 0;
             canDeposit = canDeposit ? 1 : 0;
             canWithdraw = canWithdraw ? 1 : 0;

             CharacterDatabase.PExecute(
                 "INSERT INTO guild_bank_right "
                 "(guildid,rankid,tabid,"
                 "can_view,can_deposit,can_withdraw,"
                 "daily_withdraw_limit) "
                 "VALUES ('%u','%u','%u','%u','%u','%u','%u') "
                 "ON DUPLICATE KEY UPDATE "
                 "can_view=VALUES(can_view), "
                 "can_deposit=VALUES(can_deposit), "
                 "can_withdraw=VALUES(can_withdraw), "
                 "daily_withdraw_limit=VALUES(daily_withdraw_limit)",
                 guildId,
                 requestedRankId,
                 tabId,
                 canView,
                 canDeposit,
                 canWithdraw,
                 dailyLimit
             );

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:PERMS_TAB:%u:%u:%u:%u:%u:%u",
                 requestedRankId,
                 tabId,
                 canView,
                 canDeposit,
                 canWithdraw,
                 dailyLimit
             );

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:PERMS_SAVED:TAB:%u:%u",
                 requestedRankId,
                 tabId
             );

             return;
         }

         // ----------------------------------------
         // SET_MONEY_PERMS
         // <rankId> <deposit> <withdraw>
         // <dailyMoneyLimitCopper>
         // Guild Master only
         // ----------------------------------------
         if (command.compare(0, 16, "SET_MONEY_PERMS ") == 0)
         {
             Player* player = GetPlayer();

             if (!player)
                 return;

             uint32 guildId = player->GetGuildId();

             if (!guildId)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_IN_GUILD"
                 );

                 return;
             }

             if (player->GetRank() != 0)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:NOT_GUILD_MASTER"
                 );

                 return;
             }

             uint32 requestedRankId = 0;
             uint32 canDepositMoney = 0;
             uint32 canWithdrawMoney = 0;
             unsigned long long parsedDailyMoneyLimit = 0;

             if (sscanf(
                 command.c_str(),
                 "SET_MONEY_PERMS %u %u %u %llu",
                 &requestedRankId,
                 &canDepositMoney,
                 &canWithdrawMoney,
                 &parsedDailyMoneyLimit
             ) != 4)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_SET_MONEY_PERMS"
                 );

                 return;
             }

             if (requestedRankId >= 10)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:BAD_RANK"
                 );

                 return;
             }

             if (requestedRankId == GR_GUILDMASTER)
             {
                 ChatHandler(player).SendSysMessage(
                     "PL_GBANK:ERROR:CANNOT_EDIT_GUILD_MASTER"
                 );

                 return;
             }

             canDepositMoney =
                 canDepositMoney ? 1 : 0;

             canWithdrawMoney =
                 canWithdrawMoney ? 1 : 0;

             uint64 dailyMoneyLimit =
                 static_cast<uint64>(
                     parsedDailyMoneyLimit
                     );

             CharacterDatabase.PExecute(
                 "INSERT INTO guild_bank_money_right "
                 "(guildid,rankid,"
                 "can_deposit_money,can_withdraw_money,"
                 "daily_money_limit) "
                 "VALUES ('%u','%u','%u','%u','" UI64FMTD "') "
                 "ON DUPLICATE KEY UPDATE "
                 "can_deposit_money=VALUES(can_deposit_money), "
                 "can_withdraw_money=VALUES(can_withdraw_money), "
                 "daily_money_limit=VALUES(daily_money_limit)",
                 guildId,
                 requestedRankId,
                 canDepositMoney,
                 canWithdrawMoney,
                 dailyMoneyLimit
             );

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:PERMS_MONEY:%u:%u:%u:"
                 UI64FMTD,
                 requestedRankId,
                 canDepositMoney,
                 canWithdrawMoney,
                 dailyMoneyLimit
             );

             ChatHandler(player).PSendSysMessage(
                 "PL_GBANK:PERMS_SAVED:MONEY:%u",
                 requestedRankId
             );

             return;
         }

         // Never allow PL_GBANK protocol traffic
         // to fall through into actual guild chat.
         return;
     }
 }
    // Message handling
    switch (type)
    {
        case CHAT_MSG_CHANNEL:
        {
            PlayerPointer playerPointer(GetPlayerPointer());
            ASSERT(playerPointer);
            if (ChannelMgr* cMgr = channelMgr(playerPointer->GetTeam()))
            {
                if (Channel *chn = cMgr->GetChannel(channel, playerPointer, IsMaster()))
                {
                    // Level channels restrictions
                    if (chn->IsLevelRestricted() && playerPointer->GetLevel() < sWorld.getConfig(CONFIG_UINT32_WORLD_CHAN_MIN_LEVEL)
                        && GetAccountMaxLevel() < sWorld.getConfig(CONFIG_UINT32_PUB_CHANS_MUTE_VANISH_LEVEL))
                    {
                        ChatHandler(this).SendSysMessage("You cannot use this channel yet.");
                        return;
                    }

                    // Public channels restrictions
                    if (!chn->HasFlag(Channel::CHANNEL_FLAG_CUSTOM))
                    {
                       

                        // Check strict Latin for general chat channels
                        if (sWorld.getConfig(CONFIG_BOOL_STRICT_LATIN_IN_GENERAL_CHANNELS))
                        {
                            // remove color, punct, ctrl, space
                            if (AntispamInterface *a = sAnticheatLib->GetAntispam())
                            {
                                std::string normMsg = a->normalizeMessage(msg, 0x1D);
                                std::wstring w_normMsg;
                                if (Utf8toWStr(normMsg, w_normMsg))
                                {
                                    if (!isBasicLatinString(w_normMsg, true))
                                    {
                                        ChatHandler(this).SendSysMessage("Sorry, only Latin characters are allowed in this channel.");
                                        return;
                                    }
                                }
                            }
                        }

                        if (auto cooldown = ChatCooldown())
                        {
                            ChatHandler(this).PSendSysMessage(
                                "Please wait %u seconds before sending another message.", cooldown
                            );
                            return;
                        }
                    }

                    chn->Say(playerPointer->GetObjectGuid(), msg.c_str(), lang);
                    SetLastPubChanMsgTime(time(nullptr));

                    if (lang != LANG_ADDON && chn->HasFlag(Channel::ChannelFlags::CHANNEL_FLAG_GENERAL))
                        if (AntispamInterface *a = sAnticheatLib->GetAntispam())
                            a->addMessage(msg, type, GetPlayerPointer(), nullptr);
                }
                else // If it is not a global channel, forward to Node
                    ForwardPacketToMaster();
            }

            if (lang != LANG_ADDON)
            {
                normalizePlayerName(channel);
                sWorld.LogChat(this, "Chan", msg, nullptr, 0, channel.c_str());
            }
        }
        break;

        case CHAT_MSG_SAY:
            if (GetPlayer()->GetLevel() < sWorld.getConfig(CONFIG_UINT32_SAY_MIN_LEVEL)
                && GetAccountMaxLevel() < sWorld.getConfig(CONFIG_UINT32_PUB_CHANS_MUTE_VANISH_LEVEL))
            {
                ChatHandler(this).SendSysMessage("You cannot speak yet (too low level).");
                return;
            }

            if (!GetPlayer()->IsAlive())
                return;

            GetPlayer()->Say(msg, lang);

            if (lang != LANG_ADDON)
            {
                sWorld.LogChat(this, "Say", msg);

                if (AntispamInterface *a = sAnticheatLib->GetAntispam())
                    a->addMessage(msg, type, GetPlayerPointer(), nullptr);
            }

            break;
        case CHAT_MSG_EMOTE:
            if (GetPlayer()->GetLevel() < sWorld.getConfig(CONFIG_UINT32_SAY_EMOTE_MIN_LEVEL)
                && GetAccountMaxLevel() < sWorld.getConfig(CONFIG_UINT32_PUB_CHANS_MUTE_VANISH_LEVEL))
            {
                ChatHandler(this).SendSysMessage("You cannot use emotes yet (too low level).");
                return;
            }

            if (!GetPlayer()->IsAlive())
                return;

            GetPlayer()->TextEmote(msg);

            if (lang != LANG_ADDON)
            {
                sWorld.LogChat(this, "Emote", msg);

                if (AntispamInterface *a = sAnticheatLib->GetAntispam())
                    a->addMessage(msg, type, GetPlayerPointer(), nullptr);
            }

            break;
        case CHAT_MSG_YELL:
        {
            if (GetPlayer()->GetLevel() < sWorld.getConfig(CONFIG_UINT32_YELL_MIN_LEVEL)
                && GetAccountMaxLevel() < sWorld.getConfig(CONFIG_UINT32_PUB_CHANS_MUTE_VANISH_LEVEL))
            {
                ChatHandler(this).SendSysMessage("You cannot yell yet (too low level).");
                return;
            }

            if (!GetPlayer()->IsAlive())
                return;

            GetPlayer()->Yell(msg, lang);

            if (lang != LANG_ADDON)
            {
                sWorld.LogChat(this, "Yell", msg);

                if (AntispamInterface *a = sAnticheatLib->GetAntispam())
                    a->addMessage(msg, type, GetPlayerPointer(), nullptr);
            }
        }
        break;

        case CHAT_MSG_WHISPER: // Master Side
        {
            ForwardPacketToMaster();
            if (!normalizePlayerName(to))
            {
                SendPlayerNotFoundNotice(to);
                break;
            }
            MasterPlayer* masterPlr = GetMasterPlayer();
            ASSERT(masterPlr);

            MasterPlayer *player = ObjectAccessor::FindMasterPlayer(to.c_str());
            uint32 tSecurity = GetSecurity();
            uint32 pSecurity = player ? player->GetSession()->GetSecurity() : SEC_PLAYER;
            if (!player || (tSecurity == SEC_PLAYER && pSecurity > SEC_PLAYER && !player->AcceptsWhispersFrom(masterPlr->GetObjectGuid())))
            {
                SendPlayerNotFoundNotice(to);
                return;
            }

            // Can only whisper GMs while muted.
            if (pSecurity == SEC_PLAYER)
            {
                auto currTime = time(nullptr);

                if (m_muteTime > currTime) // Muted
                {
                    std::string timeStr = secsToTimeString(m_muteTime - currTime);
                    SendNotification(GetMangosString(LANG_WAIT_BEFORE_SPEAKING), timeStr.c_str());
                    return;
                }
            }

            if (tSecurity == SEC_PLAYER && pSecurity == SEC_PLAYER)
            {
                if (!sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CHAT) && GetPlayer()->GetTeam() != player->GetTeam())
                {
                    SendWrongFactionNotice();
                    return;
                }
                if (/*player->GetZoneId() != masterPlr->GetZoneId() && */masterPlr->GetLevel() < sWorld.getConfig(CONFIG_UINT32_WHISP_DIFF_ZONE_MIN_LEVEL))
                {
                    ChatHandler(this).SendSysMessage("You cannot whisper yet.");
                    return;
                }
            }

            if (Player* toPlayer = player->GetSession()->GetPlayer())
            {
                bool allowIgnoreAntispam = toPlayer->IsAllowedWhisperFrom(masterPlr->GetObjectGuid());
                bool allowSendWhisper = allowIgnoreAntispam;
                if (!sWorld.getConfig(CONFIG_BOOL_WHISPER_RESTRICTION) || !toPlayer->IsEnabledWhisperRestriction())
                    allowSendWhisper = true;

                if (masterPlr->IsGameMaster() || allowSendWhisper)
                    masterPlr->Whisper(msg, lang, player);

                if (lang != LANG_ADDON)
                {
                    sWorld.LogChat(this, "Whisp", msg, PlayerPointer(new PlayerWrapper<MasterPlayer>(player)));

                    if (!allowIgnoreAntispam)
                        if (AntispamInterface *a = sAnticheatLib->GetAntispam())
                            a->addMessage(msg, type, GetPlayerPointer(), PlayerPointer(new PlayerWrapper<MasterPlayer>(player)));
                }
            }
        }
        break;

        case CHAT_MSG_PARTY: // Master Side: TODO
        {
            // if player is in battleground, he cannot say to battleground members by /p
            Group *group = GetPlayer()->GetOriginalGroup();
            if (!group)
            {
                group = _player->GetGroup();
                if (!group || group->isBGGroup())
                    return;
            }

            WorldPacket data;
            ChatHandler::BuildChatPacket(data, ChatMsg(type), msg.c_str(), Language(lang), _player->GetChatTag(), _player->GetObjectGuid(), _player->GetName());
            group->BroadcastPacket(&data, false, group->GetMemberGroup(GetPlayer()->GetObjectGuid()));
            if (lang != LANG_ADDON)
                sWorld.LogChat(this, "Group", msg, nullptr, group->GetId());
        }
        break;
        case CHAT_MSG_GUILD: // Master side
        {
            ForwardPacketToMaster();
            if (GetMasterPlayer()->GetGuildId())
                if (Guild* guild = sGuildMgr.GetGuildById(GetMasterPlayer()->GetGuildId()))
                    guild->BroadcastToGuild(this, msg, lang == LANG_ADDON ? LANG_ADDON : LANG_UNIVERSAL);

            if (lang != LANG_ADDON)
                sWorld.LogChat(this, "Guild", msg, nullptr, GetMasterPlayer()->GetGuildId());
            break;
        }
        case CHAT_MSG_OFFICER: // Master side
        {
            ForwardPacketToMaster();
            if (GetMasterPlayer()->GetGuildId())
                if (Guild* guild = sGuildMgr.GetGuildById(GetMasterPlayer()->GetGuildId()))
                    guild->BroadcastToOfficers(this, msg, lang == LANG_ADDON ? LANG_ADDON : LANG_UNIVERSAL);

            if (lang != LANG_ADDON)
                sWorld.LogChat(this, "Officer", msg, nullptr, GetMasterPlayer()->GetGuildId());
            break;
        }
        case CHAT_MSG_RAID: // Master side: TODO
        {
            // if player is in battleground, he cannot say to battleground members by /ra
            Group *group = GetPlayer()->GetOriginalGroup();
            if (!group)
            {
                group = GetPlayer()->GetGroup();
                if (!group || group->isBGGroup() || !group->isRaidGroup())
                    return;
            }

            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_RAID, msg.c_str(), Language(lang), _player->GetChatTag(), _player->GetObjectGuid(), _player->GetName());
            group->BroadcastPacket(&data, false);

            if (lang != LANG_ADDON)
                sWorld.LogChat(this, "Raid", msg, nullptr, group->GetId());
        }
        break;
        case CHAT_MSG_RAID_LEADER: // Master side: TODO
        {
            // if player is in battleground, he cannot say to battleground members by /ra
            Group *group = GetPlayer()->GetOriginalGroup();
            if (!group)
            {
                group = GetPlayer()->GetGroup();
                if (!group || group->isBGGroup() || !group->isRaidGroup() || !group->IsLeader(_player->GetObjectGuid()))
                    return;
            }

            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_RAID_LEADER, msg.c_str(), Language(lang), _player->GetChatTag(), _player->GetObjectGuid(), _player->GetName());
            group->BroadcastPacket(&data, false);
            if (lang != LANG_ADDON)
                sWorld.LogChat(this, "Raid", msg, nullptr, group->GetId());
        }
        break;

        case CHAT_MSG_RAID_WARNING: // Master side: TODO
        {
            Group *group = GetPlayer()->GetGroup();
            if (!group || !group->isRaidGroup() ||
                    !(group->IsLeader(GetPlayer()->GetObjectGuid()) || group->IsAssistant(GetPlayer()->GetObjectGuid())))
                return;

            WorldPacket data;
            //in battleground, raid warning is sent only to players in battleground - code is ok
            ChatHandler::BuildChatPacket(data, CHAT_MSG_RAID_WARNING, msg.c_str(), Language(lang), _player->GetChatTag(), _player->GetObjectGuid(), _player->GetName());
            group->BroadcastPacket(&data, false);

            if (lang != LANG_ADDON)
                sWorld.LogChat(this, "Raid", msg, nullptr, group->GetId());
        }
        break;

        case CHAT_MSG_BATTLEGROUND: // Node side
        {
            ForwardPacketToNode();
            // battleground raid is always in Player->GetGroup(), never in GetOriginalGroup()
            Group *group = GetPlayer()->GetGroup();
            if (!group || !group->isBGGroup())
                return;

            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_BATTLEGROUND, msg.c_str(), Language(lang), _player->GetChatTag(), _player->GetObjectGuid(), _player->GetName());
            group->BroadcastPacket(&data, false);

            if (lang != LANG_ADDON)
                sWorld.LogChat(this, "BG", msg, nullptr, group->GetId());
        }
        break;

        case CHAT_MSG_BATTLEGROUND_LEADER: // Node side
        {
            ForwardPacketToNode();
            // battleground raid is always in Player->GetGroup(), never in GetOriginalGroup()
            Group *group = GetPlayer()->GetGroup();
            if (!group || !group->isBGGroup() || !group->IsLeader(GetPlayer()->GetObjectGuid()))
                return;

            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_BATTLEGROUND_LEADER, msg.c_str(), Language(lang), _player->GetChatTag(), _player->GetObjectGuid(), _player->GetName());
            group->BroadcastPacket(&data, false);

            if (lang != LANG_ADDON)
                sWorld.LogChat(this, "BG", msg, nullptr, group->GetId());
        }
        break;

        case CHAT_MSG_AFK: // Node side (for combat Check)
        {
            ForwardPacketToNode();

            if(_player && _player->IsInCombat())
                break;

            if(!msg.empty() || !_player->IsAFK())
            {
                if (MasterPlayer* masterPlr = GetMasterPlayer())
                    masterPlr->afkMsg = msg;
            }

            if(msg.empty() || !_player->IsAFK())
            {
                _player->ToggleAFK();

                if(_player->IsAFK() && _player->IsDND())
                    _player->ToggleDND();
            }
        }

        break;

        case CHAT_MSG_DND:
        {
            if(!msg.empty() || !_player->IsDND())
            {
                if (MasterPlayer* masterPlr = GetMasterPlayer())
                    masterPlr->dndMsg = msg;
            }

            if(msg.empty() || !_player->IsDND())
            {
                _player->ToggleDND();

                if(_player->IsDND() && _player->IsAFK())
                    _player->ToggleAFK();
            }
        }

        break;
    }
}

void WorldSession::HandleEmoteOpcode(WorldPacket & recv_data)
{
    if (!GetPlayer()->IsAlive() || GetPlayer()->HasUnitState(UNIT_STAT_DIED))
        return;

    if (!GetPlayer()->CanSpeak())
    {
        std::string timeStr = secsToTimeString(m_muteTime - time(nullptr));
        SendNotification(GetMangosString(LANG_WAIT_BEFORE_SPEAKING), timeStr.c_str());
        return;
    }

    uint32 emote;
    recv_data >> emote;

    // restrict to the only emotes hardcoded in client
    if (emote != EMOTE_ONESHOT_NONE && emote != EMOTE_ONESHOT_WAVE)
        return;

    GetPlayer()->HandleEmoteCommand(emote);
}

namespace MaNGOS
{
class EmoteChatBuilder
{
public:
    EmoteChatBuilder(Player const& pl, uint32 text_emote, uint32 emote_num, Unit const* target)
        : i_player(pl), i_text_emote(text_emote), i_emote_num(emote_num), i_target(target) {}

    void operator()(WorldPacket& data, int32 loc_idx)
    {
        char const* nam = i_target ? i_target->GetNameForLocaleIdx(loc_idx) : nullptr;
        uint32 namlen = (nam ? strlen(nam) : 0) + 1;

        data.Initialize(SMSG_TEXT_EMOTE, (20 + namlen));
        data << ObjectGuid(i_player.GetObjectGuid());
        data << uint32(i_text_emote);
        data << uint32(i_emote_num);
        data << uint32(namlen);
        if (namlen > 1)
            data.append(nam, namlen);
        else
            data << uint8(0x00);
    }

private:
    Player const& i_player;
    uint32        i_text_emote;
    uint32        i_emote_num;
    Unit const*   i_target;
};
}                                                           // namespace MaNGOS

void WorldSession::HandleTextEmoteOpcode(WorldPacket & recv_data)
{
    if (!GetPlayer()->IsAlive())
        return;

    if (!GetPlayer()->CanSpeak())
    {
        std::string timeStr = secsToTimeString(m_muteTime - time(nullptr));
        SendNotification(GetMangosString(LANG_WAIT_BEFORE_SPEAKING), timeStr.c_str());
        return;
    }

    uint32 text_emote, emoteNum;
    ObjectGuid guid;

    recv_data >> text_emote;
    recv_data >> emoteNum;
    recv_data >> guid;

    EmotesTextEntry const *em = sEmotesTextStore.LookupEntry(text_emote);
    if (!em)
        return;

    uint32 emote_id = em->textid;

    switch (emote_id)
    {
        case EMOTE_STATE_SLEEP:
        case EMOTE_STATE_SIT:
        case EMOTE_STATE_KNEEL:
        case EMOTE_ONESHOT_NONE:
            break;
        default:
        {
            // in feign death state allowed only text emotes.
            if (GetPlayer()->HasUnitState(UNIT_STAT_DIED))
                break;

            GetPlayer()->HandleEmoteCommand(emote_id);
            break;
        }
    }

    Unit* unit = GetPlayer()->GetMap()->GetUnit(guid);

    MaNGOS::EmoteChatBuilder emote_builder(*GetPlayer(), text_emote, emoteNum, unit);
    MaNGOS::LocalizedPacketDo<MaNGOS::EmoteChatBuilder > emote_do(emote_builder);
    MaNGOS::CameraDistWorker<MaNGOS::LocalizedPacketDo<MaNGOS::EmoteChatBuilder > > emote_worker(GetPlayer(), sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_TEXTEMOTE), emote_do);
    Cell::VisitWorldObjects(GetPlayer(), emote_worker,  sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_TEXTEMOTE));

    //Send scripted event call
    if (unit && unit->GetTypeId() == TYPEID_UNIT && ((Creature*)unit)->AI())
        ((Creature*)unit)->AI()->ReceiveEmote(GetPlayer(), text_emote);
}

void WorldSession::HandleChatIgnoredOpcode(WorldPacket& recv_data)
{
    ObjectGuid iguid;
    //DEBUG_LOG("WORLD: Received CMSG_CHAT_IGNORED");

    recv_data >> iguid;

    Player *player = sObjectMgr.GetPlayer(iguid);
    if (!player || !player->GetSession())
        return;

    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_IGNORED, _player->GetName(), LANG_UNIVERSAL, CHAT_TAG_NONE, _player->GetObjectGuid());
    player->GetSession()->SendPacket(&data);
}

void WorldSession::SendPlayerNotFoundNotice(std::string name)
{
    WorldPacket data(SMSG_CHAT_PLAYER_NOT_FOUND, name.size() + 1);
    data << name;
    SendPacket(&data);
}

void WorldSession::SendWrongFactionNotice()
{
    WorldPacket data(SMSG_CHAT_WRONG_FACTION, 0);
    SendPacket(&data);
}

void WorldSession::SendChatRestrictedNotice()
{
#if SUPPORTED_CLIENT_BUILD > CLIENT_BUILD_1_8_4
    WorldPacket data(SMSG_CHAT_RESTRICTED, 0);
    SendPacket(&data);
#endif
}
