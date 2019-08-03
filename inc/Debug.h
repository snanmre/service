/*
 * Debug.h
 *
 *  Created on: 2 May 2014
 *      Author: root
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <cstdio>
#include <sstream>
#include <string>

#include <netinet/in.h>

#include "serializer/Bundle.h"

#define DEBUG_LEVEL_MIN				1
#define DEBUG_LEVEL_MAX				3

#define DD(fmt, ...)		fprintf(stderr, "%*s" fmt,(80 - fprintf(stderr, "%s[%d]%s", __FILE__, __LINE__, __func__)), " ", ##__VA_ARGS__)

/**
 * @author Sinan Emre Kutlu
 *
 * @date 2014/07/03
 *
 * @brief This class is used to print debug messages.
 *
 * In addition, it prints colored messages on terminal (if enabled, see #setPrintLevel)
 *
 * In terminal, you can set;
 * - #ENV_DEBUGLEVEL: application's print level with integers between 0 and #DEBUG_LEVEL_MAX
 * - #ENV_DEBUGCOLOR: color enable flag with 0 or nonzero
 *
 * For example;
 * @code
 * 		DEBUGLEVEL=3 DEBUGCOLOR=1 ./debugTest
 * @endcode
 *
 * Usage examples;
 * @code
 *		Debug debug("application name");
 *		debug.write(LEVEL, ...);
 * @endcode
 */
class Debug
{
public:
	/** @brief Debug levels */
	enum DebugLevel
	{
		ERROR = DEBUG_LEVEL_MIN, WARNING, INFO
	};

	/**
	 * @brief Default constructor
	 * @param appname				application name to set (default is binary file name)
	 */
	Debug(const std::string & appname = "");

	/**
	 * @brief Copy constructor
	 * @param d						another Debug instance
	 * @warning						socket descriptor is not copied.
	 */
	Debug(const Debug & d);

	/**
	 * @brief Destructor
	 */
	virtual ~Debug();

	const Debug & operator=(const Debug & d);

	/**
	 * @brief Send debug information to the DebugServer
	 * @param type					debug level
	 * @param message				message
	 * @return						true if successfully sent, otherwise false
	 */
	bool write(const DebugLevel type, const std::string & message);

	/**
	 * @brief Send debug information to the DebugServer
	 * @param type					debug level
	 * @param format				formatted message
	 * @return						true if successfully sent, otherwise false
	 */
	bool write(const DebugLevel type, const char * format, ...);

	/** @brief Send error message */
	bool e(const std::string & message);
	/** @brief Send error message */
	bool e(const char * format, ...);

	/** @brief Send warning message */
	bool w(const std::string & message);
	/** @brief Send warning message */
	bool w(const char * format, ...);

	/** @brief Send info message */
	bool i(const std::string & message);
	/** @brief Send info message */
	bool i(const char * format, ...);

	/**
	 * @brief Set application that is used in debug messages
	 * @param appname				application name to set (default is binary file name)
	 */
	void setApplicationName(const std::string & appname = "");

	void setTag(const std::string & tag);

	/**
	 * @brief Enable or disable sending messages to DebugServer (enabled by default)
	 * @param enabled
	 */
	void setEnabled(bool enabled);

	/**
	 * @brief Set print level for printing to the stdout
	 * @param level					print level (0 to disable or ERROR, WARNING, INFO)
	 */
	void setPrintLevel(int level);

	/** @brief Get print level for stdout */
	int getPrintLevel() const;

	/**
	 * @brief Set colored messages enabled
	 * @param enable				true for colored messages
	 */
	void setColorEnabled(bool enable);

protected:

	/** @brief Initialize resources */
	void init();
	/** @brief Release resources */
	void finalize();

	/** @brief Initialize print level with environment variable DEBUGLEVEL */
	void printLevelInit();

	/** @brief Initialize color enabled flag with terminal color support */
	void colorInit();

	/** @brief application name */
	std::string appname;

	/** @brief debug tag */
	std::string tag;

	/** @brief sending messages enable flag */
	bool b_enabled;

	/** @brief colored messages enable flag */
	bool b_color;
	/** @brief true if color enable flag set from env */
	bool b_colorEnv;

	unsigned char tmpBuf[4096];

	/** @brief print level, default Debug::ERROR */
	int printLevel;
	/** @brief true if print level set from env */
	bool b_printLevelEnv;
};

#endif /* DEBUG_H_ */
