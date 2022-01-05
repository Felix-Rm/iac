#pragma once

#include <cstring>
#include <iomanip>
#include <iostream>

#include "forward.hpp"
#include "std_provider/limits.hpp"
#include "std_provider/printf.hpp"
#include "std_provider/string.hpp"

#ifndef IAC_DISABLE_EXCEPTIONS
#include <exception>
#endif

#include "transport_routes/transport_route.hpp"

namespace iac {

#ifndef IAC_DISABLE_EXCEPTIONS
class InvalidPackageException : public std::exception {
   public:
    InvalidPackageException(const char* reason) : m_reason(reason){};

    const char* what() const noexcept override {
        return m_reason;
    }

   private:
    const char* m_reason;
};
#endif

class Package {
   public:
    friend LocalNode;

    enum buffer_management {
        IN_PLACE,
        COPY
    };

    typedef buffer_management buffer_management_t;

    Package() = default;
    Package(ep_id_t from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, buffer_management_t buffer_type = buffer_management::IN_PLACE);
    ~Package();

    Package(const Package&);
    Package(Package&&);

    ep_id_t from() const { return m_from; };
    ep_id_t to() const { return m_to; };
    package_type_t type() const { return m_type; };
    const uint8_t* payload() const { return m_payload; };
    package_size_t payload_size() const { return m_payload_size; };

    ep_id_t& from() { return m_from; };
    ep_id_t& to() { return m_to; };
    package_type_t& type() { return m_type; };
    uint8_t*& payload() { return m_payload; };
    package_size_t& payload_size() { return m_payload_size; };

    LocalTransportRoute* route() const { return m_over_route; };

    void print() const;

   private:
    bool send_over(LocalTransportRoute*) const;
    bool read_from(LocalTransportRoute*);

    static constexpr size_t m_pre_header_size = sizeof(start_byte_t) + sizeof(package_size_t);
    static constexpr size_t m_info_header_size = sizeof(ep_id_t) * 2 + sizeof(metadata_t) + sizeof(package_type_t);
    static constexpr size_t m_max_payload_size = numeric_limits<package_size_t>::max() - m_info_header_size;
    static constexpr start_byte_t m_startbyte = 0b10101010;

    ep_id_t m_from;
    ep_id_t m_to;

    package_type_t m_type;

    metadata_t m_metadata;

    uint8_t* m_payload = nullptr;
    package_size_t m_payload_size;

    buffer_management_t m_buffer_type = buffer_management::IN_PLACE;

    LocalTransportRoute* m_over_route = nullptr;
};
}  // namespace iac