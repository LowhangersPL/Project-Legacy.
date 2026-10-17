#include "TreasureTrailMgr.h"

#include "Creature.h"
#include "Database/DatabaseEnv.h"
#include "Item.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "World.h"

#include <vector>

TreasureTrailMgr sTreasureTrailMgr;

namespace
{
    enum TreasureTrailEventType
    {
        TREASURE_TRAIL_EVENT_STARTED = 1,
        TREASURE_TRAIL_EVENT_NEXT_CLUE = 2,
        TREASURE_TRAIL_EVENT_CASKET = 3,
        TREASURE_TRAIL_EVENT_STARTER_AWARDED = 4
    };

    uint8 GetTrailFaction(Player* player)
    {
        return player->GetTeam() == ALLIANCE ? 0 : 1;
    }

    uint8 GetTierForCreatureLevel(uint8 level)
    {
        if (level >= 10 && level <= 19)
            return 1;

        if (level >= 20 && level <= 29)
            return 2;

        if (level >= 30 && level <= 39)
            return 3;

        if (level >= 40 && level <= 49)
            return 4;

        if (level >= 50 && level <= 54)
            return 5;

        if (level >= 55 && level <= 60)
            return 6;

        return 0;
    }

    bool IsTailEndLevelForTier(uint8 level)
    {
        return level == 18 || level == 19 ||
            level == 28 || level == 29 ||
            level == 38 || level == 39 ||
            level == 48 || level == 49 ||
            level == 53 || level == 54;
    }

    bool IsTierOneNonFinalDungeonBoss(Creature* creature)
    {
        switch (creature->GetEntry())
        {
            // Deadmines
        case 644:   // Rhahk'Zor
        case 643:   // Sneed
        case 1763:  // Gilnid
        case 646:   // Mr. Smite

            // Ragefire Chasm
        case 11517: // Oggleflint
        case 11518: // Jergosh the Invoker
        case 11520: // Taragaman the Hungerer
            return true;
        }

        return false;
    }

    bool IsTierOneFinalDungeonBoss(Creature* creature)
    {
        switch (creature->GetEntry())
        {
        case 639:   // Edwin VanCleef
        case 11519: // Bazzalan
            return true;
        }

        return false;
    }

    uint32 GetDefaultDropChanceBps(Creature* creature)
    {
        // 100 basis points = 1%.
        if (creature->GetMap()->IsDungeon())
        {
            if (IsTierOneFinalDungeonBoss(creature))
                return 2500; // 25%

            if (IsTierOneNonFinalDungeonBoss(creature))
                return 1000; // 10%

            return creature->IsElite() ? 50 : 0; // 0.5% elite trash
        }

        if (creature->IsElite())
            return 500; // 5% world elite

        return 50; // 0.5% normal world creature
    }
}

TreasureTrailMgr::TreasureTrailMgr() :
    m_startersLoaded(false)
{
}

void TreasureTrailMgr::LoadStarterItems()
{
    if (m_startersLoaded)
        return;

    m_startersLoaded = true;
    m_starterItems.clear();

    QueryResult* result = WorldDatabase.Query(
        "SELECT alliance_starter_item, horde_starter_item "
        "FROM treasure_trail_tier "
        "WHERE enabled = 1");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 allianceItem = fields[0].GetUInt32();
        uint32 hordeItem = fields[1].GetUInt32();

        if (allianceItem)
            m_starterItems.insert(allianceItem);

        if (hordeItem)
            m_starterItems.insert(hordeItem);

    } while (result->NextRow());

    delete result;
}

bool TreasureTrailMgr::GiveItemOrMail(Player* player, uint32 itemId,
    char const* subject, char const* body)
{
    if (!player || !itemId)
        return false;

    ItemPosCountVec destination;

    if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destination, itemId, 1) == EQUIP_ERR_OK)
    {
        Item* item = player->StoreNewItem(
            destination, itemId, true, Item::GenerateItemRandomPropertyId(itemId));

        if (item)
        {
            player->SendNewItem(item, 1, true, false, false, false);
            return true;
        }
    }

    Item* mailItem = Item::CreateItem(itemId, 1, player);

    if (!mailItem)
        return false;

    MailDraft draft(subject, body);
    draft.AddItem(mailItem);
    draft.SendMailTo(
        MailReceiver(player),
        MailSender(MAIL_NORMAL, uint32(0)),
        MAIL_CHECK_MASK_HAS_BODY);

    return true;
}

