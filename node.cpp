#include "node.hpp"

#include "logging.hpp"
#include "network_types.hpp"

namespace iac {

LocalNode::LocalNode(uint16_t heartbeat_interval_ms, uint16_t assume_dead_after_ms)
    : Node(unset_id), m_connection_keep_alive_pkg_every_ms(heartbeat_interval_ms), m_connection_dead_after_ms(assume_dead_after_ms) {
    m_local = true;
    if (m_connection_keep_alive_pkg_every_ms < 100) m_connection_keep_alive_pkg_every_ms = 100;
    if (m_connection_dead_after_ms < 200) m_connection_dead_after_ms = 200;
};

uint8_t LocalNode::get_tr_id() {
    for (uint8_t id = 0; id < numeric_limits<uint8_t>::max(); ++id) {
        if (m_used_tr_ids.find(id) == m_used_tr_ids.end()) {
            m_used_tr_ids.insert(id);
            return id;
        }
    }

    IAC_HANDLE_FATAL_EXCEPTION(OutOfTrIdException, "no more available ids for transport-routes");
    return 0;
}

bool LocalNode::pop_tr_id(uint8_t id) {
    return m_used_tr_ids.erase(id) == 1;
}

[[nodiscard]] bool LocalNode::add_local_transport_route(LocalTransportRoute& route) {
    route.m_heartbeat_interval_ms = m_connection_keep_alive_pkg_every_ms;
    route.m_assume_dead_after_ms = m_connection_dead_after_ms;

    route.m_id = (m_id << 8) | get_tr_id();
    route.m_node1 = m_id;

    return add_transport_route((TransportRoute&)route);
}

[[nodiscard]] bool LocalNode::add_transport_route(TransportRoute& route) {
    if (m_tr_mapping.find(route.id()) != m_tr_mapping.end()) {
        IAC_HANDLE_FATAL_EXCEPTION(AddDuplicateException, "added already existing route, memory leak suspected");
        return false;
    }

    m_tr_mapping[route.id()].bind(route);

    if (route.m_node1 != unset_id) {
        auto node1_entry = add_node_if_not_existing(route.m_node1);
        node1_entry.second->m_routes.insert(route.id());
    }

    if (route.m_node2 != unset_id) {
        auto node2_entry = add_node_if_not_existing(route.m_node2);
        node2_entry.second->m_routes.insert(route.id());
    }

    m_mapping_changed = true;

    // IAC_ASSERT(validate_network());

    return true;
}

bool LocalNode::remove_local_transport_route(LocalTransportRoute& route) {
    route.close();
    return remove_transport_route(route.id()) && route.m_state == LocalTransportRoute::route_state::CLOSED;
}

bool LocalNode::remove_transport_route(tr_id_t route_id) {
    const auto& res = m_tr_mapping.find(route_id);

    if (res == m_tr_mapping.end()) {
        IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "removing non existant tr");
        return false;
    }

    if (res->second->m_node1 != unset_id) {
        auto node1_entry = add_node_if_not_existing(res->second->m_node1);
        if (node1_entry.first) {
            remove_node(res->second->m_node1);  // clean up as best we can, network model is in invalid state
            IAC_HANDLE_FATAL_EXCEPTION(RemoveOfInvalidException, "linked node was non existant");
        }
        node1_entry.second->m_routes.erase(res->first);
        node1_entry.second->m_local_routes.erase(res->first);
    }

    if (res->second->m_node2 != unset_id) {
        auto node2_entry = add_node_if_not_existing(res->second->m_node2);
        if (node2_entry.first) {
            remove_node(res->second->m_node2);  // clean up as best we can, network model is in invalid state
            IAC_HANDLE_FATAL_EXCEPTION(RemoveOfInvalidException, "linked node was non existant");
        }
        node2_entry.second->m_routes.erase(res->first);
        node2_entry.second->m_local_routes.erase(res->first);
    }

    m_tr_mapping.erase(res);

    m_mapping_changed = true;

    // IAC_ASSERT(validate_network());

    return true;
}

[[nodiscard]] bool LocalNode::add_local_endpoint(LocalEndpoint& ep) {
    if (m_id == unset_id) {
        m_id = ep.id();
        ep.m_node = m_id;
        add_node(*this);
    }

    return add_endpoint((Endpoint&)ep);
}

bool LocalNode::remove_local_endpoint(LocalEndpoint& ep) {
    return remove_endpoint(ep.id());
}

