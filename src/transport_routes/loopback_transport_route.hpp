#pragma once

#include <cstdio>

#include "local_node.hpp"
#include "local_transport_route.hpp"

namespace iac {

class LoopbackTransportRoute : public LocalTransportRoute {
   public:
    typedef queue<uint8_t> queue_t;

    LoopbackTransportRoute(queue_t* write_queue, queue_t* read_queue);

    size_t read(void* buffer, size_t size) override;
    size_t write(const void* buffer, size_t size) override;

    bool flush() override;
    bool clear() override;

    size_t available() override;

    bool open() override;
    bool close() override;

    void printBuffer(int cols = 4);

   private:
    queue_t* m_write_queue = nullptr;
    queue_t* m_read_queue = nullptr;
};

class LoopbackTransportRoutePackage {
   public:
    LoopbackTransportRoute& end1() { return loopback_1; };
    LoopbackTransportRoute& end2() { return loopback_2; };

    bool connect(LocalNode& a, LocalNode& b) {
        return a.add_local_transport_route(end1()) && b.add_local_transport_route(end2());
    };

   private:
    LoopbackTransportRoute::queue_t loopback_queue1{}, loopback_queue2{};

    LoopbackTransportRoute loopback_1{&loopback_queue1, &loopback_queue2};
    LoopbackTransportRoute loopback_2{&loopback_queue2, &loopback_queue1};
};
}  // namespace iac