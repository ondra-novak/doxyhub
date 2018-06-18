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

class Config {
public:

	std::string doxygen;
	std::string git;

	couchit::Config dbconfig;

	std::string output;
	std::string working;
	std::string doxyfile;
	std::string queueId;

	std::string logfile;
	std::string loglevel;

	int activityTimeout;
	int totalTimeout;

	void parse(const std::string &name);

protected:



};

} /* namespace doxyhub */

#endif /* SRC_BUILDER_CONFIG_H_ */
