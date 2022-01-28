
#ifdef ARDUINO

#include "socket_transport_route_arduino.hpp"

namespace iac {

SocketTransportRoute::SocketTransportRoute(int port) : m_port(port) {
}

size_t SocketTransportRoute::read(void* buffer, size_t size) {
    if (!m_client.connected()) return 0;

    size_t queue_read_size = read_put_back_queue(buffer, size);
    size_t socket_read_size = m_client.read((uint8_t*)buffer, size);

    // printf("write: [%d] %lu\n", m_rw_fd, size);

    return queue_read_size + socket_read_size;
}

size_t SocketTransportRoute::write(const void* buffer, size_t size) {
    if (!m_client.connected()) return 0;
    // printf("write: [%d] %lu\n", m_rw_fd, size);

    return m_client.write((uint8_t*)buffer, size);
}

bool SocketTransportRoute::flush() {
    if (!m_client.connected()) return false;

    m_client.flush();
    return true;
}

bool SocketTransportRoute::clear() {
    if (!m_client.connected()) return false;

    uint8_t buffer[64];
    size_t available_size = 0;

    while ((available_size = available())) {
        read(buffer, available_size > 64 ? 64 : available_size);
    }

    return true;
}

size_t SocketTransportRoute::available() {
    if (!m_client.connected()) return 0;

    return m_client.available() + available_put_back_queue();
}

SocketClientTransportRoute::SocketClientTransportRoute(const char* ip, int port) : SocketTransportRoute(port), m_ip(ip) {
}

bool SocketClientTransportRoute::open() {
    if (m_client.connect(m_ip, m_port)) {
        m_client.setNoDelay(true);
        return true;
    }
    return false;
}

bool SocketClientTransportRoute::close() {
    clear_put_back_queue();
    m_client.stop();
    return true;
}

SocketServerTransportRoute::SocketServerTransportRoute(int port) : SocketTransportRoute(port), m_server(m_port) {
    m_server.begin();
}

bool SocketServerTransportRoute::open() {
    WiFiClient new_client;

    do {
        m_client = new_client;
        new_client = m_server.available();
        if (new_client) iac_log(network, "loop available\n");
    } while (new_client);

    if (m_client) {
        m_client.setNoDelay(true);
        return true;
    }

    return false;
}

bool SocketServerTransportRoute::close() {
    m_client.stop();
    return true;
}

}  // namespace iac

#endif
