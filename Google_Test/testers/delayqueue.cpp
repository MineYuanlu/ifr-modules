//
// Created by yuanlu on 2022/11/30.
//

#include "gtest/gtest.h"
#include "delayqueue/delayqueue.h"
#include "tools/tools.hpp"
#include <cassert>
#include <future>
#include <iostream>
#include <memory>
#include <string>

template<typename T>
void dump(const T &v, std::chrono::system_clock::time_point epoch) {
    auto now = std::chrono::system_clock::now();
    auto elapsed = duration_cast<std::chrono::milliseconds>(now - epoch).count();
    std::cout << elapsed << "ms: " << v << std::endl;
}

TEST(DELAY_QUEUE, basic) {
    static const auto loop = 100;
    auto base = std::chrono::system_clock::now();
    delay_queue<std::unique_ptr<std::string>> q;
    auto t = std::thread([&q, &base]() {
        using namespace std::chrono_literals;
        for (int i = 0; i < loop; i++)
            q.push(std::make_unique<std::string>("str " + std::to_string(i) + " ms"),
                   base + std::chrono::milliseconds(i));
        SLEEP(SLEEP_TIME(1));
        q.close();
        ASSERT_THROW(q.push(std::make_unique<std::string>("bad"), base), closed_delay_queue);
    });

    ASSERT_NO_THROW(dump("start", base));
    for (int i = 0; i < loop; i++)ASSERT_NO_THROW(dump(*q.pop(), base));
    ASSERT_THROW(dump(*q.pop(), base), closed_delay_queue);

    while (!t.joinable());
    t.join();
}

TEST(DELAY_QUEUE, slow) {
    auto base = std::chrono::system_clock::now();
    delay_queue<std::unique_ptr<std::string>> q;
    auto t = std::thread([&q, &base]() {
        using namespace std::chrono_literals;
        q.push(std::make_unique<std::string>("str 2"), base + 2s);
        q.push(std::make_unique<std::string>("str 1"), base + 1s);
        q.push(std::make_unique<std::string>("str 3"), base + 3s);
        SLEEP(SLEEP_TIME(3.1));
        q.close();
        ASSERT_THROW(q.push(std::make_unique<std::string>("bad"), base), closed_delay_queue);
    });

    ASSERT_NO_THROW(dump("start", base));
    for (int i = 0; i < 3; i++)dump(*q.pop(), base);
    ASSERT_THROW(dump(*q.pop(), base), closed_delay_queue);

    while (!t.joinable());
    t.join();
}

TEST(DELAY_QUEUE, fast) {
    auto base = std::chrono::system_clock::now();
    delay_queue<std::unique_ptr<std::string>> q;
    auto t = std::thread([&q, &base]() {
        using namespace std::chrono_literals;
        q.push(std::make_unique<std::string>("str 2"), base);
        q.push(std::make_unique<std::string>("str 1"), base);
        q.push(std::make_unique<std::string>("str 3"), base);
        SLEEP(SLEEP_TIME(3.1));
        q.close();
        ASSERT_THROW(q.push(std::make_unique<std::string>("bad"), base), closed_delay_queue);
    });

    ASSERT_NO_THROW(dump("start", base));
    for (int i = 0; i < 3; i++)dump(*q.pop(), base);
    ASSERT_THROW(dump(*q.pop(), base), closed_delay_queue);

    while (!t.joinable());
    t.join();
}
