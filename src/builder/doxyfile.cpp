/*
 * doxyfile.cpp
 *
 *  Created on: 15. 6. 2018
 *      Author: ondra
 */

#include <builder/doxyfile.h>
#include <shared/stringview.h>
#include <imtjson/value.h>
#include <sstream>

using ondra_shared::StrViewA;

namespace doxyhub {

using ondra_shared::StrViewA;


void Doxyfile::parse(std::istream& instrm) {

	std::string line;
	std::getline(instrm, line);
	while (!instrm.eof()) {
		parseLine(line);
		std::getline(instrm, line);
	}
	if (!line.empty()) {
		parseLine(line);
	}
}

void Doxyfile::serialize(std::ostream& outstrm) {

	for (auto &&kv : kvmap) {
		outstrm << kv.first << "=" << kv.second << std::endl;
	}

}


static StrViewA eraseItems[] = {
		"DOXYFILE_ENCODING",
		"CREATE_SUBDIRS",
		"ALLOW_UNICODE_NAMES",
		"FILE_VERSION_FILTER",
		"CITE_BIB_FILES",
		"QUIET",
		"WARNINGS",
		"WARN_LOGFILE",
		"INPUT_FILTER",
		"FILTER_PATTERNS",
		"FILTER_SOURCE_FILES",
		"FILTER_SOURCE_PATTERNS",
		"CLANG_ASSISTED_PARSING",
		"CLANG_OPTIONS",
		"GENERATE_DOCSET",
		"GENERATE_HTMLHELP",
		"GENERATE_QHP",
		"GENERATE_ECLIPSEHELP",
		"PERL_PATH",
		"MSCGEN_PATH",
		"DIA_PATH",
		"DOT_PATH",
		"PLANTUML_JAR_PATH",
};

static StrViewA sanitizeItems[] = {
		"LAYOUT_FILE",
		"EXAMPLE_PATH",
		"IMAGE_PATH",
		"HTML_HEADER",
		"HTML_FOOTER",
		"HTML_STYLESHEET",
		"HTML_EXTRA_STYLESHEET",
		"HTML_EXTRA_FILES",
		"INCLUDE_PATH",
		"GENERATE_TAGFILE",
		"DOTFILE_DIRS",
		"MSCFILE_DIRS",
		"DIAFILE_DIRS"
};

void sanitizePaths(std::string &paths) {
	std::ostringstream output;
	std::istringstream input(paths);
	json::Value tmp;
	std::string tmp2;

	int i = input.get();
	while (i != EOF) {
		if (!isspace(i)) {

			StrViewA path;

			if (i == '"') {
				input.putback(i);
				try {
					tmp = json::Value::fromStream(input);
					path = tmp.getString();

				} catch (...) {
					break;
				}
			} else {
				input.putback(i);
				input >> tmp2;
				path = tmp2;
			}

			if (path[0] != '/' && path.indexOf("/..") == path.npos && path.indexOf("../") == path.npos) {
				bool hasWS = false;
				for (auto x: path) hasWS = hasWS || isspace(x);
				output << " ";
				if (hasWS) json::Value(path).toStream(output);
				else output << path;
			}

		}
		i = input.get();
	}
	paths = output.str();
}

void Doxyfile::sanitize(StrViewA sourcePath, StrViewA targetPath) {


	for (auto x: eraseItems) kvmap.erase(x);
	for (auto x: sanitizeItems) sanitizePaths(kvmap[x]);
	kvmap["OUTPUT_DIRECTORY"] = targetPath;
	kvmap["SHORT_NAMES"]="YES";
	kvmap["LOOKUP_CACHE_SIZE"]="4";
	kvmap["INPUT"]=sourcePath;
	kvmap["EXCLUDE_SYMLINKS"]="YES";
	kvmap["GENERATE_HTML"] = "YES";
	kvmap["HTML_OUTPUT"] = "html";
	kvmap["HTML_FILE_EXTENSION"] = ".html";
	kvmap["HTML_TIMESTAMP"] = "YES";
	kvmap["GENERATE_LATEX"] = "NO";
	kvmap["GENERATE_RTF"] = "NO";
	kvmap["GENERATE_MAN"] = "NO";
	kvmap["GENERATE_XML"] = "NO";
	kvmap["GENERATE_DOCBOOK"] = "NO";
	kvmap["GENERATE_AUTOGEN_DEF"] = "NO";
	kvmap["GENERATE_PERLMOD"] = "NO";
	kvmap["DOT_NUM_THREADS"] = "1";
	kvmap["DOT_CLEANUP"] = "YES";
	kvmap["DOT_MULTI_TARGETS"] = "YES";


}

void Doxyfile::parseLine(const std::string& line) {
	StrViewA sl(line);
	sl = sl.trim(isspace);
	StrViewA key;
	StrViewA value;
	bool append;

	if (sl[0] == '#') return;

	auto apos = sl.indexOf("+=");
	if (apos != sl.npos) {
		append = true;
		key = sl.substr(0,apos).trim(isspace);
		value = sl.substr(apos+2).trim(isspace);
	} else {
		auto epos = sl.indexOf("=");
		if (epos != sl.npos) {
			append = false;
			key = sl.substr(0,apos).trim(isspace);
			value = sl.substr(apos+1).trim(isspace);
		}
	}


	if (append) {
		std::string &x = kvmap[key];
		x.append(" ");
		x.append(value.data, value.length);
	} else {
		kvmap[key] = value;
	}

}

} /* namespace doxyhub */
