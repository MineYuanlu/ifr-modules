//
// Created by yuanlu on 2022/10/5.
//

#ifndef IFR_OPENCV_TASK_H
#define IFR_OPENCV_TASK_H

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include <fstream>
#include <utility>
#include <vector>
#include <atomic>
#include <thread>
#include <filesystem>
#include "set"
#include "map"
#include "config/config.h"
#include "tools/tools.hpp"
#include "logger/logger.hpp"


#define COMMON_LOOP_WAIT 500 //通用的循环等待时长 (ms) 即每到此值一次就需要尽快退出等待并判断state

namespace ifr {

    namespace Plans {
        typedef const rapidjson::Value json_in;//json输入
        typedef rapidjson::Writer<rapidjson::OStreamWrapper> json_out; //json输出

        /**
         * 流程异常
         */
        class PlanError : public std::runtime_error {
        public:
            explicit PlanError(const std::string &Message) : std::runtime_error::runtime_error(Message) {}
        };


        /**一个IO信息*/
        struct TaskIODescription {

            /**此IO的类型 */
            std::string type;
            /**此IO说明 */
            std::string description;
            /** true: Input / false: Output */
            bool isIn{};


            void operator()(std::ostream &fout) const {
                fout << type << std::endl << description << std::endl << (isIn ? 't' : 'f') << std::endl;
            }

            template<class T>
            void operator()(rapidjson::Writer<T> &jout) const {
                jout.StartObject();
                jout.Key("type"), jout.String(type);
                jout.Key("description"), jout.String(description);
                jout.Key("isIn"), jout.Bool(isIn);
                jout.EndObject();
            }


            static TaskIODescription read(std::istream &fin) {
                std::string buf;
                TaskIODescription t;
                std::getline(fin, t.type);
                std::getline(fin, t.description);
                std::getline(fin, buf);
                t.isIn = buf == "t";
                return t;
            }

            static TaskIODescription read(json_in &jin) {
                return {
                        jin["type"].GetString(),
                        jin["description"].GetString(),
                        jin["isIn"].GetBool(),
                };
            }
        };

        /**任务参数的类型*/
        enum TaskArgType {
            STR, NUMBER, BOOL
        };

        /**任务的参数描述信息*/
        struct TaskArgDescription {
            std::string description;//描述
            std::string defaultValue;//默认值
            TaskArgType type{};//值类型


            template<class T>
            void operator()(rapidjson::Writer<T> &jout) const {
                jout.StartObject();
                jout.Key("description"), jout.String(description);
                jout.Key("defaultValue"), jout.String(defaultValue);
                jout.Key("type"), jout.Int((int) type);
                jout.EndObject();
            }

            static TaskArgDescription read(json_in &jin) {
                return {
                        jin["description"].GetString(),
                        jin["defaultValue"].GetString(),
                        (TaskArgType) jin["type"].GetInt()
                };
            }


        };

        /**任务的描述信息*/
        struct TaskDescription {
            /**任务所属的组别 */
            std::string group;
            /**任务说明 */
            std::string description;
            /**任务的IO*/
            std::map<std::string, TaskIODescription> io;

            /**任务参数 (参数名-描述) */
            std::map<std::string, TaskArgDescription> args;


            void operator()(std::ostream &fout) const {
                fout << group << std::endl << description << std::endl << io.size() << std::endl;
                for (const auto &e: io) {
                    fout << e.first << std::endl;
                    e.second(fout);
                }
            }

            template<class T>
            void operator()(rapidjson::Writer<T> &jout) const {
                jout.StartObject();
                jout.Key("group"), jout.String(group);
                jout.Key("description"), jout.String(description);
                jout.Key("io");
                {
                    jout.StartObject();
                    for (const auto &e: io)jout.Key(e.first), e.second(jout);
                    jout.EndObject();
                }
                jout.Key("args");
                {
                    jout.StartObject();
                    for (const auto &e: args)jout.Key(e.first), e.second(jout);
                    jout.EndObject();
                }
                jout.EndObject();
            }


            static TaskDescription read(std::istream &fin) {
                std::string buf;
                TaskDescription t;
                std::getline(fin, t.group);
                std::getline(fin, t.description);
                std::getline(fin, buf);
                int amt = stoi(buf);
                for (int i = 0; i < amt; ++i) {
                    std::getline(fin, buf);
                    t.io[buf] = TaskIODescription::read(fin);
                }
                return t;
            }

