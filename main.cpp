#include <iostream>
#include "RadixSort.h"
#include "test.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;
using namespace RadixSort;

void defaultTest()
{
	std::cout << " [[[ INT TEST ]]]\n\n";
	testInt(100000, 100, 1234, -999, 999, false);
	//testInt(10, 1, 1234, -999, 999, true);
	std::cout << "\n\n [[[ STRING TEST ]]]\n\n";
	testStr(10000, 10, 1234, 8, false);
	std::cout << "\n\n [[[ FLOAT TEST ]]]\n\n";
	testFloat(100000, 100, 1234, -999.9f, 999.9f, false);
}

void compareStdSort(size_t size, int seed, float minVal, float maxVal)
{

	cout << "Generating random data\n";
	float *dataRad = new float[size];
	float* dataStd = new float[size];
	srand(seed);
	for (size_t i = 0; i < size; i++)
	{
		dataStd[i] = dataRad[i] = minVal + ((maxVal - minVal) * (float)rand() / (float)RAND_MAX);
	}

	Sorter<float, IndexFloat> rad;

	cout << "Begin radix sort\n";
#ifdef _WIN32
	LARGE_INTEGER begRad, endRad, begStd, endStd, pFreq;
	QueryPerformanceFrequency(&pFreq);
	QueryPerformanceCounter(&begRad);
#endif

	//testFloat(size, 1, seed, minVal, maxVal, false);
	rad.sort(dataRad, size);
#ifdef _WIN32
	QueryPerformanceCounter(&endRad);
#endif

	cout << "Verifying\n";
	for (int i = 0; i < size - 1; i++)
	{
		if (dataRad[i] > dataRad[i + 1])
		{
			cout << "Fail!\n";
			return;
		}
	}


	cout << "Begin std::sort\n";
#ifdef _WIN32
	QueryPerformanceCounter(&begStd);
#endif

std::sort(dataStd, dataStd + size);

#ifdef _WIN32
	QueryPerformanceCounter(&endStd);
#endif
	cout << "Verifying\n";
	for (int i = 0; i < size - 1; i++)
	{
		if (dataStd[i] > dataStd[i + 1])
		{
			cout << "Fail!\n";
		}
	}

	float radTime = float(endRad.QuadPart - begRad.QuadPart) / pFreq.QuadPart;
	float radRate = size / radTime;
	float stdTime = float(endStd.QuadPart - begStd.QuadPart) / pFreq.QuadPart;
	float stdRate = size / stdTime;
	cout << "Results:\n   Radix sort: " << size << " elements in " << radTime << " seconds = " << radRate << " el/s\n"
		 << "  std::sort(): " << size << " elements in " << stdTime << " seconds = " << stdRate << " el/s\n";

}

int main()
{
	//testStr(10, 1, 11, 4, true);
	int option = 0;
	size_t size = 0;
	//size_t numIterations;
	int seed = 0;
	float minVal = 0;
	float maxVal = 0;
	do
	{
		cout << "Menu:\n 0. Exit\n 1. Default test.\n 2. Compare to std::sort\n 3. Manual test (float)\n 4. Repeat last manual test\n 5. Unsigned int test\n ...\n 9. Current dev test\n> ";

		cin >> option;
		switch (option)
		{
			case 0:
				break;
			case 1:
				defaultTest();
				break;
			case 2:
				compareStdSort(1000000, 1234, -999.0f, 100.0f);
				//compareStdSort(10000000);
				break;
			case 3:
				cout << "Data size: "; cin >> size;
				cout << "Random seed: "; cin >> seed;
				cout << "Minimum value: "; cin >> minVal;
				cout << "Maximum value: "; cin >> maxVal;
				compareStdSort(size, seed, minVal, maxVal);
				break;
			case 4:
				compareStdSort(size, seed, minVal, maxVal);
				break;
			case 5:
				testInt(10, 1, 11, 1, 10, true);
				break;
			case 9:
				compareStdSort(10000000, 11, -999, 999);
				break;
			default:
				break;
		}
	} while (option != 0);
	return 0;
}
