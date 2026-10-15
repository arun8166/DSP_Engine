#include "modules/Modulation.h"
#include <algorithm>
#include <cmath>
#include "core/Utils.h"

static void setupLfos(std::vector<Oscillator>& lfos, int channels, const std::string& wave, float spreadDegrees) {
    lfos.assign(channels, Oscillator());
    for (int c = 0; c < channels; c++) {
        lfos[c].setWaveform(wave);
        lfos[c].setPhase(c * spreadDegrees / 360.0);
    }
}

AmpMod::AmpMod(bool ringMode) : ring(ringMode) {
    rate.setTarget(ring ? 440.0f : 5.0f);
    depth.setTarget(ring ? 1.0f : 0.5f);
}

void AmpMod::setParam(const std::string& name, const std::string& value) {
    if (name == "rate") rate.setTarget(toFloat(value));
    if (name == "depth") depth.setTarget(std::max(0.0f, std::min(1.0f, toFloat(value))));
    if (name == "wave") wave = value;
    if (name == "spread") spread = toFloat(value);
}

void AmpMod::prepare(double sr, int channels, int maxFrames) {
    Module::prepare(sr, channels, maxFrames);
    rate.reset(sr, 50.0);
    depth.reset(sr, 50.0);
    setupLfos(lfos, channels, wave, spread);
}

void AmpMod::process(AudioBuffer& buffer, int frames) {
    for (int i = 0; i < frames; i++) {
        float r = rate.next();
        float d = depth.next();
        for (int c = 0; c < numChannels; c++) {
            lfos[c].setFrequency(r, sampleRate);
            float m = lfos[c].next();
            float gain = ring ? (1.0f - d) + d * m : 1.0f - d * (0.5f - 0.5f * m);
            buffer.channel(c)[i] *= gain;
        }
    }
}

ModDelay::ModDelay(const std::string& kind) {
    bool flanger = kind == "flanger";
    bool vibrato = kind == "vibrato";
    rate.setTarget(flanger ? 0.25f : (vibrato ? 5.0f : 0.8f));
    depth.setTarget(flanger || vibrato ? 2.0f : 3.0f);
    delay.setTarget(flanger ? 2.5f : (vibrato ? 3.0f : 20.0f));
    feedback.setTarget(flanger ? 0.6f : 0.0f);
    mix.setTarget(vibrato ? 1.0f : 0.5f);
    if (vibrato) spread = 0.0f;
}

void ModDelay::setParam(const std::string& name, const std::string& value) {
    float v = toFloat(value);
    if (name == "rate") rate.setTarget(v);
    if (name == "depth") depth.setTarget(v);
    if (name == "delay") delay.setTarget(v);
    if (name == "feedback") feedback.setTarget(std::max(-0.95f, std::min(0.95f, v)));
    if (name == "mix") mix.setTarget(v);
    if (name == "spread") spread = v;
}

void ModDelay::prepare(double sr, int channels, int maxFrames) {
    Module::prepare(sr, channels, maxFrames);
    rate.reset(sr, 50.0);
    depth.reset(sr, 50.0);
    delay.reset(sr, 50.0);
    feedback.reset(sr, 50.0);
    mix.reset(sr, 50.0);
    lines.assign(channels, DelayLine());
    for (DelayLine& line : lines) line.setup((int)(0.1 * sr));
    setupLfos(lfos, channels, "sine", spread);
}

void ModDelay::process(AudioBuffer& buffer, int frames) {
    for (int i = 0; i < frames; i++) {
        float r = rate.next();
        float dep = depth.next();
        float base = delay.next();
        float fb = feedback.next();
        float m = mix.next();
        for (int c = 0; c < numChannels; c++) {
            lfos[c].setFrequency(r, sampleRate);
            float ms = base + dep * lfos[c].next();
            float x = buffer.channel(c)[i];
            float delayed = lines[c].read(ms * 0.001f * (float)sampleRate);
            lines[c].write(x + fb * delayed);
            buffer.channel(c)[i] = x * (1.0f - m) + delayed * m;
        }
    }
}

Phaser::Phaser() {
    rate.setTarget(0.4f);
    feedback.setTarget(0.5f);
    mix.setTarget(0.5f);
}

