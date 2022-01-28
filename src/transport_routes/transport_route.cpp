#include "transport_route.hpp"

namespace iac {

void LocalTransportRoute::put_back(const void* buffer, size_t size) {
    auto* cursor = (uint8_t*)buffer;

    for (size_t i = 0; i < size; i++) {
        m_put_back_queue.push(*cursor++);
    }
}

size_t LocalTransportRoute::read_put_back_queue(void*& buffer, size_t& size) {
    auto& cursor = (uint8_t*&)buffer;
    size_t read_size = 0;

    while (!m_put_back_queue.empty() && size > 0) {
        *cursor++ = m_put_back_queue.front();
        m_put_back_queue.pop();
        size--;
        read_size++;
    }

    return read_size;
}

size_t LocalTransportRoute::available_put_back_queue() {
    return m_put_back_queue.size();
}

void LocalTransportRoute::clear_put_back_queue() {
    queue<uint8_t> empty;
    m_put_back_queue = empty;
}

}  // namespace iac