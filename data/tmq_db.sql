CREATE DATABASE IF NOT EXISTS `tmq_db` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;

use `tmq_db`;

DROP TABLE IF EXISTS `command_conf`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `command_conf` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'ID',
  `command_name` varchar(128) NOT NULL COMMENT '消息队列别名',
  `command_vhost` varchar(256) NOT NULL COMMENT '消息队列vhost',
  `command_type` tinyint(4) NOT NULL COMMENT '消息队列类型',
  `command_encode_type` tinyint(4) NOT NULL COMMENT '消息队列内容格式',
  `command_no` bigint(20) unsigned NOT NULL  COMMENT '消息队列名称',
  `server_id` smallint(6) unsigned NOT NULL  COMMENT '消息队列编号',
  `delete_flag` tinyint(4) NOT NULL default 0 COMMENT '消息队列删除标识',
  `last_update_time` bigint(20) unsigned NOT NULL COMMENT '更新时间',
  PRIMARY KEY (`id`),
  UNIQUE KEY `command_no_key` (`command_no`, `server_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

DROP TABLE IF EXISTS `db_conf`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `db_conf` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'ID',
  `db_name` varchar(128) NOT NULL COMMENT '数据库名称',
  `db_username` varchar(128) NOT NULL COMMENT '数据库名称',
  `db_password` varchar(128) NOT NULL COMMENT '数据库地址',
  `db_host` varchar(46) NOT NULL COMMENT '数据库地址',
  `db_port` smallint(6) NOT NULL COMMENT '数据库端口',
  `db_charset` varchar(32) NOT NULL COMMENT '数据库字符集',
  `db_max_connection` int(10) unsigned COMMENT '数据库最大连接数',
  `db_min_connection` int(10) unsigned COMMENT '数据库最小连接数',
  `last_update_time` bigint(20) unsigned NOT NULL COMMENT '更新时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

DROP TABLE IF EXISTS `command_db_conf`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `command_db_conf` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'ID',
  `db_id` int(11) NOT NULL COMMENT 'db_id',
  `command_no` bigint(20) unsigned NOT NULL  COMMENT '消息队列名称',
  `sql_template` text NOT NULL COMMENT 'sql 模板',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `http_conf`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `http_conf` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'ID',
  `http_host` varchar(46) NOT NULL COMMENT 'http地址',
  `http_port` smallint(6) NOT NULL COMMENT 'http端口',
  `http_max_retrytime` smallint(6) NOT NULL COMMENT 'http重试次数',
  `last_update_time` bigint(20) unsigned NOT NULL COMMENT '更新时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

DROP TABLE IF EXISTS `command_http_conf`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `command_http_conf` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'ID',
  `http_id` int(11) NOT NULL COMMENT 'http_id',
  `http_template` text NOT NULL COMMENT 'http 模板',
  `command_no` bigint(20) unsigned NOT NULL  COMMENT '消息队列名称',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;
