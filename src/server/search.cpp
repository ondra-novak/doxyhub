/*
 * search.cpp
 *
 *  Created on: 14. 10. 2018
 *      Author: ondra
 */

#include <couchit/query.h>
#include <server/url2Hash.h>
#include "search.h"

namespace doxyhub {

static View byURL("_design/queue/_view/byURL",View::includeDocs);

String normalizeUrl(StrViewA url) {
	if (url.begins("http://")) url = url.substr(7);
	else if (url.begins("https://")) url = url.substr(8);

	if (url.begins("www.")) url = url.substr(4);
	if (url.ends(".git")) url = url.substr(0,url.length-4);
	return String(url.length, [&](char *beg) {
		char *c = beg;
		for (auto &x : url) {
			*c++ = tolower(x);
		}
		return c-beg;
	});
}

Value searchByUrl(CouchDB &db, StrViewA url, StrViewA branch) {

	String norm_url = normalizeUrl(url);

	Query q = db.createQuery(byURL);
	q.key({norm_url,branch});
	Result res = q.exec();
	if (res.empty()) return Value();
	else return Row(res[0]).doc;
}

Value searchStatusByUrl(CouchDB &db, StrViewA url, StrViewA branch) {

	Value x = searchByUrl(db,url, branch);
	if (!x.defined()) {

		return Object("id",url2hash(url,branch))
					 ("found",false);

	} else {
		return Object("id", x["_id"])
					 ("found",true);
	}

}



}


