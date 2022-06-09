#include "local_node.hpp"

namespace iac {

bool LocalNode::handle_package(const Package& package) {
    if (package.route()->state() == LocalTransportRoute::route_state::INITIALIZED || package.route()->state() == LocalTransportRoute::route_state::CLOSED) {
        iac_log_from_node(Logging::loglevels::warning, "received package on closed route\n");
        return false;
    }

    if (package.to() == reserved_endpoint_addresses::IAC) {
        if (package.type() == reserved_package_types::CONNECT && package.route()->state() == LocalTransportRoute::route_state::WAIT_CONNECT) {
            if (handle_connect(package)) {
                package.route()->state() = LocalTransportRoute::route_state::SEND_ACK;
                return true;
            }
            return false;
        }

        if (package.type() == reserved_package_types::ACK && package.route()->state() == LocalTransportRoute::route_state::WAIT_ACK) {
            if (handle_ack(package)) {
                package.route()->state() = LocalTransportRoute::route_state::CONNECTED;
                m_network.set_modified();  // force a send of network_update
                return true;
            }
            return false;
        }

        if (package.route()->state() == LocalTransportRoute::route_state::CONNECTED) {
            if (package.type() == reserved_package_types::NETWORK_UPDATE) {
                return handle_network_update(package);
            }

            if (package.type() == reserved_package_types::HEARTBEAT) {
                return handle_heartbeat(package);
            }
        }

        iac_log_from_node(Logging::loglevels::warning, "dropping package for IAC; route_id: %d; type: %d\n", package.route()->id(), package.type());
        return false;
    }
    if (!m_network.endpoint_registered(package.to())) {
        iac_log_from_node(Logging::loglevels::error, "received package for unregistered endpoint %d, dropping package\n", package.to());
        return false;
    }

    const auto& ep = m_network.endpoint(package.to());

    if (ep.local())
        return ((const LocalEndpoint&)ep).handle_package(package);

    return send_package(package);
}

bool LocalNode::handle_connect(const Package& package) {
    BufferReader reader{package.payload(), package.payload_size()};

    auto sender_id = reader.num<node_id_t>();
    auto other_tr_id = reader.num<tr_id_t>();

    package.route()->meta().timings.heartbeat_interval_ms = max_of(reader.num<uint16_t>(), package.route()->meta().timings.heartbeat_interval_ms);
    package.route()->meta().timings.assume_dead_after_ms = max_of(reader.num<uint16_t>(), package.route()->meta().timings.assume_dead_after_ms);

    IAC_LOG_PACKAGE_RECEIVE_WITH_INFO(Logging::loglevels::network, "connect", "from %d", sender_id);

    if (!m_network.node_registered(sender_id)) {
        m_network.add_node(ManagedNetworkEntry<Node>::create_and_adopt(new Node(sender_id)));

        if (other_tr_id < package.route()->id()) {
            iac_log_from_node(Logging::loglevels::debug, "changing route id\n");

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

            auto temp = iac::move(m_network.mutable_route_managed_entry(old_id));
            m_network.erase_route_managed_entry(old_id);

            erase_tr_links(package.route()->nodes().first);
            erase_tr_links(package.route()->nodes().second);

            package.route()->set_id(other_tr_id);
            m_network.add_route(iac::move(temp));

            insert_tr_links(package.route()->node1());
            insert_tr_links(package.route()->node2());
        }

        package.route()->set_node2(sender_id);
        auto& sender = m_network.mutable_node(sender_id);
        sender.add_route(package.route()->id());
        sender.add_local_route({package.route()->id(), 1});

        iac_log_from_node(Logging::loglevels::connect, "connecting %d to %d\n", id(), sender_id);

        iac_log_from_node(Logging::loglevels::verbose, "connect with timing: %d %d\n", package.route()->meta().timings.heartbeat_interval_ms, package.route()->meta().timings.assume_dead_after_ms);
    }

    return true;
}

bool LocalNode::handle_heartbeat(const Package& package) {
    auto now = timestamp::now();

    IAC_LOG_PACKAGE_RECEIVE_WITH_INFO(Logging::loglevels::verbose, "heartbeat", "with timing: last_out: %d; last_in:%d",
                                      now - package.route()->meta().last_package_in,
                                      now - package.route()->meta().last_package_out);
    return true;
}

bool LocalNode::handle_ack(const Package& package) {
    IAC_LOG_PACKAGE_RECEIVE(Logging::loglevels::network, "ack");
    return true;
}

bool LocalNode::handle_network_update(const Package& package) {
    BufferReader reader{package.payload(), package.payload_size()};

    IAC_LOG_PACKAGE_RECEIVE(Logging::loglevels::network, "network_update");

    if (package.route()->state() != LocalTransportRoute::route_state::CONNECTED) {
        iac_log_from_node(Logging::loglevels::debug, "route is not connected, discarding relay package\n");
        return true;
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

        auto& reachable_node_entry = m_network.node(reachable_node);
        auto res = reachable_node_entry.local_routes().find(package.route()->id());

        if (res == reachable_node_entry.local_routes().end() || res->second > num_hops + 1) {
            m_network.mutable_node(reachable_node).add_local_route(package.route()->id(), num_hops + 1);
        }
    }

    return send_heartbeat(package.route());
}

}  // namespace iac
