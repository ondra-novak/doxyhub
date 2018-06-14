/*
 * b\uilder.cpp
 *
 *  Created on: 14. 6. 2018
 *      Author: ondra
 */

#include <unistd.h>
#include <builder/builder.h>
#include <dirent.h>
#include <shared/raii.h>
#include <simpleServer/exceptions.h>
#include <sys/stat.h>
#include <sstream>


namespace doxyhub {

using simpleServer::SystemException;
using ondra_shared::RAII;
using ondra_shared::StrViewA;

Builder::Builder(const Config& cfg, EnvVars envVars):cfg(cfg),envVars(envVars) {

}


static bool isDots(const char *name) {
	return name[0] == '.' && ((name[1] == '.' && name[2] == 0)||name[1] == 0);
}

void myclosedir(DIR *d){
	closedir(d);
};


void recursive_erase(std::string path) {

	typedef RAII<DIR *, decltype(&myclosedir), &myclosedir> Dir;

	DIR *dirptr = opendir(path.c_str());
	if (dirptr == nullptr) return;

	Dir dir(dirptr);


	dirent dent, *entry;

	readdir_r(dir,&dent,&entry);
	auto pathlen = path.length();
	while (entry) {
		if (!isDots(entry->d_name)) {
			path.resize(pathlen);
			path.push_back('/');
			path.append(entry->d_name);
			if (entry->d_type == DT_DIR) {
				recursive_erase(path);
				if (rmdir(path.c_str())) {
					int err = errno;
					throw SystemException(err,"Cannot erase directory:" + path);
				}
			} else {
				if (unlink(path.c_str())) {
					int err = errno;
					throw SystemException(err,"Cannot erase file:" + path);
				}
			}
		}
		readdir_r(dir,&dent,&entry);
	}
}

static void makeDir(const std::string &name) {
	struct stat statBuff;
	if (stat(name.c_str(),&statBuff)) {
		int err = mkdir(name.c_str(),0755);
		if (err) {
			err = errno;
			SystemException(err, "Failed to create directory " + name);
		}
	} else if (!S_ISDIR(statBuff.st_mode)) {
		SystemException(EINVAL, "Failed to create directory " + name);
	}

}

std::string get_git_last_revision(ExternalProcessLogToFile &&git, const std::string &url) {
	std::ostringstream out;
	git.setOutput(&out);
	int res = git.execute({"ls-remote", url});
	if (res != 0) throw std::runtime_error("GIT:Cannot retrive last revision for url: " + url);
	std::string rev;
	std::istringstream in(out.str());
	in >> rev;
	return rev;
}

void Builder::buildDoc(const std::string& url, const std::string& output_name, std::string &revision) {

	ExternalProcessLogToFile doxygen(cfg.doxygen,envVars,cfg.activityTimeout, cfg.totalTimeout);
	ExternalProcessLogToFile git(cfg.doxygen,envVars,cfg.activityTimeout, cfg.totalTimeout);

	std::string curRev = get_git_last_revision(std::move(git), url);

	if (curRev == revision) return;

	std::string path = cfg.output + output_name;

	recursive_erase(cfg.working);
	makeDir(cfg.working);
	std::string priv = cfg.working+"/private";
	std::string unpack = cfg.working+"/unpack";
	std::string build = cfg.working+"/build";
	std::string build_html = build+"/html";
	makeDir(priv);
	makeDir(unpack);
	makeDir(build);
	std::ostringstream log;


	doxygen.setOutput(&log);
	git.setOutput(&log);

	int res = git.execute({"clone","--progress","--depth","1",url,"."});;
	if (res != 0) {
		this->log = log.str();
		throw std::runtime_error("GIT:clone failed for url: " + url);
	}

	std::string doxyfile = unpack+"/Doxyfile";
	std::string adj_doxyfile = priv+"/Doxyfile";
	if (access(doxyfile.c_str(),0)) {
		doxyfile = cfg.doxyfile;
	}
	prepareDoxyfile(doxyfile, adj_doxyfile);

	res = doxygen.execute({adj_doxyfile});
	if (res != 0)  {
		this->log = log.str();
		throw std::runtime_error("Doxygen failed for url: " + url);
	}

	if (rename(build_html.c_str(), path.c_str())) {
		int err = errno;
		throw SystemException(err, "Failed to update storage: " + path);
	}

	revision = curRev;


}

void Builder::prepareDoxyfile(const std::string& source,const std::string& target) {

	std::ifstream src(source);
	if (!src) {
		int err = errno;
		throw SystemException(err, "prepareDoxyfile: Failed to open source file:" + source);
	}

	std::ofstream trg(target, std::ios::out| std::ios::trunc);
	if (!trg) {
		int err = errno;
		throw SystemException(err, "prepareDoxyfile: Failed to open target file:" + target);
	}

//	trg << "DOXYFILE_ENCODING"

	std::string line;
	std::getline(src,line);
	while (!line.empty() || !src.eof()) {
		StrViewA linev(line);
		linev = linev.trim(isspace);
		if (!linev.empty() && linev[0] != '#') {

		}
		std::getline(src,line);
	}
}

} /* namespace doxyhub */

