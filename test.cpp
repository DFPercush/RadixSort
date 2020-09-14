#include "RadixSort.h"
#include <iostream>

#include <iostream>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#endif

using namespace RadixSort;

void printData(int* data, size_t count, int numbersPerLine)
{
	for (int i = 0; i < count; i++)
	{
		if (i > 0 && i % numbersPerLine == 0) { std::cout << std::endl; }
		std::cout << data[i] << "  ";
	}
}

void printStrings(std::string* astr, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		std::cout << astr[i] << std::endl;
	}
}


void clearLine()
{
#if defined(WIN32) || defined(_WIN32) || defined(_WINDOWS_)
	static bool initialized = false;
	if (!initialized)
	{
		// Set output mode to handle virtual terminal sequences
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut == INVALID_HANDLE_VALUE)
		{
			return;
		}

		DWORD dwMode = 0;
		if (!GetConsoleMode(hOut, &dwMode))
		{
			return;
		}

		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (!SetConsoleMode(hOut, dwMode))
		{
			return;
		}
		initialized = true;
		//return;
	}
	//printf("\033K");
	printf("\x1b[K");
	printf("\x1b[G");
#else
	printf("\033[2K\r");
#endif
} // clearLine()

bool testInt(size_t testSize, int numTests, int testSeed, int minValue, int maxValue, bool doPrintData, int numbersPerLine = 10)
{
	//bool doPrintData = false;
	//int testSeed = 1234;
	//size_t testSize = 1000000000;
	//int minValue = 10000;
	//int maxValue = 99999;
	//int numTests = 1;

	//int numbersPerLine = 10;

	IndexInt<int> mi;
	int t0 = mi(127, 0);
	int t1 = mi(127, 1);
	int t2 = mi(127, 2);
	int t3 = mi(127, 3);

	int* testData = new int[testSize];
	srand(testSeed);

	bool* testResults = new bool[numTests];

	std::cout << "size = " << testSize << std::endl;
	std::cout << "seed = " << testSeed << std::endl;
	std::cout << "minValue = " << minValue << std::endl;
	std::cout << "maxValue = " << maxValue << std::endl;


	if (!doPrintData) { std::cout << "\nProgress..."; std::cout.flush(); }
	for (int iTest = 0; iTest < numTests; iTest++)
	{
		if (!doPrintData) { clearLine(); }
		std::cout << "Iteration " << iTest << " / " << numTests; std::cout.flush();
		for (int i = 0; i < testSize; i++)
		{
			testData[i] = minValue + (rand() % (maxValue - minValue));
		}
		if (doPrintData)
		{
			std::cout << "\nInput data:\n\n";
			printData(testData, testSize, numbersPerLine);
			std::cout << std::endl;
		}

		Sorter<int> rad;
		rad.sort(testData, testSize);

		if (doPrintData)
		{
			std::cout << "\nSorted data:\n\n";
			printData(testData, testSize, numbersPerLine);
			std::cout << std::endl;
		}

		bool good = true;
		for (int i = 0; i < testSize - 1; i++)
		{
			if (testData[i + 1] < testData[i])
			{
				good = false;
				break;
			}
		}
		testResults[iTest] = good;
		if (good && doPrintData) { std::cout << "\nOK\n"; }
		else if (doPrintData) std::cout << "\nFail!\n";
	} // for iTest

	int nGood = 0;

	std::cout << "\n=== SUMMARY ===\n";
	for (int gi = 0; gi < numTests; gi++)
	{
		if (testResults[gi]) { nGood++; }
		else
		{
			std::cout << "    Iteration " << gi << " failed!\n";
		}
	}
	if (nGood == numTests) { std::cout << "All good! (" << numTests << " iterations.)\n"; }
	else { std::cout << nGood << " / " << numTests << " passed.\n"; }
	std::cout << "size = " << testSize << std::endl;
	std::cout << "seed = " << testSeed << std::endl;
	std::cout << "minValue = " << minValue << std::endl;
	std::cout << "maxValue = " << maxValue << std::endl;

	delete[] testData;
	return nGood == numTests;
} // main()

