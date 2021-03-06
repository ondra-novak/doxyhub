/*
 * config.cpp
 *
 *  Created on: 14. 6. 2018
 *      Author: ondra
 */


#include "config.h"
#include <simpleServer/exceptions.h>
#include <shared/virtualMember.h>
#include <shared/ini_config.h>
#include <stdexcept>

namespace doxyhub {


using ondra_shared::IniConfig;

static DBConfig parseDBConfig(const IniConfig::KeyValueMap &d1) {
	DBConfig builderdb;
	builderdb.authInfo.username = d1.mandatory["username"].getString();
	builderdb.authInfo.password = d1.mandatory["password"].getString();
	builderdb.baseUrl = d1.mandatory["url"].getString();
	builderdb.databaseName = d1.mandatory["dbname"].getString();
	builderdb.conflicts = d1["conflicts"].getBool(false);
	return builderdb;
}

void ServerConfig::parse(const std::string& name) {
	IniConfig cfg;
	cfg.load(name,[](auto &&item){
		if (item.key == "include") {
			throw std::runtime_error("Config_parse - cannot open file: "+item.data);
		}
	});

	builderdb = parseDBConfig(cfg["builderdb"]);
	controldb = parseDBConfig(cfg["controldb"]);


	const IniConfig::KeyValueMap &log = cfg["log"];
	logfile = log.mandatory["file"].getPath();
	loglevel = log.mandatory["level"].getString();

	const IniConfig::KeyValueMap &server = cfg["server"];
	bind = server.mandatory["listen"].getString();
	server_threads = server.mandatory["threads"].getUInt();
	server_dispatchers = server.mandatory["dispatchers"].getUInt();

	const IniConfig::KeyValueMap &storage = cfg["storage"];
	upload_url = storage.mandatory["upload_url"].getString();
	storage_path = storage.mandatory["storage_path"].getPath();
	pakCacheCnt = storage.mandatory["site_cache_items"].getUInt();
	clusterCacheCnt = storage.mandatory["cluster_cache_items"].getUInt();

	const IniConfig::KeyValueMap &console = cfg["console"];
	console_documentRoot = console.mandatory["document_root"].getPath();
	captchaScript = console.mandatory["captcha"].getPath();

	const IniConfig::KeyValueMap &homepage = cfg["hp"];
	homepage_documentRoot = homepage.mandatory["document_root"].getPath();
}


} /* namespace doxyhub */
