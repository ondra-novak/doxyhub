/*
 * main.cpp
 *
 *  Created on: 13. 6. 2018
 *      Author: ondra
 */

#include "config.h"
#include <simpleServer/abstractService.h>
#include <shared/raii.h>
#include <shared/stdLogFile.h>
#include <simpleServer/exceptions.h>
#include <couchit/couchDB.h>


int main(int argc, char **argv) {
	using namespace doxyhub;
	using namespace simpleServer;
	using namespace couchit;
	using namespace ondra_shared;


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

		doxyhub::ServerConfig cfg;
		cfg.parse(StrViewA(cfgpath));

		StdLogFile::create(cfg.logfile, cfg.loglevel, LogLevel::debug)->setDefault();

		control.enableRestart();

		CouchDB bulderdb(cfg.builderdb);
		CouchDB controldb(cfg.controldb);

		control.dispatch();


		return 0;
	});



}
