/*
 * main.cpp
 *
 *  Created on: 13. 6. 2018
 *      Author: ondra
 */

#include "queue.h"
#include "config.h"
#include <simpleServer/abstractService.h>
#include <couchit/changeObserver.h>
#include <shared/raii.h>
#include <shared/stdLogFile.h>
#include <simpleServer/exceptions.h>

using ondra_shared::RAII;
using ondra_shared::StdLogFile;
using ondra_shared::StdLogProviderFactory;
using ondra_shared::LogLevelToStrTable;
using ondra_shared::LogLevel;
using ondra_shared::logProgress;
using ondra_shared::PStdLogProviderFactory;


int main(int argc, char **argv, char **envp) {
	using namespace doxyhub;
	using namespace simpleServer;
	using ondra_shared::RAII;
	EnvVars envs = envp;

	return

	ServiceControl::create(argc, argv, "doxyhub builder",
			[=](ServiceControl control, StrViewA , ArgList args){

		if (args.length < 1) {
			throw std::runtime_error("You need to supply a pathname of configuration");
		}

		typedef RAII<char *, decltype(&free), &free> CStr;
		CStr cfgpath(realpath(args[0].data, nullptr));

		if ((char *)cfgpath == nullptr) {
			int err = errno;
			throw simpleServer::SystemException(err, "Unable to resolve argument" +std::string(args[0]));
		}

		doxyhub::Config cfg;
		cfg.parse(StrViewA(cfgpath));

		StdLogFile::create(cfg.logfile, cfg.loglevel, LogLevel::debug)->setDefault();

		control.enableRestart();


		CouchDB db(cfg.dbconfig);

		Builder bld(cfg, envs);
		Queue q(bld,db,cfg.queueId);
		q.run();


		control.dispatch();
		q.stop();


		return 0;
	});



}
