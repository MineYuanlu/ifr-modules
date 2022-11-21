//
// Created by yuanlu on 2022/11/8.
//

#ifndef COMMON_MODULES_TOOLS_HPP
#define COMMON_MODULES_TOOLS_HPP

#include "ext_types.h"

#if DEBUG
#define SIMPLE_INSPECT(__Expr__) \
   std::cout<<"Expression " #__Expr__ " : "<<__Expr__<<std::endl;
#else
#define SIMPLE_INSPECT(__Expr__) __Expr__
#endif

#if __OS__ == __OS_Windows__

#include <Windows.h>

#define SLEEP_TIME(t) (unsigned long) ((t) * 1000.0) //转换休眠时间, 单位: s
#define SLEEP(t) Sleep((t)) //执行休眠, 传入 SLEEP_TIME 的返回值
#define TYPE_NAME(x) (((x*)NULL),( #x )) //验证type名称, 并返回字符串形式
#elif __OS__ == __OS_Linux__

#include <unistd.h>

#define SLEEP_TIME(t) useconds_t((unsigned int)((t) * 1000000.0)) //转换休眠时间, 单位: s
#define SLEEP(t) usleep((t)) //执行休眠, 传入 SLEEP_TIME 的返回值
#define TYPE_NAME(x) #x //验证type名称, 并返回字符串形式
#endif

#endif //COMMON_MODULES_TOOLS_HPP
