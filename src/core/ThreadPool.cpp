#include "ThreadPool.hpp"

namespace beiklive {

ThreadPool& ThreadPool::instance() {
    static ThreadPool pool;
    return pool;
}

ThreadPool::ThreadPool(size_t threadCount) {
    if (threadCount == 0)
        threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0)
        threadCount = 2;

    for (size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait(lock, [this]() {
                        return m_stop.load() || !m_tasks.empty();
                    });
                    if (m_stop.load())
                        return;
                    task = std::move(m_tasks.front());
                    m_tasks.pop_front();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop.exchange(true))
            return;

        std::deque<std::function<void()>> empty;
        m_tasks.swap(empty);
    }

    m_stop.store(true);
    m_cv.notify_all();
    for (auto& worker : m_workers) {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop.load())
            return;
        m_tasks.push_back(std::move(task));
    }
    m_cv.notify_one();
}

void ThreadPool::enqueuePriority(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop.load())
            return;
        m_tasks.push_front(std::move(task));
    }
    m_cv.notify_one();
}

} // namespace beiklive
