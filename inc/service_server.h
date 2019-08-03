#ifndef SERVICESERVER_H_
#define SERVICESERVER_H_

#include <map>
#include <string>

#include "service_t.h"
#include "ipc/ipc.h"
#include "Debug.h"

class service_server
{
public:
	service_server();
	virtual ~service_server();

	void run();

protected:

	enum RunState
	{
		RS_INIT = 0, RS_SERVICE_CHECK, RS_SERVICE_RESPAWN, RS_HANDLE_COMMAND
	};

	static const std::string dirpath_service;
	static std::string get_config_filepath(const std::string & service_name);

	/** @brief load service list file */
	void load_service_list();

	/** @brief save service list file */
	void save_service_list();

	void handle_START(Bundle & bundle);
	void handle_STOP(Bundle & bundle);
	void handle_RESTART(Bundle & bundle);
	void handle_STATUS(Bundle & bundle);
	void handle_SHOW(Bundle & bundle);
	void handle_LIST(Bundle & bundle);

	/** @brief get all services */
	bool get_all_services(std::map<std::string, service_t> & all_services);

	void init();
	void finalize();

	void config_init();

	void ipc_init();
	void ipc_finalize();

	void handler_init();

	RunState run_state;

	std::string filepath_service_list;

	DomainServer domain_server;
	std::string client_address;
	Bundle bundle;

	std::map<std::string, service_t> running_services;
	Debug debug;

	std::map<std::string, void (service_server::*)(Bundle &)> command_handlers;
};

#endif /* SERVICESERVER_H_ */
