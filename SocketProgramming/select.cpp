#include <iostream>
#include <cstdlib>
#include <cstring>      // for memset()
#include <unistd.h>     // for close()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // for sockaddr_in
#include <arpa/inet.h>  // for htons()

#define PORT 8080       // Port to listen on

int main() {
    int listener;               // Listening socket descriptor
    struct sockaddr_in addr;    // Server address structure
    fd_set master_set, read_fds;
    int fdmax;                  // Keep track of the highest descriptor value

    // Create a TCP socket
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow the socket address to be reused immediately after program termination
    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to any IP on PORT 8080
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Mark the socket as listening with a backlog of 10 connections
    if (listen(listener, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Initialize the master FD set and add the listening socket
    FD_ZERO(&master_set);
    FD_SET(listener, &master_set);
    fdmax = listener;  // Initially, the highest fd is the listener

    std::cout << "select()-based server listening on port " << PORT << std::endl;

    // Main server loop
    while (true) {
        // We need to pass a temporary copy of master_set because select() modifies it.
        read_fds = master_set;
        
        // Wait indefinitely for one or more socket descriptors to become ready
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        
        // Check which file descriptors are ready
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                // New connection is incoming on the listener socket
                if (i == listener) {
                    int newfd;
                    struct sockaddr_in client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    if ((newfd = accept(listener, (struct sockaddr *)&client_addr, &addrlen)) < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    // Add the new connection to the master set
                    FD_SET(newfd, &master_set);
                    if (newfd > fdmax) {
                        fdmax = newfd;
                    }
                    std::cout << "New connection accepted (socket " << newfd << ")" << std::endl;
                } else {
                    // Existing connection has data ready to read
                    char buf[256];
                    int nbytes = recv(i, buf, sizeof(buf), 0);
                    if (nbytes <= 0) {
                        // Connection closed or error
                        if (nbytes == 0) {
                            std::cout << "Socket " << i << " closed its connection" << std::endl;
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master_set);
                    } else {
                        // Echo the received message back to the client
                        send(i, buf, nbytes, 0);
                    }
                }
            }
        } // End for-loop over descriptors
    } // End main loop

    return 0;
}

