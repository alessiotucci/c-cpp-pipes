#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <vector>

#define PORT 8080       // Port to listen on
#define TIMEOUT 10000   // Timeout for poll() in milliseconds (10 seconds)

int main()
{
	int listener;
	struct sockaddr_in addr;

	// Create a TCP socket
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Enable the socket option to reuse the address
	int yes = 1;
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	// Set up the address structure
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
	addr.sin_port = htons(PORT);

	if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}

	// Begin listening for incoming connections
	if (listen(listener, 10) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	std::cout << "poll()-based server listening on port " << PORT << std::endl;

	// Create a vector of pollfd structures to keep track of file descriptors
	std::vector<struct pollfd> fds;
	struct pollfd pfd;
	pfd.fd = listener;
	pfd.events = POLLIN;   // Monitor for input data (or a new connection)
	fds.push_back(pfd);

	// Main server loop: check for events on all file descriptors
	while (true)
	{
		// poll() waits for events on the file descriptors in the vector
		int poll_count = poll(&fds[0], fds.size(), TIMEOUT);
		if (poll_count < 0)
		{
			perror("poll");
			exit(EXIT_FAILURE);
		}
		else if (poll_count == 0)
		{
			std::cout << "Poll timeout: No events occurred." << std::endl;
			continue;
		}

		// Loop over each file descriptor to see what triggered the event
		for (size_t i = 0; i < fds.size(); i++)
		{
			if (fds[i].revents & POLLIN)
			{ // Ready for reading
				if (fds[i].fd == listener)
				{
					// A new connection is waiting to be accepted
					struct sockaddr_in client_addr;
					socklen_t addrlen = sizeof(client_addr);
					int newfd = accept(listener, (struct sockaddr*)&client_addr, &addrlen);
					if (newfd < 0)
					{
						perror("accept");
						continue;
					}
					// Add the new file descriptor to our pollfd vector
					struct pollfd npfd;
					npfd.fd = newfd;
					npfd.events = POLLIN;
					fds.push_back(npfd);
					std::cout << "New connection accepted (socket " << newfd << ")" << std::endl;
				}
				else
				{
					// Data received on an existing connection
					char buf[256];
					int nbytes = recv(fds[i].fd, buf, sizeof(buf), 0);
					if (nbytes <= 0)
					{
						if (nbytes == 0)
						{
							std::cout << "Socket " << fds[i].fd << " closed its connection" << std::endl;
						}
						else
						{
							perror("recv");
						}
						close(fds[i].fd);
						// Remove this descriptor by erasing the element from the vector.
						fds.erase(fds.begin() + i);
						i--; // Adjust index since the vector shrank
					}
					else
					{
						// Echo back the data received
						send(fds[i].fd, buf, nbytes, 0);
					}
				}
			}
		}
	}
  
	return 0;
}

