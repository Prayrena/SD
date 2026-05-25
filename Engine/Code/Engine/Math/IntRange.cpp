#include "Engine/Math/IntRange.hpp"

IntRange const IntRange::ZERO = IntRange(0, 0);
IntRange const IntRange::ONE = IntRange(1, 1);
IntRange const IntRange::ZERO_TO_ONE = IntRange(0, 1);

IntRange::IntRange(const IntRange& copyFrom)
	: m_min(copyFrom.m_min), m_max(copyFrom.m_max)
{

}

IntRange::IntRange(int min, int max)
	: m_min(min), m_max(max)
{

}

bool IntRange::operator==(IntRange const& compare) const
{
	return (m_min == compare.m_min && m_max == compare.m_max);
}

bool IntRange::operator!=(IntRange const& compare) const
{
	return (m_min != compare.m_min || m_max != compare.m_max);
}

void IntRange::operator=(IntRange const& copyFrom)
{
	m_min = copyFrom.m_min;
	m_max = copyFrom.m_max;
}

bool IntRange::IsOnRange(int value) const
{
	if (value >= m_min && value <= m_max)
	{
		return true;
	}
	else return false;
}

bool IntRange::IsOverLappingWith(IntRange range) const
{
	if (m_max >= range.m_min || range.m_max >= m_min)
	{
		return true;
	}
	else return false;
}

