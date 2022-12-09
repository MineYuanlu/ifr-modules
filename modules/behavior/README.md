# Behavior Tree

> 简单行为树

一个简单的行为树框架, 用于控制robot行为

## 内置节点

- Node 节点(abstract), 是所有行为树节点的父类
- Root 根节点, 一颗行为树只能存在一个根节点, 不会保存在json中, 由read方法构造
- Sequence 序列节点, 顺序执行子节点
- DecoratorNot 取反节点, 对子节点的结果取反
- DecoratorLoop 循环节点, 循环执行子节点指定次数
- Condition 条件节点(abstract), 所有判断都应继承此类
- Action 操作节点(abstract), 所有操作都应继承此类

## 方法介绍

读取:

```cpp
//行为树json文件
std::shared_ptr<Root> readBehaviorTreeFile(const std::string &file);
//行为树json字符串
std::shared_ptr<Root> readBehaviorTree(const std::string &json);
//行为树json对象
std::shared_ptr<Root> readBehaviorTree(json_in json);
//仅读取为节点(不用root包装)
std::shared_ptr<Node> readBehaviorNode(json_in json);
```

注册:

```cpp
/**
 * 注册一个节点
 * @param name 节点名称
 * @param description 节点描述
 * @param args 节点参数列表
 * @param builder 节点构造器
 */
void registerNode(const std::string &name,
const std::string &description,
const std::map<std::string, ArgDescription> &args,
const std::function<std::shared_ptr<Node>(json_in)> &builder);
```

描述:

```cpp
/**
 * 获取所有节点的描述
 * @return 描述json
 */
std::string getNodesDescription();
```

## 自定义节点

以下代码实现了一个向控制台打印字符串的操作节点

```cpp
/**打印节点*/
class ActionSpeak :
public BehaviorTree::Action {
private:
const std::string speak;//输出内容

public :
ActionSpeak(const std::string &name, std::string speak) : BehaviorTree::Action(name), speak(std::move(speak)) {
}

bool run(BehaviorTree::stop_tester is_stop) override {
if (is_stop())return false;
ifr::logger::log("BT", "Action Speak", speak);
return true;
}

/**
 * 注册此节点
 */
static void registerNode() {
std::map<std::string, BehaviorTree::ArgDescription> args;
args["name"] = { BehaviorTree::ArgType::STR, "节点名称" };
args["speak"] = { BehaviorTree::ArgType::STR, "显示内容" };
BehaviorTree::registerNode("speak", "打印节点, 向控制台说一句话", args,[](BehaviorTree::json_in json) {
return std::make_shared<ActionSpeak>(json["name"].GetString(), json["speak"].GetString());
});
}
};
```

## 使用方法

配合API模块及前端使用