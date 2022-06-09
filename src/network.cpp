#include "network.hpp"

namespace iac {

bool Network::add_route(ManagedNetworkEntry<TransportRoute>&& route) {
    IAC_ASSERT(validate_network());

    if (route_registered(route.element().id())) {
        IAC_HANDLE_EXCEPTION(AddDuplicateException, "added already existing route");
        return false;
    }

    auto add_if_not_unset = [&](node_id_t node_id) {
        if (node_id != unset_id && !node_registered(node_id))
            add_node(ManagedNetworkEntry<Node>::create_and_adopt(new Node(node_id)));
    };

    auto link_if_not_unset = [&](tr_id_t tr_id, node_id_t node_id) {
        if (node_id != unset_id) mutable_node(node_id).add_route(tr_id);
    };

    auto tr_id = route.element().id();
    auto nodes = route.element().nodes();

    add_if_not_unset(nodes.first);
    add_if_not_unset(nodes.second);

    m_tr_mapping[tr_id] = iac::move(route);

    link_if_not_unset(tr_id, nodes.first);
    link_if_not_unset(tr_id, nodes.second);

    set_modified();

    IAC_ASSERT(validate_network());

    return true;
}

bool Network::remove_route(tr_id_t route_id) {
    IAC_ASSERT(validate_network());

    const auto& res = m_tr_mapping.find(route_id);

    if (res == m_tr_mapping.end()) {
        IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "removing non existant tr");
        return false;
    }

    auto remove_if_not_unset = [&](tr_id_t tr_id, node_id_t node_id) {
        if (node_id != unset_id) {
            auto node_entry = m_node_mapping.find(node_id);
            if (node_entry == m_node_mapping.end()) {
                IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "linked node was non existant");
                return false;
            }

            node_entry->second->m_routes.erase(tr_id);
            node_entry->second->m_local_routes.erase(tr_id);

            if (node_entry->second->routes().empty())
                if (!remove_node(node_id)) IAC_ASSERT_NOT_REACHED();
        }
        return true;
    };

    if (!remove_if_not_unset(route_id, res->second->nodes().first)) return false;
    if (!remove_if_not_unset(route_id, res->second->nodes().first)) return false;

    m_tr_mapping.erase(res);
    set_modified();

    IAC_ASSERT(validate_network());

    return true;
}

bool Network::add_endpoint(ManagedNetworkEntry<Endpoint>&& ep) {
    IAC_ASSERT(validate_network());

    if (endpoint_registered(ep.element().id())) {
        IAC_HANDLE_EXCEPTION(AddDuplicateException, "added already existing endpoint");
        return false;
    }

    auto ep_id = ep.element().id();
    auto node_id = ep.element().node();

    if (node_id != unset_id) {
        if (!node_registered(node_id))
            add_node(ManagedNetworkEntry<Node>::create_and_adopt(new Node(node_id)));

        mutable_node(node_id).add_endpoint(ep_id);
    }

    m_ep_mapping[ep_id] = iac::move(ep);

    IAC_ASSERT(validate_network());

    return true;
}

bool Network::remove_endpoint(ep_id_t ep_id) {
    IAC_ASSERT(validate_network());

    const auto& res = m_ep_mapping.find(ep_id);

    if (res == m_ep_mapping.end()) {
        IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "removing non existant ep");
        return false;
    }

    auto node_id = res->second.element().node();
    if (node_id != unset_id) {
        auto node_entry = m_node_mapping.find(node_id);
        if (node_entry == m_node_mapping.end()) {
            IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "linked node was non existant");
            return false;
        }

        node_entry->second->remove_endpoint(ep_id);
    }

    m_ep_mapping.erase(res);
    set_modified();

    IAC_ASSERT(validate_network());

    return true;
}

bool Network::add_node(ManagedNetworkEntry<Node>&& node) {
    IAC_ASSERT(validate_network());

    if (m_node_mapping.find(node.element().id()) != m_node_mapping.end()) {
        IAC_HANDLE_EXCEPTION(AddDuplicateException, "added already existing node");
        return false;
    }

    m_node_mapping[node.element().id()] = iac::move(node);
    set_modified();

    IAC_ASSERT(validate_network());

    return true;
}

