#pragma once
#include <string>
#include <vector>

struct BiquadCoeffs {
    double b0, b1, b2, a1, a2;
};

BiquadCoeffs designBiquad(const std::string& type, double sampleRate, double freq, double q, double gainDb);
std::vector<float> designFir(const std::string& type, double sampleRate, double f1, double f2, int taps, const std::string& window);

class Biquad {
public:
    Biquad();
    void reset(int channels);
    void setCoeffs(const BiquadCoeffs& c) { k = c; }
    float process(float x, int ch);
private:
    BiquadCoeffs k;
    std::vector<double> z1;
    std::vector<double> z2;
};

class FirFilter {
public:
    void setup(const std::vector<float>& coeffs, int channels);
    float process(float x, int ch);
private:
    std::vector<float> taps;
    std::vector<std::vector<float>> history;
    std::vector<int> position;
};

class TransferFunction {
public:
    void setup(const std::vector<double>& num, const std::vector<double>& den, int channels);
    float process(float x, int ch);
    static void bilinear(std::vector<double>& num, std::vector<double>& den, double sampleRate, double freq);
private:
    std::vector<double> b;
    std::vector<double> a;
    std::vector<std::vector<double>> state;
};

class DelayLine {
public:
    void setup(int maxSamples);
    float read(float delay) const;
    void write(float x);
private:
    std::vector<float> buffer;
    int pos = 0;
};
