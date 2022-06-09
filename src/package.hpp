#pragma once

#include "forward.hpp"
#include "local_transport_route.hpp"
#include "network_types.hpp"
#include "std_provider/limits.hpp"
#include "std_provider/printf.hpp"
#include "std_provider/string.hpp"

namespace iac {

IAC_MAKE_EXCEPTION(InvalidPackageException);

class Package {
    friend LocalNode;

   public:
    enum buffer_management {
        IN_PLACE,
        COPY,
        EMPTY
    };

    typedef buffer_management buffer_management_t;

    Package() = default;
    Package(ep_id_t from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, buffer_management_t buffer_type = buffer_management::IN_PLACE);
    ~Package();

    Package(const Package& other) {
        copy_from(other);
    }

    Package(Package&& other) {
        move_from(other);
    }

    Package& operator=(const Package& other) {
        if (&other != this) copy_from(other);
        return *this;
    }

    Package& operator=(Package&& other) {
        if (&other != this) move_from(other);
        return *this;
    }

    ep_id_t from() const {
        return m_from;
    };

    ep_id_t to() const {
        return m_to;
    };

    package_type_t type() const {
        return m_type;
    };

    const uint8_t* payload() const {
        return m_payload;
    };

    package_size_t payload_size() const {
        return m_payload_size;
    };

    ep_id_t& from() {
        return m_from;
    };

    ep_id_t& to() {
        return m_to;
    };

    package_type_t& type() {
        return m_type;
    };

    uint8_t*& payload() {
        return m_payload;
    };

    package_size_t& payload_size() {
        return m_payload_size;
    };

    LocalTransportRoute* route() const {
        return m_over_route;
    };

    void print() const;

   protected:
    bool send_over(LocalTransportRoute* route) const;
    bool read_from(LocalTransportRoute* route);

    void copy_from(const Package& other);
    void move_from(Package& other);

   private:
    static constexpr size_t s_pre_header_size = sizeof(start_byte_t) + sizeof(package_size_t);
    static constexpr size_t s_info_header_size = sizeof(ep_id_t) * 2 + sizeof(metadata_t) + sizeof(package_type_t);
    static constexpr size_t s_max_payload_size = numeric_limits<package_size_t>::max() - s_info_header_size;
    static constexpr start_byte_t s_startbyte = 0b10101010;

    ep_id_t m_from{unset_id}, m_to{unset_id};

    package_type_t m_type{0};

    metadata_t m_metadata = {0};

    uint8_t* m_payload{nullptr};
    package_size_t m_payload_size{0};

    buffer_management_t m_buffer_type{buffer_management::EMPTY};

    LocalTransportRoute* m_over_route{nullptr};
};

}  // namespace iac
