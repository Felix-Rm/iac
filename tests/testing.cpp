#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "iac.hpp"
#include "logging.hpp"
#include "test_disconnect_reconnect.hpp"
#include "test_logging.hpp"
#include "test_lw_list.hpp"
#include "test_lw_pair.hpp"
#include "test_lw_queue.hpp"
#include "test_lw_unordered_map.hpp"
#include "test_lw_unordered_set.hpp"
#include "test_lw_vector.hpp"
#include "test_network_building.hpp"
#include "test_network_visualization.hpp"
#include "test_send_receive.hpp"

int main() {
    iac::Logging::loglevel = iac::Logging::loglevels::network;

    TestLogging::start_suite("lw_std");

    TestLogging::run<TestLwVector>();
    TestLogging::run<TestLwList>();
    TestLogging::run<TestLwPair>();
    TestLogging::run<TestLwQueue>();
    TestLogging::run<TestLwUnorderedSet>();
    TestLogging::run<TestLwUnorderedMap>();

    TestLogging::start_suite("communication");

    TestLogging::run<TestDisconnectReconnect>();
    TestLogging::run<TestSendReceive>();
    TestLogging::run<TestNetworkBuilding>();

    // TestLogging::start_suite("visualization");
    // TestLogging::run<TestNetworkVisualization>();

    TestLogging::results();

    return 0;
}