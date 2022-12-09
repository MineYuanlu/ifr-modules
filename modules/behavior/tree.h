//
// Created by yuanlu on 2022/12/9.
//

#ifndef IFR_OPENCV_BEHAVIOR_TREE_H
#define IFR_OPENCV_BEHAVIOR_TREE_H

#include <utility>
#include <random>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <map>
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "logger/logger.hpp"


namespace BehaviorTree {
    using json_in = const rapidjson::Value &;
    using stop_tester = const std::function<bool()> &;
    enum ArgType {
        STR, BOOL, NUMBER, NODE, NODE_LIST
    };

    struct ArgDescription {
        ArgType type;
        std::string description;
    };

    /**代表节点*/
    class Node;

    /**根节点*/
    class Root;

    /**顺序节点*/
    class Sequence;

    /**取反节点*/
    class DecoratorNot;

    /**循环节点*/
    class DecoratorLoop;

    /**条件节点*/
    class Condition;

    /**操作节点*/
    class Action;

    class Node {
    protected:
        std::string name;
    public :
        explicit Node(std::string name) : name(std::move(name)) {}

        /**
         * 单次运行
         * @param is_stop 是否需要尽快退出执行
         * @return 运行是否成功
         */
        virtual bool run(stop_tester is_stop) { return false; }

    };

    class Root : public Node {
    private:
        using sp = std::shared_ptr<Node>;
        sp node;//子节点

    public:

        explicit Root() : Node("root") {}

        explicit Root(sp node) : Node("root"), node(std::move(node)) {}

        void set(const sp &node_) { node = node_; }

        void set(Node *const node_) { node = sp(node_); }

        sp getNode() { return node; }

        bool run(stop_tester is_stop) override { return !(is_stop()) && node->run(is_stop); }

        /**
         * 单次运行, 仅在结束后退出
         * @param stop 是否需要尽快退出执行
         * @return 运行是否成功
         */
        bool run() { return run([]() { return false; }); }
    };

    class Sequence : public Node {
    private:
        using sp = std::shared_ptr<Node>;
        std::vector<sp> nodes;//子节点序列
    public:
        explicit Sequence(const std::string &name) : Node(name) {}

        Sequence(const std::string &name, std::vector<sp> nodes) : Node(name), nodes(std::move(nodes)) {}

        void add(const std::shared_ptr<Node> &node) { nodes.push_back(node); }

        void add(Node *const node) { nodes.emplace_back(node); }

        const std::vector<sp> &getNodes() { return nodes; }

        bool run(stop_tester is_stop) override {
            return std::all_of(nodes.begin(), nodes.end(), [&is_stop](const auto &node) {
                return !(is_stop()) && node->run(is_stop);
            });
        }

        static void registerNode();
    };


    /**取反节点*/
    class DecoratorNot : public Node {
    private:
        using sp = std::shared_ptr<Node>;
        sp node;//子节点
    public:
        explicit DecoratorNot(const std::string &name) : Node(name) {}

        DecoratorNot(const std::string &name, sp node) : Node(name), node(std::move(node)) {}

        void set(const sp &node_) { node = node_; }

        void set(Node *const node_) { node = sp(node_); }

        sp getNode() { return node; }

        bool run(stop_tester is_stop) override { return !node->run(is_stop); }

        static void registerNode();
    };

    class DecoratorLoop : public Node {
    private:
        using sp = std::shared_ptr<Node>;
        uint64_t loop;//循环次数
        sp node;//子节点
    public:
        explicit DecoratorLoop(const std::string &name, uint64_t loop) : Node(name), loop(loop) {}

        DecoratorLoop(const std::string &name, int loop, sp node) :
                Node(name), loop(loop), node(std::move(node)) {}

        void set(const sp &node_) { node = node_; }

        void set(Node *const node_) { node = sp(node_); }

        sp getNode() { return node; }

        bool run(stop_tester is_stop) override {
            if (loop <= 0) { while (true) if (is_stop() || !node->run(is_stop))return false; }
            else { for (uint64_t i = 0; i < loop; i++) if (is_stop() || !node->run(is_stop))return false; }
            return true;
        }

        static void registerNode();
    };

    class Condition : public Node {
    public:
        explicit Condition(const std::string &name) : Node(name) {}
    };

    class Action : public Node {
    public:
        explicit Action(const std::string &name) : Node(name) {}
    };

    /**
     * 读取(反序列化)一个节点
     * @param file 文件路径
     * @return 根节点
     */
    std::shared_ptr<Root> readBehaviorTreeFile(const std::string &file);

    /**
     * 读取(反序列化)一个节点
     * @param json json数据
     * @return 根节点
     */
    std::shared_ptr<Root> readBehaviorTree(const std::string &json);

    /**
     * 读取(反序列化)一个行为树
     * @param json json数据
     * @return 根节点
     */
    std::shared_ptr<Root> readBehaviorTree(json_in json);

    /**
     * 读取(反序列化)一个节点
     * @param json json数据
     * @return 节点
     */
    std::shared_ptr<Node> readBehaviorNode(json_in json);


    /**
     * 注册一个节点
     * @param name 节点名称
     * @param description 节点描述
     * @param args 节点参数列表
     * @param builder 节点构造器
     */
    void registerNode(const std::string &name, const std::string &description,
                      const std::map<std::string, ArgDescription> &args,
                      const std::function<std::shared_ptr<Node>(json_in)> &builder);

    /**
     * 获取所有节点的描述
     * @return 描述json
     */
    std::string getNodesDescription();
}
#endif //IFR_OPENCV_BEHAVIOR_TREE_H
