#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "ThirdParty/Noise_Squirrel/RawNoise.hpp"

int RandomNumberGenerator::RollRandomIntLessThan(int maxNotInclusive)
{
	return (Get1dNoiseUint(m_position++, m_seed) % maxNotInclusive);
	// return (rand() % maxNotInclusive);
}

int RandomNumberGenerator::RollRandomIntInRange(int minInclusive, int maxInclusive)
{
	int Result;
	int Range = maxInclusive - minInclusive + 1;//calculate the max to min, 1 for making (rand() % Range) could reach both minIclusive as well as maxInclusive
	Result = (rand() % Range) + minInclusive;
	return Result;
}
// both rand() and RAND_MAX is int
// the calculation will be zero if the int / int is less than 1
float RandomNumberGenerator::RollRandomFloatZeroToOne()
{
	return(float(rand()) / float(RAND_MAX));
}

float RandomNumberGenerator::RollRandomFloatInRange(float minInclusive, float maxInclusive)
{
	float result;
	float range = maxInclusive - minInclusive;
	result = RollRandomFloatZeroToOne() * range + minInclusive;
	return result;
}

float RandomNumberGenerator::RollRandomFloatInFloatRange(FloatRange range)
{
	return RollRandomFloatInRange(range.m_min, range.m_max);
}

bool RandomNumberGenerator::RollRandomChance(float probabilityForReturnTrue)
{
	float possibility = RollRandomFloatZeroToOne();
	if (possibility <= probabilityForReturnTrue)
	{
		return true;
	}
	else return false;
}

Vec2 RandomNumberGenerator::GetRandomPointInsideAABB2(AABB2 box)
{
	FloatRange widthRange(box.m_mins.x, box.m_maxs.x);
	FloatRange heightRange(box.m_mins.y, box.m_maxs.y);
	Vec2 result;
	result.x = RollRandomFloatInFloatRange(widthRange);
	result.y = RollRandomFloatInFloatRange(heightRange);
	return result;
}

Vec3 RandomNumberGenerator::GetRandomDirectionInCone(Vec3 direction, FloatRange range)
{
	// say we may want the direction to rotate 75 - 90 degrees in yaw and pitch
	// get random yaw and pitch
	float yawDeflection = RollRandomFloatInFloatRange(range);
	float r1 = RollRandomFloatInRange(0.f, 1.f);
	if (r1 >= 0.5f)
	{
		yawDeflection *= -1.f;
	}

	float pitchDeflection = RollRandomFloatInFloatRange(range);
	float r2 = RollRandomFloatInRange(0.f, 1.f);
	if (r2 >= 0.5f)
	{
		yawDeflection *= -1.f;
	}

	EulerAngles orientation = direction.GetOrientation();
	orientation.m_yawDegrees += yawDeflection;
	orientation.m_pitchDegrees += pitchDeflection;

	return Vec3::GetDirectionForYawPitch(orientation.m_yawDegrees, orientation.m_pitchDegrees);
}
