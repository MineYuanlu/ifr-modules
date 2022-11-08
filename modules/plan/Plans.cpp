//
// Created by yuanlu on 2022/10/5.
//

#include "Plans.h"

namespace ifr::Plans {
    using namespace rapidjson;
    /**访问锁*/
    std::recursive_mutex mtx;
    /**任务描述*/std::string taskDescriptionsJson = "{}";
    /**计划信息json*/std::string planListJson = "[]";


    /**所有任务的注册数据*/
    std::map<std::string, const Task> tasks;


    std::map<std::string, PlanInfo> plans;  //所有计划信息
    std::string currentPlans;               //当前选中的计划信息


    Config::ConfigController cc;


    /**更新任务描述的json串*/
    void updateTaskDescriptionsJson(const std::string &name, const TaskDescription &description) {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        static rapidjson::StringBuffer taskDescriptions;
        static rapidjson::Writer<rapidjson::StringBuffer, Document::EncodingType, UTF8<>> taskDescriptionsWriter(
                taskDescriptions);
        if (taskDescriptions.GetLength() <= 0)taskDescriptionsWriter.StartObject();

        taskDescriptionsWriter.Key(name), description(taskDescriptionsWriter);

        taskDescriptionsWriter.Flush();
        taskDescriptionsJson = taskDescriptions.GetString();
        taskDescriptionsJson.push_back('}');

    }

    /**检查plan名称是否合格*/
    inline bool checkPlanName(std::string str) {
        return std::all_of(str.begin(), str.end(), [](const auto &x) {
            return (('a' <= x && x <= 'z') || ('A' <= x && x <= 'Z') || ('0' <= x && x <= '9') || x == '_' ||
                    x == ' ' || x == '-');
        });
    }

    void readPlanInfo(const std::string &name) {
#if IO_USE_JSON
        std::ifstream fin("runtime/plans/" + name + ".json");
        if (fin.is_open()) {
            rapidjson::IStreamWrapper isw(fin);
            Document d;
            d.ParseStream(isw);
            plans.insert(std::pair<std::string, PlanInfo>(name, PlanInfo::read(d)));
            fin.close();
        } else {
            plans.insert(std::pair<std::string, PlanInfo>(name, {}));
        }
#else
        std::ifstream fin("runtime/plans/" + name + ".data");
        if (fin.is_open()) {
            plans.insert(std::pair<std::string, PlanInfo>(name, PlanInfo::read(fin)));
            fin.close();
        } else {
            plans.insert(std::pair<std::string, PlanInfo>(name, {}));
        }
#endif
    }

    void writePlanInfo(const PlanInfo &info) {
        if (!info.loaded)return;
#if IO_USE_JSON
        std::string file = "runtime/plans/" + info.name + ".json";
        ifr::Config::mkDir(ifr::Config::getDir(file));
        std::ofstream fout(file, std::ios_base::out | std::ios_base::trunc);
        if (fout.is_open()) {
            rapidjson::OStreamWrapper osw(fout);
            rapidjson::Writer<OStreamWrapper> w(osw);
            info(w);
            w.Flush();
            osw.Flush();
            fout.flush();
            fout.close();
        }
#else
        std::string file = "runtime/plans/" + info.name + ".data";
        ifr::Config::mkdir(ifr::Config::getDir(file));
        std::ofstream fout(file, std::ios_base::out | std::ios_base::trunc);
        if (fout.is_open()) {
            info(fout);
            fout.flush();
            fout.close();
        }
#endif
    }

    void registerTask(const std::string &name, const TaskDescription &description, const Task &registerTask) {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        updateTaskDescriptionsJson(name, description);
        tasks.insert(std::pair<std::string, const Task>(name, registerTask));
    }

    std::string getTaskDescriptionsJson() {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        return taskDescriptionsJson;
    }


