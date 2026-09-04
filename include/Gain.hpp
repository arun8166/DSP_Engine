#pragma once
#include "DSPProcessor.hpp"
#include "Parameter.hpp"

class Gain : public DSPProcessor
{
private:
    Parameter gain;
public:
Gain(float value)
:
gain(value)
{}

void setGain(float value)
{
    gain.setValue(value);
}

void process(
    AudioBuffer& buffer) override
{
    float g=gain.getValue();
    for(int c=0;c<buffer.getNumChannels();c++)
    {
        float* samples=
        buffer.getChannel(c);
        for(int i=0;i<buffer.getNumSamples();i++)
        {
            samples[i]*=g;
        }
    }

}
};