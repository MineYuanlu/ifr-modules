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
#include <memory>
#include <thread>
#include <utility>
#include <mutex>

using namespace rapidjson;

#define IFRAPI_HAS_VARIABLE RAPIDJSON_HAS_CXX17

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
        void init(const std::string &url = "0.0.0.0:8000", bool async = false);

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


        /**耗时监控器*/
        class TimeWatcher {
        public:
            typedef int64_t tick_t;
            typedef double unit_t;
        private:
            tick_t *mat;//时间点矩阵 [worker][point][c1/c2]
            int *mat_id;//时间点矩阵的状态 [worker][w_id/r_id]
            std::mutex cal_mutex;//计算锁
            std::mutex stat_lock;//状态锁
        public:
            const std::string type;//类型
            const unit_t unit_ms;//单位大小
            const size_t point_amount;//节点数量
            const size_t worker_amount;//工作线程数量
            /**
             * @brief 获取时间信息
             * @details {"u":unit_ms,"mat":[[work0_point0, work0_point1, ... ], ... ]}
             * @return 一个json 包含时间信息
             */
            std::string getTime() {
                if (point_amount < 2 || worker_amount < 1)return "";
                std::unique_lock<std::mutex> lock(cal_mutex);

                rapidjson::StringBuffer buf;
                rapidjson::Writer<rapidjson::StringBuffer> w(buf);
                w.StartObject();
                w.Key("u"), w.Double(unit_ms);
                w.Key("mat"), w.StartArray();
                {
                    std::unique_lock<std::mutex> lock_s(stat_lock);
                    for (size_t i = 0; i < worker_amount; i++)
                        mat_id[i * 2 + 1] = mat_id[i * 2] ? 0 : 1;
                }
                for (size_t i = 0; i < worker_amount; i++) {
                    const auto off_w = 2 * point_amount * i + mat_id[i * 2 + 1];
                    w.StartArray();
                    for (size_t j = 0; j < point_amount; j++)
                        w.Int64(mat[off_w + j * 2]);
                    w.EndArray();
                }
                for (size_t i = 0; i < worker_amount; i++)
                    mat_id[i * 2 + 1] = -1;


                w.EndArray();
                w.EndObject();
                w.Flush();
                return buf.GetString();
            }

            /**
             * @brief 开始一组记录
             * @param worker 开始的工作ID
             */
            inline void start(const size_t &worker) {
                std::unique_lock<std::mutex> lock(stat_lock);
                if (mat_id[worker * 2 + 1] < 0)mat_id[worker * 2] = mat_id[worker * 2] ? 0 : 1;
            }

            /**
             * @brief 记录一个时间
             * @param point 记录点
             * @param worker 工作ID
             * @param time 记录时间
             */
#if DEBUG_TIME

            inline void setTime(const size_t &point, const size_t &worker, const tick_t &time) {
#if DEBUG
                if (point >= 0 && worker >= 0 && point < point_amount && worker < worker_amount) {
#endif
                    mat[worker * point_amount * 2 + point * 2 + mat_id[worker * 2]] = time;
#if DEBUG
                    return;
                }
                throw std::runtime_error("[TimeWatcher] " + type + ": Bad Arg: p=" + std::to_string(point) + ", w=" +
                                         std::to_string(worker) + ", t=" + std::to_string(time));
#endif
            }

#else

            FORCE_INLINE void setTime(const size_t &point, const size_t &worker, const tick_t &time) {}

#endif

            TimeWatcher(std::string type, const unit_t unitMs, const size_t pointAmount, const size_t workerAmount) :
                    type(std::move(type)), unit_ms(unitMs), point_amount(pointAmount), worker_amount(workerAmount) {
                mat = new tick_t[pointAmount * workerAmount * 2];
                mat_id = new int[workerAmount * 2];
                memset(mat, 0, sizeof(tick_t) * pointAmount * workerAmount * 2);
                memset(mat_id, 0, sizeof(int) * workerAmount * 2);
            }

            ~TimeWatcher() {
                delete[] mat;
                delete[] mat_id;
            }


        };


        /**
         * 注册耗时检查点
         * @param type 类型名称
         * @param unit_ms 单位, 即(t1-t0)/unit_ms
         * @param point_amount 检查点数量
         * @param worker_amount 工作线程数量
         */
        std::shared_ptr<TimeWatcher> registerTimePoint(const std::string &type, const TimeWatcher::unit_t &unit_ms,
                                                       const size_t &point_amount, const size_t &worker_amount = 1);

