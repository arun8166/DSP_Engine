#include "modules/Synthesis.h"
#include <algorithm>
#include <cmath>
#include "core/Utils.h"

const int MAX_VOICES = 8;

OscillatorModule::OscillatorModule() {
    freq.setTarget(440.0f);
    gain.setTarget(dbToGain(-12.0f));
}

void OscillatorModule::setParam(const std::string& name, const std::string& value) {
    if (name == "wave") osc.setWaveform(value);
    if (name == "freq") freq.setTarget(toFloat(value));
    if (name == "note") freq.setTarget(noteToFrequency(noteNumber(value)));
    if (name == "gain") gain.setTarget(dbToGain(toFloat(value)));
}

void OscillatorModule::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    freq.reset(rate, 50.0);
    gain.reset(rate, 50.0);
}

void OscillatorModule::process(AudioBuffer& buffer, int frames) {
    for (int i = 0; i < frames; i++) {
        osc.setFrequency(freq.next(), sampleRate);
        float v = osc.next() * gain.next();
        for (int c = 0; c < numChannels; c++) buffer.channel(c)[i] += v;
    }
}

void EnvelopeModule::setParam(const std::string& name, const std::string& value) {
    float v = toFloat(value);
    if (name == "attack") attack = v;
    if (name == "decay") decay = v;
    if (name == "sustain") sustain = v;
    if (name == "release") release = v;
    if (name == "period") period = std::max(0.01f, v);
    if (name == "gate") gate = v;
    env.setup(sampleRate, attack, decay, sustain, release);
}

void EnvelopeModule::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    env.setup(rate, attack, decay, sustain, release);
    counter = 0;
}

void EnvelopeModule::process(AudioBuffer& buffer, int frames) {
    long long periodSamples = std::max(1LL, (long long)(period * sampleRate));
    long long gateSamples = (long long)(gate * sampleRate);
    for (int i = 0; i < frames; i++) {
        long long position = counter % periodSamples;
        if (position == 0) env.noteOn();
        if (position == gateSamples) env.noteOff();
        counter++;
        float level = env.next();
        for (int c = 0; c < numChannels; c++) buffer.channel(c)[i] *= level;
    }
}

void Synth::setParam(const std::string& name, const std::string& value) {
    float v = toFloat(value);
    if (name == "wave") wave = value;
    if (name == "step") stepTime = std::max(0.01f, v);
    if (name == "length") length = std::max(0.01f, v);
    if (name == "attack") attack = v;
    if (name == "decay") decay = v;
    if (name == "sustain") sustain = v;
    if (name == "release") release = v;
    if (name == "cutoff") cutoff = v;
    if (name == "resonance") resonance = v;
    if (name == "envamount") envAmount = v;
    if (name == "fdecay") filterDecay = v;
    if (name == "detune") detune = v;
    if (name == "gain") gain = dbToGain(v);
    if (name == "loop") loop = v > 0.5f;
    if (name == "notes") {
        steps.clear();
        for (const std::string& step : split(value, ',')) {
            std::vector<int> chord;
            for (const std::string& note : split(step, '+')) {
                int number = noteNumber(note);
                if (number >= 0) chord.push_back(number);
            }
            steps.push_back(chord);
        }
    }
}

void Synth::prepare(double rate, int channels, int maxFrames) {
    Module::prepare(rate, channels, maxFrames);
    voices.assign(MAX_VOICES, Voice());
    for (Voice& voice : voices) {
        voice.osc1.setWaveform(wave);
        voice.osc2.setWaveform(wave);
        voice.osc2.setPhase(0.37);
        voice.amp.setup(rate, attack, decay, sustain, release);
        voice.filterEnv.setup(rate, attack, filterDecay, 0.0f, release);
        voice.filter.reset(1);
    }
    stepIndex = 0;
    counter = 0;
    nextStep = 0;
}

void Synth::noteOn(int note) {
    Voice* chosen = &voices[0];
    for (Voice& voice : voices) {
        if (!voice.amp.isActive()) {
            chosen = &voice;
            break;
        }
        if (voice.startedAt < chosen->startedAt) chosen = &voice;
    }
    float freq = noteToFrequency(note);
    chosen->osc1.setFrequency(freq, sampleRate);
    chosen->osc2.setFrequency(freq * std::pow(2.0f, detune / 1200.0f), sampleRate);
    chosen->filter.setCoeffs(designBiquad("lowpass", sampleRate, cutoff, resonance, 0.0));
    chosen->amp.noteOn();
    chosen->filterEnv.noteOn();
    chosen->startedAt = counter;
    chosen->offAt = counter + (long long)(length * stepTime * sampleRate);
}

void Synth::startStep() {
    if (stepIndex >= (int)steps.size()) {
        if (!loop || steps.empty()) return;
        stepIndex = 0;
    }
    for (int note : steps[stepIndex]) noteOn(note);
    stepIndex++;
}

void Synth::process(AudioBuffer& buffer, int frames) {
    for (int i = 0; i < frames; i++) {
        if (counter == nextStep) {
            startStep();
            nextStep = counter + (long long)(stepTime * sampleRate);
        }
        float sum = 0.0f;
        for (Voice& voice : voices) {
            if (counter == voice.offAt) {
                voice.amp.noteOff();
                voice.filterEnv.noteOff();
            }
            if (!voice.amp.isActive()) continue;
            float fenv = voice.filterEnv.next();
            if (counter % 32 == 0) {
                float f = std::min(cutoff + envAmount * fenv, (float)sampleRate * 0.45f);
                voice.filter.setCoeffs(designBiquad("lowpass", sampleRate, f, resonance, 0.0));
            }
            float raw = 0.5f * (voice.osc1.next() + voice.osc2.next());
            sum += voice.filter.process(raw, 0) * voice.amp.next();
        }
        counter++;
        for (int c = 0; c < numChannels; c++) buffer.channel(c)[i] += sum * gain;
    }
}
