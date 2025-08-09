#pragma once
#include "Strategy/AlgoIO.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <optional>

#include "../Q_1_to_4/Graph/Graph.hpp"
#include "./Factory/Factory_Algorithms.hpp"



/*Added Processing functions for parsing and converting
Request and response types:
Request ----->  Response
Response -----> std::string */




inline std::string serialize_response(const Response& r) {
    std::ostringstream out;
    out << (r.ok ? "OK|" : "ERR|")
        << r.response
        << "\n";
    return out.str();
}


template<typename T>
inline Request<T> parse_request(const std::string& raw, Graph<T>& graph) {
    std::istringstream in(raw);
    std::string tok;

    std::string name;
    std::getline(in, name, '|');

    std::optional<T> start, source, sink;
    if (std::getline(in, tok, '|') && !tok.empty())
        start = static_cast<T>(std::stoi(tok));
    if (std::getline(in, tok, '|') && !tok.empty())
        source = static_cast<T>(std::stoi(tok));
    if (std::getline(in, tok, '\n') && !tok.empty())
        sink = static_cast<T>(std::stoi(tok));

    return { graph, std::move(name), start, source, sink };
}


template<typename T>
inline Response process_request(const std::string& raw, Graph<T>& graph) {
    auto req = parse_request<T>(raw, graph);
    auto algo = AlgorithmsFactory<T>::create(req);
    if (!algo)
        return { false, "Unknown algorithm: " + req.name };
    return algo->run(req);
}