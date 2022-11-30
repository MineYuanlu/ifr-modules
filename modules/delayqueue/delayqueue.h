//
// https://gist.github.com/yohhoy/6da797689e16f6fe880c84f41f788c66
// copy 2022/11/30
//

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <utility>
#include <vector>
#include <queue>


class closed_delay_queue : public std::runtime_error {
public:
    explicit closed_delay_queue() : std::runtime_error::runtime_error("[delay_queue] closed") {}
};

template<typename T, typename Clock = std::chrono::system_clock>
class delay_queue {
public:
    using value_type = T;
    using time_point = typename Clock::time_point;
    using vt_pair = std::pair<value_type, time_point>;
private:
    struct vt_pair_greater {
        _NODISCARD constexpr auto operator()(const vt_pair &Left, const vt_pair &Right) const
        noexcept(noexcept(Left.second > Right.second)) -> decltype(Left.second > Right.second) {
            return Left.second > Right.second;
        }
    } comp;

    std::vector<std::pair<value_type, time_point>> q_;//队列
    bool closed_ = false;//是否关闭
    std::mutex mtx_;
    std::condition_variable cv_;
public:

    delay_queue() = default;

    ~delay_queue() = default;

    delay_queue(const delay_queue &) = delete;

    delay_queue &operator=(const delay_queue &) = delete;

    /**
     * @brief 抛出数据
     * @return 数据
     * @throw closed_delay_queue 队列已被关闭
     */
    void push(value_type v, time_point tp) {
        std::lock_guard<decltype(mtx_)> lk(mtx_);
        if (closed_) throw closed_delay_queue();
        q_.emplace_back(std::move(v), tp);
        std::push_heap(q_.begin(), q_.end(), comp);
        cv_.notify_one();
    }

    /**
     * @brief 抛出数据
     * @return 数据
     * @throw closed_delay_queue 队列已被关闭
     */
    auto pop() {
        std::unique_lock<decltype(mtx_)> lk(mtx_);
        auto now = Clock::now();
        while (!closed_ && (q_.empty() || q_.front().second > now)) {
            if (q_.empty()) cv_.wait(lk);
            else cv_.wait_until(lk, q_.front().second);
            now = Clock::now();
        }
        if (closed_) throw closed_delay_queue();  // invalid value
        auto ret = std::move(q_.front().first);
        std::pop_heap(q_.begin(), q_.end(), comp);
        q_.pop_back();
        return ret;
    }

    /**关闭此延时队列*/
    void close() {
        std::lock_guard<decltype(mtx_)> lk(mtx_);
        closed_ = true;
        cv_.notify_all();
    }

    _NODISCARD auto empty() const noexcept(noexcept(q_.empty())) /* strengthened */ { return q_.empty(); }

    _NODISCARD auto size() const noexcept(noexcept(q_.size())) /* strengthened */ { return q_.size(); }


};
