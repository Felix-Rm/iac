#pragma once

#include <thread>

#include "../lw_std/list.hpp"
#include "test_logging.hpp"

class TestLwList {
   public:
    const std::string test_name = "list";

    TestLogging::test_result_t run() {
        lw_std::list<int> lst;

        if (lst.size() != 0) return {"initial size not 0"};

        lst.push_back(1);

        if (lst.size() != 1) return {"size wrong on first insert"};

        lst.push_back(2);
        lst.push_back(1);

        if (lst.size() != 3) return {"size wrong on successive inserts"};

        if (lst.find(1) != lst.begin()) return {"find duplicate element"};
        if (lst.find(2) != lst.begin() + 1) return {"find existing element"};
        if (lst.find(5) != lst.end()) return {"find non existing element"};

        lst.pop_front();
        if (lst.size() != 2) return {"size after pop_front"};

        lst.pop_back();
        if (lst.size() != 1) return {"size after pop_back"};

        lst.erase(lst.begin());
        if (lst.size() != 0) return {"erasing last element"};

        lst.push_back(2);
        lst.push_back(1);

        if (lst.size() != 2) return {"size wrong after (second) successive insert"};

        lst.erase(lst.find(1));

        if (lst.size() != 1) return {"size wrong after erase by find"};

        return {};
    };
};