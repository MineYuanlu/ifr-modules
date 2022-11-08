//
// Created by yuanlu on 2022/11/8.
//

#include "gtest/gtest.h"
#include "plan/Plans.h"
#include "logger/logger.hpp"

TEST(PLAN, basic) {
    ifr::Plans::registerTask("task-1", {
            "test group", "some info"
    }, [](auto io, auto args, auto state, auto cb) {

    });
    ifr::logger::log("task", "Descriptions", ifr::Plans::getTaskDescriptionsJson());
}