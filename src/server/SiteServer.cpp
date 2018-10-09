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
#include <simpleServer/query_parser.h>
#include <sstream>

namespace doxyhub {

using simpleServer::HeaderValue;
using simpleServer::HTTPResponse;
using simpleServer::Redirect;
using simpleServer::QueryParser;
using ondra_shared::BinaryView;


simpleServer::HttpFileMapper SiteServer::mimesrc("","");


bool SiteServer::create_etag(const std::string &pakfile, const StrViewA &path, std::string &etag) {
	std::uint64_t h;
	getContentHash(pakfile, path, h);
	std::ostringstream s;
	s << '\"' << h << '\"';
	etag = s.str();
	return true;
}


void SiteServer::redirect(HTTPRequest req, std::size_t vpathSize, const StrViewA &file, const StrViewA &rev, const StrViewA &path, bool permanent) {

	StrViewA fullpath = req.getPath();
	StrViewA begpath = fullpath.substr(0,fullpath.length-vpathSize);
	std::ostringstream buff;
	buff << begpath << "/" << file << "/" << rev << "/" << path;

	req.redirect(buff.str(),permanent?Redirect::permanent:Redirect::temporary);
}

bool SiteServer::serve(HTTPRequest req, StrViewA vpath) {


	QueryParser qp(vpath);

	auto m = req.getMethod();
	if (m != "GET" && m != "HEAD") {
		req.sendResponse(HTTPResponse(405)
						("Allow","GET, HEAD"),"");
		return true;
	}

	auto splt = qp.getPath().split("/",3);
	splt();
	std::string file = splt();
	StrViewA rev = splt();
	StrViewA path = splt();

	std::string new_etag;
	HeaderValue match_etag = req["If-None-Match"];

	std::unique_lock<std::mutex> _lock(mx);

	std::string currev = getRevision(file);
	if (!currev.empty() && rev != StrViewA(currev)) {
		redirect(req, vpath.length,file, StrViewA(currev),path, !rev.empty() && !path.empty());
		return true;
	}

	if (path.empty()) {
		if (req.redirectToFolderRoot(Redirect::permanent)) return true;
		path = "index.html";
	}

	if (!create_etag(file,path,new_etag)) {
		req.sendErrorPage(404);
		return true;
	}

	if (match_etag == StrViewA(new_etag)) {
		req.sendErrorPage(304);
		return true;
	}

	Data d = load(file, path);
	_lock.unlock();
	if (!d.is_valid()) {
		req.sendErrorPage(404);
		return true;
	}


	HTTPResponse resp(200);
	resp("ETag", new_etag);
	resp("Cache-Control","max-age=31536000");
	resp.contentLength(d.length);
	resp.contentType(mimesrc.mapMime(path));
	auto stream = req.sendResponse(resp);
	stream.write(d);
	return true;
}


} /* namespace doxyhub */