void TreasureTrailMgr::LogEvent(Player* player, uint8 tier, uint8 eventType,
    uint32 questId, char const* detail)
{
    if (!player)
        return;

    CharacterDatabase.PExecute(
        "INSERT INTO character_treasure_trail_log "
        "(guid, tier, event_type, quest_id, detail) "
        "VALUES (%u, %u, %u, %u, '%s')",
        player->GetGUIDLow(), tier, eventType, questId, detail ? detail : "");
}

bool TreasureTrailMgr::HasTrailState(Player* player, uint8 tier)
{
    if (!player)
        return true;

    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT 1 FROM character_treasure_trail "
        "WHERE guid = %u AND tier = %u AND status = 1",
        player->GetGUIDLow(), tier);

    if (!result)
        return false;

    delete result;
    return true;
}

bool TreasureTrailMgr::SelectNextClue(Player* player, uint8 tier, uint8 faction,
    uint32 previousClueId, TrailClue& clue)
{
    if (!player)
        return false;

    uint16 minimumProfessionSkill = 0;
    uint16 maximumProfessionSkill = 0;

    switch (tier)
    {
    case 1:
        minimumProfessionSkill = 1;
        maximumProfessionSkill = 75;
        break;
    case 2:
        minimumProfessionSkill = 75;
        maximumProfessionSkill = 150;
        break;
    case 3:
        minimumProfessionSkill = 150;
        maximumProfessionSkill = 225;
        break;
    case 4:
        minimumProfessionSkill = 225;
        maximumProfessionSkill = 250;
        break;
    case 5:
        minimumProfessionSkill = 250;
        maximumProfessionSkill = 275;
        break;
    case 6:
        minimumProfessionSkill = 275;
        maximumProfessionSkill = 300;
        break;
    default:
        return false;
    }

    QueryResult* result = WorldDatabase.PQuery(
        "SELECT id, quest_id, clue_item, weight, required_skill, required_skill_value "
        "FROM treasure_trail_clue_pool "
        "WHERE tier = %u "
        "AND faction = %u "
        "AND enabled = 1 "
        "AND id <> %u",
        tier, faction, previousClueId);

    if (!result)
        return false;

    struct WeightedClue
    {
        TrailClue clue;
        uint32 weight;
    };

    std::vector<WeightedClue> clues;
    uint32 totalWeight = 0;

    do
    {
        Field* fields = result->Fetch();

        uint16 requiredSkill = fields[4].GetUInt16();
        uint16 requiredSkillValue = fields[5].GetUInt16();

        // This checks the clue's intended tier, never the player's skill.
        if (requiredSkill &&
            (requiredSkillValue < minimumProfessionSkill ||
                requiredSkillValue > maximumProfessionSkill))
        {
            continue;
        }

        WeightedClue entry;
        entry.clue.id = fields[0].GetUInt32();
        entry.clue.questId = fields[1].GetUInt32();
        entry.clue.itemId = fields[2].GetUInt32();
        entry.weight = fields[3].GetUInt32();

        if (!entry.clue.id || !entry.clue.questId ||
            !entry.clue.itemId || !entry.weight)
        {
            continue;
        }

        clues.push_back(entry);
        totalWeight += entry.weight;

    } while (result->NextRow());

    delete result;

    if (clues.empty() || !totalWeight)
        return false;

    uint32 roll = urand(1, totalWeight);

    for (std::vector<WeightedClue>::const_iterator itr = clues.begin();
        itr != clues.end(); ++itr)
    {
        if (roll <= itr->weight)
        {
            clue = itr->clue;
            return true;
        }

        roll -= itr->weight;
    }

    return false;
}