void Phaser::setParam(const std::string& name, const std::string& value) {
    float v = toFloat(value);
    if (name == "rate") rate.setTarget(v);
    if (name == "feedback") feedback.setTarget(std::max(-0.95f, std::min(0.95f, v)));
    if (name == "mix") mix.setTarget(v);
    if (name == "stages") stages = std::max(1, (int)v);
    if (name == "low") low = v;
    if (name == "high") high = v;
}

void Phaser::prepare(double sr, int channels, int maxFrames) {
    Module::prepare(sr, channels, maxFrames);
    rate.reset(sr, 50.0);
    feedback.reset(sr, 50.0);
    mix.reset(sr, 50.0);
    setupLfos(lfos, channels, "sine", 90.0f);
    state.assign(channels, std::vector<float>(stages, 0.0f));
    last.assign(channels, 0.0f);
}

void Phaser::process(AudioBuffer& buffer, int frames) {
    for (int i = 0; i < frames; i++) {
        float r = rate.next();
        float fb = feedback.next();
        float m = mix.next();
        for (int c = 0; c < numChannels; c++) {
            lfos[c].setFrequency(r, sampleRate);
            float sweep = 0.5f + 0.5f * lfos[c].next();
            float freq = low * std::pow(high / low, sweep);
            float t = std::tan((float)PI * freq / (float)sampleRate);
            float a = (t - 1.0f) / (t + 1.0f);
            float x = buffer.channel(c)[i];
            float v = x + fb * last[c];
            for (int s = 0; s < stages; s++) {
                float y = a * v + state[c][s];
                state[c][s] = v - a * y;
                v = y;
            }
            last[c] = v;
            buffer.channel(c)[i] = x * (1.0f - m) + v * m;
        }
    }
}

Delay::Delay() {
    time.setTarget(350.0f);
    feedback.setTarget(0.4f);
    mix.setTarget(0.3f);
}

void Delay::setParam(const std::string& name, const std::string& value) {
    float v = toFloat(value);
    if (name == "time") time.setTarget(std::max(1.0f, std::min(2000.0f, v)));
    if (name == "feedback") feedback.setTarget(std::max(0.0f, std::min(0.95f, v)));
    if (name == "mix") mix.setTarget(v);
    if (name == "damping") damping = v;
    if (name == "pingpong") pingpong = v > 0.5f;
}

void Delay::prepare(double sr, int channels, int maxFrames) {
    Module::prepare(sr, channels, maxFrames);
    time.reset(sr, 100.0);
    feedback.reset(sr, 50.0);
    mix.reset(sr, 50.0);
    lines.assign(channels, DelayLine());
    for (DelayLine& line : lines) line.setup((int)(2.0 * sr) + 1);
    lowpass.assign(channels, 0.0f);
    wet.assign(channels, 0.0f);
}

void Delay::process(AudioBuffer& buffer, int frames) {
    float smoothing = (float)std::exp(-2.0 * PI * damping / sampleRate);
    bool cross = pingpong && numChannels == 2;
    for (int i = 0; i < frames; i++) {
        float samples = time.next() * 0.001f * (float)sampleRate;
        float fb = feedback.next();
        float m = mix.next();
        for (int c = 0; c < numChannels; c++) {
            float delayed = lines[c].read(samples);
            lowpass[c] = delayed + (lowpass[c] - delayed) * smoothing;
            wet[c] = lowpass[c];
        }
        for (int c = 0; c < numChannels; c++) {
            float x = buffer.channel(c)[i];
            float input = cross ? (c == 0 ? (buffer.channel(0)[i] + buffer.channel(1)[i]) * 0.5f : 0.0f) : x;
            float back = cross ? wet[1 - c] : wet[c];
            lines[c].write(input + fb * back);
            buffer.channel(c)[i] = x * (1.0f - m) + wet[c] * m;
        }
    }
}

int Delay::tailFrames() const {
    float fb = std::max(0.01f, std::min(0.95f, feedback.value()));
    float repeats = std::log(0.001f) / std::log(fb);
    return (int)std::min(10.0 * sampleRate, repeats * time.value() * 0.001 * sampleRate);
}
