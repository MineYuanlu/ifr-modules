# Data Waiter

> 数据等待器

将多线程数据简单的转换为顺序数据

## start

开始一个数据, 假设开始是顺序的

```cpp
/**
 * 启动一个数据处理, 在收到处理结果前, 此数据之后(ID比较)的数据不会被推出
 * @param id 数据ID
 */
void start(const ID &id);
```

## finish

完成一个数据

```cpp
/**
 * 完成数据处理
 * @param id 数据ID
 * @param data 数据体
 */
void finish(const ID &id, const Data &data);
```

## pop

接收数据

```cpp
/**
 * 获取数据
 * @return 获取到的数据
 */
std::pair<const ID, Data> pop();
```

```cpp
/**
 * 获取数据，有超时时间
 * @param ms 超时时间，单位毫秒
 * @return 获取到的数据
 */
std::pair<const ID, Data> pop_for(size_t ms);
```