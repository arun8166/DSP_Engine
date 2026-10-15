#pragma once
#include <string>
#include <vector>
#include "core/Module.h"
#include "core/Smoother.h"
#include "dsp/Filters.h"

class GainModule : public Module {
public:
    GainModule();
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    Smoother gain;
    Smoother pan;
};

class FirModule : public Module {
public:
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    std::string type = "lowpass";
    std::string window = "hamming";
    float freq = 1000.0f;
    float freq2 = 4000.0f;
    int taps = 101;
    std::vector<float> coeffs;
    FirFilter filter;
};

class IirModule : public Module {
public:
    IirModule();
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    void updateCoeffs();
    std::string type = "lowpass";
    int order = 2;
    Smoother freq;
    Smoother q;
    Smoother gain;
    std::vector<Biquad> sections;
};

class TfModule : public Module {
public:
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    std::vector<double> num = {1.0};
    std::vector<double> den = {1.0};
    std::string domain = "z";
    float freq = 1000.0f;
    TransferFunction tf;
};

struct EqBand {
    std::string type;
    Smoother freq;
    Smoother gain;
    Smoother q;
    Biquad filter;
};

class EqModule : public Module {
public:
    void setParam(const std::string& name, const std::string& value) override;
    void prepare(double rate, int channels, int maxFrames) override;
    void process(AudioBuffer& buffer, int frames) override;
private:
    void addBand(const std::string& type, float freq, float gain, float q);
    std::vector<EqBand> bands;
};
