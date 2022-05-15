#pragma once

#include <thread>

#include "ftest/test_logging.hpp"
#include "iac.hpp"
#include "test_utilities.hpp"

class TestSendReceive {
   public:
    static TestLogging::test_result_t run() {
        int rec_pkg_count = 0;

        TEST_UTILS_CREATE_NODE_WITH_ENDPOINT(node1, ep1, "ep1", 1);
        TEST_UTILS_CREATE_NODE_WITH_ENDPOINT(node2, ep2, "ep2", 2);

        ep1.add_package_handler(0, pkg_handler, &rec_pkg_count);
        ep2.add_package_handler(0, pkg_handler, &rec_pkg_count);

        TEST_UTILS_CONNECT_NODES_WITH_LOOPBACK(tr1, node1, node2);

        TestUtilities::update_til_connected([] {}, node1, node2);

        if (!node1.send(ep1, ep2.id(), 0, nullptr, 0)) {
            return {"failed to send pkg to ep2\n"};
        }

        while (rec_pkg_count < 1)
            TestUtilities::update_all_nodes(node1, node2);

        if (!node2.send(ep2, ep1.id(), 0, nullptr, 0)) {
            return {"failed to send pkg to ep1\n"};
        }

        while (rec_pkg_count < 2)
            TestUtilities::update_all_nodes(node1, node2);

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