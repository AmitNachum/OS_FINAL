#pragma once
#include <memory>
#include <atomic>
#include <string>
#include "Constants.hpp"
#include "BlockingQueue.hpp"
#include "Job.hpp"
#include "Result.hpp"
#include "Response.hpp"
#include "AO_Responder.hpp"
#include "AO_Aggregator.hpp"
#include "AO_Algo.hpp"
#include "AO_Fanout.hpp"
#include <mutex>

namespace Q9 {
    static std::mutex m_pipeline_mtx{}; // Mutex to protect the pipeline state

// Pipeline wires all active objects together, exposing submit() to the server.
class Pipeline {
public:
    using SendFunc = AO_Responder::SendFunc;

    explicit Pipeline(SendFunc sender);
    ~Pipeline();

    void start();
    void stop();

    void submit(const Job& job);

    // Algorithm bindings (must be set before start())
    void set_mst_func(AO_Algo::AlgoFunc f)      { m_mst_func = std::move(f); }
    void set_scc_func(AO_Algo::AlgoFunc f)      { m_scc_func = std::move(f); }
    void set_ham_func(AO_Algo::AlgoFunc f)      { m_ham_func = std::move(f); }
    void set_maxflow_func(AO_Algo::AlgoFunc f)  { m_flow_func = std::move(f); }

private:
    BlockingQueue<Job>      q_in;
    BlockingQueue<Job>      q_mst;
    BlockingQueue<Job>      q_scc;
    BlockingQueue<Job>      q_ham;
    BlockingQueue<Job>      q_flow;
    BlockingQueue<Result>   q_results;
    BlockingQueue<Outgoing> q_out;

    std::unique_ptr<AO_Fanout>     fanout;
    std::unique_ptr<AO_Algo>       ao_mst, ao_scc, ao_ham, ao_flow;
    std::unique_ptr<AO_Aggregator> aggregator;
    std::unique_ptr<AO_Responder>  responder;

    AO_Algo::AlgoFunc m_mst_func, m_scc_func, m_ham_func, m_flow_func;
    std::atomic<bool> m_started{false};
    
};

} // namespace Q9