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
#include <unistd.h>
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

static const char *formatted_attachment_header = R"html(
<!DOCTYPE html>
<html><head><title>log.txt</title><meta charset="utf-8"></head>
<style>
body {
font-family:monospace;
white-space: pre;
}
.stdout {
  color: #440;
}
.stderr {
  color: #C44;
}
.both  {
 color: black;
}
</style>
<body>
)html";

static const char *formatted_attachment_footter = R"html(
</body>
</html>
)html";


static couchit::View qstat("_design/queue/_view/stats",(couchit::View::groupLevel*1)|couchit::View::reduce);


ConsolePage::ConsolePage(CouchDB& db,
		const std::string &document_root,
		const std::string &upload_url,
		const std::string &storage_path)
	:db(db)
	,fmapper(std::string(document_root),"index.html")
	,upload_url(upload_url)
	,storage_path(storage_path)
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

template<typename Fn>
void ConsolePage::sendAttachment(Value doc, StrViewA attchname, HTTPRequest req, Fn &&formatter, StrViewA content_type) {

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
					.contentType(content_type));

			auto data = dwn.read();
			while (!data.empty()) {
				stream.write(formatter(data));
				data = dwn.read();
			}
			stream.write(formatter(data));


		} catch (const couchit::RequestError &err) {
			logNote("Get attachment error: $1", err.what());
			req.sendErrorPage(err.getCode());
		}
	}
}

void ConsolePage::sendAttachment(Value doc, StrViewA attchname, HTTPRequest req) {
	sendAttachment(doc, attchname,req,
			[](BinaryView data) {return data;},
			"application/octet-stream");
}
void ConsolePage::sendFormattedAttachment(Value doc, StrViewA attchname, HTTPRequest req) {

	std::string buffer = formatted_attachment_header;
	bool newln = true;
	bool beg = true;
	auto formatter = [beg, buffer,newln](BinaryView data) mutable {
		if (!beg) buffer.clear(); else beg = false;
		if (data.empty()) {
			return BinaryView(StrViewA(formatted_attachment_footter));
		} else {
			for (auto c: data) {
				switch (c) {
				case '\n': if (!newln) buffer.append("</div>");
						   newln = true;
						   break;
				case '!': if (newln) buffer.append("<div class=\"stderr\">");
						   buffer.push_back(c);
						   newln = false;
						   break;
				case '-': if (newln) buffer.append("<div class=\"stdout\">");
						   buffer.push_back(c);
						   newln = false;
						   break;
				case '<': buffer.append("&lt;");break;
				case '>': buffer.append("&gt;");break;
				case '&': buffer.append("&amp;");break;
				case '"': buffer.append("&quot;");break;
				default: if (newln) buffer.append("<div class=\"both\">");
						  buffer.push_back(c);
						  newln = false;
						  break;
				}
			}
			return BinaryView(StrViewA(buffer));
		}

	};

	sendAttachment(doc, attchname, req, formatter, "text/html;charset=utf-8");

}


void ConsolePage::rebuild_project(couchit::Document doc, bool force) {
	Document chdoc(doc);
	chdoc.set("status","queued");
	chdoc.set("queue",selectQueue());
	chdoc.set("upload_token",generate_token());
	chdoc.set("upload_url",String({upload_url,"/",doc.getID()}));
	chdoc.enableTimestamp();
	if (force) chdoc.set("build_rev","");
	db.put(chdoc);
}

