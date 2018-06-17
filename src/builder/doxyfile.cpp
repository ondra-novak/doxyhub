/*
 * doxyfile.cpp
 *
 *  Created on: 15. 6. 2018
 *      Author: ondra
 */

#include <builder/doxyfile.h>
#include <shared/stringview.h>
#include <imtjson/value.h>
#include <imtjson/parser.h>
#include <imtjson/serializer.h>
#include <sstream>

using ondra_shared::StrViewA;
using ondra_shared::StringView;

namespace doxyhub {

static StrViewA supportedOptions[] = {
		//KEEP ORDERED!!
		"ABBREVIATE_BRIEF",
		"ALIASES",
		"ALLEXTERNALS",
		"ALPHABETICAL_INDEX",
		"ALWAYS_DETAILED_SEC",
		"AUTOLINK_SUPPORT",
		"BRIEF_MEMBER_DESC",
		"BUILTIN_STL_SUPPORT",
		"CALLER_GRAPH",
		"CALL_GRAPH",
		"CITE_BIB_FILES",
		"CLASS_DIAGRAMS",
		"CLASS_GRAPH",
		"COLLABORATION_GRAPH",
		"COLS_IN_ALPHA_INDEX",
		"CPP_CLI_SUPPORT",
		"DIAFILE_DIRS",
		"DIRECTORY_GRAPH",
		"DISABLE_INDEX",
		"DISTRIBUTE_GROUP_DOC",
		"DOTFILE_DIRS",
		"DOT_FONTNAME",
		"DOT_FONTSIZE",
		"DOT_GRAPH_MAX_NODES",
		"DOT_IMAGE_FORMAT",
		"DOT_TRANSPARENT",
		"ENABLED_SECTIONS",
		"ENABLE_PREPROCESSING",
		"ENUM_VALUES_PER_LINE",
		"EXAMPLE_PATH",
		"EXAMPLE_PATTERNS",
		"EXAMPLE_RECURSIVE",
		"EXCLUDE",
		"EXCLUDE_PATTERNS",
		"EXCLUDE_SYMBOLS",
		"EXPAND_AS_DEFINED",
		"EXPAND_ONLY_PREDEF",
		"EXTENSION_MAPPING",
		"EXTERNAL_GROUPS",
		"EXTERNAL_PAGES",
		"EXTERNAL_SEARCH",
		"EXTERNAL_SEARCH_ID",
		"EXT_LINKS_IN_WINDOW",
		"EXTRACT_ALL",
		"EXTRACT_ANON_NSPACES",
		"EXTRACT_LOCAL_CLASSES",
		"EXTRACT_LOCAL_METHODS",
		"EXTRACT_PACKAGE",
		"EXTRACT_PRIVATE",
		"EXTRACT_STATIC",
		"EXTRA_SEARCH_MAPPINGS",
		"FORCE_LOCAL_INCLUDES",
		"FORMULA_FONTSIZE",
		"FORMULA_TRANSPARENT",
		"GENERATE_BUGLIST",
		"GENERATE_DEPRECATEDLIST",
		"GENERATE_HTML",
		"GENERATE_LEGEND",
		"GENERATE_TAGFILE",
		"GENERATE_TESTLIST",
		"GENERATE_TODOLIST",
		"GENERATE_TREEVIEW",
		"GRAPHICAL_HIERARCHY",
		"GROUP_GRAPHS",
		"GROUP_NESTED_COMPOUNDS",
		"HAVE_DOT",
		"HIDE_COMPOUND_REFERENCE",
		"HIDE_FRIEND_COMPOUNDS",
		"HIDE_IN_BODY_DOCS",
		"HIDE_SCOPE_NAMES",
		"HIDE_UNDOC_CLASSES",
		"HIDE_UNDOC_MEMBERS",
		"HIDE_UNDOC_RELATIONS",
		"HTML_COLORSTYLE_GAMMA",
		"HTML_COLORSTYLE_HUE",
		"HTML_COLORSTYLE_SAT",
		"HTML_DYNAMIC_MENUS",
		"HTML_DYNAMIC_SECTIONS",
		"HTML_EXTRA_FILES",
		"HTML_EXTRA_STYLESHEET",
		"HTML_FOOTER",
		"HTML_HEADER",
		"HTML_INDEX_NUM_ENTRIES",
		"HTML_STYLESHEET",
		"IDL_PROPERTY_SUPPORT",
		"IGNORE_PREFIX",
		"IMAGE_PATH",
		"INCLUDED_BY_GRAPH",
		"INCLUDE_FILE_PATTERNS",
		"INCLUDE_GRAPH",
		"INCLUDE_PATH",
		"INHERIT_DOCS",
		"INLINE_GROUPED_CLASSES",
		"INLINE_INFO",
		"INLINE_INHERITED_MEMB",
		"INLINE_SIMPLE_STRUCTS",
		"INLINE_SOURCES",
		"INPUT",
		"INPUT_ENCODING",
		"INTERACTIVE_SVG",
		"INTERNAL_DOCS",
		"JAVADOC_AUTOBRIEF",
		"LAYOUT_FILE",
		"MACRO_EXPANSION",
		"MARKDOWN_SUPPORT",
		"MATHJAX_CODEFILE",
		"MATHJAX_EXTENSIONS",
		"MATHJAX_FORMAT",
		"MATHJAX_RELPATH",
		"MAX_DOT_GRAPH_DEPTH",
		"MAX_INITIALIZER_LINES",
		"MSCFILE_DIRS",
		"MULTILINE_CPP_IS_BRIEF",
		"OPTIMIZE_FOR_FORTRAN",
		"OPTIMIZE_OUTPUT_FOR_C",
		"OPTIMIZE_OUTPUT_JAVA",
		"OPTIMIZE_OUTPUT_VHDL",
		"OUTPUT_LANGUAGE",
		"OUTPUT_TEXT_DIRECTION",
		"PREDEFINED",
		"PROJECT_BRIEF",
		"PROJECT_LOGO",
		"PROJECT_NAME",
		"PROJECT_NUMBER",
		"QT_AUTOBRIEF",
		"RECURSIVE",
		"REFERENCED_BY_RELATION",
		"REFERENCES_LINK_SOURCE",
		"REFERENCES_RELATION",
		"REPEAT_BRIEF",
		"SEARCHDATA_FILE",
		"SEARCHENGINE",
		"SEARCHENGINE_URL",
		"SEARCH_INCLUDES",
		"SEPARATE_MEMBER_PAGES",
		"SHOW_FILES",
		"SHOW_GROUPED_MEMB_INC",
		"SHOW_INCLUDE_FILES",
		"SHOW_NAMESPACES",
		"SHOW_USED_FILES",
		"SIP_SUPPORT",
		"SKIP_FUNCTION_MACROS",
		"SORT_BRIEF_DOCS",
		"SORT_BY_SCOPE_NAME",
		"SORT_GROUP_NAMES",
		"SORT_MEMBER_DOCS",
		"SORT_MEMBERS_CTORS_1ST",
		"SOURCE_BROWSER",
		"SOURCE_TOOLTIPS",
		"STRICT_PROTO_MATCHING",
		"STRIP_CODE_COMMENTS",
		"SUBGROUPING",
		"TAB_SIZE",
		"TAGFILES",
		"TCL_SUBST",
		"TEMPLATE_RELATIONS",
		"TOC_INCLUDE_HEADINGS",
		"TREEVIEW_WIDTH",
		"TYPEDEF_HIDES_STRUCT",
		"UML_LIMIT_NUM_FIELDS",
		"UML_LOOK",
		"USE_HTAGS",
		"USE_MATHJAX",
		"USE_MDFILE_AS_MAINPAGE",
		"VERBATIM_HEADERS",
		"WARN_AS_ERROR",
		"WARN_FORMAT",
		"WARN_IF_DOC_ERROR",
		"WARN_IF_UNDOCUMENTED",
		"WARN_NO_PARAMDOC",
};

static StrViewA optionsWithPath[] = {
		"CITE_BIB_FILES",
		"EXAMPLE_PATH",
		"IMAGE_PATH",
		"LAYOUT_FILE",
		"HTML_HEADER",
		"HTML_FOOTER",
		"HTML_STYLESHEET",
		"HTML_EXTRA_STYLESHEET",
		"HTML_EXTRA_FILES",
		"INCLUDE_PATH",
		"MSCGEN_PATH",
		"DIA_PATH",
		"DOT_FONTPATH",
		"DOTFILE_DIRS",
		"MSCFILE_DIRS",
		"DIAFILE_DIRS",
		"INPUT",
		"PROJECT_LOGO"
};


static std::pair<StrViewA,StrViewA>  predefinedOptions[] = {
		{"CREATE_SUBDIRS","NO"},
		{"ALLOW_UNICODE_NAMES","NO"},
		{"FULL_PATH_NAMES","NO"},
		{"SHORT_NAMES","YES"},
		{"LOOKUP_CACHE_SIZE","4"},
		{"QUIET","NO"},
		{"WARNINGS","YES"},
		{"CLANG_ASSISTED_PARSING","NO"},
		{"GENERATE_HTML","YES"},
		{"HTML_FILE_EXTENSION",".html"},
		{"HTML_TIMESTAMP","YES"},
		{"GENERATE_DOCSET","NO"},
		{"GENERATE_HTMLHELP","NO"},
		{"GENERATE_CHI","NO"},
		{"GENERATE_QHP","NO"},
		{"GENERATE_ECLIPSEHELP","NO"},
		{"GENERATE_LATEX","NO"},
		{"GENERATE_RTF","NO"},
		{"GENERATE_MAN","NO"},
		{"GENERATE_XML","NO"},
		{"GENERATE_DOCBOOK","NO"},
		{"GENERATE_AUTOGEN_DEF","NO"},
		{"GENERATE_PERLMOD","NO"},
		{"DOT_NUM_THREADS","1"},
		{"DOT_MULTI_TARGETS","YES"},
		{"DOT_CLEANUP","YES"}
};


void Doxyfile::parse(std::istream& instrm) {

	std::string line,line2;
	std::getline(instrm, line);
	while (!instrm.eof()) {
		if (!line.empty()) {
			while (line[line.length()-1] == '\\') {
				std::getline(instrm, line2);
				line = line.substr(0,line.length()-1)+line2;
			}
			parseLine(line);
		}
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



static std::string rebuildPath(StrViewA srcPath) {

	std::string tmp;
	if (srcPath.empty()) return tmp;
	if (srcPath[0] == '/') return tmp;
	auto splt = srcPath.split("/");
	while (!!splt) {
		StrViewA part = splt();
		if (part.empty() || part == ".") continue;
		if (part == "..") {
			while (!tmp.empty()) {
				char c = tmp[tmp.length()-1];
				tmp.pop_back();
				if (c == '/') break;
			}
		} else {
			tmp.push_back('/');
			tmp.append(part.data, part.length);
		}
	}

	if (!tmp.empty()) tmp.erase(0);
	return tmp;

}

std::string Doxyfile::sanitizePaths(const std::string &value) {
	std::ostringstream output;
	std::istringstream input(value);

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

			std::string p = rebuildPath(path);
			bool haveWS = false;
			for (char c: p) {
				if (isspace(c)) {
					haveWS= true;
					break;
				}
			}
			output << " ";
			if (haveWS) {
				json::Value(p).toStream(output);
			} else {
				output << p;
			}
		}
		i = input.get();
	}
	return output.str();



}

void Doxyfile::sanitize(StrViewA targetPath,
						StrViewA projectName,
						StrViewA revision)  {

	for (auto &&itm : optionsWithPath) {
		kvmap[itm] = sanitizePaths(kvmap[itm]);
	}

	for (auto &&itm : predefinedOptions) {
		kvmap[itm.first] = itm.second;
	}

	kvmap["OUTPUT_DIRECTORY"] = targetPath;
	std::string &pn = kvmap["PROJECT_NAME"];
	if (pn.empty()) pn = projectName;
	std::string &rv = kvmap["PROJECT_NUMBER"];
	if (rv.empty()) rv = revision;


}

void Doxyfile::erase(const StrViewA& name) {
	kvmap.erase(name);
}

std::string& Doxyfile::operator [](const StrViewA& name) {
	return kvmap[name];
}

const std::string& Doxyfile::operator [](const StrViewA& name) const {
	static std::string empty;

	auto x = kvmap.find(name);
	if (x == kvmap.end()) return empty;
	else return x->second;

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
			key = sl.substr(0,epos).trim(isspace);
			value = sl.substr(epos+1).trim(isspace);
		}
	}

	auto itr = std::lower_bound(std::begin(supportedOptions), std::end(supportedOptions), key);
	if (itr != std::end(supportedOptions) && *itr == key) {

		key = *itr;

		if (append) {
			std::string &x = kvmap[key];
			x.append(" ");
			x.append(value.data, value.length);
		} else {
			kvmap[key] = value;
		}
	}
}

} /* namespace doxyhub */
