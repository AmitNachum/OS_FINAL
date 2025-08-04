#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <optional>
#include "../Q_1_to_4/Graph/Graph.hpp"
using namespace Graph_implementation;

template <typename T>
struct Request{
  std::string name;
  Graph<T>& graph;
  std::optional<T>& start;//For Hamilton or MST
  std::optional<T>& source;//For Max flow
  std::optional<T>& sink;// For max flow
    // Explicit ctor
  Request(Graph<T>& g, std::string nm,
          std::optional<T> s = {}, std::optional<T> t = {})
    : graph(std::move(g)), name(std::move(nm)),
      source(s), sink(t)
  {}

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