#include "engine/Graph.h"
#include <algorithm>
#include <cmath>
#include "core/Utils.h"

Graph::Graph() {
    addNode("in", new Passthrough());
    addNode("out", new Passthrough());
}

Graph::~Graph() {
    for (Node& node : nodes) delete node.module;
}

bool Graph::addNode(const std::string& name, Module* module) {
    if (find(name) >= 0) {
        delete module;
        return false;
    }
    module->name = name;
    Node node = {name, module, std::vector<Connection>(), -1};
    nodes.push_back(node);
    return true;
}

int Graph::find(const std::string& name) const {
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i].name == name) return (int)i;
    }
    return -1;
}

bool Graph::connect(const std::string& from, const std::string& to, float gainDb) {
    int source = find(from);
    int target = find(to);
    if (source < 0 || target < 0 || target == 0) return false;
    Connection connection = {source, dbToGain(gainDb)};
    nodes[target].inputs.push_back(connection);
    return true;
}

bool Graph::compile() {
    int n = (int)nodes.size();
    std::vector<int> pending(n);
    std::vector<bool> done(n, false);
    for (int i = 0; i < n; i++) pending[i] = (int)nodes[i].inputs.size();
    order.clear();
    while ((int)order.size() < n) {
        int next = -1;
        for (int i = 0; i < n && next < 0; i++) {
            if (!done[i] && pending[i] == 0) next = i;
        }
        if (next < 0) return false;
        done[next] = true;
        order.push_back(next);
        for (int i = 0; i < n; i++) {
            for (const Connection& c : nodes[i].inputs) {
                if (c.source == next) pending[i]--;
            }
        }
    }
    std::vector<int> users(n, 0);
    for (int i = 0; i < n; i++) {
        for (const Connection& c : nodes[i].inputs) users[c.source]++;
    }
    users[1]++;
    std::vector<int> freeBuffers;
    int count = 0;
    for (int index : order) {
        Node& node = nodes[index];
        if (node.inputs.size() == 1 && users[node.inputs[0].source] == 1) {
            node.buffer = nodes[node.inputs[0].source].buffer;
        } else if (!freeBuffers.empty()) {
            node.buffer = freeBuffers.back();
            freeBuffers.pop_back();
        } else {
            node.buffer = count++;
        }
        for (const Connection& c : node.inputs) {
            users[c.source]--;
            if (users[c.source] == 0 && nodes[c.source].buffer != node.buffer) freeBuffers.push_back(nodes[c.source].buffer);
        }
        if (users[index] == 0) freeBuffers.push_back(node.buffer);
    }
    pool.resize(count);
    return true;
}

void Graph::prepare(double sampleRate, int channels, int blockSize) {
    for (AudioBuffer& buffer : pool) buffer.resize(channels, blockSize);
    for (Node& node : nodes) node.module->prepare(sampleRate, channels, blockSize);
}

void Graph::process(int frames) {
    for (int index : order) {
        Node& node = nodes[index];
        AudioBuffer& buffer = pool[node.buffer];
        if (index != 0) {
            bool inPlace = node.inputs.size() == 1 && nodes[node.inputs[0].source].buffer == node.buffer;
            if (inPlace) {
                if (node.inputs[0].gain != 1.0f) buffer.scale(frames, node.inputs[0].gain);
            } else {
                buffer.clear();
                for (const Connection& c : node.inputs) buffer.addFrom(pool[nodes[c.source].buffer], frames, c.gain);
            }
        }
        node.module->process(buffer, frames);
    }
}

void Graph::setParam(const std::string& node, const std::string& name, const std::string& value) {
    int index = find(node);
    if (index >= 0) nodes[index].module->setParam(name, value);
}

void Graph::finish() {
    for (int index : order) nodes[index].module->finish();
}

int Graph::tailFrames() const {
    int total = 0;
    for (const Node& node : nodes) total += node.module->tailFrames();
    return total;
}