    std::string getPlanListJson() {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        if (!planListJson.empty())return planListJson;
        StringBuffer buf;
        Writer<StringBuffer> w(buf);
        w.StartArray();
        for (const auto &e: plans)w.String(e.first);
        w.EndArray();
        w.Flush();
        planListJson = std::string(buf.GetString(), buf.GetLength());
        return planListJson;
    }

    std::string getPlanStateJson() {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        StringBuffer buf;
        Writer<StringBuffer> w(buf);

        w.StartObject();
        w.Key("current"), w.String(currentPlans);
        w.Key("running"), w.Bool(isRunning());
        w.EndObject();
        w.Flush();

        return std::string(buf.GetString(), buf.GetLength());
    }

    void init() {
        static ifr::Config::ConfigInfo<void> info = {
                [](auto *a, auto &w) {
                    std::unique_lock<std::recursive_mutex> lock(mtx);
                    w.StartObject();

                    w.Key("list");
                    w.StartArray();
                    for (const auto &e: plans)w.String(e.first);
                    w.EndArray();

                    w.Key("current"), w.String(currentPlans);

                    w.EndObject();
                },
                [](auto *a, const rapidjson::Document &d) {
                    std::unique_lock<std::recursive_mutex> lock(mtx);
                    for (const auto &item: d["list"].GetArray()) {
                        std::string name = item.GetString();
                        if (!checkPlanName(name))continue;
                        readPlanInfo(name);
                    }
                    planListJson = "";
                    currentPlans = d["current"].GetString();
                }
        };
        cc = ifr::Config::createConfig("plan-list", (void *) nullptr, info);
        cc.load();
    }

    std::vector<std::string> getPlanList() {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        std::vector<std::string> vec;
        vec.reserve(plans.size());
        for (const auto &e: plans)vec.push_back(e.first);
        return vec;
    }

    PlanInfo getPlanInfo(const std::string &name) {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        return plans[name];
    }



    ///运行数据, 包括全部流程控制
    namespace RunData {
        const auto delay = SLEEP_TIME(0.1);// 自旋等待间隔
        std::recursive_mutex state_mtx;//访问锁: 状态相关
        std::recursive_mutex running_mtx;//访问锁: 运行相关
        std::string currentPlan;//前流程名称
        std::atomic_int runID;//运行ID, 每次启动任务时改变, 防止不同批次任务混淆

        bool running = false;

        int state;//当前运行阶段, 详见Task描述
        std::set<std::string> runningTasks;//运行中的task名称
        std::set<std::string> waitingTasks;//等待中的task名称 (等待运行阶段完成)
        std::set<std::string> finishingTasks;//等待完毕的task名称 (等待程序退出)


        /**@return 当前阶段是否运行完毕*/
        bool isStepFinish() {
            std::unique_lock<std::recursive_mutex> lock(RunData::state_mtx);
            return waitingTasks.empty();
        }

        /**
         * 进入下一阶段
         * @return false = 当前阶段还未执行, 不可进入下一阶段
         * @return true = 成功进入下一阶段 (但不代表下一阶段执行完毕)
         */
        bool nextStep() {
            std::unique_lock<std::recursive_mutex> lock(RunData::state_mtx);
            if (!isStepFinish())return false;
            waitingTasks = std::set<std::string>(runningTasks.begin(), runningTasks.end());
            state++;
            ifr::logger::log("[Plan]", "nextStep(): arrive state", state);
            return true;
        }

        /**
         * 自动执行步骤, 直到到达了指定的步骤
         *
         * @param target 目标步骤
         * @param waitFinish 是否等待步骤结束
         */
        inline void untilStep(int target, bool waitFinish = false) {
            for (bool b = false; state < target; b = true) {
                if (b)SLEEP(delay);
                nextStep();
            }
            if (waitFinish)while (state == target && !isStepFinish())SLEEP(delay);
        }

        /**
         * 自动执行步骤, 但立即返回
         * @param target
         */
        inline void goStep(int target) {
            std::thread t = std::thread([target]() {
                untilStep(target);
            });
            while (!t.joinable());
            t.detach();
        }