[[nodiscard]] bool LocalNode::add_endpoint(Endpoint& ep) {
    if (m_ep_mapping.find(ep.id()) != m_ep_mapping.end()) {
        IAC_HANDLE_FATAL_EXCEPTION(AddDuplicateException, "added already existing endpoint, memory leak suspected");
        return false;
    }

    m_ep_mapping[ep.id()].bind(ep);

    auto node_entry = add_node_if_not_existing(ep.m_node);
    node_entry.second->m_endpoints.insert(ep.id());

    m_mapping_changed = true;

    // IAC_ASSERT(validate_network());

    return true;
}

bool LocalNode::remove_endpoint(ep_id_t ep_id) {
    const auto& res = m_ep_mapping.find(ep_id);

    if (res == m_ep_mapping.end()) {
        IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "removing non existant ep");
        return false;
    }

    // TODO: dont create node when not existing
    auto node_entry = add_node_if_not_existing(res->second->m_node);
    node_entry.second->m_endpoints.erase(res->first);

    m_ep_mapping.erase(res);

    m_mapping_changed = true;

    // IAC_ASSERT(validate_network());

    return true;
}

[[nodiscard]] bool LocalNode::add_node(Node& node) {
    if (m_node_mapping.find(node.id()) != m_node_mapping.end()) {
        IAC_HANDLE_FATAL_EXCEPTION(AddDuplicateException, "added already existing node, memory leak suspected");
        return false;
    }

    m_node_mapping[node.id()].bind(node);

    m_mapping_changed = true;

    // IAC_ASSERT(validate_network());

    return true;
}

bool LocalNode::remove_node(node_id_t node_id) {
    const auto& res = m_node_mapping.find(node_id);

    if (res == m_node_mapping.end()) {
        IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "removing non existant node");
        return false;
    }

    for (auto it = res->second->m_endpoints.begin(); it != res->second->m_endpoints.end();) {
        auto ep_id = *it++;
        if (!remove_endpoint(ep_id)) {
            IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "removing non existant, but linked ep");
            return false;
        }
    }

    for (auto it = res->second->m_routes.begin(); it != res->second->m_routes.end();) {
        auto tr_id = *it++;
        if (!remove_transport_route(tr_id)) {
            IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "removing non existant, but linked tr");
            return false;
        }
    }

    m_node_mapping.erase(res);

    m_mapping_changed = true;

    // IAC_ASSERT(validate_network());

    return true;
}

std::pair<bool, ManagedNetworkEntry<Node>&> LocalNode::add_node_if_not_existing(node_id_t node_id) {
    auto res = m_node_mapping.find(node_id);
    if (res == m_node_mapping.end()) {
        auto node = new Node(node_id);
        if (!add_node(*node)) {
            delete node;
            IAC_HANDLE_FATAL_EXCEPTION(AddDuplicateException, "add node failed, invalid state suspected");
        }
        return {true, m_node_mapping[node_id]};
    }
    return {false, m_node_mapping[node_id]};
}

std::pair<bool, ManagedNetworkEntry<Endpoint>&> LocalNode::add_ep_if_not_existing(ep_id_t ep_id, string name, node_id_t node) {
    auto res = m_ep_mapping.find(ep_id);
    if (res == m_ep_mapping.end()) {
        auto ep = new Endpoint(ep_id, name, node);
        if (!add_endpoint(*ep)) {
            delete ep;
            IAC_HANDLE_FATAL_EXCEPTION(AddDuplicateException, "add ep failed, invalid state suspected");
        }
        return {true, m_ep_mapping[ep_id]};
    }
    return {false, m_ep_mapping[ep_id]};
}

std::pair<bool, ManagedNetworkEntry<TransportRoute>&> LocalNode::add_tr_if_not_existing(tr_id_t tr_id, node_id_t node1, node_id_t node2) {
    auto res = m_tr_mapping.find(tr_id);
    if (res == m_tr_mapping.end()) {
        auto tr = new TransportRoute(tr_id, node1, node2);
        if (!add_transport_route(*tr)) {
            delete tr;
            IAC_HANDLE_FATAL_EXCEPTION(AddDuplicateException, "add tr failed, invalid state suspected");
        }
        return {true, m_tr_mapping[tr_id]};
    }
    return {false, m_tr_mapping[tr_id]};
}

pair<tr_id_t, uint8_t> LocalNode::best_local_route(unordered_map<tr_id_t, uint8_t>& local_routes) {
    pair<tr_id_t, uint8_t> best_route = *local_routes.begin();
    for (auto& route : local_routes)
        if (route.second < best_route.second)
            best_route = route;

    return best_route;
}

