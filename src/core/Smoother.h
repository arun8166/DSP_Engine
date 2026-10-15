#pragma once

class Smoother {
public:
    void reset(double sampleRate, double timeMs) {
        rampSamples = (int)(sampleRate * timeMs / 1000.0);
        current = target;
        remaining = 0;
    }

    void setTarget(float value) {
        if (value == target && remaining == 0) return;
        target = value;
        if (rampSamples <= 0) {
            current = value;
            remaining = 0;
            return;
        }
        step = (target - current) / rampSamples;
        remaining = rampSamples;
    }

    float next() {
        if (remaining > 0) {
            current += step;
            remaining--;
            if (remaining == 0) current = target;
        }
        return current;
    }

    void skip(int n) {
        if (remaining <= 0) return;
        if (n >= remaining) {
            current = target;
            remaining = 0;
        } else {
            current += step * n;
            remaining -= n;
        }
    }

    bool isSmoothing() const { return remaining > 0; }
    float value() const { return current; }
private:
    float current = 0.0f;
    float target = 0.0f;
    float step = 0.0f;
    int rampSamples = 0;
    int remaining = 0;
};
