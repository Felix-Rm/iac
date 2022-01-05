
#ifndef ARDUINO

#include "socket_transport_route.hpp"

namespace iac {

SocketTransportRoute::SocketTransportRoute(const char* ip, int port) : m_ip(ip), m_port(port) {
    signal(SIGPIPE, SIG_IGN);

    if (ip)
        inet_pton(AF_INET, ip, &m_addr);
}

size_t SocketTransportRoute::read(void* buffer, size_t size) {
    if (m_rw_fd == -1) return 0;

    size_t queue_read_size = read_put_back_queue(buffer, size);
    size_t socket_read_size = ::read(m_rw_fd, buffer, size);

    // printf("write: [%d] %lu\n", m_rw_fd, size);

    return queue_read_size + socket_read_size;
}

size_t SocketTransportRoute::write(const void* buffer, size_t size) {
    if (m_rw_fd == -1) return 0;

    // printf("write: [%d] %lu\n", m_rw_fd, size);

    return ::write(m_rw_fd, buffer, size);
}

bool SocketTransportRoute::flush() {
    if (m_rw_fd == -1) return false;

    return fsync(m_rw_fd) == 0;
}

bool SocketTransportRoute::clear() {
    if (m_rw_fd == -1) return false;

    uint8_t buffer[64];
    size_t available_size = 0;

    while ((available_size = available())) {
        read(buffer, available_size > 64 ? 64 : available_size);
    }

    return true;
}

size_t SocketTransportRoute::available() {
    if (m_rw_fd == -1) return 0;

    int count;
    ioctl(m_rw_fd, FIONREAD, &count);

    // printf("available: [%d] %d\n", m_rw_fd, count);

    if (count < 0)
        throw TransportRouteException("checking available read size on socket returned negative number");

    return count + available_put_back_queue();
}

SocketClientTransportRoute::SocketClientTransportRoute(const char* ip, int port) : SocketTransportRoute(ip, port) {
    m_address.sin_family = AF_INET;
    m_address.sin_addr.s_addr = m_addr;
    m_address.sin_port = htons(m_port);
}

bool SocketClientTransportRoute::open() {
    if ((m_rw_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        return false;

    long opts;
    if ((opts = fcntl(m_rw_fd, F_GETFL, NULL)) < 0)
        return false;

    opts &= (~O_NONBLOCK);
    if (fcntl(m_rw_fd, F_SETFL, opts) < 0)
        return false;

    int result, valopt;
    socklen_t len = sizeof(int);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    fd_set fds;

    if (connect(m_rw_fd, (sockaddr*)&m_address, sizeof(m_address)) < 0) {
        if (errno == EINPROGRESS) {
            while (true) {
                FD_ZERO(&fds);
                FD_SET(m_rw_fd, &fds);
                result = select(m_rw_fd + 1, NULL, &fds, NULL, &tv);

                if (result < 0 && errno == EINTR) {
                    iac_log(network, "Error connecting %d - %s\n", errno, strerror(errno));

                    return false;

                } else if (result > 0) {
                    if (getsockopt(m_rw_fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &len) < 0) {
                        iac_log(network, "Error in getsockopt() %d - %s\n", errno, strerror(errno));

                        return false;
                    }

                    if (valopt) {
                        iac_log(network, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt));

                        return false;
                    }
                    break;

                } else {
                    iac_log(network, "Timeout in select() - Cancelling!\n");

                    return false;
                }
            }
        } else {
            iac_log(network, "Error connecting %d - %s\n", errno, strerror(errno));

            return false;
        }
    }

    if ((opts = fcntl(m_rw_fd, F_GETFL, NULL)) < 0)
        return false;

    opts |= O_NONBLOCK;
    if (fcntl(m_rw_fd, F_SETFL, opts) < 0)
        return false;

    return true;
}

bool SocketClientTransportRoute::close() {
    bool close_result = ::close(m_rw_fd);
    m_rw_fd = -1;
    clear_put_back_queue();
    return close_result;
}

SocketServerTransportRoute::SocketServerTransportRoute(const char* ip, int port) : SocketTransportRoute(ip, port) {
    if ((m_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        iac_log(network, "socket failed\n");

        m_good = false;
        return;
    }

    fcntl(m_server_fd, F_SETFL, fcntl(m_server_fd, F_GETFL, 0) | O_NONBLOCK);

    int opt = 1;
    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        iac_log(network, "setsockopt\n");

        m_good = false;
        return;
    }

    m_server_address.sin_family = AF_INET;
    m_server_address.sin_addr.s_addr = m_addr;
    m_server_address.sin_port = htons(m_port);

    if (bind(m_server_fd, (struct sockaddr*)&m_server_address, sizeof(m_server_address)) < 0) {
        iac_log(network, "bind to %s %d failed\n", m_ip, m_port);

        m_good = false;
        return;
    }

    if (listen(m_server_fd, 0) < 0) {
        iac_log(network, "listen\n");

        m_good = false;
        return;
    }
}

bool SocketServerTransportRoute::open() {
    socklen_t addr_len = sizeof(m_client_address);

    return (m_rw_fd = accept(m_server_fd, (struct sockaddr*)&m_client_address, &addr_len)) >= 0;
}

bool SocketServerTransportRoute::close() {
    bool close_result = ::close(m_rw_fd) == 0;
    m_rw_fd = -1;
    clear_put_back_queue();
    return close_result;
}
}  // namespace iac
#endif