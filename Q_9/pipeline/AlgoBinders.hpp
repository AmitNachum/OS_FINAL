#pragma once
#include <memory>
#include <stdexcept>
#include <string>

// Real project includes (Factory/Strategy + Request/Response/AlgorithmIO)
#include "../../Q_7/Strategy/AlgoIO.hpp"
#include "../../Q_7/Factory/Factory_Algorithms.hpp"

#include "Result.hpp"
#include "Job.hpp"

namespace Q9 {

// Helper that mimics stage-8 run_request()
static inline Response run_request_name(GraphT& g,
                                        const std::string& name,
                                        std::optional<int> start = {},
                                        std::optional<int> source = {},
                                        std::optional<int> sink = {})
{
    Request<Vertex> req(g, name, start, source, sink);
    std::unique_ptr<AlgorithmIO<Vertex>> algo =
        AlgorithmsFactory<Vertex>::create(req);
    if (!algo) {
        return {false, std::string("ERR|Unknown algorithm: ") + name + "\n"};
    }
    return algo->run(req);
}

//======Functions that returns the Result struct based on the Algorithm Respond struct=======

inline Result run_mst(const Job& job) {
    Result r; r.job_id = job.job_id; r.kind = AlgoKind::MST;//Set the Result struct
    try {
        const int first = job.graph->get_first();// get the first vertex
        Response rr = run_request_name(*job.graph, "mst", /*start=*/first);//Set the response fields
        r.ok = rr.ok; r.value = rr.response;
        if (!r.ok && r.value.empty()) r.error_msg = "MST failed";
    } catch (const std::exception& e) {
        r.ok = false; r.error_msg = e.what();
    }
    return r;
}

inline Result run_scc(const Job& job) {
    Result r; r.job_id = job.job_id; r.kind = AlgoKind::SCC;
    try {
        Response rr = run_request_name(*job.graph, "scc");
        r.ok = rr.ok; r.value = rr.response;
        if (!r.ok && r.value.empty()) r.error_msg = "SCC failed";
    } catch (const std::exception& e) {
        r.ok = false; r.error_msg = e.what();
    }
    return r;
}

inline Result run_hamilton(const Job& job) {
    Result r; r.job_id = job.job_id; r.kind = AlgoKind::HAMILTON;
    try {
        const int first = job.graph->get_first();
        Response rr = run_request_name(*job.graph, "hamilton", /*start=*/first);
        r.ok = rr.ok; r.value = rr.response;
        if (!r.ok && r.value.empty()) r.error_msg = "Hamilton failed";
    } catch (const std::exception& e) {
        r.ok = false; r.error_msg = e.what();
    }
    return r;
}

inline Result run_maxflow(const Job& job) {
    Result r; r.job_id = job.job_id; r.kind = AlgoKind::MAXFLOW;
    try {
        const std::optional<int> s = job.s;
        const std::optional<int> t = job.t;
        if (!s.has_value() || !t.has_value()) {
            r.ok = false; r.error_msg = "Missing s/t parameters for Max-Flow";
            return r;
        }
        Response rr = run_request_name(*job.graph, "maxflow", /*start*/{}, s, t);
        r.ok = rr.ok; r.value = rr.response;
        if (!r.ok && r.value.empty()) r.error_msg = "Max-Flow failed";
    } catch (const std::exception& e) {
        r.ok = false; r.error_msg = e.what();
    }
    return r;
}

} // namespace Q9
