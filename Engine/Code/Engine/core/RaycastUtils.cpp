#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/DebugRender.hpp"

using namespace std;

Raycast2D ConvertRaycast3DToRaycast2DOnXY(Raycast3D ray)
{
	Vec2 S(ray.rayStart.x, ray.rayStart.y);
	Vec2 i = Vec2(ray.rayFwdNormal.x, ray.rayFwdNormal.y).GetNormalized();
	Vec3 rayEnd = ray.rayStart + ray.rayFwdNormal * ray.rayDist;

	// project the start and end of the ray on XY and get 2D ray dist
	float ray2DDist = (Vec2(rayEnd) - Vec2(ray.rayStart)).GetLength();

	Raycast2D result;
	result.rayStartPos = S;
	result.rayFwdNormal = i;
	result.rayDist = ray2DDist;
	return result;
}

RaycastResult2D RaycastVsDisc2D(Vec2 RayStart, Vec2 rayFwdNormal, float RayDist, Vec2 discCenter, float discRadius)
{
	RaycastResult2D hitResult;
	hitResult.m_rayFwdNormal = rayFwdNormal;
	hitResult.m_rayStartPos = RayStart;
	hitResult.m_rayDist = RayDist;

	RaycastResult2D missResult;
	missResult.m_didImpact = false;
	missResult.m_impactDist = RayDist;
	missResult.m_impactPos = RayStart + rayFwdNormal * RayDist;
	missResult.m_impactNormal = rayFwdNormal;

	missResult.m_didExit = false;
	missResult.m_travelDistInShape = 0.f;
	missResult.m_exitPos = RayStart;
	missResult.m_exitNormal = RayStart;

	missResult.m_rayFwdNormal = rayFwdNormal;
	missResult.m_rayStartPos = RayStart;
	missResult.m_rayDist = RayDist;

	Vec2 E = RayStart + rayFwdNormal * RayDist;
	Vec2 S = RayStart;
	Vec2 C = discCenter;
	Vec2 i = rayFwdNormal;
	float R = discRadius;

	// first see if the center of the circle to the start pos minus radius is larger than the disc radius---too late
	Vec2 disp = RayStart - discCenter;
	if ( (disp.GetLength() - discRadius) > RayDist)
	{
		return missResult;
	}

	// check if the ray start point is inside the disc---too early
	if ( IsPointInsideDisc2D(RayStart, discCenter, discRadius) )
	{
		hitResult.m_didImpact = true;
		hitResult.m_impactDist = 0.f;
		hitResult.m_impactPos = RayStart;
		hitResult.m_impactNormal = -rayFwdNormal;

		if (IsPointInsideDisc2D(E, discCenter, discRadius)) // ray end is inside the disc
		{
			hitResult.m_didExit = false;
			hitResult.m_travelDistInShape = RayDist;
			hitResult.m_exitPos = E;
			hitResult.m_exitNormal = rayFwdNormal;
		}
		else // ray end is at the outside the disc
		{
			hitResult.m_didExit = true;

			Vec2 SC = C - S;
			float SCi = DotProduct2D(SC, i);
			float hSqr = SC.GetLengthSquared() - SCi * SCi;
			float b = sqrtf(R * R - hSqr);
			float rayTravelDistInDisc = SCi + b;

			hitResult.m_travelDistInShape = rayTravelDistInDisc;
			hitResult.m_exitPos = hitResult.m_impactPos + rayFwdNormal * rayTravelDistInDisc;
			hitResult.m_exitNormal = (hitResult.m_exitPos - C).GetNormalized();

		}
		return hitResult;
	}

	// transfer the disk center into local space of the raycast
	Vec2& iBasis = rayFwdNormal;
	Vec2 jBasis = iBasis.GetRotated90Degrees();
	Vec2 SC = discCenter - RayStart;
	float SC_j = DotProduct2D(SC, jBasis);
	if ( fabsf(SC_j) >= discRadius )// disk is too far from the ray
	{
		return missResult;
	}
	else
	{
		// get the bottom of the side of the triangle which made from the nearest point from
		float bottomSideLength = sqrtf( (discRadius * discRadius) - (SC_j * SC_j) );
		float SC_i= DotProduct2D(SC, iBasis);

		hitResult.m_impactDist = SC_i - bottomSideLength;
		if (hitResult.m_impactDist > 0.f && hitResult.m_impactDist < RayDist) // S is at outside of the disc and could penetrate the disc
		{
			hitResult.m_didImpact = true;
			hitResult.m_impactPos = RayStart + rayFwdNormal * hitResult.m_impactDist;
			Vec2 dispFromCenterToImpactPos = hitResult.m_impactPos - discCenter;
			hitResult.m_impactNormal = dispFromCenterToImpactPos.GetNormalized();

			if (IsPointInsideDisc2D(E, discCenter, discRadius)) // ray end is inside the disc
			{
				hitResult.m_didExit = false;
				hitResult.m_travelDistInShape = RayDist;
				hitResult.m_exitPos = E;
				hitResult.m_exitNormal = rayFwdNormal;
			}
			else // ray end is at the outside the disc
			{
				hitResult.m_didExit = true;
				hitResult.m_travelDistInShape = bottomSideLength * 2.f;
				hitResult.m_exitPos = hitResult.m_impactPos + rayFwdNormal * hitResult.m_travelDistInShape;
				hitResult.m_exitNormal = (hitResult.m_exitPos - C).GetNormalized();
			}
			return hitResult;
		}
		else
		{
			return missResult;
		}
	}
}

RaycastResult2D RaycastVsDisc2D(Raycast2D ray, Vec2 discCenter, float discRadius)
{
	return RaycastVsDisc2D(ray.rayStartPos, ray.rayFwdNormal, ray.rayDist, discCenter, discRadius);
}

RaycastResult2D RaycastVsDisc2D(Raycast2D ray, Disc2 disc)
{
	return RaycastVsDisc2D(ray, disc.m_center, disc.m_radius);
}

RaycastResult2D RaycastVSLineSegment2D(Vec2 RayStart, Vec2 rayForwardNormal, float rayDist, Vec2 lineSegStart, Vec2 lineSegEnd)
{
	Vec2& P = lineSegStart;
	Vec2& Q = lineSegEnd;
	Vec2& S = RayStart;
	Vec2& i = rayForwardNormal;
	Vec2 j = i.GetRotated90Degrees();
	float& m = rayDist;
	RaycastResult2D result;
	result.m_rayFwdNormal = i;
	result.m_rayDist = m;
	result.m_rayStartPos = S;

	// straddle test - see if the start and end of the line is on the different side of the raycast
	Vec2 SP = P - S;
	Vec2 SQ = Q - S;
	float SPj = DotProduct2D(SP, j);
	float SQj = DotProduct2D(SQ, j);

	// >=0 means they are on the same side of the raycast or just touching the raycast
	if (SPj * SQj >= 0)
	{
		// miss
		result.m_didImpact = false;
		result.m_impactDist = 0.f;
		result.m_impactPos = Vec2::ZERO;
		result.m_impactNormal = Vec2::ZERO;
		return result;
	}

	// calculate the intersection point of the raycast and get the impact distance
	float t = SPj / (SPj - SQj);
	Vec2 I = P + (Q - P) * t;
	Vec2 SI = I - S;
	float impactDist = DotProduct2D(SI, i);

	if (impactDist >= m)
	{
		// miss - too far
		result.m_didImpact = false;
		result.m_impactDist = impactDist;
		result.m_impactPos = I;
		result.m_impactNormal = Vec2::ZERO;
		return result;
	}
	else if (impactDist < 0.f)
	{
		// miss - too early
		result.m_didImpact = false;
		result.m_impactDist = impactDist;
		result.m_impactPos = I;
		result.m_impactNormal = Vec2::ZERO;
		return result;
	}

	// calculate the impact normal
	Vec2 PQ = Q - P;
	Vec2 normal = PQ.GetRotated90Degrees();
	if (SQj < 0.f)
	{
		normal *= -1.f;
	}
	result.m_didImpact = true;
	result.m_impactDist = impactDist;
	result.m_impactPos = I;
	result.m_impactNormal = normal;
	return result;
}

