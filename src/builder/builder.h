/*
 * builder.h
 *
 *  Created on: 14. 6. 2018
 *      Author: ondra
 */

#ifndef SRC_BUILDER_BUILDER_H_
#define SRC_BUILDER_BUILDER_H_

#include <shared/worker.h>
#include "config.h"
#include "process.h"
#include "active.h"

#include "../errors.h"

namespace doxyhub {

class ExternalProcessWithLog;

enum class BuildStage {
	checkrev,
	download,
	generate,
	upload

};


class Builder {
public:
	Builder(const Config &cfg, EnvVars envVars);



	using ProgressFn = std::function<void(BuildStage)>;

	struct BuildRequest {
		std::string url;
		std::string branch;
		std::string revision;
		std::string upload_url;
		std::string upload_token;
		ProgressFn progressFn;
	};

	DoxyhubError buildDoc(const BuildRequest &req);

	void prepareDoxyfile(const std::string &source, const std::string &target, const std::string &buildPath, const std::string &url);

	std::string log;
	std::string revision;


	void stopTools();

protected:
	Config cfg;
	EnvVars envVars;
	ActiveObject<ExternalProcess> activeTool;
	typedef ActiveObject<ExternalProcess>::Guard AGuard;

	std::string get_git_last_revision(ExternalProcessWithLog &&git, const std::string &url, const std::string &branch);


};

} /* namespace doxyhub */

#endif /* SRC_BUILDER_BUILDER_H_ */
