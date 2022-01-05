#pragma once

#include <thread>

#include "../lw_std/queue.hpp"
#include "test_logging.hpp"

class TestLwQueue {
   public:
    const std::string test_name = "queue";

    TestLogging::test_result_t run() {
        lw_std::queue<double> q;

        q.push(1);
        q.push(1.5);

        if (q.size() != 2) return {"incorrect size of queue"};

        q.pop();
        q.pop();

        if (q.size() != 0) return {"incorrect size of queue"};

        q.pop();

        if (q.size() != 0) return {"incorrect size of queue after poping empty"};

        return {};
    };
};