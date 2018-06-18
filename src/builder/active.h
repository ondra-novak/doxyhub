/*
 * active.h
 *
 *  Created on: Jun 18, 2018
 *      Author: ondra
 */

#ifndef SRC_BUILDER_ACTIVE_H_
#define SRC_BUILDER_ACTIVE_H_
#include <mutex>


template<typename T>
class ActiveObject {
public:
	T *obj = nullptr;
	std::mutex mtx;

	void acquire(T *val) {
		std::lock_guard<std::mutex> _(mtx);
		obj = val;
	}

	void release() {
		std::lock_guard<std::mutex> _(mtx);
		obj = nullptr;
	}

	template<typename Fn>
	bool lock(Fn &&fn) {
		std::lock_guard<std::mutex> _(mtx);
		if (obj == nullptr) return false;
		fn(*obj);
		return true;
	}

	class Guard {
	public:
		Guard(ActiveObject &activeObj, T *obj):owner(activeObj) {
			owner.acquire(obj);
		}
		~Guard() {
			owner.release();
		}
		ActiveObject &owner;

	};
};



#endif /* SRC_BUILDER_ACTIVE_H_ */
