
#include "loopback_transport_route.hpp"

namespace iac {

LoopbackTransportRoute::LoopbackTransportRoute(queue_t* write_queue, queue_t* read_queue) : m_write_queue(write_queue), m_read_queue(read_queue) {}

size_t LoopbackTransportRoute::read(void* buffer, size_t size) {
    size_t read_size = read_put_back_queue(buffer, size);

    for (size_t i = 0; i < size && !m_read_queue->empty(); i++) {
        ((uint8_t*)buffer)[i] = m_read_queue->front();
        m_read_queue->pop();
        read_size++;
    }

    return read_size;
}

size_t LoopbackTransportRoute::write(const void* buffer, size_t size) {
    for (size_t i = 0; i < size; i++)
        m_write_queue->push(((uint8_t*)buffer)[i]);

    return size;
}

bool LoopbackTransportRoute::flush() {
    return true;
}

bool LoopbackTransportRoute::clear() {
    *m_read_queue = queue_t{};
    return true;
}

size_t LoopbackTransportRoute::available() {
    return m_read_queue->size() + available_put_back_queue();
}

bool LoopbackTransportRoute::open() {
    return true;
}

bool LoopbackTransportRoute::close() {
    return true;
}

void LoopbackTransportRoute::printBuffer(int cols) {
    auto buffer_cp = *m_write_queue;
    while (!buffer_cp.empty()) {
        for (int i = 0; i < cols && !buffer_cp.empty(); i++) {
            iac_log(Logging::loglevels::debug, "%02x [%c]\t", buffer_cp.front(), buffer_cp.front() >= ' ' && buffer_cp.front() <= 127 ? buffer_cp.front() : '.');
            buffer_cp.pop();
        }
        iac_log(Logging::loglevels::debug, "\n");
    }
}

}  // namespace iac