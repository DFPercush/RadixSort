



#include <string>
#include <stdexcept>
#include <algorithm>

#if defined(__GNUC__)
#if defined(_LP64) || defined(__LP64__)
#define RADIX_SORT_64_BIT
#endif
#elif defined(_WIN64)
#define RADIX_SORT_64_BIT
#endif

#ifdef RADIX_SORT_64_BIT
typedef long long mint; // machine int
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
	unsigned char operator()(float x, int i)
	{
		if (isLittleEndian()) { return ((unsigned char*)(&x))[sizeof(float) - i - 1]; }
		else { return ((unsigned char*)(&x))[i]; }
	}
};
class IndexDouble
{
public:
	unsigned char operator()(double x, int i)
	{
		if (isLittleEndian()) { return ((unsigned char*)(&x))[sizeof(double) - i - 1]; }
		else { return ((unsigned char*)(&x))[i]; }
	}
};

//class IndexDouble { public: unsigned char operator()(double x, int i) { return ((*(int64_t*)(&x)) >> ((sizeof(float) - i - 1) << 3)) & 0xFF; }};

template<typename T>
class IndexIntrinsic { public: unsigned char operator()(T x, int i) { return (x >> ((sizeof(T) - i - 1) << 3)) & 0xFF; }};
class IndexString{ public: unsigned char operator()(std::string s, int i){ return (i < s.size()) ? s[i] : 0; } };


//####################################################################################################
// Main radix sort class
template <typename T, class IndexerMSB0 = IndexIntrinsic<T>, class GetSize = GetSizeIntrinsic<T>>
class Sorter
{
public:

private:
	size_t *A, *B, *currentIndexBuffer;
	size_t allocSize;
	IndexerMSB0 getByte;
	GetSize getSize;
	int maxSize;
	bool negativeOverride;

	void init()
	{
		allocSize = 0;
		maxSize = 0;
		A = B = nullptr;
		negativeOverride = false;
	}

public:
	Sorter() 
	{
		init();
	}
	Sorter(signed int negative)
	{
		if (negative < 0) negativeOverride = true;
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
				if (std::is_floating_point<T>::value)
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

}; // class Sorter

} //namespace RadixSort

