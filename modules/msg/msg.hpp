//
// Created by yuanlu on 2022/11/5.
//

#ifndef COMMON_MODULES_MSG_HPP
#define COMMON_MODULES_MSG_HPP

#include <mutex>
#include <string>
#include <unordered_map>
#include <queue>
#include <random>
#include <condition_variable>
/**
 * 数据通讯模块
 *
 * 单发布 - 多订阅
 */
namespace ifr {
    namespace Msg {
#define MODULE_MSG_OUTPUT_PREFIX "[MSG]" //模块-MSG的输出前缀
#define MODULE_MSG_PUB_OUTPUT_PREFIX MODULE_MSG_OUTPUT_PREFIX " [Pub] "//模块-MSG-Publisher的输出前缀
#define MODULE_MSG_SUB_OUTPUT_PREFIX MODULE_MSG_OUTPUT_PREFIX " [Sub] "//模块-MSG-Subscriber的输出前缀

        /**消息异常类型*/
        class MessageError : public std::runtime_error {
        protected:
            using std::runtime_error::runtime_error;
        };

        /**错误使用异常, 此类错误由代码错误引起*/
        class MessageError_BadUse : public MessageError {
        public:
            explicit MessageError_BadUse(const std::string &str) : MessageError(str) {}
        };

        /**无消息错误: 等待超时*/
        class MessageError_NoMsg : public MessageError {
        public:
            explicit MessageError_NoMsg(const std::string &str) : MessageError(str) {}
        };

        /**破坏错误: 由于某pub/sub被回收导致频道失效等*/
        class MessageError_Broke : public MessageError {
        public:
            explicit MessageError_Broke(const std::string &str) : MessageError(str) {}
        };


        /**消息分发策略*/
        enum DistributeType {
            /**每个消息都会发给每一个订阅者*/
            same,
            /**独占模式, 每个消息随机发给某个订阅者*/
            rand,
            /**独占模式, 循环发给每个订阅者*/
            each,
            /**独占模式, 寻找等待中的第一个, 如果没有找到, 则退化为rand*/
            wait_fst
        };

        /**
         * @brief 消息发布者, 同一个频道只能有一个发布者
         *
         * @details 每个发布者可以使用不同的发布策略 (DistributeType) 来指定同一个消息发送给哪个订阅者.
         * @tparam T 消息类型
         */
        template<class T>
        class Publisher;

        /**
         * 消息订阅者, 同一个频道可以有多个订阅者
         * @tparam T 消息类型
         */
        template<class T>
        class Subscriber;

        /**
         * 等待监视器
         * @tparam T 计数器类型
         */
        template<class T>
        class WaitingWatcher {
        private:
            T &counter;
        public:
            explicit WaitingWatcher(T &c) : counter(c) { counter++; }

            ~WaitingWatcher() { counter--; }

            WaitingWatcher(const WaitingWatcher &) = delete;

            WaitingWatcher(WaitingWatcher &&obj) = delete;

            WaitingWatcher &operator=(const WaitingWatcher &obj) = delete;

            WaitingWatcher &operator=(WaitingWatcher &&obj) = delete;
        };


        template<class T>
        class Publisher {
            friend class Subscriber<T>;

        private:
            static std::mutex MTX;//全局锁
            static std::unordered_map<std::string, Publisher<T> *> PUBLISHERS;//所有的发布者
            static std::unordered_map<std::string, std::vector<Subscriber<T> *>> SUBSCRIBERS;//所有的订阅者

            std::string name;//频道名称
            DistributeType type = same;//消息分发策略

            bool locked = false;//锁定, 在锁定之后不可添加新的订阅者
            bool breaked = false;//破坏, 当任一订阅者被销毁时, 禁止发布(即清理阶段)
            std::recursive_mutex mtx;

            std::vector<Subscriber<T> *> subs;//所有订阅者

            std::default_random_engine rand_e;//random
            std::uniform_int_distribution<size_t> rand_u;
            size_t nextIndex = 0;//type=each : 下一次的index


            /**
             * @brief 破坏此频道, 此操作将使当前频道上的所有的发布者/订阅者失效
             * @details 破坏操作一般在(发布者/订阅者)释放时执行, 频道上当前的订阅者和发布者都将失效, 防止继续传输数据。 破坏后频道可重新使用。 发布者被破坏后如果再执行push操作将直接返回。
             *
             */
            void doBreak() {
                if (breaked)return;
                std::unique_lock<std::recursive_mutex> lock1(mtx);
                if (breaked)return;
                breaked = true;
                for (const auto &item: subs)item->doBreak();


                std::unique_lock<std::mutex> lock2(MTX);
                PUBLISHERS.erase(name);
            }

