/*
 * queue.h
 *
 *  Created on: Jun 17, 2018
 *      Author: ondra
 */

#ifndef SRC_BUILDER_QUEUE_H_
#define SRC_BUILDER_QUEUE_H_

#include "builder.h"
#include <couchit/changeObserver.h>
#include <couchit/changes.h>
#include <couchit/couchDB.h>
#include <couchit/document.h>
#include <shared/worker.h>

namespace doxyhub {

using ondra_shared::Worker;

using namespace couchit;

class Queue: public couchit::IChangeObserver {
public:
	Queue(Builder &bld, CouchDB &db, const std::string &queueId);
	~Queue();

	virtual void onChange(const ChangeEvent &doc);

	virtual Value getLastKnownSeqID() const;

	void run();
	void stop();

protected:
	Builder &bld;
	CouchDB &db;
	ChangesDistributor distr;
	Document queueLastID;
	Worker buildWorker;
	bool exitPhase;

	void put_merge(Document &doc);

	void processChange(const ChangeEvent &doc);
};

} /* namespace doxyhub */

#endif /* SRC_BUILDER_QUEUE_H_ */
