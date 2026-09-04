#pragma once
#include <vector>
#include "DSPProcessor.hpp"

class DSPGraph
{
private:
    std::vector<DSPProcessor*> processors;
public:
    void addProcessor(
        DSPProcessor* processor)
    {
        processors.push_back(processor);
    }
    void prepare(
        double sampleRate,
        int blockSize)
    {
        for(auto p:processors)
            p->prepare(
                sampleRate,
                blockSize);
    }

    void process(
        AudioBuffer& buffer)
    {
        for(auto p:processors)
            p->process(buffer);
    }
};