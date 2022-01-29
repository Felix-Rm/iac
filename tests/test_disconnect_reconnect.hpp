#pragma once

#include <chrono>
#include <exception>
#include <thread>

#include "ftest/test_logging.hpp"
#include "iac.hpp"
#include "std_provider/utility.hpp"

class TestDisconnectReconnect {
   public:
    const std::string test_name = "disconnect/reconnect";

    TestLogging::test_result_t run() {
        using namespace std::chrono_literals;

        int rec_pkg_count = 0;

        iac::LocalNode node1, node2;
        iac::LocalEndpoint ep1{1, "ep1"}, ep2{2, "ep2"};

        ep1.add_package_handler(0, pkg_handler, &rec_pkg_count);
        ep2.add_package_handler(0, pkg_handler, &rec_pkg_count);

        node1.add_local_endpoint(ep1);
        node2.add_local_endpoint(ep2);

        iac::LoopbackTransportRoutePackage tr;
        tr.connect(node1, node2);
        try {
            for (int i = 0; i < 5; ++i) {
                while (!node1.is_endpoint_connected(2) || !node2.is_endpoint_connected(1)) {
                    node1.update();
                    std::this_thread::sleep_for(100ms);
                    node2.update();
                }

                node1.print_network();
                node2.print_network();

                while (node1.is_endpoint_connected(2)) {
                    node1.update();
                }

                std::this_thread::sleep_for(200ms);
            }
        } catch (std::exception& e) {
            return {e.what()};
        }

        TestLogging::test_printf("node1 %s", node1.network_representation(false).c_str());
        TestLogging::test_printf("node2 %s", node2.network_representation(false).c_str());

        return {};
    };

   private:
    static void pkg_handler(const iac::Package& pkg, iac::BufferReader&& /*unused*/, void* counter) {
        TestLogging::test_printf("received package from %d", pkg.from());
        (*(int*)counter)++;
    };
};