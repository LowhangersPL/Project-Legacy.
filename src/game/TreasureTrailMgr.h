#ifndef MANGOS_TREASURE_TRAIL_MGR_H
#define MANGOS_TREASURE_TRAIL_MGR_H

#include "Common.h"

#include <set>

class Creature;
class Item;
class Player;

class TreasureTrailMgr
{
public:
    TreasureTrailMgr();

    // Called when a player uses an item. Returns true only when this system handled it.
    bool HandleItemUse(Player* player, Item* item);

    // Called after a quest reward is granted.
    void OnQuestRewarded(Player* player, uint32 questId);

    // Called for each eligible player when a creature dies.
    void TryAwardStarter(Player* player, Creature* creature);

private:
    struct TrailClue
    {
        uint32 id;
        uint32 questId;
        uint32 itemId;

        TrailClue() : id(0), questId(0), itemId(0) {}
    };

    void LoadStarterItems();

    bool SelectNextClue(Player* player, uint8 tier, uint8 faction,
        uint32 previousClueId, TrailClue& clue);

    bool GiveItemOrMail(Player* player, uint32 itemId,
        char const* subject, char const* body);

    bool HasTrailState(Player* player, uint8 tier);

    bool AwardStarterForTier(Player* player, uint8 tier, uint32 chanceBps);

    void LogEvent(Player* player, uint8 tier, uint8 eventType,
        uint32 questId, char const* detail);

    bool m_startersLoaded;
    std::set<uint32> m_starterItems;
};

extern TreasureTrailMgr sTreasureTrailMgr;

#endif