bool TreasureTrailMgr::HandleItemUse(Player* player, Item* item)
{
    if (!player || !item)
        return false;

    LoadStarterItems();

    uint32 itemId = item->GetEntry();

    if (m_starterItems.find(itemId) == m_starterItems.end())
        return false;

    uint8 faction = GetTrailFaction(player);

    QueryResult* tierResult = WorldDatabase.PQuery(
        "SELECT tier, min_steps, max_steps "
        "FROM treasure_trail_tier "
        "WHERE enabled = 1 "
        "AND %s = %u",
        faction == 0 ? "alliance_starter_item" : "horde_starter_item",
        itemId);

    if (!tierResult)
    {
        player->GetSession()->SendNotification("This clue is not configured yet.");
        return true;
    }

    Field* tierFields = tierResult->Fetch();

    uint8 tier = tierFields[0].GetUInt8();
    uint8 minSteps = tierFields[1].GetUInt8();
    uint8 maxSteps = tierFields[2].GetUInt8();

    delete tierResult;


    bool hasPendingTrail = false;

    QueryResult* stateResult = CharacterDatabase.PQuery(
        "SELECT status FROM character_treasure_trail "
        "WHERE guid = %u AND tier = %u",
        player->GetGUIDLow(), tier);

    if (stateResult)
    {
        uint8 status = stateResult->Fetch()[0].GetUInt8();
        delete stateResult;

        if (status != 0)
        {
            player->GetSession()->SendNotification(
                "You already have an active trail of this tier.");
            return true;
        }

        // Status 0 means this is the starter clue already awarded to the player.
        hasPendingTrail = true;
    }

    TrailClue firstClue;

    if (!SelectNextClue(player, tier, faction, 0, firstClue))
    {
        player->GetSession()->SendNotification(
            "No trail clues are configured for this tier yet.");
        return true;
    }

    if (!GiveItemOrMail(player, firstClue.itemId,
        "Treasure Trail Clue",
        "Your next Treasure Trail clue is enclosed."))
    {
        player->GetSession()->SendNotification(
            "Your next clue could not be delivered.");
        return true;
    }

    player->DestroyItemCount(itemId, 1, true);

    uint8 targetSteps = urand(minSteps, maxSteps);

    if (hasPendingTrail)
    {
        CharacterDatabase.PExecute(
            "UPDATE character_treasure_trail "
            "SET faction = %u, status = 1, target_steps = %u, completed_steps = 0, "
            "current_quest = %u, previous_clue_id = %u "
            "WHERE guid = %u AND tier = %u",
            faction, targetSteps, firstClue.questId, firstClue.id,
            player->GetGUIDLow(), tier);
    }
    else
    {
        CharacterDatabase.PExecute(
            "INSERT INTO character_treasure_trail "
            "(guid, tier, faction, status, target_steps, completed_steps, current_quest, previous_clue_id) "
            "VALUES (%u, %u, %u, 1, %u, 0, %u, %u)",
            player->GetGUIDLow(), tier, faction, targetSteps,
            firstClue.questId, firstClue.id);
    }

    LogEvent(player, tier, TREASURE_TRAIL_EVENT_STARTED,
        firstClue.questId, "Trail started");

    player->GetSession()->SendNotification(
        "Your Treasure Trail has begun.");

    return true;
}

