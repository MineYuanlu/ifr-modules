//
// Created by yuanlu on 2022/11/8.
//

#include "gtest/gtest.h"
#include "logger/logger.hpp"

TEST(LOGGER, basic) {
    ifr::logger::log("type", "sub", "some str");
    ifr::logger::log("type", "some str");
    ifr::logger::log("type", 1, "some str");
    ifr::logger::log("type", 1, "sub", "some str");
}

TEST(LOGGER, n_thread) {
    std::vector<std::thread> ts;
    for (int i = 0; i < 10; i++) {
        ts.emplace_back(std::thread([i]() {
            for (int j = 0; j < 30; j++) {
                ifr::logger::log("thread", i, "loop", j);
            }
        }));
    }
    for (const auto &x: ts)while (!x.joinable());
    for (auto &x: ts)x.join();

}