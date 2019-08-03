#ifndef DOMAINCLIENT_H_
#define DOMAINCLIENT_H_

#include "ipc/DomainServer.h"

/**
 * @author Sinan Emre Kutlu
 *
 * @date 2013/09/06
 *
 * @brief This class is used for the purpose of IPC
 *
 */
class DomainClient: public DomainServer
{
public:
	/**
	 * @brief Create and open a client socket (use is_open() for checking)
	 */
	DomainClient();

	/**
	 * @brief Virtual destructor
	 */
	virtual ~DomainClient();

protected:
	/**
	 * @brief Open socket
	 * @return true if successfully opened, otherwise false
	 */
	bool open(void);

private:
	/** unique object id */
	static unsigned int obj_id;

	/**
	 * @brief Open socket with given path
	 * @param path
	 * @return
	 */
	bool open(const std::string & path);
};

#endif /* DOMAINCLIENT_H_ */
