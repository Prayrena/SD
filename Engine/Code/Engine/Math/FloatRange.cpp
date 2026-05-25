#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <cmath>

FloatRange const FloatRange::ZERO = FloatRange(0.f, 0.f);
FloatRange const FloatRange::ONE = FloatRange(1.f, 1.f);
FloatRange const FloatRange::ZERO_TO_ONE = FloatRange(0.f, 1.f);

unsortedFloatRange::unsortedFloatRange(float start, float end)
	: m_start(start)
	, m_end(end)
{

}

float unsortedFloatRange::GetRangeLength() const
{
	return abs(m_end - m_start);
}

float unsortedFloatRange::EvaluateAtParametric(float fraction) const
{
	float value = Interpolate(m_start, m_end, fraction);
	return value;
}

FloatRange::FloatRange(const FloatRange& copyFrom)
	: m_min(copyFrom.m_min), m_max(copyFrom.m_max)
{

}

FloatRange::FloatRange(float min, float max)
{
	// the input min might smaller than the max, therefore we need to organize it first
	if (min <= max)
	{
		m_min = min;
		m_max = max;
	}
	else
	{
		m_min = max;
		m_max = min;
	}
}

bool FloatRange::operator==(FloatRange const& compare) const
{
	return (m_min == compare.m_min && m_max == compare.m_max);
}

bool FloatRange::operator!=(FloatRange const& compare) const
{
	return (m_min != compare.m_min || m_max != compare.m_max);
}

void FloatRange::operator=(FloatRange const& copyFrom) 
{
	m_min = copyFrom.m_min;
	m_max = copyFrom.m_max;
}

void FloatRange::operator+=(float const& addTo)
{
	m_min += addTo;
	m_max += addTo;
}

bool FloatRange::IsInRange(float value) const
{
	if (value > m_min && value < m_max)
	{
		return true;
	}
	else return false;
}

bool FloatRange::IsOverLappingWith(FloatRange range) const
{
	if (m_max >= range.m_min && range.m_max >= m_min)
	{
		if (m_max == m_min || range.m_min == range.m_max)
		{
			return false;
		}
		return true;
	}
	else return false;
}

FloatRange FloatRange::GetOverlapRange(FloatRange rangeB)
{
	if (!IsOverLappingWith(rangeB))
	{
		return FloatRange(); // no overlap range
	}
	else
	{
		FloatRange& RangeA = *this;
		if (RangeA.m_min >= rangeB.m_min) // rangeA.m_min - ?
		{
			if (RangeA.m_max >= rangeB.m_max) // rangeA.m_min - rangeB.m_max
			{
				return FloatRange(RangeA.m_min, rangeB.m_max);
			}
			else // rangeA.m_min - rangeA.m_max
			{
				return FloatRange(RangeA.m_min, RangeA.m_max);
			}
		}
		else // rangeB.m_min - ?
		{
			if (RangeA.m_max >= rangeB.m_max) // rangeB.m_min - rangeB.m_max
			{
				return FloatRange(rangeB.m_min, rangeB.m_max);
			}
			else // rangeB.m_min - rangeA.m_max
			{
				return FloatRange(rangeB.m_min, RangeA.m_max);
			}
		}
	}
}

float FloatRange::GetRangeLength() const
{
	return abs(m_max - m_min);
}

float FloatRange::EvaluateAtParametric(float fraction) const
{
	float value = Interpolate(m_min, m_max, fraction);
	return value;
}

void FloatRange::MoveFloatRange(float offset)
{
	m_min += offset;
	m_max += offset;
}

bool FloatRange::SetFromText(char const* text)
{
	Strings strings = SplitStringOnDelimiter(text, '~');
	int numStrings = (int)strings.size();
	if (numStrings != 2)
	{
		return false;
	}
	else
	{
		m_min = (float)atof(strings[0].c_str());//convert to number
		m_max = (float)atof(strings[1].c_str());
		return true; // for check if the set is successful
	}
}
