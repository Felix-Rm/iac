#pragma once

#include <cstddef>
#include <cstdint>

#include "exceptions.hpp"
#include "logging.hpp"
#include "std_provider/limits.hpp"
#include "std_provider/printf.hpp"
#include "std_provider/string.hpp"
#include "std_provider/utility.hpp"

namespace iac {

IAC_MAKE_EXCEPTION(BufferWriterGrowException);
IAC_MAKE_EXCEPTION(BufferReaderOutOfBounds);

class BufferReader {
   public:
    BufferReader(const uint8_t* buffer, size_t buffer_length) : m_buffer(buffer), m_buffer_len(buffer_length), m_cursor((uint8_t*)buffer){};
    ~BufferReader() = default;

    const uint8_t* buffer() const {
        return m_buffer;
    }

    size_t size() const {
        return m_buffer_len;
    }

    explicit operator bool() const {
        return (size_t)(m_cursor - m_buffer) < m_buffer_len;
    }

    const char* str();

    template <typename T>
    T num() {
        if (!can_read(sizeof(T))) {
            IAC_HANDLE_EXCPETION(BufferReaderOutOfBounds, "reader out of bounds reading num");
            return 0;
        }
        T result;
        memcpy(&result, m_cursor, sizeof(T));
        m_cursor += sizeof(T);
        return result;
    }

    bool boolean() {
        if (!can_read(sizeof(uint8_t))) {
            IAC_HANDLE_EXCPETION(BufferReaderOutOfBounds, "reader out of bounds reading boolean");
            return false;
        }
        return num<uint8_t>();
    };

    bool can_read(size_t size) const {
        return (size_t)(m_cursor - m_buffer + size) <= m_buffer_len;
    }

   private:
    const uint8_t* m_buffer;
    size_t m_buffer_len;
    uint8_t* m_cursor;
};

class BufferWriter {
   public:
    typedef enum class buffer_management {
        MGMT_EXTERNAL,
        MGMT_INTERNAL,
        MGMT_INTERNAL_CONSERVATIVE
    } buffer_management_t;

    BufferWriter(buffer_management_t management_type = buffer_management::MGMT_INTERNAL) : m_management_type(management_type) {
        ensure_space(min_grow_size);
    };

    BufferWriter(uint8_t* buffer, size_t buffer_length) : m_allocated_buffer_size(buffer_length), m_buffer(buffer), m_cursor((uint8_t*)buffer){};
    ~BufferWriter();

    BufferWriter& str(const char* str);
    BufferWriter& str(const string& string) { return str(string.c_str()); };

    template <typename T>
    BufferWriter& num(T num) {
        ensure_space(sizeof(T));
        memcpy(m_cursor, &num, sizeof(T));
        m_cursor += sizeof(T);
        return *this;
    }

    BufferWriter& boolean(bool b) { return num((uint8_t)b); };

    const uint8_t* buffer() const {
        return m_buffer;
    }

    size_t size() const {
        return m_cursor - m_buffer;
    }

   private:
    static constexpr size_t min_grow_size = 8;
    buffer_management_t m_management_type = buffer_management::MGMT_EXTERNAL;
    size_t m_allocated_buffer_size = 0;

    uint8_t* m_buffer = nullptr;
    uint8_t* m_cursor = nullptr;

    size_t buffer_size_on_grow();
    bool grow_buffer_to(size_t min_buffer_size);
    void ensure_space(size_t min_free_space);
};
}  // namespace iac