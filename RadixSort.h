


#ifdef RADIX_SORT_TESTING
#include <iostream>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#endif
#endif

#include <string>
#include <stdexcept>


#if defined(__GNUC__)
#if defined(_LP64) || defined(__LP64__)
#define RADIX_SORT_64_BIT
#endif
#elif defined(_WIN64)
#define RADIX_SORT_64_BIT
#endif

#ifdef RADIX_SORT_64_BIT
typedef long long mint;
typedef unsigned long long umint;
#else
typedef int mint;
typedef unsigned int umint;
#endif

namespace RadixSort
{

template<typename T>
class GetSizeIntrinsic { public: int operator()(T x) { return sizeof(T); }};
class GetSizeString { public: int operator()(std::string s) { return (int)s.size(); }};

// Note: Indexer classes must accept indeces greater than length of item and return a valid byte, usually 0 in such a case.
template <typename T>
class IndexInt { public: unsigned char operator()(T x, int i) { return (x >> ((sizeof(T) - i - 1) << 3)) & 0xFF; }};
class IndexString{ public: unsigned char operator()(std::string s, int i){ return (i < s.size()) ? s[i] : 0; } };


//####################################################################################################
// Main radix sort class
template <typename T, class IndexerMSB0 = IndexInt<T>, class GetSize = GetSizeIntrinsic<T>>
class Sorter
{
public:

private:
	size_t *A, *B, *currentIndexBuffer;
	size_t allocSize;
	IndexerMSB0 getByte;
	GetSize getSize;
	int maxSize;

public:
	Sorter() 
	{
		allocSize = 0;
		maxSize = 0;
		A = B = nullptr;
	}
	~Sorter() { free(); }
	void preAlloc(size_t numElements)
	{
		if (A) delete [] A;
		if (B) delete[] B;
		//ivpB = new IndexValuePair[numElements]
		//B = new IndexValuePair[numElements];
		//A = new IndexValuePair[numElements];
		A = new size_t[numElements];
		B = new size_t[numElements];
		allocSize = numElements;
	}

	void free()
	{
		if (A) delete [] A;
		A = nullptr;
		if (B) delete [] B;
		B = nullptr;
		currentIndexBuffer = nullptr;
		allocSize = 0;
	}

	void growAlloc(size_t numElements)
	{
		size_t newSize = 0;
		if (allocSize < numElements)
		{
			if (allocSize * 2 > numElements && allocSize < 1000000000) newSize = allocSize * 2;
			else newSize = numElements;
			free();
			//ivpA = new IndexValuePair[newSize];
			//ivpB = new IndexValuePair[newSize];
			A = new size_t[newSize];
			B = new size_t[newSize];
			allocSize = newSize;
		}
	}

private:
	void buildView(const T* a, size_t numElements)
	{
		//size_t i = 0;
		growAlloc(numElements);

		maxSize = 0;
		size_t *tmp, *in, *out;
		in = A;
		out = B;
		mint i;
		size_t buckets[0x100];

		for (i = 0; (size_t)i < numElements; i++)
		{
			int sz = getSize(a[i]);
			if (sz > maxSize) maxSize = sz;
			in[i] = i;
		}
		for (int b = maxSize - 1; b >= 0; b--)
		{
			memset(buckets, 0, 0x100 * sizeof(size_t));
			for (i = 0; (size_t)i < numElements; i++)
			{
				buckets[getByte(a[in[i]], b)]++;
			}
			size_t cum = 0;
			for (i = 0; i < 0x100; i++)
			{
				cum += buckets[i];
				buckets[i] = cum;
			}
			for (i = numElements - 1; i >= 0; i--)
			{
				int iBucket = getByte(a[in[i]], b);
#ifdef _DEBUG
				if (buckets[iBucket] == 0) throw std::logic_error("Bug in radix sort: bucket underflow.");
#endif
				buckets[iBucket]--;
				out[buckets[iBucket]] = in[i];
			}

			tmp = in;
			in = out;
			out = tmp;
			currentIndexBuffer = in;
		} // for b
	} // buildView()

public:
	void sort(T* a, size_t numElements, bool keepMemoryResources = false)
	{
		buildView(a, numElements);
		size_t* in = currentIndexBuffer;
		size_t* out = nullptr;
		if (currentIndexBuffer == A) { out = B; }
		else if (currentIndexBuffer == B) { out = A; }
#ifdef _DEBUG
		else throw std::logic_error("State of index buffer unknown.");
#endif
		for (int i = 0; (size_t)i < numElements; i++)
		{
			out[in[i]] = i;
		}
		for (int i = 0; (size_t)i < numElements; i++)
		{
			//out[in[i]] = i;
			std::swap(a[in[i]], a[i]);
			//in[out[i]] = i;
			//int tempInI = in[i];
			in[out[i]] = in[i];
			out[in[i]] = out[i];
		}
		if (!keepMemoryResources) { free(); }
	}

	void view(const T* a, size_t *IndecesOut, size_t numElements, bool keepMemoryResources = false)
	{
		buildView(a, numElements);
		size_t* in = currentIndexBuffer;
		// "in" is really out, weird but that's because of buffer swapping
		memcpy(in, IndecesOut, numElements * sizeof(size_t));
		if (!keepMemoryResources) { free(); }
	}

	template<typename IntType>
	void viewCast(const T* a, IntType* IndecesOut, size_t numElements, bool keepMemoryResources = false)
	{
		static_assert(std::is_integral<IntType>::value, "Output array must be of an integer type.");
		buildView(a, numElements);
		size_t* in = currentIndexBuffer;
		for (size_t i = 0; i < numElements; i++)
		{
			IndecesOut[i] = static_cast<IntType>(in[i]);
		}
		if (!keepMemoryResources) { free(); }
	}

}; // class RadixSorter


#ifdef RADIX_SORT_TESTING

namespace Testing
{

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

	int *testData = new int[testSize];
	srand(testSeed);

	bool *testResults = new bool[numTests];

	std::cout << "size = " << testSize << std::endl;
	std::cout << "seed = " << testSeed << std::endl;
	std::cout << "minValue = " << minValue << std::endl;
	std::cout << "maxValue = " << maxValue << std::endl;


	if (!doPrintData) { std::cout << "\nProgress..."; std::cout.flush(); }
	for (int iTest = 0; iTest < numTests; iTest++)
	{
		if (!doPrintData) { clearLine(); std::cout << "Iteration " << iTest << " / " << numTests; std::cout.flush(); }
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
	
	delete [] testData;
	return nGood == numTests;
} // main()

bool testStr(size_t testSize, int numTests, int testSeed, int maxStrLength, bool doPrintData)
{
	const char validChars[] = "123456789QWERTYUPADFGHJKLZXCVBNMqwertyupadfghjkzxcvbnm";
	int validCharLen = strlen(validChars);

	Sorter<std::string, IndexString, GetSizeString> rad;
	std::string *testData = new std::string[testSize];

	srand(testSeed);

	bool* testResults = new bool[numTests];

	std::cout << "size = " << testSize << std::endl;
	std::cout << "seed = " << testSeed << std::endl;
	std::cout << "maxStrLength = " << maxStrLength << std::endl;

	if (!doPrintData) { std::cout << "\nProgress..."; std::cout.flush(); }
	for (int iTest = 0; iTest < numTests; iTest++)
	{
		if (!doPrintData) { clearLine(); std::cout << "Iteration " << iTest << " / " << numTests; std::cout.flush(); }
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

} // testing namespace

#endif  // testing define

}
