#ifndef CONFIG_T_H_
#define CONFIG_T_H_

#include <string>

#include <serializer/Serializable.h>

class config_t: public Serializable
{
public:
	config_t();

	bool import(const std::string & filepath);

	/** @brief Clear members */
	void clear();

	/** Return true if this is a valid config */
	bool is_valid() const;

	static const std::string null_device;
	static std::string dirpath_pid;

	virtual void writeToBundle(Bundle & bundle) const;
	virtual void readFromBundle(Bundle & bundle);

	/** @brief config file name (and also service name) */
	std::string name;

	/** @brief service command */
	std::string exec;
	bool is_script;
	/** @brief temporary script filepath if script defined */
	std::string script_filepath;

	/** @brief command to run on stop */
	std::string onstop_exec;

	bool wipe_log;
	std::string logfile;

	std::string pidfile;
	bool respawn;
	int respawn_limit;
	int respawn_interval;
};

#endif /* CONFIG_T_H_ */
