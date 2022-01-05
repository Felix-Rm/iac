#pragma once

#include <thread>

#include "../lw_std/vector.hpp"
#include "test_logging.hpp"

class TestLwVector {
   public:
    const std::string test_name = "vector";

    TestLogging::test_result_t run() {
        lw_std::vector<int> vec;

        if (vec.size() != 0) return {"initial size not 0"};

        vec.push_back(1);

        if (vec.size() != 1) return {"size wrong on first insert"};

        vec.push_back(2);
        vec.push_back(1);

        if (vec.size() != 3) return {"size wrong on successive inserts"};
        if (vec[0] != 1 || vec[1] != 2 || vec[2] != 1) return {"element position"};

        if (vec.find(1) != vec.begin()) return {"find duplicate element"};
        if (vec.find(2) != vec.begin() + 1) return {"find existing element"};
        if (vec.find(5) != vec.end()) return {"find non existing element"};

        vec.pop_front();
        if (vec.size() != 2) return {"size after pop_front"};
        if (vec[0] != 2 || vec[1] != 1) return {"element position after pop_front"};

        vec.pop_back();
        if (vec.size() != 1) return {"size after pop_back"};
        if (vec[0] != 2) return {"element position after pop_back"};

        vec.erase(vec.begin());
        if (vec.size() != 0) return {"erasing last element"};

        vec.push_back(2);
        vec.push_back(1);

        if (vec.size() != 2) return {"size wrong after (second) successive insert"};

        vec.erase(vec.find(1));

        if (vec.size() != 1) return {"size wrong after erase by find"};
        if (vec[0] != 2) return {"element position erase"};

        return {};
    };
};