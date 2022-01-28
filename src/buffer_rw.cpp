#include "buffer_rw.hpp"

#include <cstdint>

namespace iac {

const char* BufferReader::str() {
    uint8_t* end_of_string = (uint8_t*)memchr(m_cursor, '\0', m_buffer_len - (m_cursor - m_buffer));
    if (end_of_string == nullptr) {
        IAC_HANDLE_EXCEPTION(BufferReaderOutOfBounds, "reader out of bounds reading str");
        return nullptr;
    }

    const char* result = (const char*)m_cursor;
    m_cursor = end_of_string + 1;

    return result;
}

const size_t BufferWriter::min_grow_size;

BufferWriter::~BufferWriter() {
    delete[] m_buffer;
}

size_t BufferWriter::buffer_size_on_grow() {
    switch (m_management_type) {
        case buffer_management::MGMT_INTERNAL:
            return m_allocated_buffer_size + std::max(m_allocated_buffer_size, min_grow_size);
            break;
        case buffer_management::MGMT_INTERNAL_CONSERVATIVE:
            return m_allocated_buffer_size + min_grow_size;
            break;
        default:
            IAC_HANDLE_FATAL_EXCEPTION(BufferWriterGrowException, "cant grow buffer with non 'internal' management type");
            break;
    }
    return 0;
}

bool BufferWriter::grow_buffer_to(size_t min_buffer_size) {
    while (m_allocated_buffer_size < min_buffer_size) {
        size_t new_buffer_size = buffer_size_on_grow();
        auto cursor_offset = m_cursor - m_buffer;

        uint8_t* new_buffer;

        new_buffer = new uint8_t[new_buffer_size];

        memcpy(new_buffer, m_buffer, m_cursor - m_buffer);
        delete[] m_buffer;

        m_buffer = new_buffer;
        m_allocated_buffer_size = new_buffer_size;
        m_cursor = m_buffer + cursor_offset;

        iac_log(verbose, "addresses from %p to %p are now owned by %p\n", m_buffer, m_buffer + m_allocated_buffer_size, this);
    }
    return true;
}

void BufferWriter::ensure_space(size_t min_free_space) {
    size_t free_space = m_allocated_buffer_size - (m_cursor - m_buffer);

    if (free_space >= min_free_space) {
        return;
    } else if (!grow_buffer_to(m_allocated_buffer_size + (min_free_space - free_space))) {
        IAC_HANDLE_FATAL_EXCEPTION(BufferWriterGrowException, "failed to grow buffer to target size [bad_alloc]");
    }
}

BufferWriter& BufferWriter::str(const char* str) {
    size_t string_length = strlen(str) + 1;  // copy null terminator as well
    ensure_space(string_length);
    strcpy((char*)m_cursor, str);
    m_cursor += string_length;

    return *this;
}

}  // namespace iac