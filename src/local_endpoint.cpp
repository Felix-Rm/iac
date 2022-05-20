#include "local_endpoint.hpp"

namespace iac {

LocalEndpoint::~LocalEndpoint() {
    for (auto& entry : m_handlers) {
        switch (entry.second.type) {
            case package_handler_type::BY_BUFFER:
                delete entry.second.ptr.by_buffer;
                break;
            case package_handler_type::BY_READER:
                delete entry.second.ptr.by_reader;
                break;
            case package_handler_type::BY_BUFFER_WITH_DATA:
                delete entry.second.ptr.by_buffer_with_data;
                break;
            case package_handler_type::BY_READER_WITH_DATA:
                delete entry.second.ptr.by_reader_with_data;
                break;

#ifndef IAC_USE_LWSTD
            case package_handler_type::BY_FN_BUFFER:
                delete entry.second.ptr.by_fn_buffer;
                break;
            case package_handler_type::BY_FN_READER:
                delete entry.second.ptr.by_fn_reader;
                break;
#endif
            default:
                IAC_ASSERT_NOT_REACHED();
        }
    }
}

void LocalEndpoint::add_package_handler(package_type_t for_type, package_handler_by_reader_t handler) {
    m_handlers[for_type].type = package_handler_type::BY_READER;
    m_handlers[for_type].ptr.by_reader = new package_handler_by_reader_t{handler};
}

void LocalEndpoint::add_package_handler(package_type_t for_type, package_handler_by_buffer_t handler) {
    m_handlers[for_type].type = package_handler_type::BY_BUFFER;
    m_handlers[for_type].ptr.by_buffer = new package_handler_by_buffer_t{handler};
}

void LocalEndpoint::add_package_handler(package_type_t for_type, package_handler_by_reader_with_data_t handler, void* data) {
    m_handlers[for_type].type = package_handler_type::BY_READER_WITH_DATA;
    m_handlers[for_type].data = data;
    m_handlers[for_type].ptr.by_reader_with_data = new package_handler_by_reader_with_data_t{handler};
}

void LocalEndpoint::add_package_handler(package_type_t for_type, package_handler_by_buffer_with_data_t handler, void* data) {
    m_handlers[for_type].type = package_handler_type::BY_BUFFER_WITH_DATA;
    m_handlers[for_type].data = data;
    m_handlers[for_type].ptr.by_buffer_with_data = new package_handler_by_buffer_with_data_t{handler};
}

#ifndef IAC_USE_LWSTD
void LocalEndpoint::add_package_handler(package_type_t for_type, package_handler_fn_by_buffer_t handler) {
    m_handlers[for_type].type = package_handler_type::BY_FN_BUFFER;
    m_handlers[for_type].ptr.by_fn_buffer = new package_handler_fn_by_buffer_t{handler};
}

void LocalEndpoint::add_package_handler(package_type_t for_type, package_handler_fn_by_reader_t handler) {
    m_handlers[for_type].type = package_handler_type::BY_FN_READER;
    m_handlers[for_type].ptr.by_fn_reader = new package_handler_fn_by_reader_t{handler};
}
#endif

bool LocalEndpoint::remove_package_handler(package_type_t for_type) {
    delete m_handlers[for_type].ptr.by_buffer;
    return m_handlers.erase(for_type) != 0U;
}

bool LocalEndpoint::handle_package(const Package& package) const {
    auto entry = m_handlers.find(package.type());
    if (entry == m_handlers.end())
        return false;

    switch (entry->second.type) {
        case package_handler_type::BY_BUFFER:
            (*entry->second.ptr.by_buffer)(package);
            break;
        case package_handler_type::BY_READER:
            (*entry->second.ptr.by_reader)(package, BufferReader(package.payload(), package.payload_size()));
            break;
        case package_handler_type::BY_BUFFER_WITH_DATA:
            (*entry->second.ptr.by_buffer_with_data)(package, entry->second.data);
            break;
        case package_handler_type::BY_READER_WITH_DATA:
            (*entry->second.ptr.by_reader_with_data)(package, BufferReader(package.payload(), package.payload_size()), entry->second.data);
            break;

#ifndef IAC_USE_LWSTD
        case package_handler_type::BY_FN_BUFFER:
            (*entry->second.ptr.by_fn_buffer)(package);
            break;
        case package_handler_type::BY_FN_READER:
            (*entry->second.ptr.by_fn_reader)(package, BufferReader(package.payload(), package.payload_size()));
            break;
#endif
        default:
            IAC_HANDLE_FATAL_EXCEPTION(EndpointException, "package handler had invalid type");
            return false;
    }

    return true;
}

}  // namespace iac