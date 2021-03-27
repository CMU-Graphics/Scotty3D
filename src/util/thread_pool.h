
#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

#include "../lib/log.h"

class Thread_Pool {
public:
    Thread_Pool(size_t threads);
    ~Thread_Pool();

    void stop();
    void wait();
    void clear();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type> {

        using return_type = typename std::invoke_result<F, Args...>::type;
        assert(!stop_now && !stop_when_done);

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

private:
    void start(size_t);
    size_t n_threads;
    bool stop_now = true;
    bool stop_when_done = true;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
};