RaycastResult2D RaycastVSAABB2(Vec2 RayStart, Vec2 rayForwardNormal, float rayDist, AABB2 targetBox)
{
	// record the raycast info
	Vec2& S = RayStart;
	Vec2& i = rayForwardNormal;
	Vec2 RayEnd = RayStart + rayForwardNormal * rayDist;
	float& d = rayDist;

	RaycastResult2D hitResult;
	hitResult.m_rayFwdNormal = i;
	hitResult.m_rayDist = d;
	hitResult.m_rayStartPos = S;

	RaycastResult2D missResult;
	missResult.m_didImpact = false;
	missResult.m_impactDist = d;
	missResult.m_impactNormal = rayForwardNormal;
	missResult.m_rayFwdNormal = i;
	missResult.m_rayDist = d;
	missResult.m_rayStartPos = S;

	// potentially, the impact normal could be among four options
	Vec2 East = (Vec2(targetBox.m_maxs.x, targetBox.m_mins.y) - targetBox.m_mins).GetNormalized();
	Vec2 West = East * (-1.f);
	Vec2 North = (Vec2(targetBox.m_mins.x, targetBox.m_maxs.y) - targetBox.m_mins).GetNormalized();
	Vec2 South = North * (-1.f);

	// the box info
	float targetBoxHeight = targetBox.m_maxs.y - targetBox.m_mins.y;
	float targetBoxWidth = targetBox.m_maxs.x - targetBox.m_mins.x;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// make a AABB2 from the raycast, if the two box do not overlap, the raycast is not hit the target box
	// this is try to get rid of most cases
	// find the min for the BL, max for the top right
	FloatRange minXmaxX(RayStart.x, RayEnd.x);
	FloatRange minYmaxY(RayStart.y, RayEnd.y);
	AABB2 rayBox(Vec2(minXmaxX.m_min, minYmaxY.m_min), Vec2(minXmaxX.m_max, minYmaxY.m_max));

	if (!DoTwoAABB2Overlap(rayBox, targetBox)) // raycast miss
	{
		return missResult;
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// see if the raycast a X parallel or Y parallel
	// in such case, we are easy to see could the raycast hit the target box or not
	if (RayEnd.x == RayStart.x) // y parallel raycast
	{
		bool overlap;
		FloatRange rayY(RayStart.y, RayEnd.y);
		FloatRange targetBoxY(targetBox.m_mins.y, targetBox.m_maxs.y);
		overlap = rayY.IsOverLappingWith(targetBoxY);

		// see if the raycast hit the box
		if (overlap && RayStart.x < targetBox.m_maxs.x && RayStart.x > targetBox.m_mins.x) // hit
		{
			Vec2 impactBPos( RayStart.x, targetBox.m_mins.y);
			Vec2 impactTPos(RayStart.x, targetBox.m_maxs.y);
			// see if the raycast hit bottom or top
			if (IsPointInsideAABB2D(RayStart, targetBox)) // start in the target box
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = 0.f;
				hitResult.m_impactPos = RayStart;
				hitResult.m_impactNormal = i * (-1.f);

				if (IsPointInsideAABB2D(RayEnd, targetBox)) // the End is still inside the box
				{
					hitResult.m_didExit = false;
					hitResult.m_exitPos = RayEnd;
					hitResult.m_travelDistInShape = rayDist;
				}
				else // ray go through the target
				{
					hitResult.m_didExit = true;
					hitResult.m_travelDistInShape = targetBoxHeight;
					hitResult.m_exitPos = RayStart + Vec2(0.f, targetBoxHeight);

					if (rayForwardNormal.y > 0.f)
					{
						hitResult.m_exitNormal = North;
					}
					else
					{
						hitResult.m_exitNormal = South;
					}
				}
				return hitResult;
			}
			else // start outside of the target box
			{
				if (rayForwardNormal.y > 0.f) // hit bottom
				{
					hitResult.m_didImpact = true;
					hitResult.m_impactDist = (impactBPos - RayStart).GetLength();
					hitResult.m_impactPos = impactBPos;
					hitResult.m_impactNormal = South;

					if (IsPointInsideAABB2D(RayEnd, targetBox)) // the End is still inside the box
					{
						hitResult.m_didExit = false;
						hitResult.m_exitPos = RayEnd;
						hitResult.m_travelDistInShape = rayDist;
					}
					else // ray go through the target
					{
						hitResult.m_didExit = true;
						hitResult.m_travelDistInShape = targetBoxHeight;
						hitResult.m_exitPos = hitResult.m_impactPos + Vec2(0.f, targetBoxHeight);

						hitResult.m_exitNormal = North;
					}
					return hitResult;
				}
				else // hit Top
				{
					hitResult.m_didImpact = true;
					hitResult.m_impactDist = (impactTPos - RayStart).GetLength();
					hitResult.m_impactPos = impactTPos;
					hitResult.m_impactNormal = North;

					if (IsPointInsideAABB2D(RayEnd, targetBox)) // the End is still inside the box
					{
						hitResult.m_didExit = false;
						hitResult.m_exitPos = RayEnd;
						hitResult.m_travelDistInShape = rayDist;
					}
					else // ray go through the target
					{
						hitResult.m_didExit = true;
						hitResult.m_travelDistInShape = targetBoxHeight;
						hitResult.m_exitPos = hitResult.m_impactPos + Vec2(0.f, targetBoxHeight * (-1.f));

						hitResult.m_exitNormal = South;
					}

					return hitResult;
				}

			}
		}
		else // miss
		{
			return missResult;
		}
	}
	else if (RayEnd.y == RayStart.y) // X parallel raycast
	{
		bool overlap;
		FloatRange rayX(RayStart.x, RayEnd.x);
		FloatRange targetBoxX(targetBox.m_mins.x, targetBox.m_maxs.x);
		overlap = rayX.IsOverLappingWith(targetBoxX);

		// see if the raycast hit the box on the left or right
		Vec2 impactLPos(targetBox.m_mins.x, RayStart.y);
		Vec2 impactRPos(targetBox.m_maxs.x, RayStart.y);

		if (overlap && RayStart.y > targetBox.m_mins.y && RayStart.y < targetBox.m_maxs.y) // hit or miss
		{
			// see if the raycast hit left or right
			// the start of the raycast might in the AABB2
			if (IsPointInsideAABB2D(RayStart, targetBox))
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = 0.f;
				hitResult.m_impactPos = RayStart;
				hitResult.m_impactNormal = i * (-1.f);

				if (IsPointInsideAABB2D(RayEnd, targetBox)) // the End is still inside the box
				{
					hitResult.m_didExit = false;
					hitResult.m_exitPos = RayEnd;
					hitResult.m_travelDistInShape = rayDist;
				}
				else // ray go through the target
				{
					hitResult.m_didExit = true;
					hitResult.m_travelDistInShape = targetBoxHeight;
					hitResult.m_exitPos = RayStart + Vec2(targetBoxWidth, 0.f);

					if (rayForwardNormal.x > 0.f) // go to the right
					{
						hitResult.m_exitNormal = East;
					}
					else // go to the left
					{
						hitResult.m_exitNormal = West;
					}
				}

				return hitResult;
			}
			if (rayForwardNormal.x >= 0) // go to the right
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = (impactLPos - RayStart).GetLength();
				hitResult.m_impactPos = impactLPos;
				hitResult.m_impactNormal = West;

				if (IsPointInsideAABB2D(RayEnd, targetBox)) // the End is still inside the box
				{
					hitResult.m_didExit = false;
					hitResult.m_exitPos = RayEnd;
					hitResult.m_travelDistInShape = rayDist;
				}
				else // ray go through the right
				{
					hitResult.m_didExit = true;
					hitResult.m_travelDistInShape = targetBoxHeight;
					hitResult.m_exitPos = hitResult.m_impactPos + Vec2(targetBoxWidth, 0.f);

					hitResult.m_exitNormal = East;
				}

				return hitResult;
			}
			else // hit Right
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = (impactRPos - RayStart).GetLength();
				hitResult.m_impactPos = impactRPos;
				hitResult.m_impactNormal = East;

				if (IsPointInsideAABB2D(RayEnd, targetBox)) // the End is still inside the box
				{
					hitResult.m_didExit = false;
					hitResult.m_exitPos = RayEnd;
					hitResult.m_travelDistInShape = rayDist;
				}
				else // ray go through the left
				{
					hitResult.m_didExit = true;
					hitResult.m_travelDistInShape = targetBoxHeight;
					hitResult.m_exitPos = hitResult.m_impactPos + Vec2(targetBoxWidth * (-1.f), 0.f);

					hitResult.m_exitNormal = West;
				}

				return hitResult;
			}
		}
		else // miss
		{
			return missResult;
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// for the general case, we are going to discuss the target box's overlap on raycast in X and Y
	// the base unit take raycast as base unit, all calculation is based on ray start and ray end
	// therefore we talked the case when the raycast a X parallel or Y parallel in advance so we would not divided by 0
	float oneUnitOverXRange = 1.f / (RayEnd.x - RayStart.x);
	float oneUnitOverYRange = 1.f / (RayEnd.y - RayStart.y);

	// get the range from the ray start to the end
	float boxMinInX = (targetBox.m_mins.x - RayStart.x) * oneUnitOverXRange;
	float boxMaxInX = (targetBox.m_maxs.x - RayStart.x) * oneUnitOverXRange;
	float boxMinInY = (targetBox.m_mins.y - RayStart.y) * oneUnitOverYRange;
	float boxMaxInY = (targetBox.m_maxs.y - RayStart.y) * oneUnitOverYRange;

	bool rangeIsOverlapped = false;

	FloatRange Xrange(boxMinInX, boxMaxInX);
	FloatRange Yrange(boxMinInY, boxMaxInY);
	float Xmin = Xrange.m_min;
	float Xmax = Xrange.m_max;
	float Ymin = Yrange.m_min;
	float Ymax = Yrange.m_max;

	rangeIsOverlapped = Xrange.IsOverLappingWith(Yrange);

	if (rangeIsOverlapped) //hit
	{	 
		Vec2 rayDisp = RayEnd - RayStart;
		if (Ymin <= Xmin) // the first intersect float is Xmin: left or right
		{
			// the raycast start is inside the AABB2
			if (IsPointInsideAABB2D(RayStart, targetBox))
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = 0.f;
				hitResult.m_impactPos = RayStart;
				hitResult.m_impactNormal = i * (-1.f);

				// the raycast end is inside the AABB2
				if (IsPointInsideAABB2D(RayEnd, targetBox))
				{
					hitResult.m_didExit = false;
					hitResult.m_exitPos = RayEnd;
					hitResult.m_travelDistInShape = rayDist;
					hitResult.m_exitNormal = rayForwardNormal;
				}
				else // the raycast end is out side of the AABB2
				{
					hitResult.m_didExit = true;

					if (Ymax <= Xmax ) // exit top or bottom // overlap range is Xmin - Ymax
					{
						hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Ymax;
						hitResult.m_travelDistInShape = rayDist * (Ymax - Xmin);
						if (rayForwardNormal.y > 0.f)
						{
							hitResult.m_exitNormal = North;
						}
						else
						{
							hitResult.m_exitNormal = South;
						}
					}
					else  // exit either right or left // overlap range is Xmin - Xmax
					{
						hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Xmax;
						hitResult.m_travelDistInShape = rayDist * (Xmax - Xmin);
						if (rayForwardNormal.x > 0.f)
						{
							hitResult.m_exitNormal = East;
						}
						else
						{
							hitResult.m_exitNormal = West;
						}
					}
				}
				return hitResult;
			}
			else // the ray start is outside of the box, the first intersect float is Xmin: left or right
			{
				if (rayForwardNormal.x > 0.f) // impact on the left
				{
					Vec2  hitPoint = Xmin * rayDisp + RayStart;
					hitResult.m_didImpact = true;
					hitResult.m_impactDist = (hitPoint - RayStart).GetLength();
					hitResult.m_impactPos = hitPoint;
					hitResult.m_impactNormal = West;

					if (IsPointInsideAABB2D(RayEnd, targetBox)) // end point is in the box
					{
						hitResult.m_didExit = false;
						hitResult.m_exitPos = RayEnd;
						hitResult.m_travelDistInShape = rayDist;
						hitResult.m_exitNormal = rayForwardNormal;
					}
					else // end point go through the box
					{
						if (Ymax <= Xmax) // go through from top or bottom, overlap range is Xmin - Ymax
						{
							hitResult.m_didExit = true;
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Ymax;
							hitResult.m_travelDistInShape = rayDist * (Ymax - Xmin);

							if (rayForwardNormal.y > 0.f)
							{
								hitResult.m_exitNormal = North;
							}
							else
							{
								hitResult.m_exitNormal = South;
							}
						}
						else // overlap range is Xmin - Xmax
						{
							hitResult.m_didExit = true;
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Xmax;
							hitResult.m_travelDistInShape = rayDist * (Xmax - Xmin);

							hitResult.m_exitNormal = East;
						}
					}
					return hitResult;
				}
				else // impact on the right
				{
					Vec2  hitPoint = Xmin * rayDisp + RayStart;
					hitResult.m_didImpact = true;
					hitResult.m_impactDist = (hitPoint - RayStart).GetLength();
					hitResult.m_impactPos = hitPoint;
					hitResult.m_impactNormal = East;

					if (IsPointInsideAABB2D(RayEnd, targetBox)) // end point is in the box
					{
						hitResult.m_didExit = false;
						hitResult.m_exitPos = RayEnd;
						hitResult.m_travelDistInShape = rayDist;
						hitResult.m_exitNormal = rayForwardNormal;
					}
					else // end point go through the box
					{
						hitResult.m_didExit = true;

						if (Ymax <= Xmax) // overlap range is Xmin - Ymax
						{
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Ymax;
							hitResult.m_travelDistInShape = rayDist * (Ymax - Xmin);

							if (rayForwardNormal.y > 0.f)
							{
								hitResult.m_exitNormal = North;
							}
							else
							{
								hitResult.m_exitNormal = South;
							}
						}
						else // overlap range is Xmin - Xmax
						{
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Xmax;
							hitResult.m_travelDistInShape = rayDist * (Xmax - Xmin);

							hitResult.m_exitNormal = West;
						}
					}
					return hitResult;
				}
			}
		}
		else // the first intersect float is Ymin:
		{
			if (IsPointInsideAABB2D(RayStart, targetBox)) // ray start inside the box
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = 0.f;
				hitResult.m_impactPos = RayStart;
				hitResult.m_impactNormal = i * (-1.f);

				if (rayForwardNormal.y < 0.f) // impact on top
				{
					if (IsPointInsideAABB2D(RayEnd, targetBox))
					{
						hitResult.m_didExit = false;
						hitResult.m_exitPos = RayEnd;
						hitResult.m_travelDistInShape = rayDist;
						hitResult.m_exitNormal = rayForwardNormal;

						return hitResult;
					}
					else
					{
						hitResult.m_didExit = true;

						if (Ymax <= Xmax) // overlap range is Ymin - Ymax
						{
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Ymax;
							hitResult.m_travelDistInShape = rayDist * (Ymax - Ymin);
							hitResult.m_exitNormal = South;
						}
						else  // overlap range is Ymin - Xmax
						{
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Xmax;
							hitResult.m_travelDistInShape = rayDist * (Xmax - Ymin);
							if (rayForwardNormal.x < 0.f)
							{
								hitResult.m_exitNormal = West;
							}
							else
							{
								hitResult.m_exitNormal = East;
							}
						}
					}
					return hitResult;
				}
				else // impact on the bottom
				{
					if (IsPointInsideAABB2D(RayEnd, targetBox))
					{
						hitResult.m_didExit = false;
						hitResult.m_exitPos = RayEnd;
						hitResult.m_travelDistInShape = rayDist;
						hitResult.m_exitNormal = rayForwardNormal;

						return hitResult;
					}
					else
					{
						hitResult.m_didExit = true;

						if (Ymax <= Xmax) // overlap range is Ymin - Ymax
						{
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Ymax;
							hitResult.m_travelDistInShape = rayDist * (Ymax - Ymin);
							hitResult.m_exitNormal = North;
						}
						else  // overlap range is Ymin - Xmax
						{
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Xmax;
							hitResult.m_travelDistInShape = rayDist * (Xmax - Ymin);

							if (rayForwardNormal.x < 0.f)
							{
								hitResult.m_exitNormal = West;
							}
							else
							{
								hitResult.m_exitNormal = East;
							}
						}
						return hitResult;
					}
				}
			}
			else // ray start outside the box
			{
				Vec2  hitPoint = Ymin * rayDisp + RayStart;

				if (rayForwardNormal.y < 0.f) // impact on top
				{
					hitResult.m_didImpact = true;
					hitResult.m_impactDist = (hitPoint - RayStart).GetLength();
					hitResult.m_impactPos = hitPoint;
					hitResult.m_impactNormal = North;

					if (IsPointInsideAABB2D(RayEnd, targetBox))
					{
						hitResult.m_didExit = false;
						hitResult.m_exitPos = RayEnd;
						hitResult.m_travelDistInShape = rayDist;
						hitResult.m_exitNormal = rayForwardNormal;

						return hitResult;
					}
					else
					{
						hitResult.m_didExit = true;

						if (Ymax <= Xmax) // overlap range is Ymin - Ymax
						{
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Ymax;
							hitResult.m_travelDistInShape = rayDist * (Ymax - Ymin);
							hitResult.m_exitNormal = South;
						}
						else  // overlap range is Ymin - Xmax
						{
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Xmax;
							hitResult.m_travelDistInShape = rayDist * (Xmax - Ymin);
							if (rayForwardNormal.x < 0.f)
							{
								hitResult.m_exitNormal = West;
							}
							else
							{
								hitResult.m_exitNormal = East;
							}
						}
					}
					return hitResult;
				}
				else // impact on the bottom
				{
					hitResult.m_didImpact = true;
					hitResult.m_impactDist = (hitPoint - RayStart).GetLength();
					hitResult.m_impactPos = hitPoint;
					hitResult.m_impactNormal = South;

					if (IsPointInsideAABB2D(RayEnd, targetBox))
					{
						hitResult.m_didExit = false;
						hitResult.m_exitPos = RayEnd;
						hitResult.m_travelDistInShape = rayDist;
						hitResult.m_exitNormal = rayForwardNormal;

						return hitResult;
					}
					else
					{
						hitResult.m_didExit = true;

						if (Ymax <= Xmax) // overlap range is Ymin - Ymax
						{
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Ymax;
							hitResult.m_travelDistInShape = rayDist * (Ymax - Ymin);
							hitResult.m_exitNormal = North;
						}
						else  // overlap range is Ymin - Xmax
						{
							hitResult.m_exitPos = RayStart + rayForwardNormal * rayDist * Xmax;
							hitResult.m_travelDistInShape = rayDist * (Xmax - Ymin);

							if (rayForwardNormal.x < 0.f)
							{
								hitResult.m_exitNormal = West;
							}
							else
							{
								hitResult.m_exitNormal = East;
							}
						}
					}
					return hitResult;
				}
			}
		}
		
	}
	else // miss
	{
		return missResult;
	}
}

