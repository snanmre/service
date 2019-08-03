#ifndef DOMAINSERVER_H_
#define DOMAINSERVER_H_

#include <string>
#include <climits>

#include <sys/socket.h>
#include <sys/un.h>

#include "serializer/Bundle.h"

/**
 * @author Sinan Emre Kutlu
 *
 * @date 2013/08/29
 *
 * @brief This class is used for the purpose of IPC
 */
class DomainServer
{
public:
	/**
	 * @brief Create a socket and open it if path is available
	 * @param path		socket path
	 */
	DomainServer(const std::string & path = "");

	/**
	 * @brief Virtual destructor
	 */
	virtual ~DomainServer();

	/**
	 * @brief Open socket with given path
	 * @param path		address path
	 * @return 			true if successfully opened, otherwise false
	 */
	bool open(const std::string & path);

	/**
	 * @brief Check whether socket is open
	 * @return			true if successfully opened, otherwise false
	 */
	inline bool is_open() const;

	/**
	 * @brief Return socket file descriptor
	 */
	inline int getFd();

	/**
	 * @brief Return last opened socket path
	 * @return			socket path
	 */
	inline std::string path() const;

	/**
	 * @brief Set socket as BLOCKING/NONBLOCKING
	 * @param block		true for blocking mode
	 * @return			true if successfully done, otherwise false
	 */
	bool setBlockingMode(bool block);

	/**
	 * @brief Get socket blocking mode
	 * @return			true if BLOCKING, otherwise false
	 */
	bool blockingMode() const;

	/**
	 * @brief Send bundle to the destination process
	 * @param dst_path	destination socket address or empty string for previously defined address path
	 * @param bundle	bundle to send
	 * @return			true if successfully sent, otherwise false
	 */
	bool sendto(const std::string & dst_path, const Bundle & bundle);

	/**
	 * @brief Send data to the destination process
	 * @param dst_path	destination socket address or empty string for previously defined address path
	 * @param buf		buffer to send
	 * @param size		buffer size
	 * @return			sent byte count, or -1
	 */
	int sendto(const std::string & dst_path, const char * buf, const int size =
			-1);

	/**
	 * @brief Send data to the destination process
	 * @param dst_path	destination socket address or empty string for previously defined address path
	 * @param buf		buffer to send
	 * @param size		buffer size
	 * @return			sent byte count, or -1
	 */
	int sendto(const std::string & dst_path, const unsigned char * buf,
			const int size = -1);

	/**
	 * @brief Send data to the destination process
	 * @param dst_path	destination socket address or empty string for previously defined address path
	 * @param str		string to send
	 * @return			sent byte count, or -1
	 */
	int sendto(const std::string & dst_path, const std::string & str);

	/**
	 * @brief Reply last client
	 * @param bundle	bundle to send
	 * @return			sent byte count, or -1
	 */
	bool reply(const Bundle & bundle);

	/**
	 * @brief Reply last client
	 * @param buf		buffer to send
	 * @param size		buffer size
	 * @return			sent byte count, or -1
	 */
	int reply(const char * buf, const int size = -1);

	/**
	 * @brief Reply last client
	 * @param buf		buffer to send
	 * @param size		buffer size
	 * @return			sent byte count, or -1
	 */
	int reply(const unsigned char * buf, const int size = -1);

	/**
	 * @brief Reply last client
	 * @param str		string to send
	 * @return			sent byte count, or -1
	 */
	int reply(const std::string & str);

	/**
	 * @brief Receive bundle from the source process
	 * @param src_path	source socket address
	 * @param bundle	bundle to receive
	 * @return			true if successfully received, otherwise false
	 */
	bool recvfrom(std::string & src_path, Bundle & bundle);

	/**
	 * @brief Receive data from the source process
	 * @param src_path	source socket address
	 * @param buf		buffer to receive data
	 * @param size		buffer size
	 * @return			received byte count, otherwise -1
	 */
	int recvfrom(std::string & src_path, char * buf, const int size);

	/**
	 * @brief Receive data from the source process
	 * @param src_path	source socket address
	 * @param buf		buffer to receive data
	 * @param size		buffer size
	 * @return			received byte count, otherwise -1
	 */
	int recvfrom(std::string & src_path, unsigned char * buf, const int size);

	/**
	 * @brief Receive data from the source process
	 * @param src_path	source socket address
	 * @param response	string to receive data
	 * @return			received byte count, otherwise -1
	 */
	int recvfrom(std::string & src_path, std::string & response);

	/**
	 * @brief Query destination process with a timeout
	 * @param dst_path				destination socket address
	 * @param toSend				bundle to send
	 * @param toReceive				bundle to receive response
	 * @param milliseconds			timeout in milliseconds
	 * @return						true if response received
	 */
	bool query(const std::string & dst_path, const Bundle & toSend,
			Bundle & toReceive, const long long milliseconds = LLONG_MAX);

	/**
	 * @brief Clear socket receive buffer
	 */
	void clear();

	/**
	 * @brief Close socket
	 */
	void close();

protected:
	std::string socket_path;
	int socket_fd;
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	socklen_t address_length;

	/** socket open flag */
	bool b_open;

	/** client socket path */
	std::string client_path;

	unsigned char tmpBuf[16384];
};

bool DomainServer::is_open() const
{
	return b_open;
}

inline int DomainServer::getFd()
{
	return socket_fd;
}

std::string DomainServer::path() const
{
	return socket_path;
}

#endif /* DOMAINSOCKET_H_ */
