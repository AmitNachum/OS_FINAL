#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <optional>

#include "../../Q_1_to_4/Graph/Graph.hpp"

/*std::optional<T> is a wrapper template that may
contain a type T value or no value at all(std::nullopt)*/
using namespace Graph_implementation;

template <typename T>
struct Request {
  std::string name;
  Graph<T>& graph;
  std::optional<T> start;  // For Hamilton or MST
  std::optional<T> source; // For Max flow
  std::optional<T> sink;   // For Max flow

  // Explicit ctor
  Request(Graph<T>& g, std::string nm,
          std::optional<T> st = {},
          std::optional<T> src = {},
          std::optional<T> snk = {})
    : name(std::move(nm)), graph(g), start(st), source(src), sink(snk) {}
};




struct Response{
   bool ok;
   std::string response;
};


template <typename T>
class AlgorithmIO{
    public:
    virtual ~AlgorithmIO() = default;
    virtual Response run(const Request<T>& request) = 0;
};




