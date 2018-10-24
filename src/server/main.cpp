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
#include <simpleServer/query_parser.h>
#include "SiteServer.h"
#include "consolePage.h"
#include "homePage.h"
#include <couchit/conflictresolver.h>
#include <simpleServer/src/simpleServer/http_server.h>


#include "initdb.h"

#include "upload.h"
#include "search.h"
#include "captcha.h"

class Conflicts: public couchit::ConflictResolver {
public:
	virtual void onResolverError() override {
		using namespace ondra_shared;
		try {
			throw;
		} catch (const std::exception &e) {
			logError("Conflict resolver error $1", e.what());
		}
	}

};


int main(int argc, char **argv) {
	using namespace doxyhub;
	using namespace simpleServer;
	using namespace couchit;
	using namespace json;
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

		Captcha captcha(cfg.captchaScript);


		CouchDB builderdb(cfg.builderdb);
		CouchDB controldb(cfg.controldb);

		initBuildDB(builderdb);

		Conflicts conflict_resolver;
		if (cfg.builderdb.conflicts) {
			conflict_resolver.runResolver(builderdb);
		}

		AsyncProvider asyncProvider = ThreadPoolAsync::create(cfg.server_threads, cfg.server_dispatchers);
		NetAddr addr = NetAddr::create(cfg.bind,8800,NetAddr::IPvAll);

		ConsolePage consolePage(builderdb,controldb, captcha, cfg.console_documentRoot, cfg.upload_url, cfg.storage_path);

		SiteServer page_sources(consolePage, cfg.storage_path, cfg.pakCacheCnt, cfg.clusterCacheCnt);
		UploadHandler upload(builderdb, cfg.storage_path, [&](auto &&a) {
			page_sources.updateArchive(a);
		});


		MiniHttpServer serverObj(addr, asyncProvider);
		RpcHttpServer::Config scfg;
		scfg.enableConsole = true;
		scfg.enableDirect = false;
		scfg.enableWS = false;
		scfg.maxReqSize = 65536;


		serverObj >> HttpPathMapper<HttpStaticPathMapper>(HttpStaticPathMapper({
			{"/upload",[&](const HTTPRequest &req, const StrViewA &vpath) {
				return upload.serve(req, vpath);
				}},
			{"/",createHomepage(cfg.homepage_documentRoot,page_sources, builderdb)}
		}));




		control.dispatch();


		return 0;
	});



}
