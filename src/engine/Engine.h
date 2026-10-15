#pragma once
#include <string>
#include <vector>
#include "engine/Graph.h"

struct Automation {
    double time;
    std::string node;
    std::string param;
    std::string value;
};

class Engine {
public:
    bool loadFile(const std::string& path);
    bool loadLines(const std::vector<std::string>& lines);
    bool run(const std::string& inputPath, const std::string& outputPath);
private:
    bool parseLine(const std::vector<std::string>& words);
    Module* buildModule(const std::vector<std::string>& words);
    void measure(const std::string& nodeName, const std::string& path);
    Graph graph;
    std::vector<std::vector<std::string>> definitions;
    std::vector<Automation> events;
    std::vector<std::vector<std::string>> measurements;
    double sampleRate = 48000.0;
    double duration = 5.0;
    double tail = -1.0;
    int channels = 0;
    int blockSize = 256;
    int bits = 16;
};
