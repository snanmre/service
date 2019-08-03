#include "service_t.h"

#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Debug.h"
#include "fileutils.h"
#include "stringutils.h"

using namespace std;

std::ostream & operator <<(std::ostream & o, const service_t & s)
{
	o << "name                  = " << s.cfg.name << endl
			<< "status                = "
			<< (s.is_running() ? "running" : "stopped") << endl
			<< "pid                   = " << s.pid << endl
			<< "cfg.exec              = " << s.cfg.exec << endl
			<< "cfg.onstop_exec       = " << s.cfg.onstop_exec << endl
			<< "cfg.wipe_log          = " << (s.cfg.wipe_log ? "true" : "false")
			<< endl << "cfg.logfile           = " << s.cfg.logfile << endl
			<< "cfg.pidfile           = " << s.cfg.pidfile << endl
			<< "cfg.respawn           = " << (s.cfg.respawn ? "true" : "false")
			<< endl << "cfg.respawn_limit     = " << s.cfg.respawn_limit << endl
			<< "cfg.respawn_interval  = " << s.cfg.respawn_interval << endl
			<< "respawn_count         = " << s.respawn_count << endl;
//	  << "respawn_timer_enabled = " << s.respawn_timer_enabled << endl;

	return o;
}

const std::string service_t::default_shell = "/bin/sh"; // default shell

service_t::service_t()
{
	clear();
}

bool service_t::start()
{
	if (!cfg.is_valid())
	{
		DD("start() failed: service is not valid.\n");
		return false;
	}

	if (is_running())
	{
		DD("start() failed: service is already running.\n");
		return false;
	}

	// remove old pid file
	fileutils::remove(cfg.pidfile);

	if (!daemonize())
		return false;

	Timer t(3000);
	while (!t.isTimeout())
	{
		pid = ::atoi(fileutils::load_file(cfg.pidfile).c_str());
		if (pid > 0)
		{
			break;
		}

		usleep(100000);
	}

	// invalidate pid
	if (pid == 0)
		pid = -1;

	return pid > 0;
}

bool service_t::stop()
{
	if (!is_running())
	{
		DD("stop() failed: service is already stopped.\n");
		return false;
	}

	bool running = true;

	if (::kill(pid, SIGTERM))
	{
		DD("kill(%d, SIGTERM) failed: %s\n", pid, strerror(errno));
	}
	else
	{
		Timer t;
		t.set(3000);
		while (!t.isTimeout())
		{
			if (!is_running())
			{
				running = false;
				break;
			}

			usleep(100000);
		}
	}

	if (running)
	{
		if (::kill(pid, SIGKILL) && errno != -ESRCH)
		{
			DD("kill(%d, SIGKILL) failed: %s\n", pid, strerror(errno));
			return false;
		}
		else
		{
			running = false;
		}
	}

	if (!running)
	{
		pid = -1;
		fileutils::remove(cfg.pidfile);

		on_post_stop();
	}

	return true;
}

bool service_t::is_running() const
{
	if (pid < 0)
		return false;

	return ::kill(pid, 0) == 0;
}

bool service_t::import(const std::string & filepath)
{
	clear();

	if (!cfg.import(filepath))
	{
		return false;
	}

	return true;
}

