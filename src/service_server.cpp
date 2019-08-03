#include "service_server.h"

#include <cerrno>
#include <cstring>
#include <sstream>

#include <signal.h>
#include <unistd.h>

#include "fileutils.h"
#include "ServiceMessages.h"
#include "stringutils.h"

#define SERVICE_LIST_SEPERATOR					":"

using namespace std;

const std::string service_server::dirpath_service = DIRPATH_SERVICES;

service_server::service_server() :
		run_state(RS_INIT)
{
#ifdef _DEBUG
	debug.setEnabled(false);
	debug.setPrintLevel(Debug::INFO);
#endif

	init();
}

service_server::~service_server()
{
	finalize();
}

void service_server::run()
{
	debug.i("SERVICE DAEMON             pid = %d", getpid());

	while (true)
	{
		switch (run_state)
		{
		case RS_INIT:
			load_service_list();

			run_state = RS_SERVICE_CHECK;
			break;

		case RS_SERVICE_CHECK:
		{
			vector<map<string, service_t>::iterator> erase_list;
			for (map<string, service_t>::iterator it = running_services.begin();
					it != running_services.end(); ++it)
			{
				service_t & s = it->second;
				if (!s.is_running() && s.cfg.respawn
						&& !s.respawn_timer_enabled)
				{
					// set up respawn timer if limit not exceeded
					if (s.respawn_count < s.cfg.respawn_limit)
					{
						s.respawn_timer.set(s.cfg.respawn_interval * 1000);
						s.respawn_timer_enabled = true;
					}
					else
					{
//							running_services.erase(it);
						erase_list.push_back(it);
						continue;
					}
				}
				// erase if not running and respawn disabled
				else if (!s.is_running() && !s.cfg.respawn)
				{
//						running_services.erase(it);
					erase_list.push_back(it);
					continue;
				}
			} // end-of-for services

			// erase services
			if (!erase_list.empty())
			{
				while (!erase_list.empty())
				{
					running_services.erase(erase_list[erase_list.size() - 1]);
					erase_list.pop_back();
				}

				save_service_list();
			}

			run_state = RS_SERVICE_RESPAWN;
			break;
		}

		case RS_SERVICE_RESPAWN:
		{
			config_t temp;
			for (map<string, service_t>::iterator it = running_services.begin();
					it != running_services.end(); ++it)
			{
				service_t & s = it->second;

				if (s.respawn_timer_enabled && s.respawn_timer.isTimeout())
				{
					debug.w("respawning service " + s.cfg.name);

					// update with new config data
					if (!temp.import(get_config_filepath(s.cfg.name)))
					{
						debug.e(
								"Could not respawn service " + s.cfg.name
										+ ": import() failed");
					}
					else
					{
						s.cfg = temp;

						if (!s.start())
							debug.e("Could not respawn service " + s.cfg.name);
						else
						{
							save_service_list();
						}
					}
					++s.respawn_count;
					s.respawn_timer_enabled = false;
				}
			} // end-of-for services

			run_state = RS_HANDLE_COMMAND;
			break;
		}
		case RS_HANDLE_COMMAND:
			while (domain_server.recvfrom(client_address, bundle))
			{
				string command = bundle.getString();

				debug.i("Message text : " + command);
				debug.i("\t" + bundle.toString());

				// search for command
				if (command_handlers.find(command) == command_handlers.end())
				{
					debug.write(Debug::WARNING, "unknown command: " + command);
					domain_server.sendto(client_address,
							Bundle() << false << "unknown command");
					continue;
				}

				try
				{
					// process message
					(this->*command_handlers[command])(bundle);
				} catch (exception & e)
				{
					debug.e(e.what());
				}
			}

			run_state = RS_SERVICE_CHECK;
			break;
		}

		usleep(10000);
	} // end-of-while true
}

std::string service_server::get_config_filepath(
		const std::string & service_name)
{
	return dirpath_service + "/" + service_name + FILE_EXTENSION_CONFIG;
}

