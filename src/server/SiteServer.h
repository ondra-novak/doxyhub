/*
 * SiteServer.h
 *
 *  Created on: 9. 10. 2018
 *      Author: ondra
 */

#ifndef SRC_SERVER_SITESERVER_H_
#define SRC_SERVER_SITESERVER_H_

#include <mutex>
#include <zwebpak/zwebpak.h>

#include <simpleServer/http_parser.h>
#include <simpleServer/http_filemapper.h>
#include <simpleServer/http_pathmapper.h>


#include "consolePage.h"

namespace doxyhub {

using ondra_shared::StrViewA;
using simpleServer::HTTPRequest;
using simpleServer::HTTPMappedHandler;




class SiteServer: public zwebpak::PakManager  {
public:


	SiteServer(ConsolePage &console, const std::string &rootPath
			,unsigned int pakCacheCnt,unsigned int clusterCacheCnt);



	bool serve(HTTPRequest req, StrViewA vpath);
	void updateArchive(const std::string &name);
protected:
	static simpleServer::HttpFileMapper mimesrc;
	std::mutex mx;
	ConsolePage &console;

	bool create_etag(const std::string &pakfile, const StrViewA &path, std::string &etag);
	void redirect(HTTPRequest req, std::size_t vpathSize, const StrViewA &file, const StrViewA &rev, const StrViewA &path, bool permanent);


	virtual PakMap::iterator loadPak(const std::string &name);
	virtual ClusterMap::iterator loadCluster(zwebpak::PakFile &pak, const zwebpak::FDirItem &entry, const ClusterKey &id);

	std::size_t reqcounter = 0;
	std::size_t pak_miss = 0;
	std::size_t cluster_miss = 0;


	void runConsole(const StrViewA &file, HTTPRequest req, StrViewA path);

};

} /* namespace doxyhub */

#endif /* SRC_SERVER_SITESERVER_H_ */
