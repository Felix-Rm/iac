#pragma once

#include <chrono>
#include <iostream>
#include <thread>

#include "forward.hpp"
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
        iac::LocalNode node1, node2, node3, node4;
        iac::LocalEndpoint ep1{1, "ep1"}, ep2{2, "ep2"}, ep3{3, "ep3"}, ep4{4, "ep4"};

        node1.add_local_endpoint(ep1);
        node2.add_local_endpoint(ep2);
        node3.add_local_endpoint(ep3);
        node4.add_local_endpoint(ep4);

        iac::LoopbackTransportRoutePackage tr1, tr2, tr3, tr4;
        tr1.connect(node1, node2);
        tr2.connect(node2, node3);
        tr3.connect(node2, node4);
        tr4.connect(node3, node4);

        TestLogging::test_printf("node1 network rep on startup %s", node1.network().network_representation().c_str());

        while (!node1.all_routes_connected() || !node2.all_routes_connected() ||
               !node3.all_routes_connected() || !node4.all_routes_connected()) {
            node1.update();
            node2.update();
            node3.update();
            node4.update();
        }

        for (int i = 0; i < 1; ++i) {
            node1.update();
            node2.update();
            node3.update();
            node4.update();
        }

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