        public:
            Publisher() = default;

            /**
             * @param name 频道名称(不得为空字符串)
             * @param type 发布策略
             */
            explicit Publisher(const std::string &name, DistributeType type = same) {
                reg(name, type);
            }

            Publisher(const Publisher &) = delete;

            Publisher(Publisher &&obj) = delete;

            Publisher &operator=(const Publisher &obj) = delete;

            Publisher &operator=(Publisher &&obj) = delete;

            ~Publisher() { doBreak(); }

            /**
             * @brief 注册发布器
             * @details 将此发布器注册到指定频道上, 频道和发布器为1:1对应关系。 同一个发布器仅能绑定一次, 同一个频道也仅能被一个发布器绑定(在上一个发布器注销前)。
             * @param _name 频道名称(不得为空字符串)
             * @param _type 发布策略
             * @throw MessageError_BadUse 发布器或频道已经被绑定/名称为空
             * */
            void reg(const std::string &_name, DistributeType _type = same) {
                std::unique_lock<std::recursive_mutex> lock1(mtx);
                if (!this->name.empty())
                    throw MessageError_BadUse(
                            MODULE_MSG_PUB_OUTPUT_PREFIX "already registered with \"" + this->name +
                            "\" trying to register with \"" +
                            _name + "\"");
                else if (_name.empty())
                    throw MessageError_BadUse(
                            MODULE_MSG_PUB_OUTPUT_PREFIX "Cannot register publisher with blank channel name");
                std::unique_lock<std::mutex> lock2(MTX);
                if (PUBLISHERS.count(_name))
                    throw MessageError_BadUse(
                            MODULE_MSG_PUB_OUTPUT_PREFIX "Channel \"" + _name + "\" has been used by other Publisher");
                PUBLISHERS[_name] = this;

                if (SUBSCRIBERS.count(_name)) {
                    subs = SUBSCRIBERS[_name];
                    for (const auto &item: subs)item->pub = this;
                    SUBSCRIBERS.erase(_name);
                }

                this->name = _name;
                this->type = _type;
            }


            /**
             * 锁定频道, 锁定后将不可再添加订阅者
             * @param must 是否必须至少存在一个订阅者
             * @throw MessageError_BadUse 已锁定/must=true但没有订阅者
             */
            void lock(bool must = false) {
                std::unique_lock<std::recursive_mutex> lock(mtx);
                if (locked)
                    throw MessageError_BadUse(
                            MODULE_MSG_PUB_OUTPUT_PREFIX "Channel \"" + name +
                            "\" Already locked");
                if (name.empty())
                    throw MessageError_BadUse(
                            MODULE_MSG_PUB_OUTPUT_PREFIX "This publisher is not registered yet");
                if (must && subs.empty())
                    throw MessageError_BadUse(
                            MODULE_MSG_PUB_OUTPUT_PREFIX "Channel \"" + name +
                            "\" has no subscribers, but requires at least one.");
                locked = true;
                if (subs.size() > 1 && (type == rand || type == DistributeType::wait_fst))
                    rand_u = std::uniform_int_distribution<size_t>(0, subs.size() - 1);
            }

            /**
             * @brief 检测此订阅者所注册的频道是否有发布者
             * @return 是否有发布者
             * @throw MessageError_BadUse 未注册
             */
            bool hasSubscriber() {
                if (!locked) [[unlikely]] {
                    throw MessageError_BadUse(MODULE_MSG_PUB_OUTPUT_PREFIX "Channel \"" + name + "\" is not locked");
                }
                return !subs.empty();
            }

            /**
             * @brief 发布一个数据
             * @details 将数据按照注册时的分发策略分发给订阅者(在被破坏和无订阅者的情况下将不会做任何事)
             * @param obj 数据
             * @throw MessageError_BadUse 未锁定 / 内部错(消息分发策略无法识别)
             */
            void push(const T &obj) {
                std::unique_lock<std::recursive_mutex> lock(mtx);
                if (!locked) [[unlikely]] {
                    throw MessageError_BadUse(MODULE_MSG_PUB_OUTPUT_PREFIX "Channel \"" + name + "\" is not locked");
                }
                const auto size = subs.size();
                if (size < 1 || breaked)return;
                if (size == 1) {
                    subs[0]->write_obj(obj);
                } else if (type == DistributeType::same) {
                    for (auto &sub: subs) sub->write_obj(obj);
                } else {
                    Subscriber<T> *sub = nullptr;
                    switch (type) {
                        case DistributeType::each: {
                            sub = subs[nextIndex];
                            nextIndex = (nextIndex + 1) % subs.size();
                            break;
                        }
                        case DistributeType::wait_fst: {
                            for (auto &s: subs)
                                if (s->waiting) {
                                    sub = s;
                                    break;
                                }
                            if (sub != nullptr) break;
                        }
                        case DistributeType::rand: {
                            sub = subs[rand_u(rand_e)];
                            break;
                        }
                        default:
                            throw MessageError_BadUse(MODULE_MSG_PUB_OUTPUT_PREFIX "Bad type: " + std::to_string(type));
                    }
                    sub->write_obj(obj);
                }
            }
        };