RaycastResult2D RaycastVSAABB2(Raycast2D ray, AABB2 targetBox)
{
	return RaycastVSAABB2(ray.rayStartPos, ray.rayFwdNormal, ray.rayDist, targetBox);
}

RaycastResult2D RaycastVSConvexPoly2(Raycast2D ray, ConvexPoly2 convex)
{
	// for convexPoly we first transform it to convex hull, then we get the result
	ConvexHull2* convexHull = new ConvexHull2(convex);
	return RaycastVSConvexHull2(ray, *convexHull);
}

RaycastResult2D RaycastVSConvexHull2(Raycast2D ray, ConvexHull2 convex)
{
	// RaycastResult2D missResult;
	Vec2& S = ray.rayStartPos;
	Vec2& I = ray.rayFwdNormal;
	float& d = ray.rayDist;
	Vec2 E = S + I * d;

	RaycastResult2D missResult;
	missResult.m_didImpact = false;
	missResult.m_impactDist = 0.f;
	missResult.m_impactNormal = ray.rayFwdNormal;
	missResult.m_impactPos = Vec2::ZERO;
	missResult.m_rayFwdNormal = ray.rayFwdNormal;
	missResult.m_rayDist = ray.rayDist;
	missResult.m_rayStartPos = ray.rayStartPos;
	missResult.m_didExit = false;
	missResult.m_travelDistInShape = 0.f;
	missResult.m_exitPos = E;
	missResult.m_exitNormal = ray.rayFwdNormal;

	// for every the bounding planes and their raycast results
	// we only need these info
	struct planeRaycastResult
	{
		Vec2 impactPos = Vec2::ZERO;
		bool isEnter = true; // false means exit
		bool didImpact = false;
		float dist_rayStart_hit = 0.f;

		// Define a comparison operator for sorting based on the dist from ray start to impact pos
		// for the map structure to use
		bool operator<(const planeRaycastResult& other) const {
			return dist_rayStart_hit < other.dist_rayStart_hit;
		}
	};

	// we are going to store them in a unordered_map
	vector<planeRaycastResult> planeRaycastResults;	// the first represent the index of the plane
	bool missAllPlanes = true;	

	for (Plane2 plane : convex.m_boundingPlanes)
	{
		RaycastResult2D result = RaycastVSPlane2(ray, plane);
		planeRaycastResult* compactResult = new planeRaycastResult();	// todo: this might have dangling pointers
		
		// if the result is hit, we need to check the impact normal and plane normal 
		// to determine whether it is a entering point or exiting point
		if (result.m_didImpact)
		{
			missAllPlanes = false;
			compactResult->didImpact = true;
			compactResult->dist_rayStart_hit = result.m_impactDist;
			compactResult->impactPos = result.m_impactPos;

			// determine whether this is a enter impact point or exit impact point
			if (DotProduct2D(plane.m_normal, I) > 0) // exit impact point
			{
				compactResult->isEnter = false;
			}
			else // enter impact point
			{
				compactResult->isEnter = true;
			}
		}
		else
		{
			compactResult->didImpact = false;
			compactResult->dist_rayStart_hit = d;
			compactResult->impactPos = S;

		}

		// store the compact result by plane index
		planeRaycastResults.push_back(*compactResult);
	}

	// if the raycast misses all planes, check if the start is inside the convex
	if (missAllPlanes)
	{
		if (IsPointBehindAllPlane2(S, convex.m_boundingPlanes)) // case 1
		{
			RaycastResult2D hitResult;
			hitResult.m_didImpact = true;
			hitResult.m_impactDist = 0.f;
			hitResult.m_impactNormal = ray.rayFwdNormal * (-1.f);
			hitResult.m_impactPos = S;
			hitResult.m_rayFwdNormal = ray.rayFwdNormal;
			hitResult.m_rayDist = ray.rayDist;
			hitResult.m_rayStartPos = ray.rayStartPos;
			hitResult.m_didExit = false;
			hitResult.m_travelDistInShape = d;
			hitResult.m_exitPos = E;
			hitResult.m_exitNormal = ray.rayFwdNormal;

			return hitResult;
		}
		else return missResult;
	}
	
	// we need to arrange two maps recording the enter points and exit point
	// we need to sort them by the their dist to the ray start pos
	map<planeRaycastResult, Plane2*> enterPts;	// we need the last enter point
	map<planeRaycastResult, Plane2*> exitPts;	// and the first exit point
	for (int i = 0; i < (int)convex.m_boundingPlanes.size(); ++i)
	{
		planeRaycastResult const& result = planeRaycastResults[i];

		if (result.didImpact)
		{
			if (result.isEnter) enterPts[result] = &convex.m_boundingPlanes[i];
			if (!result.isEnter) exitPts[result] = &convex.m_boundingPlanes[i];
		}
	}

	// if we got some impact result, we are going to discuss a series of situations
	// no enter but exits, meaning the ray start inside the convex and first exit is the leave point
	if (enterPts.empty() && !exitPts.empty())	// case 2
	{
		if (IsPointBehindAllPlane2(S, convex.m_boundingPlanes)) 
		{
			float startToExitDist = exitPts.begin()->first.dist_rayStart_hit;
			Vec2 firstExitPt = exitPts.begin()->first.impactPos;
			Plane2* exitPlane = exitPts.begin()->second;

			RaycastResult2D hitResult;
			hitResult.m_didImpact = true;
			hitResult.m_impactDist = 0.f;
			hitResult.m_impactNormal = ray.rayFwdNormal * (-1.f);	// we need to know which plane the last enter point is on
			hitResult.m_impactPos = S;
			hitResult.m_rayFwdNormal = ray.rayFwdNormal;
			hitResult.m_rayDist = ray.rayDist;
			hitResult.m_rayStartPos = ray.rayStartPos;
			hitResult.m_didExit = true;
			hitResult.m_travelDistInShape = startToExitDist;
			hitResult.m_exitPos = S + I * startToExitDist;
			hitResult.m_exitNormal = exitPlane->m_normal;

			return hitResult;
		}
		else return missResult;
	}

	// no exit pts only enter points, meaning the last enter point is the impact point
	// the raycast ending point is inside the convex
	if (!enterPts.empty() && exitPts.empty())	// case 3
	{
		if (IsPointBehindAllPlane2(E, convex.m_boundingPlanes))
		{
			// the last enter pt is the impact pos
			float startToEnterDist = prev(enterPts.end())->first.dist_rayStart_hit;
			Vec2 lastEnterPt = prev(enterPts.end())->first.impactPos;
			Plane2* plane = prev(enterPts.end())->second;

			RaycastResult2D hitResult;
			hitResult.m_didImpact = true;
			hitResult.m_impactDist = startToEnterDist;
			hitResult.m_impactNormal = plane->m_normal;
			hitResult.m_impactPos = S + I * startToEnterDist;
			hitResult.m_rayFwdNormal = ray.rayFwdNormal;
			hitResult.m_rayDist = ray.rayDist;
			hitResult.m_rayStartPos = ray.rayStartPos;
			hitResult.m_didExit = false;
			hitResult.m_travelDistInShape = d - startToEnterDist;
			hitResult.m_exitPos = E;
			hitResult.m_exitNormal = ray.rayFwdNormal;

			return hitResult;
		}
		else return missResult;
	}

	// the usual case is we both have enter and exit pts
	// let's get last enter pt and first exit first
	auto lastEnterIt = prev(enterPts.end());
	auto firstExitIt = exitPts.begin();
	float lastEnterDist = lastEnterIt->first.dist_rayStart_hit;
	float firstExitDist = firstExitIt->first.dist_rayStart_hit;

	// if the exit dist is short than the enter dist, then it is a miss
	if (firstExitDist <= lastEnterDist) return missResult; // case 4
	else 
	{
		// we need to check the mid point from start to end is inside the convex
		Vec2 lastEnterPos = lastEnterIt->first.impactPos;
		Vec2 firstExitPos = firstExitIt->first.impactPos;
		Vec2 midPos = (lastEnterPos + firstExitPos) * 0.5f;

		if (IsPointBehindAllPlane2(midPos, convex.m_boundingPlanes))	// case 5
		{
			Vec2 lastEnterPt = lastEnterIt->first.impactPos;
			Vec2 firstExitPt = firstExitIt->first.impactPos;
			Plane2* impactPlane = lastEnterIt->second;
			Plane2* exitPlane = firstExitIt->second;

			// this is unfinished
			RaycastResult2D hitResult;
			hitResult.m_didImpact = true;
			hitResult.m_impactDist = lastEnterDist;
			hitResult.m_impactNormal = impactPlane->m_normal;
			hitResult.m_impactPos = S + I * lastEnterDist;
			hitResult.m_rayFwdNormal = I;
			hitResult.m_rayDist = d;
			hitResult.m_rayStartPos = S;
			hitResult.m_didExit = true;
			hitResult.m_travelDistInShape = firstExitDist - lastEnterDist;
			hitResult.m_exitPos = S + I * firstExitDist;
			hitResult.m_exitNormal = exitPlane->m_normal;

			return hitResult;
		}
		else return missResult;
	}
}

