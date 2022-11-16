//
// Created by yuanlu on 2022/11/16.
//

#include "gtest/gtest.h"
#include "data-waiter/DataWaiter.h"
#include "tools/tools.hpp"
#include "logger/logger.hpp"

struct Infos {
    int64_t id;
    int64_t delay;
    std::string data;
};
const Infos infos[] = {{1, 3, "aaa"},
                       {2, 1, "bbb"},
                       {3, 4, "ccc"},
                       {9, 2, "ddd"}};

TEST(data_waiter, basic) {
    ifr::DataWaiter<int64_t, std::string> dw;
    std::vector<std::thread> threads;
    threads.emplace_back(std::thread([&dw]() {
        for (size_t i = 0, len = sizeof(infos) / sizeof(Infos); i < len; i++) {
            const auto data = dw.pop();
            ifr::logger::log("DW", "pop", std::to_string(data.first) + "->" + data.second);
        }
    }));
    for (size_t i = 0, len = sizeof(infos) / sizeof(Infos); i < len; i++) {
        threads.emplace_back(std::thread([i, &dw]() {
            const auto info = infos[i];
            ifr::logger::log("DW", "start", info.id);
            dw.start(info.id);
            SLEEP(SLEEP_TIME(info.delay));
            ifr::logger::log("DW", "end", info.id);
            dw.finish(info.id, info.data);
        }));
    }
    for (auto &t: threads) {
        while (!t.joinable());
        t.join();
    }
}

TEST(data_waiter, pop_for) {
    ifr::DataWaiter<int64_t, std::string> dw;
    std::vector<std::thread> threads;
    threads.emplace_back(std::thread([&dw]() {
        size_t i = 0, len = sizeof(infos) / sizeof(Infos);
        while (true) {
            try {
                const auto data = dw.pop_for(500);
                ifr::logger::log("DW", "pop_for", std::to_string(data.first) + "->" + data.second);
                if (++i >= len)break;
            } catch (ifr::DataWaiter_Timeout &) {
                ifr::logger::log("DW", "pop_for", "Timeout");
            }
        }
    }));
    for (size_t i = 0, len = sizeof(infos) / sizeof(Infos); i < len; i++) {
        threads.emplace_back(std::thread([i, &dw]() {
            const auto info = infos[i];
            ifr::logger::log("DW", "start", info.id);
            dw.start(info.id);
            SLEEP(SLEEP_TIME(info.delay));
            ifr::logger::log("DW", "end", info.id);
            dw.finish(info.id, info.data);
        }));
    }
    for (auto &t: threads) {
        while (!t.joinable());
        t.join();
    }
}