#pragma once
#include "AudioBuffer.hpp"
class DSPProcessor
{
public:
    virtual ~DSPProcessor(){}
    virtual void prepare(
        double sampleRate,
        int blockSize)
    {}
    virtual void process(
        AudioBuffer& buffer)=0;
    virtual void reset()
    {}
};