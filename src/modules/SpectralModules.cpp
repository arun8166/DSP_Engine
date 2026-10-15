#include "modules/SpectralModules.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include "core/Utils.h"
#include "core/WavFile.h"

Convolver::Convolver() {
    mix.setTarget(0.3f);
}

void Convolver::setParam(const std::string& name, const std::string& value) {
    if (name == "ir") irPath = value;
    if (name == "decay") decay = std::max(0.05f, toFloat(value));
    if (name == "predelay") predelay = toFloat(value);
    if (name == "tone") tone = toFloat(value);
    if (name == "mix") mix.setTarget(toFloat(value));
}

void Convolver::loadImpulse(AudioBuffer& ir) {
    WavReader reader;
    if (!irPath.empty()) {
        if (reader.open(irPath) && reader.totalFrames > 0) {
            ir.resize(reader.channels, (int)reader.totalFrames);
            reader.read(ir, (int)reader.totalFrames);
            if (reader.sampleRate != (int)sampleRate) std::printf("warning: %s sample rate differs from engine\n", irPath.c_str());
            return;
        }
        std::printf("warning: cannot read impulse %s, using synthetic reverb\n", irPath.c_str());
    }
    int start = (int)(predelay * 0.001 * sampleRate);
    int length = start + (int)(decay * sampleRate);
    ir.resize(2, length);
    float smoothing = (float)std::exp(-2.0 * PI * tone / sampleRate);
    unsigned int seed = 12345;
    double energy = 0.0;
    for (int c = 0; c < 2; c++) {
        float* d = ir.channel(c);
        float state = 0.0f;
        for (int i = start; i < length; i++) {
            seed = seed * 1664525u + 1013904223u;
            float noise = (seed >> 8) / 8388608.0f - 1.0f;
            state = noise + (state - noise) * smoothing;
            d[i] = state * std::exp(-6.9f * (i - start) / (float)(decay * sampleRate));
            energy += d[i] * d[i];
        }
    }
    ir.scale(length, (float)(1.0 / std::sqrt(energy / 2.0 + 1e-12)));
}

void Convolver::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    mix.reset(rate, 50.0);
    AudioBuffer ir;
    loadImpulse(ir);
    partitionSize = maxFrames;
    int n = 2 * partitionSize;
    fft.setup(n);
    irLength = ir.numFrames();
    irChannels = ir.numChannels();
    partitions = std::max(1, (irLength + partitionSize - 1) / partitionSize);
    irSpectra.assign(irChannels * partitions, std::vector<Complex>(n));
    for (int c = 0; c < irChannels; c++) {
        for (int p = 0; p < partitions; p++) {
            std::vector<Complex>& spectrum = irSpectra[c * partitions + p];
            for (int i = 0; i < partitionSize; i++) {
                int index = p * partitionSize + i;
                if (index < irLength) spectrum[i] = ir.channel(c)[index];
            }
            fft.forward(spectrum);
        }
    }
    history.assign(channels * partitions, std::vector<Complex>(n));
    inputs.assign(channels, std::vector<float>(n, 0.0f));
    work.assign(n, 0.0f);
    mixRamp.assign(maxFrames, 0.0f);
    slot = 0;
}

void Convolver::process(AudioBuffer& buffer, int frames) {
    int b = partitionSize;
    int n = 2 * b;
    for (int i = 0; i < frames; i++) mixRamp[i] = mix.next();
    for (int c = 0; c < numChannels; c++) {
        float* d = buffer.channel(c);
        std::vector<float>& in = inputs[c];
        for (int i = 0; i < b; i++) {
            in[i] = in[i + b];
            in[i + b] = i < frames ? d[i] : 0.0f;
        }
        std::vector<Complex>& spectrum = history[c * partitions + slot];
        for (int i = 0; i < n; i++) spectrum[i] = in[i];
        fft.forward(spectrum);
        std::fill(work.begin(), work.end(), Complex(0.0f, 0.0f));
        int irc = c % irChannels;
        for (int p = 0; p < partitions; p++) {
            const std::vector<Complex>& h = irSpectra[irc * partitions + p];
            const std::vector<Complex>& x = history[c * partitions + (slot - p + partitions) % partitions];
            for (int k = 0; k <= b; k++) work[k] += x[k] * h[k];
        }
        for (int k = 1; k < b; k++) work[n - k] = std::conj(work[k]);
        fft.inverse(work);
        for (int i = 0; i < frames; i++) d[i] = d[i] * (1.0f - mixRamp[i]) + work[b + i].real() * mixRamp[i];
    }
    slot = (slot + 1) % partitions;
}

