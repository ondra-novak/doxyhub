/*
 * homePage.h
 *
 *  Created on: 18. 10. 2018
 *      Author: ondra
 */

#ifndef DOXYHUB_HOMEPAGE_H_
#define DOXYHUB_HOMEPAGE_H_

#include "search.h"

namespace doxyhub {

using ondra_shared::StrViewA;


auto createHomepage(StrViewA documentRoot, doxyhub::SiteServer &docs, couchit::CouchDB &db) {
	using namespace simpleServer;

	simpleServer::HttpFileMapper homepage(std::string(documentRoot),"index.html");

	return [homepage, &docs, &db](HTTPRequest req, StrViewA vpath) mutable -> bool {

		if (vpath == "/" || vpath.begins("/files/")) {
			return homepage(req, vpath);
		} else if (vpath == "/favicon.ico") {
			req.sendErrorPage(404);
			return true;
		} else if (vpath.begins("/search?")) {
			if (req.allowMethods({"HEAD","GET"})) {
				String url;
				String branch;
				QueryParser qp(vpath);
				for (auto &&kv : qp) {
					if (kv.first == "url") {
						url = kv.second;
					}
					if (kv.first == "branch") {
						branch = kv.second;
					}
				}
				if (url.empty() || branch.empty()) {
					req.sendErrorPage(400);
				} else {
					Value res = searchStatusByUrl(db,url, branch);
					req.sendResponse("application/json",res.stringify());
				}
			}
			return true;
		} else {
			return docs.serve(req,vpath);
		}
	};

}


} /* namespace doxyhub */

#endif /* DOXYHUB_HOMEPAGE_H_ */
