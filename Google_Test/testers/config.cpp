//
// Created by yuanlu on 2022/11/8.
//


#include "gtest/gtest.h"
#include "config/config.h"
//#include "logger/logger.hpp"
//
TEST(CONFIG, basic) {
    struct Point {
        int x, y;
    } point{};
    static ifr::Config::ConfigInfo<Point> info = {
            [](auto *a, auto &w) {
                w.StartObject();
                w.Key("x"), w.Int(a->x);
                w.Key("y"), w.Int(a->y);
                w.EndObject();
            },
            [](auto *a, auto &d) {
                a->x = d["x"].GetInt();
                a->y = d["y"].GetInt();
            }
    };
    auto cc = ifr::Config::createConfig("xy", &point, info);
    point.x = (int) time(nullptr);
    point.y = point.x + 1;
    ifr::logger::log("Test", "set value: " + std::to_string(point.x) + ", " + std::to_string(point.y));
    cc.save();
    ifr::logger::log("Test", "save value");
    cc.load();
    ifr::logger::log("Test", "read value: " + std::to_string(point.x) + ", " + std::to_string(point.y));
    point.x = (int) time(nullptr) + 100;
    point.y = point.x + 1;
    ifr::logger::log("Test", "reset value: " + std::to_string(point.x) + ", " + std::to_string(point.y));
    cc.load();
    ifr::logger::log("Test", "no save and read value: " + std::to_string(point.x) + ", " + std::to_string(point.y));
}

