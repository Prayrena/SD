#include "Engine/Althorithm/LinkedList.hpp"
#include <stack>

bool PalindromeChecking::IsPalindrome_ON(Node* head)
{
	// put all values of the linked list into a container
	std::stack <int> nodeValue; // Stacks are a type of container adapter, specifically designed to operate in a LIFO context (last-in first-out)
	Node* nodePtr = head;
	while (nodePtr != nullptr) 
	{
		nodeValue.push(nodePtr->value);
		nodePtr = nodePtr->next;
	}

	// check the container value from backwards to the original linked list
	while (head != nullptr) 
	{
		if (head->value != nodeValue.top()) // std::stack.top(): Access next element
		{
			return false;
		}
		else 
		{
			head = head->next;
			nodeValue.pop(); // std::stack.pop(): Access next element
		}
	}
	return true;
}

//bool PalindromeChecking::IsPalindrome_O1(Node* head)
//{
//	if (head == nullptr || head->next == nullptr) {
//		return true;
//	}
//	Node* slowPtr = head;    // slow pointer
//	Node* fastPtr = head;    // fast pointer
//	while (fastPtr->next != nullptr && fastPtr->next->next != nullptr) 
//	{
//		slowPtr = slowPtr->next;
//		fastPtr = fastPtr->next->next;
//	}
//	fastPtr = slowPtr->next;    // fast pointer now is the first node in the second list
//	slowPtr->next = nullptr;
//	Node* n3 = nullptr;         // A ListNode to store the last node in the second list
//	while (fastPtr != nullptr) 
//	{
//		n3 = fastPtr->next;
//		fastPtr->next = slowPtr;
//		slowPtr = fastPtr;
//		fastPtr = n3;
//	}
//
//
//	n3 = slowPtr;    // here n1 is the last node in the second list, we need to store the last node to reconver
//	fastPtr = head;  // compare the first and the second list
//	while (slowPtr != nullptr && fastPtr != nullptr) {
//		if (slowPtr->val != fastPtr->val) {
//			return false;
//			break;
//		}
//		else {
//			slowPtr = slowPtr->next;
//			fastPtr = fastPtr->next;
//		}
//	}
//	slowPtr = n3->next;    // here we recover the second list to the origin order
//	n3->next = nullptr;   // the origin last node next poiter is nullptr
//	while (slowPtr != nullptr) {
//		fastPtr = slowPtr->next;
//		slowPtr->next = n3;
//		n3 = slowPtr;
//		slowPtr = fastPtr;
//	}
//	return true;
//}

