#include "Pipeline.hpp"
#include <stdexcept>

//The Pipeline Class is responsible for managing the order of operations and control each stage

namespace Q9 {

Pipeline::Pipeline(SendFunc sender)
    : q_in(Q_CAP_IN),
      q_mst(Q_CAP_ALGO),
      q_scc(Q_CAP_ALGO),
      q_ham(Q_CAP_ALGO),
      q_flow(Q_CAP_ALGO),
      q_results(Q_CAP_AGG),
      q_out(Q_CAP_OUT)
{
    aggregator = std::make_unique<AO_Aggregator>(q_results, q_out);//Group into one payload
    responder  = std::make_unique<AO_Responder>(q_out, std::move(sender));//Send final results
    fanout     = std::make_unique<AO_Fanout>(q_in, *aggregator, q_mst, q_scc, q_ham, q_flow);//Distributes ALgorithms to queues
}

Pipeline::~Pipeline() {
    stop();
}

void Pipeline::start() {
    if (m_started.exchange(true)) return;

    if (!m_mst_func || !m_scc_func || !m_ham_func || !m_flow_func) {
        throw std::runtime_error("Pipeline: algorithm functions not all set");
    }
    //Create an Algo Active Object for each algorithm
    ao_mst = std::make_unique<AO_Algo>(q_mst, q_results, m_mst_func);
    ao_scc = std::make_unique<AO_Algo>(q_scc, q_results, m_scc_func);
    ao_ham = std::make_unique<AO_Algo>(q_ham, q_results, m_ham_func);
    ao_flow= std::make_unique<AO_Algo>(q_flow, q_results, m_flow_func);

    //Perform the Stages as follows:
    responder->start();
    aggregator->start();
    ao_mst->start();
    ao_scc->start();
    ao_ham->start();
    ao_flow->start();
    fanout->start();
}

void Pipeline::stop() {
    if (!m_started.exchange(false)) return;

    q_in.close();
    if (fanout) fanout->stop();

    q_mst.close(); q_scc.close(); q_ham.close(); q_flow.close();
    if (ao_mst) ao_mst->stop();
    if (ao_scc) ao_scc->stop();
    if (ao_ham) ao_ham->stop();
    if (ao_flow) ao_flow->stop();

    q_results.close();
    if (aggregator) aggregator->stop();

    q_out.close();
    if (responder) responder->stop();
}

void Pipeline::submit(const Job& job) {
    if (!m_started.load()) return;
    q_in.push(job);
}

} // namespace Q9