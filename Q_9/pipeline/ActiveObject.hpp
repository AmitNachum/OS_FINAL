#pragma once
#include <thread>
#include <atomic>
#include <functional>
#include "BlockingQueue.hpp"

namespace Q9 {

// Base Anstract Class Active Object that consumes items from a BlockingQueue and processes them.
// Derive and implement process(T&&) to perform work on each item.
template <typename TIn>
class ActiveObject {
public:
    using QueueT = BlockingQueue<TIn>;

    explicit ActiveObject(QueueT& in_queue)
        : m_in(in_queue), m_running(false) {}

    virtual ~ActiveObject() { stop(); }

    void start() {
        bool exp = false;
        if (!m_running.compare_exchange_strong(exp, true)) return;
        m_thread = std::thread([this]{ this->run(); }); // Setting the Active Thread
    }

    void stop() {
        if (m_running.exchange(false)) { //Terminate all active threads 
            m_in.close();
            if (m_thread.joinable()) m_thread.join();
        }
    }

protected:
    virtual void process(TIn&& item) = 0;//using move semantics(std::move())

private:
    void run() {
        while (m_running.load()) {//read the var value (true | false)
            auto item = m_in.pop();
            if (!item.has_value()) break; // closed and empty
            process(std::move(*item));//Active thread handle the task
        }
    }

protected:
    QueueT& m_in;//A reference to the tasks Queue

private:
    std::atomic<bool> m_running;/*Special variable that lets multiple
                                threads read/write without any data racing
                                CPU Usage is atomically*/
    std::thread m_thread;
};

} // namespace Q9