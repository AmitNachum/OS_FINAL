#pragma once
#include <unordered_map>
#include <mutex>
#include <sstream>
#include "ActiveObject.hpp"
#include "Result.hpp"
#include "Response.hpp"
#include "Constants.hpp"


//==== Collection of each Stage's result ======
namespace Q9 {

// Collects results for a single job and formats output identical to stage 8.
struct PartialSet {
    int client_fd = -1;
    int count = 0; //Count for the workload of each stage must equal 4
    bool directed = true;
    std::string graph_header; // "===== Graph =====\n" + dump + "\n\n"

    std::string mst, scc, ham, flow;
    bool ok_mst = false, ok_scc = false, ok_ham = false, ok_flow = false;
    std::string err_mst, err_scc, err_ham, err_flow;
};

class AO_Aggregator : public ActiveObject<Result> {
public:
    AO_Aggregator(BlockingQueue<Result>& in_q, BlockingQueue<Outgoing>& out_q)
        : ActiveObject<Result>(in_q), m_out(out_q) {}

    // New signature: register job with header text and directedness
    void register_job(const std::string& job_id, int client_fd,
                      std::string header, bool directed)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        auto& ps = m_jobs[job_id];
        ps.client_fd   = client_fd;
        ps.graph_header = std::move(header);
        ps.directed    = directed;
    }

protected:
    void process(Result&& r) override { // Processing the result of the chosen Algorithm
        std::unique_lock<std::mutex> lk(m_mtx);
        auto it = m_jobs.find(r.job_id);
        if (it == m_jobs.end()) return; // unknown or already flushed

        PartialSet& ps = it->second;
        switch (r.kind) {
            case AlgoKind::MST:
                ps.ok_mst = r.ok; ps.mst = r.value; ps.err_mst = r.error_msg; break;
            case AlgoKind::SCC:
                ps.ok_scc = r.ok; ps.scc = r.value; ps.err_scc = r.error_msg; break;
            case AlgoKind::HAMILTON:
                ps.ok_ham = r.ok; ps.ham = r.value; ps.err_ham = r.error_msg; break;
            case AlgoKind::MAXFLOW:
                ps.ok_flow = r.ok; ps.flow = r.value; ps.err_flow = r.error_msg; break;
        }
        ps.count++;

        if (ps.count >= REQUIRED_RESULTS_PER_JOB) {/*If we have all the 4 stage's result Send 
                                                    send to the client the respond via the 
                                                    outgoing struct*/
            const int fd = ps.client_fd;
            std::string payload = format_payload(ps);
            m_jobs.erase(it);
            lk.unlock();
            m_out.push(Outgoing{fd, std::move(payload)});
        }
    }

private:
    static void ensure_nl(std::string& s) {
        if (s.empty() || s.back() != '\n') s.push_back('\n');
    }

    static std::string format_payload(PartialSet& ps) {
        std::ostringstream os;

        // 0) Graph header (already includes "===== Graph =====\n" + dump + "\n\n")
        os << ps.graph_header;

        // 1) MST / Directed Arborescence
        os << "===== " << (ps.directed ? "Directed Arborescence" : "MST (Prim)") << " =====\n";
        if (ps.ok_mst) {
            ensure_nl(ps.mst);
            os << ps.mst << "\n";
        } else {
            std::string e = ps.err_mst; ensure_nl(e);
            os << "ERR|" << e << "\n";
        }

        // 2) SCC / CC
        os << "===== " << (ps.directed ? "Strongly Connected Components" : "Connected Components") << " =====\n";
        if (ps.ok_scc) {
            ensure_nl(ps.scc);
            os << ps.scc << "\n";
        } else {
            std::string e = ps.err_scc; ensure_nl(e);
            os << "ERR|" << e << "\n";
        }

        // 3) Hamilton
        os << "===== Hamiltonian =====\n";
        if (ps.ok_ham) {
            ensure_nl(ps.ham);
            os << ps.ham << "\n";
        } else {
            std::string e = ps.err_ham; ensure_nl(e);
            os << "ERR|" << e << "\n";
        }

        // 4) Max-Flow
        os << "===== Max-Flow =====\n";
        if (ps.ok_flow) {
            ensure_nl(ps.flow);
            os << ps.flow << "\n";
        } else {
            std::string e = ps.err_flow; ensure_nl(e);
            os << "ERR|" << e << "\n";
        }

        os << RESPONSE_SENTINEL << "\n";
        return os.str();
    }

private:
    BlockingQueue<Outgoing>& m_out;//Outgoing Results ready to be sent to each client
    std::unordered_map<std::string, PartialSet> m_jobs;// mapping the each job to thier partial set
    std::mutex m_mtx;
};

} // namespace Q9
