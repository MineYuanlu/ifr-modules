//
// Created by yuanlu on 2022/11/5.
//

#include "gtest/gtest.h"
#include <iostream>
#include "msg/msg.hpp"
#include "semaphore"
#include "logger/logger.hpp"
#include "tools/tools.hpp"

using namespace std;
using namespace ifr::logger;

const auto delay = SLEEP_TIME(0.01);
const auto msg_loop = 20;

const string t_pub = "publisher";
const string t_sub = "subscriber";


void test_basic() {

    std::counting_semaphore s1(0);
    std::counting_semaphore s2(0);
    auto t1 = std::thread([&s1, &s2]() {//publisher
        try {
            ifr::Msg::Publisher<std::string> pub("a");
            s1.acquire();
            s2.release();
            pub.lock(true);
            for (int i = 0; i < msg_loop; i++) {
                SLEEP(delay);
                const string data = "Data " + std::to_string(i);
                log(t_pub, "Push", data);
                pub.push(data);
            }
        } catch (std::runtime_error &err) {
            log(t_pub, "Err", err.what());
            ASSERT_TRUE(0) << t_pub << " Err " << err.what() << std::endl;
        }
    });
    auto t2 = std::thread([&s1, &s2]() {
        try {
            ifr::Msg::Subscriber<std::string> sub("a");
            s1.release();
            s2.acquire();
            while (true) {
                const auto &data = sub.pop();
                log(t_sub, "Pop", data);
            }
        } catch (ifr::Msg::MessageError_Broke &e) {
            log(t_sub, "Broke", e.what());
        } catch (ifr::Msg::MessageError_NoMsg &e) {
            log(t_sub, "No Msg", e.what());
        } catch (std::runtime_error &err) {
            log(t_sub, "Err", err.what());
            ASSERT_TRUE(0) << t_sub << " Err " << err.what() << std::endl;
        }
    });
    while (!t1.joinable());
    while (!t2.joinable());
    t1.join();
    t2.join();
}

void test_n_sub(const int n, ifr::Msg::DistributeType type, bool subDelay = false) {
    std::counting_semaphore s1(0);
    std::counting_semaphore s2(0);
    vector<std::thread> ts;
    ts.emplace_back([&s1, &s2, n, &type]() {//publisher
        try {
            ifr::Msg::Publisher<std::string> pub("b", type);
            for (int i = 0; i < n; i++) s1.acquire();
            for (int i = 0; i < n; i++) s2.release();
            pub.lock(true);
            for (int i = 0; i < msg_loop; i++) {
                SLEEP(delay);
                const string data = "Data " + std::to_string(i);
                log(t_pub, "Push", data);
                pub.push(data);
            }
        } catch (std::runtime_error &err) {
            log(t_pub, "Err", err.what());
            ASSERT_TRUE(0) << t_pub << " Err " << err.what() << std::endl;
        }
    });
    for (int i = 0; i < n; i++) {
        ts.emplace_back([&s1, &s2, i, subDelay]() {
            try {
                ifr::Msg::Subscriber<std::string> sub("b");
                s1.release();
                s2.acquire();
                while (true) {
                    const auto &data = sub.pop();
                    log(t_sub, i, "Pop", data);
                    if (subDelay)SLEEP(delay * 2);
                }
            } catch (ifr::Msg::MessageError_Broke &e) {
                log(t_sub, "Broke", e.what());
            } catch (ifr::Msg::MessageError_NoMsg &e) {
                log(t_sub, i, "No Msg", e.what());
            } catch (std::runtime_error &err) {
                log(t_sub, i, "Err", err.what());
                ASSERT_TRUE(0) << t_sub << ' ' << i << " Err " << err.what() << std::endl;
            }
        });
    }
    for (const auto &t: ts) while (!t.joinable());
    for (auto &t: ts) t.join();
}

TEST(MSG, basic) {//????????????
    test_basic();
}

