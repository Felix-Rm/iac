#pragma once

#include <utility>

#include "forward.hpp"
#include "logging.hpp"
#include "network_types.hpp"
#include "std_provider/printf.hpp"
#include "std_provider/queue.hpp"
#include "std_provider/string.hpp"
#include "std_provider/utility.hpp"

namespace iac {

class LocalTransportRoute : public TransportRoute {
   public:
    enum class route_state {
        INITIALIZED,
        OPEN,
        WAITING_FOR_HANDSHAKE_REPLY,
        CONNECTED,
        CLOSED
    };

    typedef route_state route_state_t;
    typedef uint32_t timestamp_t;

    LocalTransportRoute();

    virtual size_t read(void* buffer, size_t size) = 0;
    virtual size_t write(const void* buffer, size_t size) = 0;

    virtual bool flush() = 0;
    virtual bool clear() = 0;

    virtual size_t available() = 0;

    virtual bool open() = 0;
    virtual bool close() = 0;

    void put_back(const void* buffer, size_t size);

    route_state_t& state() {
        return m_state;
    };

    route_state_t state() const {
        return m_state;
    };

    // FIXME: This should really be stored in Package or Node
    auto& meta() {
        return m_meta;
    };

   protected:
    size_t read_put_back_queue(void*& buffer, size_t& size);
    void clear_put_back_queue();
    size_t available_put_back_queue();

   private:
    queue<uint8_t> m_put_back_queue;

    typedef struct route_meta {
        timestamp_t last_package_in = 0;
        timestamp_t last_package_out = 0;

        size_t wait_for_available_size = 0;
        route_timings_t timings;
    } route_meta_t;

    route_meta_t m_meta{};
    route_state_t m_state = route_state::INITIALIZED;
};
}  // namespace iac