        template<class T>
        class Subscriber {
            friend class Publisher<T>;

        private:
            std::string name;//频道名
            bool registered = false;//是否已经注册
            Publisher<T> *pub = nullptr;//所属的发布者
            bool breaked = false;//是否被破坏
            mutable std::mutex mtx;
            mutable std::condition_variable cv;
            std::atomic_int waiting;//是否正在等待数据

            size_t maxSize = 1;//最大订阅长度
            std::queue<T> que;//消息队列

            //由发布者调用, 向此订阅者推送一个消息
            void write_obj(const T &obj) {
                std::unique_lock<std::mutex> lock(mtx);
                if (breaked)return;//破坏后不做任何事
                if (maxSize > 0)while (que.size() >= maxSize)que.pop();
                que.push(obj);
                cv.notify_one();
            }


            void doBreak() {
                if (breaked)return;
                std::unique_lock<std::mutex> lock(mtx);
                if (breaked)return;
                breaked = true;
                cv.notify_all();
                if (pub != nullptr) pub->doBreak();
            }

        public:
            Subscriber() = default;

            /**
             * @param name 频道名称(不得为空字符串)
             * @param maxSize 订阅者的消息队列最大长度
             */
            explicit Subscriber(const std::string &name, size_t maxSize = 1) { reg(name, maxSize); }

            Subscriber(const Subscriber &) = delete;

            Subscriber(Subscriber &&obj) = delete;

            Subscriber &operator=(const Subscriber &obj) = delete;

            Subscriber &operator=(Subscriber &&obj) = delete;


            ~Subscriber() { doBreak(); }

            /**
              * @brief 注册订阅器
              * @details 将此订阅器注册到指定频道上, 频道和订阅器为1:n对应关系。 同一个订阅器仅能绑定一次, 但同一个频道可以有多个订阅器。
              * @param _name 频道名称(不得为空字符串)
              * @param _maxSize 订阅者的消息队列最大长度
              * @throw MessageError_BadUse 订阅器已经被绑定/名称为空
              */
            void reg(const std::string &_name, size_t _maxSize = 1) {
                std::unique_lock<std::mutex> lock1(mtx);
                if (!this->name.empty())
                    throw MessageError_BadUse(
                            MODULE_MSG_SUB_OUTPUT_PREFIX "already registered with \"" + this->pub->name +
                            "\" trying to register with \"" +
                            _name + "\"");
                else if (_name.empty())
                    throw MessageError_BadUse(
                            MODULE_MSG_PUB_OUTPUT_PREFIX "Cannot register publisher with blank channel name");
                registered = true;

                std::unique_lock<std::mutex> lock2(Publisher<T>::MTX);
                if (Publisher<T>::PUBLISHERS.count(_name)) {
                    const auto _pub = Publisher<T>::PUBLISHERS[_name];
                    std::unique_lock<std::recursive_mutex> lock3(_pub->mtx);
                    if (_pub->locked)
                        throw MessageError_BadUse(
                                MODULE_MSG_SUB_OUTPUT_PREFIX "Channel \"" + _name + "\" locked");
                    _pub->subs.push_back(this);
                    pub = _pub;
                } else if (Publisher<T>::SUBSCRIBERS.count(_name)) {
                    Publisher<T>::SUBSCRIBERS[_name].push_back(this);
                } else {
                    Publisher<T>::SUBSCRIBERS.insert(
                            std::pair<std::string, std::vector<Subscriber<T> *>>
                                    (_name, std::vector<Subscriber<T> *>()));
                    Publisher<T>::SUBSCRIBERS[_name].push_back(this);
                }
                name = _name;
                maxSize = _maxSize;
            }

            /**
             * @brief 检测此订阅者所注册的频道是否有发布者
             * @return 是否有发布者
             * @throw MessageError_BadUse 未注册
             */
            bool hasPublisher() {
                if (!registered) [[unlikely]] {
                    throw MessageError_BadUse(MODULE_MSG_SUB_OUTPUT_PREFIX "This subscriber is not registered yet");
                }
                return pub != nullptr;
            }

