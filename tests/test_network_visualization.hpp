#pragma once

#include <chrono>
#include <iostream>
#include <thread>

#include "ftest/test_logging.hpp"
#include "iac.hpp"
#include "test_utilities.hpp"

class TestNetworkVisualization {
   public:
    static void run_viz(bool* run, iac::Visualization* viz) {
        using namespace std::chrono_literals;

        while (*run) {
            viz->update();
            std::this_thread::sleep_for(10ms);
        }
    }

    const std::string test_name = "network_visualization";

    TestLogging::test_result_t run() {
        TEST_UTILS_CREATE_NODE_WITH_ENDPOINT(node1, ep1, "ep1", 1);
        TEST_UTILS_CREATE_NODE_WITH_ENDPOINT(node2, ep2, "ep2", 2);
        TEST_UTILS_CREATE_NODE_WITH_ENDPOINT(node3, ep3, "ep3", 3);
        TEST_UTILS_CREATE_NODE_WITH_ENDPOINT(node4, ep4, "ep4", 4);

        TEST_UTILS_CONNECT_NODES_WITH_LOOPBACK(tr1, node1, node2);
        TEST_UTILS_CONNECT_NODES_WITH_LOOPBACK(tr2, node2, node3);
        TEST_UTILS_CONNECT_NODES_WITH_LOOPBACK(tr3, node2, node4);
        TEST_UTILS_CONNECT_NODES_WITH_LOOPBACK(tr4, node3, node4);

        TestLogging::test_printf("node1 network rep on startup %s", node1.network().network_representation().c_str());

        TestUtilities::update_til_connected([] {}, node1, node2, node3, node4);

        for (int i = 0; i < 5; ++i)
            TestUtilities::update_all_nodes(node1, node2, node3, node4);

        bool networks_equal = TestUtilities::test_networks_equal({&node1, &node2, &node3, &node4});

        if (!networks_equal) {
            TestLogging::test_printf("node1 %s", node1.network().network_representation(false).c_str());
            TestLogging::test_printf("node2 %s", node2.network().network_representation(false).c_str());
            TestLogging::test_printf("node3 %s", node3.network().network_representation(false).c_str());
            TestLogging::test_printf("node4 %s", node4.network().network_representation(false).c_str());
        }

        iac::Visualization viz{"127.0.0.1", 3000};
        viz.add_network("node1", node1.network());
        viz.add_network("node2", node2.network());
        viz.add_network("node3", node3.network());
        viz.add_network("node4", node4.network());

        bool run = true;
        auto t = std::thread(run_viz, &run, &viz);

        printf("Visualization up on http://127.0.0.1:3000\npress any key to continue...");

        char c;
        std::cin >> c;

        printf("...continuing\n");

        run = false;
        t.join();

        return {};
    };
};
