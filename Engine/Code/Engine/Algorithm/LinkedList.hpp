# pragma once

class Node
{
public:
	int value = 0;
	Node* next = nullptr;
};


//----------------------------------------------------------------------------------------------------------------------------------------------------
// 【 题目】 给定一个单链表的头节点head， 请判断该链表是否为回文结构。
// 【 例子】 1->2->1， 返回true； 1->2->2->1， 返回true； 15->6->15， 返回true；
//  1->2->3， 返回false。
// 【 例子】 如果链表长度为N， 时间复杂度达到O(N)， 额外空间复杂度达到O(1)

// space complexity O(N)
class PalindromeChecking
{
	bool IsPalindrome_ON(Node* head);
	bool IsPalindrome_O1(Node* head);
};
