#include "local_node.hpp"

namespace iac {

/*

On init:        +-----------------+                 +-----------------+
                | INITIALIZED     |                 | INITIALIZED     |
                +-----------------+                 +-----------------+
                         |                                   |
                         V                                   V
                +-----------------+ --------------> +-----------------+
             +> | SEND_CONNECT    |   msg::connect  | SEND_CONNECT    | <+
             |  +-----------------+ <-------------- +-----------------+  |
       until |           |                                   |           | until
msg::connect |           V                                   V           | msg::connect
    received |  +-----------------+                 +-----------------+  | received
             +- | WAIT_CONNECT    |                 | WAIT_CONNECT    | -+
                +-----------------+                 +-----------------+
                         |                                   |
                         V                                   V
                +-----------------+ --------------> +-----------------+
                | SEND_ACK        |   msg::ack      | SEND_ACK        |
                +-----------------+ <-------------- +-----------------+
                         |                                   |
                         V                                   V
                +-----------------+                 +-----------------+
                | WAIT_ACK        |                 | WAIT_ACK        |
                +-----------------+                 +-----------------+
                         |                                   |
                         V                                   V
                +-----------------+                 +-----------------+
                | CONNECTED       |                 | CONNECTED       |
                +-----------------+                 +-----------------+

*/

bool LocalNode::state_handling(LocalTransportRoute* route) {
    iac_log_from_node(Logging::loglevels::verbose, "state of route %d @ node %d is %d\n", route->id(), id(), route->state());
    auto now = timestamp::now();

    if (route->state() != LocalTransportRoute::route_state::CLOSED &&
        route->state() != LocalTransportRoute::route_state::INITIALIZED &&
        route->meta().last_package_in.is_more_than_n_in_past(now, route->meta().timings.assume_dead_after_ms)) {
        if (!close_route(route)) return false;
        route->state() = LocalTransportRoute::route_state::CLOSED;
    }

    switch (route->state()) {
        case LocalTransportRoute::route_state::INITIALIZED:
        case LocalTransportRoute::route_state::CLOSED:
            if (!open_route(route)) return false;
            route->state() = LocalTransportRoute::route_state::SEND_CONNECT;
            // intentional fall-through

        case LocalTransportRoute::route_state::SEND_CONNECT:
            if (!send_connect(route)) return false;
            route->state() = LocalTransportRoute::route_state::WAIT_CONNECT;
            // intentional fall-through

        case LocalTransportRoute::route_state::WAIT_CONNECT:
            // NOTE: This loop will be broken when a CONNECT package from this route arrives
            if (route->meta().last_package_out.is_more_than_n_in_past(now, route->meta().timings.heartbeat_interval_ms))
                route->state() = LocalTransportRoute::route_state::SEND_CONNECT;
            break;

        case LocalTransportRoute::route_state::SEND_ACK:

            if (!send_ack(route)) return false;
            route->state() = LocalTransportRoute::route_state::WAIT_ACK;
            // intentional fall-through

        case LocalTransportRoute::route_state::WAIT_ACK:
            // NOTE: This loop will be broken when a ACK package from this route arrives
            if (route->meta().last_package_out.is_more_than_n_in_past(now, route->meta().timings.heartbeat_interval_ms))
                route->state() = LocalTransportRoute::route_state::SEND_ACK;
            break;

        case LocalTransportRoute::route_state::CONNECTED:

            if (route->meta().last_package_out.is_more_than_n_in_past(now, route->meta().timings.heartbeat_interval_ms)) {
                if (!send_heartbeat(route)) return false;
            }

            break;
    }

    // NOTE: route should always be open at this point
    if (!read_from(route)) return false;

    return true;
}

bool LocalNode::open_route(LocalTransportRoute* route) {
    if (route->connection().open()) {
        auto now = timestamp::now();
        route->meta().last_package_in = now;
        route->meta().last_package_out = now;
        iac_log_from_node(Logging::loglevels::network, "opened route %d [%s]\n", route->id(), route->typestring().c_str());
        return true;
    }

    iac_log_from_node(Logging::loglevels::verbose, "error opening route %d [%s]\n", route->id(), route->typestring().c_str());
    return false;
}

bool LocalNode::close_route(LocalTransportRoute* route) {
    if (route->connection().close()) {
        if (!(m_network.disconnect_route(route->id()) && route->reset())) {
            iac_log_from_node(Logging::loglevels::warning, "error disconnecting route %d [%s] \n", route->id(), route->typestring().c_str());
            return false;
        }

        iac_log_from_node(Logging::loglevels::network, "closed route %d [%s]\n", route->id(), route->typestring().c_str());
        return true;
    }

    iac_log_from_node(Logging::loglevels::warning, "error closing route %d [%s] \n", route->id(), route->typestring().c_str());
    return false;
}

bool LocalNode::send_network_update(LocalTransportRoute* route) {
    BufferWriter writer;

    writer.num<uint8_t>(m_network.endpoint_mapping().size());
    for (const auto& ep_entry : m_network.endpoint_mapping()) {
        writer.num(ep_entry.second.element().id());
        writer.str(ep_entry.second.element().name());
        writer.num(ep_entry.second->m_node);
    }

    writer.num<uint8_t>(m_network.route_mapping().size() - 1);
    for (const auto& tr_entry : m_network.route_mapping()) {
        if (tr_entry.second.element_ptr() == route) continue;

        writer.num(tr_entry.second.element().id());
        writer.num(tr_entry.second->node1());
        writer.num(tr_entry.second->node2());
    }

    writer.num<uint8_t>(m_network.node_mapping().size() - 1);
    for (const auto& node_entry : m_network.node_mapping()) {
        if (node_entry.second.element_ptr() == this) continue;

        if (node_entry.second->local_routes().empty()) {
            IAC_HANDLE_FATAL_EXCEPTION(NonExistingException, "no local route leading to node, invalid state suspected");
            return false;
        }

        auto best_route = best_local_route(node_entry.second->local_routes());

        writer.num(node_entry.first);
        writer.num(best_route.second);
    }

    Package package{reserved_endpoint_addresses::IAC,
                    reserved_endpoint_addresses::IAC,
                    reserved_package_types::NETWORK_UPDATE, writer.buffer(), writer.size()};

    IAC_LOG_PACKAGE_SEND(Logging::loglevels::network, "network_update");

    return send_package(package, route);
}

bool LocalNode::send_heartbeat(LocalTransportRoute* route) {
    Package package{reserved_endpoint_addresses::IAC,
                    reserved_endpoint_addresses::IAC,
                    reserved_package_types::HEARTBEAT, nullptr, 0};

    IAC_LOG_PACKAGE_SEND(Logging::loglevels::verbose, "heartbeat");

    return send_package(package, route);
}

bool LocalNode::send_ack(LocalTransportRoute* route) {
    Package package{reserved_endpoint_addresses::IAC,
                    reserved_endpoint_addresses::IAC,
                    reserved_package_types::ACK, nullptr, 0};

    IAC_LOG_PACKAGE_SEND(Logging::loglevels::network, "ack");

    return send_package(package, route);
}

bool LocalNode::send_connect(LocalTransportRoute* route) {
    BufferWriter writer;

    writer.num(id());
    writer.num(route->id());

    writer.num(route->meta().timings.heartbeat_interval_ms);
    writer.num(route->meta().timings.assume_dead_after_ms);

    Package package{reserved_endpoint_addresses::IAC,
                    reserved_endpoint_addresses::IAC,
                    reserved_package_types::CONNECT, writer.buffer(), writer.size()};

    IAC_LOG_PACKAGE_SEND(Logging::loglevels::network, "connect");

    return send_package(package, route);
}

}  // namespace iac