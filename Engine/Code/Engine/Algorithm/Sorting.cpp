#include "Engine/Althorithm/Sorting.hpp"


void Swap(int arr[], int i, int j)
{
	int temp = arr[i];
	arr[i] = arr[j];
	arr[j] = temp;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void MergeSortAlgorithm::MergeSort(vector<int>& arr)
{
	if (arr.empty() || (int)(arr.size()) < 2)
	{
		return;
	}

	else Process(arr, 0, ((int)(arr.size()) - 1));
}

void MergeSortAlgorithm::Process(vector<int>& arr, int L, int R)
{
	if (L < R) {
		// M is the point where the array is divided into two sub arrays
		int M = L + ((R - L) >> 1);

		Process(arr, L, M);
		Process(arr, M + 1, R);

		// Merge the sorted sub arrays
		Merge(arr, L, M, R);
	}
}

int MergeSortAlgorithm::Merge(vector<int>& arr, int L, int M, int R)
{
	// create a new int vector the same size as original one
	vector<int>help;
	help.resize(R - L + 1);

	int i = 0,
		p1 = L,
		p2 = M + 1;

	// Until we reach either end of either L or M, pick larger among
	// elements L and M and place them in the correct position at A[p..r]
	while (p1 < L && p2 < R) 
	{
		help[i++] = arr[p1] < arr[p2] ? arr[p1++] : arr[p2++];
	}

	// When we run out of elements in either left or right part,
	// pick up the remaining elements and put into the array
	while (p1 <= M) 
	{
		arr[i++] = arr[p1++];

		// that equals to
		// arr[i] = L[p1];
		// i++;
		// p1++;
	}

	while (p2 <= R) 
	{
		arr[i++] = arr[p2++];
	}

	// replace the original array with temp int vector array
	for (int i = 0; i < int(help.size()); i++)
	{
		arr[i] = help[i];
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
int SmallSumAlgorithm::SmallSum(vector<int>& arr)
{
	if (arr.empty())
	{
		return 0;
	}

	return Process(arr, 0, arr.size() - 1);
}

// sorting while getting the small sum
int SmallSumAlgorithm::Process(vector<int>& arr, int L, int R)
{
	if (L == R)
	{
		return 0;
	}
	int mid = L + ((R - L) >> 1);

	//小和等于左边排好序求小和 + 右边排好序求小和 + merge求小和的结果
	return Process(arr, L, mid) + Process(arr, mid + 1, R) + Merge(arr, L, mid, R);
}

void SmallSumAlgorithm::Merge(vector<int>& arr, int L, int M, int R)
{
	// create a new int vector the same size as original one
	vector<int>help;
	help.resize(R - L + 1);

	int i = 0, 
		p1 = L, 
		p2 = M + 1, 
		res = 0; // use res to receive the small sum result

	while (p1 <= M && p2 <= R)
	{
		res += arr[p1] < arr[p2] ? (R - p2 + 1) * arr[p1] : 0; // use index of the right part to calculate the small sum
		
		// only copy the left part when it is smaller to the right
		// when it is equal or left is larger, we copy the right part
		help[i++] = arr[p1] < arr[p2] ? arr[p1++] : arr[p2++]; 
	}
	while (p1 <= M)//如果右边临界了，把左边加入到辅助数组中
	{
		help[i++] = arr[p1++];
	}
	while (p2 <= R)//如果左边临界了 把右边加入到辅助数组中，上下只会中一个情况
	{
		help[i++] = arr[p2++];
	}

	// modify the original int vector
	for (int k = 0; k < help.size(); k++)
	{
		arr[L + k] = help[k];
	}

	return res;
}

void ThreewayPartitionAlgorithm::ThreewayPartition(vector<int>& arr, int lowVal, int highVal)
{
	// Initialize next available positions for
	// smaller (than range) and greater elements
	int lowerZoneIndex = 0,
		higherZoneIndex = int(arr.size()) - 1,
		i = 0;

	// Traverse elements from left
	while(i < (int(arr.size()) - 1))
	{
		// If current element is smaller than range, put it on next available smaller position.
		if (arr[i] < lowVal) 
		{
			// if i and lowerZoneIndex are the same
			// meaning we don't need to sort the original int vector array
			if (i == lowerZoneIndex) 
			{
				lowerZoneIndex++;
				i++;
			}
			else swap(arr[i++], arr[lowerZoneIndex++]);
		}

		// If current element is greater than range, put it on next available greater position.
		else if (arr[i] > highVal)
		{
			swap(arr[i], arr[higherZoneIndex--]);
		}

		// If current element is in the value range, no need to swap but go to next value
		else i++;
	}
}

void QuickSortAlgorithm::QuickSort(int arr[], int L, int R)
{
	if (L < R) 
	{

		// find the pivot element and sort the original int vector array in such order:
		// elements smaller than pivot are on left of pivot
		// elements greater than pivot are on right of pivot
		int* boundaries = Partition(arr, L, R);

		// recursive call on the left part to the left of the middle part
		QuickSort(arr, L, boundaries[0]);

		// recursive call on the right part to the right of the middle part
		QuickSort(arr, boundaries[1] + 1, R);
	}
}

int* QuickSortAlgorithm::Partition(int arr[], int L, int R)
{
	// select the rightmost element as pivot
	int pivot = arr[R];

	// pointer for greater element
	int lowerZoneIndex = L - 1,
		higherZoneIndex = R;

	// traverse each element of the array
	// compare them with the pivot
	while (L < higherZoneIndex)
	{
		if (arr[L] < pivot)
		{
			Swap(arr, ++lowerZoneIndex, L++);
		}
		else if (arr[L] > arr[R])
		{
			Swap(arr, --higherZoneIndex, L); // because we swap and get a new array element, no need to advance the L
		}
		else // when two element are equal
		{
			L++;
		}
	}

	// swap the pivot element, which is the last one, to the center
	Swap(arr, higherZoneIndex, R);

	// return a 2 number array, including the boundaries of the lower and higher zones
	int* boundaries = new int[2];
	boundaries[0] = lowerZoneIndex;
	boundaries[1] = higherZoneIndex;

	return boundaries;
}

