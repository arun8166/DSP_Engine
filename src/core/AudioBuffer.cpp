#include "core/AudioBuffer.h"
#include <algorithm>
#include <cmath>

AudioBuffer::AudioBuffer() : channels(0), frames(0) {}

AudioBuffer::AudioBuffer(int numChannels, int numFrames) : channels(0), frames(0) {
    resize(numChannels, numFrames);
}

void AudioBuffer::resize(int numChannels, int numFrames) {
    channels = numChannels;
    frames = numFrames;
    data.assign((size_t)channels * frames, 0.0f);
}

void AudioBuffer::clear() {
    std::fill(data.begin(), data.end(), 0.0f);
}

void AudioBuffer::addFrom(const AudioBuffer& other, int n, float gain) {
    for (int c = 0; c < channels; c++) {
        const float* src = other.channel(c % other.numChannels());
        float* dst = channel(c);
        for (int i = 0; i < n; i++) dst[i] += gain * src[i];
    }
}

void AudioBuffer::scale(int n, float gain) {
    for (int c = 0; c < channels; c++) {
        float* d = channel(c);
        for (int i = 0; i < n; i++) d[i] *= gain;
    }
}

float AudioBuffer::peak(int n) const {
    float p = 0.0f;
    for (int c = 0; c < channels; c++) {
        const float* d = channel(c);
        for (int i = 0; i < n; i++) p = std::max(p, std::fabs(d[i]));
    }
    return p;
}
