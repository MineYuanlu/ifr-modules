//
// Created by yuanlu on 2022/9/22.
//

#ifndef IFR_OPENCV_API_H
#define IFR_OPENCV_API_H

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "mongoose/mongoose.h"
#include "vector"
#include "logger/logger.hpp"
#include "plan/Plans.h"
#include "tools/tools.hpp"
#include <thread>

using namespace rapidjson;

namespace ifr {

    namespace API {

        /**
         * 获取query的值
         * @param query query参数
         * @param key 要查找的键
         * @return 键对应的值
         */
        mg_str mgx_getquery(const mg_str &query, const std::string &key);

        /**
         * @brief 初始化服务器
         * @param async 是否异步, false=直接在当前线程循环
         */
        void init(bool async = false);

        /**
         * @brief websocket广播
         * @details 原始的信息
         * @param str 广播字符串
         */
        void sendWsReal(const std::string &str);

        /**
         * @brief websocket广播
         * @details 发送websocket类型信息
         * @param wsType ws信息类型
         * @param type 发送源类型
         * @param subType 发送源子类型
         * @param msg 消息内容
         */
        void
        sendWs(ifr::Plans::msgType wsType, const std::string &type, const std::string &subType, const std::string &msg);

    };
} // ifr

#endif //IFR_OPENCV_API_H
