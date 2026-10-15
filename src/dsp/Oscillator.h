#pragma once
#include <string>

class Oscillator {
public:
    void setWaveform(const std::string& name);
    void setFrequency(double freq, double sampleRate);
    void setPhase(double value);
    float next();
private:
    int waveform = 0;
    double phase = 0.0;
    double increment = 0.0;
    unsigned int noiseState = 22222;
};

class Envelope {
public:
    void setup(double sampleRate, float attackMs, float decayMs, float sustain, float releaseMs);
    void noteOn() { stage = 1; }
    void noteOff() { if (stage != 0) stage = 4; }
    float next();
    bool isActive() const { return stage != 0; }
private:
    int stage = 0;
    float level = 0.0f;
    float attackStep = 1.0f;
    float decayCoeff = 0.0f;
    float sustainLevel = 1.0f;
    float releaseCoeff = 0.0f;
};
