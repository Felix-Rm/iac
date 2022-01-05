#pragma once

#include <thread>

#include "lw_std/unordered_set.hpp"
#include "test_logging.hpp"

class TestLwUnorderedSet {
   public:
    const std::string test_name = "unordered_set";

    TestLogging::test_result_t run() {
        lw_std::unordered_set<int> s;

        s.insert(0);
        s.insert(1);

        if (s.size() != 2) return {"incorrect size of set"};

        s.insert(0);

        if (s.size() != 2) return {"incorrect size of set after inserting same element twice"};

        s.erase(0);

        if (s.size() != 1) return {"incorrect size of set after erasing"};

        if (s.find(1) == s.end()) return {"error finding elt in set"};

        return {};
    };
};