#pragma once

#include "exceptions.hpp"
#include "forward.hpp"
#include "logging.hpp"
#include "network_types.hpp"
#include "std_provider/limits.hpp"
#include "std_provider/printf.hpp"
#include "std_provider/unordered_map.hpp"
#include "std_provider/unordered_set.hpp"
#include "std_provider/utility.hpp"

namespace iac {

class Network {
    friend LocalNode;

   public:
    typedef unordered_map<ep_id_t, ManagedNetworkEntry<Endpoint>> ep_mapping_t;
    typedef unordered_map<tr_id_t, ManagedNetworkEntry<TransportRoute>> tr_mapping_t;
    typedef unordered_map<node_id_t, ManagedNetworkEntry<Node>> node_mapping_t;

    const auto& node_mapping() const {
        return m_node_mapping;
    };

    const auto& endpoint_mapping() const {
        return m_ep_mapping;
    };

    const auto& route_mapping() const {
        return m_tr_mapping;
    };

    [[nodiscard]] bool node_registered(node_id_t node_id) const;
    [[nodiscard]] bool endpoint_registered(ep_id_t ep_id) const;
    [[nodiscard]] bool route_registered(tr_id_t tr_id) const;

    [[nodiscard]] const Node& node(node_id_t node_id) const {
        return m_node_mapping.at(node_id).element();
    };

    [[nodiscard]] const Endpoint& endpoint(ep_id_t ep_id) const {
        return m_ep_mapping.at(ep_id).element();
    };

    [[nodiscard]] const TransportRoute& route(tr_id_t tr_id) const {
        return m_tr_mapping.at(tr_id).element();
    };

    void print_endpoint_list() const;
    void print_network() const;

    [[nodiscard]] string network_representation(bool include_local_trs = true) const;

   protected:
    [[nodiscard]] Node& mutable_node(node_id_t node_id) {
        set_modified();
        return m_node_mapping.at(node_id).element();
    };

    [[nodiscard]] Endpoint& mutable_endpoint(ep_id_t ep_id) {
        set_modified();
        return m_ep_mapping.at(ep_id).element();
    };

    [[nodiscard]] TransportRoute& mutable_route(tr_id_t tr_id) {
        set_modified();
        return m_tr_mapping.at(tr_id).element();
    };

    [[nodiscard]] auto& mutable_node_managed_entry(node_id_t node_id) {
        set_modified();
        return m_node_mapping.at(node_id);
    };

    [[nodiscard]] auto& mutable_endpoint_managed_entry(ep_id_t ep_id) {
        set_modified();
        return m_ep_mapping.at(ep_id);
    };

    [[nodiscard]] auto& mutable_route_managed_entry(tr_id_t tr_id) {
        set_modified();
        return m_tr_mapping.at(tr_id);
    };

    void erase_node_managed_entry(node_id_t node_id) {
        m_node_mapping.erase(node_id);
    };

    void erase_endpoint_managed_entry(ep_id_t ep_id) {
        m_ep_mapping.erase(ep_id);
    };

    void erase_route_managed_entry(tr_id_t tr_id) {
        m_tr_mapping.erase(tr_id);
    };

    bool add_route(ManagedNetworkEntry<TransportRoute>&& route);
    bool remove_route(tr_id_t route_id);

    bool add_endpoint(ManagedNetworkEntry<Endpoint>&& ep);
    bool remove_endpoint(ep_id_t ep_id);

    bool add_node(ManagedNetworkEntry<Node>&& node);
    bool remove_node(node_id_t node_id);

    bool disconnect_route(tr_id_t route_id) {
        return true;
    };

    bool validate_network() const;

    bool is_modified() const {
        return m_mapping_changed;
    };

    void reset_modified() {
        m_mapping_changed = false;
    };

   private:
    void set_modified() {
        m_mapping_changed = true;
    };

    ep_mapping_t m_ep_mapping;
    tr_mapping_t m_tr_mapping;
    node_mapping_t m_node_mapping;
    bool m_mapping_changed = false;
};

}  // namespace iac