/*
 * SiteServer.cpp
 *
 *  Created on: 9. 10. 2018
 *      Author: ondra
 */

#include "SiteServer.h"
#include <cstdint>
#include <mutex>
#include <imtjson/fnv.h>
#include <sstream>

namespace doxyhub {

using simpleServer::HeaderValue;
using simpleServer::HTTPResponse;
using ondra_shared::BinaryView;


simpleServer::HttpFileMapper SiteServer::mimesrc("","");

std::string calc_etag(BinaryView data) {
	std::uint64_t h = 0;
	FNV1a64 hash(h);
	for (auto &&x: data) hash(x);
	std::ostringstream buff;
	buff << '"' << h << '"';
	return buff.str();

}


bool SiteServer::serve(HTTPRequest req, StrViewA vpath) {



	auto m = req.getMethod();
	if (m != "GET" && m != "HEAD") {
		req.sendResponse(HTTPResponse(405)
						("Allow","GET, HEAD"),"");
		return true;
	}

	auto splt = vpath.split("/",3);
	splt();
	StrViewA file = splt();
	StrViewA path = splt();

	std::unique_lock<std::mutex> _lock(mx);
	Data d = load(file, path);
	_lock.unlock();
	if (!d.is_valid()) {
		req.sendErrorPage(404);
		return true;
	}


	HTTPResponse resp(200);

	HeaderValue etag = req["If-None-Match"];


	if (etag.defined()) {
		std::string curtag = calc_etag(d);
		if (etag == StrViewA(curtag)) {
			req.sendErrorPage(304);
			return true;
		}

		resp("ETag", curtag);
	}

	resp.contentLength(d.length);
	resp.contentType(mimesrc.mapMime(path));
	auto stream = req.sendResponse(resp);
	stream.write(d);
	return true;
}


} /* namespace doxyhub */
