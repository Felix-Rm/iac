#pragma once

#ifdef ARDUINO

#    include <ESP8266WiFi.h>

#    include "connection.hpp"

namespace iac {

class SocketConnection : public Connection {
   public:
    SocketConnection(int port);
    virtual ~SocketConnection() = default;

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

class SocketClientConnection : public SocketConnection {
   public:
    SocketClientConnection(const char* ip, int port);
    ~SocketClientConnection() = default;

    bool open() override;
    bool close() override;

    void printBuffer(int cols = 4);

   private:
    const char* m_ip;
};

class SocketServerConnection : public SocketConnection {
   public:
    SocketServerConnection(int port);
    ~SocketServerConnection() = default;

    bool open() override;
    bool close() override;

    void printBuffer(int cols = 4);

   private:
    WiFiServer m_server;
};
}  // namespace iac
#endif