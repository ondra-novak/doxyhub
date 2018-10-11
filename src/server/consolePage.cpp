/*
 * consolePage.cpp
 *
 *  Created on: 11. 10. 2018
 *      Author: ondra
 */

#include <server/consolePage.h>

namespace doxyhub {


doxyhub::ConsolePage::ConsolePage(CouchDB& db,
		const std::string& console_page_url)
	:db(db),console_page_url(console_page_url0)
{
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





}


}
