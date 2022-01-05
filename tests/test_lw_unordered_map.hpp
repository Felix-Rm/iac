#pragma once

#include <thread>

#include "lw_std/unordered_map.hpp"
#include "test_logging.hpp"

class TestLwUnorderedMap {
   public:
    const std::string test_name = "unordered_map";

    TestLogging::test_result_t run() {
        lw_std::unordered_map<double, int> m;

        m[0.5] = 1;
        m[1] = 8;

        if (m.size() != 2) return {"incorrect size of map"};

        if (m[0.5] != 1) return {"error reading value from map"};

        m[0.5] = 8;

        if (m[0.5] != 8) return {"error reading value from map"};
        if (m.size() != 2) return {"incorrect size of map"};

        m.erase(1);

        if (m.size() != 1) return {"incorrect size of map"};

        return {};
    };
};