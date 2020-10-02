
#include <string>
#include <stdexcept>
#include <algorithm>

#ifndef RADIX_SORT_NO_MMINTRIN // #define this if you get errors about _mm_prefetch or this header
#include <mmintrin.h>
#endif
#define CACHE_LINE_SIZE 64


#ifndef RADIX_SORT_32_BIT
#if defined(__GNUC__)
#if defined(_LP64) || defined(__LP64__)
#define RADIX_SORT_64_BIT
#endif
#elif defined(_WIN64)
#define RADIX_SORT_64_BIT
#endif
#endif // RADIX_SORT_32_BIT

#ifdef RADIX_SORT_64_BIT
typedef long long mint; // machine int
typedef unsigned long long umint;
//extern "C" void PreserveCache64();
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

//class IndexFloat { public: unsigned char operator()(float x, int i) { return ((*(int*)(&x)) >> ((sizeof(float) - i - 1) << 3)) & 0xFF; }};
union EndianTestUnion
{
	//EndianTestUnion(): s{0xFF00} {} // {s = (unsigned short)0xFF00;}
	unsigned short s;
	char c;
};
constexpr EndianTestUnion etu{(unsigned short)0xFF00};
constexpr bool isLittleEndian() { return etu.c == 0; }

class IndexFloat
{
public:
	inline unsigned char operator()(float x, int i)
	{
		// TODO: fast proper endianness
		//if (isLittleEndian()) { return ((unsigned char*)(&x))[sizeof(float) - i - 1]; }
		//else { return ((unsigned char*)(&x))[i]; }
		// just assume little endian for now
		return ((unsigned char*)(&x))[sizeof(float) - i - 1];
	}
};
class IndexDouble
{
public:
	inline unsigned char operator()(double x, int i)
	{
		//if (isLittleEndian()) { return ((unsigned char*)(&x))[sizeof(double) - i - 1]; }
		//else { return ((unsigned char*)(&x))[i]; }
		return ((unsigned char*)(&x))[sizeof(double) - i - 1];
	}
};

template<typename T>
class IndexIntrinsic { public: inline unsigned char operator()(const T& x, int i) { return (x >> ((sizeof(T) - i - 1) << 3)) & 0xFF; }};
class IndexString{ public: inline unsigned char operator()(const std::string& s, int i){ return (i < s.size()) ? s[i] : 0; } };

// Pair prefabs
template <typename T = int> struct GetSizeIntPair { constexpr size_t operator(T x)() { return sizeof(T); } };
struct GetSizeFloatPair { constexpr size_t operator()(const std::pair<float, size_t>& x) { return sizeof(float); } };
struct GetSizeDoublePair { constexpr size_t operator()(const std::pair<double, size_t>& x) { return sizeof(double); } };
struct GetSizeStringPair { inline size_t operator()(const std::pair<std::string, size_t>& x) { return x.first.size(); } };
template <typename T = int> struct IndexIntPair
{
	IndexInt<T> ind;
	inline unsigned char operator()(std::pair<T, size_t> p, int byte)
	{
		static_assert(std::is_integral<T>::value, "Template argument to IndexIntPair must be an integer type");
		return ind(p.first, byte);
	}
};
struct IndexFloatPair
{
	IndexFloat ind;
	inline unsigned char operator()(std::pair<float, size_t> p, int byte) { return ind(p.first, byte); }
};
struct IndexDoublePair
{
	IndexDouble ind;
	inline unsigned char operator()(std::pair<double, size_t> p, int byte) { return ind(p.first, byte); }
};
struct IndexStringPair
{
	IndexString ind;
	inline unsigned char operator()(const std::pair<std::string, size_t>& x, int i) { return x.first[i]; }
};




//####################################################################################################
// Main radix sort class
template <typename T, class IndexerMSB0 = IndexIntrinsic<T>, class GetSize = GetSizeIntrinsic<T>>
class Sorter
{
public:

private:
	size_t *A, *B, *currentIndexBuffer; // for view()
	size_t allocSizeA, allocSizeB;
	T* sortBuf; // for sort()
	size_t sortBufSize;
	IndexerMSB0 getByte;
	GetSize getSize;
	int maxSize;
	bool negativeOverride;
	bool floatOverride;