RaycastResult2D RaycastVSPlane2(Raycast2D ray, Plane2 plane)
{
	// we are going to figure out the miss cases first
	Vec2& S = ray.rayStartPos;
	Vec2& i = ray.rayFwdNormal;
	float& d = ray.rayDist;
	Vec2 E = S + i * d;

	RaycastResult2D missResult;
	missResult.m_didImpact = false;
	missResult.m_impactDist = 0.f;
	missResult.m_impactNormal = ray.rayFwdNormal;
	missResult.m_rayFwdNormal = ray.rayFwdNormal;
	missResult.m_rayDist = ray.rayDist;
	missResult.m_rayStartPos = ray.rayStartPos;
	missResult.m_didExit = false;
	missResult.m_travelDistInShape = 0.f;
	missResult.m_exitPos = ray.rayStartPos + ray.rayDist * ray.rayFwdNormal;
	missResult.m_exitNormal = ray.rayFwdNormal;

	// the first missing 
	Vec2 nearsestPtOnPlane = plane.GetNearestPointOnPlane(S);

	// if the ray normal is not in the same direction as the direction towards the plane
	if (DotProduct2D(i, (nearsestPtOnPlane - S)) < 0.f)
	{
		return missResult;
	}

	// check if the ray could ever reach the plane
	if (GetDistance2D(S, nearsestPtOnPlane) > d)
	{
		return missResult;
	}

	// check if the ray is straddle on the plane
	bool startIsInfront = plane.IsThePointInFrontOfPlane(S);
	bool endIsInfront = plane.IsThePointInFrontOfPlane(E);
	if (startIsInfront == endIsInfront)
	{
		return missResult;
	}

	// The rest cases are which the raycast hit the plane
	// use similar triangle to calculate the impact pos
	float raycastVerticalLength = DotProduct2D(i * d, plane.m_normal);
	float altitude = plane.GetAltitudeOfPoint(S);
	if (altitude == 0.f) // rayStart is on the plane
	{
		RaycastResult2D result = missResult;
		result.m_didImpact = true;
		result.m_impactNormal = plane.m_normal;
		return missResult;
	}
	else
	{
		float fraction = abs(altitude / raycastVerticalLength);

		RaycastResult2D hitResult;
		hitResult.m_didImpact = true;
		hitResult.m_impactDist = d * fraction;
		hitResult.m_impactPos = S + i * hitResult.m_impactDist;

		// check if the ray normal is align with 
		if (startIsInfront)
		{
			hitResult.m_impactNormal = plane.m_normal;
			hitResult.m_exitNormal = plane.m_normal * (-1.f);
		}
		else
		{
			hitResult.m_impactNormal = plane.m_normal * (-1.f);
			hitResult.m_exitNormal = plane.m_normal;
		}

		hitResult.m_didExit = true;
		hitResult.m_travelDistInShape = 0.f;
		hitResult.m_exitPos = hitResult.m_impactPos;;

		hitResult.m_rayFwdNormal = i;
		hitResult.m_rayStartPos = S;
		hitResult.m_rayDist = d;

		return hitResult;
	}
}

