/*
 * upload.h
 *
 *  Created on: 11. 10. 2018
 *      Author: ondra
 */

#pragma once
#include <string>
#include <couchit/couchDB.h>
#include <imtjson/stringview.h>
#include <simpleServer/http_parser.h>

namespace doxyhub {

class UploadHandler {
public:

	typedef std::function<void(std::string)> Notify;

	UploadHandler(couchit::CouchDB &builderDB, const std::string &storage_path, const Notify &onUpdate);

	bool serve(simpleServer::HTTPRequest req, ondra_shared::StrViewA vpath);



protected:
	couchit::CouchDB &db;
	std::string storage_path;
	Notify onUpdate;

};


}



