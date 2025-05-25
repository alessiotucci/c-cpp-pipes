#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define PORT 8080       // Port number to listen on
#define MAX_EVENTS 10   // Maximum number of events to handle per epoll_wait call

int main() {
    int listener;
    struct sockaddr_in addr;

    // Create a TCP socket
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Enable reuse of the address
    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Setup the server address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on all interfaces
    addr.sin_port = htons(PORT);
  
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(listener, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
  
    std::cout << "epoll()-based server listening on port " << PORT << std::endl;

    // Create an epoll instance. epoll_create1 is available on newer Linux kernels.
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
  
    // Register the listening socket with the epoll instance for input events (new connections)
    struct epoll_event ev;
    ev.events = EPOLLIN;  // Notify when ready for reading
    ev.data.fd = listener;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &ev) < 0) {
        perror("epoll_ctl: listener");
        exit(EXIT_FAILURE);
    }
  
    // Buffer to hold the events returned by epoll_wait
    struct epoll_event events[MAX_EVENTS];
  
    // Main event loop using epoll
    while (true) {
        // Wait indefinitely for events (blocking call)
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
  
        // Loop through all triggered events
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listener) {
                // The listening socket is ready: new incoming connection!
                struct sockaddr_in client_addr;
                socklen_t addrlen = sizeof(client_addr);
                int newfd = accept(listener, (struct sockaddr*)&client_addr, &addrlen);
                if (newfd < 0) {
                    perror("accept");
                    continue;
                }
      
                // Register the new connection with epoll for reading
                ev.events = EPOLLIN;
                ev.data.fd = newfd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newfd, &ev) < 0) {
                    perror("epoll_ctl: newfd");
                    exit(EXIT_FAILURE);
                }
                std::cout << "New connection accepted (socket " << newfd << ")" << std::endl;
            } else {
                // A connected client socket is ready to be read from
                char buf[256];
                int nbytes = recv(events[i].data.fd, buf, sizeof(buf), 0);
                if (nbytes <= 0) {
                    // Connection closed or error occurred
                    if (nbytes == 0) {
                        std::cout << "Socket " << events[i].data.fd << " closed the connection" << std::endl;
                    } else {
                        perror("recv");
                    }
                    close(events[i].data.fd);
                    // Remove the closed fd from the epoll instance
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                } else {
                    // Echo the received data back to the client
                    send(events[i].data.fd, buf, nbytes, 0);
                }
            }
        }
    }
  
    // Cleanup: this code is unreachable in this infinite loop but demonstrates proper closure.
    close(listener);
    close(epoll_fd);
    return 0;
}

