#ifndef SLC_QUEUE_H_INCLUDED
#define SLC_QUEUE_H_INCLUDED

#include <mutex>
#include <condition_variable>
#include <queue>

template<typename T>
class ConcurrentQueue {
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable ready_;

public:
    ConcurrentQueue() = default;
    ConcurrentQueue(const ConcurrentQueue&) = delete;
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

public:
    T pop() {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty()) {
            ready_.wait(mlock);
        }
        auto val = queue_.front();
        queue_.pop();
        return val;
    }

    void push(const T& item) {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(item);
        mlock.unlock();
        ready_.notify_one();
    }

};

#endif
