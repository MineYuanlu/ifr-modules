# TOOLS

> 基础工具包

此模块定义了一些基础工具

## 宏

宏函数

- `SLEEP_TIME`: 定义休眠时间, 以秒为单位, Windows & Linux通用
- `SLEEP`: 休眠, 参数为`SLEEP_TIME`的返回值
- `SIMPLE_INSPECT`: 简单检查函数, 在`DEBUG`宏为true时, 自动在控制台打印语句的字符串形式及其返回值, 反之仅运行代码。

其它宏

- `__OS__`
- `__OS_MacOS__`
- `__OS_Android__`
- `__OS_Windows__`
- `__OS_Linux__`
- `__OS_Solaris__`
- `__OS_UNIX__`
- `__OS_Unknown__`
- `__ARCH_NAME__`
- `__ARCH_x64__`
- `__ARCH_ARM__`
- `__ARCH_Unknown__`
- `FORCE_INLINE`
- `NO_INLINE`
