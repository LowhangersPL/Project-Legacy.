#include "Chat.h"
#include "NodesMgr.h"
#include "NodeSession.h"
#include "WorldSession.h"
#include "Player.h"

bool ChatHandler::HandleNodeServersListCommand(char*)
{
    sNodesMgr->ListServers(*this);
    return true;
}

bool ChatHandler::HandleNodeServersSwitchCommand(char* args)
{
    uint32 nodeServerId = 0;
    if (!ExtractUInt32(&args, nodeServerId))
        return false;
    NodeSession* node = sNodesMgr->GetNodeById(nodeServerId);
    SetSentErrorMessage(true);
    if (!node || !node->IsReady())
    {
        PSendSysMessage("Node #%u not found or not ready.", nodeServerId);
        return false;
    }
    Player* me = GetSession()->GetPlayer();
    if (!me)
    {
        SendSysMessage("You are not on the right node.");
        return false;
    }
    GetSession()->LoginPlayerToNode(node);
    return true;
}

void NodesMgr::ListServers(ChatHandler& handler)
{
    handler.PSendSysMessage("%u nodes.", m_nodes.size());
    for (NodesMap::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        handler.PSendSysMessage("[%3u][%s] %s", it->first, it->second->IsConnectedToMaster() ? "MSTR" : "NODE", it->second->GetName());
}

bool ChatHandler::HandleRMTCommand(char* args)
{
    if (!*args)
    {
        SendSysMessage("Usage: .rmt player <playername>");
        return true;
    }

    char* nameStr = ExtractLiteralArg(&args);
    if (!nameStr)
    {
        SendSysMessage("Usage: .rmt player <playername>");
        return true;
    }

    std::string name = nameStr;
    normalizePlayerName(name);

    if (!sObjectMgr.GetPlayerGuidByName(name))
    {
        PSendSysMessage("No character found named %s.", name.c_str());
        return true;
    }

    SendSysMessage("|cffff0000[RMT Investigation]|r");

    QueryResult* totals = CharacterDatabase.PQuery(
        "SELECT "
        "IFNULL((SELECT SUM(gold_amount) FROM rmt_trade_log WHERE sender_name = '%s'), 0), "
        "IFNULL((SELECT SUM(gold_amount) FROM rmt_trade_log WHERE receiver_name = '%s'), 0), "
        "IFNULL((SELECT SUM(gold_amount) FROM rmt_mail_log WHERE sender_name = '%s'), 0), "
        "IFNULL((SELECT SUM(gold_amount) FROM rmt_mail_log WHERE receiver_name = '%s'), 0), "
        "IFNULL((SELECT SUM(cod_amount) FROM rmt_mail_log WHERE sender_name = '%s'), 0), "
        "IFNULL((SELECT SUM(cod_amount) FROM rmt_mail_log WHERE receiver_name = '%s'), 0)",
        name.c_str(), name.c_str(), name.c_str(),
        name.c_str(), name.c_str(), name.c_str()
    );

    uint32 tradeSentAmount = 0;
    uint32 tradeReceivedAmount = 0;
    uint32 mailSentAmount = 0;
    uint32 mailReceivedAmount = 0;

    if (totals)
    {
        Field* t = totals->Fetch();

        tradeSentAmount = t[0].GetUInt32();
        tradeReceivedAmount = t[1].GetUInt32();
        mailSentAmount = t[2].GetUInt32();
        mailReceivedAmount = t[3].GetUInt32();

        PSendSysMessage("Totals:");
        PSendSysMessage("Trade sent: %.2fg | Trade received: %.2fg", float(t[0].GetUInt32()) / 10000.0f, float(t[1].GetUInt32()) / 10000.0f);
        PSendSysMessage("Mail sent: %.2fg | Mail received: %.2fg", float(t[2].GetUInt32()) / 10000.0f, float(t[3].GetUInt32()) / 10000.0f);
        PSendSysMessage("COD requested: %.2fg | COD paid/received: %.2fg", float(t[4].GetUInt32()) / 10000.0f, float(t[5].GetUInt32()) / 10000.0f);

        delete totals;
    }

    QueryResult* counts = CharacterDatabase.PQuery(
        "SELECT "
        "IFNULL((SELECT COUNT(*) FROM rmt_trade_log WHERE sender_name = '%s'), 0), "
        "IFNULL((SELECT COUNT(*) FROM rmt_trade_log WHERE receiver_name = '%s'), 0), "
        "IFNULL((SELECT COUNT(*) FROM rmt_mail_log WHERE sender_name = '%s'), 0), "
        "IFNULL((SELECT COUNT(*) FROM rmt_mail_log WHERE receiver_name = '%s'), 0), "
        "IFNULL((SELECT COUNT(DISTINCT receiver_name) FROM rmt_trade_log WHERE sender_name = '%s'), 0), "
        "IFNULL((SELECT COUNT(DISTINCT sender_name) FROM rmt_trade_log WHERE receiver_name = '%s'), 0)",
        name.c_str(), name.c_str(), name.c_str(),
        name.c_str(), name.c_str(), name.c_str()
    );

    uint32 tradesSentCount = 0;
    uint32 tradesReceivedCount = 0;
    uint32 mailsSentCount = 0;
    uint32 mailsReceivedCount = 0;
    uint32 uniqueReceivers = 0;
    uint32 uniqueSenders = 0;

    if (counts)
    {
        Field* c = counts->Fetch();

        tradesSentCount = c[0].GetUInt32();
        tradesReceivedCount = c[1].GetUInt32();
        mailsSentCount = c[2].GetUInt32();
        mailsReceivedCount = c[3].GetUInt32();
        uniqueReceivers = c[4].GetUInt32();
        uniqueSenders = c[5].GetUInt32();

        PSendSysMessage("Counts:");
        PSendSysMessage("Trades sent: %u | Trades received: %u", tradesSentCount, tradesReceivedCount);
        PSendSysMessage("Mails sent: %u | Mails received: %u", mailsSentCount, mailsReceivedCount);
        PSendSysMessage("Unique trade receivers: %u | Unique trade senders: %u", uniqueReceivers, uniqueSenders);

        delete counts;
    }

    uint32 riskScore = 0;
    std::string riskReasons = "";

    uint32 totalSent = tradeSentAmount + mailSentAmount;
    uint32 totalReceived = tradeReceivedAmount + mailReceivedAmount;
    uint32 totalTx = tradesSentCount + tradesReceivedCount + mailsSentCount + mailsReceivedCount;

    if (totalReceived >= 1000000)
    {
        riskScore += 10;
        riskReasons += "- Received 100g+ total.\n";
    }

    if (totalReceived >= 5000000)
    {
        riskScore += 20;
        riskReasons += "- Received 500g+ total.\n";
    }

    if (totalSent >= 5000000)
    {
        riskScore += 15;
        riskReasons += "- Sent 500g+ total.\n";
    }

    if (uniqueReceivers >= 5)
    {
        riskScore += 10;
        riskReasons += "- Sent gold to 5+ different players.\n";
    }

    if (uniqueSenders >= 5)
    {
        riskScore += 10;
        riskReasons += "- Received gold from 5+ different players.\n";
    }

    if (totalTx >= 10)
    {
        riskScore += 10;
        riskReasons += "- 10+ gold movement transactions logged.\n";
    }

    PSendSysMessage("Risk hint: %u", riskScore);

    if (!riskReasons.empty())
        PSendSysMessage("Risk reasons:\n%s", riskReasons.c_str());

    if (riskScore >= 30)
        SendSysMessage("|cffff0000High risk: review this player carefully.|r");
    else if (riskScore >= 15)
        SendSysMessage("|cffffff00Medium risk: worth checking recent activity.|r");
    else
        SendSysMessage("|cff00ff00Low risk based on current gold movement.|r");

    QueryResult* links = CharacterDatabase.PQuery(
        "SELECT other_name, SUM(amount) AS total_amount, COUNT(*) AS tx_count FROM ("
        "SELECT receiver_name AS other_name, gold_amount AS amount FROM rmt_trade_log WHERE sender_name = '%s' "
        "UNION ALL "
        "SELECT sender_name AS other_name, gold_amount AS amount FROM rmt_trade_log WHERE receiver_name = '%s' "
        "UNION ALL "
        "SELECT receiver_name AS other_name, gold_amount AS amount FROM rmt_mail_log WHERE sender_name = '%s' "
        "UNION ALL "
        "SELECT sender_name AS other_name, gold_amount AS amount FROM rmt_mail_log WHERE receiver_name = '%s' "
        ") x "
        "GROUP BY other_name "
        "ORDER BY total_amount DESC "
        "LIMIT 5",
        name.c_str(), name.c_str(), name.c_str(), name.c_str()
    );

    if (links)
    {
        SendSysMessage("Top linked players:");

        do
        {
            Field* f = links->Fetch();

            PSendSysMessage("%s | %.2fg total | %u transactions",
                f[0].GetString(),
                float(f[1].GetUInt32()) / 10000.0f,
                f[2].GetUInt32());
        } while (links->NextRow());

        delete links;
    }

    QueryResult* result = CharacterDatabase.PQuery(
        "(SELECT 'TRADE' AS source_type, sender_name, receiver_name, gold_amount, 0 AS cod_amount, created_at "
        "FROM rmt_trade_log "
        "WHERE sender_name = '%s' OR receiver_name = '%s' "
        "ORDER BY created_at DESC "
        "LIMIT 10) "
        "UNION ALL "
        "(SELECT 'MAIL' AS source_type, sender_name, receiver_name, gold_amount, cod_amount, created_at "
        "FROM rmt_mail_log "
        "WHERE sender_name = '%s' OR receiver_name = '%s' "
        "ORDER BY created_at DESC "
        "LIMIT 10) "
        "ORDER BY created_at DESC "
        "LIMIT 15",
        name.c_str(), name.c_str(), name.c_str(), name.c_str()
    );

    if (!result)
    {
        PSendSysMessage("No recent gold movement found for %s.", name.c_str());
        return true;
    }

    PSendSysMessage("Recent gold movement for %s:", name.c_str());

    uint32 count = 1;

    do
    {
        Field* fields = result->Fetch();

        std::string sourceType = fields[0].GetString();
        std::string senderName = fields[1].GetString();
        std::string receiverName = fields[2].GetString();
        uint32 goldAmount = fields[3].GetUInt32();
        uint32 codAmount = fields[4].GetUInt32();
        std::string createdAt = fields[5].GetString();

        if (codAmount > 0)
            PSendSysMessage("%u. [%s] %s -> %s | %.2fg sent | %.2fg COD | %s",
                count, sourceType.c_str(), senderName.c_str(), receiverName.c_str(),
                float(goldAmount) / 10000.0f, float(codAmount) / 10000.0f, createdAt.c_str());
        else
            PSendSysMessage("%u. [%s] %s -> %s | %.2fg | %s",
                count, sourceType.c_str(), senderName.c_str(), receiverName.c_str(),
                float(goldAmount) / 10000.0f, createdAt.c_str());

        ++count;
    } while (result->NextRow());

    delete result;
    return true;
}

bool ChatHandler::HandleRMTFlagsCommand(char* args)
{
    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT player_name, score, reason, related_player, source_type, gold_amount, created_at "
        "FROM rmt_flag_log "
        "ORDER BY created_at DESC "
        "LIMIT 15"
    );

    if (!result)
    {
        SendSysMessage("No recent RMT flags found.");
        return true;
    }

    SendSysMessage("|cffff0000[RMT Flags]|r Recent suspicious gold movement:");

    uint32 count = 1;

    do
    {
        Field* fields = result->Fetch();

        std::string playerName = fields[0].GetString();
        uint32 score = fields[1].GetUInt32();
        std::string reason = fields[2].GetString();
        std::string relatedPlayer = fields[3].GetString();
        std::string sourceType = fields[4].GetString();
        uint32 goldAmount = fields[5].GetUInt32();
        std::string createdAt = fields[6].GetString();

        PSendSysMessage(
            "%u. %s | Score %u | %s | Related: %s | %.2fg | %s | %s",
            count,
            playerName.c_str(),
            score,
            sourceType.c_str(),
            relatedPlayer.c_str(),
            float(goldAmount) / 10000.0f,
            createdAt.c_str(),
            reason.c_str()
        );

        ++count;
    } while (result->NextRow());

    delete result;
    return true;
}