#include <iostream>
#include "AudioBuffer.hpp"
#include "DSPGraph.hpp"
#include "Gain.hpp"
#include "Biquad.hpp"

int main()
{

const int samples=512;
AudioBuffer buffer(
    2,
    samples);
// generate test signal
for(int c=0;c<2;c++)
{
float* x=
buffer.getChannel(c);
for(int i=0;i<samples;i++)
{
    x[i]=1.0f;
}
}

Gain gain(0.5f);
Biquad filter;
filter.makeLowPass(
    1000,
    48000,
    0.707);

DSPGraph engine;
engine.addProcessor(
    &gain);
engine.addProcessor(
    &filter);

engine.prepare(
    48000,
    samples);

engine.process(buffer);

std::cout
<<"First samples:\n";

float* output=
buffer.getChannel(0);


for(int i=0;i<10;i++)
{
    std::cout
    <<output[i]
    <<"\n";
}
return 0;
}