        public:
            /**
             * @brief 尝试获取一条消息
             * @details 如果当前消息上没有发布器，则会抛出一条异常
             * @return 读取到的消息
             * @throw MessageError_BadUse 未注册 / 无发布者
             * @throw MessageError_NoMsg 无数据(发布者被破坏)
             */
            T pop() {
                std::unique_lock<std::mutex> lock(mtx);
                WaitingWatcher<std::atomic_int> ww(waiting);
                if (!registered) [[unlikely]] {
                    throw MessageError_BadUse(MODULE_MSG_SUB_OUTPUT_PREFIX "This subscriber is not registered yet");
                } else if (pub == nullptr) [[unlikely]] {
                    throw MessageError_BadUse(
                            MODULE_MSG_SUB_OUTPUT_PREFIX "Channel \"" + name + "\" has no publisher!");
                }
                cv.wait(lock, [this]() { return breaked || !que.empty(); });
                if (!que.empty()) [[likely]] {
                    auto tmp = std::move(que.front());
                    que.pop();
                    return tmp;
                }
                throw MessageError_Broke(MODULE_MSG_SUB_OUTPUT_PREFIX "Broke");
            }

            /**
             * @brief 尝试获取一条消息，有超时时间
             * @details 如果当前消息上没有发布器，则会抛出一条异常；如果超时，也会抛出一条异常
             * @param ms 超时时间，单位毫秒
             * @return 读取到的消息
             * @throw MessageError_BadUse 未注册 / 无发布者
             * @throw MessageError_NoMsg 无数据(发布者被破坏 / 超时)
             */
            T pop_for(size_t ms) {
                std::unique_lock<std::mutex> lock(mtx);
                WaitingWatcher<std::atomic_int> ww(waiting);
                if (!registered) [[unlikely]] {
                    throw MessageError_BadUse(MODULE_MSG_SUB_OUTPUT_PREFIX "This subscriber is not registered yet");
                } else if (pub == nullptr) [[unlikely]] {
                    throw MessageError_BadUse(
                            MODULE_MSG_SUB_OUTPUT_PREFIX "Channel \"" + name + "\" has no publisher!");
                }
                if (!cv.wait_for(lock, std::chrono::milliseconds(ms),
                                 [this]() { return breaked || !que.empty(); })) {
                    throw MessageError_NoMsg(MODULE_MSG_SUB_OUTPUT_PREFIX "Timeout");
                }
                if (!que.empty()) [[likely]] {
                    auto tmp = std::move(que.front());
                    que.pop();
                    return tmp;
                }
                throw MessageError_Broke(MODULE_MSG_SUB_OUTPUT_PREFIX "Broke");
            }

            /**
             * @brief 尝试获取一条消息，直到某个时间点超时
             * @details 如果当前消息上没有发布器，则会抛出一条异常；如果超时，也会抛出一条异常
             * @param pt 超时时间点，为std::chrono::time_point类型
             * @return 读取到的消息
             * @throw MessageError_BadUse 未注册 / 无发布者
             * @throw MessageError_NoMsg 无数据(发布者被破坏 / 超时)
             */
            template<class P>
            T pop_until(P pt) {
                std::unique_lock<std::mutex> lock(mtx);
                WaitingWatcher<std::atomic_int> ww(waiting);
                if (!registered) [[unlikely]] {
                    throw MessageError_BadUse(MODULE_MSG_SUB_OUTPUT_PREFIX "This subscriber is not registered yet");
                } else if (pub == nullptr) [[unlikely]] {
                    throw MessageError_BadUse(
                            MODULE_MSG_SUB_OUTPUT_PREFIX "Channel \"" + name + "\" has no publisher!");
                }
                if (!cv.wait_until(lock, pt, [this]() { return breaked || !que.empty(); })) {
                    throw MessageError_NoMsg(MODULE_MSG_SUB_OUTPUT_PREFIX "Timeout");
                }
                if (!que.empty()) [[likely]] {
                    auto tmp = std::move(que.front());
                    que.pop();
                    return tmp;
                }
                throw MessageError_Broke(MODULE_MSG_SUB_OUTPUT_PREFIX "Broke");
            }

        };


        template<class T> std::mutex Publisher<T>::MTX;
        template<class T> std::unordered_map<std::string, Publisher<T> *> Publisher<T>::PUBLISHERS;
        template<class T> std::unordered_map<std::string, std::vector<Subscriber<T> *>> Publisher<T>::SUBSCRIBERS;
    }
} // ifr

#endif //COMMON_MODULES_MSG_HPP
