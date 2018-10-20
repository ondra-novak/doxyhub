/*
 * initdb.cpp
 *
 *  Created on: Jul 22, 2018
 *      Author: ondra
 */


#include <couchit/couchDB.h>
#include "initdb.h"


using namespace couchit;
using namespace json;


static StrViewA filterFn = R"javascript(
function(doc,req) {
	return doc.url && doc.upload_url && doc.upload_token && doc.status == "queued" && doc.queue == req.query.queueid;
}
)javascript";

static StrViewA queueStats = R"javascript(
function(doc) {
	if (doc.url && doc.queue && (doc.status == "queued" || doc.status == "building")) {
		emit(doc.queue, (doc.build_time && doc.build_time.avg_duration)?doc.build_time.avg_duration:600);
	} else if (doc.type == "queue") {
		emit(doc._id, 0);
	}
}
)javascript";

static StrViewA byURL = R"javascript(

function(doc) {
  
  function begins(t,x) {
    return t.substr(0,x.length) == x;
  }
  function ends(t,x) {
    return t.substr(t.length-x.length) == x;
  }
  
	if (doc.url && doc.status != "error") {
		var url = doc.url;

		if (begins(url,"https://")) url= url.substr(8);
		else if (begins(url,"http://")) url = url.substr(7);
		if (begins(url,"www.")) url = url.substr(4);

		if (ends(url,".git")) url = url.substr(0,url.length-4);
		url = url.toLowerCase();
		emit([url,doc.branch]);
	}
}

)javascript";

static StrViewA curBuilding = R"javascript(

function(doc) {
	if (doc.status && doc.status == "building") emit(doc.queue);
}

)javascript";

static Value queueDesignDoc = Object("_id","_design/queue")
		("language","javascript")
		("filters", Object("queue", filterFn))
		("views", Object
				("stats", Object("map",queueStats)("reduce","_stats"))
				("byURL", Object("map",byURL))
				("building", Object("map",curBuilding))
		);




void initBuildDB(CouchDB& db) {
	db.putDesignDocument(queueDesignDoc);
}
