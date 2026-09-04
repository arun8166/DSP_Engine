#pragma once
#include "DSPProcessor.hpp"
#include <cmath>
class Biquad : public DSPProcessor
{
float b0,b1,b2;
float a1,a2;
float x1=0;
float x2=0;
float y1=0;
float y2=0;
public:
void makeLowPass(
    float frequency,
    float sampleRate,
    float Q)
{
float omega =
2.0f*M_PI*frequency/sampleRate;

float alpha =
sin(omega)/(2.0f*Q);

float cosw =
cos(omega);

b0=(1-cosw)/2;
b1=1-cosw;
b2=b0;

float a0=
1+alpha;

a1=
(-2*cosw)/a0;

a2=
(1-alpha)/a0;

b0/=a0;
b1/=a0;
b2/=a0;
}

void process(
AudioBuffer& buffer) override
{
for(int c=0;c<buffer.getNumChannels();c++)
{
float* x=
buffer.getChannel(c);
for(int i=0;i<buffer.getNumSamples();i++)
{
float input=x[i];
float output=
b0*input+
b1*x1+
b2*x2-
a1*y1-
a2*y2;
x2=x1;
x1=input;
y2=y1;
y1=output;
x[i]=output;
}
}
}
};