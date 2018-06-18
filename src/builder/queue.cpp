/*
 * queue.cpp
 *
 *  Created on: Jun 17, 2018
 *      Author: ondra
 */

#include <builder/queue.h>
#include <shared/logOutput.h>
#include <shared/waitableEvent.h>
#include <thread>


namespace doxyhub {

using ondra_shared::logError;
using ondra_shared::WaitableEvent;

static StrViewA filterFn = R"javascript(
function(doc) {
	return doc.url && (doc.status == "queued" || doc.status == "delete");
}

)javascript";

static Value queueDesignDoc = Object("_id","_design/queue")
		("language","javascript")
		("filters", Object("queue", filterFn));


Queue::Queue(Builder &bld, CouchDB &db)
	:bld(bld)
	,db(db)
	,distr(db.createChangesFeed())
	,queueLastID(db.getLocal("queueLastID", CouchDB::flgCreateNew))
{
	exitPhase = false;

		db.putDesignDocument(queueDesignDoc);
		if (!queueLastID["lastId"].defined()) {
			queueLastID.set("lastId",nullptr);
		}
		distr.add(this,false);
		distr.includeDocs();
		distr.setFilter(Filter("queue/queue"));
}

Queue::~Queue() {

}

void Queue::onChange(const ChangedDoc& doc) {
	buildWorker >> [doc = ChangedDoc(doc),this] {
		if (exitPhase) return;
		try {
			queueLastID.set("lastId", doc.seqId);
			db.put(queueLastID);
			Document curDoc(doc.doc);
			if (curDoc["status"] == "delete") {
				bld.deleteDoc(curDoc.getID());
				curDoc.unset("disksize");
				curDoc.unset("build_rev");
				curDoc.unset("error");
				curDoc.set("status","deleted");
			} else if (curDoc["status"] == "queued") {
				curDoc.set("status","building");
				curDoc.unset("error");
				put_merge(curDoc);
				try {
					bld.buildDoc(curDoc["url"].getString(),
							curDoc.getID(), curDoc["build_rev"].getString());
					curDoc.set("disksize", bld.calcSize(curDoc.getID()));
					curDoc.set("status","done");
					curDoc.unset("error");
					curDoc.set("build_rev", StrViewA(bld.revision));
					put_merge(curDoc);
				} catch (std::exception &e) {
					if (exitPhase) {
						curDoc.set("status","queued");
					} else {
						curDoc.set("status","error")
								  ("error",e.what());
					}
				}
				curDoc.inlineAttachment("stdout",AttachmentDataRef(
						BinaryView(StrViewA(bld.log)),"text/plain"
				));
				curDoc.inlineAttachment("stderr",AttachmentDataRef(
						BinaryView(StrViewA(bld.warnings)),"text/plain"
				));
			}
			put_merge(curDoc);

		} catch (std::exception &e) {
			logError("Building exception $1", e.what());
		}
	};
}

void Queue::run() {
	exitPhase = false;
	buildWorker = Worker::create(1);

	distr.runService([=]{
			try {
				throw;
			} catch (std::exception &e) {
				logError("Database queue monitoring failure %1", e.what());
			}
			std::this_thread::sleep_for(std::chrono::seconds(2));
			return true;
		});
}

void Queue::stop() {
	distr.stopService();
	exitPhase = true;
	WaitableEvent ev;
	bld.stopTools();
	buildWorker >> [&]{
		ev.signal();
	};
	while (!ev.wait_for(std::chrono::seconds(1))) {
		bld.stopTools();
	}

}

void Queue::put_merge(Document &doc) {
	try {
		db.put(doc);
	} catch (UpdateException &e) {
		auto err = e.getError(0);
		if (err.isConflict()) {
			doc.setBaseObject(db.get(doc.getID()));
			put_merge(doc);
		}
	}
}

Value Queue::getLastKnownSeqID() const {
	return queueLastID["lastId"];
}


} /* namespace doxyhub */
