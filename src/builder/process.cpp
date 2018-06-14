/*
 * process.cpp
 *
 *  Created on: 14. 6. 2018
 *      Author: ondra
 */

#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <simpleServer/exceptions.h>
#include <sys/wait.h>
#include "process.h"
#include <chrono>
#include <thread>
#include <ctime>
#include <memory>

namespace doxyhub {

using simpleServer::SystemException;


ExternalProcess::ExternalProcess(std::string pathname, EnvVars envp, unsigned int activityTimeout, unsigned int totalTimeout)
	:pathname(pathname)
	,envp(envp)
	,activityTimeout(activityTimeout)
	,totalTimeout(totalTimeout)

{

}

void ExternalProcess::onLogOutput(StrViewA ,bool) {}


class Fd {
	int fd;
public:
	explicit Fd(int fd):fd(fd) {}
	Fd():fd(-1) {}
	~Fd() {close();}
	Fd(Fd &&x):fd(x.fd) {x.fd = -1;}
	Fd(const Fd &) = delete;
	Fd &operator=(Fd &&x) {
		close();
		fd = x.fd;
		x.fd = -1;
		return *this;
	}
	void operator=(const Fd &x) =  delete;
	operator int() const {return fd;}
	void close() {
		if (fd>=0) ::close(fd);
		fd = -1;
	}
};

void create_pipe(Fd &read, Fd &write) {
	int fds[2];
	if (pipe2(fds, O_CLOEXEC))  {
		int err = errno;
		throw SystemException(err, "pipe2 failed");
	}

	read = Fd(fds[0]);
	write = Fd(fds[1]);
}


enum ExitExec {
	exitExec
};

static int waitForPidTimeout(pid_t pid) {
	int status;
	for (int i = 0; i < 20; i++) {
		if (waitpid(pid,&status,WNOHANG)) {
			return status;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	kill(pid,SIGKILL);
	waitpid(pid,&status,0);
	return status;

}

int ExternalProcess::execute(std::initializer_list<StrViewA> args) {

	try {
		Fd stdout_read, stdout_write;
		Fd stderr_read, stderr_write;
		Fd status_read, status_write;

		int err;
		pid_t pid;

		create_pipe(stdout_read, stdout_write);
		create_pipe(stderr_read, stderr_write);
		create_pipe(status_read, status_write);

		pid = fork();
		if (pid == -1) {
			err = errno;
			throw SystemException(err, "fork failed");
		}


		if (pid == 0) {

			dup2(stdout_write, 1);
			dup2(stderr_write, 2);

			auto listsize = args.size();
			char **arglist = new char *[listsize+1];
			arglist[listsize] = nullptr;
			for (auto &&v : args) {
				char *c = new char[v.length+1];
				std::memcpy(c, v.data, v.length);
				c[v.length] = 0;
			}

			execve(pathname.c_str(), arglist, envp);
			err = errno;
			write(status_write, &err, sizeof(err));
			throw exitExec;


		} else {

			stdout_write.close();
			stderr_write.close();
			status_write.close();

			int x = read(status_read, &err, sizeof(err));
			status_read.close();

			if (x)
				throw SystemException(err, "exec failed");


			time_t start;
			time(&start);


			int count = 3;
			struct pollfd pnfo[2];
			pnfo[0].fd = stdout_read;
			pnfo[0].events = POLL_IN|POLL_HUP|POLL_ERR;
			pnfo[0].revents = 0;
			pnfo[1] = pnfo[0];
			pnfo[1].fd = stdout_write;

			do {

				int pres = poll(pnfo,count, activityTimeout * 1000);
				if (pres == -1) {
					int err = errno;
					if (err == EINTR) continue;
					throw SystemException(err,"poll failed");
				}
				if (pres == 0) {
					kill(pid,SIGTERM);
					continue;
				}

				time_t curtime;
				time(&curtime);
				if (curtime - start > totalTimeout) {
					kill(pid,SIGTERM);
					break;
				}

				char buff[256];
				for (auto &&pf : pnfo) {
					bool erase = false;
					if (pf.revents & POLL_IN) {
						int k = read(pf.fd,buff,256);
						if (k <= 0) {
							erase = true;
						} else {
							StrViewA data(buff, k);
							onLogOutput(data, pf.fd == stderr_read);
						}

					} else if (pf.revents & (POLL_ERR | POLL_HUP)) {
								erase = true;
					}
					pf.revents = 0;
					if (erase) {
						count--;
						std::swap(pf, pnfo[count]);
					}
				}

			} while (count > 0);

			return waitForPidTimeout(pid);
		}
	} catch (ExitExec) {
		_exit(0);
		throw;
	}

}


void ExternalProcessLogToFile::setOutput(std::ostream* stream) {
	this->stream = stream;
}

void ExternalProcessLogToFile::onLogOutput(StrViewA line, bool error) {
	if (stream) {
		std::vector<char> &buff = error?errline:outline;

		auto nlpos = line.indexOf("\n");
		decltype(nlpos) i = 0;
		if (nlpos != line.npos) {
			while (i < nlpos) {
				buff.push_back(line[i++]);
			}
			(*stream) << (error?"ERR: ":"OUT: ");
			stream->write(buff.data(),buff.size());
			buff.clear();
		}
		while (i < line.length) {
			buff.push_back(line[i++]);
		}
	}
	ExternalProcess::onLogOutput(line,error);
}

int ExternalProcessLogToFile::execute(std::initializer_list<StrViewA> args) {
	if (stream) {
		(*stream) << "$ " << pathname;
		for (auto &&x: args) {
			(*stream) << " " << x ;
		}
		(*stream) << std::endl;
	}
	int res = ExternalProcess::execute(args);
	if (stream) {
		(*stream) << "EXIT: " << pathname << " " << res;
	}
	return res;
}

} /* namespace doxyhub */
