/*
 * consolePage.h
 *
 *  Created on: 11. 10. 2018
 *      Author: ondra
 */

#pragma once
#include <couchit/couchDB.h>
#include <imtjson/stringview.h>
#include <imtjson/value.h>
#include <simpleServer/http_parser.h>
#include <simpleServer/http_filemapper.h>


namespace doxyhub {
using couchit::CouchDB;
using ondra_shared::StrViewA;
using simpleServer::HTTPRequest;
using simpleServer::HttpFileMapper;
using json::Value;

class ConsolePage {
public:
	ConsolePage(CouchDB &db,
				const std::string &document_root,
				const std::string &upload_url,
				const std::string &storage_path);

	bool serve(StrViewA projectId, HTTPRequest req, StrViewA vpath);


	void run_api(StrViewA projectId, HTTPRequest req, StrViewA api_path);
protected:
	CouchDB &db;
	HttpFileMapper fmapper;
	std::string upload_url;
	std::string storage_path;

	void sendAttachment(Value doc, StrViewA attchname, HTTPRequest req);
	void sendFormattedAttachment(Value doc, StrViewA attchname, HTTPRequest req);
	Value selectQueue();
	Value generate_token();

	void rebuild_project(couchit::Document doc, bool force);
	bool checkUrl(StrViewA url) const;
	Value checkExist(StrViewA url, StrViewA branch) const;
	bool checkCapcha(StrViewA capcha) const;
	bool checkBranchName(StrViewA branch) const;

	void checkExist(const StrViewA &projectId, Value &doc);

	template<typename Fn>
	void sendAttachment(Value doc, StrViewA attchname, HTTPRequest req, Fn &&formatter, StrViewA content_type);

};

} /* namespace doxyhub */

