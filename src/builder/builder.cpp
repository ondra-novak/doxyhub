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
#include <shared/logOutput.h>

#include "../zwebpak/zwebpak.h"

namespace doxyhub {

using simpleServer::SystemException;
using ondra_shared::RAII;
using ondra_shared::logDebug;
using ondra_shared::logError;
using ondra_shared::logNote;
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
	std::string stderr_linebuff;
	std::string stdout_linebuff;
	std::string tmp;
	bool stderr_cr = false;
	bool stdout_cr = false;

	using ExternalProcess::ExternalProcess;

	int execute(std::initializer_list<StrViewA> args) {
		int res = ExternalProcess::execute(args);
		flush();
		if (res) output << "! "; else output << "- ";
		output << "Exit code: " << res << std::endl;
		return res;
	}

	virtual void onLogOutput(StrViewA line, bool error) {
		std::string &curline = error?stderr_linebuff:stdout_linebuff;
		bool &flag = error?stderr_cr:stdout_cr;
		for (auto c: line) {
			if (c == '\n') {
				if (error) output << "! "; else output << "- ";
				output << curline;
				output << std::endl;
				flag = false;
				curline.clear();
			} else if (c == '\r') {
				flag = true;
			} else {
				if (flag) curline.clear();
				flag = false;
				curline.push_back(c);
			}
		}
	}

	void flush() {

		if (!stdout_linebuff.empty()) output << "- " << stdout_linebuff << std::endl;
		if (!stderr_linebuff.empty()) output << "! " << stderr_linebuff << std::endl;
	}

	void clear() {
		output = std::ostringstream();
		stderr_linebuff.clear();
		stdout_linebuff.clear();
	}



};


std::string Builder::get_git_last_revision(ExternalProcessWithLog &&git, const std::string &url, const std::string &branch) {
	AGuard _(activeTool, &git);
	int res = git.execute({"ls-remote", url, branch});
	if (res != 0) {
		logNote("GIT Failed, exit code: $1", res);
		throw std::runtime_error("GIT:Cannot retrive last revision for url: " + url);
	}
	std::string rev;
	std::istringstream in(git.output.str());
	std::string tmp;
	in >> tmp >> rev;
	git.clear();
	return rev;
}

static std::string combineLogs(std::ostringstream &git, std::ostringstream &doxygen) {
	std::ostringstream tmp;
	tmp << "= -------- DOWNLOAD ------------" << std::endl;
	tmp << git.str() << std::endl;
	tmp << "= -------- GENERATE ------------" << std::endl;
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
/*
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
*/

/*
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
*/

static void clean_after_clone(const std::string &path) {
	WalkDir::walk_directory(path, false, [&](const std::string &p, WalkDir::WalkEvent ev) {
		if (p[path.length()+1] == '.') {
			if (ev == WalkDir::directory_leave) {
				recursive_erase(p);
			} else if (ev == WalkDir::file_entry) {
				remove(p.c_str());
			}
		}
		return true;
	});
}

static void clean_after_build(const std::string &path) {
	recursive_erase(path);
}

bool pack_files(const std::string &path, const std::string &file, const std::string &revision, std::size_t clusterSize) {

	std::vector<std::string> files;
	WalkDir::walk_directory(path, true, [&](const std::string &p, WalkDir::WalkEvent ev) {
		if (ev == WalkDir::file_entry) {
			std::string fname =p.substr(path.length()+1);
			files.push_back(fname);
		}
		return true;
	});

	std::string manifest = path+"/manifest.txt";
	std::ofstream m(manifest, std::ios::out|std::ios::trunc);
	for(auto &&c:files) m << c << std::endl;
	m.close();
	files.push_back(manifest.substr(path.length()+1));

	return zwebpak::packFiles(files, path+"/", file, revision, clusterSize);
}

DoxyhubError Builder::buildDoc(const BuildRequest &req) {

	req.progressFn(BuildStage::checkrev);
	ExternalProcessWithLog doxygen(cfg.doxygen,envVars,cfg.activityTimeout, cfg.totalTimeout);
	ExternalProcessWithLog git(cfg.git,envVars,cfg.activityTimeout, cfg.totalTimeout);
	ExternalProcessWithLog curl(cfg.curl, envVars, cfg.activityTimeout, cfg.totalTimeout);


	std::string curRev;
	try {
		curRev = get_git_last_revision(std::move(git), req.url, req.branch);
	} catch (...) {
		git.flush();
		this->log = git.output.str();
		return DoxyhubError::git_connect_failed;
	}


	this->revision = curRev;
	this->log.clear();
	if (curRev == req.revision) return DoxyhubError::not_modified;


	std::string path = cfg.working +"/pack";

	recursive_erase(cfg.working);
	makeDir(cfg.working);
	std::string unpack = cfg.working+"/unpack";
	std::string build = cfg.working+"/build";
	std::string build_html = build+"/html";
	makeDir(unpack);
	makeDir(build);

	git.set_start_dir(unpack);
	doxygen.set_start_dir(unpack);

	req.progressFn(BuildStage::download);


	int res;

	try {
		AGuard _(activeTool, &git);
		res = git.execute({"clone","--progress","-b",req.branch,"--depth","1",req.url,"."});;
	} catch (std::exception &e) {
		git.flush();
		git.output << "! Exception:" << e.what() << std::endl;
		res = -1;
	}

	this->log = git.output.str();
	if (res != 0) {
		return git.wasTimeout()?DoxyhubError::git_clone_timeout:DoxyhubError::git_clone_failed;
	}

	req.progressFn(BuildStage::generate);


	clean_after_clone(unpack);

	std::string doxyfile = unpack+"/Doxyfile";
	std::string adj_doxyfile = doxyfile+".doxyhub.1316048469";
	if (access(doxyfile.c_str(),0)) {
		doxyfile = cfg.doxyfile;
	}

	prepareDoxyfile(doxyfile, adj_doxyfile, "../build",req.url);
	copy_file_only(cfg.footer,unpack+"/.footer.html");
	try {
		AGuard _(activeTool, &doxygen);
		res = doxygen.execute({"Doxyfile.doxyhub.1316048469"});
	} catch (std::exception &e) {
		doxygen.flush();
		doxygen.output << std::endl << "! Exception:" << e.what() << std::endl;
		res = -1;
	}

	this->log = combineLogs(git.output, doxygen.output);

	if (res != 0)  {
		return doxygen.wasTimeout()?DoxyhubError::build_timeout:DoxyhubError::build_failed;
	}

	req.progressFn(BuildStage::upload);

	clean_after_build(unpack);


/*	makeDir(newpath);
	recursive_erase(newpath);
	recursive_move(build_html, newpath);
	fast_replace(newpath,path);*/
	if (!pack_files(build_html, path, curRev, cfg.clusterSize)) {
		return DoxyhubError::no_space;
	}
	curl.set_start_dir(build_html);
	std::string token_header = "Authorization: bearer "+req.upload_token;
	res = curl.execute({"-X","PUT",
			"-H","Content-Type: application/octet-stream",
			"-H",token_header,
			"--data-binary", "@"+path, req.upload_url});
	unlink(path.c_str());
	recursive_erase(build);

	if (res != 0) {
		logError ("Upload failed to url: $1 (error: $2)" , req.upload_url, res);
		return DoxyhubError::upload_failed;
	}

	return DoxyhubError::ok;
}

static StrViewA extractNameFromURL(StrViewA url) {
	auto p = url.lastIndexOf("/");
	if (p == url.npos) return url;
	else return url.substr(p+1);

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