RaycastResult3D RaycastVsAABB3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayDist, AABB3 targetBox)
{
	Vec3 rayEnd = rayStart + rayForwardNormal * rayDist;

	// if it misses, we will use this result
	RaycastResult3D missResult;
	missResult.m_didImpact = false;
	missResult.m_impactDist = rayDist;
	missResult.m_impactPos = rayEnd;
	missResult.m_impactNormal = rayForwardNormal;

	missResult.m_didExit = false;
	missResult.m_travelDistInShape = 0.f;
	missResult.m_exitPos = rayStart;
	missResult.m_exitNormal = rayForwardNormal;

	missResult.m_rayFwdNormal = rayForwardNormal;
	missResult.m_rayStartPos = rayStart;
	missResult.m_rayDist = rayDist;

	// if it hit, we will use this result
	RaycastResult3D hitResult;

	hitResult.m_rayFwdNormal = rayForwardNormal;
	hitResult.m_rayStartPos = rayStart;
	hitResult.m_rayDist = rayDist;

	// firstly try to get rid of general cases where ray and box are too far away to each other
	// try to construct AABB3 of the ray
	FloatRange rayRangeX(rayStart.x, rayEnd.x);
	FloatRange rayRangeY(rayStart.y, rayEnd.y);
	FloatRange rayRangeZ(rayStart.z, rayEnd.z);
	AABB3 rayBox(Vec3(rayRangeX.m_min, rayRangeY.m_min, rayRangeZ.m_min), Vec3(rayRangeX.m_max, rayRangeY.m_max, rayRangeZ.m_max));

	if (!DoAABB3sOverlap(rayBox, targetBox))
	{
		return missResult;
	}
	else
	{
		// we will get the overlap range from x, y, z and therefore get the raycast result
		// first get 2D raycast on XY
		Raycast3D ray3D(rayStart, rayForwardNormal, rayDist);
		Raycast2D ray2D = ConvertRaycast3DToRaycast2DOnXY(ray3D);
		AABB2 box2D(Vec2(targetBox.m_mins.x, targetBox.m_mins.y), Vec2(targetBox.m_maxs.x, targetBox.m_maxs.y));
		RaycastResult2D AABB2Result = RaycastVSAABB2(ray2D, box2D);

		if (!AABB2Result.m_didImpact)
		{
			return missResult;
		}
		else
		{
			// based on the result, get float range on XY
			float ts = GetDistance2D(AABB2Result.m_impactPos, ray2D.rayStartPos) / ray2D.rayDist;
			float te = GetDistance2D(AABB2Result.m_exitPos, ray2D.rayStartPos) / ray2D.rayDist;
			FloatRange range2D(ts, te);

			// get the height float range
			float rayHeight = rayEnd.z - rayStart.z;
			// discuss the case when the ray parallel to the XY
			if (rayHeight == 0.f)
			{
				if (rayStart.z > targetBox.m_mins.z && rayStart.z < targetBox.m_mins.z)
				{
					// hit 
					hitResult.m_didImpact = true;
					hitResult.m_impactDist = AABB2Result.m_impactDist;
					hitResult.m_impactPos = Vec3(AABB2Result.m_impactPos, rayStart.z);
					hitResult.m_impactNormal = Vec3(AABB2Result.m_impactNormal, 0.f);

					hitResult.m_didExit = AABB2Result.m_didExit;
					hitResult.m_travelDistInShape = AABB2Result.m_travelDistInShape;
					hitResult.m_exitPos = Vec3(AABB2Result.m_impactPos, rayStart.z);
					hitResult.m_exitNormal = Vec3(AABB2Result.m_exitNormal, 0.f);
					return hitResult;
				}
				else
				{
					return missResult;
				}
			}
			else
			{
				float oneUnitOverZRange = 1.f / rayHeight;
				float boxMinInZ = (targetBox.m_mins.z - rayStart.z) * oneUnitOverZRange;
				float boxMaxInZ = (targetBox.m_maxs.z - rayStart.z) * oneUnitOverZRange;
				FloatRange ZRange(boxMinInZ, boxMaxInZ);
				float Zmin = ZRange.m_min;
				float Zmax = ZRange.m_max;

				// if the ray is parallel to the axis Z in space
				if (range2D.GetRangeLength() == 0.f)
				{
					if (rayForwardNormal.z > 0.f) // hit on bottom
					{
						// hit
						hitResult.m_didImpact = true;
						hitResult.m_impactDist = targetBox.m_mins.z - rayStart.z;
						hitResult.m_impactPos = Vec3(AABB2Result.m_impactPos, targetBox.m_mins.z);
						hitResult.m_impactNormal = Vec3(0.f, 0.f, -1.f);

						hitResult.m_didExit = AABB2Result.m_didExit;
						if (AABB2Result.m_didExit)
						{
							hitResult.m_travelDistInShape = targetBox.m_maxs.z - targetBox.m_mins.z;
							hitResult.m_exitPos = hitResult.m_impactPos + Vec3(0.f, 0.f, hitResult.m_travelDistInShape);
						}
						else
						{
							hitResult.m_travelDistInShape = rayEnd.z - rayStart.z;
							hitResult.m_exitPos = rayEnd;
						}
						hitResult.m_exitNormal = Vec3(0.f, 0.f, 1.f);
						return hitResult;
					}
					else // hit on top
					{
						// hit
						hitResult.m_didImpact = true;
						hitResult.m_impactDist = targetBox.m_mins.z - rayStart.z;
						hitResult.m_impactPos = Vec3(AABB2Result.m_impactPos, targetBox.m_maxs.z);
						hitResult.m_impactNormal = Vec3(0.f, 0.f, 1.f);

						hitResult.m_didExit = AABB2Result.m_didExit;
						if (AABB2Result.m_didExit)
						{
							hitResult.m_travelDistInShape = targetBox.m_maxs.z - targetBox.m_mins.z;
							hitResult.m_exitPos = hitResult.m_impactPos - Vec3(0.f, 0.f, hitResult.m_travelDistInShape);
						}
						else
						{
							hitResult.m_travelDistInShape = rayStart.z - rayEnd.z;
							hitResult.m_exitPos = rayEnd;
						}
						hitResult.m_exitNormal = Vec3(0.f, 0.f, -1.f);
						return hitResult;
					}
				}

				if (!range2D.IsOverLappingWith(ZRange))
				{
					return missResult;
				}
				else
				{
					// general case
					float oneUnitOverXRange = 1.f / (rayEnd.x - rayStart.x);
					float oneUnitOverYRange = 1.f / (rayEnd.y - rayStart.y);

					// get the range from the ray start to the end
					float boxMinInX = (targetBox.m_mins.x - rayStart.x) * oneUnitOverXRange;
					float boxMaxInX = (targetBox.m_maxs.x - rayStart.x) * oneUnitOverXRange;
					float boxMinInY = (targetBox.m_mins.y - rayStart.y) * oneUnitOverYRange;
					float boxMaxInY = (targetBox.m_maxs.y - rayStart.y) * oneUnitOverYRange;

					FloatRange Xrange(boxMinInX, boxMaxInX);
					FloatRange Yrange(boxMinInY, boxMaxInY);
					float Xmin = Xrange.m_min;
					float Xmax = Xrange.m_max;
					float Ymin = Yrange.m_min;
					float Ymax = Yrange.m_max;

					FloatRange XYOverlapRange = Xrange.GetOverlapRange(Yrange);
					FloatRange overlapRange = XYOverlapRange.GetOverlapRange(ZRange);
					if (overlapRange.GetRangeLength() != 0.f)
					{
						hitResult.m_didImpact = true;

						Vec3 ptA = rayStart + rayForwardNormal * rayDist * overlapRange.m_min;
						Vec3 ptB = rayStart + rayForwardNormal * rayDist * overlapRange.m_max;
						float SA_lengthSqr = (ptA - rayStart).GetLengthSquared();
						float SB_lengthSqr = (ptB - rayStart).GetLengthSquared();

						// the impact could have six situation
						Vec3 Right(1.f, 0.f, 0.f);
						Vec3 Left(-1.f, 0.f, 0.f);
						Vec3 font(0.f, 1.f, 0.f);
						Vec3 Back(0.f, -1.f, 0.f);
						Vec3 Up(0.f, 0.f, 1.f);
						Vec3 Down(0.f, 0.f, -1.f);

						if (SA_lengthSqr < SB_lengthSqr) // impact on ptA, exit on ptB
						{
							// impact info
							hitResult.m_impactPos = ptA;
							hitResult.m_exitNormal = ptB;
							hitResult.m_impactDist = GetDistance3D(ptA, rayStart);
							if (overlapRange.m_min == Xmin) // hit on L or R
							{
								if (rayForwardNormal.x > 0.f)
								{
									hitResult.m_impactNormal = Left;
								}
								else
								{
									hitResult.m_impactNormal = Right;
								}
							}
							else if (overlapRange.m_min == Ymin) // hit on F or B
							{
								if (rayForwardNormal.y > 0.f)
								{
									hitResult.m_impactNormal = Back;
								}
								else
								{
									hitResult.m_impactNormal = font;
								}
							}
							else if (overlapRange.m_min == Zmin) // hit on T or D
							{
								if (rayForwardNormal.z > 0.f)
								{
									hitResult.m_impactNormal = Down;
								}
								else
								{
									hitResult.m_impactNormal = Up;
								}
							}
							return hitResult;
							// todo: discuss exit normal
						}
						else // impact on ptB, exit on ptA
						{
							// impact info
							hitResult.m_impactPos = ptB;
							hitResult.m_exitNormal = ptA;
							hitResult.m_impactDist = GetDistance3D(ptB, rayStart);
							if (overlapRange.m_max == Xmax) // hit on L or R
							{
								if (rayForwardNormal.x > 0.f)
								{
									hitResult.m_impactNormal = Left;
								}
								else
								{
									hitResult.m_impactNormal = Right;
								}
							}
							else if (overlapRange.m_max == Ymax) // hit on F or B
							{
								if (rayForwardNormal.y > 0.f)
								{
									hitResult.m_impactNormal = Back;
								}
								else
								{
									hitResult.m_impactNormal = font;
								}
							}
							else if (overlapRange.m_max == Zmax) // hit on T or D
							{
								if (rayForwardNormal.z > 0.f)
								{
									hitResult.m_impactNormal = Down;
								}
								else
								{
									hitResult.m_impactNormal = Up;
								}
							}
							return hitResult;
							// todo: discuss exit normal					
						}
					}
					else
					{
						return missResult;
					}
				}
			}

		}
	}
}

