/*
 * url2Hash.h
 *
 *  Created on: 14. 10. 2018
 *      Author: ondra
 */

#ifndef SRC_SERVER_URL2HASH_H_
#define SRC_SERVER_URL2HASH_H_

#include <imtjson/string.h>

namespace doxyhub {




json::String url2hash(const json::StrViewA &url,const json::StrViewA &branch);

} /* namespace doxyhub */

#endif /* SRC_SERVER_URL2HASH_H_ */
