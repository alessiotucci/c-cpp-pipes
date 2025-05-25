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
	int fdmax;

	fd_set master_set;
	fd_set read_fds;

	std::map<int, std::string> client_buffers;

	void initSocket();
	void runLoop();
	void handleNewConnection();
	void handleClientMessage(int client_fd);
	std::string buildHttpResponse();
	

	//methods
	std::string parseRequestLine(const std::string& request, std::string& method, std::string& path);
	void handleGet(int client_fd, const std::string& path);
	void handlePost(int client_fd, const std::string& path, const std::string& body);

public:
	SelectServer(int port);
	~SelectServer();
	void run();
};

#endif

