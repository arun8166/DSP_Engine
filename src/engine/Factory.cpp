#include "engine/Factory.h"
#include "modules/Dynamics.h"
#include "modules/FilterModules.h"
#include "modules/Modulation.h"
#include "modules/SpectralModules.h"
#include "modules/Synthesis.h"

Module* createModule(const std::string& type) {
    if (type == "gain") return new GainModule();
    if (type == "fir") return new FirModule();
    if (type == "iir") return new IirModule();
    if (type == "tf") return new TfModule();
    if (type == "eq") return new EqModule();
    if (type == "convolver" || type == "reverb") return new Convolver();
    if (type == "analyzer") return new Analyzer();
    if (type == "compressor" || type == "limiter" || type == "gate" || type == "expander") return new Dynamics(type);
    if (type == "tremolo") return new AmpMod(false);
    if (type == "ringmod") return new AmpMod(true);
    if (type == "chorus" || type == "flanger" || type == "vibrato") return new ModDelay(type);
    if (type == "phaser") return new Phaser();
    if (type == "delay") return new Delay();
    if (type == "oscillator") return new OscillatorModule();
    if (type == "envelope") return new EnvelopeModule();
    if (type == "synth") return new Synth();
    if (type == "passthrough") return new Passthrough();
    return nullptr;
}
