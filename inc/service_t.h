#ifndef SERVICE_T_H_
#define SERVICE_T_H_

#include <string>
#include <ostream>

#include <sys/types.h>

#include <serializer/Serializable.h>
#include <Timer.h>

#include "config_t.h"

class service_t: public Serializable
{
	friend std::ostream & operator <<(std::ostream & o, const service_t & s);
public:
	service_t();

	bool start();
	bool stop();

	bool is_running() const;

	bool import(const std::string & filepath);

//protected:
	static const std::string default_shell;

	/**
	 * @brief Create a daemon process and start service;
	 * @return true if fork successful
	 * @warning Only parent process returns
	 */
	bool daemonize();

	/** @brief Redirect std fds and close opened files/sockets (inherited from parent process) */
	void redirect_fds();

	/** @brief Clear members */
	void clear();

	void on_post_start();
	void on_post_stop();

	virtual void writeToBundle(Bundle & bundle) const;
	virtual void readFromBundle(Bundle & bundle);

	config_t cfg;
	pid_t pid;

	int respawn_count;
	Timer respawn_timer;
	bool respawn_timer_enabled;
};

#endif /* SERVICE_T_H_ */
