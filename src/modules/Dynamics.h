#pragma once
#include <string>
#include "core/Module.h"

class Dynamics : public Module {
public:
    Dynamics(const std::string& mode);
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
    void finish() override;
private:
    float computeGain(float levelDb) const;
    void updateTimes();
    bool downward;
    bool limiter;
    float threshold = -20.0f;
    float ratio = 4.0f;
    float knee = 6.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float makeup = 0.0f;
    float range = -80.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;
    float detector = 0.0f;
    float maxReduction = 0.0f;
};
