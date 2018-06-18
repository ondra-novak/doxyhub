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
using ondra_shared::logProgress;
using ondra_shared::WaitableEvent;
using ondra_shared::AbstractLogProviderFactory;


static StrViewA filterFn = R"javascript(
function(doc,req) {
	return doc.url && (doc.status == "queued" || doc.status == "delete") && doc.queue == req.query.queueid;
}

)javascript";

static Value queueDesignDoc = Object("_id","_design/queue")
		("language","javascript")
		("filters", Object("queue", filterFn));


Queue::Queue(Builder &bld, CouchDB &db, const std::string &queueId)
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
		distr.add(*this);
		distr.includeDocs();
		distr.setFilter(Filter("queue/queue"));
		distr.arg("queueid",queueId);
}

Queue::~Queue() {

}


void Queue::onChange(const ChangedDoc& doc) {
	buildWorker >> [doc = ChangedDoc(doc),this] {

		AbstractLogProviderFactory *logProvider = AbstractLogProviderFactory::getInstance();
		if (logProvider) logProvider->reopenLogs();

		if (exitPhase) return;
		processChange(doc);
	};
}

void Queue::processChange(const ChangedDoc &doc) {
		try {
			queueLastID.set("lastId", doc.seqId);
			db.put(queueLastID);
			Document curDoc(doc.doc);
			logProgress("Build started: doc=$1, url=$2, status=$3",
					curDoc.getID(),curDoc["url"].getString(),curDoc["status"].getString()
			);
			auto startTime = std::chrono::system_clock::now();

			if (curDoc["status"] == "delete") {
				bld.deleteDoc(curDoc.getID());
				curDoc.unset("disksize");
				curDoc.unset("build_rev");
				curDoc.unset("error");
				curDoc.deleteAttachment("stdout");
				curDoc.deleteAttachment("stderr");
				curDoc.set("status","deleted");
			} else if (curDoc["status"] == "queued") {
				curDoc.set("status","building");
				curDoc.unset("error");
				curDoc.object("build_time").set("start",(std::size_t)std::chrono::system_clock::to_time_t(startTime));
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
				auto endTime = std::chrono::system_clock::now();
				curDoc.object("build_time")
						.set("end",(std::size_t)std::chrono::system_clock::to_time_t(endTime))
						.set("duration",std::chrono::duration_cast<std::chrono::seconds>(endTime-startTime).count());
			}

			logProgress("Build finished : doc=$1, url=$2, status=$3",
					curDoc.getID(),curDoc["url"].getString(),curDoc["status"].getString()
			);


			put_merge(curDoc);

		} catch (std::exception &e) {
			logError("Building exception $1", e.what());
		}
}

void Queue::run() {
	exitPhase = false;
	buildWorker = Worker::create(1);

	distr.runService([=]{
			try {
				throw;
			} catch (std::exception &e) {
				logError("Database queue monitoring failure $1", e.what());
			}
			std::this_thread::sleep_for(std::chrono::seconds(2));
			return true;
		});
	logProgress("queue started");
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

	logProgress("queue stopped");
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
