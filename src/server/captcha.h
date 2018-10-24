/*
 * captcha.h
 *
 *  Created on: 24. 10. 2018
 *      Author: ondra
 */

#ifndef SRC_SERVER_CAPTCHA_H_
#define SRC_SERVER_CAPTCHA_H_
#include <string>

namespace doxyhub {

class Captcha {
public:
	Captcha(const std::string &processLine);

	bool check(const std::string &code);

protected:
	std::string processLine;
};

} /* namespace doxyhub */

#endif /* SRC_SERVER_CAPTCHA_H_ */
