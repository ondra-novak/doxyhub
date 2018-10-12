/*
 * consolePage.h
 *
 *  Created on: 11. 10. 2018
 *      Author: ondra
 */

#pragma once
#include <couchit/couchDB.h>
#include <imtjson/stringview.h>
#include <simpleServer/http_parser.h>


namespace doxyhub {
using couchit::CouchDB;
using ondra_shared::StrViewA;
using simpleServer::HTTPRequest;

class ConsolePage {
public:
	ConsolePage(CouchDB &db, const std::string &console_page_url);

	bool operator()(HTTPRequest req, StrViewA vpath);

protected:
	CouchDB &db;
	std::string console_page_url;
};

} /* namespace doxyhub */