bool LocalNode::handle_package(Package& package, LocalTransportRoute* route) {
    // if no route is forced, it has to be resolved
    if (!route) {
        // if the package destination is 'IAC' read it now

        if (package.type() == reserved_package_types::CONNECT) {
            return handle_connect(package);
        } else if (package.type() == reserved_package_types::DISCONNECT) {
            return handle_disconnect(package);
        } else if (package.type() == reserved_package_types::HEARTBEAT) {
            return handle_heartbeat(package);
        } else if (package.to() == reserved_endpoint_addresses::IAC) {
            IAC_HANDLE_EXCEPTION(InvalidPackageException, "package had reserved address, but not reserved type");
            return false;
        }

        auto ep_entry = m_ep_mapping.find(package.to());
        if (ep_entry == m_ep_mapping.end())
            return false;

        if (ep_entry->second->m_local)
            return ((LocalEndpoint*)ep_entry->second.element_ptr())->handle_package(package);

        auto& available_routes = m_node_mapping[ep_entry->second->m_node]->m_local_routes;
        if (available_routes.size() == 0)
            return false;

        pair<tr_id_t, uint8_t> best_tr_entry = *(available_routes.begin());
        for (auto& tr_entry : available_routes) {
            if (tr_entry.second < best_tr_entry.second)
                best_tr_entry = tr_entry;
        }

        route = (LocalTransportRoute*)m_tr_mapping[best_tr_entry.first].element_ptr();
    }

    // send package over 'route' if 'route' is connected and setup
    if (route->m_state == LocalTransportRoute::route_state::TESTING || route->m_state == LocalTransportRoute::route_state::WORKING) {
        route->m_last_package_out = timestamp();
        return package.send_over(route);
    }

    return false;
}

bool LocalNode::handle_connect(Package& package) {
    BufferReader reader{package.payload(), package.payload_size()};

    // package.print();

    auto relay = reader.boolean();

    iac_log(debug, "received connect pkg %d\n", relay);

    auto sender_pid = reader.num<node_id_t>();
    auto other_tr_id = reader.num<tr_id_t>();

    auto other_connection_keep_alive_pkg_every_ms = max(reader.num<uint16_t>(), m_connection_keep_alive_pkg_every_ms);
    auto other_connection_dead_after_ms = max(reader.num<uint16_t>(), m_connection_dead_after_ms);

    if (!relay) {
        m_tr_mapping[package.route()->id()]->m_node2 = sender_pid;
        add_node_if_not_existing(sender_pid).second->m_routes.insert(package.route()->id());

        if (other_tr_id < package.route()->id()) {
            iac_log(debug, "changing route id\n");
            auto& new_entry = m_tr_mapping[other_tr_id];
            new_entry = move(m_tr_mapping[package.route()->id()]);

            auto replace_tr_links = [&](node_id_t node_id) {
        if (node_id != unset_id) {
            auto& node_entry = m_node_mapping[node_id];
            auto node_route_res = node_entry->m_routes.find(package.route()->id());
            if (node_route_res != node_entry->m_routes.end()) {
                node_entry->m_routes.erase(node_route_res);
                node_entry->m_routes.insert(other_tr_id);
            }

             auto node_local_route_res = node_entry->m_local_routes.find(package.route()->id());
             if (node_local_route_res != node_entry->m_local_routes.end()) {
                 auto num_hops = node_local_route_res->second;
                node_entry->m_local_routes.erase(node_local_route_res);
                node_entry->m_local_routes.insert({other_tr_id,num_hops});
            }
        } };

            replace_tr_links(new_entry->m_node1);
            replace_tr_links(new_entry->m_node2);

            m_tr_mapping.erase(package.route()->id());
            package.route()->m_id = other_tr_id;
        }

        iac_log(connect, "connecting %d to %d\n", m_id, sender_pid);

        package.route()->m_heartbeat_interval_ms = other_connection_keep_alive_pkg_every_ms;
        package.route()->m_assume_dead_after_ms = other_connection_dead_after_ms;

        iac_log(verbose, "connect with timing: %d %d\n",
                package.route()->m_heartbeat_interval_ms,
                package.route()->m_assume_dead_after_ms);
    } else {
        iac_log(debug, "relay pkg\n");
    }

    auto res = add_node_if_not_existing(sender_pid);
    res.second->m_local_routes.insert({package.route()->id(), 1});

    auto num_ep_data = reader.num<uint8_t>();
    for (uint8_t i = 0; i < num_ep_data; ++i) {
        auto ep_id = reader.num<ep_id_t>();
        auto ep_name = reader.str();
        auto ep_node = reader.num<node_id_t>();

        add_ep_if_not_existing(ep_id, ep_name, ep_node);
    }

    auto num_tr_data = reader.num<uint8_t>();
    for (uint8_t i = 0; i < num_tr_data; ++i) {
        auto tr_id = reader.num<tr_id_t>();
        auto node1_id = reader.num<node_id_t>();
        auto node2_id = reader.num<node_id_t>();

        if (node1_id != unset_id && node2_id != unset_id)
            add_tr_if_not_existing(tr_id, node1_id, node2_id);
    }

    auto num_ltr_data = reader.num<uint8_t>();
    for (uint8_t i = 0; i < num_ltr_data; ++i) {
        auto reachable_node = reader.num<node_id_t>();
        auto num_hops = reader.num<uint8_t>();

        auto res = m_node_mapping[reachable_node]->m_local_routes.find(package.route()->id());
        if (res == m_node_mapping[reachable_node]->m_local_routes.end() || res->second > num_hops + 1) {
            auto& node_entry = m_node_mapping[reachable_node];
            node_entry->m_local_routes[package.route()->id()] = num_hops + 1;
            m_mapping_changed = true;
        }
    }

    IAC_ASSERT(validate_network());

    return true;
}

