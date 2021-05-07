# Wireless Conferencing Experiments with BEOF

This repository contains the necessary code to perform a locating-array based wireless conferencing experiment on the w.iLab.t wireless testbed using the Bash Experiment Orchestration Framework (BEOF). The research behind the experiment described here first appeared in [1].

## Getting Started

### Introduction

Screening experiments are an important first step in determining which factors should be included in implementation, and which ones can be safely eliminated from consideration. Locating array based screening experiments use the properties of locating arrays to reduce the number of tests needed to discover influential factors and 2-way interactions with many less tests than other traditional methods.

This guide provides the steps necessary to reproduce experiments conducted in [1].

### Prerequisites

This user guide is designed to be used with the w.iLab.t-2 testbed. The Zotac, Warp, and Server type nodes were used.

Requirements:
- w.iLab.t-2 testbed
- jFed Experimenter software (not required, but highly recommended) can be found [here](https://jfed.ilabt.imec.be).
- one Server node
- one Warp node associated with a Zotac node (e.g. ZotacG3/Warp2, ZotacH3/Warp3)
- at least three Zotac nodes
- Ubuntu 14.04 installed on the nodes

The first thing to do is to swap in some nodes on wilab. For more detailed instructions on how to swap in nodes, please refer to the w.iLab.t documentation [here](https://doc.ilabt.imec.be/ilabt/wilab/getting_started.html "here"). In this repository is a file named `small_exp41_3.rspec`. This file can be opened with jFed Experimenter and has already configured a small topology of nodes to be used. Simply duplicate the Zotac nodes to increase the size of the experiment.

### Xilinx

(NEEDS UPDATING) You have to install Xilinx next, and I didn't do this part before. Daniel, you're welcome to add here as you can remember!

### Clone BEOF

This experiment runs on the Bash Experiment Orchestration Framework. The BEOF repository can be found [here](https://github.com/mmehari/BEOF). Clone the repository into this folder.

## Setting Up the Conferencing Experiment

Preparing this experiment comes with four distinct phases:
- Set up the Warp node
- Set up a MySQL database on the server node
- Set up the `LA_INFR.sh` file
- Begin the experiment from a Zotac node.

Each of these phases will now be described in detail.

### Setting up the Warp Node

To prepare the Warp node, open `setup_WARP.sh`, located inside the `setup` folder.
1. Edit the `SETUP_PATH` to the path where your `setup` folder is located.
2. On line 5, change the path to the `WARP_images` folder to where it is located for you. 
3. On line 6, change the username to your username.
4. (NEEDS UPDATING) On line 8, change the path to your installation of Xilinx. Also, change the path to your `WARP_images` folder and specify the Warp file to use. For example, if you swapped in ZotacG3 and Warp2, you would use the `WARP2_80M_tx.txt` file. If you swapped in Warp1 or Warp3, change the path to the corresponding file.
5. Open an SSH terminal into the Zotac node associated with your Warp node. For example, you would SSH into ZotacG3 if you swapped in Warp2. From your Zotac node, execute `setup_WARP.sh` and allow the process to complete.

### Setting up a MySQL Database on the Server Node

This experiment requires a MySQL database to store experiment data before outputting it.

(NEEDS UPDATING) I took very general notes on this, but we're going to have to go through this to make it MUCH more specific.

These commands must be run on the server node you swapped in. In the example file provided, server3 is used.
1. To begin, run `sudo apt-get install mysql-server`. When prompted, set the root account password to something you will remember.
2. Create a new user with all privileges. Make note of these credentials as you will need them later.
3. Edit the `my.cnf` file in `~/etc/mysql` and change the bind address to `0.0.0.0`. This must be done so the database can be accessed from another server.
4. Restart the server with `sudo service mysql restart`.
5. Create a new database called `benchmarking`.
6. Create new tables in this database using the code provided below. There are 20 tables that need to be created.

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

Setting up a MySQL database can be time consuming, so it is recommended that you create a disk image once you complete this step. This can be done by right-clicking a node in jFed and selecting "Create Image." Be sure to check the "Save System Users in Image" checkbox.

When loading your MySQL database from the disk image, you may not be able to start the server (`sudo service mysql start`). If this is the case, the following command should be run:

```
sudo adduser --disabled-password --gecos "" mysql && sudo chown mysql:mysql -R /var/lib/mysql && sudo chmod -R 755 /var/lib/mysql/ && sudo service mysql start
```

### Setting Up the `LA_INFR.sh` File

Open an SSH terminal into the Zotac node you want to use as the Experiment Controller. Then open 'LA_INFR.sh'.
1. On line 3, change the directory to your location of the BEOF folder.
2. On line 5, change the IP address to that of the node you want to use as the Experiment Controller (EC). That should be the node you are currently logged into. The IP address can be found by entering `ifconfig -a`. Use the IP address of the first wireless interface.
3. On line 22, edit the `SPEAKER_node` value with the last digit(s) of the IP address of the speaker node. The speaker node must be one of the Zotac nodes.
4. Edit the `LISTENER_nodes` variable with the last digit(s) of the IP addresses of the listener nodes. The listener nodes are all Zotac nodes swapped in excluding the speaker node. Put a space between the values of each node.
5. Edit the `SNIFFER_nodes` variable with the last digit(s) of the IP addresses of sniffer nodes. Sniffer nodes are all Zotac nodes (including the speaker node).
6. Edit the `INTRF_nodes` variable with the last digit of the IP address of the server node.
7. Scroll down until you see a comment labeled `database parameters`. Edit the `HOST`, `USER`, `PASSWD`, and `DB` values with the IP address of the server hosting the MySQL database, the username and password for the user you created with all privileges, and the name of the database (which should remain `benchmarking`).

Save these changes. Then execute the `setup_EC.sh` file inside the `setup` folder.

## Executing the Experiment and Retrieving Results

To run the experiment, execute `LA_INFR.sh` on the node you designated the Experiment Controller.

Results will be generated in the home folder of the Experiment Controller, e.g. `~/BEOF/tmp`. There will be a file for every Zotac node in the experiment. Each file will be named based on the hardware address of the node's first wireless interface. Be sure to save these results before swapping the experiment out or they will be lost.

## Other Notes

There is a known issue with this experiment that can cause the experiment to hang indefinitely when a large number of Zotac nodes (e.g. >8) are swapped in.

## References
[1] R. Compton, M. T. Mehari, C. J. Colbourn, E. D. Poorer, I. Moerman, V. R. Syrotiuk, "Identifying Parameters and Interactions that impact Wireless Network Performance Efficiently," Unpublished?, October 2017.
