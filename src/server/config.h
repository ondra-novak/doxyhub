/*
 * config.h
 *
 *  Created on: 14. 6. 2018
 *      Author: ondra
 */

#ifndef SRC_BUILDER_CONFIG_H_
#define SRC_BUILDER_CONFIG_H_
#include <istream>
#include <string>
#include <couchit/json.h>
#include <couchit/config.h>

namespace doxyhub {

class ServerConfig {
public:

	couchit::Config builderdb, controldb;

	std::string logfile;
	std::string loglevel;

	int server_threads;
	int server_dispatchers;
	std::string bind;

	std::string upload_url;
	std::string storage_path;

	std::size_t pakCacheCnt;
	std::size_t clusterCacheCnt;

	std::string console_documentRoot;
	std::string homepage_documentRoot;

	void parse(const std::string &name);

protected:



};

} /* namespace doxyhub */

#endif /* SRC_BUILDER_CONFIG_H_ */
