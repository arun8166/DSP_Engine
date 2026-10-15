#include "dsp/Oscillator.h"
#include <algorithm>
#include <cmath>
#include "core/Utils.h"

static double polyBlep(double t, double dt) {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

void Oscillator::setWaveform(const std::string& name) {
    waveform = 0;
    if (name == "saw") waveform = 1;
    if (name == "square") waveform = 2;
    if (name == "triangle") waveform = 3;
    if (name == "noise") waveform = 4;
}

void Oscillator::setFrequency(double freq, double sampleRate) {
    increment = std::max(0.0, std::min(freq, sampleRate * 0.45)) / sampleRate;
}

void Oscillator::setPhase(double value) {
    phase = value - std::floor(value);
}

float Oscillator::next() {
    double t = phase;
    double value = 0.0;
    if (waveform == 0) {
        value = std::sin(2.0 * PI * t);
    } else if (waveform == 1) {
        value = 2.0 * t - 1.0 - polyBlep(t, increment);
    } else if (waveform == 2) {
        value = (t < 0.5 ? 1.0 : -1.0) + polyBlep(t, increment) - polyBlep(std::fmod(t + 0.5, 1.0), increment);
    } else if (waveform == 3) {
        value = 4.0 * std::fabs(t - 0.5) - 1.0;
    } else {
        noiseState = noiseState * 1664525u + 1013904223u;
        value = (noiseState >> 8) / 8388608.0 - 1.0;
    }
    phase += increment;
    if (phase >= 1.0) phase -= 1.0;
    return (float)value;
}

void Envelope::setup(double sampleRate, float attackMs, float decayMs, float sustain, float releaseMs) {
    attackStep = (float)(1.0 / std::max(1.0, attackMs * 0.001 * sampleRate));
    decayCoeff = (float)std::exp(-6.9 / std::max(1.0, decayMs * 0.001 * sampleRate));
    releaseCoeff = (float)std::exp(-6.9 / std::max(1.0, releaseMs * 0.001 * sampleRate));
    sustainLevel = std::max(0.0f, std::min(1.0f, sustain));
}

float Envelope::next() {
    if (stage == 1) {
        level += attackStep;
        if (level >= 1.0f) {
            level = 1.0f;
            stage = 2;
        }
    } else if (stage == 2) {
        level = sustainLevel + (level - sustainLevel) * decayCoeff;
        if (level - sustainLevel < 1e-4f) {
            level = sustainLevel;
            stage = 3;
        }
    } else if (stage == 4) {
        level *= releaseCoeff;
        if (level < 1e-4f) {
            level = 0.0f;
            stage = 0;
        }
    }
    return level;
}
