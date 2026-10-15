#pragma once
#include <string>
#include <vector>

const double PI = 3.14159265358979323846;

std::vector<std::string> split(const std::string& text, char delimiter);
std::vector<std::string> splitWhitespace(const std::string& line);
float toFloat(const std::string& text);
std::vector<double> toList(const std::string& text);
float dbToGain(float db);
float gainToDb(float gain);
int noteNumber(const std::string& text);
float noteToFrequency(int note);
