/*
 * url2Hash.cpp
 *
 *  Created on: 14. 10. 2018
 *      Author: ondra
 */

#include <cstdint>
#include <imtjson/fnv.h>
#include <imtjson/binary.h>
#include "url2Hash.h"

namespace doxyhub {

json::String url2hash(const json::StrViewA &url) {

	using namespace json;

	std::uint64_t hash;
	FNV1a64 hasher(hash);
	for (auto &&x:url) hasher(x);

	unsigned char bytes[8];
	for (unsigned int i = 0; i < 8; i++) {
		bytes[i] = static_cast<unsigned char>(hash & 0xFF);
		hash >>= 8;
	}
	return String(base64url->encodeBinaryValue(BinaryView(bytes)));

}

} /* namespace doxyhub */
