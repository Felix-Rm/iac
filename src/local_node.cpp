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

    IAC_HANDLE_FATAL_EXCEPTION(OutOfTrIdException, "no more available ids for transport-routes");
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
    return close_route(&route) && m_network.remove_route(route.id());
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

bool LocalNode::update() {
    if (m_network.endpoint_mapping().empty()) {
        IAC_HANDLE_EXCEPTION(NoRegisteredEndpointsException, "updating node with no endpoints");
        return false;
    }

    for (auto it = m_network.route_mapping().begin(); it != m_network.route_mapping().end();) {
        if (!it->second->local()) {
            it++;
            continue;
        }

        auto* route = (LocalTransportRoute*)(it->second.element_ptr());

        it++;  // iterator might get invalidated in handle_connect_package, so we have to increment now;

        if (!state_handling(route)) return false;
    }

    if (m_network.is_modified()) {
        m_network.reset_modified();
        for (const auto& route_entry : m_network.route_mapping()) {
            if (!route_entry.second->local()) continue;
            auto* route = (LocalTransportRoute*)route_entry.second.element_ptr();
            if (route->state() != LocalTransportRoute::route_state::CONNECTED) continue;
            send_network_update(route);
        }
    }

    return true;
}

bool LocalNode::read_from(LocalTransportRoute* route) {
    for (size_t i = 0; i < s_num_package_reads_from_route_per_update && route->connection().available() > 0; i++) {
        Package package;
        if (package.read_from(route)) {
            if (!handle_package(package)) return false;
            route->meta().last_package_in = timestamp::now();
        } else {
            break;
        }
    }
    return true;
}

bool LocalNode::send_package(const Package& package) {
    if (!m_network.endpoint_registered(package.to())) {
        iac_log_from_node(Logging::loglevels::error, "asked to send package for unregistered endpoint %d, dropping package\n", package.to());
        return false;
    }

    const auto& ep = m_network.endpoint(package.to());

    const auto& available_routes = m_network.node(ep.node()).local_routes();
    if (available_routes.empty())
        return false;

    auto route = (LocalTransportRoute*)&m_network.route(best_local_route(available_routes).first);

    return send_package(package, route);
}

bool LocalNode::send_package(const Package& package, LocalTransportRoute* route) {
    if (route->state() == LocalTransportRoute::route_state::INITIALIZED || route->state() == LocalTransportRoute::route_state::CLOSED) {
        iac_log_from_node(Logging::loglevels::warning, "asked to send package with type %d to %d over route in state %d, dropping package\n", package.type(), package.to(), route->state());
        return false;
    }

    route->meta().last_package_out = timestamp::now();
    return package.send_over(route);
}

}  // namespace iac