int Convolver::tailFrames() const {
    return irLength;
}

void Analyzer::setParam(const std::string& name, const std::string& value) {
    if (name == "file") path = value;
    if (name == "size") {
        size = 256;
        while (size < (int)toFloat(value) && size < 65536) size *= 2;
    }
}

void Analyzer::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    fft.setup(size);
    window.resize(size);
    for (int i = 0; i < size; i++) window[i] = (float)(0.5 - 0.5 * std::cos(2.0 * PI * i / size));
    frame.assign(size, 0.0f);
    power.assign(size / 2 + 1, 0.0);
    work.resize(size);
    filled = 0;
    frameCount = 0;
}

void Analyzer::process(AudioBuffer& buffer, int frames) {
    for (int i = 0; i < frames; i++) {
        float mono = 0.0f;
        for (int c = 0; c < numChannels; c++) {
            float v = buffer.channel(c)[i];
            mono += v;
            sumSquares += v * v;
            peakValue = std::max(peakValue, std::fabs(v));
        }
        frame[filled++] = mono / numChannels;
        if (filled == size) {
            analyzeFrame();
            std::copy(frame.begin() + size / 2, frame.end(), frame.begin());
            filled = size / 2;
        }
    }
    samples += (long long)frames * numChannels;
}

void Analyzer::analyzeFrame() {
    for (int i = 0; i < size; i++) work[i] = frame[i] * window[i];
    fft.forward(work);
    for (int k = 0; k <= size / 2; k++) power[k] += std::norm(work[k]);
    frameCount++;
}

void Analyzer::finish() {
    if (frameCount == 0) {
        std::fill(frame.begin() + filled, frame.end(), 0.0f);
        analyzeFrame();
    }
    int bins = size / 2 + 1;
    double binHz = sampleRate / size;
    std::vector<double> db(bins);
    double weighted = 0.0, total = 0.0;
    std::ofstream out(path);
    out << "frequency_hz,magnitude_db\n";
    for (int k = 0; k < bins; k++) {
        double p = power[k] / frameCount;
        db[k] = 10.0 * std::log10(p + 1e-20) + 20.0 * std::log10(4.0 / size);
        weighted += k * binHz * p;
        total += p;
        out << k * binHz << "," << db[k] << "\n";
    }
    std::vector<std::pair<double, double>> peaks;
    for (int k = 1; k < bins - 1; k++) {
        if (db[k] > db[k - 1] && db[k] >= db[k + 1]) {
            double offset = 0.5 * (db[k - 1] - db[k + 1]) / (db[k - 1] - 2.0 * db[k] + db[k + 1]);
            peaks.push_back(std::make_pair(db[k], (k + offset) * binHz));
        }
    }
    std::sort(peaks.rbegin(), peaks.rend());
    double rms = std::sqrt(sumSquares / std::max(1LL, samples));
    std::printf("[%s] rms %.1f dBFS, peak %.1f dBFS, centroid %.0f Hz, spectrum -> %s\n", name.c_str(),
                gainToDb((float)rms), gainToDb(peakValue), weighted / (total + 1e-20), path.c_str());
    std::printf("[%s] strongest peaks:", name.c_str());
    for (size_t i = 0; i < peaks.size() && i < 5; i++) std::printf(" %.1f Hz (%.1f dB)", peaks[i].second, peaks[i].first);
    std::printf("\n");
}
