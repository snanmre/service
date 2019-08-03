#include "ipc/DomainServer.h"

#include <cstring>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <iostream>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>

#include "Debug.h"
#include "Timer.h"

using namespace std;

DomainServer::DomainServer(const string & path) :
		socket_path(path), socket_fd(-1), address_length(
				sizeof(struct sockaddr_un)), b_open(false)
{
	if (!path.empty())
		open(path);
}

DomainServer::~DomainServer()
{
	close();
}

bool DomainServer::open(const string & path)
{
	b_open = true;

	socket_path = path;

	socket_fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
	if (socket_fd < 0)
	{
		DD("socket() failed: %s\n", strerror(errno));
		b_open = false;
	}
	else
	{
		// fill server address

		::memset(&server_address, 0, sizeof(server_address));
		server_address.sun_family = AF_UNIX;

		// copy path
		::strcpy(server_address.sun_path + 1, path.c_str());
		// make path abstract
		server_address.sun_path[0] = 0;

		if (::bind(socket_fd, (const struct sockaddr *) &server_address,
				sizeof(struct sockaddr_un)) < 0)
		{
			DD("bind() failed: %s\n", strerror(errno));
			b_open = false;
		}
	}

	// fill client address

	::memset(&client_address, 0, sizeof(client_address));
	client_address.sun_family = AF_UNIX;

	return b_open;
}

bool DomainServer::setBlockingMode(bool block)
{
	bool res = true;

	int flags;

	if ((flags = ::fcntl(socket_fd, F_GETFL)) < 0)
	{
		DD("fcntl(F_GETFL) failed: %s\n", strerror(errno));
		res = false;
	}
	else
	{
		flags = block ? (flags & (~O_NONBLOCK)) : (flags | O_NONBLOCK);

		if (::fcntl(socket_fd, F_SETFL, flags) < 0)
		{
			DD("fcntl(F_SETFL) failed: %s\n", strerror(errno));
			res = false;
		}
	}

	return res;
}

bool DomainServer::blockingMode() const
{
	int flags;

	if ((flags = ::fcntl(socket_fd, F_GETFL)) < 0)
	{
		DD("fcntl(F_GETFL) failed: %s\n", strerror(errno));
		return false;
	}

	return !(flags & O_NONBLOCK);
}

bool DomainServer::sendto(const string& dst_path, const Bundle & bundle)
{
	unsigned char * buf = tmpBuf;
	int bufSize = sizeof(tmpBuf);
	if (bundle.byteCount() > bufSize)
	{
		bufSize = bundle.byteCount();
		buf = new unsigned char[bufSize];
	}

	int len = bundle.exportData(buf, bufSize);
	if (len < 0)
		return false;

	bool b = (len == sendto(dst_path, buf, len));
	if (buf != tmpBuf)
	{
		delete[] buf;
		buf = NULL;
	}
	return b;
}

int DomainServer::sendto(const string & dst_path, const char* buf,
		const int size)
{
	return sendto(dst_path, (unsigned char *) buf, size);
}

int DomainServer::sendto(const string & dst_path, const unsigned char* buf,
		const int size)
{
	if (!b_open)
		return -1;

	const string & dst_client_path =
			(dst_path.empty()) ? client_path : dst_path;

	// if destination path is empty, then send to last client
	if (dst_client_path.empty())
		throw std::invalid_argument(
				"DomainServer::sendto last client is not available");

	// destination address
	::memset(&client_address, 0, sizeof(client_address));
	client_address.sun_family = AF_UNIX;
	client_address.sun_path[0] = 0;
	strcpy(client_address.sun_path + 1, dst_client_path.c_str());

	client_path = dst_client_path;

	size_t bsize = (size < 0) ? ::strlen((const char *) buf) + 1 : size; // +1 for '\0'

	int wlen = ::sendto(socket_fd, buf, bsize, 0,
			(struct sockaddr *) &client_address, address_length);

	if (wlen < 0)
	{
		DD("sendto(%d, %s) failed: %s\n", socket_fd, dst_client_path.c_str(),
				strerror(errno));
	}

	return wlen;
}

int DomainServer::sendto(const string & dst_path, const string & str)
{
	return this->sendto(dst_path, str.c_str(), str.length());
}

