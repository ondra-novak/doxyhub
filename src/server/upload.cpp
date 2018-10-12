/*
 * upload.cpp
 *
 *  Created on: 11. 10. 2018
 *      Author: ondra
 */

#include <fstream>
#include <couchit/couchDB.h>
#include <couchit/document.h>
#include <imtjson/stringview.h>
#include <imtjson/value.h>
#include <shared/shared_state.h>
#include <simpleServer/src/simpleServer/http_headervalue.h>
#include "upload.h"

using couchit::CouchDB;
using couchit::Document;
using json::Value;
using ondra_shared::StrViewA;
using simpleServer::HeaderValue;
using simpleServer::HTTPRequest;

namespace doxyhub {


UploadHandler::UploadHandler(couchit::CouchDB& builderDB,
		const std::string& storage_path, const Notify &onUpdate)
	:db(builderDB),storage_path(storage_path),onUpdate(onUpdate)
{
}

class Download: public ondra_shared::SharedState::Object {
public:
	Download(const std::string &file, std::function<void()> finalize)
	:file(file)
	,tmp_file(file+".part")
	,f(tmp_file, std::ios::out|std::ios::binary|std::ios::trunc)
	,finalize(finalize) {}

	void operator()(HTTPRequest req) {
		auto buffer = req.getUserBuffer();
		if (buffer.empty()) {
			f.close();
			rename(tmp_file.c_str(), file.c_str());
			finalize();
		} else {
			f.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
		}
	}

protected:
	std::string file,tmp_file;
	std::ofstream f;
	std::function<void()> finalize;
};


bool UploadHandler::serve(simpleServer::HTTPRequest req,
		ondra_shared::StrViewA vpath) {

	if (!req.allowMethods({"PUT"})) return true;
	vpath = vpath.trim([](char c){return c == '/';});
	if (vpath.indexOf("/") != vpath.npos) {
		req.sendErrorPage(403);
		return true;
	}

	StrViewA key = vpath;
	Value doc = db.get(key, CouchDB::flgNullIfMissing);
	if (doc.isNull()) {
		req.sendErrorPage(404);
		return true;
	}

	HeaderValue auth = req["Authorization"];
	StrViewA token = auth.trim(isspace);
	if (token.substr(0,7) != "bearer ") {
		req.sendErrorPage(401);
		return true;
	}

	token = token.substr(7).trim(isspace);
	if (token != doc["upload_token"].getString()) {
		req.sendErrorPage(403);
		return true;
	}

	std::string fullPath = storage_path;
	fullPath.append(key.data, key.length);

	auto finalize = [upfn = Notify(onUpdate), key = std::string(key)]() {
		upfn(key);
	};

	req.readBodyAsync(static_cast<std::size_t>(-1),
			ondra_shared::SharedState::make<Download>(fullPath, finalize));

	return true;
}



}
