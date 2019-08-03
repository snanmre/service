#ifndef SERVICE_CLIENT_H_
#define SERVICE_CLIENT_H_

class service_client
{
public:
	service_client();

	static void exit_with_usage(int exit_code);

	int process(int argc, char * argv[]);
};

#endif /* SERVICE_CLIENT_H_ */
