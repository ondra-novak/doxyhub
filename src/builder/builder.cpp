/*
 * b\uilder.cpp
 *
 *  Created on: 14. 6. 2018
 *      Author: ondra
 */

#include <unistd.h>
#include <builder/builder.h>
#include <builder/doxyfile.h>
#include <dirent.h>
#include <shared/raii.h>
#include <simpleServer/exceptions.h>
#include <sys/stat.h>
#include <sstream>
#include <fstream>

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


	const dirent *entry;

	auto pathlen = path.length();

	while ((entry = readdir(dir)) != nullptr) {
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

class ExternalProcessWithLog: public ExternalProcess {
public:
	std::ostringstream output;
	std::ostringstream error;

	using ExternalProcess::ExternalProcess;

	virtual void onLogOutput(StrViewA line, bool error) {
		if (error) this->error << line;
		else this->output << line;
	}

	void clear() {
		output = std::ostringstream();
		error = std::ostringstream();
	}



};


std::string get_git_last_revision(ExternalProcessWithLog &&git, const std::string &url) {
	int res = git.execute({"ls-remote", url});
	if (res != 0) throw std::runtime_error("GIT:Cannot retrive last revision for url: " + url);
	std::string rev;
	std::istringstream in(git.output.str());
	in >> rev;
	git.clear();
	return rev;
}

static std::string combineLogs(std::ostringstream &git, std::ostringstream &doxygen) {
	std::ostringstream tmp;
	tmp << "-------- DOWNLOAD ------------" << std::endl;
	tmp << git.str() << std::endl;
	tmp << "-------- GENERATE ------------" << std::endl;
	tmp << doxygen.str() << std::endl;
	return tmp.str();
}

void Builder::buildDoc(const std::string& url, const std::string& output_name, const std::string &revision) {


	ExternalProcessWithLog doxygen(cfg.doxygen,envVars,cfg.activityTimeout, cfg.totalTimeout);
	ExternalProcessWithLog git(cfg.doxygen,envVars,cfg.activityTimeout, cfg.totalTimeout);

	std::string curRev = get_git_last_revision(std::move(git), url);


	this->revision = curRev;
	this->log.clear();
	this->warnings.clear();
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

	git.set_start_dir(unpack);
	doxygen.set_start_dir(unpack);


	int res = git.execute({"clone","--progress","--depth","1",url,"."});;
	if (res != 0) {
		this->log = git.output.str();
		this->warnings = git.error.str();
		throw std::runtime_error("GIT:clone failed for url: " + url);
	}

	std::string doxyfile = unpack+"/Doxyfile";
	std::string adj_doxyfile = adj_doxyfile;
	if (access(doxyfile.c_str(),0)) {
		doxyfile = cfg.doxyfile;
	}

	prepareDoxyfile(doxyfile, adj_doxyfile, build);

	res = doxygen.execute({adj_doxyfile});

	this->log = combineLogs(git.output, doxygen.output);
	this->warnings = combineLogs(git.error, doxygen.error);

	if (res != 0)  {
		throw std::runtime_error("Doxygen failed for url: " + url);
	}

	if (rename(build_html.c_str(), path.c_str())) {
		int err = errno;
		throw SystemException(err, "Failed to update storage: " + path);
	}


}

static StrViewA extractNameFromURL(StrViewA url) {
	auto p = url.lastIndexOf("/");
	if (p == url.npos) return url;
	else return url.substr(p+1);

}

void Builder::prepareDoxyfile(const std::string& source,const std::string& target, const std::string &buildPath) {

	Doxyfile df;
	{
		std::ifstream src(source);
		if (!src) {
			int err = errno;
			throw SystemException(err, "prepareDoxyfile: Failed to open source file:" + source);
		}


		df.parse(src);
	}


	StrViewA name = extractNameFromURL(source);
	df.sanitize(buildPath, name, revision);

	{
		std::ofstream trg(target, std::ios::out| std::ios::trunc);
		if (!trg) {
			int err = errno;
			throw SystemException(err, "prepareDoxyfile: Failed to open target file:" + target);
		}

		df.serialize(trg);

	}
}

} /* namespace doxyhub */