TEST(MSG, no_reg) {//??????????????????
    {
        ifr::Msg::Publisher<std::string> pub;
        ASSERT_THROW(pub.lock(), ifr::Msg::MessageError_BadUse);
    }
    {
        ifr::Msg::Publisher<std::string> pub("a");
        ASSERT_THROW(pub.lock(true), ifr::Msg::MessageError_BadUse);
    }
    {
        ifr::Msg::Publisher<std::string> pub;
        ASSERT_THROW(pub.push("any"), ifr::Msg::MessageError_BadUse);
    }
    {
        ifr::Msg::Subscriber<std::string> sub;
        ASSERT_THROW(sub.pop(), ifr::Msg::MessageError_BadUse);
    }
    {
        ifr::Msg::Subscriber<std::string> sub("a");
        ASSERT_THROW(sub.pop(), ifr::Msg::MessageError_BadUse);
    }
}

TEST(MSG, n_pub) {//??????????????????
    ifr::Msg::Publisher<std::string> pub1("a");

    ASSERT_THROW(ifr::Msg::Publisher<std::string> pub2("a"), ifr::Msg::MessageError_BadUse);
}

TEST(MSG, re_reg) {//???????????? ??????
    ifr::Msg::Publisher<std::string> pub("a");

    ASSERT_THROW(pub.reg("b"), ifr::Msg::MessageError_BadUse);
}

TEST(MSG, no_lock) {//???????????????
    {
        ifr::Msg::Publisher<std::string> pub("a");
        ifr::Msg::Subscriber<std::string> sub("a");

        ASSERT_THROW(pub.push("any"), ifr::Msg::MessageError_BadUse);
    }
    {
        ifr::Msg::Publisher<std::string> pub("a");
        ASSERT_THROW(pub.push("any"), ifr::Msg::MessageError_BadUse);
    }
}

TEST(MSG, reuse_channel) {//??????????????????
    log("logic", "use 1");
    test_basic();
    log("logic", "use 2");
    test_basic();
}

TEST(MSG, n_sub_same) {// ????????? ??? same ??????????????????
    test_n_sub(4, ifr::Msg::DistributeType::same);
}

TEST(MSG, n_sub_rand) {// ????????? ??? rand ??????????????????
    test_n_sub(4, ifr::Msg::DistributeType::rand);
}

TEST(MSG, n_sub_wait_fst) {// ????????? ??? wait_fst ??????????????????
    test_n_sub(4, ifr::Msg::DistributeType::wait_fst, true);
}

TEST(MSG, n_sub_each) {// ????????? ??? each ??????????????????
    test_n_sub(4, ifr::Msg::DistributeType::each);
}


TEST(MSG, pop_for) {//TODO ??????bug????????????:  pub.push ?????????????????? (????????????)

    std::counting_semaphore s1(0);
    std::counting_semaphore s2(0);
    auto t1 = std::thread([&s1, &s2]() {//publisher
        try {
            ifr::Msg::Publisher<std::string> pub("a");
            s1.acquire();
            s2.release();
            pub.lock(true);
            for (int i = 0; i < msg_loop; i++) {
                SLEEP(delay);
                const string data = "Data " + std::to_string(i);
                log(t_pub, "Push", data);
                pub.push(data); //TODO bug
            }
        } catch (std::runtime_error &err) {
            log(t_pub, "Err", err.what());
            ASSERT_TRUE(0) << t_pub << " Err " << err.what() << std::endl;
        }
    });
    auto t2 = std::thread([&s1, &s2]() {
        try {
            ifr::Msg::Subscriber<std::string> sub("a");
            s1.release();
            s2.acquire();
            while (true) {
                const auto &data = sub.pop_for(5);
                log(t_sub, "Pop", data);
            }
        } catch (ifr::Msg::MessageError_Broke &e) {
            log(t_sub, "Broke", e.what());
        } catch (ifr::Msg::MessageError_NoMsg &e) {
            log(t_sub, "No Msg", e.what());
        } catch (std::runtime_error &err) {
            log(t_sub, "Err", err.what());
            ASSERT_TRUE(0) << t_sub << " Err " << err.what() << std::endl;
        }
    });
    while (!t1.joinable());
    while (!t2.joinable());
    t1.join();
    t2.join();
}