#pragma once
#include <vector>
#include <algorithm>

class AudioBuffer
{
private:
    std::vector<std::vector<float>> data;
public:
    AudioBuffer(
        int channels,
        int samples)
    {
        data.resize(channels);
        for(auto& c:data)
            c.resize(samples,0.0f);
    }
    int getNumChannels() const
    {
        return data.size();
    }
    int getNumSamples() const
    {
        return data[0].size();
    }
    float* getChannel(int channel)
    {
        return data[channel].data();
    }
    void clear()
    {
        for(auto& c:data)
            std::fill(
                c.begin(),
                c.end(),
                0.0f);
    }
    void applyGain(float gain)
    {
        for(auto& channel:data)
        {
            for(auto& sample:channel)
                sample*=gain;
        }
    }

};