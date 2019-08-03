#include "config_t.h"

#include <climits>

#include "fileutils.h"
#include "Debug.h"
#include "stringutils.h"

#define KEYWORD_EXEC						"exec"

#define KEYWORD_ONSTOP_EXEC					"onstop exec"

#define KEYWORD_SCRIPT						"script"
#define KEYWORD_END_SCRIPT					"end script"

#define KEYWORD_WIPE_LOG					"wipe log"
#define KEYWORD_LOG							"log"
#define KEYWORD_PIDFILE						"pidfile"

#define KEYWORD_RESPAWN						"respawn"
#define KEYWORD_RESPAWN__LIMIT				"limit"    // use with respawn

using namespace std;

const std::string config_t::null_device = "/dev/null";
std::string config_t::dirpath_pid;

config_t::config_t()
{
	if (dirpath_pid.empty())
	{
		dirpath_pid = DIRPATH_SERVICE_PIDS;
	}

	clear();
}

bool config_t::import(const std::string& filepath)
{
	vector<string> lines;

	if (!fileutils::load_file(filepath, lines))
		return false;

	clear();

	vector<string> parts;

	for (size_t i = 0; i < lines.size(); ++i)
	{
		lines[i] = stringutils::trim(lines[i]);

		if (lines[i].empty() || lines[i][0] == '#')
			continue;

		stringutils::split(lines[i], " ", parts, false, true);
		const string & key = parts[0];

		if (key == KEYWORD_EXEC)
		{
			if (!exec.empty())
			{
				DD("import() failed: command already defined.\n");
				return false;
			}

			string c = stringutils::trim(
					lines[i].substr(::strlen(KEYWORD_EXEC) + 1));
			if (c.empty())
			{
				DD("import() failed: empty script.\n");
			}

			exec = c;

			is_script = false;
		}
		else if (key == KEYWORD_SCRIPT)
		{
			if (!exec.empty())
			{
				DD("import() failed: command already defined.\n");
				return false;
			}

			stringstream ss;
			for (++i; i < lines.size(); ++i)
			{
				if (lines[i] == KEYWORD_END_SCRIPT)
				{
					break;
				}

				ss << lines[i] << endl;
			}

			string c = stringutils::trim(ss.str());
			if (c.empty())
			{
				DD("import() failed: empty script.\n");
			}

			exec = c;

			is_script = true;
		}
		else if (lines[i].find(KEYWORD_ONSTOP_EXEC) == 0)
		{
			onstop_exec = stringutils::trim(
					lines[i].substr(::strlen(KEYWORD_ONSTOP_EXEC) + 1));
			if (onstop_exec.empty())
			{
				DD("import() failed: empty script.\n");
			}
		}
		else if (lines[i] == KEYWORD_WIPE_LOG)
		{
			wipe_log = true;
		}
		else if (key == KEYWORD_LOG)
		{
			logfile = stringutils::trim(
					lines[i].substr(::strlen(KEYWORD_LOG) + 1));
		}
		else if (key == KEYWORD_PIDFILE)
		{
			pidfile = stringutils::trim(
					lines[i].substr(::strlen(KEYWORD_PIDFILE) + 1));
		}
		else if (lines[i] == KEYWORD_RESPAWN)
		{
			respawn = true;

			respawn_interval = 0;
			respawn_limit = INT_MAX;
		}
		else if (parts[0] == KEYWORD_RESPAWN
				&& parts[1] == KEYWORD_RESPAWN__LIMIT)
		{
			if (parts.size() != 4)
			{
				DD("import() failed: error in 'respawn limit'\n");
				return false;
			}

			// parse

			respawn_limit = ::atoi(parts[2].c_str());
			respawn_interval = ::atoi(parts[3].c_str());

			// controls

			if (respawn_limit == 0 || respawn_interval == 0)
			{
				DD("import() failed: invalid value in 'respawn limit'\n");
				return false;
			}
		}
	} // end-of-for lines

	// check validity
	if (!is_valid())
	{
		DD("import() failed: not valid.\n");
		return false;
	}

	name = fileutils::basename2(filepath, true);

	// set pid filepath if not set
	if (pidfile.empty())
	{
		pidfile = config_t::dirpath_pid + "/" + name + ".pid";
	}

	if (is_script)
	{
		script_filepath = string(DIRPATH_SERVICE_SCRIPTS "/") + name;
	}

	return true;
}

bool config_t::is_valid() const
{
	if (exec.empty())
		return false;

	return true;
}

void config_t::clear()
{
	name.clear();

	exec.clear();

	is_script = false;

	onstop_exec.clear();

	wipe_log = false;
	logfile = null_device;

	pidfile.clear();

	respawn = false;
	respawn_limit = 0;
	respawn_interval = 0;

}

void config_t::writeToBundle(Bundle& bundle) const
{
	bundle << name << exec << onstop_exec << wipe_log << logfile << pidfile
			<< respawn << respawn_limit << respawn_interval;
}

void config_t::readFromBundle(Bundle & bundle)
{
	bundle >> name >> exec >> onstop_exec >> wipe_log >> logfile >> pidfile
			>> respawn >> respawn_limit >> respawn_interval;
}
