#include "local_transport_route.hpp"

namespace iac {

LocalTransportRoute::LocalTransportRoute(Connection& connection)
    : m_connection(&connection) {
    set_local(true);
};

}  // namespace iac