#pragma once

#include <stdio.h>
#include <sys/types.h>

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <utility>

#include "endpoint.hpp"
#include "forward.hpp"
#include "logging.hpp"
#include "network_types.hpp"
#include "package.hpp"
#include "std_provider/printf.hpp"
#include "std_provider/string.hpp"
#include "std_provider/unordered_map.hpp"
#include "std_provider/unordered_set.hpp"
#include "std_provider/utility.hpp"
#include "std_provider/vector.hpp"
#include "transport_routes/transport_route.hpp"

#ifndef ARDUINO
#    include <chrono>
#endif

namespace iac {

#ifndef IAC_DISABLE_EXCEPTIONS
IAC_CREATE_MESSAGE_EXCEPTION(OutOfTrIdException);
IAC_CREATE_MESSAGE_EXCEPTION(RemoveOfInvalidException);
IAC_CREATE_MESSAGE_EXCEPTION(NoRegisteredEndpointsException);
IAC_CREATE_MESSAGE_EXCEPTION(AddDuplicateException);
IAC_CREATE_MESSAGE_EXCEPTION(NonExistingException);
IAC_CREATE_MESSAGE_EXCEPTION(NetworkStateException);
#endif

class Node {
    friend LocalNode;
    friend LocalEndpoint;
    friend LocalTransportRoute;
    friend ManagedNetworkEntry<Node>;

   public:
    node_id_t id() const { return m_id; };

    bool local() const { return m_local; };

    const auto& endpoints() const { return m_endpoints; }
    const auto& routes() const { return m_routes; }
    const auto& local_routes() const { return m_local_routes; }

   private:
    Node(node_id_t id) : m_id(id){};
    ~Node() = default;

    node_id_t m_id = unset_id;
    bool m_local = false;

    unordered_set<ep_id_t> m_endpoints{};
    unordered_set<tr_id_t> m_routes{};
    unordered_map<tr_id_t, uint8_t> m_local_routes{};
};

class LocalNode : public Node {
    friend LocalEndpoint;
    friend Visualization;

   public:
    LocalNode(uint16_t heartbeat_interval_ms = 0, uint16_t assume_dead_after_ms = 0);
    ~LocalNode() = default;

    bool is_endpoint_connected(ep_id_t address);
    bool are_endpoints_connected(vector<ep_id_t> addresses);

    bool are_endpoints_connected(ep_id_t address) { return is_endpoint_connected(address); };
    template <typename... Args>
    bool are_endpoints_connected(ep_id_t address, Args... args) {
        return is_endpoint_connected(address) && are_endpoints_connected(args...);
    }

    bool update();

    bool add_local_transport_route(LocalTransportRoute& route);
    bool remove_local_transport_route(LocalTransportRoute& route);

    bool add_local_endpoint(LocalEndpoint& ep);
    bool remove_local_endpoint(LocalEndpoint& ep);

    bool send(Endpoint& from, ep_id_t to, package_type_t type, const BufferWriter& buffer, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);
    bool send(Endpoint& from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);
    bool send(ep_id_t from, ep_id_t to, package_type_t type, const BufferWriter& buffer, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);
    bool send(ep_id_t from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);

    void print_endpoint_list();
    void print_network();

    string network_representation(bool include_local_trs = true);

    typedef unordered_map<ep_id_t, ManagedNetworkEntry<Endpoint>> ep_mapping_t;
    typedef unordered_map<tr_id_t, ManagedNetworkEntry<TransportRoute>> tr_mapping_t;
    typedef unordered_map<node_id_t, ManagedNetworkEntry<Node>> node_mapping_t;

    const ep_mapping_t& ep_mapping() const { return m_ep_mapping; };
    const tr_mapping_t& tr_mapping() const { return m_tr_mapping; };
    const node_mapping_t& node_mapping() const { return m_node_mapping; };

   private:
    uint16_t m_connection_keep_alive_pkg_every_ms;
    uint16_t m_connection_dead_after_ms;

    ep_mapping_t m_ep_mapping;
    tr_mapping_t m_tr_mapping;
    node_mapping_t m_node_mapping;
    bool m_mapping_changed = false;

    unordered_set<uint8_t> m_used_tr_ids;

    bool add_transport_route(TransportRoute& route);
    bool remove_transport_route(tr_id_t route_id);

    bool add_endpoint(Endpoint& ep);
    bool remove_endpoint(ep_id_t ep_id);

    bool add_node(Node& node);
    bool remove_node(node_id_t node_id);

    std::pair<bool, ManagedNetworkEntry<Node>&> add_node_if_not_existing(node_id_t node_id);
    std::pair<bool, ManagedNetworkEntry<Endpoint>&> add_ep_if_not_existing(ep_id_t ep_id, string name, node_id_t node);
    std::pair<bool, ManagedNetworkEntry<TransportRoute>&> add_tr_if_not_existing(tr_id_t tr_id, node_id_t node1, node_id_t node2);

    bool handle_package(Package& package, LocalTransportRoute* route = nullptr);

    bool handle_connect(Package& package);
    bool handle_disconnect(const Package& package);
    bool handle_disconnect(LocalTransportRoute* route);
    bool handle_heartbeat(const Package& package);

    bool route_try_read(LocalTransportRoute* route);
    bool route_open(LocalTransportRoute* route);
    bool route_close(LocalTransportRoute* route);
    bool route_test(LocalTransportRoute* route);
    bool route_keep_alive(LocalTransportRoute* route);

    bool send_connect_package(LocalTransportRoute* route, bool relay = true);

    bool validate_network();

    pair<tr_id_t, uint8_t> best_local_route(unordered_map<tr_id_t, uint8_t>& local_routes);

    uint8_t get_tr_id();
    bool pop_tr_id(uint8_t id);

    LocalTransportRoute::timestamp_t timestamp() {
#ifdef ARDUINO
        return millis();
#else
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif
    }

    template <typename T, typename U>
    void delete_mapping(T mapping, LocalNode* obj, bool (LocalNode::*remove_func)(U)) {
        for (auto it = mapping.begin(); it != mapping.end();) {
            U id = it->first;
            it++;

            (obj->*remove_func)(id);
        }
    };
};

}  // namespace iac