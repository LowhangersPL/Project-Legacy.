#include "scriptPCH.h"

bool GOHello_go_karazhan_crypts_gate(Player* player, GameObject* go)
{
    if (!player || !go)
        return false;

    if (!player->HasItemCount(900100, 1))
    {
        player->GetSession()->SendNotification(
            "You need the Karazhan Crypts Key."
        );

        return true;
    }

    go->UseDoorOrButton();

    return true;
}

void AddSC_custom_karazhan()
{
    Script* script = new Script;

    script->Name = "go_karazhan_crypts_gate";
    script->pGOHello = &GOHello_go_karazhan_crypts_gate;

    script->RegisterSelf();
}