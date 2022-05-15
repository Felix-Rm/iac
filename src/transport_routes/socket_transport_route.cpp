#ifndef ARDUINO

#    include "socket_transport_route.hpp"

namespace iac {

SocketTransportRoute::SocketTransportRoute(const char* ip, int port)
    : m_ip(ip), m_port(port) {
    signal(SIGPIPE, SIG_IGN);

    if (ip != nullptr)
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

    static constexpr size_t clear_buffer_size = 128;
    uint8_t buffer[clear_buffer_size];
    size_t available_size = 0;

    while ((available_size = available()) > 0) {
        read(buffer, available_size > clear_buffer_size ? clear_buffer_size : available_size);
    }

    return true;
}

size_t SocketTransportRoute::available() {
    if (m_rw_fd == -1) return 0;

    unsigned long count = 0;
    ioctl(m_rw_fd, FIONREAD, &count);

    // printf("available: [%d] %d\n", m_rw_fd, count);

    return count + available_put_back_queue();
}

SocketClientTransportRoute::SocketClientTransportRoute(const char* ip, int port)
    : SocketTransportRoute(ip, port) {
    m_address.sin_family = AF_INET;
    m_address.sin_addr.s_addr = m_addr;
    m_address.sin_port = htons(m_port);
}

bool SocketClientTransportRoute::open() {
    if ((m_rw_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        return false;

    int opts = fcntl(m_rw_fd, F_GETFL, NULL);
    if (opts < 0) return false;

    opts &= (~O_NONBLOCK);
    if (fcntl(m_rw_fd, F_SETFL, opts) < 0)
        return false;

    socklen_t len = sizeof(int);

    struct timeval tv {};
    tv.tv_sec = 0;
    tv.tv_usec = s_connect_timeout;

    fd_set fds;

    if (setsockopt(m_rw_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
        iac_log(Logging::loglevels::network, "Failed setting receive timout\n");

        return false;
    }

    if (setsockopt(m_rw_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
        iac_log(Logging::loglevels::network, "Failed setting send timout\n");

        return false;
    }

    if (connect(m_rw_fd, (sockaddr*)&m_address, sizeof(m_address)) < 0) {
        if (errno == EINPROGRESS) {
            FD_ZERO(&fds);
            FD_SET(m_rw_fd, &fds);
            int result = select(m_rw_fd + 1, nullptr, &fds, nullptr, &tv);

            if (result < 0 && errno == EINTR) {
                iac_log(Logging::loglevels::network, "Error connecting %d - %s\n", errno, strerror(errno));

                return false;
            }
            if (result > 0) {
                int valopt = -1;
                if (getsockopt(m_rw_fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &len) < 0) {
                    iac_log(Logging::loglevels::network, "Error in getsockopt() %d - %s\n", errno, strerror(errno));

                    return false;
                }

                if (valopt != 0) {
                    iac_log(Logging::loglevels::network, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt));

                    return false;
                }

            } else {
                iac_log(Logging::loglevels::network, "Timeout in select() - Cancelling!\n");

                return false;
            }

        } else {
            iac_log(Logging::loglevels::network, "Error connecting %d - %s\n", errno, strerror(errno));

            return false;
        }
    }

    if ((opts = fcntl(m_rw_fd, F_GETFL, NULL)) < 0)
        return false;

    opts |= O_NONBLOCK;
    if (fcntl(m_rw_fd, F_SETFL, opts) < 0)
        return false;

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (setsockopt(m_rw_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
        iac_log(Logging::loglevels::network, "Failed setting receive timout\n");

        return false;
    }

    if (setsockopt(m_rw_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
        iac_log(Logging::loglevels::network, "Failed setting send timout\n");

        return false;
    }

    return true;
}

bool SocketClientTransportRoute::close() {
    bool close_result = ::close(m_rw_fd) != 0;
    m_rw_fd = -1;
    clear_put_back_queue();
    return close_result;
}

SocketServerTransportRoute::SocketServerTransportRoute(const char* ip, int port)
    : SocketTransportRoute(ip, port) {
    if ((m_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        iac_log(Logging::loglevels::network, "socket failed\n");

        m_good = false;
        return;
    }

    fcntl(m_server_fd, F_SETFL, fcntl(m_server_fd, F_GETFL, 0) | O_NONBLOCK);

    int opt = 1;
    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        iac_log(Logging::loglevels::network, "setsockopt\n");

        m_good = false;
        return;
    }

    m_server_address.sin_family = AF_INET;
    m_server_address.sin_addr.s_addr = m_addr;
    m_server_address.sin_port = htons(m_port);

    if (bind(m_server_fd, (struct sockaddr*)&m_server_address, sizeof(m_server_address)) < 0) {
        iac_log(Logging::loglevels::network, "bind to %s %d failed\n", m_ip, m_port);

        m_good = false;
        return;
    }

    if (listen(m_server_fd, 0) < 0) {
        iac_log(Logging::loglevels::network, "listen\n");

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