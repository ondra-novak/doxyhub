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

void Config::parse(const std::string& name) {
	IniConfig cfg;
	cfg.load(name,[](auto &&item){
		if (item.key == "include") {
			throw std::runtime_error("Config_parse - cannot open file: "+item.data);
		}
	});


	const IniConfig::KeyValueMap &tools = cfg["tools"];
	doxygen = tools.mandatory["doxygen"].getPath();
	git = tools.mandatory["git"].getPath();
	curl = tools.mandatory["curl"].getPath();
	activityTimeout = tools.mandatory["activity_timeout"].getUInt();
	totalTimeout = tools.mandatory["total_timeout"].getUInt();

	const IniConfig::KeyValueMap &database = cfg["database"];
	dbconfig.authInfo.username = database.mandatory["username"].getString();
	dbconfig.authInfo.password = database.mandatory["password"].getString();
	dbconfig.baseUrl = database.mandatory["url"].getString();
	dbconfig.databaseName = database.mandatory["dbname"].getString();

	const IniConfig::KeyValueMap &generate = cfg["generate"];
	working= generate.mandatory["working"].getPath();
	doxyfile = generate.mandatory["doxyfile"].getPath();
	queueId = generate.mandatory["queue_id"].getString();
	clusterSize = generate.mandatory["cluster_size"].getUInt();


	const IniConfig::KeyValueMap &log = cfg["log"];
	logfile = log.mandatory["file"].getPath();
	loglevel = log.mandatory["level"].getString();

}


} /* namespace doxyhub */