RaycastResult3D RaycastVsOBB3(Vec3 rayStart, Vec3 rayForwardNormal, float rayDist, OBB3 targetbox)
{
	// transfer all the raycast info into OBB3 local space and test the raycast vs AABB3
	Mat44 worldToLocalMat = targetbox.GetModelMatrix().GetOrthonormalInverse();
	Vec3 localRayStart = worldToLocalMat.TransformPosition3D(rayStart);
	Vec3 localRayNormal = worldToLocalMat.TransformVectorQuantity3D(rayForwardNormal);

	// raycast in local space
	Vec3 mins = targetbox.m_halfDimensions * -1.f;
	Vec3 maxs = targetbox.m_halfDimensions;
	AABB3 OBB3InLocal(mins, maxs);
	RaycastResult3D result = RaycastVsAABB3D(localRayStart, localRayNormal, rayDist, OBB3InLocal);

	// transform the result into world 
	Mat44 localToWorldMat = targetbox.GetModelMatrix();
	result.m_impactPos = localToWorldMat.TransformPosition3D(result.m_impactPos);
	result.m_impactNormal = localToWorldMat.TransformVectorQuantity3D(result.m_impactNormal);
	result.m_exitPos = localToWorldMat.TransformPosition3D(result.m_exitPos);
	result.m_exitNormal = localToWorldMat.TransformVectorQuantity3D(result.m_exitNormal);
	result.m_rayStartPos = rayStart;
	result.m_rayFwdNormal = rayForwardNormal;
	return result;
}

