#pragma once
#include "Engine/Math/Vec2.hpp"
#include <functional>

struct AABB2
{
public:
	Vec2 m_mins;
	Vec2 m_maxs;

public:
	AABB2() {}
	~AABB2() {}
	AABB2(AABB2 const& copyfrom);
	explicit AABB2(float minX, float minY, float maxX, float maxY); // explicit means that it does not allow implicit conversion, which means we could not input int to let if convert it for us
	explicit AABB2(Vec2 const& mins, Vec2 const& maxs);

	bool		IsPointInside(Vec2 const& point) const;

	// Accessors (const methods)
	Vec2 const	GetCenter() const;
	Vec2 const	GetDimensions() const;
	Vec2 const	GetNearestPoint(Vec2 const& referencePosition) const;
	Vec2 const	GetPointAtUV(Vec2 const& uv) const;
	Vec2 const	GetUVForPoint(Vec2 const& point) const;
	AABB2		GetBoxAtUVs(Vec2 uvMins, Vec2 uvMaxs) const;

	// Mutators (non-const methods)
	void		SetCenter(Vec2 const& newCenter);
	void		SetDimensions(Vec2 const& newDimensions);
	void		SetScale(float const& scaleX, float const& scaleY);
	void		SetUniformScale(float const& scale);

	void		Translate(Vec2 const& translationToApply);
	void		StretchToIncludePoint(Vec2 const& point);
	void		AddPadding(float xToAddOnBothSides, float yToAddToTopAndBottom);

	// operators (const)
	static AABB2 const ZERO_TO_ONE;
	AABB2 const operator*(float uniformScale) const;
	AABB2 const operator+(Vec2 offsetDist) const;
	bool operator==(AABB2 comparison) const;

	bool operator<(AABB2 const& other) const;
};

// Hash function for AABB2
struct AABB2Hash
{
	size_t operator()(const AABB2& aabb) const
	{
		std::hash<float> floatHasher;
		size_t h1 = floatHasher(aabb.m_mins.x);
		size_t h2 = floatHasher(aabb.m_mins.y);
		size_t h3 = floatHasher(aabb.m_maxs.x);
		size_t h4 = floatHasher(aabb.m_maxs.y);
		return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
	}
};
