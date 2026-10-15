#pragma once
#include <vector>

class AudioBuffer {
public:
    AudioBuffer();
    AudioBuffer(int channels, int frames);
    void resize(int channels, int frames);
    void clear();
    void addFrom(const AudioBuffer& other, int frames, float gain);
    void scale(int frames, float gain);
    float peak(int frames) const;
    float* channel(int c) { return &data[(size_t)c * frames]; }
    const float* channel(int c) const { return &data[(size_t)c * frames]; }
    int numChannels() const { return channels; }
    int numFrames() const { return frames; }
private:
    int channels;
    int frames;
    std::vector<float> data;
};
