#pragma once

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <utility>

#include "exceptions.hpp"
#include "forward.hpp"
#include "local_endpoint.hpp"
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
#include "transport_routes/local_transport_route.hpp"

#ifndef ARDUINO
#    include <chrono>
#endif

namespace iac {

IAC_MAKE_EXCEPTION(OutOfTrIdException);

class LocalNode : public Node {
   public:
    LocalNode(route_timings_t route_timings = {});

    bool endpoint_connected(ep_id_t address) const;
    bool endpoints_connected(const vector<ep_id_t>& addresses) const;

    bool are_endpoints_connected(ep_id_t address) const { return endpoint_connected(address); };
    template <typename... Args>
    bool are_endpoints_connected(ep_id_t address, Args... args) const {
        return endpoint_connected(address) && are_endpoints_connected(args...);
    }

    bool all_routes_connected() const;

    bool update();

    bool add_local_transport_route(LocalTransportRoute& route);
    bool remove_local_transport_route(LocalTransportRoute& route);

    bool add_local_endpoint(LocalEndpoint& ep);
    bool remove_local_endpoint(LocalEndpoint& ep);

    bool send(Endpoint& from, ep_id_t to, package_type_t type, const BufferWriter& buffer, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);
    bool send(Endpoint& from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);
    bool send(ep_id_t from, ep_id_t to, package_type_t type, const BufferWriter& buffer, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);
    bool send(ep_id_t from, ep_id_t to, package_type_t type, const uint8_t* buffer, size_t buffer_length, Package::buffer_management_t buffer_management = Package::buffer_management::IN_PLACE);

    const Network& network() const { return m_network; };

   private:
    static constexpr uint16_t s_min_heartbeat_interval_ms = 100;
    static constexpr uint16_t s_min_assume_dead_time = s_min_heartbeat_interval_ms * 2;
    static constexpr uint8_t s_num_package_reads_from_route_per_update = 5;

    route_timings_t m_default_route_timings;

    Network m_network{};

    unordered_set<uint8_t> m_used_tr_ids;

    bool handle_package(Package& package, LocalTransportRoute* route = nullptr);

    bool handle_connect(Package& package);
    bool handle_disconnect(const Package& package);
    bool handle_disconnect(LocalTransportRoute* route);
    bool handle_heartbeat(const Package& package);

    bool read_from(LocalTransportRoute* route);
    bool open_route(LocalTransportRoute* route);
    bool close_route(LocalTransportRoute* route);
    bool send_handshake(LocalTransportRoute* route);
    bool send_heartbeat(LocalTransportRoute* route);

    bool send_connect_package(LocalTransportRoute* route, bool relay);

    uint8_t get_tr_id();
    bool pop_tr_id(uint8_t id);

    static pair<tr_id_t, uint8_t> best_local_route(const unordered_map<tr_id_t, uint8_t>& local_routes);
    static LocalTransportRoute::timestamp_t timestamp() {
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