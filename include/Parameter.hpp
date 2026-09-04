#pragma once
class Parameter
{
float current;
float target;
float smoothing;
public:
Parameter(
    float value=0.0f)
{
    current=value;
    target=value;
    smoothing=0.001f;
}

void setValue(float v)
{
    target=v;
}

float getValue()
{
    current += 
    (target-current)
    * smoothing;
    return current;
}
};