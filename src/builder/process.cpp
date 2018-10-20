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
#include <shared/raii.h>
#include <shared/logOutput.h>

namespace doxyhub {

using simpleServer::SystemException;
using ondra_shared::RAII;
using ondra_shared::logDebug;


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

namespace {
	void setZero(pid_t *v) {
		*v = 0;
	}
}

int ExternalProcess::execute(std::initializer_list<StrViewA> args) {

	logDebug("External process (@$3): $1 $2 ", pathname, args, start_dir);
	timeouted = false;

	try {
		Fd stdout_read, stdout_write;
		Fd stderr_read, stderr_write;
		Fd status_read, status_write;

		int err;

		create_pipe(stdout_read, stdout_write);
		create_pipe(stderr_read, stderr_write);
		create_pipe(status_read, status_write);

		RAII<int *, decltype(&setZero),&setZero> pidptr(&pid);

		pid = fork();


		if (pid == -1) {
			err = errno;
			throw SystemException(err, "fork failed");
		}


		if (pid == 0) {

			if (!start_dir.empty()) {
				if (chdir(start_dir.c_str())) {
					int err = errno;
					write(status_write,&err,sizeof(err));
					throw exitExec;
				}
			}

			dup2(stdout_write, 1);
			dup2(stderr_write, 2);

			auto listsize = args.size();
			char **arglist = new char *[listsize+2];
			arglist[listsize+1] = nullptr;
			int x = 0;
			arglist[x++] = const_cast<char *>(pathname.c_str());
			for (auto &&v : args) {
				char *c = new char[v.length+1];
				std::memcpy(c, v.data, v.length);
				c[v.length] = 0;
				arglist[x++] = c;
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
				throw SystemException(err, "exec failed: " + pathname);


			time_t start;
			time(&start);


			int count = 2;
			struct pollfd pnfo[2];
			pnfo[0].fd = stdout_read;
			pnfo[0].events = POLLIN|POLLHUP|POLLERR;
			pnfo[0].revents = 0;
			pnfo[1] = pnfo[0];
			pnfo[1].fd = stderr_read;

			do {

				int pres = poll(pnfo,count, activityTimeout * 1000);
				if (pres == -1) {
					int err = errno;
					if (err == EINTR) continue;
					throw SystemException(err,"poll failed");
				}
				if (pres == 0) {
					timeouted = true;
					kill(pid,SIGTERM);
					continue;
				}

				time_t curtime;
				time(&curtime);
				if (curtime - start > totalTimeout) {
					timeouted = true;
					kill(pid,SIGTERM);
					break;
				}

				char buff[256];
				for (int i = 0; i < count; i++) {
					pollfd &pf = pnfo[i];
					bool erase = false;
					if (pf.revents & POLLIN) {
						int k = read(pf.fd,buff,256);
						if (k <= 0) {
							erase = true;
						} else {
							StrViewA data(buff, k);
							onLogOutput(data, pf.fd == stderr_read);
						}

					} else if (pf.revents & (POLLERR | POLLHUP)) {
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


void ExternalProcess::terminate() {
	pid_t p = pid;
	if (p) kill(p, SIGTERM);
}


} /* namespace doxyhub */

