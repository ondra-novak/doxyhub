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
#include <sys/types.h>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include "walk_dir.h"

namespace doxyhub {

using simpleServer::SystemException;
using ondra_shared::RAII;
using ondra_shared::StrViewA;

Builder::Builder(const Config& cfg, EnvVars envVars):cfg(cfg),envVars(envVars) {
}

static void recursive_erase(std::string path) {

	WalkDir::walk_directory(path, true,
			[](const std::string &path, WalkDir::WalkEvent event) {
		switch (event) {
		case WalkDir::file_entry: remove(path.c_str());break;
		case WalkDir::directory_leave:rmdir(path.c_str());break;
		default:break;
		}

		return true;
	});

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


std::string Builder::get_git_last_revision(ExternalProcessWithLog &&git, const std::string &url) {
	AGuard _(activeTool, &git);
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

static bool copy_file_only(const std::string &src, const std::string &trg) {

	typedef RAII<int, decltype(&close), &close> FD;
	FD srcfd = ::open(src.c_str(), O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
	if (srcfd == -1) return false;
	FD trgfd = ::open(trg.c_str(), O_WRONLY|O_CLOEXEC|O_EXCL|O_CREAT, 0666);
	if (trgfd == -1) return false;
	unsigned char buffer[65536];
	auto i = ::read(srcfd, buffer,sizeof(buffer));
	while (i > 0) {
		::write(trgfd, buffer,i);
		i = ::read(srcfd, buffer,sizeof(buffer));
	}
	return true;
}

static void recursive_move(const std::string &source, const std::string& target) {

	WalkDir::walk_directory(source, true, [&](const std::string &path, WalkDir::WalkEvent event) {

		std::string targetPath = target+path.substr(source.length());
		switch (event) {
			case WalkDir::WalkEvent::directory_enter: makeDir(targetPath);break;
			case WalkDir::WalkEvent::file_entry: copy_file_only(path, targetPath);
							unlink(path.c_str());
							break;
			case WalkDir::WalkEvent::directory_leave:
							rmdir(path.c_str());
							break;
		}

		return true;
	});
}

static void fast_replace(const std::string& src_path, const std::string& target_path) {
	std::string movedOut;
	if (access(target_path.c_str(), 0) == 0) {
		movedOut = target_path + "_old";
		if (rename(target_path.c_str(), movedOut.c_str())) {
			int err = errno;
			throw SystemException(err, "Failed to update storage: " + movedOut);
		}
	}
	if (rename(src_path.c_str(), target_path.c_str())) {
		int err = errno;
		throw SystemException(err, "Failed to update storage: " + target_path);
	}
	if (!movedOut.empty()) {
		recursive_erase(movedOut);
		rmdir(movedOut.c_str());
	}
}

void Builder::buildDoc(const std::string& url, const std::string& output_name, const std::string &revision) {


	ExternalProcessWithLog doxygen(cfg.doxygen,envVars,cfg.activityTimeout, cfg.totalTimeout);
	ExternalProcessWithLog git(cfg.git,envVars,cfg.activityTimeout, cfg.totalTimeout);


	std::string curRev = get_git_last_revision(std::move(git), url);


	this->revision = curRev;
	this->log.clear();
	this->warnings.clear();
	if (curRev == revision) return;

	std::string path = cfg.output +"/"+ output_name;
	std::string newpath = path+"_new";

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

	int res;

	{
		AGuard _(activeTool, &git);
		res = git.execute({"clone","--progress","--depth","1",url,"."});;
	}
	if (res != 0) {
		this->log = git.output.str();
		this->warnings = git.error.str();
		throw std::runtime_error("GIT:clone failed for url: " + url);
	}

	std::string doxyfile = unpack+"/Doxyfile";
	std::string adj_doxyfile = doxyfile+".doxyhub.1316048469";
	if (access(doxyfile.c_str(),0)) {
		doxyfile = cfg.doxyfile;
	}

	prepareDoxyfile(doxyfile, adj_doxyfile, "../build",url);

	{
		AGuard _(activeTool, &doxygen);
		res = doxygen.execute({"Doxyfile.doxyhub.1316048469"});
	}

	this->log = combineLogs(git.output, doxygen.output);
	this->warnings = combineLogs(git.error, doxygen.error);

	if (res != 0)  {
		throw std::runtime_error("Doxygen failed for url: " + url);
	}

	makeDir(newpath);
	recursive_erase(newpath);
	recursive_move(build_html, newpath);
	fast_replace(newpath,path);
}

static StrViewA extractNameFromURL(StrViewA url) {
	auto p = url.lastIndexOf("/");
	if (p == url.npos) return url;
	else return url.substr(p+1);

}

void Builder::deleteDoc(const std::string& output_name) {
	std::string path = cfg.output +"/" + output_name;
	recursive_erase(path);
}

std::size_t Builder::calcSize(const std::string& output_name) {
	std::string path = cfg.output +"/" + output_name;
	std::size_t size = 0;
	WalkDir::walk_directory(path,true,
			[&size](const std::string &path, WalkDir::WalkEvent ev){
		if (ev == WalkDir::file_entry) {
			struct stat st;
			if (lstat(path.c_str(), &st) == 0) {
				size += st.st_size;
			}
		}
		return true;
	});
	return size;
}

void Builder::prepareDoxyfile(const std::string& source,const std::string& target, const std::string &buildPath, const std::string &url) {

	Doxyfile df;
	{
		std::ifstream src(source);
		if (!src) {
			int err = errno;
			throw SystemException(err, "prepareDoxyfile: Failed to open source file:" + source);
		}


		df.parse(src);
	}


	StrViewA name = extractNameFromURL(url);
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

void Builder::stopTools() {
	activeTool.lock([](ExternalProcess &proc){
		proc.terminate();
	});
}


} /* namespace doxyhub */

