/*
 * search.h
 *
 *  Created on: 14. 10. 2018
 *      Author: ondra
 */

#ifndef SRC_SERVER_SEARCH_H_
#define SRC_SERVER_SEARCH_H_
#include <couchit/couchDB.h>
#include <imtjson/value.h>

namespace doxyhub {

using namespace json;
using namespace couchit;


String normalizeUrl(StrViewA url);
Value searchByUrl(CouchDB &db, StrViewA url, StrViewA branch);
Value searchStatusByUrl(CouchDB &db, StrViewA url, StrViewA branch);

}




#endif /* SRC_SERVER_SEARCH_H_ */
