#pragma once

#include "ftest/test_logging.hpp"
#include "iac.hpp"

#define TEST_UTILS_CREATE_NODE_WITH_ENDPOINT(node_name, ep_name, name, id) \
    iac::LocalNode node_name{};                                            \
    iac::LocalEndpoint ep_name{id, name};                                  \
    node_name.add_local_endpoint(ep_name);

#define TEST_UTILS_CONNECT_NODES_WITH_LOOPBACK(tr_name, node1_name, node2_name) \
    iac::LoopbackTransportRoutePackage tr_name;                                 \
    tr_name.connect(node1_name, node2_name)

class TestUtilities {
   private:
    static bool all_nodes_connected() { return true; };
    static void update_all_nodes(){};

   public:
    template <typename F, typename... Args>
    static void update_til_connected(F after_update, iac::LocalNode& node, Args&... nodes) {
        while (!all_nodes_connected(node, nodes...)) {
            update_all_nodes(node, nodes...);
            after_update();
        }
    };

    template <typename... Args>
    static bool all_nodes_connected(const iac::LocalNode& node, const Args&... nodes) {
        return node.all_routes_connected() && all_nodes_connected(nodes...);
    }

    template <typename... Args>
    static void update_all_nodes(iac::LocalNode& node, Args&... nodes) {
        node.update();
        update_all_nodes(nodes...);
    }

    static bool test_networks_equal(const std::vector<iac::LocalNode*>& nodes) {
        for (const auto* node : nodes) {
            for (const auto* compare_node : nodes) {
                if (node == compare_node) continue;

                for (const auto& entry : node->network().node_mapping()) {
                    auto node_res = compare_node->network().node_mapping().find(entry.first);
                    if (node_res == compare_node->network().node_mapping().end()) {
                        TestLogging::test_printf("node %d in node %d not in compare_node listing %d", entry.first, node->id(), compare_node->id());
                        return false;
                    }

                    const iac::Node& node_result = node_res->second.element();

                    for (auto ep_id : entry.second.element().endpoints()) {
                        auto ep_res = node_result.endpoints().find(ep_id);
                        if (ep_res == node_result.endpoints().end()) {
                            TestLogging::test_printf("ep %d has mismatch in ep between node %d not and compare_node %d", ep_id, node->id(), compare_node->id());
                            return false;
                        }
                    }

                    for (auto tr_id : entry.second.element().routes()) {
                        auto tr_res = node_result.routes().find(tr_id);
                        if (tr_res == node_result.routes().end()) {
                            TestLogging::test_printf("tr %d has mismatch in tr between node %d not and compare_node %d", tr_id, node->id(), compare_node->id());
                            return false;
                        }
                    }
                }

                for (const auto& entry : node->network().endpoint_mapping()) {
                    auto ep_res = compare_node->network().endpoint_mapping().find(entry.first);
                    if (ep_res == compare_node->network().endpoint_mapping().end()) return false;
                    if (entry.second.element().node() != ep_res->second.element().node()) {
                        TestLogging::test_printf("node %d not and compare_node %d have mismatch in ep %d", node->id(), compare_node->id(), entry.first);
                        return false;
                    }
                }

                for (const auto& entry : node->network().route_mapping()) {
                    auto tr_res = compare_node->network().route_mapping().find(entry.first);
                    if (tr_res == compare_node->network().route_mapping().end()) {
                        TestLogging::test_printf("node %d not and compare_node %d have mismatch tr %d", node->id(), compare_node->id(), entry.first);
                        return false;
                    }

                    auto a_min_node = std::min(entry.second.element().node1(), entry.second.element().node2());
                    auto a_max_node = std::max(entry.second.element().node1(), entry.second.element().node2());

                    auto b_min_node = std::min(tr_res->second.element().node1(), tr_res->second.element().node2());
                    auto b_max_node = std::max(tr_res->second.element().node1(), tr_res->second.element().node2());

                    if (a_min_node != b_min_node) {
                        TestLogging::test_printf("node %d not and compare_node %d have mismatch in node %d != %d", node->id(), compare_node->id(), a_min_node, b_min_node);
                        return false;
                    }
                    if (a_max_node != b_max_node) {
                        TestLogging::test_printf("node %d not and compare_node %d have mismatch in node %d", node->id(), compare_node->id(), a_max_node, b_max_node);
                        return false;
                    }
                }
            }
        }

        return true;
    }
};