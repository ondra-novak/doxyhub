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
namespace doxyhub {

using ondra_shared::StrViewA;
using simpleServer::HTTPRequest;

class SiteServer: public zwebpak::PakManager  {
public:
	using zwebpak::PakManager::PakManager;


	bool serve(HTTPRequest req, StrViewA vpath);
protected:
	static simpleServer::HttpFileMapper mimesrc;
	std::mutex mx;

	bool create_etag(const std::string &pakfile, const StrViewA &path, std::string &etag);
	void redirect(HTTPRequest req, std::size_t vpathSize, const StrViewA &file, const StrViewA &rev, const StrViewA &path, bool permanent);

};

} /* namespace doxyhub */

#endif /* SRC_SERVER_SITESERVER_H_ */
