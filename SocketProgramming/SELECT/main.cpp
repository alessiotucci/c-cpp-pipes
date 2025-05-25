#include "SelectServer.hpp"

int main() {
    SelectServer server(8080);
    server.run();
    return 0;
}

