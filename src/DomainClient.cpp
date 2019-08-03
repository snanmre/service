/*
 * DomainClient.cpp
 *
 *  Created on: 06 Eyl 2013
 *      Author: root
 */

#include "ipc/DomainClient.h"

#include <pthread.h>

#include <stdexcept>
#include <cstdio>

#include <unistd.h>

using namespace std;

extern char * __progname;

unsigned int DomainClient::obj_id = 0;

DomainClient::DomainClient() :
		DomainServer()
{
	++DomainClient::obj_id;

	open(); // check with is_open()
}

DomainClient::~DomainClient()
{
}

bool DomainClient::open(void)
{
	sprintf((char *) tmpBuf, "%s__%06u_%06lu_%06u", __progname, getpid(),
			pthread_self(), DomainClient::obj_id);

	return DomainServer::open((char *) tmpBuf);
}

bool DomainClient::open(const string & path)
{
	throw std::runtime_error(
			"DomainClient::open : This function can not be used.");
}
