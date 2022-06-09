#pragma once

#include "local_node.hpp"
#include "local_transport_route.hpp"

namespace iac {

template <typename LoopbackConnectionType>
class LoopbackConnectionPackage {
   public:
    LocalTransportRoutePackage<LoopbackConnectionType>& end1() {
        return loopback_1;
    };

    LocalTransportRoutePackage<LoopbackConnectionType>& end2() {
        return loopback_2;
    };

    bool connect(LocalNode& a, LocalNode& b) {
        return a.add_local_transport_route(end1()) && b.add_local_transport_route(end2());
    };

   private:
    typename LoopbackConnectionType::queue_t loopback_queue1{}, loopback_queue2{};

    LocalTransportRoutePackage<LoopbackConnectionType> loopback_1{&loopback_queue1, &loopback_queue2};
    LocalTransportRoutePackage<LoopbackConnectionType> loopback_2{&loopback_queue2, &loopback_queue1};
};
}  // namespace iac
