#pragma once

struct IntRange
{
public: // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
	int m_min = 0;
	int m_max = 0;

public:
	// Construction/Destruction
	~IntRange() {}
	IntRange() {}
	//add & when call yourself class
	IntRange(const IntRange& copyFrom);
	explicit IntRange(int min, int max);

	// operators(const)
	bool operator == (IntRange const& compare) const;
	bool operator != (IntRange const& compare) const;

	// operators non-const
	void operator = (IntRange const& copyFrom);

	bool IsOnRange(int value) const;
	bool IsOverLappingWith(IntRange range) const;

	// static variable
	static IntRange const ZERO;
	static IntRange const ONE;
	static IntRange const ZERO_TO_ONE;
};