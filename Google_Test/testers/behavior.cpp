//
// Created by yuanlu on 2022/12/9.
//


#include <utility>

#include "gtest/gtest.h"
#include "behavior/tree.h"

/**打印节点*/
class ActionSpeak : public BehaviorTree::Action {
private:
    const std::string speak;//输出内容

public :
    explicit ActionSpeak(BehaviorTree::json_in json) : BehaviorTree::Action(json), speak(json["speak"].GetString()) {}

    ActionSpeak(const std::string &name, std::string speak) : BehaviorTree::Action(name), speak(std::move(speak)) {}

    bool run(BehaviorTree::stop_tester is_stop) override {
        if (is_stop())return false;
        ifr::logger::log("BT", "Action Speak", speak);
        return true;
    }

    /**
     * 注册此节点
     */
    static void registerNode() {
        BehaviorTree::registerNode<ActionSpeak>("speak", "打印节点, 向控制台说一句话");
    }

    static void addArgs(BehaviorTree::args_out args) {
        Node::addArgs(args);
        args["speak"] = {BehaviorTree::ArgType::STR, "显示内容"};
    }
};

void init() {
    static bool inited = false;
    if (inited)return;
    inited = true;
    ActionSpeak::registerNode();
}

TEST(behavior, basic) {
    init();
    std::string json = R"({"type":"speak","args":{"name":"speak node","speak":"something output"}})";
    ifr::logger::log("Behavior tree", "bt json", json);
    auto node = BehaviorTree::readBehaviorTree(json);
    node->run();
}

TEST(behavior, complex) {
    init();
    std::string json = R"({"type":"DecoratorLoop","args":{"name":"loop node","loop":5,"node":{"type":"Sequence","args":{"name":"sequence node","nodes":[{"type":"speak","args":{"name":"speak node 1","speak":"hi~~~ NCUT ifr DreamTeam"}},{"type":"speak","args":{"name":"speak node 2","speak":"i am yuanlu"}}]}}}})";
    ifr::logger::log("Behavior tree", "bt json", json);
    auto node = BehaviorTree::readBehaviorTree(json);
    node->run();
}

TEST(behavior, description) {
    init();
    ifr::logger::log("Behavior tree", "bt nodes", BehaviorTree::getNodesDescription());
}