            static TaskDescription read(json_in &jin) {
                TaskDescription t = {
                        jin["group"].GetString(),
                        jin["description"].GetString(),
                };
                for (auto &m: jin["io"].GetObj())t.io[m.name.GetString()] = TaskIODescription::read(m.value);
                if (jin["args"].IsObject())
                    for (auto &m: jin["args"].GetObj())t.args[m.name.GetString()] = TaskArgDescription::read(m.value);
                return t;
            }

        };

        /**任务的IO信息*/
        struct TaskIOInfo {
            /**此IO使用的频道名 */
            std::string channel;


            void operator()(std::ostream &fout) const {
                fout << channel << std::endl;
            }

            template<class T>
            void operator()(rapidjson::Writer<T> &jout) const {
                jout.StartObject();
                jout.Key("channel"), jout.String(channel);
                jout.EndObject();
            }


            static TaskIOInfo read(std::istream &fin) {
                TaskIOInfo t;
                std::getline(fin, t.channel);
                return t;
            }

            static TaskIOInfo read(json_in &jin) {
                return {jin["channel"].GetString()};
            }
        };

        /**一个任务的数据 */
        struct TaskInfo {
            /**是否启用 */
            bool enable = false;
            /**任务的IO*/
            std::map<const std::string, TaskIOInfo> io;

            /**任务参数 (参数名-数据) */
            std::map<std::string, std::string> args;


            void operator()(std::ostream &fout) const {
                fout << (enable ? 't' : 'f') << std::endl << io.size() << std::endl;
            }

            template<class T>
            void operator()(rapidjson::Writer<T> &jout) const {
                jout.StartObject();
                jout.Key("enable"), jout.Bool(enable);
                jout.Key("io");
                {
                    jout.StartObject();
                    for (const auto &e: io)jout.Key(e.first), e.second(jout);
                    jout.EndObject();
                }
                jout.Key("args");
                {
                    jout.StartObject();
                    for (const auto &e: args)jout.Key(e.first), jout.String(e.second);
                    jout.EndObject();
                }
                jout.EndObject();
            }


            static TaskInfo read(std::istream &fin) {
                std::string buf;
                TaskInfo t;
                std::getline(fin, buf);
                t.enable = buf == "t";
                std::getline(fin, buf);
                int amt = std::stoi(buf);
                for (int i = 0; i < amt; ++i) {
                    std::getline(fin, buf);
                    t.io[buf] = TaskIOInfo::read(fin);
                }
                return t;
            }

            static TaskInfo read(json_in &jin) {
                TaskInfo t = {
                        jin["enable"].GetBool(),
                };
                for (auto &m: jin["io"].GetObj())t.io[m.name.GetString()] = TaskIOInfo::read(m.value);
                if (jin["args"].IsObject())
                    for (auto &m: jin["args"].GetObj())t.args[m.name.GetString()] = m.value.GetString();
                return t;
            }
        };

        /**流程信息*/
        struct PlanInfo {
            PlanInfo() : loaded(false) {};
            bool loaded;
            /**流程名称 */
            std::string name;
            /**流程说明 */
            std::string description;
            /**所有任务*/
            std::map<const std::string, TaskInfo> tasks;


            void operator()(std::ostream &fout) const {
                fout << name << std::endl << description << std::endl << tasks.size() << std::endl;
                for (const auto &e: tasks) {
                    fout << e.first << std::endl;
                    e.second(fout);
                }
            }

            template<class T>
            void operator()(rapidjson::Writer<T> &jout) const {
                jout.StartObject();
                jout.Key("name"), jout.String(name);
                jout.Key("description"), jout.String(description);
                jout.Key("tasks");
                {
                    jout.StartObject();
                    for (const auto &e: tasks)jout.Key(e.first), e.second(jout);
                    jout.EndObject();
                }
                jout.EndObject();
            }

            static PlanInfo read(std::istream &fin) {
                std::string buf;
                PlanInfo t;
                std::getline(fin, t.name);
                std::getline(fin, t.description);
                std::getline(fin, buf);
                int amt = stoi(buf);
                for (int i = 0; i < amt; ++i) {
                    std::getline(fin, buf);
                    t.tasks[buf] = TaskInfo::read(fin);
                }
                t.loaded = true;
                return t;
            }

