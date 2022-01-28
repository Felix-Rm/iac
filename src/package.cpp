#include "package.hpp"

#include "network_types.hpp"

namespace iac {

Package::Package(ep_id_t from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, buffer_management_t buffer_type) : m_from(from), m_to(to), m_type(type), m_payload((uint8_t*)buffer), m_buffer_type(buffer_type) {
    if (buffer_length > m_max_payload_size) {
#ifndef IAC_DISABLE_EXCEPTIONS
        throw InvalidPackageException("payload to big");
#endif
    } else {
        m_payload_size = buffer_length;
    }

    if (m_buffer_type == buffer_management::COPY) {
        m_payload = new uint8_t[m_payload_size];
        memcpy(m_payload, (uint8_t*)buffer, m_payload_size);
    }
}

Package::Package(const Package& other) : Package(other.m_from, other.m_to, other.m_type, other.m_payload, other.m_payload_size, Package::buffer_management::COPY) {
    m_over_route = other.m_over_route;
}

Package::Package(Package&& other) : Package(other.m_from, other.m_to, other.m_type, other.m_payload, other.m_payload_size) {
    other.m_payload = nullptr;
    other.m_payload_size = 0;
    m_over_route = other.m_over_route;
}

Package::~Package() {
    if (m_buffer_type == buffer_management::COPY) {
        delete[] m_payload;
    }
}

bool Package::send_over(LocalTransportRoute* route) const {
    package_size_t package_size = m_info_header_size + m_payload_size;

    route->write(&m_startbyte, sizeof(start_byte_t));
    route->write(&package_size, sizeof(package_size_t));

    route->write(&m_metadata, sizeof(metadata_t));
    route->write(&m_to, sizeof(ep_id_t));
    route->write(&m_from, sizeof(ep_id_t));
    route->write(&m_type, sizeof(package_type_t));

    if (m_payload_size)
        route->write(m_payload, m_payload_size);

    route->flush();

    return true;
}

bool Package::read_from(LocalTransportRoute* route) {
    m_over_route = route;

#ifdef ARDUINO
    uint8_t dummy = 0;
    route->write(&dummy, 1);
#endif

    if (route->m_wait_for_available_size && route->available() < route->m_wait_for_available_size)
        return false;

    if (route->available() < m_pre_header_size)
        return false;

    route->m_wait_for_available_size = 0;

    start_byte_t start_byte = 0;
    package_size_t package_size;

    while (route->available() >= m_pre_header_size) {
        if (route->read(&start_byte, sizeof(start_byte_t)) != sizeof(start_byte_t)) {
            IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading start_byte returned less bytes than 'available'");

            return false;
        }

        if (start_byte == m_startbyte)
            break;

        // NOTE: zero-bytes can be used as dummy writes by transport routes, so no warning to avoid spamming
        if (start_byte != 0)
            iac_log(warning, "corrupt message start\n");
    }

    if (start_byte != m_startbyte) return false;

    if (route->read(&package_size, sizeof(package_size_t)) != sizeof(package_size_t)) {
        IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading package_len returned less bytes than 'available'");

        return false;
    }

    if (route->available() < package_size) {
        route->put_back(&start_byte, sizeof(start_byte_t));
        route->put_back(&package_size, sizeof(package_size_t));
        route->m_wait_for_available_size = package_size + m_pre_header_size;
        return false;
    }

    if (route->read(&m_metadata, sizeof(metadata_t)) != sizeof(metadata_t)) {
        IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading meta returned less bytes than 'available'");

        return false;
    }

    if (route->read(&m_to, sizeof(ep_id_t)) != sizeof(ep_id_t)) {
        IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading to_addr returned less bytes than 'available'");

        return false;
    }

    if (route->read(&m_from, sizeof(ep_id_t)) != sizeof(ep_id_t)) {
        IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading from_addr returned less bytes than 'available'");

        return false;
    }

    if (route->read(&m_type, sizeof(package_type_t)) != sizeof(package_type_t)) {
        IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading type returned less bytes than 'available'");

        return false;
    }

    m_payload_size = package_size - m_info_header_size;
    m_payload = new uint8_t[m_payload_size];
    m_buffer_type = buffer_management::COPY;  // we need to delete this on deconstruction

    if (m_payload_size)
        if (route->read(m_payload, m_payload_size) != m_payload_size) {
            IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading payload returned less bytes than 'available'");
            return false;
        }

    // if (m_over_route == nullptr) {
    //     printf("%p %p %p\n", m_payload, m_payload + m_payload_size, &m_over_route);
    //     int b = 0;
    // }

    return true;
}

void Package::print() const {
    IAC_PRINT_PROVIDER("package @%p:\n", this);
    IAC_PRINT_PROVIDER("\tmeta: 0x%02x\n", m_metadata);
    IAC_PRINT_PROVIDER("\tto: %03u\n", m_to);
    IAC_PRINT_PROVIDER("\tfrom: %03u\n", m_from);
    IAC_PRINT_PROVIDER("\ttype: 0x%03u\n", m_type);

    IAC_PRINT_PROVIDER("\tpayload:");
    for (size_t i = 0; i < m_payload_size && i < 6 * 20; i++)
        IAC_PRINT_PROVIDER((i % 6 == 5 && i + 1 < m_payload_size) ? " @0x%02lx:0x%02x[%c]\n\t        " : " @0x%02lx:0x%02x[%c]", i, m_payload[i], (m_payload[i] >= ' ' && m_payload[i] <= 127) ? m_payload[i] : '.');

    IAC_PRINT_PROVIDER("\n\tpayload_size: %d\n\n", m_payload_size);
}

}  // namespace iac