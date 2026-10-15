#include "core/Utils.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (char ch : text) {
        if (ch == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current += ch;
        }
    }
    parts.push_back(current);
    return parts;
}

std::vector<std::string> splitWhitespace(const std::string& line) {
    std::vector<std::string> words;
    std::istringstream stream(line);
    std::string word;
    while (stream >> word) words.push_back(word);
    return words;
}

float toFloat(const std::string& text) {
    return (float)std::atof(text.c_str());
}

std::vector<double> toList(const std::string& text) {
    std::vector<double> values;
    for (const std::string& part : split(text, ',')) {
        if (!part.empty()) values.push_back(std::atof(part.c_str()));
    }
    return values;
}

float dbToGain(float db) {
    return std::pow(10.0f, db / 20.0f);
}

float gainToDb(float gain) {
    return 20.0f * std::log10(std::max(gain, 1e-9f));
}

int noteNumber(const std::string& text) {
    if (text.empty() || text == "-") return -1;
    if (std::isdigit((unsigned char)text[0])) return std::atoi(text.c_str());
    const int semitones[7] = {9, 11, 0, 2, 4, 5, 7};
    char letter = (char)std::toupper((unsigned char)text[0]);
    if (letter < 'A' || letter > 'G') return -1;
    int note = semitones[letter - 'A'];
    size_t pos = 1;
    if (pos < text.size() && text[pos] == '#') { note++; pos++; }
    else if (pos < text.size() && text[pos] == 'b') { note--; pos++; }
    int octave = pos < text.size() ? std::atoi(text.c_str() + pos) : 4;
    return 12 * (octave + 1) + note;
}

float noteToFrequency(int note) {
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}
