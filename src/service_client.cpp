#include "service_client.h"

#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <ipc/ipc.h>

#include "ServiceMessages.h"
#include "service_server.h"

using namespace std;

#define APP_VERSION									"1.2"

#define CLI_COMMAND_START							"start"
#define CLI_COMMAND_STOP							"stop"
#define CLI_COMMAND_RESTART							"restart"
#define CLI_COMMAND_STATUS							"status"
#define CLI_COMMAND_SHOW							"show"
#define CLI_COMMAND_LIST							"list"

extern char * __progname;

service_client::service_client()
{
}

void service_client::exit_with_usage(int exit_code)
{
	cerr << __progname << " " << APP_VERSION << endl << "USAGE:" << endl << "\t"
			<< __progname << "  -h" << endl << "\t" << __progname << "  -d"
			<< endl << endl << "\t" << __progname << "  " << CLI_COMMAND_START
			<< "  <service>" << endl << "\t" << __progname << "  "
			<< CLI_COMMAND_STOP << "  <service>" << endl << "\t" << __progname
			<< "  " << CLI_COMMAND_RESTART << "  <service>" << endl << "\t"
			<< __progname << "  " << CLI_COMMAND_STATUS << "  <service>" << endl
			<< "\t" << __progname << "  " << CLI_COMMAND_SHOW << "  <service>"
			<< endl << "\t" << __progname << "  " << CLI_COMMAND_LIST << "  -v"
			<< endl;

	::exit(exit_code);
}

int service_client::process(int argc, char * argv[])
{
	if (argc < 2)
	{
		service_client::exit_with_usage(1);
	}
	if (argc == 2)
	{
		if (::strcmp(argv[1], "-h") == 0)
		{
			service_client::exit_with_usage(0);
		}

		if (::strcmp(argv[1], "-d") == 0)
		{
			service_server ss;
			ss.run();
			return 0;
		}
	}

	string command = argv[1];
	string name = (argv[2] ? argv[2] : "");
	Bundle bundle;
	bool verbose = false;

	if (command == CLI_COMMAND_START)
	{
		if (argc != 3)
			service_client::exit_with_usage(1);
		bundle << SERVICE_CMD_START << name;
	}
	else if (command == CLI_COMMAND_STOP)
	{
		if (argc != 3)
			service_client::exit_with_usage(1);
		bundle << SERVICE_CMD_STOP << name;
	}
	else if (command == CLI_COMMAND_RESTART)
	{
		if (argc != 3)
			service_client::exit_with_usage(1);
		bundle << SERVICE_CMD_RESTART << name;
	}
	else if (command == CLI_COMMAND_STATUS)
	{
		if (argc != 3)
			service_client::exit_with_usage(1);
		bundle << SERVICE_CMD_STATUS << name;
	}
	else if (command == CLI_COMMAND_SHOW)
	{
		if (argc != 3)
			service_client::exit_with_usage(1);
		bundle << SERVICE_CMD_SHOW << name;
	}
	else if (command == CLI_COMMAND_LIST)
	{
		if (argc == 3 && ::strcmp(argv[2], "-v") == 0)
			verbose = true;
		else if (argc != 2)
			service_client::exit_with_usage(1);

		bundle << SERVICE_CMD_LIST;
	}
	else
	{
		service_client::exit_with_usage(1);
	}

	DomainClient c;

	if (!c.is_open())
	{
		cerr << "ERROR: Could not open client socket." << endl;
		exit(1);
	}

	Bundle response;
	if (!c.query(IPC_PATH_SERVICE, bundle, response, 5000))
	{
		cerr << "ERROR: Could not get response." << endl;
		exit(1);
	}

	if (!response.getBool())
	{
		string message = "";
		if (response.count() > 0
				&& response.getNextType() == Bundle::TYPE_STRING)
			message = response.getString();

		cerr << "ERROR: command failed: " << message << endl;
		exit(1);
	}

	//	response.printElements();

	if (command == CLI_COMMAND_START)
	{
		cout << name << " is started." << endl;
	}
	else if (command == CLI_COMMAND_STOP)
	{
		cout << name << " is stopped." << endl;
	}
	else if (command == CLI_COMMAND_RESTART)
	{
		cout << name << " is restarted." << endl;
	}
	else if (command == CLI_COMMAND_STATUS)
	{
		bool is_running = response.getBool();
		cout << name << " is " << (is_running ? "running." : "stopped.")
				<< endl;
	}
	else if (command == CLI_COMMAND_SHOW)
	{
		service_t s;
		response >> s;
		cout << s << endl;
	}
	else if (command == CLI_COMMAND_LIST)
	{
		service_t s;
		bool b;
		while (response.count() > 0)
		{
			response >> s;

			if (verbose)
			{
				cout << "---------------------------------------" << endl << s
						<< endl;
			}
			else
			{
				b = s.is_running();
				cout << s.cfg.name << ": ";
				if (b)
					cout << "running, process " << s.pid;
				else
					cout << "stopped.";
				cout << endl;
			}
		}
	}

	return 0;
}
