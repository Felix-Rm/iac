#pragma once

#include "buffer_rw.hpp"
#include "forward.hpp"
#include "network_types.hpp"
#include "package.hpp"
#include "std_provider/string.hpp"
#include "std_provider/unordered_map.hpp"
#include "std_provider/utility.hpp"

#ifndef IAC_USE_LWSTD
#    include <functional>
#endif

namespace iac {

IAC_MAKE_EXCEPTION(EndpointException);

class LocalEndpoint : public Endpoint {
    friend LocalNode;

   public:
    enum class package_handler_type {
        BY_READER,
        BY_BUFFER,
        BY_READER_WITH_DATA,
        BY_BUFFER_WITH_DATA,

#ifndef IAC_USE_LWSTD
        BY_FN_READER,
        BY_FN_BUFFER
#endif
    };

    typedef package_handler_type package_handler_type_t;

    typedef void (*package_handler_by_buffer_t)(const Package& pkg);
    typedef void (*package_handler_by_reader_t)(const Package& pkg, BufferReader&& reader);
    typedef void (*package_handler_by_buffer_with_data_t)(const Package& pkg, void* data);
    typedef void (*package_handler_by_reader_with_data_t)(const Package& pkg, BufferReader&& reader, void* data);

#ifndef IAC_USE_LWSTD
    typedef std::function<void(const Package& pkg)> package_handler_fn_by_buffer_t;
    typedef std::function<void(const Package& pkg, BufferReader&& reader)> package_handler_fn_by_reader_t;
#endif

    typedef struct package_handler_t {
        package_handler_type_t type;
        void* data{nullptr};
        union {
            package_handler_by_reader_t* by_reader = nullptr;
            package_handler_by_buffer_t* by_buffer;
            package_handler_by_reader_with_data_t* by_reader_with_data;
            package_handler_by_buffer_with_data_t* by_buffer_with_data;

#ifndef IAC_USE_LWSTD
            package_handler_fn_by_reader_t* by_fn_reader;
            package_handler_fn_by_buffer_t* by_fn_buffer;
#endif
        } ptr;
    } package_handler_t;

    LocalEndpoint(ep_id_t id, string&& name)
        : Endpoint(id, name) {
        set_local(true);
    };

    ~LocalEndpoint();

    LocalEndpoint(LocalEndpoint&) = delete;
    LocalEndpoint(LocalEndpoint&&) = delete;
    LocalEndpoint& operator=(LocalEndpoint&) = delete;
    LocalEndpoint& operator=(LocalEndpoint&&) = delete;

    void add_package_handler(package_type_t for_type, package_handler_by_reader_t handler);
    void add_package_handler(package_type_t for_type, package_handler_by_buffer_t handler);
    void add_package_handler(package_type_t for_type, package_handler_by_reader_with_data_t handler, void* data);
    void add_package_handler(package_type_t for_type, package_handler_by_buffer_with_data_t handler, void* data);

#ifndef IAC_USE_LWSTD
    void add_package_handler(package_type_t for_type, package_handler_fn_by_reader_t handler);
    void add_package_handler(package_type_t for_type, package_handler_fn_by_buffer_t handler);
#endif

    bool remove_package_handler(package_type_t for_type);

   private:
    bool handle_package(const Package& package) const;

    unordered_map<package_type_t, package_handler_t> m_handlers;
};
}  // namespace iac