	void init()
	{
		allocSizeB = 0;
		maxSize = 0;
		A = B = nullptr;
		sortBuf = nullptr;
		sortBufSize = 0;
		negativeOverride = false;
		floatOverride = false;
	}

public:
	Sorter() 
	{
		init();
	}
	Sorter(signed int negative)
	{
		init();
		if (negative < 0) negativeOverride = true;
	}
	Sorter(double negative)
	{
		init();
		if (negative < 0)
		{
			negativeOverride = true;
			floatOverride = true;
		}
	}
	~Sorter() { free(); }
	void preAllocView(size_t numElements)
	{
		if (A) delete [] A;
		if (B) delete[] B;
		//ivpB = new IndexValuePair[numElements]
		//B = new IndexValuePair[numElements];
		//A = new IndexValuePair[numElements];
		A = new size_t[numElements];
		B = new size_t[numElements];
		allocSizeB = numElements;
	}

	void free()
	{
		if (A) delete [] A;
		A = nullptr;
		allocSizeA = 0;
		
		if (B) delete [] B;
		B = nullptr;
		allocSizeB = 0;

		if (sortBuf) delete[] sortBuf;
		sortBuf = nullptr;
		sortBufSize = 0;

		currentIndexBuffer = nullptr;
	}

	void growAllocView(size_t numElements)
	{
		size_t newSize = 0;
		size_t minSize = std::min(allocSizeA, allocSizeB);
		if (minSize < numElements)
		{
			if (minSize * 2 > numElements && minSize < 1000000000) newSize = minSize * 2;
			else newSize = numElements;
			free();
			//ivpA = new IndexValuePair[newSize];
			//ivpB = new IndexValuePair[newSize];
			A = new size_t[newSize];
			B = new size_t[newSize];
			allocSizeB = allocSizeA = newSize;
		}
	}

	void growAllocSort(size_t numElements)
	{
		size_t newSize = 0;
		if (sortBufSize < numElements)
		{
			if (sortBufSize * 2 > numElements && sortBufSize < 1000000000) newSize = sortBufSize * 2;
			else newSize = numElements;
			if (sortBuf != nullptr) delete[] sortBuf;
			//ivpA = new IndexValuePair[newSize];
			//ivpB = new IndexValuePair[newSize];
			sortBuf = new T[newSize];
			sortBufSize = newSize;
		}
	}

private:
	void buildView(const T* a, size_t numElements)
	{
		//size_t i = 0;
		growAllocView(numElements);

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
				//buckets[getByte(a[in[i]], b)]++; 
				// Let's break that down...
				size_t in_i = in[i];
				
				const auto& aini = a[in_i];
				unsigned char byteVal = getByte(aini, b); // <-- Hot path
				//const auto aval = a[in_i];
				//unsigned char byteVal = getByte(aval, b); // <-- Hot path
				// ^ That translates to 2 movss instructions. Not much to do about it at this point.
				// This makes me think moving the original data around might be faster, since a[in[i]]
				// causes the array to be accessed in a random order and constantly incur cache misses.
				// Welp, that's a brick in the face.

				buckets[byteVal]++;

				// Keep buckets in cache
#ifndef RADIX_SORT_NO_MMINTRIN
				char* cacheStartAddr = (char*)&buckets[0];
				char* cacheEndAddr = cacheStartAddr + sizeof(buckets) - 1;
				for (char* cacheAddr = cacheStartAddr; cacheAddr < cacheEndAddr; cacheAddr += CACHE_LINE_SIZE)
				{
					::_mm_prefetch(cacheAddr, 1); // second arg of 1 means we'll need this again later
				}
				::_mm_prefetch(cacheEndAddr, 1);
#endif
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
				out[buckets[iBucket]] = in[i]; // Hot path
			}