#if IFRAPI_HAS_VARIABLE

        class Variable {
        private:

            template<class T>
            static std::string toStr(const T &data) {
                if (std::is_same<T, bool>::value) return data ? "true" : "false";
                else return std::to_string(data);
            }


        public:
            static std::map<std::string, Variable> vars;
            static std::mutex mutex;
            static bool locked;
            static ifr::Config::ConfigController cc;


            const std::function<void(std::string)> setValue;
            const std::function<std::string()> getValue;


            const bool editable_; //是否可以编辑
            const std::string group_; //所属的组
            const std::string type_;//变量类型
            const std::string name_;//名称
            void *const data_;//数据指针
            const std::string def_;//默认值
            const std::string min_;//最小值
            const std::string max_;//最大值

        private:

            template<class T>
            std::function<std::string()> summonGet(T *const data) { return [data]() { return toStr(*((T *) data)); }; }

            template<class T>
            std::function<void(std::string)> summonSet(bool editable, T *const data) {
                if (!editable)return [](auto) {};
                if (std::is_same<T, bool>::value) {
                    return [data](auto v) { *((T *) data) = v == "true"; };
                } else if (std::is_same<T, int>::value) {
                    return [data](auto v) { *((T *) data) = std::stoi(v); };
                } else if (std::is_same<T, double>::value) {
                    return [data](auto v) { *((T *) data) = std::stod(v); };
                } else if (std::is_same<T, float>::value) {
                    return [data](auto v) { *((T *) data) = std::stof(v); };
                } else if (std::is_same<T, long>::value) {
                    return [data](auto v) { *((T *) data) = std::stol(v); };
                } else if (std::is_same<T, unsigned long>::value) {
                    return [data](auto v) { *((T *) data) = std::stoul(v); };
                } else throw std::runtime_error("[API Variable] Unsupported type: " + type_);
            }

        public:

            template<class T>
            Variable(bool editable, std::string group,
                     std::string type, std::string name, T *const data,
                     const T &def, const T &min, const T &max) :
                    setValue(summonSet<T>(editable, data)), getValue(summonGet<T>(data)), editable_(editable),
                    group_(std::move(group)), type_(std::move(type)), name_(std::move(name)), data_((void *) data),
                    def_(toStr(def)), min_(toStr(min)), max_(toStr(max)) {
            }


#define IFRAPI_VARIABLE(editable, group, name, min, max) \
        ifr::API::Variable::registerVar<decltype(name)>(editable, #group, #name, &(name), min,max); \
///注册可前端调试变量: 是否可编辑, 所属组, 变量名, 最小值, 最大值

            /**
             * @brief 注册一个可调变量
             * @tparam T 变量类型
             * @param editable 是否可以编辑
             * @param group 所属组
             * @param prefix 变量修饰
             * @param type 类型
             * @param name 名称
             * @param data 数据指针
             * @param def 默认值
             * @param min 最小值
             * @param max 最大值
             */
            template<class T>
            static void registerVar(bool editable, const std::string &group,
                                    const std::string &name, T *const data, const T &min, const T &max) {
                auto key = group + " " + name;
                std::unique_lock<decltype(mutex)> lock(mutex);
                if (locked)
                    throw std::runtime_error("[API Variable] Unable to register: " + key + ", locked");
                if (vars.count(key))
                    throw std::runtime_error("[API Variable] Variable with key " + key + " already exists");
                assert(*data >= min && *data <= max);
                vars.emplace(key,
                             ifr::API::Variable(editable, group, typeid(*data).name(), name, data, *data, min, max));
            }

            /**保存所有变量*/
            static void save();

            /**初始化*/
            static bool init();

        };

#endif
    }
} // ifr

#endif //IFR_OPENCV_API_H