bool Network::remove_node(node_id_t node_id) {
    IAC_ASSERT(validate_network());

    const auto& res = m_node_mapping.find(node_id);

    if (res == m_node_mapping.end()) {
        IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "removing non existant node");
        return false;
    }

    for (auto it = res->second->endpoints().begin(); it != res->second->endpoints().end();) {
        auto ep_id = *it++;
        if (!remove_endpoint(ep_id)) {
            IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "removing non existant, but linked ep");
            return false;
        }
    }

    for (auto it = res->second->routes().begin(); it != res->second->routes().end();) {
        auto tr_id = *it++;

        if (!route_registered(tr_id)) {
            IAC_HANDLE_EXCEPTION(RemoveOfInvalidException, "removing non existant, but linked tr");
            return false;
        }

        auto& tr_entry = mutable_route(tr_id);
        if (tr_entry.node1() == node_id)
            tr_entry.set_node1(unset_id);
        if (tr_entry.node2() == node_id)
            tr_entry.set_node2(unset_id);

        if (tr_entry.nodes().first == unset_id && tr_entry.nodes().second == unset_id)
            if (!remove_route(tr_id)) IAC_ASSERT_NOT_REACHED();
    }

    m_node_mapping.erase(res);
    set_modified();

    IAC_ASSERT(validate_network());

    return true;
}

bool Network::node_registered(node_id_t node_id) const {
    return m_node_mapping.find(node_id) != m_node_mapping.end();
}

bool Network::endpoint_registered(ep_id_t ep_id) const {
    return m_ep_mapping.find(ep_id) != m_ep_mapping.end();
}

bool Network::route_registered(tr_id_t tr_id) const {
    return m_tr_mapping.find(tr_id) != m_tr_mapping.end();
}

void Network::print_endpoint_list() const {
    for (const auto& entry : m_ep_mapping) {
        const auto& ep_entry = entry.second;
        const auto& routes = m_node_mapping.at(entry.second->m_node)->routes();

        iac_printf("  |\n  +-- %s - routes to%s endpoint #%d (%lu):\n",
                   ep_entry.element().name().c_str(),
                   ep_entry->local() ? " local" : "",
                   ep_entry.element().id(),
                   routes.size());

        for (const auto& route_id : routes) {
            const auto& tr_entry = m_tr_mapping.at(route_id);
            iac_printf("  |   +-- ");
            iac_printf("route #%d of type %s [%s]", tr_entry.element().id(), tr_entry.element().typestring().c_str(), tr_entry.element().infostring().c_str());
            iac_printf("\n");
        }

        iac_printf("\n");
    }
}

bool Network::validate_network() const {
    for (const auto& node_entry : m_node_mapping) {
        for (const auto& ep_entry : node_entry.second->endpoints()) {
            if (!endpoint_registered(ep_entry)) {
                iac_log(Logging::loglevels::error, "ep (%d) linked to node (%d) not registered:\n%s\n", ep_entry, node_entry.first, network_representation(true).c_str());
                return false;
            }
            if (endpoint(ep_entry).node() != node_entry.second->id()) {
                iac_log(Logging::loglevels::error, "ep (%d) linked to node (%d) not linked back:\n%s\n", ep_entry, node_entry.first, network_representation(true).c_str());
                return false;
            }
        }
        for (const auto& tr_entry : node_entry.second->routes()) {
            if (!route_registered(tr_entry)) {
                iac_log(Logging::loglevels::error, "tr (%d) linked to node (%d) not registered:\n%s\n", tr_entry, node_entry.first, network_representation(true).c_str());
                return false;
            }
            if (route(tr_entry).node1() != node_entry.second->id() && route(tr_entry).node2() != node_entry.second->id()) {
                iac_log(Logging::loglevels::error, "tr (%d) linked to node (%d) not linked back:\n%s\n", tr_entry, node_entry.first, network_representation(true).c_str());
                return false;
            }
        }
        for (const auto& tr_entry : node_entry.second->local_routes())
            if (!route_registered(tr_entry.first)) {
                iac_log(Logging::loglevels::error, "tr (%d) linked to node (%d) not registered:\n%s\n", tr_entry.first, node_entry.first, network_representation(true).c_str());
                return false;
            }
    }

    for (const auto& ep_entry : m_ep_mapping) {
        if (!node_registered(ep_entry.second->node())) {
            iac_log(Logging::loglevels::error, "node (%d) linked to ep (%d) not registered:\n%s\n", ep_entry.second->m_node, ep_entry.first, network_representation(true).c_str());
            return false;
        }
    }

    for (const auto& tr_entry : m_tr_mapping) {
        if (tr_entry.second->node1() != unset_id && !node_registered(tr_entry.second->node1())) {
            iac_log(Logging::loglevels::error, "node1 (%d) linked to tr (%d) not registered:\n%s\n", tr_entry.second->node1(), tr_entry.first, network_representation(true).c_str());
            return false;
        }

        if (tr_entry.second->node1() != unset_id && node(tr_entry.second->node1()).routes().find(tr_entry.first) == node(tr_entry.second->node1()).routes().end()) {
            iac_log(Logging::loglevels::error, "node1 (%d) linked to tr (%d) not linked back:\n%s\n", tr_entry.second->node1(), tr_entry.first, network_representation(true).c_str());
            return false;
        }

        if (tr_entry.second->node2() != unset_id && !node_registered(tr_entry.second->node2())) {
            iac_log(Logging::loglevels::error, "node2 (%d) linked to tr (%d) not registered:\n%s\n", tr_entry.second->node2(), tr_entry.first, network_representation(true).c_str());
            return false;
        }

        if (tr_entry.second->node2() != unset_id && node(tr_entry.second->node2()).routes().find(tr_entry.first) == node(tr_entry.second->node2()).routes().end()) {
            iac_log(Logging::loglevels::error, "node2 (%d) linked to tr (%d) not linked back:\n%s\n", tr_entry.second->node1(), tr_entry.first, network_representation(true).c_str());
            return false;
        }
    }

    return true;
}

