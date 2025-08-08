#pragma once
#include <mutex>
#include <condition_variable>
#include <deque>
#include <optional>

namespace Q9 {

// Bounded blocking queue with close() support.
// push() blocks when the queue is full; returns false if queue is closed.
// pop() blocks when the queue is empty; returns std::nullopt when closed and empty.
template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(size_t capacity)
        : m_capacity(capacity), m_closed(false) {}

    bool push(const T& item) {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_cv_not_full.wait(lk, [&]{ return m_closed || m_q.size() < m_capacity; });
        if (m_closed) return false;
        m_q.push_back(item);
        m_cv_not_empty.notify_one();
        return true;
    }

    bool push(T&& item) {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_cv_not_full.wait(lk, [&]{ return m_closed || m_q.size() < m_capacity; });
        if (m_closed) return false;
        m_q.push_back(std::move(item));
        m_cv_not_empty.notify_one();
        return true;
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_cv_not_empty.wait(lk, [&]{ return m_closed || !m_q.empty(); });
        if (m_q.empty()) return std::nullopt; // closed and empty
        T item = std::move(m_q.front());
        m_q.pop_front();
        m_cv_not_full.notify_one();
        return item;
    }

    void close() {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_closed = true;
        m_cv_not_empty.notify_all();
        m_cv_not_full.notify_all();
    }

    bool closed() const {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_closed;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_q.size();
    }

private:
    const size_t m_capacity;
    mutable std::mutex m_mtx;
    std::condition_variable m_cv_not_empty;
    std::condition_variable m_cv_not_full;
    std::deque<T> m_q;
    bool m_closed;
};

} // namespace Q9