void service_server::load_service_list()
{
	vector<string> lines, parts;

	fileutils::load_file(filepath_service_list, lines);
	for (size_t i = 0; i < lines.size(); ++i)
	{
		stringutils::split(lines[i], SERVICE_LIST_SEPERATOR, parts);

		if (parts.size() == 2)
		{
			pid_t pid = ::atoi(parts[1].c_str());

			service_t s;

			// if running
			if (pid > 0 && ::kill(pid, 0) == 0)
			{
				if (s.import(get_config_filepath(parts[0])))
				{
					debug.w("%s service is already running... pid = %d",
							s.cfg.name.c_str(), pid);
					s.pid = pid;
					running_services[s.cfg.name] = s;
				}
			}
		}
	} // end-of-for lines
}

void service_server::save_service_list()
{
	stringstream ss;
	for (map<string, service_t>::const_iterator it = running_services.begin();
			it != running_services.end(); ++it)
	{
		const service_t & s = it->second;
		if (s.is_running())
		{
			ss << s.cfg.name << SERVICE_LIST_SEPERATOR << s.pid << endl;
		}
	}

	if (fileutils::save_file(filepath_service_list, ss.str(), 777))
	{
		debug.i("service list file updated.");
	}
	else
	{
		debug.e("Could not save service list file.");
	}
}

void service_server::handle_START(Bundle & bundle)
{
	if (bundle.count() != 1)
	{
		domain_server.sendto(client_address,
				Bundle() << false << "invalid argument.");
		return;
	}

	string name = bundle.getString();

	map<string, service_t>::iterator it = running_services.find(name);

	// if already running
	if (it != running_services.end() && it->second.is_running())
	{
		domain_server.sendto(client_address, Bundle() << true);
		return;
	}

	string filepath_cfg = get_config_filepath(name);
	if (!fileutils::exist(filepath_cfg))
	{
		domain_server.sendto(client_address,
				Bundle() << false << "config file not found.");
		return;
	}

	service_t s;
	if (!s.import(filepath_cfg))
	{
		domain_server.sendto(client_address,
				Bundle() << false << "error in config file.");
		return;
	}

	if (!s.start())
	{
		domain_server.sendto(client_address,
				Bundle() << false << "start() failed.");
		return;
	}

	// save running services
	running_services[name] = s;
	save_service_list();

	domain_server.sendto(client_address, Bundle() << true);
}

void service_server::handle_STOP(Bundle & bundle)
{
	if (bundle.count() != 1)
	{
		domain_server.sendto(client_address,
				Bundle() << false << "invalid argument.");
		return;
	}

	const string name = bundle.getString();

	string filepath_cfg = get_config_filepath(name);
	bool found = fileutils::exist(filepath_cfg, fileutils::FT_REG);

	map<string, service_t>::iterator it = running_services.find(name);

	// if already stopped
	if (it == running_services.end() && found)
	{
		domain_server.sendto(client_address, Bundle() << true);
		return;
	}

	if (!found)
	{
		domain_server.sendto(client_address,
				Bundle() << false << "config file not found.");
		return;
	}

	// if it is stopped
	if (!it->second.is_running())
	{
		domain_server.sendto(client_address, Bundle() << true);
	}
	else if (!it->second.stop())
	{
		domain_server.sendto(client_address,
				Bundle() << false << "stop() failed.");
		return;
	}

	// save running services
	running_services.erase(it);
	save_service_list();

	domain_server.sendto(client_address, Bundle() << true);
}

void service_server::handle_RESTART(Bundle & bundle)
{
	if (bundle.count() != 1)
	{
		domain_server.sendto(client_address,
				Bundle() << false << "invalid argument.");
		return;
	}

	string name = bundle.getString();

	map<string, service_t>::iterator it = running_services.find(name);

	// stop if running
	if (it != running_services.end())
	{
		if (!it->second.stop())
		{
			domain_server.sendto(client_address,
					Bundle() << false << "stop() failed.");
			return;
		}
	}

	// start

	string filepath_cfg = get_config_filepath(name);
	if (!fileutils::exist(filepath_cfg))
	{
		domain_server.sendto(client_address,
				Bundle() << false << "config file not found.");
		return;
	}

	service_t s;
	if (!s.import(filepath_cfg))

	{
		domain_server.sendto(client_address,
				Bundle() << false << "error in config file.");
		return;
	}

	if (!s.start())
	{
		domain_server.sendto(client_address,
				Bundle() << false << "start() failed.");
		return;
	}

	// save running services (pid changed)
	running_services[name] = s;
	save_service_list();

	domain_server.sendto(client_address, Bundle() << true);
}

