#pragma once

#include <thread>

#include "ftest/test_logging.hpp"
#include "iac.hpp"

class TestSendReceive {
   public:
    const std::string test_name = "send/receive";

    TestLogging::test_result_t run() {
        int rec_pkg_count = 0;

        iac::LocalNode node1, node2;
        iac::LocalEndpoint ep1{1, "ep1"}, ep2{2, "ep2"};

        ep1.add_package_handler(0, pkg_handler, &rec_pkg_count);
        ep2.add_package_handler(0, pkg_handler, &rec_pkg_count);

        node1.add_local_endpoint(ep1);
        node2.add_local_endpoint(ep2);

        iac::LoopbackTransportRoutePackage tr;
        tr.connect(node1, node2);

        while (!node1.endpoint_connected(2) || !node2.endpoint_connected(1)) {
            node1.update();
            node2.update();
        }

        if (!node1.send(ep1, ep2.id(), 0, nullptr, 0)) {
            return {"failed to send pkg to ep2\n"};
        }

        while (rec_pkg_count < 1) {
            node1.update();
            node2.update();
        }

        if (!node2.send(ep2, ep1.id(), 0, nullptr, 0)) {
            return {"failed to send pkg to ep1\n"};
        }

        while (rec_pkg_count < 2) {
            node1.update();
            node2.update();
        }

        TestLogging::test_printf("node1 %s", node1.network().network_representation(false).c_str());
        TestLogging::test_printf("node2 %s", node2.network().network_representation(false).c_str());

        return {};
    };

   private:
    static void pkg_handler(const iac::Package& pkg, iac::BufferReader&& /*unused*/, void* counter) {
        TestLogging::test_printf("received package from %d", pkg.from());
        (*(int*)counter)++;
    };
};