template <typename It>
static void print_extension_if_needed(It current, It end) {
    if (++current == end) iac_printf("    ");
    else iac_printf("|   ");
}

void Network::print_network() const {
    for (auto node_it = m_node_mapping.begin(); node_it != m_node_mapping.end(); ++node_it) {
        const auto& node_entry = node_it->second;
        const auto& routes = node_entry->m_routes;
        const auto& endpoints = node_entry->m_endpoints;

        iac_printf("+-- listing for node 0x%x\n", node_entry.element().id());

        print_extension_if_needed(node_it, m_node_mapping.end());
        iac_printf("+-- local endpoints @ node:\n");

        for (auto ep_it = endpoints.begin(); ep_it != endpoints.end(); ++ep_it) {
            const auto& ep_entry = m_ep_mapping.at(*ep_it);
            print_extension_if_needed(node_it, m_node_mapping.end());
            iac_printf("|   ");
            iac_printf("+-- ep id: 0x%x name: '%s'\n", ep_entry.element().id(), ep_entry.element().name().c_str());
        }

        print_extension_if_needed(node_it, m_node_mapping.end());
        iac_printf("|   \n");

        print_extension_if_needed(node_it, m_node_mapping.end());
        iac_printf("+-- connections from node:\n");
        for (auto tr_it = routes.begin(); tr_it != routes.end(); ++tr_it) {
            const auto& tr_entry = m_tr_mapping.at(*tr_it);
            print_extension_if_needed(node_it, m_node_mapping.end());
            iac_printf("    ");
            iac_printf("+-- tr id: 0x%x from: %d type: '%s' info: '%s'\n",
                       tr_entry.element().id(),
                       node_it->first == tr_entry->node1() ? tr_entry->node2() : tr_entry->node1(),
                       tr_entry.element().typestring().c_str(),
                       tr_entry.element().infostring().c_str());
        }

        print_extension_if_needed(node_it, m_node_mapping.end());
        iac_printf("\n");
    }
}

string Network::network_representation(bool include_local_trs) const {
    string output;

    for (const auto& node : m_node_mapping) {
        output += to_string(node.first) + (node.second->m_local ? "[_local]" : "[remote]") + ":";
        output += "eps[ ";
        for (const auto& ep_id : node.second->m_endpoints)
            output += to_string(ep_id) + " ";

        output += "] trs[ ";
        for (const auto& tr_id : node.second->m_routes)
            output += to_string(tr_id) + " ";

        if (include_local_trs) {
            output += "] l_trs[ ";
            for (const auto& tr_id : node.second->m_local_routes)
                output += to_string(tr_id.first) + "#" + to_string(tr_id.second) + " ";
        }

        output += "] ";
    }

    return output;
}

}  // namespace iac