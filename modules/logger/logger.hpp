//
// Created by yuanlu on 2022/11/8.
//

#ifndef COMMON_MODULES_LOGGER_H
#define COMMON_MODULES_LOGGER_H

#include "string"
#include "iostream"
#include "mutex"

namespace ifr {
    namespace logger {

#define IFR_LOGGER(type, __Expr__) ifr::logger::log(type,#__Expr__,__Expr__)
#define IFR_LOGGER_LOE(type, __Expr__) ifr::logger::log_or_err(type,#__Expr__,__Expr__)
        static std::mutex log_mtx;

        /**
         * @brief 打印一行日志
         * @details 4种输出形式
         * @details [type id] sub_type data
         * @details [type] sub_type data
         * @details [type id] data
         * @details [type] data
         * @tparam T log内容类型
         * @param type log分类
         * @param sub_type log子分类(为空不输出)
         * @param data log内容
         * @param id 分类ID(小于0不输出)
         */
        template<typename T>
        inline void log(const std::string &type, const std::string &sub_type, const T &data) {
            std::unique_lock<std::mutex> lock(log_mtx);
            std::cout << '[' << type;
            std::cout << ']';
            if (!sub_type.empty())std::cout << ' ' << sub_type << ':';
            std::cout << ' ' << data << std::endl;
        }

        /**
         * @brief 打印一行日志
         * @details 2种输出形式
         * @details [type id] data
         * @details [type] data
         * @tparam T log内容类型
         * @param type log分类
         * @param data log内容
         * @param id 分类ID(小于0不输出)
         */
        template<typename T>
        inline void log(const std::string &type, const T &data) { log(type, "", data); }

        /**
         * @brief 打印一行日志
         * @details 4种输出形式
         * @details [type id] sub_type data
         * @details [type] sub_type data
         * @details [type id] data
         * @details [type] data
         * @tparam T log内容类型
         * @param type log分类
         * @param id 分类ID(小于0不输出)
         * @param sub_type log子分类(为空不输出)
         * @param data log内容
         */
        template<typename T>
        inline void log(const std::string &type, int id, const std::string &sub_type, const T &data) {
            std::unique_lock<std::mutex> lock(log_mtx);
            std::cout << '[' << type;
            if (id >= 0)std::cout << ' ' << id;
            std::cout << ']';
            if (!sub_type.empty())std::cout << ' ' << sub_type << ':';
            std::cout << ' ' << data << std::endl;
        }

        /**
         * @brief 打印一行日志
         * @details 2种输出形式
         * @details [type id] data
         * @details [type] data
         * @tparam T log内容类型
         * @param type log分类
         * @param id 分类ID(小于0不输出)
         * @param data log内容
         */
        template<typename T>
        inline void log(const std::string &type, int id, const T &data) { log(type, id, "", data); }


        /**
         * @brief 打印一行错误日志
         * @details 4种输出形式
         * @details [type id] sub_type data
         * @details [type] sub_type data
         * @details [type id] data
         * @details [type] data
         * @tparam T log内容类型
         * @param type log分类
         * @param sub_type log子分类(为空不输出)
         * @param data log内容
         * @param id 分类ID(小于0不输出)
         */
        template<typename T>
        inline void err(const std::string &type, const std::string &sub_type, const T &data) {
            std::unique_lock<std::mutex> lock(log_mtx);
            std::cerr << '[' << type;
            std::cerr << ']';
            if (!sub_type.empty())std::cerr << ' ' << sub_type << ':';
            std::cerr << ' ' << data << std::endl;
        }

        /**
         * @brief 打印一行错误日志
         * @details 2种输出形式
         * @details [type id] data
         * @details [type] data
         * @tparam T log内容类型
         * @param type log分类
         * @param data log内容
         * @param id 分类ID(小于0不输出)
         */
        template<typename T>
        inline void err(const std::string &type, const T &data) { err(type, "", data); }

        /**
         * @brief 打印一行错误日志
         * @details 4种输出形式
         * @details [type id] sub_type data
         * @details [type] sub_type data
         * @details [type id] data
         * @details [type] data
         * @tparam T log内容类型
         * @param type log分类
         * @param id 分类ID(小于0不输出)
         * @param sub_type log子分类(为空不输出)
         * @param data log内容
         */
        template<typename T>
        inline void err(const std::string &type, int id, const std::string &sub_type, const T &data) {
            std::unique_lock<std::mutex> lock(log_mtx);
            std::cerr << '[' << type;
            if (id >= 0)std::cerr << ' ' << id;
            std::cerr << ']';
            if (!sub_type.empty())std::cerr << ' ' << sub_type << ':';
            std::cerr << ' ' << data << std::endl;
        }

        /**
         * @brief 打印一行错误日志
         * @details 2种输出形式
         * @details [type id] data
         * @details [type] data
         * @tparam T log内容类型
         * @param type log分类
         * @param id 分类ID(小于0不输出)
         * @param data log内容
         */
        template<typename T>
        inline void err(const std::string &type, int id, const T &data) { err(type, id, "", data); }

        /**
         * @brief 打印一行日志
         * @details 4种输出形式
         * @details [type id] sub_type data
         * @details [type] sub_type data
         * @details [type id] data
         * @details [type] data
         * @tparam T log内容类型
         * @param type log分类
         * @param sub_type log子分类(为空不输出)
         * @param data log内容
         * @param id 分类ID(小于0不输出)
         */
        template<typename T>
        inline void log_or_err(const std::string &type, const std::string &sub_type, const T &data) {
            if (data)log(type, sub_type, data);
            else err(type, sub_type, data);
        }

    }
} // ifr

#endif //COMMON_MODULES_LOGGER_H
