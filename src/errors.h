/*
 * errors.h
 *
 *  Created on: 20. 10. 2018
 *      Author: ondra
 */

#ifndef SRC_ERRORS_H_
#define SRC_ERRORS_H_
#include <stdexcept>
#include <string>

namespace doxyhub {

enum class DoxyhubError {

	ok = 0,
	internal_error = 600,
	not_modified = 603,
	no_space = 605,
	git_clone_failed = 610,
	git_clone_timeout = 611,
	build_failed = 620,
	build_timeout = 621,
	upload_failed = 630,

	internal_restart=700




};



class BuildError: public std::exception {
public:
	BuildError(DoxyhubError code):code(code) {}

	const char *what() const noexcept override{
		if (msg.empty()) generateMsg();
		return msg.c_str();
	}

protected:
	DoxyhubError code;
	mutable std::string msg;

	void generateMsg() const {
		push_number(static_cast<int>(code));
		msg.push_back(' ');
		msg.append(getMessageByCode(code));
	}

	void push_number(int code) const {
		if (code) {
			push_number(code/10);
			msg.push_back('0'+code%10);
		}
	}

	static const char *getMessageByCode(DoxyhubError err) {
		switch (err) {
			case DoxyhubError::internal_error: return "Internal generato error";
			case DoxyhubError::not_modified: return "Not modified";
			default: return "Undefined error code";
		}
	}

};

}



#endif /* SRC_ERRORS_H_ */