bool LocalNode::handle_disconnect(const Package& package) {
    return handle_disconnect(package.route());
}

bool LocalNode::handle_disconnect(LocalTransportRoute* route) {
    // save what the route was connected to and reset it to unset value

    // FIXME: node2 is not necessarily the other node
    auto was_connected_to = m_tr_mapping[route->id()]->m_node2;
    if (was_connected_to != unset_id) {
        m_tr_mapping[route->id()]->m_node2 = unset_id;

        iac_log(connect, "disconnecting %d(self) from %d\n", m_id, was_connected_to);

        // the node the route was connected to
        auto& concerning_node = m_node_mapping[was_connected_to];

        // remove the disconnected route
        concerning_node->m_routes.erase(route->id());
        concerning_node->m_local_routes.erase(route->id());

        IAC_ASSERT(validate_network());

        // if no connections lead to this node anymore, delete it
        if (concerning_node->m_local_routes.empty()) {
            remove_node(was_connected_to);
        }
    }

    IAC_ASSERT(validate_network());

    return true;
}

bool LocalNode::handle_heartbeat(const Package& package) {
    // iac_log(debug,"heartbeat from: %d node: %p %d %d\n", package.from(),
    //                this,
    //                timestamp() - package.route()->m_last_package_in,
    //                timestamp() - package.route()->m_last_package_out);
    return true;
}

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

bool LocalNode::update() {
    if (!m_ep_mapping.size()) {
        IAC_HANDLE_EXCEPTION(NoRegisteredEndpointsException, "updating node with no endpoints");
        return false;
    }

    for (auto it = m_tr_mapping.begin(); it != m_tr_mapping.end();) {
        if (!it->second->m_local) {
            it++;
            continue;
        }

        auto route = (LocalTransportRoute*)it->second.element_ptr();

        it++;  // iterator might get invalidatend in handle_connect_package, so we have to increment now;

        // iac_printf("route %p; mode %d; last in: %d; last out: %d\n", route.first,
        // route.first->m_state, timestamp() - route.first->m_last_package_in,
        // timestamp() - route.first->m_last_package_out);
        switch (route->m_state) {
            case LocalTransportRoute::route_state::INIT:
            case LocalTransportRoute::route_state::CLOSED:
                if (!route_open(route)) return false;
                if (route->m_state != LocalTransportRoute::route_state::OPEN)
                    break;

            case LocalTransportRoute::route_state::OPEN:
                if (!route_test(route)) return false;
                route->m_last_package_in = timestamp();
                break;

            case LocalTransportRoute::route_state::TESTING:
            case LocalTransportRoute::route_state::WORKING:

                if (timestamp() - route->m_last_package_in > route->m_assume_dead_after_ms) {
                    if (!route_close(route)) return false;
                }

                if (timestamp() - route->m_last_package_out > route->m_heartbeat_interval_ms) {
                    if (!route_keep_alive(route)) return false;
                }

                if (!route_try_read(route)) return false;
                break;
        }
    }

    if (m_mapping_changed) {
        m_mapping_changed = false;
        for (auto& route_entry : m_tr_mapping) {
            if (!route_entry.second->m_local) continue;
            auto route = (LocalTransportRoute*)route_entry.second.element_ptr();
            if (route->m_state != LocalTransportRoute::route_state::WORKING) continue;
            send_connect_package(route);
        }
    }

    return true;
}

