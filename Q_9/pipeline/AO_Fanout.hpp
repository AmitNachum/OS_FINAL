#pragma once
#include <sstream>
#include "ActiveObject.hpp"
#include "Job.hpp"
#include "AO_Aggregator.hpp"

namespace Q9 {

/* AO_Fanout registers the job with the aggregator 
    and pushes copies into all algo queues.*/
class AO_Fanout : public ActiveObject<Job> {
public:
    AO_Fanout(BlockingQueue<Job>& in_q,
              AO_Aggregator& aggregator,
              BlockingQueue<Job>& q_mst,
              BlockingQueue<Job>& q_scc,
              BlockingQueue<Job>& q_ham,
              BlockingQueue<Job>& q_flow)
        : ActiveObject<Job>(in_q),
          m_agg(aggregator),
          m_q_mst(q_mst), m_q_scc(q_scc),
          m_q_ham(q_ham), m_q_flow(q_flow) {}

protected:
    void process(Job&& job) override {
        std::ostringstream hdr;
        hdr << "===== Graph =====\n";
        hdr << job.graph->to_string_with_weights(false) << "\n\n";

        // Register job with aggregator (client fd, header, directed)
        m_agg.register_job(job.job_id, job.client_fd, hdr.str(), job.directed);

        // Fan-out copies to all algo queues
        m_q_mst.push(job);
        m_q_scc.push(job);
        m_q_ham.push(job);
        m_q_flow.push(std::move(job));
    }

private:
    AO_Aggregator& m_agg;
    BlockingQueue<Job>& m_q_mst;
    BlockingQueue<Job>& m_q_scc;
    BlockingQueue<Job>& m_q_ham;
    BlockingQueue<Job>& m_q_flow;
};

} // namespace Q9
