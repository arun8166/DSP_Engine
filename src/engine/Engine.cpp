#include "engine/Engine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include "core/FFT.h"
#include "core/Utils.h"
#include "core/WavFile.h"
#include "engine/Factory.h"

static bool earlier(const Automation& a, const Automation& b) {
    return a.time < b.time;
}

bool Engine::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::printf("error: cannot open graph %s\n", path.c_str());
        return false;
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) lines.push_back(line);
    return loadLines(lines);
}

bool Engine::loadLines(const std::vector<std::string>& lines) {
    for (size_t i = 0; i < lines.size(); i++) {
        std::vector<std::string> words = splitWhitespace(lines[i]);
        if (words.empty() || words[0][0] == '#') continue;
        if (!parseLine(words)) {
            std::printf("error on line %d: %s\n", (int)i + 1, lines[i].c_str());
            return false;
        }
    }
    return true;
}

Module* Engine::buildModule(const std::vector<std::string>& words) {
    Module* module = createModule(words[2]);
    if (!module) return nullptr;
    for (size_t i = 3; i < words.size(); i++) {
        size_t eq = words[i].find('=');
        if (eq != std::string::npos) module->setParam(words[i].substr(0, eq), words[i].substr(eq + 1));
    }
    return module;
}

bool Engine::parseLine(const std::vector<std::string>& w) {
    const std::string& cmd = w[0];
    size_t n = w.size();
    if (cmd == "node" && n >= 3) {
        Module* module = buildModule(w);
        if (!module) return false;
        definitions.push_back(w);
        return graph.addNode(w[1], module);
    }
    if (cmd == "connect" && n >= 3) return graph.connect(w[1], w[2], n > 3 ? toFloat(w[3]) : 0.0f);
    if (cmd == "chain" && n >= 3) {
        for (size_t i = 1; i + 1 < n; i++) {
            if (!graph.connect(w[i], w[i + 1], 0.0f)) return false;
        }
        return true;
    }
    if (cmd == "at" && n >= 5) {
        Automation event = {toFloat(w[1]), w[2], w[3], w[4]};
        events.push_back(event);
        return graph.find(w[2]) >= 0;
    }
    if (cmd == "measure" && n >= 3) {
        measurements.push_back(w);
        return true;
    }
    if (n < 2) return false;
    float v = toFloat(w[1]);
    if (cmd == "samplerate") sampleRate = v;
    else if (cmd == "duration") duration = v;
    else if (cmd == "tail") tail = v;
    else if (cmd == "channels") channels = std::max(1, std::min(8, (int)v));
    else if (cmd == "blocksize") {
        blockSize = 16;
        while (blockSize < (int)v && blockSize < 8192) blockSize *= 2;
    } else if (cmd == "bits") bits = (v == 24 || v == 32) ? (int)v : 16;
    else return false;
    return true;
}

void Engine::measure(const std::string& nodeName, const std::string& path) {
    Module* module = nullptr;
    for (const std::vector<std::string>& def : definitions) {
        if (def[1] == nodeName) module = buildModule(def);
    }
    if (!module) {
        std::printf("warning: cannot measure unknown node %s\n", nodeName.c_str());
        return;
    }
    const int size = 16384;
    module->prepare(sampleRate, 1, blockSize);
    AudioBuffer block(1, blockSize);
    std::vector<Complex> response(size);
    for (int start = 0; start < size; start += blockSize) {
        block.clear();
        if (start == 0) block.channel(0)[0] = 1.0f;
        module->process(block, blockSize);
        for (int i = 0; i < blockSize && start + i < size; i++) response[start + i] = block.channel(0)[i];
    }
    delete module;
    FFT fft;
    fft.setup(size);
    fft.forward(response);
    std::ofstream out(path);
    out << "frequency_hz,magnitude_db,phase_deg\n";
    for (int k = 1; k <= size / 2; k++) {
        out << k * sampleRate / size << "," << 20.0 * std::log10(std::abs(response[k]) + 1e-12) << ","
            << std::arg(response[k]) * 180.0 / PI << "\n";
    }
    std::printf("[%s] frequency response -> %s\n", nodeName.c_str(), path.c_str());
}

bool Engine::run(const std::string& inputPath, const std::string& outputPath) {
    WavReader reader;
    bool hasInput = inputPath != "none";
    if (hasInput && !reader.open(inputPath)) {
        std::printf("error: cannot read %s (PCM or float WAV expected)\n", inputPath.c_str());
        return false;
    }
    if (hasInput) sampleRate = reader.sampleRate;
    if (channels == 0) channels = hasInput ? reader.channels : 2;
    if (!graph.compile()) {
        std::printf("error: graph contains a cycle\n");
        return false;
    }
    if (!graph.hasOutput()) std::printf("warning: nothing is connected to out\n");
    graph.prepare(sampleRate, channels, blockSize);
    for (const std::vector<std::string>& m : measurements) measure(m[1], m[2]);
    long long inputFrames = hasInput ? reader.totalFrames : (long long)(duration * sampleRate);
    long long tailFrames = tail >= 0.0 ? (long long)(tail * sampleRate) : graph.tailFrames();
    long long total = inputFrames + tailFrames;
    WavWriter writer;
    if (!writer.open(outputPath, channels, (int)sampleRate, bits)) {
        std::printf("error: cannot write %s\n", outputPath.c_str());
        return false;
    }
    std::stable_sort(events.begin(), events.end(), earlier);
    size_t nextEvent = 0;
    double busy = 0.0, worst = 0.0;
    float peak = 0.0f;
    for (long long pos = 0; pos < total; pos += blockSize) {
        while (nextEvent < events.size() && events[nextEvent].time * sampleRate <= pos) {
            const Automation& e = events[nextEvent++];
            graph.setParam(e.node, e.param, e.value);
        }
        AudioBuffer& in = graph.inputBuffer();
        in.clear();
        if (hasInput) reader.read(in, blockSize);
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        graph.process(blockSize);
        double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        busy += elapsed;
        worst = std::max(worst, elapsed);
        int frames = (int)std::min((long long)blockSize, total - pos);
        peak = std::max(peak, graph.outputBuffer().peak(frames));
        writer.write(graph.outputBuffer(), frames);
    }
    writer.close();
    graph.finish();
    double seconds = total / sampleRate;
    double budget = blockSize / sampleRate;
    std::printf("rendered %.2f s, %d ch, %.0f Hz -> %s\n", seconds, channels, sampleRate, outputPath.c_str());
    std::printf("graph: %d nodes sharing %d buffers of %d frames\n", graph.nodeCount(), graph.bufferCount(), blockSize);
    std::printf("speed: %.1fx real time, worst block %.3f ms of %.3f ms budget, output peak %.1f dBFS\n",
                seconds / std::max(busy, 1e-9), worst * 1000.0, budget * 1000.0, gainToDb(peak));
    return true;
}