bool LocalNode::route_try_read(LocalTransportRoute* route) {
    for (size_t i = 0; i < 5 && route->available(); i++) {
        Package package;
        if (package.read_from(route)) {
            route->m_last_package_in = timestamp();
            if (!handle_package(package)) return false;
            route->m_state = LocalTransportRoute::route_state::WORKING;
        } else {
            break;
        }
    }
    return true;
}

bool LocalNode::route_open(LocalTransportRoute* route) {
    if (route->m_state != LocalTransportRoute::route_state::OPEN && route->open())
        route->m_state = LocalTransportRoute::route_state::OPEN;

    iac_log(network, "opening route %p [%s] %s\n", route, route->typestring().c_str(), route->m_state == LocalTransportRoute::route_state::OPEN ? "succeeded" : "failed");
    return true;
}

bool LocalNode::route_close(LocalTransportRoute* route) {
    if (route->m_state != LocalTransportRoute::route_state::CLOSED && route->close()) {
        if (!handle_disconnect(route)) return false;
        route->m_state = LocalTransportRoute::route_state::CLOSED;
    }

    iac_log(network, "closing route %p [%s] %s\n", route, route->typestring().c_str(), route->m_state == LocalTransportRoute::route_state::CLOSED ? "succeeded" : "failed");
    return true;
}

bool LocalNode::route_test(LocalTransportRoute* route) {
    route->m_state = LocalTransportRoute::route_state::TESTING;
    return send_connect_package(route, false);
}

bool LocalNode::route_keep_alive(LocalTransportRoute* route) {
    Package package{reserved_endpoint_addresses::IAC,
                    reserved_endpoint_addresses::IAC,
                    reserved_package_types::HEARTBEAT, nullptr, 0};

    // iac_log(debug,"sending heartbeat package\n");
    //  package.print();

    return handle_package(package, route);
}

bool LocalNode::send_connect_package(LocalTransportRoute* route, bool relay) {
    BufferWriter writer;

    writer.boolean(relay);

    writer.num(m_id);
    writer.num(route->id());

    writer.num(m_connection_keep_alive_pkg_every_ms);
    writer.num(m_connection_dead_after_ms);

    writer.num<uint8_t>(m_ep_mapping.size());
    for (auto& ep_entry : m_ep_mapping) {
        writer.num(ep_entry.second.element().id());
        writer.str(ep_entry.second.element().name());
        writer.num(ep_entry.second->m_node);
    }

    writer.num<uint8_t>(m_tr_mapping.size() - 1);
    for (auto& tr_entry : m_tr_mapping) {
        if (tr_entry.second.element_ptr() == route) continue;

        writer.num(tr_entry.second.element().id());
        writer.num(tr_entry.second->m_node1);
        writer.num(tr_entry.second->m_node2);
    }

    writer.num<uint8_t>(m_node_mapping.size() - 1);
    for (auto& node_entry : m_node_mapping) {
        if (node_entry.second.element_ptr() == this) continue;

        if (node_entry.second->m_local_routes.size() == 0) {
            IAC_HANDLE_FATAL_EXCEPTION(NonExistingException, "no local route leading to node, invalid state suspected");
            return false;
        }

        auto best_route = best_local_route(node_entry.second->m_local_routes);

        writer.num(node_entry.first);
        writer.num(best_route.second);
    }

    Package package{reserved_endpoint_addresses::IAC,
                    reserved_endpoint_addresses::IAC,
                    reserved_package_types::CONNECT, writer.buffer(), writer.size()};

    iac_log(network, "sending %s package from %d to %d\n", relay ? "relay" : "connect", m_id, m_tr_mapping[route->id()]->m_node2);
    // package.print();

    return handle_package(package, route);
}

bool LocalNode::is_endpoint_connected(ep_id_t address) {
    return m_ep_mapping.find(address) != m_ep_mapping.end();
}

bool LocalNode::are_endpoints_connected(vector<ep_id_t> addresses) {
    for (auto& address : addresses)
        if (!is_endpoint_connected(address)) {
            iac_log(verbose, "node %d connected test failed for %d\n", m_id, address);
            return false;
        }
    return true;
}

