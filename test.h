#pragma once

#include <string>

void printData(int* data, size_t count, int numbersPerLine);
void printStrings(std::string* astr, size_t count);
void clearLine();
bool testInt(size_t testSize, int numTests, int testSeed, int minValue, int maxValue, bool doPrintData, int numbersPerLine = 10);
bool testStr(size_t testSize, int numTests, int testSeed, int maxStrLength, bool doPrintData);
bool testFloat(size_t testSize, int numTests, int testSeed, float minValue, float maxValue, bool doPrintData, int numbersPerLine = 10);
