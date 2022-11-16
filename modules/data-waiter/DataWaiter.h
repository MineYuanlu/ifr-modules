//
// Created by yuanlu on 2022/9/19.
//

#ifndef IFR_OPENCV_DATAWAITER_H
#define IFR_OPENCV_DATAWAITER_H

#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <map>

namespace ifr {
    /**
     * @brief 消息异常类型，消息读取超时
     */
    class DataWaiter_Timeout : public std::runtime_error {
    public:
        DataWaiter_Timeout() : std::runtime_error::runtime_error("message read timeout!") {}
    };

/**
 * 等待数据, 多线程下数据处理先后不一致, 通过此类将数据排序
 * @tparam ID
 * @tparam Data
 */
    template<class ID, class Data>
    class DataWaiter {

    public:
        /**
         * 启动一个数据处理, 在收到处理结果前, 此数据之后(ID比较)的数据不会被推出
         * @param id 数据ID
         */
        void start(const ID &id) {
            std::unique_lock<std::mutex> subs_lock(mtx);
            que.push(id);
        }

        /**
         * 完成数据处理
         * @param id 数据ID
         * @param data 数据体
         */
        void finish(const ID &id, const Data &data) {
            std::unique_lock<std::mutex> lock(mtx);
            datas[id] = data;
            if (canPop())cv.notify_one();
        }

        /**
         * 获取数据
         * @return 获取到的数据
         */
        std::pair<const ID, Data> pop() {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]() { return canPop(); });
            const auto id = que.top();
            const auto fst = datas.find(id);
            que.pop();
            std::pair<const ID, Data> e(id, fst->second);
            datas.erase(fst);
            return e;
        }

        /**
         * 获取数据，有超时时间
         * @param ms 超时时间，单位毫秒
         * @return 获取到的数据
         * @throw DataWaiter_Timeout 超时
         */
        std::pair<const ID, Data> pop_for(size_t ms) {
            std::unique_lock<std::mutex> lock(mtx);
            if (!cv.wait_for(lock, std::chrono::milliseconds(ms), [this]() { return canPop(); })) {
                throw DataWaiter_Timeout();
            }
            const auto id = que.top();
            const auto fst = datas.find(id);
            que.pop();
            std::pair<const ID, Data> e(id, fst->second);
            datas.erase(fst);
            return e;
        }


    private:
        mutable std::mutex mtx;
        mutable std::condition_variable cv;
        std::priority_queue<ID, std::vector<ID>, std::greater<>> que;
        std::map<ID, Data> datas;

        bool canPop() {
            if (que.empty())return false;
            return datas.count(que.top());
        }
    };
}


#endif //IFR_OPENCV_DATAWAITER_H
