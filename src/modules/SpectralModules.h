#pragma once
#include <string>
#include <vector>
#include "core/FFT.h"
#include "core/Module.h"
#include "core/Smoother.h"

class Convolver : public Module {
public:
    Convolver();
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
    int tailFrames() const override;
private:
    void loadImpulse(AudioBuffer& ir);
    std::string irPath;
    float decay = 2.0f;
    float predelay = 10.0f;
    float tone = 8000.0f;
    Smoother mix;
    FFT fft;
    int partitionSize = 0;
    int partitions = 0;
    int irLength = 0;
    int irChannels = 1;
    int slot = 0;
    std::vector<std::vector<Complex>> irSpectra;
    std::vector<std::vector<Complex>> history;
    std::vector<std::vector<float>> inputs;
    std::vector<Complex> work;
    std::vector<float> mixRamp;
};

class Analyzer : public Module {
public:
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
    void finish() override;
private:
    void analyzeFrame();
    std::string path = "spectrum.csv";
    int size = 4096;
    FFT fft;
    std::vector<float> window;
    std::vector<float> frame;
    std::vector<double> power;
    std::vector<Complex> work;
    int filled = 0;
    int frameCount = 0;
    double sumSquares = 0.0;
    long long samples = 0;
    float peakValue = 0.0f;
};
