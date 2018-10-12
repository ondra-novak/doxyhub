/*
 * consolePage.cpp
 *
 *  Created on: 11. 10. 2018
 *      Author: ondra
 */

#include <imtjson/value.h>
#include <server/consolePage.h>
#include <shared/logOutput.h>
#include <simpleServer/http_parser.h>

using json::Value;
using ondra_shared::logDebug;
using simpleServer::HTTPResponse;

namespace doxyhub {





doxyhub::ConsolePage::ConsolePage(CouchDB& db,
		const std::string& console_page_url)
	:db(db),console_page_url(console_page_url)
{
	auto fn = create_calc_fn(10);
	logDebug("Result $1", fn(5));
}

bool doxyhub::ConsolePage::operator ()(HTTPRequest req, StrViewA vpath) {

	auto splt = vpath.split("/");
	splt();
	StrViewA id = splt();

	Value doc = db.get(id, CouchDB::flgNullIfMissing);
	if (doc == nullptr) {
		req.sendErrorPage(404);
		return true;
	}

	std::string wholeurl = console_page_url+"#"+id;

	HTTPResponse resp(404);





}


}