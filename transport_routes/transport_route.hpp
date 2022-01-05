#pragma once

#include "../forward.hpp"
#include "../logging.hpp"
#include "../network_types.hpp"
#include "../std_provider/printf.hpp"
#include "../std_provider/queue.hpp"
#include "../std_provider/string.hpp"

namespace iac {

#ifndef IAC_DISABLE_EXCEPTIONS
class TransportRouteException : public std::exception {
   public:
    TransportRouteException(const char* reason) : m_reason(reason){};

    const char* what() const noexcept override {
        return m_reason;
    }

   private:
    const char* m_reason;
};
#endif

class TransportRoute {
    friend LocalNode;
    friend LocalEndpoint;
    friend LocalTransportRoute;
    friend ManagedNetworkEntry<TransportRoute>;

   public:
    string typestring() const { return m_typestring; };
    string infostring() const { return m_infostring; };

    tr_id_t id() const { return m_id; };
    bool local() const { return m_local; };

    node_id_t node1() const { return m_node1; };
    node_id_t node2() const { return m_node2; };

    void printInfo() {
        iac_log(network, "[%s]", infostring().c_str());
    };

    void printTypestring() {
        iac_log(network, "'%s'", typestring().c_str());
    }

   private:
    TransportRoute() = default;
    TransportRoute(id_t id, node_id_t node1, node_id_t node2) : m_id(id), m_node1(node1), m_node2(node2){};
    ~TransportRoute() = default;

    tr_id_t m_id;
    bool m_local = false;

    string m_typestring, m_infostring;
    node_id_t m_node1{unset_id}, m_node2{unset_id};
};

class LocalTransportRoute : public TransportRoute {
   public:
    friend Endpoint;
    friend LocalNode;
    friend Package;
    friend Visualization;

    enum class route_state {
        INIT,
        OPEN,
        TESTING,
        WORKING,
        CLOSED
    };
    typedef route_state route_state_t;
    typedef uint32_t timestamp_t;

    LocalTransportRoute() { m_local = true; };
    virtual ~LocalTransportRoute() = default;

   protected:
    virtual size_t read(void* buffer, size_t size) = 0;
    virtual size_t write(const void* buffer, size_t size) = 0;

    virtual bool flush() = 0;
    virtual bool clear() = 0;

    virtual size_t available() = 0;

    virtual bool open() = 0;
    virtual bool close() = 0;

    size_t read_put_back_queue(void*& buffer, size_t& size);
    void clear_put_back_queue();
    size_t available_put_back_queue();

    void put_back(const void* buffer, size_t size);

    timestamp_t m_last_package_in = 0;
    timestamp_t m_last_package_out = 0;

    size_t m_wait_for_available_size = 0;
    route_state_t m_state = route_state::INIT;

    uint16_t m_heartbeat_interval_ms, m_assume_dead_after_ms;

   private:
    queue<uint8_t> m_put_back_queue;
};
}  // namespace iac
