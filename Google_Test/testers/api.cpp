//
// Created by yuanlu on 2022/11/9.
//


#include "gtest/gtest.h"
#include "api/API.h"
#include "logger/logger.hpp"

TEST(API, time_watcher) {
    auto tw = ifr::API::registerTimePoint("test", 1, 2, 2);
    ifr::logger::log("API", tw->getTime());
    tw->start(0), tw->start(1);
    tw->setTime(0, 0, 1);
    tw->setTime(1, 0, 2);
    tw->setTime(0, 1, 1);
    tw->setTime(1, 1, 3);
    tw->start(0), tw->start(1);
    ifr::logger::log("API", tw->getTime());

    tw->setTime(0, 0, 3);
    tw->setTime(1, 0, 5);
    tw->setTime(0, 1, 3);
    tw->setTime(1, 1, 9);

    ifr::logger::log("API", tw->getTime());
    tw->start(0), tw->start(1);
    ifr::logger::log("API", tw->getTime());
}
