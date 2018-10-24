/*
 * captcha.cpp
 *
 *  Created on: 24. 10. 2018
 *      Author: ondra
 */

#include "captcha.h"
#include <common/process.h>
#include <cstdio>


namespace doxyhub {


doxyhub::Captcha::Captcha(const std::string& processLine):processLine(processLine) {
}

bool doxyhub::Captcha::check(const std::string& code) {
	FILE *f = popen(processLine.c_str(), "w");
	if (f) {
		fprintf(f,"%s\n", code.c_str());
		return pclose(f) == 0;
	}
	return false;
}


} /* namespace doxyhub */
