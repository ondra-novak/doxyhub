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
protected:
	std::string pathname;
	EnvVars envp;
	unsigned int activityTimeout;
	unsigned int totalTimeout;
};


class ExternalProcessLogToFile: public ExternalProcess {
public:
	using ExternalProcess::ExternalProcess;

	void setOutput(std::ostream *stream);

	virtual void onLogOutput(StrViewA line, bool error);
	int execute(std::initializer_list<StrViewA> args);

protected:
	std::ostream *stream = nullptr;
	std::vector<char> outline, errline;

};

} /* namespace doxyhub */

#endif /* SRC_BUILDER_PROCESS_H_ */
