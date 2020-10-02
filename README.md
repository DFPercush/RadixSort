

# RadixSort

A radix sort is a well known algorithm for sorting large amounts of data in linear time, i.e. `O(n)`. `*1`
Instead of comparing elements to each other many times, it uses some extra memory to arrange the data based on its raw
numerical value, in order of increasing significance. This makes it most suitable for simple unsigned integers; however, what I aimed
to achieve with this class was flexibility, to extend the idea to any type - strings, floats, even your own custom classes.
I'm sure this is not the first attempt to do so, but I think it turned out rather well and could be useful to somebody.
For very small data sets, say, under 100 elements, the overhead of setting it up may outweigh the savings in terms of array
accesses, and it does take a fair bit of memory, being an out-of-place algorithm, but in many cases, the performance of a radix
sort simply outclasses any other traditional sort which compares and swaps elements.

#### Performance update

I have implemented a direct sort method which does not use indexed references and thus performs much better
in cache. Currently, I'm beating VS 2019's `std::sort()` by roughly a factor of 2 for anywhere between 100k and 10M floats.
I will leave the old `view()` for utility purposes, but it remains quite slow in comparison.
For arrays of large structures, I recommend first building an array of pairs: the sortable value, and the
index to reference the original element. `sort()` that directly, then refer to the original data.
You can use the typedefs `Int/Float/DoublePairSorter` as a shortcut for this setup, along with an array type of
`std::pair<int/float/double, size_t>`. Or you could write your own custom indexer - see below.


## Quick reference:

Everything important is in `RadixSort.h`. The other files in the project are for testing. Copy it to your project and include it...

    #include "RadixSort.h"

Then declare an instance of the class.  
  
    RadixSort::Sorter<int> rad;

> There are several `typedef`s for common arrangements:  
 `IntSorter`, `FloatSorter`, `DoubleSorter`, `StringSorter`,
 `IntPairSorter`, `FloatPairSorter`, `DoublePairSorter`, and `StringPairSorter`.  
 The *pair versions assume a `.second` of type size_t for referencing the original data.

For floats:

    RadixSort::Sorter<float, RadixSorter::IndexFloat> rad; 
    rad.sort(data, count);


>Namespaces are omitted from here down, for readability. Be sure to include them, or `using namespace` beforehand.

For strings:  

    Sorter<string, IndexString, GetSizeString> rad;

To sort the original data:

    rad.sort(data, length);

To create a sorted array of indeces without modifying the original array:

    size_t myIndexBuffer = new size_t[myDataSize];
    rad.view(myData, myIndexBuffer, myDataSize);

^ This is much slower due to cache performance on non-contiguous memory ranges.

#### Extra template params

An indexer is necessary for non-integer types.
The indexer provides access to the sortable bytes of the element, starting with MSB, the **m**ost **s**ignificant **b**yte.
Indexers for common types are pre-packaged with the header: `IndexFloat`, `IndexDouble`, `IndexString`
(for `std::string`s), and `IndexInt<T>` for any native integral type, signed or unsigned.

In addition to the indexer, for strings and custom types we need another functor class to tell Sorter the size of an element. `*1`

### Custom types

To apply a radix sort to your type, you may need to define these 2 functor classes:
one to obtain the size of an element, and one to provide access to the sortable bytes (indexer). `*2`


    class MyCustomType {...};
    struct MyIndexer
    {
        unsigned char operator()(const MyCustomType& el, int i) { return something[i]; }
    } 
    struct MyGetSize
    {
        int operator()(const MyCustomType& el) { return el.data.size(); }
    }
    Sorter<MyCustomType, MyIndexer, MyGetSize> MyCustomSorter;

Sometimes, you can use the pre-packaged functors or even the default types.
If your type is binary equivalent to an integer type in memory (and small enough to fit in a single cpu word),
you can omit the indexer. If the size is just `sizeof(MyCustomType)` then you don't need a GetSize...
but if the meaningful data is stored in an externally allocated resource, you will need a GetSize... functor. `*1`

Does element size == `sizeof(YourType)` ?  
   Y: Omit the last template argument, or use GetSizeIntrinsic.  
   N: Define a custom GetSize... functor.

	
	
> Remember to use namespace `RadixSort` and/or `std` as necessary.
	  

#### Signed custom data
If your data can be interpreted as a signed value, you can manually flag the sorter to process negative values.  
For **integer**-like data, pass `-1` to the constructor.  
For **float**-like values pass `-1.0`.  
Otherwise, for unsigned or string-like data, the default constructor is fine.

    Sorter<...> MyIntLikeSorter(-1);
    Sorter<...> MyFloatLikeSorter(-1.0);
    Sorter<...> MyUnsignedSorter;
    
The reason floats must be treated specially is that their binary representation is the same whether positive or negative,
except for the sign bit itself (they don't "wrap around"). This means that they will actually be in reversed order after sorting
and must be swapped. If this behavior is what you need, initialize with the float-like constructor.

Then

    rad.sort(data, count);

or

    size_t MySortedIndeces[] = new size_t[MyDataLength];
    rad.view(


#### Footnotes

`*1` Be careful about element size, because the sort time depends on the length of the longest element.  
`O(n * MaxSize(n))`  
For fixed-length types, `MaxSize(n)` reduces to a constant, but it may be quite significant for variable length elements.
One abnormally long string can wreck the performance. In that case, you might be better off with std::sort()
or some other comparison sort, as string comparisons can `break` early when non-equal characters are encountered.  

Processing signed values involves a few extra steps: an `O(log2(n))` binary search for the boundary between negative and positive,
and for floating point types, an `O(n)` reverse operation on the negative partition.
Note that n for the negative partition is different than n for the entire data set, it may be 0 or everything.
In the worst case, these times add, not multiply, with the inherent `O(n)` behavior of the algorithm.
So, `O(n + log(n) + n)` --> `O(2n + log(n))`  which is still fundamentally `O(n)` at the end of the day.
    
`*2` The indexer must accept inputs beyond the length of the element if they are not all the same size.  
Index 0 should return the most significant byte, which may or may not include a sign bit.
You may pass by value or reference, either is fine for the template code.
Note that indeces here are given as `int`s, not `size_t`, because if each element is more than 2GB,
you're probably using the wrong algorithm.

