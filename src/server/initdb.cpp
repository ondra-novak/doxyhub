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
	return doc.url && (doc.status == "queued" || doc.status == "delete") && doc.queue == req.query.queueid;
}
)javascript";

static StrViewA queueStats = R"javascript(
function(doc) {
	if (doc.url && doc.queue && doc.disksize) emit(doc.queue, doc.disksize);
}
)javascript";

static StrViewA queueList = R"javascript(
function(doc) {
	if (doc.type=="queue") emit(doc._id, doc.space);
}
)javascript";

static Value queueDesignDoc = Object("_id","_design/queue")
		("language","javascript")
		("filters", Object("queue", filterFn))
		("views", Object("stats", Object("map",queueStats)("reduce","_sum"))
				        ("list", Object("map",queueList)("reduce","_sum"))
		);



void initBuildDB(CouchDB& db) {
	db.putDesignDocument(queueDesignDoc);
}
