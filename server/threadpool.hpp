#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
class ThreadPool
{
public:
    explicit ThreadPool(size_t threadCount);
    ~ThreadPool();
    template <class F>
    void enqueue(F &&task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};
template <class F>
void ThreadPool::enqueue(F &&task)
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.emplace(std::forward<F>(task));
    }
    condition.notify_one();
}

#endif // THREADPOOL_HPP