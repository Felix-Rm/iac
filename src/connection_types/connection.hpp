#pragma once

#include "../std_provider/queue.hpp"
#include "../std_provider/utility.hpp"

namespace iac {

class Connection {
   public:
    virtual size_t read(void* buffer, size_t size) = 0;
    virtual size_t write(const void* buffer, size_t size) = 0;

    virtual bool flush() = 0;
    virtual bool clear() = 0;

    virtual size_t available() = 0;

    virtual bool open() = 0;
    virtual bool close() = 0;

    void put_back(const void* buffer, size_t size);

   protected:
    size_t read_put_back_queue(void*& buffer, size_t& size);
    void clear_put_back_queue();
    size_t available_put_back_queue();

   private:
    queue<uint8_t> m_put_back_queue;
};

}  // namespace iac