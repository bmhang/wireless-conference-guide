```
CREATE TABLE `COR` (
  `ts` bigint(20) NOT NULL,
  `sniffer_mac` varchar(18) NOT NULL,
  `freq` smallint(9) NOT NULL,
  `COR` double NOT NULL,
  `intval` mediumint(9) NOT NULL,
  PRIMARY KEY (`sniffer_mac`,`ts`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `COR_ZIG` (
  `ts` bigint(20) NOT NULL,
  `sniffer_mac` varchar(18) NOT NULL,
  `freq` smallint(9) NOT NULL,
  `COR_Zig1` double NOT NULL,
  `COR_Zig2` double NOT NULL,
  `COR_Zig3` double NOT NULL,
  `COR_Zig4` double NOT NULL,
  `intval` mediumint(9) NOT NULL,
  PRIMARY KEY (`sniffer_mac`,`ts`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `COT` (
  `ts` varchar(20) NOT NULL,
  `freq` mediumint(9) NOT NULL,
  `FCS` varchar(10) NOT NULL,
  `COT` varchar(20) NOT NULL,
  PRIMARY KEY (`ts`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `MOS` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `ts` bigint(20) NOT NULL,
  `src_mac` varchar(18) NOT NULL,
  `sniffer_mac` varchar(18) NOT NULL,
  `MOS` double NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `PL_JT_LAT_MOS` (
  `ts` bigint(20) NOT NULL,
  `src_IP\mac` varchar(18) NOT NULL,
  `sniffer_mac` varchar(18) NOT NULL,
  `packetLoss` double NOT NULL,
  `jitter` double NOT NULL,
  `latency` double NOT NULL,
  `MOS` double NOT NULL,
  PRIMARY KEY (`ts`,`sniffer_mac`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `RM` (
  `ts` bigint(20) NOT NULL,
  `client_MAC` varchar(18) NOT NULL,
  `SSID` varchar(38) NOT NULL,
  `AP_MAC` varchar(18) NOT NULL,
  `freq` smallint(9) NOT NULL,
  `auth_method` varchar(18) NOT NULL,
  `last_state` varchar(22) NOT NULL,
  `SCAN_time` varchar(10) NOT NULL,
  `AUTHN_AP_time` varchar(10) NOT NULL,
  `ASSN_AP_time` varchar(10) NOT NULL,
  `AUTHN_RADIUS_time` varchar(10) NOT NULL,
  `KEY_DISTR_time` varchar(10) NOT NULL,
  `AUTHN_AP_count` varchar(5) NOT NULL,
  `ASSN_AP_count` varchar(5) NOT NULL,
  `AUTHN_RADIUS_count` varchar(5) NOT NULL,
  `KEY_DISTR_count` varchar(5) NOT NULL,
  `status` varchar(10) NOT NULL,
  PRIMARY KEY (`ts`,`client_MAC`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `SV` (
  `ts` bigint(20) NOT NULL,
  `bssid` varchar(18) NOT NULL,
  `station` varchar(18) NOT NULL,
  `rx_bytes` int(10) unsigned NOT NULL,
  `rx_packets` int(10) unsigned NOT NULL,
  `tx_bytes` int(10) unsigned NOT NULL,
  `tx_packets` int(10) unsigned NOT NULL,
  `tx_retries` int(10) unsigned NOT NULL,
  `tx_failed` int(10) unsigned NOT NULL,
  `signal` int(10) NOT NULL,
  `tx_bitrate` float NOT NULL,
  PRIMARY KEY (`ts`,`station`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `USRP_COR` (
  `ts` bigint(20) NOT NULL,
  `LOC` mediumint(9) NOT NULL,
  `COR` float NOT NULL,
  PRIMARY KEY (`ts`,`LOC`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `allPar` (
  `id` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `ts` varchar(20) NOT NULL,
  `deviceID` int(11) NOT NULL,
  `preamble` varchar(5) NOT NULL,
  `rate` float unsigned NOT NULL,
  `freq` mediumint(9) NOT NULL,
  `bits_per_channel` smallint(6) NOT NULL,
  `channel_type` varchar(6) NOT NULL,
  `RSSI` smallint(8) NOT NULL,
  `frameType` varchar(5) NOT NULL,
  `frameSubType` varchar(30) NOT NULL,
  `src_mac` varchar(20) NOT NULL,
  `FCS` varchar(10) NOT NULL,
  `frameLength` smallint(8) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `applicationTable` (
  `platform` varchar(20) DEFAULT NULL,
  `application` varchar(15) DEFAULT NULL,
  `version` varchar(10) NOT NULL,
  `description` varchar(500) NOT NULL,
  `inputFormat` varchar(1024) NOT NULL,
  `outputFormat` varchar(2048) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `bt_sniffer` (
  `ts` bigint(20) NOT NULL,
  `warp_id` int(11) NOT NULL,
  `PSD0` float NOT NULL,
  `PSD1` float NOT NULL,
  `PSD2` float NOT NULL,
  `PSD3` float NOT NULL,
  `PSD4` float NOT NULL,
  `PSD5` float NOT NULL,
  `PSD6` float NOT NULL,
  `PSD7` float NOT NULL,
  `PSD8` float NOT NULL,
  `PSD9` float NOT NULL,
  `PSD10` float NOT NULL,
  `PSD11` float NOT NULL,
  `PSD12` float NOT NULL,
  `PSD13` float NOT NULL,
  `PSD14` float NOT NULL,
  `PSD15` float NOT NULL,
  `PSD16` float NOT NULL,
  `PSD17` float NOT NULL,
  `PSD18` float NOT NULL,
  `PSD19` float NOT NULL,
  `PSD20` float NOT NULL,
  `PSD21` float NOT NULL,
  `PSD22` float NOT NULL,
  `PSD23` float NOT NULL,
  `PSD24` float NOT NULL,
  `PSD25` float NOT NULL,
  `PSD26` float NOT NULL,
  `PSD27` float NOT NULL,
  `PSD28` float NOT NULL,
  `PSD29` float NOT NULL,
  `PSD30` float NOT NULL,
  `PSD31` float NOT NULL,
  `PSD32` float NOT NULL,
  `PSD33` float NOT NULL,
  `PSD34` float NOT NULL,
  `PSD35` float NOT NULL,
  `PSD36` float NOT NULL,
  `PSD37` float NOT NULL,
  `PSD38` float NOT NULL,
  `PSD39` float NOT NULL,
  `PSD40` float NOT NULL,
  `PSD41` float NOT NULL,
  `PSD42` float NOT NULL,
  `PSD43` float NOT NULL,
  `PSD44` float NOT NULL,
  `PSD45` float NOT NULL,
  `PSD46` float NOT NULL,
  `PSD47` float NOT NULL,
  `PSD48` float NOT NULL,
  `PSD49` float NOT NULL,
  `PSD50` float NOT NULL,
  `PSD51` float NOT NULL,
  `PSD52` float NOT NULL,
  `PSD53` float NOT NULL,
  `PSD54` float NOT NULL,
  `PSD55` float NOT NULL,
  `PSD56` float NOT NULL,
  `PSD57` float NOT NULL,
  `PSD58` float NOT NULL,
  `PSD59` float NOT NULL,
  `PSD60` float NOT NULL,
  `PSD61` float NOT NULL,
  `PSD62` float NOT NULL,
  `PSD63` float NOT NULL,
  `PSD64` float NOT NULL,
  `PSD65` float NOT NULL,
  `PSD66` float NOT NULL,
  `PSD67` float NOT NULL,
  `PSD68` float NOT NULL,
  `PSD69` float NOT NULL,
  `PSD70` float NOT NULL,
  `PSD71` float NOT NULL,
  `PSD72` float NOT NULL,
  `PSD73` float NOT NULL,
  `PSD74` float NOT NULL,
  `PSD75` float NOT NULL,
  `PSD76` float NOT NULL,
  `PSD77` float NOT NULL,
  `PSD78` float NOT NULL,
  PRIMARY KEY (`ts`,`warp_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `factual_qos` (
  `qos_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `link_id` int(11) unsigned NOT NULL,
  `ts` varchar(20) NOT NULL,
  `app_id` tinyint(4) unsigned NOT NULL,
  `type` varchar(20) NOT NULL,
  `packetCount` smallint(6) NOT NULL,
  `MOS` varchar(20) NOT NULL,
  PRIMARY KEY (`qos_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `hardConfigTable` (
  `node` varchar(10) NOT NULL,
  `interface` varchar(20) NOT NULL,
  `mode` varchar(30) NOT NULL,
  `channel` varchar(128) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `imageTable` (
  `platform` varchar(20) NOT NULL,
  `image` varchar(40) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `link_desc` (
  `link_id` int(10) NOT NULL AUTO_INCREMENT,
  `TxNode` varchar(16) DEFAULT NULL,
  `RxNode` varchar(16) DEFAULT NULL,
  `technology` varchar(10) DEFAULT NULL,
  `freq` int(10) DEFAULT NULL,
  PRIMARY KEY (`link_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `nodeInfoTable` (
  `node` varchar(5) NOT NULL,
  `xPos` int(11) NOT NULL,
  `yPos` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `ping_stat` (
  `ts` bigint(20) NOT NULL,
  `hostIP` varchar(18) NOT NULL,
  `rtt` float NOT NULL,
  PRIMARY KEY (`ts`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

REATE TABLE `platformTable` (
  `platform` varchar(20) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `wifi_sniffer` (
  `ts` bigint(20) NOT NULL,
  `sniffer_mac` varchar(18) NOT NULL,
  `rate` float unsigned NOT NULL,
  `freq` mediumint(9) NOT NULL,
  `SSI` smallint(8) NOT NULL,
  `frame_type` tinyint(4) NOT NULL,
  `frame_subtype` tinyint(4) NOT NULL,
  `flag` smallint(6) NOT NULL,
  `dst_mac` varchar(18) NOT NULL,
  `src_mac` varchar(18) NOT NULL,
  `frag_no` smallint(6) NOT NULL,
  `seq_no` mediumint(9) NOT NULL,
  `COT` float NOT NULL,
  `FCS` varchar(10) NOT NULL,
  `frameLength` smallint(8) unsigned NOT NULL,
  PRIMARY KEY (`ts`,`sniffer_mac`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `zigbee_trace` (
  `deviceID` tinyint(3) unsigned NOT NULL,
  `channelID` tinyint(3) unsigned NOT NULL,
  `ts` double NOT NULL,
  `rssi` smallint(6) NOT NULL,
  `seqNo` smallint(5) unsigned NOT NULL,
  `dst_pan` varchar(4) NOT NULL,
  `dst_addr` varchar(16) NOT NULL,
  `src_pan` varchar(4) NOT NULL,
  `src_addr` varchar(16) NOT NULL,
  PRIMARY KEY (`deviceID`,`ts`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
```