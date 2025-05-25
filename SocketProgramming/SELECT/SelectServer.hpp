#ifndef SELECTSERVER_HPP
#define SELECTSERVER_HPP

#include <string>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>

class SelectServer
{
private:
	int listener;
	int port;
	fd_set master_set;
	fd_set read_fds;
	int fdmax;

	std::map<int, std::string> client_buffers;

	void initSocket();
	void runLoop();
	void handleNewConnection();
	void handleClientMessage(int client_fd);
	std::string buildHttpResponse();

public:
	SelectServer(int port);
	~SelectServer();
	void run();
};

#endif

