#pragma once
#include <string>
#include <vector>
#include "core/Module.h"
#include "core/Smoother.h"
#include "dsp/Filters.h"
#include "dsp/Oscillator.h"

class AmpMod : public Module {
public:
    AmpMod(bool ringMode);
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    bool ring;
    std::string wave = "sine";
    float spread = 0.0f;
    Smoother rate;
    Smoother depth;
    std::vector<Oscillator> lfos;
};

class ModDelay : public Module {
public:
    ModDelay(const std::string& kind);
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    float spread = 90.0f;
    Smoother rate;
    Smoother depth;
    Smoother delay;
    Smoother feedback;
    Smoother mix;
    std::vector<DelayLine> lines;
    std::vector<Oscillator> lfos;
};

class Phaser : public Module {
public:
    Phaser();
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    int stages = 6;
    float low = 300.0f;
    float high = 3000.0f;
    Smoother rate;
    Smoother feedback;
    Smoother mix;
    std::vector<Oscillator> lfos;
    std::vector<std::vector<float>> state;
    std::vector<float> last;
};

class Delay : public Module {
public:
    Delay();
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
    int tailFrames() const override;
private:
    float damping = 6000.0f;
    bool pingpong = false;
    Smoother time;
    Smoother feedback;
    Smoother mix;
    std::vector<DelayLine> lines;
    std::vector<float> lowpass;
    std::vector<float> wet;
};
