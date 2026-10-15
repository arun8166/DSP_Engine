#pragma once
#include <string>
#include <vector>
#include "core/AudioBuffer.h"
#include "core/Module.h"

struct Connection {
    int source;
    float gain;
};

struct Node {
    std::string name;
    Module* module;
    std::vector<Connection> inputs;
    int buffer;
};

class Graph {
public:
    Graph();
    ~Graph();
    bool addNode(const std::string& name, Module* module);
    int find(const std::string& name) const;
    bool connect(const std::string& from, const std::string& to, float gainDb);
    bool compile();
    void prepare(double sampleRate, int channels, int blockSize);
    void process(int frames);
    void setParam(const std::string& node, const std::string& name, const std::string& value);
    void finish();
    int tailFrames() const;
    AudioBuffer& inputBuffer() { return pool[nodes[0].buffer]; }
    const AudioBuffer& outputBuffer() const { return pool[nodes[1].buffer]; }
    int nodeCount() const { return (int)nodes.size(); }
    int bufferCount() const { return (int)pool.size(); }
    bool hasOutput() const { return !nodes[1].inputs.empty(); }
private:
    std::vector<Node> nodes;
    std::vector<int> order;
    std::vector<AudioBuffer> pool;
};
