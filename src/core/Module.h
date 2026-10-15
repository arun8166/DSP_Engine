#pragma once
#include <string>
#include "core/AudioBuffer.h"

class Module {
public:
    virtual ~Module() {}

    virtual void setParam(const std::string&, const std::string&) {}

    virtual void prepare(double rate, int channels, int maxFrames) {
        sampleRate = rate;
        numChannels = channels;
        blockSize = maxFrames;
    }

    virtual void process(AudioBuffer& buffer, int frames) = 0;
    virtual int tailFrames() const { return 0; }
    virtual void finish() {}

    std::string name;
protected:
    double sampleRate = 44100.0;
    int numChannels = 1;
    int blockSize = 256;
};

class Passthrough : public Module {
public:
    void process(AudioBuffer&, int) override {}
};