            static PlanInfo read(json_in &jin) {
                PlanInfo t;
                t.name = jin["name"].GetString();
                t.description = jin["description"].GetString();
                for (auto &m: jin["tasks"].GetObj())t.tasks[m.name.GetString()] = TaskInfo::read(m.value);
                t.loaded = true;
                return t;
            }

        };

        /**全部任务的描述信息*/
        typedef std::map<std::string, TaskDescription> TaskDescriptions;

        /**
         * 任务运行体
         *
         * 一个任务运行体分为多个阶段:
         * 0 = 等待阶段, 此阶段不可做任何事情, 需要等待数值改变
         * 1 = 初始化阶段, 在此阶段初始化所有需要的数据, 包括资源文件, 推送/订阅器, 窗口等
         * 2 = 运行阶段, 此阶段正常工作
         * 3 = 停止阶段, 在此阶段需要尽快停止运行并退出运行阶段
         * 4 = 清理阶段, 在此阶段清理回收所有使用过的数据, 包括释放资源, 取消推送/订阅, 内存回收, 关闭窗口
         *
         * @param arg1 每个IO对应的数据
         * @param arg2 每个arg对应的数据
         * @param arg3 阶段数字, 会随着状态不同而改变, 只读
         * @param arg4 阶段完成的回调函数, 要传入完成的阶段数字
         */
        typedef std::function<void(std::map<const std::string, TaskIOInfo>, std::map<const std::string, std::string>,
                                   const int *,
                                   const std::function<void(const int)>)> Task;

        /**
         * 初始化操作, 包括读取流程等
         */
        void init();

        /**
         * 注册一个任务
         * @param name 任务名称
         * @param description 任务说明
         * @param initializer 初始化器, 可以返回一些数据, 传入运行体中
         * @param runner 运行体 ()
         */
        void registerTask(const std::string &name, const TaskDescription &description, const Task &registerTask);

        /***
         * 获取全部任务的描述信息
         * @return json
         */
        std::string getTaskDescriptionsJson();

        /**
         * 获取流程列表
         * @return json
         */
        std::string getPlanListJson();

        /**
         * 获取流程状态
         * @return json
         */
        std::string getPlanStateJson();

        /**@return 计划列表*/
        std::vector<std::string> getPlanList();

        /**
         * 获取计划信息
         * @param name 计划名称
         * @return 计划信息
         */
        PlanInfo getPlanInfo(const std::string &name);

        /**
         * 保存计划信息
         * @param info 计划信息
         */
        void savePlanInfo(const PlanInfo &info);

        /**
         * 删除计划信息
         * @param name 计划名称
         * @return 是否成功删除 false: 不存在/IO错误
         */
        bool removePlanInfo(const std::string &name);


        /**
         * 使用计划信息
         * @param name 计划名称
         */
        void usePlanInfo(const std::string &name);

        /**启动计划*/
        bool startPlan();

        /**停止计划*/
        void stopPlan();

        /**@return 是否正在运行计划*/
        bool isRunning();

        /**@return 当前状态*/
        int getState();


        enum msgType {
            /**日志*/
            LOG,
            /**错误信息*/
            ERR,
            /**消息框提醒*/
            POPUP
        };
        typedef std::function<void(msgType, const std::string &, const std::string &, const std::string &)> MsgOutter;

        /**
         * @brief 注册消息输出
         * @details 用于通知输出, 一般由API等接口调用
         * @param outter 输出方法: 消息类型 - 信息源 - 信息体
         */
        void registerMsgOut(const MsgOutter &outter);

        namespace Tools {
            /**
             * 等待状态开始
             * @param state 当前状态
             * @param target 目标状态
             */
            inline void waitState(const int *state, int target) {
                static const auto delay = SLEEP_TIME(0.001);
                while (*state < target)SLEEP(delay);

            }

            /**
             * 完成状态并等待下一个状态开始
             * @param cb 完成回调函数
             * @param state 当前状态
             * @param finished 当前完成的状态
             */
            inline void
            finishAndWait(const std::function<void(const int)> &cb, const int *state, int finished, double d = 0.001) {
                const auto delay = SLEEP_TIME(d);
                cb(finished);
                finished++;
                while (*state < finished)SLEEP(delay);
            }
        }
    }

} // ifr

#endif //IFR_OPENCV_TASK_H
