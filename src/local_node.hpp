#pragma once

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <utility>

#include "exceptions.hpp"
#include "forward.hpp"
#include "local_endpoint.hpp"
#include "local_transport_route.hpp"
#include "logging.hpp"
#include "network.hpp"
#include "network_types.hpp"
#include "package.hpp"
#include "std_provider/printf.hpp"
#include "std_provider/string.hpp"
#include "std_provider/unordered_map.hpp"
#include "std_provider/unordered_set.hpp"
#include "std_provider/utility.hpp"
#include "std_provider/vector.hpp"

namespace iac {

IAC_MAKE_EXCEPTION(OutOfTrIdException);

#define IAC_LOG_PACKAGE_SEND(level, type) \
    iac_log_from_node(level, "sending " type " package to %d over %d\n", m_network.route(route->id()).node2(), route->id());

#define IAC_LOG_PACKAGE_RECEIVE_WITH_INFO(level, type, info, ...) \
    iac_log_from_node(level, "received " type " package over %d " info "\n", package.route()->id(), __VA_ARGS__);

#define IAC_LOG_PACKAGE_RECEIVE(level, type) \
    IAC_LOG_PACKAGE_RECEIVE_WITH_INFO(level, type, "", 0);

class LocalNode : public Node {
   public:
    LocalNode(route_timings_t route_timings = {});

    bool endpoint_connected(ep_id_t address) const;
    bool endpoints_connected(const vector<ep_id_t>& addresses) const;

    bool are_endpoints_connected(ep_id_t address) const {
        return endpoint_connected(address);
    };

    template <typename... Args>
    bool are_endpoints_connected(ep_id_t address, Args... args) const {
        return endpoint_connected(address) && are_endpoints_connected(args...);
    }

    bool all_routes_connected() const;

    bool update();

    bool add_local_transport_route(LocalTransportRoute& route);
    bool remove_local_transport_route(LocalTransportRoute& route);

    template <typename ConnectionType>
    bool add_local_transport_route(LocalTransportRoutePackage<ConnectionType>& package) {
        return add_local_transport_route(package.route());
    };

    bool add_local_endpoint(LocalEndpoint& ep);
    bool remove_local_endpoint(LocalEndpoint& ep);

    bool send(Endpoint& from, ep_id_t to, package_type_t type, const BufferWriter& buffer, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);
    bool send(Endpoint& from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);
    bool send(ep_id_t from, ep_id_t to, package_type_t type, const BufferWriter& buffer, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);
    bool send(ep_id_t from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);

    const Network& network() const {
        return m_network;
    };

   private:
    static constexpr uint16_t s_min_heartbeat_interval_ms = 100;
    static constexpr uint16_t s_min_assume_dead_time = s_min_heartbeat_interval_ms * 3;
    static constexpr uint8_t s_num_package_reads_from_route_per_update = 5;

    route_timings_t m_default_route_timings;

    Network m_network{};

    unordered_set<uint8_t> m_used_tr_ids;

    bool send_package(const Package& package);
    bool send_package(const Package& package, LocalTransportRoute* route);
    bool handle_package(const Package& package);

    bool handle_connect(const Package& package);
    bool handle_heartbeat(const Package& package);
    bool handle_ack(const Package& package);
    bool handle_network_update(const Package& package);

    bool read_from(LocalTransportRoute* route);

    bool state_handling(LocalTransportRoute* route);

    bool open_route(LocalTransportRoute* route);
    bool close_route(LocalTransportRoute* route);

    bool send_connect(LocalTransportRoute* route);
    bool send_heartbeat(LocalTransportRoute* route);
    bool send_ack(LocalTransportRoute* route);
    bool send_network_update(LocalTransportRoute* route);

    uint8_t get_tr_id();
    bool pop_tr_id(uint8_t id);

    static pair<tr_id_t, uint8_t> best_local_route(const unordered_map<tr_id_t, uint8_t>& local_routes);
};

}  // namespace iac