/*
 * bldcontrol.h
 *
 *  Created on: Jul 21, 2018
 *      Author: ondra
 */

#ifndef SRC_SERVER_BLDCONTROL_H_
#define SRC_SERVER_BLDCONTROL_H_
#include <couchit/couchDB.h>
#include <imtjson/rpc.h>

namespace doxyhub {



class BldControl {
public:
	BldControl(couchit::CouchDB &blddb);

	void init(json::RpcServer &rpc);


	void searchProject(json::RpcRequest req);
	void buildProject(json::RpcRequest req);
	void statusProject(json::RpcRequest req);
	void projectBuildReport(json::RpcRequest req);

protected:
	couchit::CouchDB &blddb;

	ondra_shared::StrViewA checkLink(json::RpcRequest req);
	json::Value selectQueue();
};

} /* namespace doxyhub */

#endif /* SRC_SERVER_BLDCONTROL_H_ */