        /**
         * 停止流程并重置
         */
        void reset() {
            std::unique_lock<std::recursive_mutex> lock(RunData::running_mtx);
            ifr::logger::log("[Plan]", "reset(): running =", running);
            if (!running)return;

            untilStep(4, true);//结束
            while (!finishingTasks.empty()) SLEEP(delay);

            std::unique_lock<std::recursive_mutex> lock2(RunData::state_mtx);
            state = 0;
            runningTasks.clear();
            waitingTasks.clear();
            finishingTasks.clear();
            lock2.unlock();

            running = false;
        }

        /**
         * 启动流程
         */
        bool start(const std::string &name) {
            std::unique_lock<std::recursive_mutex> lock(RunData::running_mtx);
            if (running)reset();
            currentPlan = name;

            const int rid = ++runID;
            const auto plan = plans[name];
            if (!plan.loaded)return false;

            std::unique_lock<std::recursive_mutex> lock2(RunData::state_mtx);

            int cnt = 0;//任务启动计数
            for (const auto &ele: plan.tasks) {
                const auto &tname = ele.first;
                const auto &task = ele.second;
                if (!task.enable)continue;
                cnt++;

                std::map<const std::string, TaskIOInfo> io(task.io.begin(), task.io.end());
                std::map<const std::string, std::string> args(task.args.begin(), task.args.end());
                runningTasks.insert(tname);

                const auto regTask = tasks[tname];
                std::thread t = std::thread(
                        [](const auto regTask, const auto rid, const auto tname, auto io, auto args) {
                            try {
                                regTask(io, args, &state, [&rid, &tname](const auto finish) {
                                    std::unique_lock<std::recursive_mutex> lock(RunData::state_mtx);
                                    if (runID != rid || finish != state)return;
                                    waitingTasks.erase(tname);
                                });
                                std::unique_lock<std::recursive_mutex> lock(RunData::state_mtx);
                                if (runID == rid && state == 4)waitingTasks.erase(tname);//可以自动释放最后一步
                            } catch (PlanError &err) {
                                ifr::logger::log("[Plan]", "PlanError:", tname + ", " + err.what());
                                std::thread t(reset);
                                while (!t.joinable());
                                t.detach();
                            } catch (std::exception &err) {
                                ifr::logger::log("[Plan]", "Error:", tname + ", " + err.what());
                            } catch (...) {
                                ifr::logger::log("[Plan]", "Error:", tname);
                            }
                            ifr::logger::log("[Plan]", "Exit Running:", tname);
                            if (runID != rid)return;
                            std::unique_lock<std::recursive_mutex> lock(RunData::state_mtx);
                            finishingTasks.erase(tname);
                            runningTasks.erase(tname);
                            waitingTasks.erase(tname);
                        }, regTask, rid, tname, io, args);
                while (!t.joinable());
                t.detach();
            }
            finishingTasks = std::set<std::string>(runningTasks.begin(),
                                                   runningTasks.end());
            goStep(2);//进入运行阶段
            return running = cnt > 0;
        }
    }

    void savePlanInfo(const PlanInfo &info) {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        planListJson = "";
        plans[info.name] = info;
        writePlanInfo(info);
        cc.save();
        if (currentPlans == info.name)stopPlan();
    }

    bool removePlanInfo(const std::string &name) {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        if (currentPlans == name)currentPlans = "";
        planListJson = "";
        plans.erase(name);
        cc.save();
        stopPlan();
        return true;
    }

    void usePlanInfo(const std::string &name) {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        if (currentPlans != name) {
            currentPlans = name;
            cc.save();
            stopPlan();
        }
    }

    /**启动计划*/
    bool startPlan() {
        std::unique_lock<std::recursive_mutex> lock(mtx);
        if (currentPlans.empty() || !plans[currentPlans].loaded)return false;
        return RunData::start(currentPlans);
    }

    /**停止计划*/
    void stopPlan() {
        RunData::reset();
    }

    bool isRunning() {
        return RunData::running;
    }

} // ifr