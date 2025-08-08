#pragma once

#include "../Q_1_to_4/Graph/Graph.hpp"
#include "../Q_7/Strategy/AlgoIO.hpp"
#include "../Q_7/Factory/Factory_Algorithms.hpp"

#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <optional>

namespace lf_stage8 {

using Vertex = int;
using Graph  = Graph_implementation::Graph<Vertex>;

static inline std::string run_request(Request<Vertex>& req) {
    try {
        std::unique_ptr<AlgorithmIO<Vertex>> algo =
            AlgorithmsFactory<Vertex>::create(req);
        if (!algo) {
            return std::string("ERR|Unknown algorithm: ") + req.name + "\n";
        }
        Response r = algo->run(req);
        if (!r.response.empty()) {
            return r.response + (r.response.back() == '\n' ? "" : "\n");
        }
        return "ERR|Algorithm returned no output\n";
    } catch (const std::exception& e) {
        std::ostringstream os;
        os << "ERR|Exception while running '" << req.name << "': " << e.what() << "\n";
        return os.str();
    } catch (...) {
        return std::string("ERR|Unknown error while running '") + req.name + "'\n";
    }
}

// Always run all 4 algorithms. MST/Hamilton start from graph.get_first().
// Max-Flow runs on both modes; if undirected, return a friendly "not applicable".
static inline std::string run_all_algorithms(
    Graph& g,
    int n_from_init,
    std::optional<int> mf_source = {},
    std::optional<int> mf_sink   = {}
) {
    std::ostringstream out;

    // 0) Print (directly from Graph)
    out << "===== Graph =====\n";
    out << g.to_string_with_weights(false) << "\n\n";

    const int first = g.get_first();
    const int default_s = 0;
    const int default_t = (n_from_init > 0 ? n_from_init - 1 : 0);
    const int src  = mf_source.value_or(default_s);
    const int sink = mf_sink.value_or(default_t);

    // 1) MST / Directed Arborescence (start from the first vertex)
    {
        Request<Vertex> req(g, "mst", /*start=*/first);
        out << "===== " << (g.is_directed() ? "Directed Arborescence" : "MST (Prim)") << " =====\n";
        out << run_request(req) << "\n";
    }

    // 2) SCC / CC
    {
        Request<Vertex> req(g, "scc");
        out << "===== " << (g.is_directed() ? "Strongly Connected Components" : "Connected Components") << " =====\n";
        out << run_request(req) << "\n";
    }

    // 3) Hamilton (start from the first vertex)
    {
        Request<Vertex> req(g, "hamilton", /*start=*/first);
        out << "===== Hamiltonian =====\n";
        out << run_request(req) << "\n";
    }

    // 4) Max-Flow
    {
        out << "===== Max-Flow =====\n";
        Request<Vertex> req(g, "maxflow",
                            /*start*/{},
                            /*source*/src,
                            /*sink*/sink);
        out << run_request(req) << "\n";
    }

    out << "===== DONE =====\n";
    return out.str();
}

} // namespace lf_stage8