			tmp = in;
			in = out;
			out = tmp;
			currentIndexBuffer = in;

		} // for b


		if (negativeOverride || std::is_signed<T>::value)
		{
			// Move negative numbers to the beginning of the array and reverse order
			// At this point, they will be at the end, because sign bit is most significant
			// First, binary search for start of negatives.

			// Not searching for an exact element here, so I don't think this will work...
			//std::binary_search(in, in + numElements, 

			//size_t negStartMin = numElements >> 1;
			//size_t negStartMax = numElements - 1;
			size_t negStart = numElements >> 1;
			size_t negDelta = numElements >> 2;
			//while (negStartMax > negStartMin)
			//while ((getByte(a[in[negStart]], 0) & 0x80) == 0 || (getByte(a[in[negStart - 1]], 0) & 0x80) != 0 || negStart == 0 || negStart >= numElements)
			while ((negStart > 0) && (negStart < numElements) && !(
			(getByte(a[in[negStart]], 0) & 0x80) != 0 && (getByte(a[in[negStart - 1]], 0) & 0x80) == 0  
			))
			{
				//if (a[in[negStartMin]] < 0)
				//if (a[in[negStart]] < 0)
				//{
				//	//if (negStartMin == numElements - 1 || a[in[negStartMin + 1]] >= 0)
				//	if (negStart == numElements - 1 || a[in[negStart + 1]] >= 0)
				//	{
				//		negStart = negStartMin;
				//		break;
				//	}
				//	negStart -= negDelta;
				//}
				//else if (a[in[negStartMin]] >= 0) {negStartMin += negDelta + 1;}
				//else if (a[in[negStartMax]] < 0) {negStartMax += }

				if (getByte(a[in[negStart]], 0) & 0x80) negStart -= negDelta;
				else negStart += negDelta;
				negDelta >>= 1;
				if (negDelta < 1) negDelta = 1;
			}
			size_t negCount = numElements - negStart;
			if (negStart < numElements)
			{
				memcpy(out, in + negStart, negCount * sizeof(size_t));
				memmove(in + negCount, in, negStart * sizeof(size_t));
				memcpy(in, out, negCount * sizeof(size_t));
				if (floatOverride || std::is_floating_point<T>::value)
				{
					// Integer wrap-around/overflow of negative numbers preserves order here, but
					// floating point types will be reversed, because the binary value is the same
					// whether positive or negative, except the sign bit.
					// Basically floats are backwards.
					size_t negSwapLo = 0;
					size_t negSwapHi = negCount - 1;
					while (negSwapLo < negSwapHi) std::swap(in[negSwapLo++], in[negSwapHi--]);
				}
			}
		} // if negatives



	} // buildView()