void LocalNode::print_endpoint_list() {
    IAC_PRINT_PROVIDER("endpoint listing for #%d @ %p\n", m_id, this);
    for (auto& entry : m_ep_mapping) {
        auto& ep_entry = entry.second;
        auto& routes = m_node_mapping[entry.second->m_node]->m_routes;

        IAC_PRINT_PROVIDER("  |\n  +-- %s - routes to%s endpoint #%d (%lu):\n",
                           ep_entry.element().name().c_str(),
                           ep_entry->m_local ? " local" : "",
                           ep_entry.element().id(),
                           routes.size());

        for (auto& route_id : routes) {
            auto& tr_entry = m_tr_mapping[route_id];
            IAC_PRINT_PROVIDER("  |   +-- ");
            IAC_PRINT_PROVIDER("route #%d of type %s [%s]", tr_entry.element().id(), tr_entry.element().typestring().c_str(), tr_entry.element().infostring().c_str());
            IAC_PRINT_PROVIDER("\n");
        }

        IAC_PRINT_PROVIDER("\n");
    }
}

bool LocalNode::validate_network() {
    for (auto& node_entry : m_node_mapping) {
        for (auto& ep_entry : node_entry.second->m_endpoints)
            if (m_ep_mapping.find(ep_entry) == m_ep_mapping.end()) {
                iac_log(error, "ep linked to node not registered\n");
                return false;
            }
        for (auto& tr_entry : node_entry.second->m_routes)
            if (m_tr_mapping.find(tr_entry) == m_tr_mapping.end()) {
                iac_log(error, "ep linked to node not registered\n");
                return false;
            }
    }

    for (auto& node_entry : m_ep_mapping) {
        if (m_node_mapping.find(node_entry.second->m_node) == m_node_mapping.end()) {
            iac_log(error, "node linked to ep not registered\n");
            return false;
        }
    }
    for (auto& node_entry : m_tr_mapping) {
        if (node_entry.second->m_node1 != unset_id && m_node_mapping.find(node_entry.second->m_node1) == m_node_mapping.end()) {
            iac_log(error, "node1 linked to tr not registered\n");
            return false;
        }
        if (node_entry.second->m_node2 != unset_id && m_node_mapping.find(node_entry.second->m_node2) == m_node_mapping.end()) {
            iac_log(error, "node2 linked to tr not registered\n");
            return false;
        }
    }

    return true;
}

void LocalNode::print_network() {
    IAC_PRINT_PROVIDER("network state for #%d @ %p\n", m_id, this);
    for (auto& entry : m_node_mapping) {
        auto& node_entry = entry.second;
        auto& routes = node_entry->m_routes;
        auto& endpoints = node_entry->m_endpoints;

        IAC_PRINT_PROVIDER("  |\n  +-- listing for node 0x%x\n", node_entry.element().id());

        IAC_PRINT_PROVIDER("  |  +-- local endpoints @ node:\n");
        for (auto& ep_id : endpoints) {
            auto& ep_entry = m_ep_mapping[ep_id];
            IAC_PRINT_PROVIDER("  |  |  +-- ep id: 0x%x name: '%s'\n", ep_entry.element().id(), ep_entry.element().name().c_str());
        }

        IAC_PRINT_PROVIDER("  |  +-- connections from node:\n");
        for (auto& tr_id : routes) {
            auto& tr_entry = m_tr_mapping[tr_id];
            IAC_PRINT_PROVIDER("  |  |  +-- tr id: 0x%x from: %d type: '%s' info: '%s'\n",
                               tr_entry.element().id(),
                               entry.first == tr_entry->m_node1 ? tr_entry->m_node2 : tr_entry->m_node1,
                               tr_entry.element().typestring().c_str(),
                               tr_entry.element().infostring().c_str());
        }
    }
}

string LocalNode::network_representation(bool include_local_trs) {
    string output;

    for (auto& node : m_node_mapping) {
        output += to_string(node.first) + (node.second->m_local ? "[_local]" : "[remote]") + ":";
        output += "eps[ ";
        for (auto& ep_id : node.second->m_endpoints)
            output += to_string(ep_id) + " ";

        output += "] trs[ ";
        for (auto& tr_id : node.second->m_routes)
            output += to_string(tr_id) + " ";

        if (include_local_trs) {
            output += "] l_trs[ ";
            for (auto& tr_id : node.second->m_local_routes)
                output += to_string(tr_id.first) + "#" + to_string(tr_id.second) + " ";
        }

        output += "] ";
    }

    return output;
}

}  // namespace iac