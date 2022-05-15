#pragma once

#ifdef ARDUINO

#    include <ESP8266WiFi.h>

#    include "local_transport_route.hpp"

namespace iac {

class SocketTransportRoute : public LocalTransportRoute {
   public:
    SocketTransportRoute(int port);
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
    int m_port;

    WiFiClient m_client;

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
    const char* m_ip;
};

class SocketServerTransportRoute : public SocketTransportRoute {
   public:
    SocketServerTransportRoute(int port);
    ~SocketServerTransportRoute() = default;

    bool open() override;
    bool close() override;

    void printBuffer(int cols = 4);

   private:
    WiFiServer m_server;
};
}  // namespace iac
#endif