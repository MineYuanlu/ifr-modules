# PLANS

> 流程定义模块

此模块为程序运行框架, 以每个算法模块为基本单元(称为Task), 使用流程(称为Plan)来组织起来所有Task,
使其工作。不同的Task之间使用`msg`模块通讯

Plans模块应配合`API`模块使用, 达到前端调参的目的。

## Task

任务, 是此框架下的基本单元, 每一个功能被认为是一个模块(例如相机输入模块、图像处理模块、运动预测模块、串口输出模块等)

每个Task可以分为2部分: 描述(Description)和数据(Info)

### Description

描述部分由每个模块定义, 其中包含了Task的名称、分组、描述、接口类型及参数类型。  
描述通过硬编码在注册阶段确定。

### Info

数据部分由用户定义，不同的Plan内每个Task有不同的Info。 其中包含了Task的启用状态、接口使用的频道及参数设定。

### 接口

接口(又称IO), 是每个Task之间通讯的方式, 使用`msg`模块实现(原使用`umt`模块)。

在用户侧可以定义接口所使用的频道, 和数据分配方式(原使用`umt`模块时不包含。)

### 参数

使用参数(又称arg)来简单地控制每个模块的特殊行为, 例如图像处理模块的线程数量, 预测模块的枪口偏移参数等。

参数目前支持如下类型:

- `STR` 字符串类型
- `NUMBER` 数字类型(所有数字)
- `BOOL` 布尔类型

在传递时, 所有参数都以string形式传递, 类型仅限定了前端调参时的输入, 在使用时需要自行转换类型。

## Plan

流程是组织任务的定义, 程序可以拥有多个Plan, 通过定义Plan并切换Plan, 可以是程序执行不同的任务, 让多个不同的机器人可以使用同一套代码,
仅需在用户侧选择对应的Plan即可。

Plan内包含了Plan的名称和描述，及所有task的数据。

## state

阶段(又称state)是指Plan的运行阶段, 程序应在不同的阶段做不同的事, 以达到Task同步的目的。

- `0` = **等待阶段**, 此阶段不可做任何事情, 需要等待数值改变
- `1` = **初始化阶段**, 在此阶段初始化所有需要的数据, 包括资源文件, 推送/订阅器, 窗口等
- `2` = **运行阶段**, 此阶段正常工作
- `3` = **停止阶段**, 在此阶段需要尽快停止运行并退出运行阶段
- `4` = **清理阶段**, 在此阶段清理回收所有使用过的数据, 包括释放资源, 取消推送/订阅, 内存回收, 关闭窗口

## Task运行主体

Task运行主体是一个函数, 其定义为:

```cpp
/**
 * 任务运行体
 *
 * @param arg1 每个IO对应的数据
 * @param arg2 每个arg对应的数据
 * @param arg3 阶段数字, 会随着状态不同而改变, 只读
 * @param arg4 阶段完成的回调函数, 要传入完成的阶段数字
 */
std::function<void(std::map<const std::string, TaskIOInfo>, std::map<const std::string, std::string>, const int *, const std::function<void(const int)>)>
```

在Task运行主体内将完成模块的整个生命周期, 通过判断state指针来确定程序应做什么事情。

## 样例

Task注册(以Video输入为例)

```cpp
static void registerTask() {
  static const std::string io_src = "src"; //io名称定义
  static const std::string arg_path = "path"; //arg名称定义
  Plans::TaskDescription description{"input", "视频输入, 模拟相机输入"}; //描述结构体
  description.io[io_src] = {TYPE_NAME(datas::FrameData), "输出一帧数据", false}; //定义IO信息
  description.args[arg_path] = {"视频文件的路径", "", ifr::Plans::TaskArgType::STR};//定义arg信息

  //注册Task
  ifr::Plans::registerTask("video", description, [](auto io, auto args, auto state, auto cb) {
        //Task运行主体
        Plans::Tools::waitState(state, 1);//等待状态从0变为1

        Video video(args[arg_path]);//创建Video
        ifr::Msg::Publisher<datas::FrameData> fdOut(io[io_src].channel);//发布者
        Plans::Tools::finishAndWait(cb, state, 1);//通知状态1就绪, 并等待状态变为2
        fdOut.lock();//锁定发布者
        cb(2);//通知状态2就绪
        const auto delay = SLEEP_TIME(1.0 / video.fps);//延时
        while (*state < 3) {//只要还在状态2(运行状态), 就持续循环
            SLEEP(delay);//休眠定长
            cv::Mat mat;
            video.read(mat);
            fdOut.push({mat, video.id, 1000 * video.id / video.fps, cv::getTickCount(), datas::FrameType::BGR});
        }
        Plans::Tools::finishAndWait(cb, state, 3);//通知状态3就绪, 并等待状态变为4
        //auto release && cb(4) 自动释放相关资源, 并自动通知状态4就绪
      });
}
```