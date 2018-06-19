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

static couchit::Config parseDBConfig(const IniConfig::KeyValueMap &d1) {
	couchit::Config builderdb;
	builderdb.authInfo.username = d1.mandatory["username"].getString();
	builderdb.authInfo.password = d1.mandatory["password"].getString();
	builderdb.baseUrl = d1.mandatory["url"].getString();
	builderdb.databaseName = d1.mandatory["dbname"].getString();
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

}


} /* namespace doxyhub */
