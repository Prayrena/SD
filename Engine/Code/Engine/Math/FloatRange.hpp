#pragma once

struct unsortedFloatRange
{
	unsortedFloatRange() = default;
	explicit unsortedFloatRange(float start, float end);
	~unsortedFloatRange() = default;

	float   GetRangeLength() const;
	float	EvaluateAtParametric(float fraction) const;

	float m_start = 0.f;
	float m_end = 0.f;
};

struct FloatRange
{
public:
	// Construction/Destruction
	FloatRange() = default;
	~FloatRange() = default;
	//add & when call yourself class
	FloatRange(const FloatRange& copyFrom);
	explicit FloatRange(float min, float max);		

public: // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
	float m_min = 0.f;
	float m_max = 0.f;

	// operators(const)
	bool operator == (FloatRange const& compare) const;
	bool operator != (FloatRange const& compare) const;

	// operators non-const
	void operator = (FloatRange const& copyFrom);
	void operator += (float const& addTo);
	 
	bool	IsInRange(float value) const;
	bool	IsOverLappingWith(FloatRange range) const;
	FloatRange GetOverlapRange(FloatRange rangeB);
	float   GetRangeLength() const;
	float	EvaluateAtParametric(float fraction) const;

	void	MoveFloatRange(float offset);

	bool  SetFromText(char const* text);

	// float	GetFirstIntersectValue(FloatRange secondRange) const;

	// static variable
	static FloatRange const ZERO;
	static FloatRange const ONE;
	static FloatRange const ZERO_TO_ONE;
};