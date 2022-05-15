#include "local_node.hpp"

namespace iac {

bool LocalNode::send(Endpoint& from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, Package::buffer_management_t buffer_management) {
    return send(from.id(), to, type, buffer, buffer_length, buffer_management);
}

bool LocalNode::send(ep_id_t from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, Package::buffer_management_t buffer_management) {
    Package package{from, to, type, buffer, buffer_length, buffer_management};
    return handle_package(package);
}

bool LocalNode::send(ep_id_t from, ep_id_t to, package_type_t type, const BufferWriter& buffer, Package::buffer_management_t buffer_management) {
    return send(from, to, type, buffer.buffer(), buffer.size(), buffer_management);
}

bool LocalNode::send(Endpoint& from, ep_id_t to, package_type_t type, const BufferWriter& buffer, Package::buffer_management_t buffer_management) {
    return send(from, to, type, buffer.buffer(), buffer.size(), buffer_management);
}

bool LocalNode::endpoint_connected(ep_id_t address) const {
    return m_network.endpoint_registered(address);
}

bool LocalNode::endpoints_connected(const vector<ep_id_t>& addresses) const {
    for (const auto& address : addresses)
        if (!endpoint_connected(address)) {
            iac_log(Logging::loglevels::verbose, "node %d connected test failed for %d\n", id(), address);
            return false;
        }
    return true;
}

bool LocalNode::all_routes_connected() const {
    for (const auto& route : m_network.route_mapping()) {
        if (!route.second.element().local()) continue;
        const auto& ltr = (const LocalTransportRoute&) route.second.element();
        if (ltr.state() != LocalTransportRoute::route_state::CONNECTED) return false;
    }

    return true;
}

}