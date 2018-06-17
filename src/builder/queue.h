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

namespace doxyhub {

using namespace couchit;

class Queue: public couchit::IChangeObserver {
public:
	Queue(Builder &bld, CouchDB &db);


	virtual void onChange(const ChangedDoc &doc);

	virtual Value getLastKnownSeqID() const;

	void run();
	void stop();

protected:
	Builder &bld;
	CouchDB &db;
	ChangesDistributor distr;
	Document queueLastID;

	void put_merge(Document &doc);
};

} /* namespace doxyhub */

#endif /* SRC_BUILDER_QUEUE_H_ */
