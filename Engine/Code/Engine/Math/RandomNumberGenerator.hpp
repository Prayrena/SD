#pragma once
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include <cstdlib>

class RandomNumberGenerator
{
public:
	RandomNumberGenerator() {}
	explicit RandomNumberGenerator(unsigned int seed, int position = 0)
		: m_seed(seed)
		, m_position(position)
	{}

	void SetSeed(unsigned int seed)
	{
		m_seed = seed;
		m_position = 0;
	};

	int RollRandomIntLessThan(int maxNotInclusive);
	int RollRandomIntInRange(int minInclusive, int maxInclusive);

	float RollRandomFloatZeroToOne();
	float RollRandomFloatInRange(float minInclusive, float maxInclusive);
	float RollRandomFloatInFloatRange(FloatRange range);

	bool RollRandomChance(float probabilityForReturnTrue);

	Vec2 GetRandomPointInsideAABB2(AABB2 box);

	// 3D
	Vec3 GetRandomDirectionInCone(Vec3 direction, FloatRange range);

	unsigned int m_seed = 0;
	int m_position = 0;
};