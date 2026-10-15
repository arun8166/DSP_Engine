#include "modules/Dynamics.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "core/Utils.h"

Dynamics::Dynamics(const std::string& mode) {
    downward = mode == "compressor" || mode == "limiter";
    limiter = mode == "limiter";
    bool gate = mode == "gate";
    threshold = limiter ? -1.0f : (gate ? -50.0f : (downward ? -20.0f : -40.0f));
    ratio = limiter ? 1000.0f : (gate ? 20.0f : (downward ? 4.0f : 2.0f));
    knee = limiter || gate ? 0.0f : 6.0f;
    attackMs = limiter ? 0.1f : (gate ? 1.0f : 10.0f);
    releaseMs = limiter ? 50.0f : 100.0f;
}

void Dynamics::setParam(const std::string& name, const std::string& value) {
    float v = toFloat(value);
    if (name == "threshold" || name == "ceiling") threshold = v;
    if (name == "ratio") ratio = std::max(1.0f, v);
    if (name == "knee") knee = std::max(0.0f, v);
    if (name == "attack") attackMs = v;
    if (name == "release") releaseMs = v;
    if (name == "makeup") makeup = v;
    if (name == "range") range = v;
    updateTimes();
}

void Dynamics::updateTimes() {
    attackCoeff = (float)std::exp(-1.0 / std::max(1.0, attackMs * 0.001 * sampleRate));
    releaseCoeff = (float)std::exp(-1.0 / std::max(1.0, releaseMs * 0.001 * sampleRate));
}

void Dynamics::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    updateTimes();
    envelope = 0.0f;
    detector = 0.0f;
}

float Dynamics::computeGain(float levelDb) const {
    float over = levelDb - threshold;
    if (downward) {
        float slope = 1.0f / ratio - 1.0f;
        if (2.0f * over <= -knee) return 0.0f;
        if (2.0f * std::fabs(over) < knee) return slope * (over + knee / 2) * (over + knee / 2) / (2.0f * knee);
        return slope * over;
    }
    float slope = ratio - 1.0f;
    float gain = slope * over;
    if (2.0f * over >= knee) gain = 0.0f;
    else if (2.0f * std::fabs(over) < knee) gain = -slope * (over - knee / 2) * (over - knee / 2) / (2.0f * knee);
    return std::max(gain, range);
}

void Dynamics::process(AudioBuffer& buffer, int frames) {
    float ceiling = dbToGain(threshold);
    for (int i = 0; i < frames; i++) {
        float peak = 0.0f;
        for (int c = 0; c < numChannels; c++) peak = std::max(peak, std::fabs(buffer.channel(c)[i]));
        detector = std::max(peak, detector * releaseCoeff);
        float target = computeGain(gainToDb(detector));
        bool moreReduction = target < envelope;
        float coeff = (moreReduction == downward) ? attackCoeff : releaseCoeff;
        envelope = target + (envelope - target) * coeff;
        maxReduction = std::min(maxReduction, envelope);
        float gain = dbToGain(envelope + makeup);
        for (int c = 0; c < numChannels; c++) {
            float y = buffer.channel(c)[i] * gain;
            if (limiter) y = std::max(-ceiling, std::min(ceiling, y));
            buffer.channel(c)[i] = y;
        }
    }
}

void Dynamics::finish() {
    std::printf("[%s] max gain reduction %.1f dB\n", name.c_str(), maxReduction);
}
