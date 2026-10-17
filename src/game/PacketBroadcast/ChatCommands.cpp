#include "Chat.h"
#include "MovementBroadcaster.h"
#include "PlayerBroadcaster.h"
#include "World.h"
#include "Util.h"

bool ChatHandler::HandlePBCastStatsCommand(char*)
{
    MovementBroadcaster* bcaster = sWorld.GetBroadcaster();
    auto const& stats = bcaster->GetStats();
    PSendSysMessage("PacketBroadcast: %u threads.", stats.size());
    for (int i = 0; i < stats.size(); ++i)
        PSendSysMessage("Thread #%02u: Update %03ums | %u packets",
            i, stats[i].update_time, stats[i].num_packets);
    PSendSysMessage("Created %u broadcasters | Deleted %u",
        PlayerBroadcaster::num_bcaster_created, PlayerBroadcaster::num_bcaster_deleted);
    return true;
}

bool ChatHandler::HandlePBCastSetThreadsCommand(char* args)
{
    MovementBroadcaster* bcaster = sWorld.GetBroadcaster();
    std::size_t num_threads_before = bcaster->GetNumThreads();
    uint32 num_threads_after;
    if (!ExtractUInt32(&args, num_threads_after))
        return false;

    if (num_threads_after > 50)
    {
        PSendSysMessage("PacketBroadcast: Please decrease the thread count (< 50)");
        return false;
    }

    if (!num_threads_after)
        PSendSysMessage("PacketBroadcast: Disabling broadcaster...");
    else
        PSendSysMessage("PacketBroadcast: Changing num of threads from %u to %u...",
            num_threads_before, num_threads_after);

    bcaster->UpdateConfiguration(num_threads_after, bcaster->GetSleepTimer());
    SendSysMessage(".. done!");
    return true;
}

bool ChatHandler::HandlePrestigeCheckCommand(char* args)
{
    if (!args || !*args)
        return false;

    // Extract only the character name and discard trailing whitespace.
    char* nameArg = ExtractArg(&args);
    if (!nameArg || !*nameArg)
        return false;

    std::string playerName = nameArg;

    // Converts names such as "testname" into the core's normal form,
    // normally "Testname", and rejects invalid character names.
    if (!normalizePlayerName(playerName))
        return false;

    std::string escapedName = playerName;
    CharacterDatabase.escape_string(escapedName);

    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT cp.prestige_count "
        "FROM characters c "
        "LEFT JOIN character_prestige cp ON cp.guid = c.guid "
        "WHERE c.name = '%s' "
        "LIMIT 1",
        escapedName.c_str());

    uint32 prestige = 0;

    if (result)
    {
        Field* fields = result->Fetch();
        prestige = fields[0].GetUInt32();

        delete result;
    }

    PSendSysMessage(
        "PRESTIGEUI %s %u",
        playerName.c_str(),
        prestige);

    return true;
}
