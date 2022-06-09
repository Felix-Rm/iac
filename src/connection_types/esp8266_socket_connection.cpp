
#ifdef ARDUINO

#    include "esp8266_socket_connection.hpp"

#    include "../logging.hpp"

namespace iac {

SocketConnection::SocketConnection(int port)
    : m_port(port) {
}

size_t SocketConnection::read(void* buffer, size_t size) {
    if (!m_client.connected()) return 0;

    size_t queue_read_size = read_put_back_queue(buffer, size);
    size_t socket_read_size = m_client.read((uint8_t*)buffer, size);

    // printf("write: [%d] %lu\n", m_rw_fd, size);

    return queue_read_size + socket_read_size;
}

size_t SocketConnection::write(const void* buffer, size_t size) {
    if (!m_client.connected()) return 0;
    // printf("write: [%d] %lu\n", m_rw_fd, size);

    return m_client.write((uint8_t*)buffer, size);
}

bool SocketConnection::flush() {
    if (!m_client.connected()) return false;

    m_client.flush();
    return true;
}

bool SocketConnection::clear() {
    if (!m_client.connected()) return false;

    uint8_t buffer[64];
    size_t available_size = 0;

    while ((available_size = available())) {
        read(buffer, available_size > 64 ? 64 : available_size);
    }

    return true;
}

size_t SocketConnection::available() {
    if (!m_client.connected()) return 0;

    return m_client.available() + available_put_back_queue();
}

SocketClientConnection::SocketClientConnection(const char* ip, int port)
    : SocketConnection(port), m_ip(ip) {
}

bool SocketClientConnection::open() {
    if (m_client.connect(m_ip, m_port)) {
        m_client.setNoDelay(true);
        return true;
    }
    return false;
}

bool SocketClientConnection::close() {
    clear_put_back_queue();
    m_client.stop();
    return true;
}

SocketServerConnection::SocketServerConnection(int port)
    : SocketConnection(port), m_server(m_port) {
    m_server.begin();
}

bool SocketServerConnection::open() {
    WiFiClient new_client;

    do {
        m_client = new_client;
        new_client = m_server.available();
        if (new_client) iac_log(Logging::loglevels::network, "loop available\n");
    } while (new_client);

    if (m_client) {
        m_client.setNoDelay(true);
        return true;
    }

    return false;
}

bool SocketServerConnection::close() {
    m_client.stop();
    return true;
}

}  // namespace iac

#endif
