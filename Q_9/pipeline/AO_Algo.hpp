#pragma once
#include <functional>
#include "ActiveObject.hpp"
#include "Job.hpp"
#include "Result.hpp"

namespace Q9 {

// AO_Algo is a generic algorithm stage that turns a Job into a Result.
class AO_Algo : public ActiveObject<Job> {
public:
    using AlgoFunc = std::function<Result(const Job&)>;

    AO_Algo(BlockingQueue<Job>& in_q, BlockingQueue<Result>& out_q, AlgoFunc f)
        : ActiveObject<Job>(in_q), m_out(out_q), m_func(std::move(f)) {}

protected:
    void process(Job&& job) override {
        if (!m_func) return;
        Result r = m_func(job);
        m_out.push(std::move(r));
    }

private:
    BlockingQueue<Result>& m_out;
    AlgoFunc m_func;
};

} // namespace Q9