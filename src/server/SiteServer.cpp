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
#include <shared/logOutput.h>

namespace doxyhub {

using simpleServer::HeaderValue;
using simpleServer::HTTPResponse;
using simpleServer::Redirect;
using simpleServer::QueryParser;
using ondra_shared::BinaryView;
using ondra_shared::logProgress;



simpleServer::HttpFileMapper SiteServer::mimesrc("","");


bool SiteServer::create_etag(const std::string &pakfile, const StrViewA &path, std::string &etag) {
	std::uint64_t h;
	getContentHash(pakfile, path, h);
	std::ostringstream s;
	s << '\"' << h << '\"';
	etag = s.str();
	return true;
}

void SiteServer::updateArchive(const std::string& name) {
	std::unique_lock<std::mutex> _(mx);
	invalidate(name);
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

	auto splt = qp.getPath().split("/",3);
	splt();
	std::string file = splt();
	StrViewA rev = splt();
	StrViewA path = splt();

	if (rev == "console")
		return console.serve(file, req, vpath.substr(file.length()+rev.length+2));

	if (!req.allowMethods({"HEAD","GET"}))
		return true;

	std::string new_etag;
	HeaderValue match_etag = req["If-None-Match"];

	std::unique_lock<std::mutex> _lock(mx);
	reqcounter++;

	std::string currev;
	if (! getRevision(file,currev)) {
		redirect(req, vpath.length,file,"console","index.html",false);
		return true;
	}

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

SiteServer::PakMap::iterator SiteServer::loadPak(const std::string& name) {
	pak_miss++;
	logProgress("Loading directory: $1  - cache miss ration $2/$3 ($4)", name, pak_miss, reqcounter, pak_miss*100.0/reqcounter);
	mx.unlock();
	return zwebpak::PakManager::loadPak(name);
}

SiteServer::ClusterMap::iterator SiteServer::loadCluster(zwebpak::PakFile& pak,
		const zwebpak::FDirItem& entry, const ClusterKey& id) {

	cluster_miss++;
	logProgress("Loading cluster: $1:$2  - cache miss ration $3/$4 ($5)", *id.first, id.second, cluster_miss, reqcounter, cluster_miss*100.0/reqcounter);
	mx.unlock();
	return zwebpak::PakManager::loadCluster(pak, entry, id);
}

SiteServer::SiteServer(ConsolePage& console, const std::string& rootPath,
		unsigned int pakCacheCnt, unsigned int clusterCacheCnt)
:zwebpak::PakManager(rootPath, pakCacheCnt, clusterCacheCnt),console(console)
{
}

void SiteServer::onLoadDone() noexcept {
	mx.lock();
}

} /* namespace doxyhub */

