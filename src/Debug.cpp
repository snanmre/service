#include "Debug.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cerrno>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "stringutils.h"

using namespace std;

/** print level environment variable */
#define ENV_DEBUGLEVEL						"DEBUGLEVEL"

/** color enable environment variable */
#define ENV_DEBUGCOLOR						"DEBUGCOLOR"

extern char * __progname;

Debug::Debug(const string & appname) :
		b_enabled(true), b_color(false), b_colorEnv(false), printLevel(
				Debug::ERROR), b_printLevelEnv(false)
{
	setApplicationName(appname);
	init();
}

Debug::Debug(const Debug & d) :
		b_enabled(d.b_enabled), b_color(d.b_color), b_colorEnv(d.b_colorEnv), printLevel(
				d.printLevel), b_printLevelEnv(d.b_printLevelEnv)
{
	setApplicationName(d.appname);

	init();
}

Debug::~Debug()
{
	finalize();
}

const Debug & Debug::operator=(const Debug & d)
{
	b_enabled = d.b_enabled;
	b_color = d.b_color;
	b_colorEnv = d.b_colorEnv;
	printLevel = d.printLevel;
	b_printLevelEnv = d.b_printLevelEnv;

	setApplicationName(d.appname);

	return *this;
}

bool Debug::write(const DebugLevel level, const string & message)
{
	bool res = false;
	vector<string> lines;
	stringutils::splitLine(message, lines);
	char taggedline[1024];

	// process each line one by one
	for (size_t i = 0; i < lines.size(); ++i)
	{
		if (lines[i].empty())
			continue;

		if (tag.empty())
			::snprintf(taggedline, sizeof(taggedline), "%s", lines[i].c_str());
		else
			::snprintf(taggedline, sizeof(taggedline), "[%s] %s", tag.c_str(),
					lines[i].c_str());

		if (printLevel >= level)
		{
			struct timespec ts;
			struct tm tm;
			long millis;

			// get time
			::clock_gettime(CLOCK_REALTIME, &ts);
			millis = (ts.tv_nsec / 1000000) % 1000;

			localtime_r(&ts.tv_sec, &tm);

			// re-modify for printing
			if (tag.empty())
				::snprintf(taggedline, sizeof(taggedline), "%s",
						lines[i].c_str());
			else
				::snprintf(taggedline, sizeof(taggedline), "[%-10s] %s",
						tag.c_str(), lines[i].c_str());

			switch (level)
			{
			case INFO:

				fprintf(stderr,
						"%s[I] [%d-%02d-%02d %02d:%02d:%02d.%03ld] [%-20s] %s%s\n",
						b_color ? "\x1b[1;32m" : "", tm.tm_year + 1900,
						tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
						tm.tm_sec, millis, appname.c_str(), taggedline,
						b_color ? "\x1b[0m" : "");
				break;

			case WARNING:

				fprintf(stderr,
						"%s[W] [%d-%02d-%02d %02d:%02d:%02d.%03ld] [%-20s] %s%s\n",
						b_color ? "\x1b[1;33m" : "", tm.tm_year + 1900,
						tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
						tm.tm_sec, millis, appname.c_str(), taggedline,
						b_color ? "\x1b[0m" : "");
				break;

			case ERROR:

				fprintf(stderr,
						"%s[E] [%d-%02d-%02d %02d:%02d:%02d.%03ld] [%-20s] %s%s\n",
						b_color ? "\x1b[1;31m" : "", tm.tm_year + 1900,
						tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
						tm.tm_sec, millis, appname.c_str(), taggedline,
						b_color ? "\x1b[0m" : "");
				break;
			}

			fflush(stdout);

		} // end-of-if printLevel

		if (!res)
			break;

	} // end-of-for lines

	return res;
}

bool Debug::write(const DebugLevel type, const char * format, ...)
{
	va_list list;

	va_start(list, format);

	vsnprintf((char *) tmpBuf, sizeof(tmpBuf), format, list);

	va_end(list);

	bool b = this->write(type, string((char *) tmpBuf));

	return b;
}

bool Debug::e(const string & message)
{
	return this->write(Debug::ERROR, message);
}

bool Debug::e(const char * format, ...)
{
	va_list list;
	va_start(list, format);
	vsnprintf((char *) tmpBuf, sizeof(tmpBuf), format, list);
	va_end(list);

	return this->write(Debug::ERROR, string((char *) tmpBuf));
}

bool Debug::w(const string & message)
{
	return this->write(Debug::WARNING, message);
}

bool Debug::w(const char * format, ...)
{
	va_list list;
	va_start(list, format);
	vsnprintf((char *) tmpBuf, sizeof(tmpBuf), format, list);
	va_end(list);

	return this->write(Debug::WARNING, string((char *) tmpBuf));
}

bool Debug::i(const string & message)
{
	return this->write(Debug::INFO, message);
}

bool Debug::i(const char * format, ...)
{
	va_list list;
	va_start(list, format);
	vsnprintf((char *) tmpBuf, sizeof(tmpBuf), format, list);
	va_end(list);

	return this->write(Debug::INFO, string((char *) tmpBuf));
}

void Debug::setApplicationName(const string & appname)
{
	if (appname.empty())
	{
		this->appname = __progname;
		return;
	}

	this->appname = appname;
}

void Debug::setTag(const std::string & tag)
{
	this->tag = tag;
}

void Debug::setEnabled(bool enabled)
{
	b_enabled = enabled;
}

void Debug::setPrintLevel(int level)
{
	if (!this->b_printLevelEnv)
		this->printLevel = level;
}

int Debug::getPrintLevel() const
{
	return this->printLevel;
}

void Debug::setColorEnabled(bool enable)
{
	if (!this->b_colorEnv)
		this->b_color = enable;
}

void Debug::init()
{
	printLevelInit();
	colorInit();
}

void Debug::finalize()
{
}

void Debug::printLevelInit()
{
	char * printl = ::getenv(ENV_DEBUGLEVEL);
	if (printl)
	{
		int level = ::atoi(printl);

		if (level >= 0 && level <= DEBUG_LEVEL_MAX)
		{
			printLevel = level;
			b_printLevelEnv = true;
		}
	}
}

void Debug::colorInit()
{
	char * colore = ::getenv(ENV_DEBUGCOLOR);
	if (colore)
	{
		b_color = (::atoi(colore) != 0);
		b_colorEnv = true;
	}
	else
	{
		// check whether stdout is terminal (and supports colors - trick)
		b_color = (::isatty(STDOUT_FILENO) == 1);
		if (b_color)
		{
			char * term = ::getenv("TERM");
			if (!term)
				b_color = false;
		}
	}
}
