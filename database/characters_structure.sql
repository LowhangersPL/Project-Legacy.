-- --------------------------------------------------------
-- Host:                         127.0.0.1
-- Server version:               5.5.62 - MySQL Community Server (GPL)
-- Server OS:                    Win32
-- HeidiSQL Version:             12.17.0.7270
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;


-- Dumping database structure for characters
CREATE DATABASE IF NOT EXISTS `characters` /*!40100 DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci */;
USE `characters`;

-- Dumping structure for table characters.anticheat_log
CREATE TABLE IF NOT EXISTS `anticheat_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int(10) unsigned NOT NULL DEFAULT '0',
  `guid` int(10) unsigned NOT NULL DEFAULT '0',
  `player_name` varchar(12) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `check_type` varchar(32) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `map` int(10) unsigned NOT NULL DEFAULT '0',
  `x` float NOT NULL DEFAULT '0',
  `y` float NOT NULL DEFAULT '0',
  `z` float NOT NULL DEFAULT '0',
  `details` text COLLATE utf8_unicode_ci,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_guid` (`guid`),
  KEY `idx_check_type` (`check_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.auction
CREATE TABLE IF NOT EXISTS `auction` (
  `id` int(11) unsigned NOT NULL DEFAULT '0',
  `houseid` int(11) unsigned NOT NULL DEFAULT '0',
  `itemguid` int(11) unsigned NOT NULL DEFAULT '0',
  `item_template` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Item Identifier',
  `itemowner` int(11) unsigned NOT NULL DEFAULT '0',
  `buyoutprice` int(11) NOT NULL DEFAULT '0',
  `time` bigint(40) NOT NULL DEFAULT '0',
  `buyguid` int(11) unsigned NOT NULL DEFAULT '0',
  `lastbid` int(11) NOT NULL DEFAULT '0',
  `startbid` int(11) NOT NULL DEFAULT '0',
  `deposit` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `key_item_guid` (`itemguid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.bot_detection_log
CREATE TABLE IF NOT EXISTS `bot_detection_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `account` int(10) unsigned NOT NULL,
  `guid` int(10) unsigned NOT NULL,
  `player_name` varchar(12) COLLATE utf8_unicode_ci NOT NULL,
  `score` int(10) unsigned NOT NULL,
  `reason` text COLLATE utf8_unicode_ci NOT NULL,
  `map` int(10) unsigned NOT NULL,
  `x` float NOT NULL,
  `y` float NOT NULL,
  `z` float NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_guid` (`guid`),
  KEY `idx_score` (`score`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.bugreport
CREATE TABLE IF NOT EXISTS `bugreport` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
  `type` longtext NOT NULL,
  `content` longtext NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Debug System';

-- Data exporting was unselected.

-- Dumping structure for table characters.bugreports
CREATE TABLE IF NOT EXISTS `bugreports` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `reportTime` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `playerGuid` int(11) DEFAULT NULL,
  `playerMap` int(11) DEFAULT NULL,
  `playerInstanceId` int(11) DEFAULT NULL,
  `playerX` float DEFAULT NULL,
  `playerY` float DEFAULT NULL,
  `playerZ` float DEFAULT NULL,
  `clientIp` varchar(100) DEFAULT NULL,
  `reportType` tinyint(4) DEFAULT NULL,
  `reportText` text,
  `serverInformation` text,
  `bugStatus` enum('New','NeedTest','Fixed','Invalid','Duplicate','Confirmed') NOT NULL DEFAULT 'New',
  KEY `idx_id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Data exporting was unselected.

-- Dumping structure for table characters.census
CREATE TABLE IF NOT EXISTS `census` (
  `time` datetime DEFAULT NULL,
  `race` int(11) DEFAULT NULL,
  `onlineCount` int(11) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_action
CREATE TABLE IF NOT EXISTS `character_action` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `button` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `action` int(11) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`button`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_aura
CREATE TABLE IF NOT EXISTS `character_aura` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `caster_guid` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT 'Full Global Unique Identifier',
  `item_guid` int(11) unsigned NOT NULL DEFAULT '0',
  `spell` int(11) unsigned NOT NULL DEFAULT '0',
  `stackcount` int(11) unsigned NOT NULL DEFAULT '1',
  `remaincharges` int(11) unsigned NOT NULL DEFAULT '0',
  `basepoints0` int(11) NOT NULL DEFAULT '0',
  `basepoints1` int(11) NOT NULL DEFAULT '0',
  `basepoints2` int(11) NOT NULL DEFAULT '0',
  `periodictime0` int(11) unsigned NOT NULL DEFAULT '0',
  `periodictime1` int(11) unsigned NOT NULL DEFAULT '0',
  `periodictime2` int(11) unsigned NOT NULL DEFAULT '0',
  `maxduration` int(11) NOT NULL DEFAULT '0',
  `remaintime` int(11) NOT NULL DEFAULT '0',
  `effIndexMask` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`caster_guid`,`item_guid`,`spell`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_battleground_data
CREATE TABLE IF NOT EXISTS `character_battleground_data` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `instance_id` int(11) unsigned NOT NULL DEFAULT '0',
  `team` int(11) unsigned NOT NULL DEFAULT '0',
  `join_x` float NOT NULL DEFAULT '0',
  `join_y` float NOT NULL DEFAULT '0',
  `join_z` float NOT NULL DEFAULT '0',
  `join_o` float NOT NULL DEFAULT '0',
  `join_map` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_bgqueue
CREATE TABLE IF NOT EXISTS `character_bgqueue` (
  `PlayerGUID` int(11) NOT NULL DEFAULT '0',
  `playerName` varchar(12) CHARACTER SET latin1 NOT NULL,
  `playerIP` int(11) NOT NULL,
  `BGtype` int(11) NOT NULL,
  `action` int(11) NOT NULL,
  `time` int(11) NOT NULL,
  PRIMARY KEY (`PlayerGUID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_creation_times
CREATE TABLE IF NOT EXISTS `character_creation_times` (
  `guid` int(10) unsigned NOT NULL,
  `creation_time` bigint(20) unsigned NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_deleted_items
CREATE TABLE IF NOT EXISTS `character_deleted_items` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `player_guid` int(11) unsigned NOT NULL DEFAULT '0',
  `item_entry` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `stack_count` mediumint(8) unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_dualspec
CREATE TABLE IF NOT EXISTS `character_dualspec` (
  `guid` int(10) unsigned NOT NULL,
  `has_dual_spec` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `active_spec` tinyint(3) unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_dualspec_talents
CREATE TABLE IF NOT EXISTS `character_dualspec_talents` (
  `guid` int(10) unsigned NOT NULL,
  `spec` tinyint(3) unsigned NOT NULL,
  `talent_id` int(10) unsigned NOT NULL,
  `rank` tinyint(3) unsigned NOT NULL,
  PRIMARY KEY (`guid`,`spec`,`talent_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_duplicate_account
CREATE TABLE IF NOT EXISTS `character_duplicate_account` (
  `account` int(11) DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_dynamic_world_interaction
CREATE TABLE IF NOT EXISTS `character_dynamic_world_interaction` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Character GUID',
  `interaction_id` int(10) unsigned NOT NULL DEFAULT '0',
  `completion_count` int(10) unsigned NOT NULL DEFAULT '0',
  `first_completed` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT 'Unix timestamp',
  `last_completed` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT 'Unix timestamp',
  PRIMARY KEY (`guid`,`interaction_id`),
  KEY `idx_interaction_id` (`interaction_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Per-character Dynamic World Interaction history';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_forgotten_skills
CREATE TABLE IF NOT EXISTS `character_forgotten_skills` (
  `guid` int(11) unsigned NOT NULL COMMENT 'Global Unique Identifier',
  `skill` mediumint(9) unsigned NOT NULL,
  `value` mediumint(9) unsigned NOT NULL,
  PRIMARY KEY (`guid`,`skill`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_gifts
CREATE TABLE IF NOT EXISTS `character_gifts` (
  `guid` int(20) unsigned NOT NULL DEFAULT '0',
  `item_guid` int(11) unsigned NOT NULL DEFAULT '0',
  `entry` int(20) unsigned NOT NULL DEFAULT '0',
  `flags` int(20) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`item_guid`),
  KEY `idx_guid` (`guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_homebind
CREATE TABLE IF NOT EXISTS `character_homebind` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `map` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Map Identifier',
  `zone` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Zone Identifier',
  `position_x` float NOT NULL DEFAULT '0',
  `position_y` float NOT NULL DEFAULT '0',
  `position_z` float NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_honor_cp
CREATE TABLE IF NOT EXISTS `character_honor_cp` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `victimType` tinyint(3) unsigned NOT NULL DEFAULT '4',
  `victim` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Creature / Player Identifier',
  `cp` float NOT NULL DEFAULT '0',
  `date` int(11) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(3) unsigned NOT NULL DEFAULT '0',
  KEY `idx_guid` (`guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_instance
CREATE TABLE IF NOT EXISTS `character_instance` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `instance` int(11) unsigned NOT NULL DEFAULT '0',
  `permanent` tinyint(1) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`instance`),
  KEY `idx_instance` (`instance`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_inventory
CREATE TABLE IF NOT EXISTS `character_inventory` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `bag` int(11) unsigned NOT NULL DEFAULT '0',
  `slot` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `item` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Item Global Unique Identifier',
  `item_template` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Item Identifier',
  PRIMARY KEY (`item`),
  KEY `idx_guid` (`guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_lorewalker
CREATE TABLE IF NOT EXISTS `character_lorewalker` (
  `guid` int(10) unsigned NOT NULL,
  `active` tinyint(3) unsigned NOT NULL DEFAULT '1',
  `abandoned` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `last_reward_level` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `joined_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `completed` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `reward_10` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `reward_20` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `reward_30` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `reward_40` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `reward_50` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `reward_60` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `grand_reward` tinyint(1) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_pet
CREATE TABLE IF NOT EXISTS `character_pet` (
  `id` int(11) unsigned NOT NULL DEFAULT '0',
  `entry` int(11) unsigned NOT NULL DEFAULT '0',
  `owner` int(11) unsigned NOT NULL DEFAULT '0',
  `modelid` int(11) unsigned DEFAULT '0',
  `CreatedBySpell` int(11) unsigned NOT NULL DEFAULT '0',
  `PetType` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `level` int(11) unsigned NOT NULL DEFAULT '1',
  `exp` int(11) unsigned NOT NULL DEFAULT '0',
  `Reactstate` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `loyaltypoints` int(11) NOT NULL DEFAULT '0',
  `loyalty` int(11) unsigned NOT NULL DEFAULT '0',
  `trainpoint` int(11) NOT NULL DEFAULT '0',
  `name` varchar(100) DEFAULT 'Pet',
  `renamed` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `slot` int(11) unsigned NOT NULL DEFAULT '0',
  `curhealth` int(11) unsigned NOT NULL DEFAULT '1',
  `curmana` int(11) unsigned NOT NULL DEFAULT '0',
  `curhappiness` int(11) unsigned NOT NULL DEFAULT '0',
  `savetime` bigint(20) unsigned NOT NULL DEFAULT '0',
  `resettalents_cost` int(11) unsigned NOT NULL DEFAULT '0',
  `resettalents_time` bigint(20) unsigned NOT NULL DEFAULT '0',
  `abdata` longtext,
  `teachspelldata` longtext,
  PRIMARY KEY (`id`),
  KEY `idx_owner` (`owner`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Pet System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_prestige
CREATE TABLE IF NOT EXISTS `character_prestige` (
  `guid` int(10) unsigned NOT NULL,
  `prestige_count` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `last_prestiged_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `prestige_travel_used` tinyint(1) NOT NULL DEFAULT '0',
  `travel_used` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `prestige_pending` tinyint(1) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_priest_tradition
CREATE TABLE IF NOT EXISTS `character_priest_tradition` (
  `guid` int(10) unsigned NOT NULL,
  `swap_count` int(10) unsigned NOT NULL DEFAULT '0',
  `last_swap_at` timestamp NULL DEFAULT NULL,
  `current_tradition` tinyint(3) unsigned NOT NULL DEFAULT '255',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_queststatus
CREATE TABLE IF NOT EXISTS `character_queststatus` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `quest` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Quest Identifier',
  `status` int(11) unsigned NOT NULL DEFAULT '0',
  `rewarded` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `explored` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `timer` bigint(20) unsigned NOT NULL DEFAULT '0',
  `mobcount1` int(11) unsigned NOT NULL DEFAULT '0',
  `mobcount2` int(11) unsigned NOT NULL DEFAULT '0',
  `mobcount3` int(11) unsigned NOT NULL DEFAULT '0',
  `mobcount4` int(11) unsigned NOT NULL DEFAULT '0',
  `itemcount1` int(11) unsigned NOT NULL DEFAULT '0',
  `itemcount2` int(11) unsigned NOT NULL DEFAULT '0',
  `itemcount3` int(11) unsigned NOT NULL DEFAULT '0',
  `itemcount4` int(11) unsigned NOT NULL DEFAULT '0',
  `reward_choice` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`quest`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_reputation
CREATE TABLE IF NOT EXISTS `character_reputation` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `faction` int(11) unsigned NOT NULL DEFAULT '0',
  `standing` int(11) NOT NULL DEFAULT '0',
  `flags` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`faction`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_skills
CREATE TABLE IF NOT EXISTS `character_skills` (
  `guid` int(11) unsigned NOT NULL COMMENT 'Global Unique Identifier',
  `skill` mediumint(9) unsigned NOT NULL,
  `value` mediumint(9) unsigned NOT NULL,
  `max` mediumint(9) unsigned NOT NULL,
  PRIMARY KEY (`guid`,`skill`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_social
CREATE TABLE IF NOT EXISTS `character_social` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Character Global Unique Identifier',
  `friend` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Friend Global Unique Identifier',
  `flags` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT 'Friend Flags',
  PRIMARY KEY (`guid`,`friend`,`flags`),
  KEY `idx_guid` (`guid`),
  KEY `idx_friend` (`friend`),
  KEY `idx_guid_flags` (`guid`,`flags`),
  KEY `idx_friend_flags` (`friend`,`flags`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_spell
CREATE TABLE IF NOT EXISTS `character_spell` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `spell` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Spell Identifier',
  `active` tinyint(3) unsigned NOT NULL DEFAULT '1',
  `disabled` tinyint(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`spell`),
  KEY `idx_spell` (`spell`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_spell_cooldown
CREATE TABLE IF NOT EXISTS `character_spell_cooldown` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier, Low part',
  `spell` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Spell Identifier',
  `item` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Item Identifier',
  `time` bigint(20) unsigned NOT NULL DEFAULT '0',
  `catTime` bigint(20) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`spell`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_stats
CREATE TABLE IF NOT EXISTS `character_stats` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier, Low part',
  `maxhealth` int(10) unsigned NOT NULL DEFAULT '0',
  `maxpower1` int(10) unsigned NOT NULL DEFAULT '0',
  `maxpower2` int(10) unsigned NOT NULL DEFAULT '0',
  `maxpower3` int(10) unsigned NOT NULL DEFAULT '0',
  `maxpower4` int(10) unsigned NOT NULL DEFAULT '0',
  `maxpower5` int(10) unsigned NOT NULL DEFAULT '0',
  `maxpower6` int(10) unsigned NOT NULL DEFAULT '0',
  `maxpower7` int(10) unsigned NOT NULL DEFAULT '0',
  `strength` int(10) unsigned NOT NULL DEFAULT '0',
  `agility` int(10) unsigned NOT NULL DEFAULT '0',
  `stamina` int(10) unsigned NOT NULL DEFAULT '0',
  `intellect` int(10) unsigned NOT NULL DEFAULT '0',
  `spirit` int(10) unsigned NOT NULL DEFAULT '0',
  `armor` int(10) unsigned NOT NULL DEFAULT '0',
  `resHoly` int(10) unsigned NOT NULL DEFAULT '0',
  `resFire` int(10) unsigned NOT NULL DEFAULT '0',
  `resNature` int(10) unsigned NOT NULL DEFAULT '0',
  `resFrost` int(10) unsigned NOT NULL DEFAULT '0',
  `resShadow` int(10) unsigned NOT NULL DEFAULT '0',
  `resArcane` int(10) unsigned NOT NULL DEFAULT '0',
  `blockPct` float unsigned NOT NULL DEFAULT '0',
  `dodgePct` float unsigned NOT NULL DEFAULT '0',
  `parryPct` float unsigned NOT NULL DEFAULT '0',
  `critPct` float unsigned NOT NULL DEFAULT '0',
  `rangedCritPct` float unsigned NOT NULL DEFAULT '0',
  `attackPower` int(10) unsigned NOT NULL DEFAULT '0',
  `rangedAttackPower` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_ticket
CREATE TABLE IF NOT EXISTS `character_ticket` (
  `ticket_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `ticket_text` text,
  `response_text` text,
  `ticket_lastchange` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`ticket_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_treasure_trail
CREATE TABLE IF NOT EXISTS `character_treasure_trail` (
  `guid` int(10) unsigned NOT NULL,
  `tier` tinyint(3) unsigned NOT NULL,
  `faction` tinyint(3) unsigned NOT NULL,
  `status` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `target_steps` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `completed_steps` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `current_quest` int(10) unsigned NOT NULL DEFAULT '0',
  `previous_clue_id` int(10) unsigned NOT NULL DEFAULT '0',
  `started_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`guid`,`tier`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_treasure_trail_log
CREATE TABLE IF NOT EXISTS `character_treasure_trail_log` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `guid` int(10) unsigned NOT NULL,
  `tier` tinyint(3) unsigned NOT NULL,
  `event_type` tinyint(3) unsigned NOT NULL,
  `quest_id` int(10) unsigned NOT NULL DEFAULT '0',
  `detail` varchar(255) NOT NULL DEFAULT '',
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_treasure_trail_log_guid` (`guid`,`created_at`)
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.character_tutorial
CREATE TABLE IF NOT EXISTS `character_tutorial` (
  `account` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT 'Account Identifier',
  `tut0` int(11) unsigned NOT NULL DEFAULT '0',
  `tut1` int(11) unsigned NOT NULL DEFAULT '0',
  `tut2` int(11) unsigned NOT NULL DEFAULT '0',
  `tut3` int(11) unsigned NOT NULL DEFAULT '0',
  `tut4` int(11) unsigned NOT NULL DEFAULT '0',
  `tut5` int(11) unsigned NOT NULL DEFAULT '0',
  `tut6` int(11) unsigned NOT NULL DEFAULT '0',
  `tut7` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM AUTO_INCREMENT=20 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.character_warmode
CREATE TABLE IF NOT EXISTS `character_warmode` (
  `guid` int(10) unsigned NOT NULL,
  `enabled` tinyint(1) NOT NULL DEFAULT '0',
  `disable_count` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `reenable_after` int(10) unsigned NOT NULL DEFAULT '0',
  `disabled_forever` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.characters
CREATE TABLE IF NOT EXISTS `characters` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `account` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Account Identifier',
  `name` varchar(12) NOT NULL DEFAULT '',
  `race` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `gender` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `level` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `xp` int(10) unsigned NOT NULL DEFAULT '0',
  `money` int(10) unsigned NOT NULL DEFAULT '0',
  `playerBytes` int(10) unsigned NOT NULL DEFAULT '0',
  `playerBytes2` int(10) unsigned NOT NULL DEFAULT '0',
  `playerFlags` int(10) unsigned NOT NULL DEFAULT '0',
  `position_x` float NOT NULL DEFAULT '0',
  `position_y` float NOT NULL DEFAULT '0',
  `position_z` float NOT NULL DEFAULT '0',
  `map` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Map Identifier',
  `orientation` float NOT NULL DEFAULT '0',
  `taximask` longtext,
  `online` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `cinematic` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `totaltime` int(11) unsigned NOT NULL DEFAULT '0',
  `leveltime` int(11) unsigned NOT NULL DEFAULT '0',
  `logout_time` bigint(20) unsigned NOT NULL DEFAULT '0',
  `is_logout_resting` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `rest_bonus` float NOT NULL DEFAULT '0',
  `resettalents_multiplier` int(11) unsigned NOT NULL DEFAULT '0',
  `resettalents_time` bigint(20) unsigned NOT NULL DEFAULT '0',
  `trans_x` float NOT NULL DEFAULT '0',
  `trans_y` float NOT NULL DEFAULT '0',
  `trans_z` float NOT NULL DEFAULT '0',
  `trans_o` float NOT NULL DEFAULT '0',
  `transguid` bigint(20) unsigned NOT NULL DEFAULT '0',
  `extra_flags` int(11) unsigned NOT NULL DEFAULT '0',
  `stable_slots` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `at_login` int(11) unsigned NOT NULL DEFAULT '0',
  `zone` int(11) unsigned NOT NULL DEFAULT '0',
  `death_expire_time` bigint(20) unsigned NOT NULL DEFAULT '0',
  `taxi_path` text,
  `honorRankPoints` float NOT NULL DEFAULT '0',
  `honorHighestRank` int(11) unsigned NOT NULL DEFAULT '0',
  `honorStanding` int(11) unsigned NOT NULL DEFAULT '0',
  `honorLastWeekHK` int(11) unsigned NOT NULL DEFAULT '0',
  `honorLastWeekCP` float NOT NULL DEFAULT '0',
  `honorStoredHK` int(11) NOT NULL DEFAULT '0',
  `honorStoredDK` int(11) NOT NULL DEFAULT '0',
  `watchedFaction` int(10) unsigned NOT NULL DEFAULT '0',
  `drunk` smallint(5) unsigned NOT NULL DEFAULT '0',
  `health` int(10) unsigned NOT NULL DEFAULT '0',
  `power1` int(10) unsigned NOT NULL DEFAULT '0',
  `power2` int(10) unsigned NOT NULL DEFAULT '0',
  `power3` int(10) unsigned NOT NULL DEFAULT '0',
  `power4` int(10) unsigned NOT NULL DEFAULT '0',
  `power5` int(10) unsigned NOT NULL DEFAULT '0',
  `exploredZones` longtext,
  `equipmentCache` longtext,
  `ammoId` int(10) unsigned NOT NULL DEFAULT '0',
  `actionBars` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `deleteInfos_Account` int(11) unsigned DEFAULT NULL,
  `deleteInfos_Name` varchar(12) DEFAULT NULL,
  `deleteDate` bigint(20) DEFAULT NULL,
  `area` int(11) unsigned NOT NULL DEFAULT '0',
  `world_phase_mask` int(11) DEFAULT '0',
  `customFlags` int(11) NOT NULL DEFAULT '0',
  `world_channel_joined` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `creation_time` bigint(20) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`),
  KEY `idx_account` (`account`),
  KEY `idx_online` (`online`),
  KEY `idx_name` (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.characters_guid_delete
CREATE TABLE IF NOT EXISTS `characters_guid_delete` (
  `guid` int(11) DEFAULT NULL,
  UNIQUE KEY `key_guid` (`guid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- Data exporting was unselected.

-- Dumping structure for table characters.characters_item_delete
CREATE TABLE IF NOT EXISTS `characters_item_delete` (
  `entry` int(11) DEFAULT NULL,
  UNIQUE KEY `key_entry` (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- Data exporting was unselected.

-- Dumping structure for table characters.corpse
CREATE TABLE IF NOT EXISTS `corpse` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `player` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Character Global Unique Identifier',
  `position_x` float NOT NULL DEFAULT '0',
  `position_y` float NOT NULL DEFAULT '0',
  `position_z` float NOT NULL DEFAULT '0',
  `orientation` float NOT NULL DEFAULT '0',
  `map` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Map Identifier',
  `time` bigint(20) unsigned NOT NULL DEFAULT '0',
  `corpse_type` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `instance` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`),
  KEY `idx_type` (`corpse_type`),
  KEY `idx_instance` (`instance`),
  KEY `idx_player` (`player`),
  KEY `idx_time` (`time`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Death System';

-- Data exporting was unselected.

-- Dumping structure for table characters.creature_respawn
CREATE TABLE IF NOT EXISTS `creature_respawn` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `respawntime` bigint(20) NOT NULL DEFAULT '0',
  `instance` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `map` int(5) unsigned DEFAULT '0',
  PRIMARY KEY (`guid`,`instance`),
  KEY `idx_instance` (`instance`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Grid Loading System';

-- Data exporting was unselected.

-- Dumping structure for table characters.game_event_status
CREATE TABLE IF NOT EXISTS `game_event_status` (
  `event` smallint(6) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`event`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Game event system';

-- Data exporting was unselected.

-- Dumping structure for table characters.gameobject_respawn
CREATE TABLE IF NOT EXISTS `gameobject_respawn` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `respawntime` bigint(20) NOT NULL DEFAULT '0',
  `instance` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `map` int(5) unsigned DEFAULT '0',
  PRIMARY KEY (`guid`,`instance`),
  KEY `idx_instance` (`instance`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Grid Loading System';

-- Data exporting was unselected.

-- Dumping structure for table characters.gm_subsurveys
CREATE TABLE IF NOT EXISTS `gm_subsurveys` (
  `surveyId` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `subsurveyId` int(10) unsigned NOT NULL DEFAULT '0',
  `rank` int(10) unsigned NOT NULL DEFAULT '0',
  `comment` text NOT NULL,
  PRIMARY KEY (`surveyId`,`subsurveyId`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.gm_surveys
CREATE TABLE IF NOT EXISTS `gm_surveys` (
  `surveyId` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `guid` int(10) unsigned NOT NULL DEFAULT '0',
  `mainSurvey` int(10) unsigned NOT NULL DEFAULT '0',
  `overallComment` longtext NOT NULL,
  `createTime` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`surveyId`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.gm_tickets
CREATE TABLE IF NOT EXISTS `gm_tickets` (
  `ticketId` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `guid` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier of ticket creator',
  `name` varchar(12) NOT NULL COMMENT 'Name of ticket creator',
  `message` text NOT NULL,
  `createTime` int(10) unsigned NOT NULL DEFAULT '0',
  `mapId` smallint(5) unsigned NOT NULL DEFAULT '0',
  `posX` float NOT NULL DEFAULT '0',
  `posY` float NOT NULL DEFAULT '0',
  `posZ` float NOT NULL DEFAULT '0',
  `lastModifiedTime` int(10) unsigned NOT NULL DEFAULT '0',
  `closedBy` int(10) NOT NULL DEFAULT '0',
  `assignedTo` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'GUID of admin to whom ticket is assigned',
  `comment` text NOT NULL,
  `response` text NOT NULL,
  `completed` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `escalated` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `viewed` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `haveTicket` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `ticketType` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `securityNeeded` tinyint(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`ticketId`)
) ENGINE=MyISAM AUTO_INCREMENT=12 DEFAULT CHARSET=utf8 COMMENT='Player System';

-- Data exporting was unselected.

-- Dumping structure for table characters.group_instance
CREATE TABLE IF NOT EXISTS `group_instance` (
  `leaderGuid` int(11) unsigned NOT NULL DEFAULT '0',
  `instance` int(11) unsigned NOT NULL DEFAULT '0',
  `permanent` tinyint(1) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`leaderGuid`,`instance`),
  KEY `idx_instance` (`instance`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.group_member
CREATE TABLE IF NOT EXISTS `group_member` (
  `groupId` int(11) unsigned NOT NULL,
  `memberGuid` int(11) unsigned NOT NULL,
  `assistant` tinyint(1) unsigned NOT NULL,
  `subgroup` smallint(6) unsigned NOT NULL,
  PRIMARY KEY (`groupId`,`memberGuid`),
  KEY `idx_memberGuid` (`memberGuid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Groups';

-- Data exporting was unselected.

-- Dumping structure for table characters.groups
CREATE TABLE IF NOT EXISTS `groups` (
  `groupId` int(11) unsigned NOT NULL,
  `leaderGuid` int(11) unsigned NOT NULL,
  `mainTank` int(11) unsigned NOT NULL,
  `mainAssistant` int(11) unsigned NOT NULL,
  `lootMethod` tinyint(4) unsigned NOT NULL,
  `looterGuid` int(11) unsigned NOT NULL,
  `lootThreshold` tinyint(4) unsigned NOT NULL,
  `icon1` int(11) unsigned NOT NULL,
  `icon2` int(11) unsigned NOT NULL,
  `icon3` int(11) unsigned NOT NULL,
  `icon4` int(11) unsigned NOT NULL,
  `icon5` int(11) unsigned NOT NULL,
  `icon6` int(11) unsigned NOT NULL,
  `icon7` int(11) unsigned NOT NULL,
  `icon8` int(11) unsigned NOT NULL,
  `isRaid` tinyint(1) unsigned NOT NULL,
  PRIMARY KEY (`groupId`),
  UNIQUE KEY `key_leaderGuid` (`leaderGuid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Groups';

-- Data exporting was unselected.

-- Dumping structure for table characters.guild
CREATE TABLE IF NOT EXISTS `guild` (
  `guildid` int(6) unsigned NOT NULL DEFAULT '0',
  `name` varchar(255) NOT NULL DEFAULT '',
  `leaderguid` int(6) unsigned NOT NULL DEFAULT '0',
  `EmblemStyle` int(5) NOT NULL DEFAULT '0',
  `EmblemColor` int(5) NOT NULL DEFAULT '0',
  `BorderStyle` int(5) NOT NULL DEFAULT '0',
  `BorderColor` int(5) NOT NULL DEFAULT '0',
  `BackgroundColor` int(5) NOT NULL DEFAULT '0',
  `info` text NOT NULL,
  `motd` varchar(255) NOT NULL DEFAULT '',
  `createdate` bigint(20) NOT NULL DEFAULT '0',
  PRIMARY KEY (`guildid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Guild System';

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_bank
CREATE TABLE IF NOT EXISTS `guild_bank` (
  `guildid` int(10) unsigned NOT NULL,
  `money` bigint(20) unsigned NOT NULL DEFAULT '0',
  `purchased_tabs` tinyint(3) unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`guildid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_bank_daily_usage
CREATE TABLE IF NOT EXISTS `guild_bank_daily_usage` (
  `guildid` int(10) unsigned NOT NULL,
  `player_guid` int(10) unsigned NOT NULL,
  `tabid` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `items_withdrawn` int(10) unsigned NOT NULL DEFAULT '0',
  `money_withdrawn` bigint(20) unsigned NOT NULL DEFAULT '0',
  `reset_day` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guildid`,`player_guid`,`tabid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_bank_item
CREATE TABLE IF NOT EXISTS `guild_bank_item` (
  `guildid` int(10) unsigned NOT NULL,
  `tabid` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `slot` tinyint(3) unsigned NOT NULL,
  `item_guid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`guildid`,`tabid`,`slot`),
  UNIQUE KEY `uq_guild_bank_item_guid` (`item_guid`),
  KEY `idx_guild_bank_guild` (`guildid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_bank_log
CREATE TABLE IF NOT EXISTS `guild_bank_log` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `guildid` int(10) unsigned NOT NULL,
  `player_guid` int(10) unsigned NOT NULL,
  `action` tinyint(3) unsigned NOT NULL,
  `tabid` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `slot` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `item_guid` int(10) unsigned NOT NULL DEFAULT '0',
  `item_entry` int(10) unsigned NOT NULL DEFAULT '0',
  `item_count` int(10) unsigned NOT NULL DEFAULT '0',
  `money` bigint(20) unsigned NOT NULL DEFAULT '0',
  `timestamp` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_guild_bank_log_guild` (`guildid`,`timestamp`)
) ENGINE=InnoDB AUTO_INCREMENT=68 DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_bank_money_right
CREATE TABLE IF NOT EXISTS `guild_bank_money_right` (
  `guildid` int(10) unsigned NOT NULL,
  `rankid` tinyint(3) unsigned NOT NULL,
  `can_deposit_money` tinyint(3) unsigned NOT NULL DEFAULT '1',
  `can_withdraw_money` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `daily_money_limit` bigint(20) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guildid`,`rankid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_bank_right
CREATE TABLE IF NOT EXISTS `guild_bank_right` (
  `guildid` int(10) unsigned NOT NULL,
  `rankid` tinyint(3) unsigned NOT NULL,
  `tabid` tinyint(3) unsigned NOT NULL,
  `can_view` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `can_deposit` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `can_withdraw` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `daily_withdraw_limit` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guildid`,`rankid`,`tabid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_bank_tab
CREATE TABLE IF NOT EXISTS `guild_bank_tab` (
  `guildid` int(10) unsigned NOT NULL,
  `tabid` tinyint(3) unsigned NOT NULL,
  `name` varchar(64) NOT NULL DEFAULT '',
  `icon` varchar(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`guildid`,`tabid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_eventlog
CREATE TABLE IF NOT EXISTS `guild_eventlog` (
  `guildid` int(11) NOT NULL COMMENT 'Guild Identificator',
  `LogGuid` int(11) NOT NULL COMMENT 'Log record identificator - auxiliary column',
  `EventType` tinyint(1) NOT NULL COMMENT 'Event type',
  `PlayerGuid1` int(11) NOT NULL COMMENT 'Player 1',
  `PlayerGuid2` int(11) NOT NULL COMMENT 'Player 2',
  `NewRank` tinyint(2) NOT NULL COMMENT 'New rank(in case promotion/demotion)',
  `TimeStamp` bigint(20) NOT NULL COMMENT 'Event UNIX time',
  PRIMARY KEY (`guildid`,`LogGuid`),
  KEY `idx_PlayerGuid1` (`PlayerGuid1`),
  KEY `idx_PlayerGuid2` (`PlayerGuid2`),
  KEY `idx_LogGuid` (`LogGuid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Guild Eventlog';

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_housing_objects
CREATE TABLE IF NOT EXISTS `guild_housing_objects` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `gameobject_guid` int(10) unsigned NOT NULL,
  `guildid` int(10) unsigned NOT NULL,
  `plot_id` int(10) unsigned NOT NULL DEFAULT '1',
  `gameobject_entry` mediumint(8) unsigned NOT NULL,
  `map` mediumint(8) unsigned NOT NULL,
  `position_x` float NOT NULL,
  `position_y` float NOT NULL,
  `position_z` float NOT NULL,
  `orientation` float NOT NULL,
  `rotation0` float NOT NULL DEFAULT '0',
  `rotation1` float NOT NULL DEFAULT '0',
  `rotation2` float NOT NULL DEFAULT '0',
  `rotation3` float NOT NULL DEFAULT '1',
  `scale` float NOT NULL DEFAULT '1',
  `placed_by` int(10) unsigned NOT NULL,
  `placed_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_gameobject_guid` (`gameobject_guid`),
  KEY `idx_guild_plot` (`guildid`,`plot_id`),
  KEY `idx_map` (`map`)
) ENGINE=InnoDB AUTO_INCREMENT=19 DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_member
CREATE TABLE IF NOT EXISTS `guild_member` (
  `guildid` int(6) unsigned NOT NULL DEFAULT '0',
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `rank` tinyint(2) unsigned NOT NULL DEFAULT '0',
  `pnote` varchar(255) NOT NULL DEFAULT '',
  `offnote` varchar(255) NOT NULL DEFAULT '',
  UNIQUE KEY `key_guid` (`guid`),
  KEY `idx_guildid` (`guildid`),
  KEY `idx_guildid_rank` (`guildid`,`rank`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Guild System';

-- Data exporting was unselected.

-- Dumping structure for table characters.guild_rank
CREATE TABLE IF NOT EXISTS `guild_rank` (
  `guildid` int(6) unsigned NOT NULL DEFAULT '0',
  `rid` int(11) unsigned NOT NULL,
  `rname` varchar(255) NOT NULL DEFAULT '',
  `rights` int(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guildid`,`rid`),
  KEY `idx_rid` (`rid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Guild System';

-- Data exporting was unselected.

-- Dumping structure for table characters.instance
CREATE TABLE IF NOT EXISTS `instance` (
  `id` int(11) unsigned NOT NULL DEFAULT '0',
  `map` int(11) unsigned NOT NULL DEFAULT '0',
  `resettime` bigint(40) NOT NULL DEFAULT '0',
  `data` longtext,
  PRIMARY KEY (`id`),
  KEY `idx_map` (`map`),
  KEY `idx_resettime` (`resettime`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.instance_reset
CREATE TABLE IF NOT EXISTS `instance_reset` (
  `mapid` int(11) unsigned NOT NULL DEFAULT '0',
  `resettime` bigint(40) NOT NULL DEFAULT '0',
  PRIMARY KEY (`mapid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.item_instance
CREATE TABLE IF NOT EXISTS `item_instance` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0',
  `itemEntry` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `owner_guid` int(10) unsigned NOT NULL DEFAULT '0',
  `creatorGuid` int(10) unsigned NOT NULL DEFAULT '0',
  `giftCreatorGuid` int(10) unsigned NOT NULL DEFAULT '0',
  `count` int(10) unsigned NOT NULL DEFAULT '1',
  `duration` int(10) NOT NULL DEFAULT '0',
  `charges` tinytext,
  `flags` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `enchantments` text NOT NULL,
  `randomPropertyId` smallint(5) NOT NULL DEFAULT '0',
  `durability` smallint(5) unsigned NOT NULL DEFAULT '0',
  `text` int(10) unsigned NOT NULL DEFAULT '0',
  `generated_loot` tinyint(4) DEFAULT '0',
  PRIMARY KEY (`guid`),
  KEY `idx_owner_guid` (`owner_guid`),
  KEY `idx_itemEntry` (`itemEntry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Item System';

-- Data exporting was unselected.

-- Dumping structure for table characters.item_loot
CREATE TABLE IF NOT EXISTS `item_loot` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `owner_guid` int(11) unsigned NOT NULL DEFAULT '0',
  `itemid` int(11) unsigned NOT NULL DEFAULT '0',
  `amount` int(11) unsigned NOT NULL DEFAULT '0',
  `property` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`itemid`),
  KEY `idx_owner_guid` (`owner_guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Item System';

-- Data exporting was unselected.

-- Dumping structure for table characters.item_text
CREATE TABLE IF NOT EXISTS `item_text` (
  `id` int(11) unsigned NOT NULL DEFAULT '0',
  `text` longtext,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Item System';

-- Data exporting was unselected.

-- Dumping structure for table characters.mail
CREATE TABLE IF NOT EXISTS `mail` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
  `messageType` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `stationery` tinyint(3) NOT NULL DEFAULT '41',
  `mailTemplateId` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `sender` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Character Global Unique Identifier',
  `receiver` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Character Global Unique Identifier',
  `subject` longtext,
  `itemTextId` int(11) unsigned NOT NULL DEFAULT '0',
  `has_items` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `expire_time` bigint(40) NOT NULL DEFAULT '0',
  `deliver_time` bigint(40) NOT NULL DEFAULT '0',
  `money` int(11) unsigned NOT NULL DEFAULT '0',
  `cod` int(11) unsigned NOT NULL DEFAULT '0',
  `checked` tinyint(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_receiver` (`receiver`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Mail System';

-- Data exporting was unselected.

-- Dumping structure for table characters.mail_items
CREATE TABLE IF NOT EXISTS `mail_items` (
  `mail_id` int(11) unsigned NOT NULL DEFAULT '0',
  `item_guid` int(11) unsigned NOT NULL DEFAULT '0',
  `item_template` int(11) unsigned NOT NULL DEFAULT '0',
  `receiver` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Character Global Unique Identifier',
  PRIMARY KEY (`mail_id`,`item_guid`),
  KEY `idx_receiver` (`receiver`),
  KEY `idx_item_guid` (`item_guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

-- Data exporting was unselected.

-- Dumping structure for table characters.migrations
CREATE TABLE IF NOT EXISTS `migrations` (
  `id` varchar(255) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.pet_aura
CREATE TABLE IF NOT EXISTS `pet_aura` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `caster_guid` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT 'Full Global Unique Identifier',
  `item_guid` int(11) unsigned NOT NULL DEFAULT '0',
  `spell` int(11) unsigned NOT NULL DEFAULT '0',
  `stackcount` int(11) unsigned NOT NULL DEFAULT '1',
  `remaincharges` int(11) unsigned NOT NULL DEFAULT '0',
  `basepoints0` int(11) NOT NULL DEFAULT '0',
  `basepoints1` int(11) NOT NULL DEFAULT '0',
  `basepoints2` int(11) NOT NULL DEFAULT '0',
  `periodictime0` int(11) unsigned NOT NULL DEFAULT '0',
  `periodictime1` int(11) unsigned NOT NULL DEFAULT '0',
  `periodictime2` int(11) unsigned NOT NULL DEFAULT '0',
  `maxduration` int(11) NOT NULL DEFAULT '0',
  `remaintime` int(11) NOT NULL DEFAULT '0',
  `effIndexMask` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`caster_guid`,`item_guid`,`spell`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Pet System';

-- Data exporting was unselected.

-- Dumping structure for table characters.pet_spell
CREATE TABLE IF NOT EXISTS `pet_spell` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `spell` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Spell Identifier',
  `active` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`spell`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Pet System';

-- Data exporting was unselected.

-- Dumping structure for table characters.pet_spell_cooldown
CREATE TABLE IF NOT EXISTS `pet_spell_cooldown` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier, Low part',
  `spell` int(11) unsigned NOT NULL DEFAULT '0' COMMENT 'Spell Identifier',
  `time` bigint(20) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`spell`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.petition
CREATE TABLE IF NOT EXISTS `petition` (
  `ownerguid` int(10) unsigned NOT NULL,
  `petitionguid` int(10) unsigned DEFAULT '0',
  `charterguid` int(10) unsigned DEFAULT NULL COMMENT 'Charter item GUID',
  `name` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`ownerguid`),
  UNIQUE KEY `key_ownerguid_petitionguid` (`ownerguid`,`petitionguid`),
  UNIQUE KEY `charterguid` (`charterguid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Guild System';

-- Data exporting was unselected.

-- Dumping structure for table characters.petition_sign
CREATE TABLE IF NOT EXISTS `petition_sign` (
  `ownerguid` int(10) unsigned NOT NULL,
  `petitionguid` int(11) unsigned NOT NULL DEFAULT '0',
  `playerguid` int(11) unsigned NOT NULL DEFAULT '0',
  `player_account` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`petitionguid`,`playerguid`),
  KEY `idx_playerguid` (`playerguid`),
  KEY `idx_ownerguid` (`ownerguid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Guild System';

-- Data exporting was unselected.

-- Dumping structure for table characters.playerbot
CREATE TABLE IF NOT EXISTS `playerbot` (
  `char_guid` bigint(20) unsigned NOT NULL,
  `chance` int(10) unsigned NOT NULL DEFAULT '10',
  `comment` varchar(255) DEFAULT NULL,
  `ai` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`char_guid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- Data exporting was unselected.

-- Dumping structure for table characters.rmt_chat_log
CREATE TABLE IF NOT EXISTS `rmt_chat_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `flag_id` int(10) unsigned NOT NULL,
  `guid` int(10) unsigned NOT NULL,
  `account` int(10) unsigned NOT NULL,
  `player_name` varchar(12) COLLATE utf8_unicode_ci NOT NULL,
  `chat_type` varchar(20) COLLATE utf8_unicode_ci NOT NULL,
  `target_name` varchar(12) COLLATE utf8_unicode_ci DEFAULT NULL,
  `message` text COLLATE utf8_unicode_ci NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_flag_id` (`flag_id`),
  KEY `idx_guid` (`guid`),
  KEY `idx_account` (`account`),
  KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.rmt_flag_log
CREATE TABLE IF NOT EXISTS `rmt_flag_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `account` int(10) unsigned NOT NULL,
  `guid` int(10) unsigned NOT NULL,
  `player_name` varchar(12) COLLATE utf8_unicode_ci NOT NULL,
  `score` int(10) unsigned NOT NULL DEFAULT '0',
  `reason` text COLLATE utf8_unicode_ci NOT NULL,
  `related_account` int(10) unsigned DEFAULT NULL,
  `related_guid` int(10) unsigned DEFAULT NULL,
  `related_player` varchar(12) COLLATE utf8_unicode_ci DEFAULT NULL,
  `source_type` varchar(20) COLLATE utf8_unicode_ci NOT NULL,
  `gold_amount` int(10) unsigned DEFAULT '0',
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_guid` (`guid`),
  KEY `idx_account` (`account`),
  KEY `idx_related_guid` (`related_guid`),
  KEY `idx_score` (`score`),
  KEY `idx_source_type` (`source_type`),
  KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.rmt_mail_log
CREATE TABLE IF NOT EXISTS `rmt_mail_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `sender_guid` int(10) unsigned NOT NULL,
  `sender_account` int(10) unsigned NOT NULL,
  `sender_name` varchar(12) COLLATE utf8_unicode_ci NOT NULL,
  `receiver_guid` int(10) unsigned NOT NULL,
  `receiver_account` int(10) unsigned NOT NULL,
  `receiver_name` varchar(12) COLLATE utf8_unicode_ci NOT NULL,
  `gold_amount` int(10) unsigned NOT NULL,
  `cod_amount` int(10) unsigned NOT NULL DEFAULT '0',
  `subject` varchar(255) COLLATE utf8_unicode_ci DEFAULT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_sender_guid` (`sender_guid`),
  KEY `idx_receiver_guid` (`receiver_guid`),
  KEY `idx_sender_account` (`sender_account`),
  KEY `idx_receiver_account` (`receiver_account`),
  KEY `idx_gold_amount` (`gold_amount`),
  KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.rmt_trade_log
CREATE TABLE IF NOT EXISTS `rmt_trade_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `sender_guid` int(10) unsigned NOT NULL,
  `sender_account` int(10) unsigned NOT NULL,
  `sender_name` varchar(12) COLLATE utf8_unicode_ci NOT NULL,
  `receiver_guid` int(10) unsigned NOT NULL,
  `receiver_account` int(10) unsigned NOT NULL,
  `receiver_name` varchar(12) COLLATE utf8_unicode_ci NOT NULL,
  `gold_amount` int(10) unsigned NOT NULL,
  `map` int(10) unsigned NOT NULL,
  `x` float NOT NULL,
  `y` float NOT NULL,
  `z` float NOT NULL,
  `note` text COLLATE utf8_unicode_ci,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_sender_guid` (`sender_guid`),
  KEY `idx_receiver_guid` (`receiver_guid`),
  KEY `idx_created_at` (`created_at`),
  KEY `idx_gold_amount` (`gold_amount`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.saved_variables
CREATE TABLE IF NOT EXISTS `saved_variables` (
  `key` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `cleaning_flags` int(11) unsigned NOT NULL DEFAULT '0',
  `lastHonorMaintenanceDay` int(11) unsigned NOT NULL DEFAULT '0',
  `nextHonorMaintenanceDay` int(11) unsigned NOT NULL DEFAULT '0',
  `honorMaintenanceMarker` tinyint(1) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`key`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Variable Saves';

-- Data exporting was unselected.

-- Dumping structure for table characters.server_firsts
CREATE TABLE IF NOT EXISTS `server_firsts` (
  `category` varchar(32) COLLATE utf8_unicode_ci NOT NULL,
  `guid` int(10) unsigned NOT NULL,
  `name` varchar(12) COLLATE utf8_unicode_ci NOT NULL,
  `achieved` int(10) unsigned NOT NULL,
  PRIMARY KEY (`category`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.

-- Dumping structure for table characters.website_account_deletions
CREATE TABLE IF NOT EXISTS `website_account_deletions` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int(10) unsigned NOT NULL,
  `email` varchar(255) NOT NULL,
  `token` char(64) NOT NULL,
  `created_at` datetime NOT NULL,
  `expires_at` datetime NOT NULL,
  `used_at` datetime DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_account_deletion_token` (`token`),
  KEY `idx_account_deletion_account` (`account_id`),
  KEY `idx_account_deletion_expires` (`expires_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.world
CREATE TABLE IF NOT EXISTS `world` (
  `map` int(11) unsigned NOT NULL DEFAULT '0',
  `data` longtext,
  PRIMARY KEY (`map`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table characters.worldstates
CREATE TABLE IF NOT EXISTS `worldstates` (
  `entry` int(11) DEFAULT NULL,
  `value` int(11) DEFAULT NULL,
  `comment` varchar(255) DEFAULT NULL,
  UNIQUE KEY `key_entry` (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- Data exporting was unselected.

/*!40103 SET TIME_ZONE=IFNULL(@OLD_TIME_ZONE, 'system') */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IFNULL(@OLD_FOREIGN_KEY_CHECKS, 1) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=IFNULL(@OLD_SQL_NOTES, 1) */;
