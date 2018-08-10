/*
 * bldcontrol.cpp
 *
 *  Created on: Jul 21, 2018
 *      Author: ondra
 */

#include "bldcontrol.h"
#include <openssl/ripemd.h>
#include "shared/stringview.h"
#include "shared/logOutput.h"
#include <couchit/document.h>
#include <couchit/couchDB.h>
#include <couchit/query.h>
#include <couchit/query.tcc>



namespace doxyhub {

using ondra_shared::BinaryView;
using ondra_shared::logError;
using ondra_shared::logDebug;
using namespace couchit;
using namespace json;




BldControl::BldControl(CouchDB& blddb):blddb(blddb) {
}

void BldControl::init(RpcServer& rpc) {

	rpc.add("search",this,&BldControl::searchProject);
	rpc.add("build",this,&BldControl::buildProject);
	rpc.add("status",this,&BldControl::statusProject);
	rpc.add("buildReport",this,&BldControl::projectBuildReport);

}

static String calcHash(StrViewA text) {

	std::vector<unsigned char> buff;
	buff.reserve(text.length);
	for (char c:text) {
		buff.push_back(tolower(c));
	}


	BinaryView b(buff.data(), buff.size());
	unsigned char digest[RIPEMD160_DIGEST_LENGTH];
	RIPEMD160(b.data, b.length,digest);
	return base64url->encodeBinaryValue(BinaryView(digest, sizeof(digest))).toString();
}

ondra_shared::StrViewA BldControl::checkLink(json::RpcRequest req) {

	StrViewA link = req[0].getString().trim(isspace);
	if (link.substr(0,7) != "http://" && link.substr(0,8) != "https://") {
		req.setError(501,"Unsupported schema");
		return StrViewA();
	}

	return link;


}

void BldControl::searchProject(json::RpcRequest req) {
	if (!req.checkArgs(Value(json::array,{"string"}))) return req.setArgError();

	StrViewA link = checkLink(req);
	if (link.empty()) return;
	String h = calcHash(link);

	Value doc = blddb.get(h, CouchDB::flgNullIfMissing);
	if (doc == nullptr) {
		req.setResult(Object("_id",h)
				("url", link)
				("status","new"));
	} else {
		req.setResult(Object("_id",doc["_id"])
				("url", doc["url"])
				("status",doc["status"])
				("queue",doc["queue"])
		);
	}
}

void BldControl::buildProject(json::RpcRequest req) {
	if (!req.checkArgs(Value(json::array,{"string"}))) return req.setArgError();

	StrViewA link = checkLink(req);
	if (link.empty()) return;
	String h = calcHash(link);

	Document doc = blddb.get(h, CouchDB::flgCreateNew);
	StrViewA status = doc["status"].getString();
	if (status != "done" && status != "deleted" && status != "") {
		return req.setError(402,"Busy");
	}
	StrViewA queue = doc["queue"].getString();
	if (queue == "" || status == "deleted") {
		Value newq = selectQueue();
		doc.set("queue", newq);
	}
	doc.set("url",link);
	doc.set("status","queued");
	blddb.put(doc);
	req.setResult(h);

}

void BldControl::statusProject(json::RpcRequest req) {
	if (!req.checkArgs(Value(json::array,{"string"}))) return req.setArgError();

	StrViewA id = req[0].getString();
	Value doc = blddb.get(id, CouchDB::flgNullIfMissing);
	if (doc == nullptr) {
		req.setError(404,"Not found");
	} else {
		req.setResult(Object("build_time",doc["build_time"])
							("cluster",doc["queue"])
							("status",doc["status"])
							("error",doc["error"])
							("url",doc["url"])
							);
	}
}

Value BldControl::selectQueue() {
	Query stats = blddb.createQuery(View("_design/queue/_view/stats", View::groupLevel*1| View::reduce));
	Query space = blddb.createQuery(View("_design/queue/_view/list"));
	Query joins = space.join(stats,
			[&](Row row){return row.key;},
			[&](Array arr){return Row(arr[0]).value;},
			[&](Row space, Row stat){
				return space.replace(Path::root/"value", Object("total",space.value)
						                                 ("used",stat.value)
														 ("avail",space.value.getInt()-stat.value.getInt()));
			 });
	Result res = joins.exec();
	std::intptr_t maxAvail = 0;
	Value bestQueue;
	for (Row rw: res) {
		logDebug("Queue stats: $1 = $2", rw.key.toString(), rw.value.toString());
		auto avail = rw.value["avail"].getInt();
		if (avail > maxAvail) {
			bestQueue = rw.key;
		}
	}
	return bestQueue;
}

static String downloadToString(CouchDB &db, StrViewA id, StrViewA name) {
	Download dwn = db.getAttachment(id,name);
	String outstr (dwn.length, [&](char *p){
		dwn.read(p, dwn.length);
		return dwn.length;
	});
	return outstr;
}

void BldControl::projectBuildReport(json::RpcRequest req) {
	if (!req.checkArgs(Value(json::array,{"string"}))) return req.setArgError();

	StrViewA id = req[0].getString();
	try {
		String outstr = downloadToString(blddb, id, "stdout");
		String errstr = downloadToString(blddb, id, "stderr");
		req.setResult(Object("stdout",outstr)("stderr",errstr));
	} catch (RequestError &e) {
		if (e.getCode() == 404) {
			req.setError(404,"Not found");
		} else {
			logError("Exception reading attachment: $1", e.what());
			req.setError(RpcServer::errorInternalError,"Internal error");
		}
	}
}



} /* namespace doxyhub */