RaycastResult3D RaycastVsPlane3D(Vec3 rayStart, Vec3 rayFwdNormal, float rayDist, Plane3 plane)
{
	RaycastResult3D missResult;
	missResult.m_didImpact = false;
	missResult.m_impactDist = rayDist;
	Vec3 E = rayStart + rayFwdNormal * rayDist;
	missResult.m_impactPos = E;
	missResult.m_impactNormal = rayFwdNormal;

	missResult.m_didExit = false;
	missResult.m_travelDistInShape = 0.f;
	missResult.m_exitPos = rayStart + rayFwdNormal * rayDist;;
	missResult.m_exitNormal = rayFwdNormal;

	missResult.m_rayFwdNormal = rayFwdNormal;
	missResult.m_rayStartPos = rayStart;
	missResult.m_rayDist = rayDist;
	Vec3 nearsestPtOnPlane = GetNearestPointOnPlane(rayStart, plane);

	// check if the ray could ever reach the plane
	if (GetDistance3D(rayStart, nearsestPtOnPlane) >= rayDist)
	{
		return missResult;
	}

	// if the ray normal is not in the same direction as the direction towards the plane
	if (DotProduct3D(rayFwdNormal, (nearsestPtOnPlane - rayStart)) < 0.f)
	{
		return missResult;
	}

	// check if the ray is straddle on the plane
	bool startIsInfront = plane.IfThePointIsInFrontOfPlane(rayStart);
	bool endIsInfront = plane.IfThePointIsInFrontOfPlane(E);
	if (startIsInfront == endIsInfront)
	{
		return missResult;
	}

	// The rest cases are which the raycast hit the plane
	// use similar triangle to calculate the impact pos
	float raycastVerticalLength = DotProduct3D(rayFwdNormal * rayDist, plane.m_normal);
	float altitude = plane.GetAltitudeOfPoint(rayStart);
	if (altitude == 0.f) // raystart is on the plane
	{
		return missResult;
	}
	else
	{
		float fraction = abs(altitude / raycastVerticalLength);
		RaycastResult3D hitResult;

		hitResult.m_didImpact = true;
		hitResult.m_impactDist = rayDist * fraction;
		hitResult.m_impactPos = rayStart + rayFwdNormal * hitResult.m_impactDist;

		// check if the ray normal is align with 
		if (plane.IfThePointIsInFrontOfPlane(rayStart))
		{
			hitResult.m_impactNormal = plane.m_normal;
			hitResult.m_exitNormal = plane.m_normal * (-1.f);
		}
		else
		{
			hitResult.m_impactNormal = plane.m_normal * (-1.f);
			hitResult.m_exitNormal = plane.m_normal;
		}

		hitResult.m_didExit = true;
		hitResult.m_travelDistInShape = 0.f;
		hitResult.m_exitPos = hitResult.m_impactPos;;

		hitResult.m_rayFwdNormal = rayFwdNormal;
		hitResult.m_rayStartPos = rayStart;
		hitResult.m_rayDist = rayDist;

		return hitResult;
	}
}

RaycastResult3D RaycastVsSphere3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayDist, Sphere sphere)
{
	// we will analysis the situation based on the 2D surface of S, C and raycast end point
	Vec3 S = rayStart;
	Vec3 C = sphere.Center;
	Vec3 rayEnd = rayStart + rayForwardNormal * rayDist;

	// if it misses, we will use this result
	RaycastResult3D missResult;
	missResult.m_didImpact = false;
	missResult.m_impactDist = rayDist;
	missResult.m_impactPos = rayEnd;
	missResult.m_impactNormal = rayForwardNormal;
	missResult.m_rayFwdNormal = rayForwardNormal;
	missResult.m_rayStartPos = rayStart;
	missResult.m_rayDist = rayDist;

	// if it hit, we will use this result
	RaycastResult3D hitResult;

	float R = sphere.Radius;
	Vec3 SC = C - S;
	float SCLengthOnRay = DotProduct3D(SC, rayForwardNormal); // the projection length of SC on raycast

	// Case A: disc is in the back of the rayStart and is the opposite of the rayForwardNormal: miss!
	if (SCLengthOnRay <= 0.f)
	{
		return missResult;
	}

	// Case B: the ray could not reach to the disc: miss!
	if (SCLengthOnRay >= (rayDist + R))
	{
		return missResult;
	}

	Vec3 SCi = SCLengthOnRay * rayForwardNormal;
	Vec3 SCjk = SC - SCi;
	float SCjkSq = SCjk.GetLengthSquared();

	// Case C: the ray go too left or right: miss!
	if (SCjkSq >= (R * R))
	{
		return missResult;
	}

	// Case D: the ray starts inside the sphere
	if (IsPointInsideSphere(rayStart, sphere))
	{
		return missResult;
	}

	// construct a right triangle with the center of disc, impact pos and the ray
	float a = sqrtf((R * R) - SCjkSq);
	float impactDist = SCLengthOnRay - a;

	// Case D: impact dist is faraway than the ray end
	if (impactDist >= rayDist)
	{
		return missResult;
	}

	// for the rest cases, we got a hit
	Vec3 impactPos = rayStart + (rayForwardNormal * impactDist);
	Vec3 impactNormal = (impactPos - sphere.Center).GetNormalized();
	hitResult.m_didImpact = true;
	hitResult.m_impactDist = impactDist;
	hitResult.m_impactPos = impactPos;
	hitResult.m_impactNormal = impactNormal;

	// calculate the exit point
	float travelDist = rayDist - (impactPos - rayStart).GetLength();
	Vec3 E = impactPos + travelDist * rayForwardNormal;
	hitResult.m_exitPos = E;
	hitResult.m_travelDistInShape = travelDist;

	hitResult.m_rayFwdNormal = rayForwardNormal;
	hitResult.m_rayStartPos = rayStart;
	hitResult.m_rayDist = rayDist;
	return hitResult;
}

