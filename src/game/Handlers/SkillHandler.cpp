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
#include "Database/DatabaseEnv.h"
#include "Opcodes.h"
#include "Log.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateMask.h"
#include "Anticheat.h"

void WorldSession::HandleLearnTalentOpcode(WorldPacket& recv_data)
{
    uint32 talent_id, requested_rank;
    recv_data >> talent_id >> requested_rank;

    _player->LearnTalent(talent_id, requested_rank);

    // Strength of Conviction depends on the talent aura being present.
    // Refresh Intellect immediately after learning or upgrading a talent.
    if (_player->GetClass() == CLASS_PALADIN)
        _player->UpdateStats(STAT_INTELLECT);
}

void WorldSession::HandleTalentWipeConfirmOpcode(WorldPacket & recv_data)
{
    DETAIL_LOG("MSG_TALENT_WIPE_CONFIRM");
    ObjectGuid guid;
    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleTalentWipeConfirmOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    if (!(_player->ResetTalents()))
    {
        WorldPacket data(MSG_TALENT_WIPE_CONFIRM, 8 + 4);
        data << uint64(0);
        data << uint32(0);
        SendPacket(&data);
        return;
    }

    // Refresh Intellect after Strength of Conviction is removed.
    if (_player->GetClass() == CLASS_PALADIN)
        _player->UpdateStats(STAT_INTELLECT);

    unit->CastSpell(_player, 14867, true);

    unit->CastSpell(_player, 14867, true);                  //spell: "Untalent Visual Effect"
}

void WorldSession::HandleUnlearnSkillOpcode(WorldPacket & recv_data)
{
    uint32 skill_id;
    recv_data >> skill_id;
    SkillRaceClassInfoEntry const* rcEntry = GetSkillRaceClassInfo(skill_id, GetPlayer()->GetRace(), GetPlayer()->GetClass());
    if (!rcEntry || !(rcEntry->flags & SKILL_FLAG_UNLEARNABLE))
    {
        std::stringstream reason;
        reason << "Attempt to unlearn not unlearnable skill #" << skill_id;
        ProcessAnticheatAction("PassiveAnticheat", reason.str().c_str(), CHEAT_ACTION_LOG | CHEAT_ACTION_REPORT_GMS);
        return;
    }
    GetPlayer()->SetSkill(skill_id, 0, 0);
}
