#pragma once
#include <fstream>
#include <string>
#include <vector>
#include "core/AudioBuffer.h"

class WavReader {
public:
    bool open(const std::string& path);
    int read(AudioBuffer& buffer, int frames);
    int channels = 0;
    int sampleRate = 0;
    long long totalFrames = 0;
private:
    std::ifstream file;
    int bitsPerSample = 0;
    int format = 0;
    long long framesLeft = 0;
    std::vector<unsigned char> raw;
};

class WavWriter {
public:
    bool open(const std::string& path, int channels, int sampleRate, int bits);
    void write(const AudioBuffer& buffer, int frames);
    void close();
private:
    void writeHeader();
    std::ofstream file;
    int channels = 0;
    int sampleRate = 0;
    int bits = 16;
    long long framesWritten = 0;
    std::vector<unsigned char> raw;
};