bool DomainServer::reply(const Bundle & bundle)
{
	return sendto("", bundle);
}

int DomainServer::reply(const char * buf, const int size)
{
	return sendto("", (unsigned char *) buf, size);
}

int DomainServer::reply(const unsigned char * buf, const int size)
{
	return sendto("", buf, size);
}

int DomainServer::reply(const string & str)
{
	::strcpy((char *) tmpBuf, str.c_str());

	return sendto("", tmpBuf, str.length());
}

bool DomainServer::recvfrom(string& src_path, Bundle& bundle)
{
	int len = recvfrom(src_path, tmpBuf, sizeof(tmpBuf));

	if (len < 0)
		return false;

	if (!bundle.importData(tmpBuf, len))
		return false;

	return true;
}

int DomainServer::recvfrom(string& src_path, char* buf, const int size)
{
	return recvfrom(src_path, (unsigned char *) buf, size);
}

int DomainServer::recvfrom(string& src_path, unsigned char* buf, const int size)
{
	if (!b_open)
		return -1;

	int rlen = ::recvfrom(socket_fd, buf, size, 0,
			(struct sockaddr *) &client_address, &address_length);

	if (rlen > 0)
	{
		src_path = (client_address.sun_path + 1);

		if (src_path.empty())
			DD("recvfrom() : source path is empty\n");

		client_path = src_path;
	}

	return rlen;
}

int DomainServer::recvfrom(string & src_path, string & response)
{
	int len = this->recvfrom(src_path, tmpBuf, sizeof(tmpBuf));

	if (len <= 0)
		return len;

	tmpBuf[len] = '\0';

	response.assign((char *) tmpBuf);

	return len;
}

//bool DomainServer::query(const string & dst_path, const Bundle & toSend, Bundle & toReceive, const long long milliseconds)
//{
//	clear();
//
//	if (!sendto(dst_path, toSend))
//		return false;
//
//	Timer timer;
//	string src;
//
//	timer.set(milliseconds);
//
//	while (true)
//	{
//		if (timer.isTimeout())
//			return false;
//
//		if (recvfrom(src, toReceive))
//			break;
//
//		usleep(100000);
//	} // end-of-while timeout
//
//	return true;
//}

bool DomainServer::query(const string & dst_path, const Bundle & toSend,
		Bundle & toReceive, const long long milliseconds)
{
	clear();

	if (!sendto(dst_path, toSend))
		return false;

	string src;
	fd_set fd_read;
	struct timeval tv;

	tv.tv_sec = milliseconds / 1000;
	tv.tv_usec = (milliseconds - tv.tv_sec * 1000) * 1000;

	FD_ZERO(&fd_read);
	FD_SET(socket_fd, &fd_read);

	int rc = ::select(socket_fd + 1, &fd_read, (fd_set *) NULL, (fd_set *) NULL,
			&tv);
	if (rc == 0)
	{
		// timeout

		return false;
	}
	else if (rc < 0)
	{
		DD("select() failed: %s\n", strerror(errno));
		return false;
	}

	// now, rc > 0

	if (!FD_ISSET(socket_fd, &fd_read))
		return false;

	return recvfrom(src, toReceive);
}

//void DomainServer::clear()
//{
//	bool b_blocking = blockingMode();
//
//	// change to nonblocking
//	if (b_blocking)
//		this->setBlockingMode(false);
//
//	string src;
//	char ch;
//
//	// drain receive buffer
//	while (this->recvfrom(src, &ch, 1) > 0);
//
//	// reset blocking mode
//	if (b_blocking)
//		this->setBlockingMode(true);
//}

void DomainServer::clear()
{
	int bytesAvailable;

	// get the number of bytes in input buffer
	if (::ioctl(socket_fd, FIONREAD, &bytesAvailable) < 0)
	{
		DD("ioctl(FIONREAD) failed: %s\n", strerror(errno));
		return;
	}

	// clear if data available
	if (bytesAvailable > 0)
	{
		char buf[bytesAvailable];
		string src;
		this->recvfrom(src, buf, bytesAvailable);
	}
}

void DomainServer::close()
{
	if (b_open)
		::close(socket_fd);

	b_open = false;
}
