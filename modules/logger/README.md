# LOGGER

> 简单日志模块

## 用法

```cpp
void log(const std::string &type, const std::string &sub_type, const T &data, int id = -1)
void log(const std::string &type, const T &data, int id = -1)
void log(const std::string &type, int id, const T &data)
void log(const std::string &type, int id, const std::string &sub_type, const T &data)

void err(const std::string &type, const std::string &sub_type, const T &data, int id = -1)
void err(const std::string &type, const T &data, int id = -1)
void err(const std::string &type, int id, const T &data)
void err(const std::string &type, int id, const std::string &sub_type, const T &data)
```

## 参数

```bash
type #log分类
id #分类ID (小于0不输出)
sub_type #log子分类 (为空不输出)
data #log内容
```

## 输出形式

- `[type id] sub_type data`
- `[type] sub_type data`
- `[type id] data`
- `[type] data`

## 宏

```cpp
IFR_LOGGER(type, __Expr__) // [type] #__Expr__ __Expr__
IFR_LOGGER_LOE(type, __Expr__) // 同上 但当__Expr__结果为false(或0或等价值)时, 使用err输出 
IFR_LOC_LOGGER(__Expr__) // [file:line:func] #__Expr__ -> __Expr__ 
```

## 解决问题

多线程情况下, 输出乱序。  
使用此模块可以保证单行输出不会被打乱。