/*
 * consolePage.cpp
 *
 *  Created on: 11. 10. 2018
 *      Author: ondra
 */

#include <netdb.h>
#include <imtjson/value.h>
#include <couchit/document.h>
#include <server/consolePage.h>
#include <shared/logOutput.h>
#include <simpleServer/http_parser.h>
#include <couchit/exception.h>
#include <couchit/query.h>
#include <simpleServer/query_parser.h>
#include "url2Hash.h"
#include "search.h"


using json::Value;
using json::Object;
using ondra_shared::logDebug;
using simpleServer::HTTPResponse;
using simpleServer::QueryParser;
using couchit::UpdateException;
using couchit::Document;
using ondra_shared::BinaryView;
using ondra_shared::logError;
using ondra_shared::logInfo;
using ondra_shared::logNote;
using ondra_shared::logProgress;

namespace doxyhub {


static couchit::View qstat("_design/queue/_view/stats",(couchit::View::groupLevel*1)|couchit::View::reduce);


ConsolePage::ConsolePage(CouchDB& db,
		const std::string &document_root,
		const std::string &upload_url)
	:db(db)
	,fmapper(std::string(document_root),"index.html")
	,upload_url(upload_url)
{
}

bool ConsolePage::serve(StrViewA projectId, HTTPRequest req, StrViewA vpath) {

	if (vpath.begins("/api/")) {
		run_api(projectId, req, vpath.substr(5));
	} else {
		if (!fmapper(req,vpath)) {
			req.sendErrorPage(404);
		}
	}
	return true;
}

void ConsolePage::sendAttachment(Value doc, StrViewA attchname, HTTPRequest req) {

	if (doc == nullptr) {
		req.sendErrorPage(404);
	} else {

		try {
			auto dwn = db.getAttachment(doc, attchname, req["If-None-Match"]);
			if (dwn.notModified) {
				req.sendResponse("text/plain","",304);
				return;
			}

			auto stream = req.sendResponse(HTTPResponse(200)
					("ETag",dwn.etag)
					.contentLength(dwn.length)
					.contentType(dwn.contentType));

			auto data = dwn.read();
			while (!data.empty()) {
				stream.write(data);
				data = dwn.read();
			}


		} catch (const couchit::RequestError &err) {
			logNote("Get attachment error: $1", err.what());
			req.sendErrorPage(err.getCode());
		}
	}


}

void ConsolePage::rebuild_project(couchit::Document doc, bool force) {
	Document chdoc(doc);
	chdoc.set("status","queued");
	chdoc.set("queue",selectQueue());
	chdoc.set("upload_token",generate_token());
	chdoc.set("upload_url",upload_url);
	if (force) chdoc.set("build_rev","");
	db.put(chdoc);
}

bool ConsolePage::checkUrl(StrViewA url) const {
	int start;
	if (url.begins("http://")) start = 7;
	else if (url.begins("https://")) start = 8;
	else return false;
	auto sep = std::min(url.indexOf("/"), url.indexOf(":"));
	if (sep == url.npos) return false;
	auto beg = url.indexOf("@");
	if (beg > sep) beg =  start;
	std::string domain = url.substr(start, sep - start);

	struct addrinfo *ainfo;

	if (getaddrinfo(domain.c_str(),"443",nullptr,&ainfo)) {
		return false;
	}
	freeaddrinfo(ainfo);

	return true;
}

Value ConsolePage::checkExist(StrViewA url) const {

	return searchByUrl(db,url);
}


void ConsolePage::run_api(StrViewA projectId, HTTPRequest req,StrViewA api_path) {

	using namespace couchit;

	Value doc = db.get(projectId, CouchDB::flgNullIfMissing);;

	QueryParser qp(api_path);

	StrViewA cmd = qp.getPath();


	if (cmd == "status") {

		Object result;
		if (doc == nullptr) {
			result("id", projectId)
				  ("status","unknown");
		} else {
			result("id", projectId)
				  ("url",doc["url"])
				  ("status",doc["status"])
				  ("last_error",doc["error"])
				  ("revision",doc["revision"])
				  ("build_time",doc["build_time"]);
		}

		req.sendResponse("application/json",Value(result).stringify());

	} else if (cmd == "stdout") {
		sendAttachment(doc, "stdout", req);
	} else if (cmd == "stderr") {
		sendAttachment(doc, "stderr", req);
	} else if (cmd == "queue") {
		couchit::Query q = db.createQuery(qstat);
		couchit::Result res = q.exec();
		req.sendResponse("application/json",res.stringify());

	} else if (cmd == "build") {

		if (doc == nullptr) {
			req.sendErrorPage(404);
		} else if (!req.allowMethods({"POST"})) {
			return;
		} else {

			req.readBodyAsync(10000,[=](HTTPRequest req){

				if (!req["Content-Type"].begins("application/json")) {
					req.sendErrorPage(415);
					return;
				}

				auto buffer = req.getUserBuffer();
				Value options;
				StrViewA captcha_code;
				bool force;

				options = Value::fromString(StrViewA(BinaryView(buffer)));
				captcha_code = options["captcha"].getString();
				force = options["force"].getBool();

				if (!checkCapcha(captcha_code)) {
					req.sendErrorPage(402);
					return;
				}

				if (doc["status"] == "queued" || doc["status"] == "building") {
					req.sendErrorPage(409);
					return;
				}
				if (doc["status"] == "deleted") {
					force = true;
				}

				try {
					rebuild_project(doc,force);
				} catch (const UpdateException &e) {
					if (e.getError(0).isConflict()) {
						logNote("Update conflict $1", projectId);
						req.sendErrorPage(409);
					} else {
						logError("Unable to update document $1 - error $2", projectId, e.what());
						req.sendErrorPage(500);
					}
					return;
				}
				req.sendResponse("text/plain","",202,"Accepted");
			});
		}
	} else if (cmd == "create") {

		if (doc != nullptr) {
			req.sendErrorPage(409);
		} else if (!req.allowMethods({"POST"})) {
			return;
		} else {

			req.readBodyAsync(10000,[=](HTTPRequest req){

				if (req["Content-Type"] != "application/json") {
					req.sendErrorPage(415);
					return;
				}

				auto buffer = req.getUserBuffer();
				Value options;
				StrViewA captcha_code;
				String url;

				options = Value::fromString(StrViewA(BinaryView(buffer)));
				captcha_code = options["captcha"].getString();
				url = options["url"].toString();

				if (checkCapcha(captcha_code)) {
					req.sendErrorPage(402);
					return;
				}

				if (!checkUrl(url)) {
					req.sendErrorPage(400,StrViewA(),"Invalid URL");
					return;
				}


				Value exist = checkExist(url);
				Value response;
				int code;

				if (exist.defined()) {
					response = Object("status","found")
									("id",exist["_id"]);
					if (exist["status"] == "deleted") {
						try {
							rebuild_project(exist, false);
						} catch (...) {

						}
					}
					code = 200;
				} else {
					Document doc;
					doc.setID(url2hash(url));
					doc.set("url",url);
					rebuild_project(doc,false);
					response = Object("status","created")
									 ("id",doc.getIDValue());
					code = 201;
				}
				req.sendResponse("application/json",response.stringify(),code);
			});


		}


	} else {
		req.sendErrorPage(404);
	}
}

json::Value ConsolePage::selectQueue() {
	using namespace couchit;
	Query q = db.createQuery(qstat);
	Result r = q.exec();

	std::size_t maxlen = static_cast<std::size_t>(-1);
	Value choosen;
	for (Row rw:r) {
		std::size_t len = rw.value["sum"].getUInt();
		if (len < maxlen) {
			choosen = rw.key;
			maxlen = len;
		}
	}

	if (choosen.defined())
		return choosen;
	else
		throw std::runtime_error("No queue available");

}

Value ConsolePage::generate_token() {
	return db.genUID("build_token-");
}

bool ConsolePage::checkCapcha(StrViewA capcha) const {
	//TODO:
	return !capcha.empty();
}

}
