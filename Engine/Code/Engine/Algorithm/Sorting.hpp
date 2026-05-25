#include <vector>

// all algorithm related to sorting will be listed in this hpp
using namespace std;

// argument for array sorting in C++
// void foo(int* x);
// void foo(int x[100]);
// void foo(int x[]);
// These three are different ways of declaring the same function
// They're all treated as taking an int * parameter, you can pass any size array to them

// void foo(int(&x)[100]);
// This only accepts arrays of 100 integers.You can safely use size of on x

// void foo(int& x[100]); // error
// This is parsed as an "array of references" - which isn't legal.

void Swap(int arr[], int i, int j);

//----------------------------------------------------------------------------------------------------------------------------------------------------
class MergeSortAlgorithm
{
public:
	void MergeSort(vector<int>& arr);

	void Process(vector<int>& arr, int L, int R);
	void Merge(vector<int>& arr, int L, int M, int R);
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
class SmallSumAlgorithm
{
public:
	int SmallSum(vector<int>& arr);

	int Process(vector<int>& arr, int L, int R);
	int Merge(vector<int>& arr, int L, int M, int R);
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
// Dutch National Flag problem
// Given N balls of color red, white or blue arranged in a line in random order. 
// You have to arrange all the balls such that the balls with the same colors are adjacent with the order of the balls, with the order of the colors being red, white and blue 
// (i.e., all red colored balls come first then the white colored balls and then the blue colored balls). 
 
// Input: {0, 1, 2, 0, 1, 2}
// Output: {0, 0, 1, 1, 2, 2}
// Explanation: {0, 0, 1, 1, 2, 2} has all 0s first, then all 1s and all 2s in last.
// Input : {0, 1, 1, 0, 1, 2, 1, 2, 0, 0, 0, 1}
// Output: {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2}
// Explanation: {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2} has all 0s first, then all 1s and all 2s in last

// Three way partitioning of an array around a given range
// Given an array and a range[lowVal, highVal], partition the array around the range such that array is divided in three parts.

// All elements smaller than lowVal come first.
// All elements in range lowVal to highVal come next.
// All elements greater than highVal appear in the end.
// The individual elements of the three sets can appear in any order.

// Input: arr[] = { 1, 14, 5, 20, 4, 2, 54, 20, 87, 98, 3, 1, 32 }, lowVal = 14, highVal = 20
// Output : arr[] = { 1, 5, 4, 2, 1, 3, 14, 20, 20, 98, 87, 32, 54 }
// Explanation: all elements which are less than 14 are present in the range of 0 to 6
// all elements which are greater than 14 and less than 20 are present in the range of 7 to 8
// all elements which are greater than 20 are present in the range of 9 to 12

// Input : arr[] = { 1, 14, 5, 20, 4, 2, 54, 20, 87, 98, 3, 1, 32 }, lowVal = 20, highVal = 20
// Output : arr[] = { 1, 14, 5, 4, 2, 1, 3, 20, 20, 98, 87, 32, 54 }

class ThreewayPartitionAlgorithm
{
public:
	void ThreewayPartition(vector<int>& arr, int lowVal, int highVal);
};

class QuickSortAlgorithm
{
public:
	void QuickSort(int arr[], int L, int R);
	int* Partition(int array[], int L, int R);
};
