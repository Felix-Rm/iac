#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "ftest/test_logging.hpp"
#include "iac.hpp"
#include "logging.hpp"
#include "test_disconnect_reconnect.hpp"
#include "test_network_building.hpp"
#include "test_network_visualization.hpp"
#include "test_send_receive.hpp"

int main() {
    iac::Logging::set_loglevel(iac::Logging::loglevels::network);

    TestLogging::start_suite("communication");

    TestLogging::run<TestDisconnectReconnect>();
    TestLogging::run<TestSendReceive>();
    TestLogging::run<TestNetworkBuilding>();

    // TestLogging::start_suite("visualization");
    // TestLogging::run<TestNetworkVisualization>();

    TestLogging::results();

    return 0;
}