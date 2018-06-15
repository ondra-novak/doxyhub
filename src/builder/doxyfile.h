/*
 * doxyfile.h
 *
 *  Created on: 15. 6. 2018
 *      Author: ondra
 */

#ifndef SRC_BUILDER_DOXYFILE_H_
#define SRC_BUILDER_DOXYFILE_H_

#include <shared/stringview.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace doxyhub {

using ondra_shared::StrViewA;

class Doxyfile {
public:


	void parse(std::istream &instrm);
	void serialize(std::ostream &outstrm);

	void sanitize(StrViewA sourcePath, StrViewA targetPath);


protected:


	typedef std::unordered_map<std::string,std::string> KeyValueMap;

	KeyValueMap kvmap;

	void parseLine(const std::string &line);
};

} /* namespace doxyhub */

#endif /* SRC_BUILDER_DOXYFILE_H_ */
