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
         * @param str 广播字符串
         */
        void sendWS(const std::string &str);

    };
} // ifr

#endif //IFR_OPENCV_API_H
