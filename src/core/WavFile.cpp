#include "core/WavFile.h"
#include <algorithm>
#include <cmath>
#include <cstring>

static unsigned int readU32(std::ifstream& f) {
    unsigned char b[4] = {0, 0, 0, 0};
    f.read((char*)b, 4);
    return b[0] | (b[1] << 8) | (b[2] << 16) | ((unsigned int)b[3] << 24);
}

static unsigned int readU16(std::ifstream& f) {
    unsigned char b[2] = {0, 0};
    f.read((char*)b, 2);
    return b[0] | (b[1] << 8);
}

static void writeU32(std::ofstream& f, unsigned int v) {
    unsigned char b[4] = {(unsigned char)v, (unsigned char)(v >> 8), (unsigned char)(v >> 16), (unsigned char)(v >> 24)};
    f.write((char*)b, 4);
}

static void writeU16(std::ofstream& f, unsigned int v) {
    unsigned char b[2] = {(unsigned char)v, (unsigned char)(v >> 8)};
    f.write((char*)b, 2);
}

static float decodeSample(const unsigned char* p, int bits, int format) {
    if (format == 3) {
        float v;
        std::memcpy(&v, p, 4);
        return v;
    }
    if (bits == 8) return (p[0] - 128) / 128.0f;
    if (bits == 16) return (short)(p[0] | (p[1] << 8)) / 32768.0f;
    if (bits == 24) {
        int v = p[0] | (p[1] << 8) | (p[2] << 16);
        if (v & 0x800000) v -= 0x1000000;
        return v / 8388608.0f;
    }
    int v = (int)(p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned int)p[3] << 24));
    return v / 2147483648.0f;
}

bool WavReader::open(const std::string& path) {
    file.open(path, std::ios::binary);
    if (!file) return false;
    char id[4];
    file.read(id, 4);
    readU32(file);
    char wave[4];
    file.read(wave, 4);
    if (std::string(id, 4) != "RIFF" || std::string(wave, 4) != "WAVE") return false;
    while (file.read(id, 4)) {
        std::string chunk(id, 4);
        unsigned int size = readU32(file);
        if (chunk == "fmt ") {
            format = readU16(file);
            channels = readU16(file);
            sampleRate = readU32(file);
            readU32(file);
            readU16(file);
            bitsPerSample = readU16(file);
            unsigned int used = 16;
            if (format == 0xFFFE && size >= 26) {
                readU16(file);
                readU16(file);
                readU32(file);
                format = readU16(file);
                used = 26;
            }
            file.seekg(size - used + (size & 1), std::ios::cur);
        } else if (chunk == "data") {
            if (channels == 0 || bitsPerSample == 0) return false;
            totalFrames = size / (channels * (bitsPerSample / 8));
            framesLeft = totalFrames;
            return format == 1 || (format == 3 && bitsPerSample == 32);
        } else {
            file.seekg(size + (size & 1), std::ios::cur);
        }
    }
    return false;
}

int WavReader::read(AudioBuffer& buffer, int frames) {
    int bytes = bitsPerSample / 8;
    int count = (int)std::min((long long)frames, framesLeft);
    raw.resize((size_t)count * channels * bytes);
    file.read((char*)raw.data(), raw.size());
    count = (int)(file.gcount() / (channels * bytes));
    int outChannels = buffer.numChannels();
    for (int i = 0; i < count; i++) {
        const unsigned char* frame = &raw[(size_t)i * channels * bytes];
        for (int c = 0; c < outChannels; c++) {
            float value = 0.0f;
            if (outChannels == 1) {
                for (int k = 0; k < channels; k++) value += decodeSample(frame + k * bytes, bitsPerSample, format);
                value /= channels;
            } else {
                value = decodeSample(frame + (c % channels) * bytes, bitsPerSample, format);
            }
            buffer.channel(c)[i] = value;
        }
    }
    framesLeft -= count;
    return count;
}

bool WavWriter::open(const std::string& path, int numChannels, int rate, int bitDepth) {
    file.open(path, std::ios::binary);
    if (!file) return false;
    channels = numChannels;
    sampleRate = rate;
    bits = bitDepth;
    framesWritten = 0;
    writeHeader();
    return true;
}

void WavWriter::writeHeader() {
    int bytes = bits / 8;
    unsigned int dataSize = (unsigned int)(framesWritten * channels * bytes);
    file.write("RIFF", 4);
    writeU32(file, 36 + dataSize + (dataSize & 1));
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    writeU32(file, 16);
    writeU16(file, bits == 32 ? 3 : 1);
    writeU16(file, channels);
    writeU32(file, sampleRate);
    writeU32(file, sampleRate * channels * bytes);
    writeU16(file, channels * bytes);
    writeU16(file, bits);
    file.write("data", 4);
    writeU32(file, dataSize);
}

void WavWriter::write(const AudioBuffer& buffer, int frames) {
    int bytes = bits / 8;
    raw.resize((size_t)frames * channels * bytes);
    unsigned char* p = raw.data();
    for (int i = 0; i < frames; i++) {
        for (int c = 0; c < channels; c++) {
            float v = buffer.channel(c)[i];
            if (bits == 32) {
                std::memcpy(p, &v, 4);
            } else {
                v = std::max(-1.0f, std::min(1.0f, v));
                int s = (int)std::lrint(v * (bits == 16 ? 32767.0f : 8388607.0f));
                p[0] = s & 0xFF;
                p[1] = (s >> 8) & 0xFF;
                if (bits == 24) p[2] = (s >> 16) & 0xFF;
            }
            p += bytes;
        }
    }
    file.write((char*)raw.data(), raw.size());
    framesWritten += frames;
}

void WavWriter::close() {
    if ((framesWritten * channels * (bits / 8)) % 2 == 1) file.put(0);
    file.seekp(0);
    writeHeader();
    file.close();
}
