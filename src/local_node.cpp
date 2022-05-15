#include "local_node.hpp"

#include "exceptions.hpp"
#include "network_types.hpp"

namespace iac {

LocalNode::LocalNode(route_timings_t timings)
    : Node(unset_id), m_default_route_timings(timings) {
    set_local(true);

    if (m_default_route_timings.heartbeat_interval_ms < s_min_heartbeat_interval_ms)
        m_default_route_timings.heartbeat_interval_ms = s_min_heartbeat_interval_ms;

    if (m_default_route_timings.assume_dead_after_ms < s_min_assume_dead_time)
        m_default_route_timings.assume_dead_after_ms = s_min_assume_dead_time;
};

uint8_t LocalNode::get_tr_id() {
    for (uint8_t id = 0; id < numeric_limits<uint8_t>::max(); ++id) {
        if (m_used_tr_ids.find(id) == m_used_tr_ids.end()) {
            m_used_tr_ids.insert(id);
            return id;
        }
    }

    IAC_HANDLE_FATAL_EXCPETION(OutOfTrIdException, "no more available ids for transport-routes");
    return 0;
}

bool LocalNode::pop_tr_id(uint8_t id) {
    return m_used_tr_ids.erase(id) == 1;
}

bool LocalNode::add_local_transport_route(LocalTransportRoute& route) {
    auto& timings = route.meta().timings;

    if (timings.heartbeat_interval_ms < s_min_heartbeat_interval_ms)
        timings.heartbeat_interval_ms = s_min_heartbeat_interval_ms;
    if (timings.assume_dead_after_ms < s_min_assume_dead_time)
        timings.assume_dead_after_ms = s_min_assume_dead_time;

    // node ids consist of a 2-byte int, the 8 msb are the node id
    // the other 8 bits represent a unique (in LocalNode) id
    static constexpr uint8_t shift_by = 8;
    route.set_id((id() << shift_by) | get_tr_id());
    route.set_node1(id());

    return m_network.add_route(ManagedNetworkEntry<TransportRoute>::create_and_bind(route));
}

bool LocalNode::remove_local_transport_route(LocalTransportRoute& route) {
    route.close();
    return m_network.remove_route(route.id()) && route.state() == LocalTransportRoute::route_state::CLOSED;
}

bool LocalNode::add_local_endpoint(LocalEndpoint& ep) {
    if (id() == unset_id) {
        set_id(ep.id());
        ep.set_node(id());
        m_network.add_node(ManagedNetworkEntry<Node>::create_and_bind(this));
    }

    return m_network.add_endpoint(ManagedNetworkEntry<Endpoint>::create_and_bind(ep));
}

bool LocalNode::remove_local_endpoint(LocalEndpoint& ep) {
    return m_network.remove_endpoint(ep.id());
}

pair<tr_id_t, uint8_t> LocalNode::best_local_route(const unordered_map<tr_id_t, uint8_t>& local_routes) {
    pair<tr_id_t, uint8_t> best_route = *local_routes.begin();
    for (const auto& route : local_routes)
        if (route.second < best_route.second)
            best_route = route;

    return best_route;
}

bool LocalNode::handle_package(Package& package, LocalTransportRoute* route) {
    // if no route is forced, it has to be resolved
    if (route == nullptr) {
        if (package.type() == reserved_package_types::CONNECT) {
            return handle_connect(package);
        }

        if (package.type() == reserved_package_types::HEARTBEAT) {
            if (package.route()->state() == LocalTransportRoute::route_state::WAITING_FOR_HANDSHAKE_REPLY)
                package.route()->state() = LocalTransportRoute::route_state::CONNECTED;
            return handle_heartbeat(package);
        }

        if (package.type() == reserved_package_types::DISCONNECT && package.route()->state() == LocalTransportRoute::route_state::CONNECTED) {
            return handle_disconnect(package);
        }

        if (!m_network.endpoint_registered(package.to())) {
            iac_log(Logging::loglevels::error, "received package for unregistered endpoint\n");
            return false;
        }

        const auto& ep = m_network.endpoint(package.to());

        if (ep.local())
            return ((const LocalEndpoint&)ep).handle_package(package);

        const auto& available_routes = m_network.node(ep.node()).local_routes();
        if (available_routes.empty())
            return false;

        route = (LocalTransportRoute*)&m_network.route(best_local_route(available_routes).first);
    }

    if (route->state() != LocalTransportRoute::route_state::INITIALIZED || route->state() != LocalTransportRoute::route_state::CLOSED) {
        route->meta().last_package_out = timestamp();
        return package.send_over(route);
    } else {
        iac_log(Logging::loglevels::warning, "asked to send package with type %d to %d over route in state %d\n", package.type(), package.to(), route->state());
    }

    return false;
}

bool LocalNode::handle_connect(Package& package) {
    BufferReader reader{package.payload(), package.payload_size()};

    // package.print();

    auto relay = reader.boolean();

    iac_log(Logging::loglevels::debug, "node %d received connect pkg; state: %d%s\n", id(), package.route()->state(), relay == 1 ? " [relay]" : "");

    if (relay && package.route()->state() != LocalTransportRoute::route_state::CONNECTED) {
        iac_log(Logging::loglevels::debug, "route is not connected, discarding relay package\n");
        return true;
    }

    auto sender_id = reader.num<node_id_t>();
    auto other_tr_id = reader.num<tr_id_t>();

    package.route()->meta().timings.heartbeat_interval_ms = max_of(reader.num<uint16_t>(), package.route()->meta().timings.heartbeat_interval_ms);
    package.route()->meta().timings.assume_dead_after_ms = max_of(reader.num<uint16_t>(), package.route()->meta().timings.assume_dead_after_ms);

    if (!m_network.node_registered(sender_id)) {
        m_network.add_node(ManagedNetworkEntry<Node>::create_and_adopt(new Node(sender_id)));
    }

    if (!relay) {
        if (other_tr_id < package.route()->id()) {
            iac_log(Logging::loglevels::debug, "changing route id\n");
            const auto old_id = package.route()->id();

            auto erase_tr_links = [&](node_id_t node_id) {
                if (node_id != unset_id) {
                    auto& node_entry = m_network.mutable_node(node_id);

                    auto node_route_res = node_entry.routes().find(package.route()->id());
                    if (node_route_res != node_entry.routes().end()) {
                        node_entry.remove_route(node_route_res);
                    }

                    auto node_local_route_res = node_entry.local_routes().find(package.route()->id());
                    if (node_local_route_res != node_entry.local_routes().end()) {
                        node_entry.remove_local_route(node_local_route_res);
                    }
                }
            };

            auto insert_tr_links = [&](node_id_t node_id) {
                if (node_id != unset_id) {
                    auto& node_entry = m_network.mutable_node(node_id);

                    node_entry.add_route(other_tr_id);
                    node_entry.add_local_route(other_tr_id, 1);
                }
            };

            auto temp = move(m_network.mutable_route_managed_entry(old_id));
            m_network.erase_route_managed_entry(old_id);

            erase_tr_links(package.route()->nodes().first);
            erase_tr_links(package.route()->nodes().second);

            package.route()->set_id(other_tr_id);
            m_network.add_route(move(temp));

            insert_tr_links(package.route()->node1());
            insert_tr_links(package.route()->node2());
        }

        package.route()->set_node2(sender_id);
        auto& sender = m_network.mutable_node(sender_id);
        sender.add_route(package.route()->id());
        sender.add_local_route({package.route()->id(), 1});

        iac_log(Logging::loglevels::connect, "connecting %d to %d\n", id(), sender_id);

        iac_log(Logging::loglevels::verbose, "connect with timing: %d %d\n",
                package.route()->meta().timings.heartbeat_interval_ms,
                package.route()->meta().timings.assume_dead_after_ms);
    } else {
        iac_log(Logging::loglevels::debug, "relay pkg\n");
    }

    const auto num_ep_data = reader.num<uint8_t>();
    for (uint8_t i = 0; i < num_ep_data; ++i) {
        const auto ep_id = reader.num<ep_id_t>();
        const auto* ep_name = reader.str();
        const auto ep_node = reader.num<node_id_t>();

        if (!m_network.endpoint_registered(ep_id)) {
            auto* ep = new Endpoint(ep_id);
            ep->set_name(ep_name);
            ep->set_node(ep_node);
            m_network.add_endpoint(ManagedNetworkEntry<Endpoint>::create_and_adopt(ep));
        }
    }

    const auto num_tr_data = reader.num<uint8_t>();
    for (uint8_t i = 0; i < num_tr_data; ++i) {
        const auto tr_id = reader.num<tr_id_t>();
        const auto node1_id = reader.num<node_id_t>();
        const auto node2_id = reader.num<node_id_t>();

        if (node1_id != unset_id && node2_id != unset_id && !m_network.route_registered(tr_id)) {
            auto* tr = new TransportRoute(tr_id, {node1_id, node2_id});
            m_network.add_route(ManagedNetworkEntry<TransportRoute>::create_and_adopt(tr));
        }
    }

    const auto num_ltr_data = reader.num<uint8_t>();
    for (uint8_t i = 0; i < num_ltr_data; ++i) {
        const auto reachable_node = reader.num<node_id_t>();
        const auto num_hops = reader.num<uint8_t>();

        auto& reachable_node_entry = m_network.mutable_node(reachable_node);
        auto res = reachable_node_entry.local_routes().find(package.route()->id());

        if (res == reachable_node_entry.local_routes().end() || res->second > num_hops + 1) {
            reachable_node_entry.add_local_route(package.route()->id(), num_hops + 1);
        }
    }

    return send_heartbeat(package.route());
}

bool LocalNode::handle_disconnect(const Package& package) {
    return handle_disconnect(package.route());
}

bool LocalNode::handle_disconnect(LocalTransportRoute* route) {
    // save what the route was connected to and reset it to unset value
    auto was_connected_to = m_network.route(route->id()).node2();

    if (was_connected_to != unset_id) {
        iac_log(Logging::loglevels::connect, "disconnecting %d(self) from %d\n", id(), was_connected_to);

        // the node the route was connected to
        auto& concerning_node = m_network.mutable_node(was_connected_to);

        // remove the disconnected route
        concerning_node.remove_route(route->id());
        concerning_node.remove_local_route(route->id());
        route->set_node2(unset_id);

        // if no connections lead to this node anymore, delete it
        if (concerning_node.local_routes().empty()) {
            if (!m_network.remove_node(was_connected_to)) IAC_ASSERT_NOT_REACHED();
        }
    }

    return true;
}

bool LocalNode::handle_heartbeat(const Package& package) {
    iac_log(Logging::loglevels::debug, "heartbeat from: %d node: %d (self) %d %d\n", package.from(),
            id(),
            timestamp() - package.route()->meta().last_package_in,
            timestamp() - package.route()->meta().last_package_out);
    return true;
}

bool LocalNode::update() {
    if (m_network.endpoint_mapping().empty()) {
        IAC_HANDLE_EXCPETION(NoRegisteredEndpointsException, "updating node with no endpoints");
        return false;
    }

    for (auto it = m_network.route_mapping().begin(); it != m_network.route_mapping().end();) {
        if (!it->second->local()) {
            it++;
            continue;
        }

        auto* route = (LocalTransportRoute*)(it->second.element_ptr());

        it++;  // iterator might get invalidatend in handle_connect_package, so we have to increment now;

        // iac_printf("route %p; mode %d; last in: %d; last out: %d\n", route.first,
        // route.first->m_state, timestamp() - route.first->m_last_package_in,
        // timestamp() - route.first->m_last_package_out);

        iac_log(Logging::loglevels::verbose, "state of route %d @ node %d is %d\n", route->id(), id(), route->state());

        switch (route->state()) {
            case LocalTransportRoute::route_state::INITIALIZED:
            case LocalTransportRoute::route_state::CLOSED:
                if (!open_route(route)) break;

            case LocalTransportRoute::route_state::OPEN:
                route->meta().last_package_in = timestamp();

            case LocalTransportRoute::route_state::WAITING_FOR_HANDSHAKE_REPLY:
                if (!send_handshake(route)) break;

            case LocalTransportRoute::route_state::CONNECTED:
                if (timestamp() - route->meta().last_package_in > route->meta().timings.assume_dead_after_ms) {
                    if (!close_route(route)) return false;
                }

                if (timestamp() - route->meta().last_package_out > route->meta().timings.heartbeat_interval_ms) {
                    if (!send_heartbeat(route)) return false;
                }

                if (!read_from(route)) return false;
                break;
        }
    }

    if (m_network.is_modified()) {
        m_network.reset_modified();
        for (const auto& route_entry : m_network.route_mapping()) {
            if (!route_entry.second->local()) continue;
            auto* route = (LocalTransportRoute*)route_entry.second.element_ptr();
            if (route->state() != LocalTransportRoute::route_state::CONNECTED) continue;
            send_connect_package(route, true);
        }
    }

    return true;
}

bool LocalNode::read_from(LocalTransportRoute* route) {
    for (size_t i = 0; i < s_num_package_reads_from_route_per_update && route->available() > 0; i++) {
        Package package;
        if (package.read_from(route)) {
            route->meta().last_package_in = timestamp();
            if (!handle_package(package)) return false;
        } else {
            break;
        }
    }
    return true;
}

bool LocalNode::open_route(LocalTransportRoute* route) {
    if (route->state() != LocalTransportRoute::route_state::OPEN && route->open()) {
        route->state() = LocalTransportRoute::route_state::OPEN;
        iac_log(Logging::loglevels::network, "opened route %d [%s]\n", route->id(), route->typestring().c_str());
        return true;
    }

    iac_log(Logging::loglevels::warning, "error opening route %d [%s]\n", route->id(), route->typestring().c_str());
    return false;
}

bool LocalNode::close_route(LocalTransportRoute* route) {
    if (route->state() != LocalTransportRoute::route_state::CLOSED && route->close()) {
        if (!handle_disconnect(route)) {
            iac_log(Logging::loglevels::warning, "error disconnecting route %d [%s] \n", route->id(), route->typestring().c_str());
            return false;
        }
        route->state() = LocalTransportRoute::route_state::CLOSED;
        iac_log(Logging::loglevels::network, "closed route %d [%s]\n", route->id(), route->typestring().c_str());
        return true;
    }

    iac_log(Logging::loglevels::warning, "error closing route %d [%s] \n", route->id(), route->typestring().c_str());
    return false;
}

bool LocalNode::send_handshake(LocalTransportRoute* route) {
    if (send_connect_package(route, false)) {
        if (route->state() == LocalTransportRoute::route_state::OPEN)
            route->state() = LocalTransportRoute::route_state::WAITING_FOR_HANDSHAKE_REPLY;
    } else {
        iac_log(Logging::loglevels::warning, "error sending connect package to route %d [%s] \n", route->id(), route->typestring().c_str());
        return false;
    }

    return true;
}

bool LocalNode::send_heartbeat(LocalTransportRoute* route) {
    Package package{reserved_endpoint_addresses::IAC,
                    reserved_endpoint_addresses::IAC,
                    reserved_package_types::HEARTBEAT, nullptr, 0};

    // iac_log(Logging::loglevels::debug,"sending heartbeat package\n");
    //  package.print();

    return handle_package(package, route);
}

bool LocalNode::send_connect_package(LocalTransportRoute* route, bool relay) {
    BufferWriter writer;

    writer.boolean(relay);

    writer.num(id());
    writer.num(route->id());

    writer.num(route->meta().timings.heartbeat_interval_ms);
    writer.num(route->meta().timings.assume_dead_after_ms);

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
            IAC_HANDLE_FATAL_EXCPETION(NonExistingException, "no local route leading to node, invalid state suspected");
            return false;
        }

        auto best_route = best_local_route(node_entry.second->local_routes());

        writer.num(node_entry.first);
        writer.num(best_route.second);
    }

    Package package{reserved_endpoint_addresses::IAC,
                    reserved_endpoint_addresses::IAC,
                    reserved_package_types::CONNECT, writer.buffer(), writer.size()};

    iac_log(Logging::loglevels::network, "sending %s package from %d to %d over %d\n", relay ? "relay" : "connect", id(), m_network.route(route->id()).node2(), route->id());
    // package.print();

    return handle_package(package, route);
}

}  // namespace iac