RaycastResult3D RaycastVsCylinderZ3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayDist, ZCylinder cylinder)
{
	Vec3 rayEnd = rayStart + rayForwardNormal * rayDist;

	// if it misses, we will use this result
	RaycastResult3D missResult;
	missResult.m_didImpact = false;
	missResult.m_impactDist = rayDist;
	missResult.m_impactPos = rayEnd;
	missResult.m_impactNormal = rayForwardNormal;

	missResult.m_didExit = false;
	missResult.m_travelDistInShape = 0.f;
	missResult.m_exitPos = rayStart;
	missResult.m_exitNormal = rayForwardNormal;

	missResult.m_rayFwdNormal = rayForwardNormal;
	missResult.m_rayStartPos = rayStart;
	missResult.m_rayDist = rayDist;

	// if it hit, we will use this result
	RaycastResult3D hitResult;

	hitResult.m_rayFwdNormal = rayForwardNormal;
	hitResult.m_rayStartPos = rayStart;
	hitResult.m_rayDist = rayDist;

	// we will get rid of the general miss case at first
	// construct a sphere based on the cylinder
	float cylinderHeight = (cylinder.MinMaxZ.m_max - cylinder.MinMaxZ.m_min);
	Vec3 cylinderCenter(cylinder.CenterXY.x, cylinder.CenterXY.y, (cylinder.MinMaxZ.m_min + (cylinderHeight * 0.5f)));
	float cylinderRadius = sqrtf((cylinderHeight * 0.5f) * (cylinderHeight * 0.5f) + cylinder.Radius * cylinder.Radius);
	Sphere cylinderSphere(cylinderCenter, cylinderRadius);
	
	// construct a sphere based on the raycast
	Vec3 rayCenter = (rayStart + rayEnd) * 0.5f;
	float rayRadius = rayDist * 0.5f;
	Sphere raySphere(rayCenter, rayRadius);
	// if the raycast sphere is not going to overlap with the target cylinder, it is definitely not going to impact
	// this could get rid of most general cases, could also use AABB3()
	if (!DoSpheresOverlap3D(cylinderSphere, raySphere)) // miss
	{
		return missResult;
	}

	// get 2D raycast result on the top view of the cylinder
	Raycast3D ray3D(rayStart, rayForwardNormal, rayDist);
	Raycast2D ray2D = ConvertRaycast3DToRaycast2DOnXY(ray3D);
	RaycastResult2D result2D = RaycastVsDisc2D(ray2D, cylinder.CenterXY, cylinder.Radius);
	if (!result2D.m_didImpact)
	{
		return missResult;
	}
	else
	{
		// we first going to get the float range of 2D ray with the disc
		Vec2& discImpactPt = result2D.m_impactPos;
		Vec2& discExitPt = result2D.m_exitPos;

		if (result2D.m_rayDist == 0.f)
		{
			return missResult;
		}
		float ts = (discImpactPt - result2D.m_rayStartPos).GetLength() / result2D.m_rayDist;
		float te = (discExitPt - result2D.m_rayStartPos).GetLength() / result2D.m_rayDist;
		FloatRange discRange(ts, te);

		// then we are going to consider the side view
		// secondly we are going to get the float range of the cylinder with ray in 2D
		FloatRange heightRange;
		// first avoid the case which the raycast parallel to the XY
		if (rayEnd.z - rayStart.z == 0.f)
		{
			if (rayStart.z > cylinder.MinMaxZ.m_min && rayStart.z < cylinder.MinMaxZ.m_max && (discRange.GetRangeLength() != 0.f)) // hit the side
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = result2D.m_impactDist;
				hitResult.m_impactPos = Vec3(discImpactPt, rayStart.z);
				hitResult.m_impactNormal = Vec3(result2D.m_impactNormal, 0.f);

				hitResult.m_didExit = result2D.m_didExit;
				hitResult.m_travelDistInShape = result2D.m_travelDistInShape;
				hitResult.m_exitPos = Vec3(discExitPt, rayStart.z);
				hitResult.m_exitNormal = Vec3(result2D.m_exitNormal, 0.f);
				return hitResult;
			}
			else
			{
				return missResult;
			}
		}
		// after avoid divide 0 we could then calculate cylinder height range on raycast in side view
		float onePerRayHeight = 1.f / (rayEnd.z - rayStart.z);
		heightRange.m_min = (cylinder.MinMaxZ.m_min - rayStart.z) * onePerRayHeight;
		heightRange.m_max = (cylinder.MinMaxZ.m_max - rayStart.z) * onePerRayHeight;
		heightRange = FloatRange(heightRange.m_min, heightRange.m_max); // notice that onPerRayHeight might be negative

		// based on the disc float range and height float range, we could calculate all the result
		if (!heightRange.IsOverLappingWith(discRange)) // kissing cases will be counted as false
		{
			return missResult;
		}
		else
		{
			FloatRange overlapRange = heightRange.GetOverlapRange(discRange);
			hitResult.m_didImpact = true;

			// think about flip the raycast direction, the impact and exit point switch as well
			// so we are going to calculate this two point and see which is closet to the ray start point
			Vec3 ptA = rayStart + rayForwardNormal * rayDist * overlapRange.m_min;
			Vec3 ptB = rayStart + rayForwardNormal * rayDist * overlapRange.m_max;
			float SA_lengthSqr = (ptA - rayStart).GetLengthSquared();
			float SB_lengthSqr = (ptB - rayStart).GetLengthSquared();

			if (SA_lengthSqr < SB_lengthSqr) // impact on ptA, exit on ptB
			{
				// impact info
				hitResult.m_impactPos = ptA;
				hitResult.m_impactDist = rayDist * abs(overlapRange.m_min);
				if (overlapRange.m_min == discRange.m_min) // hit on side
				{
					hitResult.m_impactNormal = (ptA - Vec3(cylinder.CenterXY, ptA.z)).GetNormalized();
				}
				else // hit on the top or bottom
				{
					if (rayForwardNormal.z < 0.f) // impact on top
					{
						hitResult.m_impactNormal = Vec3(0.f, 0.f, 1.f);
					}
					else // impact on bottom
					{
						hitResult.m_impactNormal = Vec3(0.f, 0.f, -1.f);
					}
				}

				// exit info: todo
				// if (result2D.m_didExit)
				// {
				// }
				hitResult.m_didExit = true;
				hitResult.m_travelDistInShape = rayDist * (overlapRange.m_max - overlapRange.m_min);
				hitResult.m_exitPos = ptB;
				if (overlapRange.m_max == discRange.m_min || overlapRange.m_max == discRange.m_max) // exit on top or bottom
				{
					if (rayForwardNormal.z < 0.f) // exit on bottom
					{
						hitResult.m_exitNormal = Vec3(0.f, 0.f, -1.f);
					}
					else // exit on top
					{
						hitResult.m_exitNormal = Vec3(0.f, 0.f, 1.f);
					}
				}
				else // exit on the side
				{
					hitResult.m_exitNormal = (ptB - Vec3(cylinder.CenterXY, ptB.z)).GetNormalized();
				}
			}
			else // impact on ptB, exit on ptA
			{
				hitResult.m_impactPos = ptB;
				hitResult.m_impactDist = rayDist * abs(overlapRange.m_max);

				// impact info
				if (overlapRange.m_max == discRange.m_min) // hit on side
				{
					hitResult.m_impactNormal = (ptB - Vec3(cylinder.CenterXY, ptA.z)).GetNormalized();
				}
				else // hit on the top or bottom
				{
					if (rayForwardNormal.z < 0.f) // impact on top
					{
						hitResult.m_impactNormal = Vec3(0.f, 0.f, 1.f);
					}
					else // impact on bottom
					{
						hitResult.m_impactNormal = Vec3(0.f, 0.f, -1.f);
					}
				}

				// exit info
				hitResult.m_didExit = true;
				hitResult.m_travelDistInShape = rayDist * (overlapRange.m_max - overlapRange.m_min);
				hitResult.m_exitPos = ptB;
				if (overlapRange.m_max == discRange.m_min || overlapRange.m_max == discRange.m_max) // exit on top or bottom
				{
					if (rayForwardNormal.z < 0.f) // exit on bottom
					{
						hitResult.m_exitNormal = Vec3(0.f, 0.f, -1.f);
					}
					else // exit on top
					{
						hitResult.m_exitNormal = Vec3(0.f, 0.f, 1.f);
					}
				}
				else // exit on the side
				{
					hitResult.m_exitNormal = (ptB - Vec3(cylinder.CenterXY, ptB.z)).GetNormalized();
				}
			}
			return hitResult;
		}
	}
}
