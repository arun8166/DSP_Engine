#pragma once
#include <string>
#include <vector>
#include "core/Module.h"
#include "core/Smoother.h"
#include "dsp/Filters.h"
#include "dsp/Oscillator.h"

class OscillatorModule : public Module {
public:
    OscillatorModule();
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    Oscillator osc;
    Smoother freq;
    Smoother gain;
};

class EnvelopeModule : public Module {
public:
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    float attack = 10.0f;
    float decay = 150.0f;
    float sustain = 0.6f;
    float release = 200.0f;
    float period = 0.5f;
    float gate = 0.25f;
    long long counter = 0;
    Envelope env;
};

struct Voice {
    Oscillator osc1;
    Oscillator osc2;
    Envelope amp;
    Envelope filterEnv;
    Biquad filter;
    long long offAt = -1;
    long long startedAt = 0;
};

class Synth : public Module {
public:
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    void startStep();
    void noteOn(int note);
    std::string wave = "saw";
    std::vector<std::vector<int>> steps;
    float stepTime = 0.25f;
    float length = 0.8f;
    float attack = 5.0f;
    float decay = 200.0f;
    float sustain = 0.6f;
    float release = 250.0f;
    float cutoff = 800.0f;
    float resonance = 2.0f;
    float envAmount = 3000.0f;
    float filterDecay = 300.0f;
    float detune = 7.0f;
    float gain = 0.25f;
    bool loop = true;
    int stepIndex = 0;
    long long counter = 0;
    long long nextStep = 0;
    std::vector<Voice> voices;
};