public:
	void sort_old(T* a, size_t numElements, bool keepMemoryResources = false)
	{
		sortDirect(a, numElements, keepMemoryResources);
		return; 


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

	static void mv(T* dest, T* src, size_t count)
	{
		if (std::is_class<T>::value)
		{
			size_t i;
			if (dest < src) { for (i = 0; i < count; i++) dest[i  ] = std::move(src[i  ]); }
			else            { for (i = count; i > 0; i--) dest[i-1] = std::move(src[i-1]); }
		}
		else
		{
			memmove(dest, src, count * sizeof(T));
		}
	}

	void sort(T* data, size_t numElements, bool keepMemoryResources = false) //, M T::* value, bool hasNegative)
	{
		// TODO: observe the resource management options but we only need one buffer here 
		
		growAllocSort(numElements);
		T* src = data;
		T* dest = sortBuf; //new T[numElements];
		size_t buckets[0x100];
		int iByte; // sizeof(T);
		//size_t maxSize = 0;
		for (int i = 0; (size_t)i < numElements; i++)
		{
			int sz = getSize(data[i]);
			if (sz > maxSize) maxSize = sz;
		}


 		iByte = maxSize;
		while (iByte > 0)
		{
			iByte--;
			memset(buckets, 0, sizeof(buckets));
			size_t iData = 0;
			for (size_t iData = 0; iData < numElements; iData++)
			{
				//buckets[(getByte(src[iData], iByte) >> (iByte << 3)) & 0xFF] ++;
				buckets[getByte(src[iData], iByte)] ++;
			}
			size_t cum = 0; // cumulative total
			for (int iBucket = 0; iBucket < 0x100; iBucket++)
			{
				cum += buckets[iBucket];
				buckets[iBucket] = cum;

			}

			iData = numElements;
			while (iData > 0)
			{
				iData--;
				dest[--buckets[getByte(src[iData], iByte)]] = src[iData];

#ifndef RADIX_SORT_NO_MMINTRIN
				// If this block is giving you errors, it can be safely commented out.
				// This is an optimization to keep certain things in cache.
				if ((iData % (0x8000 / sizeof(T))) == 0)
				{
					char* cacheStartAddr = (char*)&buckets[0];
					char* cacheEndAddr = cacheStartAddr + sizeof(buckets) - 1;
					for (char* cacheAddr = cacheStartAddr; cacheAddr < cacheEndAddr; cacheAddr += CACHE_LINE_SIZE)
					{
						::_mm_prefetch(cacheAddr, 1); // second arg of 1 means we'll need this again later
					}
					::_mm_prefetch(cacheEndAddr, 1);
				}
#endif

			}
			std::swap(src, dest);
		} // iByte


		if (negativeOverride || std::is_signed<T>::value)
		{
			// Move negative numbers to the beginning of the array and reverse order
			// At this point, they will be at the end, because sign bit is most significant
			// First, binary search for start of negatives.

			size_t negStart = numElements >> 1;
			size_t negDelta = numElements >> 2;
			while ((negStart > 0) && (negStart < numElements) && !(
				(getByte(src[negStart], 0) & 0x80) != 0 && (getByte(src[negStart - 1], 0) & 0x80) == 0
				))
			{
				if (getByte(src[negStart], 0) & 0x80) negStart -= negDelta;
				else negStart += negDelta;
				negDelta >>= 1;
				if (negDelta < 1) negDelta = 1;
			}
			size_t negCount = numElements - negStart;
			if (negStart < numElements)
			{
				//memcpy(dest, src + negStart, negCount * sizeof(T));
				//memmove(src + negCount, src, negStart * sizeof(T));
				//memcpy(src, dest, negCount * sizeof(T));

				mv(dest, src + negStart, negCount);
				mv(src + negCount, src, negStart);
				mv(src, dest, negCount);

				//std::swap(src, dest);
				if (floatOverride || std::is_floating_point<T>::value)
				{
					// Integer wrap-around/overflow of negative numbers preserves order here, but
					// floating point types will be reversed, because the binary value is the same
					// whether positive or negative, except the sign bit.
					// Basically floats are backwards.
					size_t negSwapLo = 0;
					size_t negSwapHi = negCount - 1;
					while (negSwapLo < negSwapHi) std::swap(src[negSwapLo++], src[negSwapHi--]);
				}
			}
		} // if negatives


		if (data == src)
		{
			//delete [] dest;
		}
		else if (data == dest)
		{
			//memcpy(data, src, numElements * sizeof(T));
			mv(data, src, numElements);
			//delete [] src;
		}
		else throw std::logic_error("Unknown buffer");
		if (!keepMemoryResources) { free(); }
	} // sortDirect()


}; // class Sorter


//template <class T, typename M>


typedef Sorter<int> IntSorter;
typedef Sorter<float, IndexFloat> FloatSorter;
typedef Sorter<double, IndexDouble> DoubleSorter;
typedef Sorter<std::string, IndexString, GetSizeString> StringSorter;

typedef Sorter<std::pair<int, size_t>, IndexIntPair<int>, GetSizeIntPair<int>> IntPairSorter;
typedef Sorter<std::pair<float, size_t>, IndexFloatPair, GetSizeFloatPair> FloatPairSorter;
typedef Sorter<std::pair<double, size_t>, IndexDoublePair, GetSizeDoublePair> DoublePairSorter;
typedef Sorter<std::pair<std::string, size_t>, IndexStringPair, GetSizeStringPair> StringPairSorter;

} //namespace RadixSort