void service_server::handle_STATUS(Bundle & bundle)
{
	if (bundle.count() != 1)
	{
		domain_server.sendto(client_address,
				Bundle() << false << "invalid argument.");
		return;
	}

	string name = bundle.getString();

	map<string, service_t>::iterator it = running_services.find(name);

	domain_server.sendto(client_address,
			Bundle() << true
					<< (it != running_services.end() && it->second.is_running()));
}

void service_server::handle_SHOW(Bundle & bundle)
{
	if (bundle.count() != 1)
	{
		domain_server.sendto(client_address,
				Bundle() << false << "invalid argument.");
		return;
	}

	string name = bundle.getString();
	Bundle response;

	map<string, service_t>::iterator it = running_services.find(name);
	if (it != running_services.end())
	{
		domain_server.sendto(client_address, Bundle() << true// command response
				<< it->second);	// information
		return;
	}

	string filepath_cfg = get_config_filepath(name);
	if (!fileutils::exist(filepath_cfg))
	{
		domain_server.sendto(client_address,
				Bundle() << false << "config file not found.");
		return;
	}

	service_t s;
	if (!s.import(filepath_cfg))
	{
		domain_server.sendto(client_address,
				Bundle() << false << "error in config file.");
		return;
	}

	domain_server.sendto(client_address, Bundle() << true	// command response
			<< s);	// information
}

void service_server::handle_LIST(Bundle & bundle)
{
	// we used map here to order services by their name
	map<string, service_t> all_services;

	if (!get_all_services(all_services))
	{
		domain_server.sendto(client_address,
				Bundle() << false << "could not read services.");
		return;
	}

	Bundle response;

	response << true;

	for (map<string, service_t>::const_iterator it = all_services.begin();
			it != all_services.end(); ++it)
		response << it->second;

	domain_server.sendto(client_address, response);
}

bool service_server::get_all_services(
		std::map<string, service_t> & all_services)
{
	for (map<string, service_t>::const_iterator it = running_services.begin();
			it != running_services.end(); ++it)
	{
		all_services[it->first] = it->second;
	}

	vector<string> filenames;
	service_t temp;

	fileutils::read_dir(dirpath_service, filenames);

	for (size_t i = 0; i < filenames.size(); ++i)
	{
		if (fileutils::extension(filenames[i]) == FILE_EXTENSION_CONFIG)
		{
			// if it can be imported
			if (temp.import(dirpath_service + "/" + filenames[i]))
			{
				// if not found in running services
				if (all_services.find(temp.cfg.name) == all_services.end())
				{
					all_services[temp.cfg.name] = temp;
				}
			}
		}
	}

	return true;
}

void service_server::init()
{
	config_init();
	ipc_init();
	handler_init();

	debug.i("file paths:");
	debug.i("service directory: %s", DIRPATH_SERVICES);
	debug.i("service script directory: %s", DIRPATH_SERVICE_SCRIPTS);
	debug.i("service pid directory: %s", DIRPATH_SERVICE_PIDS);
	debug.i("service list: %s", FILEPATH_SERVICES_LIST);
}

void service_server::finalize()
{
	ipc_finalize();
}

void service_server::config_init()
{
	filepath_service_list = FILEPATH_SERVICES_LIST;
}

void service_server::ipc_init()
{
	if (!domain_server.open(IPC_PATH_SERVICE))
	{
		debug.e("Could not open domain server socket.");
		exit(1);
	}

	domain_server.setBlockingMode(false);
}

void service_server::ipc_finalize()
{
	domain_server.close();
}

void service_server::handler_init()
{
	command_handlers[SERVICE_CMD_START] = &service_server::handle_START;
	command_handlers[SERVICE_CMD_STOP] = &service_server::handle_STOP;
	command_handlers[SERVICE_CMD_RESTART] = &service_server::handle_RESTART;
	command_handlers[SERVICE_CMD_STATUS] = &service_server::handle_STATUS;
	command_handlers[SERVICE_CMD_SHOW] = &service_server::handle_SHOW;
	command_handlers[SERVICE_CMD_LIST] = &service_server::handle_LIST;
}

