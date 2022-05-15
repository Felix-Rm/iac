#include "ftest/test_logging.hpp"
#include "iac.hpp"
#include "logging.hpp"
#include "test_disconnect_reconnect.hpp"
#include "test_network_building.hpp"
#include "test_send_receive.hpp"

#ifndef IAC_DISABLE_VISUALIZATION
#    include "test_network_visualization.hpp"
#endif

int main(int argc, char* argv[]) {
    iac::Logging::set_loglevel(iac::Logging::loglevels::debug);

    TestLogging::start_suite("communication");

    TestLogging::run("disconnect-reconnect", TestDisconnectReconnect::run);
    TestLogging::run("send-receive", TestSendReceive::run);
    TestLogging::run("network-building", TestNetworkBuilding::run);

#ifndef IAC_DISABLE_VISUALIZATION
    if (argc >= 2 && strncmp(argv[1], "-h", 2) == 0) {
        TestLogging::start_suite("visualization");
        TestLogging::run("visualization", TestNetworkVisualization::run);
    }
#endif

    return TestLogging::results();
}