bool testStr(size_t testSize, int numTests, int testSeed, int maxStrLength, bool doPrintData)
{
	const char validChars[] = "123456789QWERTYUPADFGHJKLZXCVBNMqwertyupadfghjkzxcvbnm";
	int validCharLen = (int)strlen(validChars);

	Sorter<std::string, IndexString, GetSizeString> rad;
	std::string* testData = new std::string[testSize];

	srand(testSeed);

	bool* testResults = new bool[numTests];

	std::cout << "size = " << testSize << std::endl;
	std::cout << "seed = " << testSeed << std::endl;
	std::cout << "maxStrLength = " << maxStrLength << std::endl;

	if (!doPrintData) { std::cout << "\nProgress..."; std::cout.flush(); }
	for (int iTest = 0; iTest < numTests; iTest++)
	{
		if (!doPrintData) { clearLine(); }
		std::cout << "Iteration " << iTest << " / " << numTests; std::cout.flush();
		for (int i = 0; i < testSize; i++)
		{
			int len = rand() % maxStrLength;
			testData[i].clear();
			for (int si = 0; si < len; si++)
			{
				testData[i] += validChars[rand() % validCharLen];
			}
		}
		if (doPrintData)
		{
			std::cout << "\nInput data:\n\n";
			printStrings(testData, testSize);
			std::cout << std::endl;
		}

		rad.sort(testData, testSize);

		if (doPrintData)
		{
			std::cout << "\nSorted data:\n\n";
			printStrings(testData, testSize);
			std::cout << std::endl;
		}

		bool good = true;
		for (int i = 0; i < testSize - 1; i++)
		{
			if (testData[i + 1] < testData[i])
			{
				good = false;
				break;
			}
		}
		testResults[iTest] = good;
		if (good && doPrintData) { std::cout << "\nOK\n"; }
		else if (doPrintData) std::cout << "\nFail!\n";
	} // for iTest

	int nGood = 0;

	std::cout << "\n=== SUMMARY ===\n";
	for (int gi = 0; gi < numTests; gi++)
	{
		if (testResults[gi]) { nGood++; }
		else
		{
			std::cout << "    Iteration " << gi << " failed!\n";
		}
	}
	if (nGood == numTests) { std::cout << "All good! (" << numTests << " iterations.)\n"; }
	else { std::cout << nGood << " / " << numTests << " passed.\n"; }
	std::cout << "size = " << testSize << std::endl;
	std::cout << "seed = " << testSeed << std::endl;
	std::cout << "maxStrLength = " << maxStrLength << std::endl;

	delete[] testData;
	return nGood == numTests;
}

void printFloats(float* data, size_t count, int numbersPerLine)
{
	for (int i = 0; i < count; i++)
	{
		if (i > 0 && i % numbersPerLine == 0) { std::cout << std::endl; }
		std::cout << data[i] << "  ";
	}
}

bool testFloat(size_t testSize, int numTests, int testSeed, float minValue, float maxValue, bool doPrintData, int numbersPerLine = 10)
{
	float* testData = new float[testSize];
	srand(testSeed);

	bool* testResults = new bool[numTests];

	std::cout << "size = " << testSize << std::endl;
	std::cout << "seed = " << testSeed << std::endl;
	std::cout << "minValue = " << minValue << std::endl;
	std::cout << "maxValue = " << maxValue << std::endl;


	if (!doPrintData) { std::cout << "\nProgress..."; std::cout.flush(); }
	for (int iTest = 0; iTest < numTests; iTest++)
	{
		if (!doPrintData) { clearLine(); }
		std::cout << "Iteration " << iTest << " / " << numTests; std::cout.flush();

		for (int i = 0; i < testSize; i++)
		{
			testData[i] = minValue + (((float)rand() / (float)RAND_MAX) * (maxValue - minValue));
		}
		if (doPrintData)
		{
			std::cout << "\nInput data:\n\n";
			printFloats(testData, testSize, numbersPerLine);
			std::cout << std::endl;
		}

		Sorter<float, IndexFloat> rad;
		rad.sort(testData, testSize);

		if (doPrintData)
		{
			std::cout << "\nSorted data:\n\n";
			printFloats(testData, testSize, numbersPerLine);
			std::cout << std::endl;
		}

		bool good = true;
		for (int i = 0; i < testSize - 1; i++)
		{
			if (testData[i + 1] < testData[i])
			{
				good = false;
				break;
			}
		}
		testResults[iTest] = good;
		if (good && doPrintData) { std::cout << "\nOK\n"; }
		else if (doPrintData) std::cout << "\nFail!\n";
	} // for iTest

	int nGood = 0;

	std::cout << "\n=== SUMMARY ===\n";
	for (int gi = 0; gi < numTests; gi++)
	{
		if (testResults[gi]) { nGood++; }
		else
		{
			std::cout << "    Iteration " << gi << " failed!\n";
		}
	}
	if (nGood == numTests) { std::cout << "All good! (" << numTests << " iterations.)\n"; }
	else { std::cout << nGood << " / " << numTests << " passed.\n"; }
	std::cout << "size = " << testSize << std::endl;
	std::cout << "seed = " << testSeed << std::endl;
	std::cout << "minValue = " << minValue << std::endl;
	std::cout << "maxValue = " << maxValue << std::endl;

	delete[] testData;
	return nGood == numTests;
} // main()


