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
#include <shared/shared_function.h>
#include <simpleServer/asyncProvider.h>
#include <simpleServer/http_headervalue.h>
#include "upload.h"

using couchit::CouchDB;
using couchit::Document;
using json::Value;
using ondra_shared::StrViewA;
using simpleServer::HeaderValue;
using simpleServer::HTTPRequest;

namespace doxyhub {


using namespace simpleServer;
using namespace ondra_shared;

UploadHandler::UploadHandler(couchit::CouchDB& builderDB,
		const std::string& storage_path, const Notify &onUpdate)
	:db(builderDB),storage_path(storage_path),onUpdate(onUpdate)
{
}



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
	std::string tmpPath = fullPath+".part"+db.genUID().toString().c_str();


	auto stream = req.getBodyStream();


	auto finalize = [upfn = Notify(onUpdate), key = std::string(key)]() {
		upfn(key);
	};

	using SharedFn = shared_function<void(AsyncState, BinaryView)>;
	auto copyfn = [=,f=std::ofstream(tmpPath, std::ios::out|std::ios::binary|std::ios::trunc)]
				   (SharedFn fn, AsyncState st, BinaryView data)mutable{

		if (st == asyncOK) {
			StrViewA str(data);
			f.write(str.data, str.length);
			stream.readAsync(fn);
		} else if (st == asyncEOF) {
			f.close();
			rename(tmpPath.c_str(), fullPath.c_str());
			req.sendResponse("text/plain","",204);
			finalize();
		} else {
			remove(tmpPath.c_str());
		}
	};

	stream.readAsync(SharedFn(std::move(copyfn)));

	return true;
}



}
