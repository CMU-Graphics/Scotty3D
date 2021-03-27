
#include "thread_pool.h"
#include "../util/rand.h"

Thread_Pool::Thread_Pool(size_t threads) {
    start(threads);
}

Thread_Pool::~Thread_Pool() {
    stop();
}

void Thread_Pool::start(size_t threads) {
    n_threads = threads;
    stop_now = false;
    stop_when_done = false;
    for(size_t i = 0; i < threads; i++)
        workers.emplace_back([this] {
            RNG::seed();
            for(;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] {
                        return this->stop_now || this->stop_when_done || !this->tasks.empty();
                    });
                    if(this->stop_now || (this->stop_when_done && this->tasks.empty())) return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
}

void Thread_Pool::clear() {
    stop();
    start(n_threads);
}

void Thread_Pool::wait() {

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop_when_done = true;
    }

    condition.notify_all();
    for(std::thread& worker : workers) {
        worker.join();
    }
    workers.clear();

    start(n_threads);
}

void Thread_Pool::stop() {

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop_now = true;
    }

    condition.notify_all();
    for(std::thread& worker : workers) {
        worker.join();
    }
    workers.clear();

    std::queue<std::function<void()>> empty;
    std::swap(tasks, empty);
}