bool ConsolePage::checkUrl(StrViewA url) const {
	int start;
	if (url.begins("http://")) start = 7;
	else if (url.begins("https://")) start = 8;
	else return false;
	auto sep = std::min(url.indexOf("/",start), url.indexOf(":",start));
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

Value ConsolePage::checkExist(StrViewA url, StrViewA branch) const {

	return searchByUrl(db,url,branch);
}

bool ConsolePage::checkBranchName(StrViewA branch) const {
	if (branch.empty() || branch == "@") return false;

	char prevchar =0;
	constexpr StrViewA specChars( "-_/<>.,|;{}!@#$%&()");
	for(auto &&c : branch) {
		if (!isalnum(c) && specChars.indexOf(StrViewA(&c,1)) == specChars.npos) return false;
		if (c == '.' && (prevchar == '.' || prevchar == '/')) return false;
		if (c == '{' && prevchar == '@') return false;
		if (c == '/' && prevchar == '/') return false;
		prevchar = c;
	}
	if (branch.ends(".lock") || branch.ends(".") || branch.begins("/") || branch.ends("/")) return false;
	return true;
}

void ConsolePage::run_api(StrViewA projectId, HTTPRequest req,StrViewA api_path) {

	using namespace couchit;
	bool force = false;

	if (projectId.ends("-force")) {
		projectId = projectId.substr(0,projectId.length-6);
		force = true;
	}

	Value doc = db.get(projectId, CouchDB::flgNullIfMissing);;

	QueryParser qp(api_path);

	StrViewA cmd = qp.getPath();


	if (cmd == "status") {

		checkExist(projectId, doc);

		Object result;
		if (doc == nullptr) {
			result("id", projectId)
				  ("status","unknown");
		} else {
			result("id", projectId)
				  ("url",doc["url"])
				  ("branch",doc["branch"])
				  ("status",doc["status"])
				  ("last_error",doc["error_code"])
				  ("stage",doc["build_stage"])
				  ("timestamp",doc["~timestamp"])
				  ("rev",doc["build_rev"])
				  ("build_time",doc["build_time"]);
		}

		req.sendResponse("application/json",Value(result).stringify());

	} else if (cmd == "log.html") {
		sendFormattedAttachment(doc, "log", req);
	} else if (cmd == "log.txt") {
		sendAttachment(doc, "log", req);
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

			req.readBodyAsync(10000,[=](HTTPRequest req) mutable {

				if (!req["Content-Type"].begins("application/json")) {
					req.sendErrorPage(415);
					return;
				}

				auto buffer = req.getUserBuffer();
				Value options;
				StrViewA captcha_code;

				options = Value::fromString(StrViewA(BinaryView(buffer)));
				captcha_code = options["captcha"].getString();
				if (options["force"].defined())
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
				String branch;

				options = Value::fromString(StrViewA(BinaryView(buffer)));
				captcha_code = options["captcha"].getString();
				url = options["url"].toString();
				branch = options["branch"].toString();


				if (!checkCapcha(captcha_code)) {
					req.sendErrorPage(402);
					return;
				}

				if (!checkUrl(url)) {
					req.sendErrorPage(400,StrViewA(),"Invalid URL");
					return;
				}

				if (!checkBranchName(branch)) {
					req.sendErrorPage(400,StrViewA(),"Invalid branch name");
					return;
				}


				Value exist = checkExist(url,branch);
				Value response;
				int code;

				if (exist.defined()) {
					response = Object("status","found")
									("id",exist["_id"])
									("redir", exist["_id"].getString() != projectId);
					if (exist["status"] == "deleted") {
						try {
							rebuild_project(exist, false);
						} catch (...) {

						}
					}
					code = 200;
				} else {
					Document doc;
					doc.setID(url2hash(url,branch));
					doc.set("url",url);
					doc.set("branch",branch);
					rebuild_project(doc,false);
					response = Object("status","created")
									 ("id",doc.getIDValue())
									("redir", doc.getID() != projectId);
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

void ConsolePage::checkExist(const StrViewA &projectId, Value &doc) {
	Value status = doc["status"];
	if (status.getString() == "done") {
		std::string path = storage_path;
		path.append(projectId.data,projectId.length);
		if (access(path.c_str(),0)) {
			Document x(doc);
			x.set("status","deleted");
			x.set("build_rev","");
			try {
				db.put(x);
			} catch (...) {

			}
			doc = x;
		}
	}
}

}
