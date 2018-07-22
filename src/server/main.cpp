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
#include <simpleServer/realpath.h>
#include <simpleServer/threadPoolAsync.h>
#include <simpleServer/address.h>
#include <rpc/rpcServer.h>
#include <couchit/couchDB.h>


#include "initdb.h"
#include "bldcontrol.h"

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

		std::string cfgpath = realpath(args[0]);
		doxyhub::ServerConfig cfg;
		cfg.parse(cfgpath);

		StdLogFile::create(cfg.logfile, cfg.loglevel, LogLevel::debug)->setDefault();

		control.enableRestart();

		CouchDB builderdb(cfg.builderdb);
		CouchDB controldb(cfg.controldb);

		initBuildDB(builderdb);

		AsyncProvider asyncProvider = ThreadPoolAsync::create(cfg.server_threads, cfg.server_dispatchers);
		NetAddr addr = NetAddr::create(cfg.bind,8800,NetAddr::IPvAll);

		RpcHttpServer serverObj(addr, asyncProvider);
		RpcHttpServer::Config scfg;
		scfg.enableConsole = true;
		scfg.enableDirect = false;
		scfg.enableWS = false;
		scfg.maxReqSize = 65536;

		serverObj.addRPCPath("/RPC", scfg);
		serverObj.add_listMethods("methods");
		serverObj.add_ping("ping");

		BldControl bldcont(builderdb);
		bldcont.init(serverObj);

		serverObj.start();

		control.dispatch();


		return 0;
	});



}
