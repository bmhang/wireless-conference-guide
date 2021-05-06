-- phpMyAdmin SQL Dump
-- version 4.0.10deb1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Jan 14, 2015 at 02:44 PM
-- Server version: 5.5.40-0ubuntu0.14.04.1
-- PHP Version: 5.5.9-1ubuntu4.5

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `test`
--

-- --------------------------------------------------------

--
-- Table structure for table `wifi_sniffer`
--

CREATE TABLE IF NOT EXISTS `wifi_sniffer` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `ts` varchar(20) NOT NULL,
  `sniffer_mac` varchar(18) NOT NULL,
  `rate` float unsigned NOT NULL,
  `freq` mediumint(9) NOT NULL,
  `SSI` smallint(8) NOT NULL,
  `frame_type` tinyint(4) NOT NULL,
  `frame_subtype` tinyint(4) NOT NULL,
  `dst_mac` varchar(18) NOT NULL,
  `src_mac` varchar(18) NOT NULL,
  `seq_no` mediumint(9) NOT NULL,
  `COT` float NOT NULL,
  `FCS` varchar(10) NOT NULL,
  `frameLength` smallint(8) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
