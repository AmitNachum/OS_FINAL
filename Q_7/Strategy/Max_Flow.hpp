#pragma once




template <typename T>

class MaxFlow : public AlgorithmIO<T>{

    virtual Response run(const Request<T>& req ) override{
        if(!req.source||!req.sink) 
        return {false,"Missing source or sink"};

        const T& source = *req.source;
        const T& sink = *req.sink;

        double value = req.graph.edmon_karp_algorithm(source,sink);

        return {true,std::to_string(value)};

    }
};