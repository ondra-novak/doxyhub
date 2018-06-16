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
#include <map>

namespace doxyhub {

using ondra_shared::StrViewA;

class Doxyfile {
public:


	void parse(std::istream &instrm);
	void serialize(std::ostream &outstrm);


	static std::string sanitizePaths(const std::string &value);



	void sanitize(StrViewA targetPath,
				StrViewA projectName,
				StrViewA revision) ;

	void erase(const StrViewA &name);
	std::string &operator[](const StrViewA &name);
	const std::string &operator[](const StrViewA &name) const;



protected:


	typedef std::map<StrViewA,std::string> KeyValueMap;

	KeyValueMap kvmap;

	void parseLine(const std::string &line);
};



} /* namespace doxyhub */

#endif /* SRC_BUILDER_DOXYFILE_H_ */
