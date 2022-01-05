#pragma once

#include <cstdio>
#include <string>
#include <vector>

class TestLogging {
   public:
    typedef struct test_result {
        std::string test_name{};
        std::string fail_reason{};
        bool passed;

        test_result() : passed(true){};
        test_result(std::string reason) : fail_reason(reason), passed(false){};
    } test_result_t;

    typedef struct suite {
        std::string name{};
        int tests_passed{}, tests_failed{};
        std::vector<test_result_t> fails;
    } suite_t;

    static std::vector<suite_t> s_suites;
    static std::string s_current_test_name;

    static void start_suite(std::string name) {
        s_suites.push_back({});

        s_suites.back().name = name;
    }

    static void test_start(std::string name) {
        s_current_test_name = name;
        printf("\n\n%s::%s \e[33mSTART\e[0m\n", s_suites.back().name.c_str(), s_current_test_name.c_str());
    }

    static void test_pass() {
        s_suites.back().tests_passed++;
        printf("\n%s::%s \e[32mPASSED\e[0m\n", s_suites.back().name.c_str(), s_current_test_name.c_str());
    }

    static void test_failed(test_result_t result) {
        s_suites.back().tests_failed++;
        s_suites.back().fails.push_back(result);
        printf("\n%s::%s \e[31mFAILED [%s]\e[0m\n", s_suites.back().name.c_str(), s_current_test_name.c_str(), result.fail_reason.c_str());
    }

    template <typename... Args>
    static void test_printf(Args... args) {
        printf("\t");
        printf(args...);
        printf("\n");
    }

    static void results() {
        for (auto& suite : s_suites) {
            printf("\n\nsummary for %s: %s (%d/%d passed; %d/%d failed)\n", suite.name.c_str(), suite.tests_failed == 0 ? "\e[32mPASSED\e[0m" : "\e[31mFAILED\e[0m", suite.tests_passed, suite.tests_passed + suite.tests_failed, suite.tests_failed, suite.tests_passed + suite.tests_failed);
            for (auto& fail_reason : suite.fails)
                printf("\t\e[31m%s::%s failed\e[0m - reason: %s\n", suite.name.c_str(), fail_reason.test_name.c_str(), fail_reason.fail_reason.c_str());
        }
    }

    template <typename T, typename... Args>
    static void run(Args... args) {
        T test_instance{args...};

        test_start(test_instance.test_name);
        test_result_t res = test_instance.run();

        if (res.passed) {
            test_pass();
        } else {
            res.test_name = s_current_test_name;
            test_failed(res);
        }
    }
};
