# API

> 调试接口

此模块用于网页调参, 开放端口(默认`0.0.0.0:8000`)提供http及websocket路由。

API模块目前主要用于Plan编辑, 请配合[前端](https://github.com/MineYuanlu/ifr-opencv-web)使用

## 安全

由于仅供内部人员使用, 故未添加任何安全措施(仅前端有数据验证), 可能后期会添加身份验证。

## 默认路由

默认路由在`registerRoute`中注册:

- `GET` /time/detail
- `GET` /time/list
- `GET`/vars/enable
- `GET`/vars/var
- `POST`/vars/var
- `GET`/vars/descriptions
- `GET` /task/descriptions
- `GET` /plan/list
- `GET` /plan/state
- `GET` /plan/get
- `POST` /plan/save
- `DELETE` /plan/remove
- `GET` /plan/use
- `GET` /plan/start
- `GET` /plan/stop
- `GET` /api.json

## 内部路由

所有`OPTIONS`都将返回如下信息:

```
HTTP/1.1 204 No Content
Access-Control-Allow-Methods: OPTIONS, GET, POST, DELETE
Allow: OPTIONS, GET, POST, DELETE
Access-Control-Allow-Origin: *
Access-Control-Allow-Headers: *
Content-Length: 0000000000
```

## TimeWatcher

> 耗时监控器

注册:

```cpp
/**
 * 注册耗时检查点
 * @param type 类型名称
 * @param unit_ms 单位, 即(t1-t0)/unit_ms
 * @param point_amount 检查点数量
 * @param worker_amount 工作线程数量
 */
std::shared_ptr<TimeWatcher> registerTimePoint(const std::string &type, const TimeWatcher::unit_t &unit_ms,
const size_t &point_amount, const size_t &worker_amount = 1);

//例如: MineYuanlu/ifr-opencv: src/task/FinderArmor.h
auto tw = ifr::API::registerTimePoint("FinderArmor", cv::getTickFrequency() / 1000, 8, finder_thread_amount);
```

使用:

```cpp
//单线程每次开始
tw->start(thread_id);

//每个point
tw->setTime(point, thread_id, cv::getTickCount());
```

## Variable

> 前端变量控制

启用条件: std C++17 及以上

```cpp
/**
 * 
 * 宏
 * 
 * @brief 定义一个变量
 * @param editable 可编辑
 * @param group 属组
 * @param prefix 变量修饰
 * @param type 变量类型
 * @param name 变量名
 * @param def 默认值
 * @param min 最小值
 * @param max 最大值
 */
IFRAPI_VARIABLE(editable, group, prefix, type, name, def, min, max)
//值不可比较版本
IFRAPI_VARIABLE_NC(editable, group, prefix, type, name, def)  
```

## WebSocket

目前Websocket预留了接口, 但还未实际使用。