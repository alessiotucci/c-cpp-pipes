#include "SelectServer.hpp"
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <arpa/inet.h>

SelectServer::SelectServer(int port) : listener(-1), port(port), fdmax(0)
{
	FD_ZERO(&master_set);
	FD_ZERO(&read_fds);
}

SelectServer::~SelectServer()
{
	if (listener != -1)
		close(listener);
}

void SelectServer::run()
{
	initSocket();
	runLoop();
}

void SelectServer::initSocket()
{
    struct sockaddr_in addr;

    // Create socket
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow address reuse
    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
	{
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind socket
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(listener, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Add listener to fd set
    FD_SET(listener, &master_set);
    fdmax = listener;

    std::cout << "Listening on port " << port << std::endl;
}

void SelectServer::runLoop() {
    while (true) {
        read_fds = master_set; // copy master
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    handleNewConnection();
                } else {
                    handleClientMessage(i);
                }
            }
        }
    }
}

void SelectServer::handleNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int newfd = accept(listener, (struct sockaddr*)&client_addr, &addrlen);
    if (newfd < 0) {
        perror("accept");
        return;
    }

    FD_SET(newfd, &master_set);
    if (newfd > fdmax)
        fdmax = newfd;

    std::cout << "New connection accepted (socket " << newfd << ")" << std::endl;
}

std::string SelectServer::parseRequestLine(const std::string& request, std::string& method, std::string& path) {
    std::istringstream stream(request);
    std::string line;
    if (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        lineStream >> method >> path;
    }
    return path;
}

void SelectServer::handleGet(int client_fd, const std::string& path) {
    std::ostringstream body;

    if (path == "/" || path == "/index.html") {
        std::ifstream file("index.html");
        if (!file) {
            body << "<h1>404 Not Found</h1>";
            std::string response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n" + body.str();
            send(client_fd, response.c_str(), response.length(), 0);
            return;
        }
        body << file.rdbuf();
    } else if (path.find("/get_test") == 0) {
        // Echo query string
        size_t q = path.find('?');
        std::string query = (q != std::string::npos) ? path.substr(q + 1) : "";
        body << "<h1>GET Received</h1><p>Query: " << query << "</p>";
    } else {
        body << "<h1>404 Not Found</h1>";
        std::string response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n" + body.str();
        send(client_fd, response.c_str(), response.length(), 0);
        return;
    }

    std::ostringstream response;
    std::string html = body.str();
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << html.length() << "\r\n\r\n";
    response << html;
    send(client_fd, response.str().c_str(), response.str().length(), 0);
}
void SelectServer::handlePost(int client_fd, const std::string& path, const std::string& body) {
    std::ostringstream html;
    if (path == "/post_test") {
        html << "<h1>POST Received</h1><p>Body: " << body << "</p>";
    } else {
        html << "<h1>404 Not Found</h1>";
        std::string response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n" + html.str();
        send(client_fd, response.c_str(), response.length(), 0);
        return;
    }

    std::ostringstream response;
    std::string page = html.str();
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << page.length() << "\r\n\r\n";
    response << page;
    send(client_fd, response.str().c_str(), response.str().length(), 0);
}

//TODO
void SelectServer::handleClientMessage(int client_fd)
{
    char buf[4096];
    int nbytes = recv(client_fd, buf, sizeof(buf), 0);

    if (nbytes <= 0)
	{
        if (nbytes == 0)
            std::cout << "Socket " << client_fd << " closed the connection." << std::endl;
        else
            perror("recv");
        close(client_fd);
        FD_CLR(client_fd, &master_set);
        return;
    }

    std::string request(buf, nbytes);
    std::string method, path;
    std::string firstLine = parseRequestLine(request, method, path);

    if (method == "GET")
	{
        handleGet(client_fd, path);
    }
	else if (method == "POST")
	{
        // Extract body after \r\n\r\n
        size_t bodyPos = request.find("\r\n\r\n");
        std::string body = (bodyPos != std::string::npos) ? request.substr(bodyPos + 4) : "";
        handlePost(client_fd, path, body);
    }
	else
	{
        std::string response = "HTTP/1.1 501 Not Implemented\r\n\r\n";
        send(client_fd, response.c_str(), response.length(), 0);
    }
    close(client_fd);
    FD_CLR(client_fd, &master_set);
}


std::string SelectServer::buildHttpResponse()
{
    std::ifstream file("index.html");
    std::ostringstream body;

    if (!file) {
        body << "<html><body><h1>404 Not Found</h1></body></html>";
        std::ostringstream response;
        response << "HTTP/1.1 404 Not Found\r\n";
        response << "Content-Length: " << body.str().length() << "\r\n";
        response << "Content-Type: text/html\r\n";
        response << "\r\n" << body.str();
        return response.str();
    }

    body << file.rdbuf();
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Length: " << body.str().length() << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "\r\n" << body.str();
    return response.str();
}

