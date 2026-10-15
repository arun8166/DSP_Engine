#include "dsp/Filters.h"
#include <algorithm>
#include <cmath>
#include "core/Utils.h"

BiquadCoeffs designBiquad(const std::string& type, double fs, double freq, double q, double gainDb) {
    double w0 = 2.0 * PI * std::max(1.0, std::min(freq, fs * 0.49)) / fs;
    double cosw = std::cos(w0);
    double alpha = std::sin(w0) / (2.0 * std::max(q, 0.01));
    double A = std::pow(10.0, gainDb / 40.0);
    double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;
    if (type == "lowpass" || type == "highpass" || type == "bandpass" || type == "notch" || type == "allpass") {
        a0 = 1 + alpha;
        a1 = -2 * cosw;
        a2 = 1 - alpha;
    }
    if (type == "lowpass") {
        b0 = (1 - cosw) / 2; b1 = 1 - cosw; b2 = b0;
    } else if (type == "highpass") {
        b0 = (1 + cosw) / 2; b1 = -(1 + cosw); b2 = b0;
    } else if (type == "bandpass") {
        b0 = alpha; b1 = 0; b2 = -alpha;
    } else if (type == "notch") {
        b0 = 1; b1 = -2 * cosw; b2 = 1;
    } else if (type == "allpass") {
        b0 = 1 - alpha; b1 = -2 * cosw; b2 = 1 + alpha;
    } else if (type == "peak") {
        b0 = 1 + alpha * A; b1 = -2 * cosw; b2 = 1 - alpha * A;
        a0 = 1 + alpha / A; a1 = -2 * cosw; a2 = 1 - alpha / A;
    } else if (type == "lowshelf" || type == "highshelf") {
        double s = type == "lowshelf" ? 1.0 : -1.0;
        double sq = 2.0 * std::sqrt(A) * alpha;
        b0 = A * ((A + 1) - s * (A - 1) * cosw + sq);
        b1 = s * 2 * A * ((A - 1) - s * (A + 1) * cosw);
        b2 = A * ((A + 1) - s * (A - 1) * cosw - sq);
        a0 = (A + 1) + s * (A - 1) * cosw + sq;
        a1 = -s * 2 * ((A - 1) + s * (A + 1) * cosw);
        a2 = (A + 1) + s * (A - 1) * cosw - sq;
    }
    BiquadCoeffs c = {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
    return c;
}

static double idealLowpass(int n, double cutoff) {
    if (n == 0) return 2.0 * cutoff;
    return std::sin(2.0 * PI * cutoff * n) / (PI * n);
}

std::vector<float> designFir(const std::string& type, double fs, double f1, double f2, int taps, const std::string& window) {
    if (taps < 3) taps = 3;
    if (taps % 2 == 0) taps++;
    int mid = taps / 2;
    std::vector<float> h(taps);
    for (int i = 0; i < taps; i++) {
        int n = i - mid;
        double impulse = n == 0 ? 1.0 : 0.0;
        double low1 = idealLowpass(n, f1 / fs);
        double low2 = idealLowpass(n, f2 / fs);
        double value = low1;
        if (type == "highpass") value = impulse - low1;
        else if (type == "bandpass") value = low2 - low1;
        else if (type == "bandstop") value = impulse - (low2 - low1);
        double t = 2.0 * PI * i / (taps - 1);
        double w = 0.54 - 0.46 * std::cos(t);
        if (window == "blackman") w = 0.42 - 0.5 * std::cos(t) + 0.08 * std::cos(2.0 * t);
        else if (window == "hann") w = 0.5 - 0.5 * std::cos(t);
        else if (window == "rect") w = 1.0;
        h[i] = (float)(value * w);
    }
    return h;
}

Biquad::Biquad() {
    BiquadCoeffs unity = {1, 0, 0, 0, 0};
    k = unity;
}

void Biquad::reset(int channels) {
    z1.assign(channels, 0.0);
    z2.assign(channels, 0.0);
}

float Biquad::process(float x, int ch) {
    double y = k.b0 * x + z1[ch];
    z1[ch] = k.b1 * x - k.a1 * y + z2[ch];
    z2[ch] = k.b2 * x - k.a2 * y;
    return (float)y;
}

void FirFilter::setup(const std::vector<float>& coeffs, int channels) {
    taps.assign(coeffs.rbegin(), coeffs.rend());
    int n = (int)taps.size();
    history.assign(channels, std::vector<float>(2 * n, 0.0f));
    position.assign(channels, 0);
}

float FirFilter::process(float x, int ch) {
    int n = (int)taps.size();
    std::vector<float>& h = history[ch];
    int p = position[ch];
    h[p] = x;
    h[p + n] = x;
    position[ch] = (p + 1) % n;
    const float* window = &h[p + 1];
    float sum = 0.0f;
    for (int i = 0; i < n; i++) sum += taps[i] * window[i];
    return sum;
}

void TransferFunction::setup(const std::vector<double>& num, const std::vector<double>& den, int channels) {
    size_t order = std::max(num.size(), den.size());
    b.assign(order, 0.0);
    a.assign(order, 0.0);
    for (size_t i = 0; i < num.size(); i++) b[i] = num[i] / den[0];
    for (size_t i = 0; i < den.size(); i++) a[i] = den[i] / den[0];
    state.assign(channels, std::vector<double>(order, 0.0));
}

float TransferFunction::process(float x, int ch) {
    std::vector<double>& z = state[ch];
    int n = (int)b.size();
    double y = b[0] * x + z[0];
    for (int i = 1; i < n; i++) z[i - 1] = b[i] * x - a[i] * y + z[i];
    return (float)y;
}

void TransferFunction::bilinear(std::vector<double>& num, std::vector<double>& den, double fs, double freq) {
    int order = (int)std::max(num.size(), den.size()) - 1;
    double k = 1.0 / std::tan(PI * std::min(freq, fs * 0.49) / fs);
    std::vector<double> numZ(order + 1, 0.0);
    std::vector<double> denZ(order + 1, 0.0);
    for (int pass = 0; pass < 2; pass++) {
        const std::vector<double>& src = pass == 0 ? num : den;
        std::vector<double>& dst = pass == 0 ? numZ : denZ;
        int offset = order + 1 - (int)src.size();
        for (int i = 0; i < (int)src.size(); i++) {
            int power = order - (offset + i);
            std::vector<double> poly(1, src[i] * std::pow(k, power));
            for (int j = 0; j < order; j++) {
                double sign = j < power ? -1.0 : 1.0;
                poly.push_back(0.0);
                for (int m = (int)poly.size() - 1; m > 0; m--) poly[m] += sign * poly[m - 1];
            }
            for (int m = 0; m <= order; m++) dst[m] += poly[m];
        }
    }
    num = numZ;
    den = denZ;
}

void DelayLine::setup(int maxSamples) {
    buffer.assign(maxSamples + 2, 0.0f);
    pos = 0;
}

float DelayLine::read(float delay) const {
    int size = (int)buffer.size();
    delay = std::max(1.0f, std::min(delay, (float)(size - 2)));
    double r = pos - (double)delay;
    if (r < 0) r += size;
    int i0 = (int)r;
    int i1 = (i0 + 1) % size;
    float frac = (float)(r - i0);
    return buffer[i0] + (buffer[i1] - buffer[i0]) * frac;
}

void DelayLine::write(float x) {
    buffer[pos] = x;
    pos = (pos + 1) % (int)buffer.size();
}
