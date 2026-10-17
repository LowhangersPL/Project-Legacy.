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


-- Dumping database structure for realmd
CREATE DATABASE IF NOT EXISTS `realmd` /*!40100 DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci */;
USE `realmd`;

-- Dumping structure for table realmd.account
CREATE TABLE IF NOT EXISTS `account` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
  `username` varchar(32) NOT NULL,
  `sha_pass_hash` varchar(40) NOT NULL,
  `gmlevel` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `sessionkey` longtext,
  `v` longtext,
  `s` longtext,
  `reg_mail` varchar(255) NOT NULL DEFAULT '',
  `token_key` varchar(100) NOT NULL DEFAULT '',
  `email` text,
  `joindate` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `last_ip` varchar(30) NOT NULL DEFAULT '0.0.0.0',
  `last_attempt_ip` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `last_local_ip` varchar(30) NOT NULL DEFAULT '127.0.0.1',
  `failed_logins` int(11) unsigned NOT NULL DEFAULT '0',
  `locked` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `lock_country` varchar(2) NOT NULL DEFAULT '00',
  `last_login` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `last_pwd_reset` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `online` tinyint(4) NOT NULL DEFAULT '0',
  `expansion` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `mutetime` bigint(40) NOT NULL DEFAULT '0',
  `mutereason` varchar(255) NOT NULL DEFAULT '',
  `muteby` varchar(50) NOT NULL DEFAULT '',
  `locale` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `os` varchar(4) NOT NULL DEFAULT '',
  `recruiter` int(11) NOT NULL DEFAULT '0',
  `current_realm` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `banned` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `mail_verif` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `remember_token` varchar(100) NOT NULL DEFAULT '',
  `flags` int(10) unsigned NOT NULL DEFAULT '0',
  `security` varchar(255) DEFAULT NULL,
  `pass_verif` varchar(255) DEFAULT NULL COMMENT 'Web recover password',
  `email_verif` tinyint(1) NOT NULL DEFAULT '0' COMMENT 'Email verification',
  `email_check` varchar(255) DEFAULT NULL,
  `nostalrius_token` varchar(255) DEFAULT NULL,
  `nostalrius_token_enabled` tinyint(1) NOT NULL DEFAULT '0',
  `nostalrius_email` text,
  `nostalrius_reason` text,
  `geolock_pin` int(11) DEFAULT '0',
  `beta_access` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `launcher_verified_at` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_username` (`username`),
  KEY `idx_gmlevel` (`gmlevel`)
) ENGINE=InnoDB AUTO_INCREMENT=20 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Account System';

-- Data exporting was unselected.

-- Dumping structure for table realmd.account_access
CREATE TABLE IF NOT EXISTS `account_access` (
  `id` int(11) unsigned NOT NULL,
  `gmlevel` tinyint(3) unsigned NOT NULL,
  `RealmID` int(11) NOT NULL,
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

-- Data exporting was unselected.

-- Dumping structure for table realmd.account_banned
CREATE TABLE IF NOT EXISTS `account_banned` (
  `banid` bigint(20) NOT NULL AUTO_INCREMENT,
  `id` bigint(20) NOT NULL DEFAULT '0' COMMENT 'Account id',
  `bandate` bigint(40) NOT NULL DEFAULT '0',
  `unbandate` bigint(40) NOT NULL DEFAULT '0',
  `bannedby` varchar(50) NOT NULL,
  `banreason` varchar(255) NOT NULL,
  `active` tinyint(4) NOT NULL DEFAULT '1',
  `realm` tinyint(4) NOT NULL DEFAULT '1',
  `gmlevel` tinyint(4) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`,`bandate`),
  UNIQUE KEY `banid` (`banid`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Ban List';

-- Data exporting was unselected.

-- Dumping structure for table realmd.antispam_blacklist
CREATE TABLE IF NOT EXISTS `antispam_blacklist` (
  `word` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`word`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table realmd.antispam_detected
CREATE TABLE IF NOT EXISTS `antispam_detected` (
  `id` bigint(20) unsigned NOT NULL DEFAULT '0',
  `detectScore` mediumint(5) NOT NULL DEFAULT '0',
  `detectTime` bigint(20) unsigned NOT NULL DEFAULT '0',
  `unmuteTime` bigint(20) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table realmd.antispam_replacement
CREATE TABLE IF NOT EXISTS `antispam_replacement` (
  `from` varchar(32) NOT NULL DEFAULT '',
  `to` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`from`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table realmd.antispam_scores
CREATE TABLE IF NOT EXISTS `antispam_scores` (
  `word` varchar(32) NOT NULL DEFAULT '',
  `score` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(1) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`word`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table realmd.antispam_unicode
CREATE TABLE IF NOT EXISTS `antispam_unicode` (
  `from` mediumint(5) unsigned NOT NULL DEFAULT '0',
  `to` mediumint(5) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`from`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for event realmd.DetectUpdate
DELIMITER //
CREATE EVENT `DetectUpdate` ON SCHEDULE EVERY 1 MINUTE STARTS '2016-12-31 22:00:00' ON COMPLETION PRESERVE ENABLE DO BEGIN

        SELECT @time:= UNIX_TIMESTAMP();

        UPDATE `antispam_detected` SET `unmuteTime` = 0 WHERE @time > `unmuteTime`;

	    UPDATE `antispam_detected` SET `detectScore` = `detectScore` - 1 WHERE `detectScore` < 3 AND (@time - `detectTime`) > 7200;

	    DELETE FROM `antispam_detected` WHERE `detectScore` <= 0 AND (@time - `detectTime`) > 7200;

	END//
DELIMITER ;

-- Dumping structure for table realmd.geoip
CREATE TABLE IF NOT EXISTS `geoip` (
  `network_start_integer` int(11) DEFAULT NULL,
  `network_last_integer` int(11) DEFAULT NULL,
  `geoname_id` text,
  `registered_country_geoname_id` text,
  `represented_country_geoname_id` text,
  `is_anonymous_proxy` int(11) DEFAULT NULL,
  `is_satellite_provider` int(11) DEFAULT NULL,
  `postal_code` text,
  `latitude` double DEFAULT NULL,
  `longitude` double DEFAULT NULL,
  `accuracy_radius` int(11) DEFAULT NULL,
  KEY `ip_start` (`network_start_integer`),
  KEY `ip_end` (`network_last_integer`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table realmd.ip_banned
CREATE TABLE IF NOT EXISTS `ip_banned` (
  `ip` varchar(32) NOT NULL DEFAULT '0.0.0.0',
  `bandate` int(11) NOT NULL,
  `unbandate` int(11) NOT NULL,
  `bannedby` varchar(50) NOT NULL DEFAULT '[Console]',
  `banreason` varchar(50) NOT NULL DEFAULT 'no reason',
  PRIMARY KEY (`ip`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Banned IPs';

-- Data exporting was unselected.

-- Dumping structure for table realmd.ip2nation
CREATE TABLE IF NOT EXISTS `ip2nation` (
  `ip` int(11) unsigned NOT NULL DEFAULT '0',
  `country` char(2) NOT NULL DEFAULT '',
  KEY `ip` (`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Data exporting was unselected.

-- Dumping structure for table realmd.ip2nationcountries
CREATE TABLE IF NOT EXISTS `ip2nationcountries` (
  `code` varchar(4) NOT NULL DEFAULT '',
  `iso_code_2` varchar(2) NOT NULL DEFAULT '',
  `iso_code_3` varchar(3) DEFAULT '',
  `iso_country` varchar(255) NOT NULL DEFAULT '',
  `country` varchar(255) NOT NULL DEFAULT '',
  `lat` float NOT NULL DEFAULT '0',
  `lon` float NOT NULL DEFAULT '0',
  PRIMARY KEY (`code`),
  KEY `code` (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Data exporting was unselected.

-- Dumping structure for table realmd.migrations
CREATE TABLE IF NOT EXISTS `migrations` (
  `id` varchar(255) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table realmd.realmcharacters
CREATE TABLE IF NOT EXISTS `realmcharacters` (
  `realmid` int(11) unsigned NOT NULL DEFAULT '0',
  `acctid` bigint(20) unsigned NOT NULL,
  `numchars` tinyint(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`realmid`,`acctid`),
  KEY `acctid` (`acctid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Realm Character Tracker';

-- Data exporting was unselected.

-- Dumping structure for table realmd.realmlist
CREATE TABLE IF NOT EXISTS `realmlist` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(32) NOT NULL DEFAULT '',
  `address` varchar(32) NOT NULL DEFAULT '127.0.0.1',
  `localAddress` varchar(255) NOT NULL DEFAULT '127.0.0.1',
  `localSubnetMask` varchar(255) NOT NULL DEFAULT '255.255.255.0',
  `port` int(11) NOT NULL DEFAULT '8085',
  `icon` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `realmflags` tinyint(3) unsigned NOT NULL DEFAULT '2',
  `timezone` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `allowedSecurityLevel` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `population` float unsigned NOT NULL DEFAULT '0',
  `gamebuild_min` int(11) unsigned NOT NULL DEFAULT '0',
  `gamebuild_max` int(11) unsigned NOT NULL DEFAULT '0',
  `flag` tinyint(3) unsigned NOT NULL DEFAULT '2',
  `realmbuilds` varchar(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_name` (`name`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Realm System';

-- Data exporting was unselected.

-- Dumping structure for table realmd.uptime
CREATE TABLE IF NOT EXISTS `uptime` (
  `realmid` int(11) unsigned NOT NULL,
  `starttime` bigint(20) unsigned NOT NULL DEFAULT '0',
  `startstring` varchar(64) NOT NULL DEFAULT '',
  `uptime` bigint(20) unsigned NOT NULL DEFAULT '0',
  `onlineplayers` smallint(5) unsigned NOT NULL DEFAULT '0',
  `maxplayers` smallint(5) unsigned NOT NULL DEFAULT '0',
  `revision` varchar(255) NOT NULL DEFAULT 'Trinitycore',
  PRIMARY KEY (`realmid`,`starttime`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Uptime system';

-- Data exporting was unselected.

-- Dumping structure for table realmd.website_pending_registrations
CREATE TABLE IF NOT EXISTS `website_pending_registrations` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `username` varchar(32) NOT NULL,
  `email` varchar(255) NOT NULL,
  `sha_pass_hash` varchar(40) NOT NULL,
  `token` char(64) NOT NULL,
  `beta_access` tinyint(1) NOT NULL DEFAULT '0',
  `created_at` datetime NOT NULL,
  `expires_at` datetime NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_pending_username` (`username`),
  UNIQUE KEY `uq_pending_email` (`email`),
  UNIQUE KEY `uq_pending_token` (`token`),
  KEY `idx_pending_expires` (`expires_at`)
) ENGINE=InnoDB AUTO_INCREMENT=11 DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

-- Dumping structure for table realmd.website_settings
CREATE TABLE IF NOT EXISTS `website_settings` (
  `setting_name` varchar(64) NOT NULL,
  `setting_value` varchar(255) NOT NULL,
  `updated_at` datetime NOT NULL,
  PRIMARY KEY (`setting_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.

/*!40103 SET TIME_ZONE=IFNULL(@OLD_TIME_ZONE, 'system') */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IFNULL(@OLD_FOREIGN_KEY_CHECKS, 1) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=IFNULL(@OLD_SQL_NOTES, 1) */;
