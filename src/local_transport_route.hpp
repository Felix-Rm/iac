#pragma once

#include "connection_types/connection.hpp"
#include "exceptions.hpp"
#include "forward.hpp"
#include "logging.hpp"
#include "network_types.hpp"
#include "std_provider/printf.hpp"
#include "std_provider/queue.hpp"
#include "std_provider/string.hpp"
#include "std_provider/utility.hpp"

namespace iac {

IAC_MAKE_EXCEPTION(TransportRouteWithoutConnection);

class LocalTransportRoute : public TransportRoute {
   public:
    enum class route_state {
        INITIALIZED,
        SEND_CONNECT,
        WAIT_CONNECT,
        SEND_ACK,
        WAIT_ACK,
        CONNECTED,
        CLOSED
    };

    typedef struct route_meta {
        timestamp last_package_in;
        timestamp last_package_out;

        size_t wait_for_available_size = 0;
        route_timings_t timings;
    } route_meta_t;

    typedef route_state route_state_t;

    LocalTransportRoute(Connection& connection);

    bool reset() {
        return true;
    };

    route_state_t& state() {
        return m_state;
    };

    route_state_t state() const {
        return m_state;
    };

    route_meta_t& meta() {
        return m_meta;
    };

    Connection& connection() {
        // NOTE: `m_connection` should never be able to be `nullptr`
        return *m_connection;
    };

   private:
    Connection* m_connection{nullptr};
    route_meta_t m_meta{};
    route_state_t m_state = route_state::INITIALIZED;
};

template <typename ConnectionType>
class LocalTransportRoutePackage {
   public:
    template <typename... Args>
    LocalTransportRoutePackage(Args... args)
        : m_connection(args...), m_route(m_connection) {
    }

    auto& route() {
        return m_route;
    }

    const auto& connection() const {
        return m_connection;
    }

   private:
    ConnectionType m_connection;
    LocalTransportRoute m_route;
};

}  // namespace iac
