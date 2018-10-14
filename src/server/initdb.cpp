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
		emit(doc.queue, (doc.build_time && doc.build_time.duration)?doc.build_time.duration:90);
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
  
	if (doc.url) {
		var url = doc.url;

		if (begins(url,"https://")) url= url.substr(8);
		else if (begins(url,"http://")) url = url.substr(7);
		
		if (ends(url,".git")) url = url.substr(0,url.length-4);
		url = url.toLowerCase();
		emit(url);
	}
}

)javascript";

static Value queueDesignDoc = Object("_id","_design/queue")
		("language","javascript")
		("filters", Object("queue", filterFn))
		("views", Object
				("stats", Object("map",queueStats)("reduce","_stats"))
				("byURL", Object("map",byURL))
		);




void initBuildDB(CouchDB& db) {
	db.putDesignDocument(queueDesignDoc);
}
