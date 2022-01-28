#pragma once

#include <chrono>
#include <iostream>
#include <thread>

#include "forward.hpp"
#include "ftest/test_logging.hpp"
#include "iac.hpp"

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

        TestLogging::test_printf("node1 network rep on startup %s", node1.network_representation().c_str());

        while (!node1.are_endpoints_connected(2, 3, 4) ||
               !node2.are_endpoints_connected(1, 3, 4) ||
               !node3.are_endpoints_connected(1, 2, 4) ||
               !node4.are_endpoints_connected(1, 2, 3)) {
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

        iac::Visualization viz{"127.0.0.1", 3000};
        viz.add_node("node1", node1);
        viz.add_node("node2", node2);
        viz.add_node("node3", node3);
        viz.add_node("node4", node4);

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

   private:
    bool test_networks_equal(std::vector<iac::LocalNode*> nodes) {
        for (auto node : nodes) {
            for (auto compare_node : nodes) {
                if (node == compare_node) continue;

                for (auto& entry : node->node_mapping()) {
                    auto node_res = compare_node->node_mapping().find(entry.first);
                    if (node_res == compare_node->node_mapping().end())
                        return false;

                    for (auto ep_id : entry.second.element().endpoints()) {
                        auto ep_res = node_res->second.element().endpoints().find(ep_id);
                        if (ep_res == node_res->second.element().endpoints().end())
                            return false;
                    }

                    for (auto tr_id : entry.second.element().routes()) {
                        auto tr_res = node_res->second.element().routes().find(tr_id);
                        if (tr_res == node_res->second.element().routes().end())
                            return false;
                    }
                }

                for (auto& entry : node->ep_mapping()) {
                    auto ep_res = compare_node->ep_mapping().find(entry.first);
                    if (ep_res == compare_node->ep_mapping().end()) return false;
                    if (entry.second.element().node() != ep_res->second.element().node())
                        return false;
                }

                for (auto& entry : node->tr_mapping()) {
                    auto tr_res = compare_node->tr_mapping().find(entry.first);
                    if (tr_res == compare_node->tr_mapping().end()) return false;

                    auto a_min_node = std::min(entry.second.element().node1(), entry.second.element().node2());
                    auto a_max_node = std::max(entry.second.element().node1(), entry.second.element().node2());

                    auto b_min_node = std::min(tr_res->second.element().node1(), tr_res->second.element().node2());
                    auto b_max_node = std::max(tr_res->second.element().node1(), tr_res->second.element().node2());

                    if (a_min_node != b_min_node)
                        return false;
                    if (a_max_node != b_max_node)
                        return false;
                }
            }
        }

        return true;
    }
};