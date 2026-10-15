#include "modules/FilterModules.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "core/Utils.h"

const int CONTROL_RATE = 32;

GainModule::GainModule() {
    gain.setTarget(1.0f);
}

void GainModule::setParam(const std::string& name, const std::string& value) {
    if (name == "gain") gain.setTarget(dbToGain(toFloat(value)));
    if (name == "pan") pan.setTarget(std::max(-1.0f, std::min(1.0f, toFloat(value))));
}

void GainModule::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    gain.reset(rate, 50.0);
    pan.reset(rate, 50.0);
}

void GainModule::process(AudioBuffer& buffer, int frames) {
    for (int i = 0; i < frames; i++) {
        float g = gain.next();
        float angle = (pan.next() + 1.0f) * 0.25f * (float)PI;
        for (int c = 0; c < numChannels; c++) {
            float panGain = 1.0f;
            if (numChannels == 2) panGain = (c == 0 ? std::cos(angle) : std::sin(angle)) * 1.4142136f;
            buffer.channel(c)[i] *= g * panGain;
        }
    }
}

void FirModule::setParam(const std::string& name, const std::string& value) {
    if (name == "type") type = value;
    if (name == "window") window = value;
    if (name == "freq") freq = toFloat(value);
    if (name == "freq2") freq2 = toFloat(value);
    if (name == "taps") taps = (int)toFloat(value);
    if (name == "coeffs") {
        std::vector<double> list = toList(value);
        coeffs.assign(list.begin(), list.end());
    }
}

void FirModule::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    if (coeffs.empty()) filter.setup(designFir(type, rate, freq, freq2, taps, window), channels);
    else filter.setup(coeffs, channels);
}

void FirModule::process(AudioBuffer& buffer, int frames) {
    for (int c = 0; c < numChannels; c++) {
        float* d = buffer.channel(c);
        for (int i = 0; i < frames; i++) d[i] = filter.process(d[i], c);
    }
}

IirModule::IirModule() {
    freq.setTarget(1000.0f);
    q.setTarget(0.7071f);
}

void IirModule::setParam(const std::string& name, const std::string& value) {
    if (name == "type") type = value;
    if (name == "order") order = std::max(2, (int)toFloat(value) / 2 * 2);
    if (name == "freq") freq.setTarget(toFloat(value));
    if (name == "q") q.setTarget(toFloat(value));
    if (name == "gain") gain.setTarget(toFloat(value));
}

void IirModule::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    freq.reset(rate, 30.0);
    q.reset(rate, 30.0);
    gain.reset(rate, 30.0);
    bool cascade = order > 2 && (type == "lowpass" || type == "highpass");
    sections.assign(cascade ? order / 2 : 1, Biquad());
    for (Biquad& section : sections) section.reset(channels);
    updateCoeffs();
}

void IirModule::updateCoeffs() {
    int n = (int)sections.size();
    for (int k = 0; k < n; k++) {
        double sectionQ = q.value();
        if (n > 1) sectionQ = 1.0 / (2.0 * std::sin((2 * k + 1) * PI / (4.0 * n)));
        sections[k].setCoeffs(designBiquad(type, sampleRate, freq.value(), sectionQ, gain.value()));
    }
}

void IirModule::process(AudioBuffer& buffer, int frames) {
    for (int start = 0; start < frames; start += CONTROL_RATE) {
        int count = std::min(CONTROL_RATE, frames - start);
        if (freq.isSmoothing() || q.isSmoothing() || gain.isSmoothing()) {
            freq.skip(count);
            q.skip(count);
            gain.skip(count);
            updateCoeffs();
        }
        for (int c = 0; c < numChannels; c++) {
            float* d = buffer.channel(c) + start;
            for (int i = 0; i < count; i++) {
                float x = d[i];
                for (Biquad& section : sections) x = section.process(x, c);
                d[i] = x;
            }
        }
    }
}

void TfModule::setParam(const std::string& name, const std::string& value) {
    if (name == "b") num = toList(value);
    if (name == "a") den = toList(value);
    if (name == "domain") domain = value;
    if (name == "freq") freq = toFloat(value);
}

void TfModule::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    std::vector<double> b = num.empty() ? std::vector<double>(1, 1.0) : num;
    std::vector<double> a = den;
    if (a.empty() || a[0] == 0.0) a = std::vector<double>(1, 1.0);
    if (domain == "s") TransferFunction::bilinear(b, a, rate, freq);
    tf.setup(b, a, channels);
}

void TfModule::process(AudioBuffer& buffer, int frames) {
    for (int c = 0; c < numChannels; c++) {
        float* d = buffer.channel(c);
        for (int i = 0; i < frames; i++) d[i] = tf.process(d[i], c);
    }
}

void EqModule::addBand(const std::string& type, float freq, float gain, float q) {
    EqBand band;
    band.type = type;
    band.freq.setTarget(freq);
    band.gain.setTarget(gain);
    band.q.setTarget(q);
    bands.push_back(band);
}

void EqModule::setParam(const std::string& name, const std::string& value) {
    if (name == "bands") {
        bands.clear();
        for (const std::string& spec : split(value, ',')) {
            std::vector<std::string> f = split(spec, ':');
            float freq = f.size() > 1 ? toFloat(f[1]) : 1000.0f;
            float gain = f.size() > 2 ? toFloat(f[2]) : 0.0f;
            float q = f.size() > 3 ? toFloat(f[3]) : 0.7071f;
            addBand(f[0], freq, gain, q);
        }
        prepare(sampleRate, numChannels, blockSize);
        return;
    }
    if (name == "graphic") {
        bands.clear();
        std::vector<double> gains = toList(value);
        for (size_t i = 0; i < gains.size(); i++) addBand("peak", 31.25f * std::pow(2.0f, (float)i), (float)gains[i], 1.41f);
        prepare(sampleRate, numChannels, blockSize);
        return;
    }
    size_t pos = name.find_first_of("0123456789");
    if (pos == std::string::npos) return;
    int index = std::atoi(name.c_str() + pos) - 1;
    if (index < 0 || index >= (int)bands.size()) return;
    std::string key = name.substr(0, pos);
    if (key == "freq") bands[index].freq.setTarget(toFloat(value));
    if (key == "gain") bands[index].gain.setTarget(toFloat(value));
    if (key == "q") bands[index].q.setTarget(toFloat(value));
}

void EqModule::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    for (EqBand& band : bands) {
        band.freq.reset(rate, 30.0);
        band.gain.reset(rate, 30.0);
        band.q.reset(rate, 30.0);
        band.filter.reset(channels);
        band.filter.setCoeffs(designBiquad(band.type, rate, band.freq.value(), band.q.value(), band.gain.value()));
    }
}

void EqModule::process(AudioBuffer& buffer, int frames) {
    for (int start = 0; start < frames; start += CONTROL_RATE) {
        int count = std::min(CONTROL_RATE, frames - start);
        for (EqBand& band : bands) {
            if (!band.freq.isSmoothing() && !band.gain.isSmoothing() && !band.q.isSmoothing()) continue;
            band.freq.skip(count);
            band.gain.skip(count);
            band.q.skip(count);
            band.filter.setCoeffs(designBiquad(band.type, sampleRate, band.freq.value(), band.q.value(), band.gain.value()));
        }
        for (int c = 0; c < numChannels; c++) {
            float* d = buffer.channel(c) + start;
            for (int i = 0; i < count; i++) {
                float x = d[i];
                for (EqBand& band : bands) x = band.filter.process(x, c);
                d[i] = x;
            }
        }
    }
}
