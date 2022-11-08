//
// Created by yuanlu on 2022/11/8.
//
#include "gtest/gtest.h"
#include "tools/tools.hpp"
#include "logger/logger.hpp"

TEST(TOOLS, basic) {
    auto x = SLEEP_TIME(1);
    ifr::logger::log("Sleep", "sleep time", x);
    SLEEP(x);
}

TEST(TOOLS, macro) {
    ifr::logger::log("macro", "当前系统: ", __OS__);
    ifr::logger::log("macro", "OS - Win: ", __OS_Windows__);
    ifr::logger::log("macro", "OS - Linux: ", __OS_Linux__);
    ifr::logger::log("macro", "arch", __ARCH_NAME__);
    ifr::logger::log("macro", "__cplusplus", __cplusplus);
}