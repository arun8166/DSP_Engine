#include <cstdio>
#include <string>
#include <vector>
#include "engine/Engine.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("usage: dspengine <input.wav|none> <output.wav> [graph.txt]\n");
        return 1;
    }
    Engine engine;
    bool loaded = false;
    if (argc > 3) {
        loaded = engine.loadFile(argv[3]);
    } else {
        std::vector<std::string> defaults;
        defaults.push_back("node hp iir type=highpass freq=30 order=4");
        defaults.push_back("node eq eq bands=lowshelf:120:2:0.7,peak:3000:-1.5:1.2,highshelf:10000:2:0.7");
        defaults.push_back("node comp compressor threshold=-18 ratio=3 attack=10 release=150 makeup=3");
        defaults.push_back("node lim limiter ceiling=-1");
        defaults.push_back("chain in hp eq comp lim out");
        loaded = engine.loadLines(defaults);
    }
    if (!loaded) return 1;
    return engine.run(argv[1], argv[2]) ? 0 : 1;
}
