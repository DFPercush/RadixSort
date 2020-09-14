
#define RADIX_SORT_TESTING
#include <iostream>
#include "RadixSort.h"
#include "test.h"


int main()
{
	std::cout << " [[[ INT TEST ]]]\n\n";
	testInt(100000, 100, 1234, -999, 999, false);
	//testInt(10, 1, 1234, -999, 999, true);
	std::cout << "\n\n [[[ STRING TEST ]]]\n\n";
	testStr(10000, 10, 1234, 8, false);
	std::cout << "\n\n [[[ FLOAT TEST ]]]\n\n";
	testFloat(100000, 100, 1234, -999.9f, 999.9f, false);
	return 0;
}
