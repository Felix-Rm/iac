#include "package.hpp"

namespace iac {

constexpr size_t Package::s_pre_header_size;
constexpr size_t Package::s_info_header_size;
constexpr size_t Package::s_max_payload_size;
constexpr start_byte_t Package::s_startbyte;

Package::Package(ep_id_t from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, buffer_management_t buffer_type)
    : m_from(from), m_to(to), m_type(type), m_payload((uint8_t*)buffer), m_buffer_type(buffer_type) {
    if (buffer_length > s_max_payload_size) {
        IAC_HANDLE_EXCEPTION(InvalidPackageException, "payload to big");

    } else {
        m_payload_size = buffer_length;
    }

    if (m_buffer_type == buffer_management::COPY) {
        m_payload = new uint8_t[m_payload_size];
        memcpy(m_payload, (uint8_t*)buffer, m_payload_size);
    }
}

void Package::copy_from(const Package& other) {
    m_from = other.m_from;
    m_to = other.m_to;
    m_type = other.m_type;
    m_payload_size = other.m_payload_size;
    m_buffer_type = buffer_management::COPY;
    memmove(m_payload, other.m_payload, m_payload_size);
}

void Package::move_from(Package& other) {
    m_from = other.m_from;
    m_to = other.m_to;
    m_type = other.m_type;
    m_payload_size = other.m_payload_size;
    m_buffer_type = other.m_buffer_type;
    m_payload = other.m_payload;

    other.m_payload = nullptr;
    other.m_payload_size = 0;
    other.m_buffer_type = buffer_management::EMPTY;
}

Package::~Package() {
    if (m_buffer_type == buffer_management::COPY) {
        delete[] m_payload;
    }
}

bool Package::send_over(LocalTransportRoute* route) const {
    package_size_t package_size = s_info_header_size + m_payload_size;

    route->connection().write(&s_startbyte, sizeof(start_byte_t));
    route->connection().write(&package_size, sizeof(package_size_t));

    route->connection().write(&m_metadata, sizeof(metadata_t));
    route->connection().write(&m_to, sizeof(ep_id_t));
    route->connection().write(&m_from, sizeof(ep_id_t));
    route->connection().write(&m_type, sizeof(package_type_t));

    if (m_payload_size > 0)
        route->connection().write(m_payload, m_payload_size);

    route->connection().flush();

    return true;
}

bool Package::read_from(LocalTransportRoute* route) {
    m_over_route = route;

#ifdef ARDUINO
    uint8_t dummy = 0;
    route->connection().write(&dummy, 1);
#endif

    if (route->meta().wait_for_available_size > 0 && route->connection().available() < route->meta().wait_for_available_size)
        return false;

    if (route->connection().available() < s_pre_header_size)
        return false;

    route->meta().wait_for_available_size = 0;

    start_byte_t start_byte = 0;

    while (route->connection().available() >= s_pre_header_size) {
        if (route->connection().read(&start_byte, sizeof(start_byte_t)) != sizeof(start_byte_t)) {
            IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading start_byte returned less bytes than 'available'");

            return false;
        }

        if (start_byte == s_startbyte)
            break;

        // NOTE: zero-bytes can be used as dummy writes by transport routes, so no warning to avoid spamming
        if (start_byte != 0)
            iac_log(Logging::loglevels::warning, "corrupt message start\n");
    }

    if (start_byte != s_startbyte) return false;

    package_size_t package_size = 0;
    if (route->connection().read(&package_size, sizeof(package_size_t)) != sizeof(package_size_t)) {
        IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading package_len returned less bytes than 'available'");

        return false;
    }

    if (route->connection().available() < package_size) {
        route->connection().put_back(&start_byte, sizeof(start_byte_t));
        route->connection().put_back(&package_size, sizeof(package_size_t));
        route->meta().wait_for_available_size = package_size + s_pre_header_size;
        return false;
    }

    if (route->connection().read(&m_metadata, sizeof(metadata_t)) != sizeof(metadata_t)) {
        IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading meta returned less bytes than 'available'");

        return false;
    }

    if (route->connection().read(&m_to, sizeof(ep_id_t)) != sizeof(ep_id_t)) {
        IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading to_addr returned less bytes than 'available'");

        return false;
    }

    if (route->connection().read(&m_from, sizeof(ep_id_t)) != sizeof(ep_id_t)) {
        IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading from_addr returned less bytes than 'available'");

        return false;
    }

    if (route->connection().read(&m_type, sizeof(package_type_t)) != sizeof(package_type_t)) {
        IAC_HANDLE_FATAL_EXCEPTION(InvalidPackageException, "reading type returned less bytes than 'available'");

        return false;
    }

    m_payload_size = package_size - s_info_header_size;
    m_payload = new uint8_t[m_payload_size];
    m_buffer_type = buffer_management::COPY;  // we need to delete this on deconstruction

    if (m_payload_size > 0)
        if (route->connection().read(m_payload, m_payload_size) != m_payload_size) {
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
    iac_printf("package @%p:\n", this);
    iac_printf("\tmeta: 0x%02x\n", m_metadata);
    iac_printf("\tto: %03u\n", m_to);
    iac_printf("\tfrom: %03u\n", m_from);
    iac_printf("\ttype: 0x%03u\n", m_type);

    static constexpr unsigned bytes_per_line = 6;
    static constexpr unsigned max_lines = 20;

    iac_printf("\tpayload:");
    for (unsigned i = 0; i < m_payload_size && i < bytes_per_line * max_lines; i++)
        iac_printf((i % bytes_per_line == bytes_per_line - 1 && i + 1 < m_payload_size) ? " @0x%02x:0x%02x[%c]\n\t        " : " @0x%02x:0x%02x[%c]", i, m_payload[i], (m_payload[i] >= ' ' && m_payload[i] <= '\x7f') ? m_payload[i] : '.');

    iac_printf("\n\tpayload_size: %d\n\n", m_payload_size);
}

}  // namespace iac