void TreasureTrailMgr::OnQuestRewarded(Player* player, uint32 questId)
{
    if (!player || !questId)
        return;

    QueryResult* stateResult = CharacterDatabase.PQuery(
        "SELECT tier, faction, target_steps, completed_steps, previous_clue_id "
        "FROM character_treasure_trail "
        "WHERE guid = %u "
        "AND status = 1 "
        "AND current_quest = %u",
        player->GetGUIDLow(), questId);

    if (!stateResult)
        return;

    Field* fields = stateResult->Fetch();

    uint8 tier = fields[0].GetUInt8();
    uint8 faction = fields[1].GetUInt8();
    uint8 targetSteps = fields[2].GetUInt8();
    uint8 completedSteps = fields[3].GetUInt8();
    uint32 previousClueId = fields[4].GetUInt32();

    delete stateResult;

    // Normal clue quests consume their clue through RewardQuest.
// Bounty clues do not use ReqItemId, so remove their clue here instead.
    QueryResult* clueItemResult = WorldDatabase.PQuery(
        "SELECT item_id FROM treasure_trail_clue_pool WHERE id = %u",
        previousClueId);

    if (clueItemResult)
    {
        uint32 clueItemId = clueItemResult->Fetch()[0].GetUInt32();
        delete clueItemResult;

        if (clueItemId)
            player->DestroyItemCount(clueItemId, 1, true);
    }

    ++completedSteps;

    if (completedSteps >= targetSteps)
    {
        QueryResult* rewardResult = WorldDatabase.PQuery(
            "SELECT casket_item, xp_reward "
            "FROM treasure_trail_tier "
            "WHERE tier = %u AND enabled = 1",
            tier);

        if (!rewardResult)
        {
            player->GetSession()->SendNotification(
                "This trail's casket reward is not configured yet.");
            return;
        }

        uint32 casketItem = rewardResult->Fetch()[0].GetUInt32();
        uint32 xpReward = rewardResult->Fetch()[1].GetUInt32();

        delete rewardResult;

        if (!GiveItemOrMail(player, casketItem,
            "Treasure Trail Casket",
            "Your Treasure Trail casket is enclosed."))
        {
            player->GetSession()->SendNotification(
                "Your casket could not be delivered.");
            return;
        }

        if (xpReward && player->GetLevel() <
            sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
        {
            player->GiveXP(xpReward, NULL);
        }

        CharacterDatabase.PExecute(
            "DELETE FROM character_treasure_trail "
            "WHERE guid = %u AND tier = %u",
            player->GetGUIDLow(), tier);

        LogEvent(player, tier, TREASURE_TRAIL_EVENT_CASKET,
            questId, "Trail completed");

        player->GetSession()->SendNotification(
            "You completed a Treasure Trail and received a casket.");

        return;
    }

    TrailClue nextClue;

    if (!SelectNextClue(player, tier, faction, previousClueId, nextClue))
    {
        player->GetSession()->SendNotification(
            "No valid next clue is configured for this trail.");
        return;
    }

    if (!GiveItemOrMail(player, nextClue.itemId,
        "Treasure Trail Clue",
        "Your next Treasure Trail clue is enclosed."))
    {
        player->GetSession()->SendNotification(
            "Your next clue could not be delivered.");
        return;
    }

    CharacterDatabase.PExecute(
        "UPDATE character_treasure_trail "
        "SET completed_steps = %u, current_quest = %u, previous_clue_id = %u "
        "WHERE guid = %u AND tier = %u",
        completedSteps, nextClue.questId, nextClue.id,
        player->GetGUIDLow(), tier);

    LogEvent(player, tier, TREASURE_TRAIL_EVENT_NEXT_CLUE,
        nextClue.questId, "Next clue awarded");
}

bool TreasureTrailMgr::AwardStarterForTier(Player* player, uint8 tier,
    uint32 chanceBps)
{
    if (!player || !tier || !chanceBps)
        return false;

    if (HasTrailState(player, tier))
        return false;

    if (urand(1, 10000) > chanceBps)
        return false;

    uint8 faction = GetTrailFaction(player);

    QueryResult* result = WorldDatabase.PQuery(
        "SELECT %s "
        "FROM treasure_trail_tier "
        "WHERE tier = %u AND enabled = 1",
        faction == 0 ? "alliance_starter_item" : "horde_starter_item",
        tier);

    if (!result)
        return false;

    uint32 starterItem = result->Fetch()[0].GetUInt32();

    delete result;

    if (!starterItem)
        return false;

    // An unused starter in inventory or bank prevents another starter drop.
    // Destroying it removes that block and allows a later drop.
    if (player->HasItemCount(starterItem, 1, true))
        return false;

    if (!GiveItemOrMail(player, starterItem,
        "Treasure Trail Starter Clue",
        "A sealed Treasure Trail clue is enclosed."))
    {
        return false;
    }

    LogEvent(player, tier, TREASURE_TRAIL_EVENT_STARTER_AWARDED,
        0, "Starter clue awarded");

    player->GetSession()->SendNotification(
        "You found a Treasure Trail clue.");

    return true;
}

void TreasureTrailMgr::TryAwardStarter(Player* player, Creature* creature)
{
    if (!player || !creature || !player->IsAlive())
        return;


    uint8 creatureLevel = creature->GetLevel();
    uint8 tier = GetTierForCreatureLevel(creatureLevel);

    if (!tier)
        return;

    uint32 baseChanceBps = GetDefaultDropChanceBps(creature);

    // A creature can award one clue maximum.
    if (AwardStarterForTier(player, tier, baseChanceBps))
        return;

    // The final two levels of a tier can award the next tier at 20% chance.
    if (tier < 6 && IsTailEndLevelForTier(creatureLevel))
    {
        uint32 nextTierChanceBps = baseChanceBps / 5;

        AwardStarterForTier(player, tier + 1, nextTierChanceBps);
    }
}