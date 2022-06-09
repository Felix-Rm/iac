#pragma once

#include "../std_provider/queue.hpp"
#include "loopback_connection.hpp"

namespace iac {

class LatentLoopbackConnection : public LoopbackConnection {
   public:
    LatentLoopbackConnection(queue_t* write_queue, queue_t* read_queue)
        : LoopbackConnection(write_queue, read_queue){};

    size_t available() override {
        static constexpr int fluctuation = 10;

        auto available = LoopbackConnection::available();
        if (available > m_last_available) {
            m_latent_bytes = min_of(available, m_latent_bytes + rand() % fluctuation);
        }

        size_t decrease_by = rand() % fluctuation / 2;
        m_latent_bytes -= min_of(m_latent_bytes, decrease_by);
        m_last_available = available;

        return LoopbackConnection::available() - m_latent_bytes;
    };

   private:
    size_t m_latent_bytes = 0;
    size_t m_last_available = 0;
};

}  // namespace iac