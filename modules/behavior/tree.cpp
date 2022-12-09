//
// Created by yuanlu on 2022/12/9.
//

#include "tree.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <memory>

namespace BehaviorTree {
    /**节点构造器的数据*/
    struct NodeBuilder {
        std::string description;//节点描述json
        std::function<std::shared_ptr<Node>(json_in)> builder;
    };
    std::unordered_map<std::string, NodeBuilder> nodeBuilders;//所有的node构造器
    std::recursive_mutex mtx;

    std::string node_description_cache;//节点描述json缓存

    void init() {
        static bool inited = false;
        if (inited)return;
        inited = true;
        Sequence::registerNode();
        DecoratorNot::registerNode();
        DecoratorLoop::registerNode();
    }

    std::shared_ptr<Root> readBehaviorTreeFile(const std::string &file) {

        std::ifstream fin(file);
        if (fin.is_open()) {
            rapidjson::Document d;
            rapidjson::IStreamWrapper isw(fin);
            d.ParseStream(isw);
            return readBehaviorTree(d);
        } else return {};
    }

    std::shared_ptr<Root> readBehaviorTree(const std::string &json) {
        rapidjson::Document d;
        d.Parse(json);
        return readBehaviorTree(d);
    }


    std::shared_ptr<Root> readBehaviorTree(const rapidjson::Value &json) {
        return std::make_shared<Root>(readBehaviorNode(json));
    }

    std::shared_ptr<Node> readBehaviorNode(json_in json) {
        if (!json.IsObject())throw std::invalid_argument("[Behavior Tree]Can not parse Behavior Tree");
        const std::string type = json["type"].GetString();
        const auto args = json["args"].GetObj();
        std::unique_lock<decltype(mtx)> lock(mtx);
        init();
        if (!nodeBuilders.count(type)) throw std::invalid_argument("[Behavior Tree]Cannot find a node named: " + type);
        return nodeBuilders[type].builder(args);
    }

    void registerNode(const std::string &name, const std::string &description,
                      const std::map<std::string, ArgDescription> &args,
                      const std::function<std::shared_ptr<Node>(json_in)> &builder) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> w(sb);
        w.StartObject();
        w.Key("description"), w.String(description);
        w.Key("args"), w.StartObject();
        for (const auto &e: args) {
            w.Key(e.first);
            w.StartObject();
            w.Key("description"), w.String(e.second.description);
            w.Key("type"), w.Uint(e.second.type);
            w.EndObject();
        }
        w.EndObject();
        w.Flush();

        std::unique_lock<decltype(mtx)> lock(mtx);
        node_description_cache = "";
        nodeBuilders.insert(std::pair<std::string, NodeBuilder>(name, {sb.GetString(), builder}));
    }

    std::string getNodesDescription() {
        std::unique_lock<decltype(mtx)> lock(mtx);
        init();
        if (node_description_cache.empty()) {
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);

            w.StartObject();
            for (const auto &nb: nodeBuilders)
                w.Key(nb.first), w.RawValue(nb.second.description.c_str(), nb.second.description.size(),
                                            rapidjson::Type::kObjectType);
            w.EndObject();

            w.Flush();
            node_description_cache = sb.GetString();
        }
        return node_description_cache;
    }

    void Sequence::registerNode() {
        std::map<std::string, BehaviorTree::ArgDescription> args;
        args["name"] = {BehaviorTree::ArgType::STR, "节点名称"};
        args["nodes"] = {BehaviorTree::ArgType::NODE_LIST, "子节点序列"};
        BehaviorTree::registerNode("Sequence", "序列节点, 顺序执行子节点", args, [](json_in json) {
            std::vector<sp> nodes_;
            for (const auto &node: json["nodes"].GetArray()) {
                nodes_.push_back(readBehaviorNode(node));
            }
            return std::make_shared<Sequence>(json["name"].GetString(), nodes_);
        });
    }

    void DecoratorNot::registerNode() {
        std::map<std::string, BehaviorTree::ArgDescription> args;
        args["name"] = {BehaviorTree::ArgType::STR, "节点名称"};
        args["node"] = {BehaviorTree::ArgType::NODE, "子节点"};
        BehaviorTree::registerNode("DecoratorNot", "取反节点, 对子节点的结果取反", args, [](json_in json) {
            return std::make_shared<DecoratorNot>(json["name"].GetString(), readBehaviorNode(json["node"]));
        });
    }

    void DecoratorLoop::registerNode() {
        std::map<std::string, BehaviorTree::ArgDescription> args;
        args["name"] = {BehaviorTree::ArgType::STR, "节点名称"};
        args["loop"] = {BehaviorTree::ArgType::NUMBER, "循环次数, 小于等于0为无限循环"};
        args["node"] = {BehaviorTree::ArgType::NODE, "子节点"};
        BehaviorTree::registerNode("DecoratorLoop", "循环节点, 循环执行子节点指定次数", args, [](json_in json) {
            return std::make_shared<DecoratorLoop>(json["name"].GetString(), json["loop"].GetInt64(),
                                                   readBehaviorNode(json["node"]));
        });
    }
}