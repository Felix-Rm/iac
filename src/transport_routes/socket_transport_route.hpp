#pragma once

#ifndef ARDUINO

#    include <arpa/inet.h>
#    include <netinet/in.h>
#    include <signal.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
#    include <sys/fcntl.h>
#    include <sys/ioctl.h>
#    include <sys/socket.h>
#    include <unistd.h>

#    include "transport_route.hpp"

namespace iac {

class SocketTransportRoute : public LocalTransportRoute {
   public:
    SocketTransportRoute(const char* ip, int port);
    virtual ~SocketTransportRoute() = default;

    size_t read(void* buffer, size_t size) override;
    size_t write(const void* buffer, size_t size) override;

    bool flush() override;
    bool clear() override;

    size_t available() override;

    operator bool() const {
        return m_good;
    };

   protected:
    const char* m_ip;
    in_addr_t m_addr = INADDR_ANY;
    int m_port;

    int m_rw_fd = -1;

    bool m_good = true;
};

class SocketClientTransportRoute : public SocketTransportRoute {
   public:
    SocketClientTransportRoute(const char* ip, int port);
    ~SocketClientTransportRoute() = default;

    bool open() override;
    bool close() override;

    void printBuffer(int cols = 4);

   private:
    sockaddr_in m_address;
};

class SocketServerTransportRoute : public SocketTransportRoute {
   public:
    SocketServerTransportRoute(const char* ip, int port);
    ~SocketServerTransportRoute() = default;

    bool open() override;
    bool close() override;

    void printBuffer(int cols = 4);

   private:
    int m_server_fd;
    sockaddr_in m_server_address, m_client_address;
};

}  // namespace iac

#endif