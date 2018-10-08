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
		emit(doc.queue, 1);
	} else if (doc.type == "queue") {
		emit(doc._id, 0);
	}
}
)javascript";


static Value queueDesignDoc = Object("_id","_design/queue")
		("language","javascript")
		("filters", Object("queue", filterFn))
		("views", Object("stats", Object("map",queueStats)("reduce","_sum"))
		);



void initBuildDB(CouchDB& db) {
	db.putDesignDocument(queueDesignDoc);
}
