
#define RADIX_SORT_TESTING
#include "RadixSort.h"
#include <iostream>

int main()
{
	std::cout << " [[[ INT TEST ]]]\n\n";
	RadixSort::Testing::testInt(1000000, 10, 1234, 100, 999, false);
	std::cout << "\n\n [[[ STRING TEST ]]]\n\n";
	RadixSort::Testing::testStr(100000, 10, 1234, 8, false);
	return 0;
}
