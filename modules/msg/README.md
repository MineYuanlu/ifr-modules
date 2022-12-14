# MSG

> 数据通讯模块


本模块基于发布、订阅功能，通过频道在多个模块间传递数据。

代码借鉴 [Harry-hhj/UltraMultiThread](https://github.com/Harry-hhj/UltraMultiThread)

## Channel

频道

频道是字符串，是链接发布者与订阅者之间的名称。同一个频道上只能有一个发布者, 但可以有多个订阅者。

## Publisher

发布者

发布者是发布数据的类, 使用前需要注册到一个频道。一个发布者仅能注册一次，且被注册的频道（在此发布者被破坏前）不能再被其它发布者注册。

### 构造 & 注册

```cpp
Publisher() // 空构造
Publisher(const std::string &name, DistributeType type = same) //构造并注册
void reg(const std::string &name, DistributeType type = same) //注册
```

发布者需要先注册才能执行后续步骤。

DistributeType: 分发模式  
当一个频道内有多个订阅者时, 使用发布模式指定数据应该如何分发给订阅者

- `same`: 每个消息都会发给每一个订阅者
- `rand`: 独占模式, 每个消息(平均)随机发给某个订阅者
- `each`: 独占模式, 每个消息循环发给每个订阅者
- `wait_fst`: 独占模式, 寻找等待中的第一个订阅者。如果没有找到, 则退化为rand

### 锁定

```cpp
void lock(bool must = false) //must = true : 必须至少拥有一个订阅者
```

发布者初始时是未锁定状态，此时可以向此频道注册订阅者。
在使用`lock`函数后, (直到被破坏前)不可继续向此频道注册订阅者, 同时可以开始使用发送数据。

### 发送

```cpp
void push(const T &obj)
```

在锁定后, 可以开始向频道发送数据。  
如果频道内没有订阅者, 则此调用函数将不会产生实质性效果。  
可以使用`hasSubscriber()`检测是否有订阅者

### 破坏

当同一个频道的发布者/订阅者有任意一个被回收, 则会破坏整个频道上所有的发布者/订阅者。  
破坏后发布者将无法继续使用, 调用`push()`将不会产生实质性效果, 此时可以以同样的频道名注册新的发布者/订阅者。

此设计目的是回收频道, 当任一对象被回收时, 将会认为此时程序处于清理阶段, 不应再传输数据。  
此时应尽快结束程序。

## Subscriber

订阅者

订阅者是用于接收发布者发布的数据的类, 与发布者一样, 使用前需要注册到一个频道。但同一个频道可以被多个订阅者注册。

### 构造 & 注册

```cpp
Subscriber() //空构造
Subscriber(const std::string &name, size_t maxSize = 1) //构造并注册
reg(const std::string &name, size_t maxSize)//注册
```

订阅者需要先注册才能执行后续步骤。

maxSize: 订阅者的消息队列长度

### 接收

```cpp
T pop() //尝试获取一条消息
T pop_for(size_t ms) //尝试获取一条消息，有超时时间
T pop_until(P pt) //尝试获取一条消息，直到某个时间点超时
```

在发布者注册后, 即可开始获取数据。  
**如果获取时没有发布者, 则会抛出`BadUse`异常, 请注意线程同步**  
在超时/被破坏的情况下, 会抛出`NoMsg`异常, 捕获后做相应处理(如结束程序运行或重新等待)

### 破坏

与发布者一致, 当对象被释放时, 将会破坏整个频道。

# 注意事项

请不要将破坏旧发布者/订阅者 与 注册新的发布者/订阅者的代码同时运行, 否则结果未定义。  
应等待频道内所有发布者/订阅者都被回收之后再进行注册操作。

`test.cpp`内定义了一些GTest的单元测试, 用于测试代码是否有BUG, 修改代码后应运行一遍或多遍单元测试。
