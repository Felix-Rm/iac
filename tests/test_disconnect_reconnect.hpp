#pragma once

#include <chrono>
#include <exception>
#include <thread>

#include "ftest/test_logging.hpp"
#include "iac.hpp"
#include "std_provider/utility.hpp"
#include "test_utilities.hpp"

class TestDisconnectReconnect {
   public:
    static TestLogging::test_result_t run() {
        using namespace std::chrono_literals;

        int rec_pkg_count = 0;

        TEST_UTILS_CREATE_NODE_WITH_ENDPOINT(node1, ep1, "ep1", 1);
        TEST_UTILS_CREATE_NODE_WITH_ENDPOINT(node2, ep2, "ep2", 2);

        ep1.add_package_handler(0, pkg_handler, &rec_pkg_count);
        ep2.add_package_handler(0, pkg_handler, &rec_pkg_count);

        TEST_UTILS_CONNECT_NODES_WITH_LOOPBACK(tr1, node1, node2);

        try {
            for (int i = 0; i < 2; ++i) {
                TestUtilities::update_til_connected([] {}, node1, node2);

                node1.network().print_network();
                node2.network().print_network();

                while (node1.endpoint_connected(2)) {
                    node1.update();
                }

                std::this_thread::sleep_for(200ms);
            }
        } catch (std::exception& e) {
            return {e.what()};
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