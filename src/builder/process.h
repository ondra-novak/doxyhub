/*
 * process.h
 *
 *  Created on: 14. 6. 2018
 *      Author: ondra
 */

#ifndef SRC_BUILDER_PROCESS_H_
#define SRC_BUILDER_PROCESS_H_

#include <shared/stringview.h>

namespace doxyhub {

using ondra_shared::StrViewA;
typedef char *const *EnvVars;

class ExternalProcess {
public:
	ExternalProcess(std::string pathname, EnvVars envp, unsigned int activityTimeout, unsigned int totalTimeout);

	virtual void onLogOutput(StrViewA line, bool error);
	virtual ~ExternalProcess() {}

	int execute(std::initializer_list<StrViewA> args);
	void set_start_dir(std::string &&sd) {start_dir = std::move(sd);}
	void set_start_dir(const std::string &sd) {start_dir = sd;}
protected:
	std::string pathname;
	std::string start_dir;
	EnvVars envp;
	unsigned int activityTimeout;
	unsigned int totalTimeout;
};




} /* namespace doxyhub */

#endif /* SRC_BUILDER_PROCESS_H_ */