bool service_t::daemonize()
{
	pid_t pid = ::fork();
	if (pid < 0)
	{
		cerr << "fork() failed: " << strerror(errno) << endl;
		return false;
	}

	// return parent process
	if (pid > 0)
	{
		// wait for child process to avoid zombie processes
		int es;
		wait(&es);
		return true;
	}

	// Start a new session for the daemon.
	if (::setsid() == -1)
	{
		cerr << "setsid() failed: " << strerror(errno) << endl;
		exit(1);
	}

	// fork again, allowing the parent process to terminate
	::signal(SIGHUP, SIG_IGN);
	pid = ::fork();
	if (pid < 0)
	{
		cerr << "fork() failed: " << strerror(errno) << endl;
		exit(1);
	}

	// terminate parent process
	if (pid > 0)
	{
		exit(0);
	}

	// change current directory
	if (::chdir("/") == -1)
	{
		cerr << "chdir() failed: " << strerror(errno) << endl;
		exit(1);
	}

	// set file creation mask
	::umask(0);

	// redirect std fds and close previously opened files/sockets (inherited from parent process)
	redirect_fds();

	// save pid file
	fileutils::save_file(cfg.pidfile, stringutils::to_string(getpid()), 777);

	// unset DEBUGLEVEL inherited from parent process
	unsetenv("DEBUGLEVEL");

	on_post_start();

	// close and re-open standard file descriptors

//	// close stdin
//	::close(STDIN_FILENO);
//	if (::open("/dev/null", O_RDONLY) == -1)
//	{
//		cerr << "open(stdin) failed: " << strerror(errno) << endl;
//		exit(1);
//	}
//
//	// enable logging for stdout/stderr
//
//	string fp_log = string("/dev/shm/log/") + __progname + ".log";
//	int log_fd = ::open(fp_log.c_str(), O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
//	if (log_fd == -1)
//	{
//		cerr << "open(" << fp_log << ") failed: " << strerror(errno) << endl;
//		exit(1);
//	}
//
//	::dup2(log_fd, STDOUT_FILENO);
//	::dup2(log_fd, STDERR_FILENO);
//	::close(log_fd);

	const char * sh = ::getenv("SHELL");
	if (sh == NULL)
		sh = service_t::default_shell.c_str();

	int r = -1;

	// start daemon

	if (cfg.is_script)
	{
		string script_content;

		// look for shebang, insert if not exist
		if (cfg.exec.find("#!") == string::npos)
		{
			script_content = "#!/bin/sh\n\n" + cfg.exec;
		}
		else
		{
			script_content = cfg.exec;
		}

		string pname = cfg.name + "-service";

		fileutils::save_file(cfg.script_filepath, script_content, 777);

		char * const argv[] =
		{ ::strdup(pname.c_str()),
		NULL };

		r = ::execvp(cfg.script_filepath.c_str(), argv);
		DD("%s : exit_code = %d\n", cfg.name.c_str(), r);

		// free duplicates
		for (int i = 0; argv[i] != NULL; ++i)
			::free(argv[i]);
	}
	else
	{

		// arguments
		char * const argv[] =
		{ ::strdup(sh), ::strdup("-c"), ::strdup(cfg.exec.c_str()),
		NULL };

		r = ::execvp(sh, argv);
		DD("%s : exit_code = %d\n", cfg.name.c_str(), r);

		// free duplicates
		for (int i = 0; argv[i] != NULL; ++i)
			::free(argv[i]);
	}

	::exit(r);
}

void service_t::redirect_fds()
{
	struct rlimit rlim;
	int ret;

	if ((ret = ::getrlimit(RLIMIT_NOFILE, &rlim)) == -1)
	{
		perror("getrlimit");
		return;
	}

	int maxFD = rlim.rlim_cur;

	for (int i = 3; i < maxFD; ++i)
	{
		struct stat statbuf;

		if (fstat(i, &statbuf) == -1)
		{
			if (errno == EBADF)
			{
//			   printf("file descriptor %d is not open\n", i);
			}
			else
			{
//			   perror("fstat");
//			   exit(1);
			}
			continue;
		}

		::close(i);
	}

	// open standard input/outputs
	int in = ::open(config_t::null_device.c_str(), O_RDONLY);
	if (in == -1)
	{
		DD("open(in) failed: %d - %s\n", errno, ::strerror(errno));
	}
	else
	{
		::dup2(in, STDIN_FILENO);
		::close(in);
	}

	int flags = O_CREAT | O_WRONLY | O_APPEND;

	// add O_TRUNC flag if it is need to be wiped
	if (cfg.wipe_log)
	{
		flags |= O_TRUNC;
	}

	int outerr = ::open(cfg.logfile.c_str(), flags);
	if (outerr == -1)
	{
		DD("open(%s) failed: %d - %s\n", cfg.logfile.c_str(), errno, ::strerror(errno));
	}
	else
	{
		::dup2(outerr, STDOUT_FILENO);
		::dup2(outerr, STDERR_FILENO);
		::close(outerr);
	}
}

void service_t::clear()
{
	pid = -1;

	cfg.clear();

	respawn_count = 0;

	respawn_timer_enabled = false;
}

void service_t::on_post_start()
{
	if (cfg.logfile == config_t::null_device)
		return;

	ofstream ofs;
	ofs.open(cfg.logfile.c_str(), ofstream::out | ofstream::app);
	if (ofs.is_open())
	{
		ofs << "service: started." << endl;
		ofs.close();
	}
}

void service_t::on_post_stop()
{
	if (cfg.logfile != config_t::null_device)
	{
		ofstream ofs;
		ofs.open(cfg.logfile.c_str(), ofstream::out | ofstream::app);
		if (ofs.is_open())
		{
			ofs << "service: stopped." << endl;
			ofs.close();
		}
	}
	// run onstop command
	::system(cfg.onstop_exec.c_str());
}

void service_t::writeToBundle(Bundle & bundle) const
{
	bundle << cfg << pid << respawn_count;
}

void service_t::readFromBundle(Bundle & bundle)
{
	bundle >> cfg >> pid >> respawn_count;
}
