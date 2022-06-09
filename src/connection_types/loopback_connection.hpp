#pragma once

#include "../local_node.hpp"
#include "../std_provider/queue.hpp"
#include "connection.hpp"

namespace iac {

class LoopbackConnection : public Connection {
   public:
    typedef queue<uint8_t> queue_t;

    LoopbackConnection(queue_t* write_queue, queue_t* read_queue);

    size_t read(void* buffer, size_t size) override;
    size_t write(const void* buffer, size_t size) override;

    bool flush() override;
    bool clear() override;

    size_t available() override;

    bool open() override;
    bool close() override;

    void printBuffer(int cols = 4);

   private:
    queue_t* m_write_queue = nullptr;
    queue_t* m_read_queue = nullptr;
};

}  // namespace iac