/*
 * builder.h
 *
 *  Created on: 14. 6. 2018
 *      Author: ondra
 */

#ifndef SRC_BUILDER_BUILDER_H_
#define SRC_BUILDER_BUILDER_H_

#include "config.h"
#include "process.h"

namespace doxyhub {


class Builder {
public:
	Builder(const Config &cfg, EnvVars envVars);

	void buildDoc(const std::string &url, const std::string &output_name, const std::string &revision);

	void prepareDoxyfile(const std::string &source, const std::string &target, const std::string &buildPath);

protected:
	Config cfg;
	EnvVars envVars;
	std::string log;
	std::string warnings;
	std::string revision;
};

} /* namespace doxyhub */

#endif /* SRC_BUILDER_BUILDER_H_ */
