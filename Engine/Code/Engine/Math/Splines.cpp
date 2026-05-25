#include "Engine/Math/Splines.hpp"
#include "Engine/Math/MathUtils.hpp"

float ComputeCubicBezier1D(float A, float B, float C, float D, float t)
{
	float AB = Interpolate(A, B, t);
	float BC = Interpolate(B, C, t);
	float CD = Interpolate(C, D, t);

	float ABC = Interpolate(AB, BC, t);
	float BCD = Interpolate(BC, CD, t);

	float ABCD = Interpolate(ABC, BCD, t);
	return ABCD;
}

Vec2 ComputeCubicBezier2D(Vec2 A, Vec2 B, Vec2 C, Vec2 D, float t)
{
	// Vec2 result;
	// result.x = ComputeCubicBezier1D(A.x, B.x, C.x, D.x, t);
	// result.y = ComputeCubicBezier1D(A.y, B.y, C.y, D.y, t);
	// return result;

	Vec2 AB = Interpolate(A, B, t);
	Vec2 BC = Interpolate(B, C, t);
	Vec2 CD = Interpolate(C, D, t);

	Vec2 ABC = Interpolate(AB, BC, t);
	Vec2 BCD = Interpolate(BC, CD, t);

	Vec2 result = Interpolate(ABC, BCD, t);
	return result;
}

Vec3 ComputeCubicBezier3D(Vec3 A, Vec3 B, Vec3 C, Vec3 D, float t)
{
	Vec3 AB = Interpolate(A, B, t);
	Vec3 BC = Interpolate(B, C, t);
	Vec3 CD = Interpolate(C, D, t);

	Vec3 ABC = Interpolate(AB, BC, t);
	Vec3 BCD = Interpolate(BC, CD, t);

	Vec3 result = Interpolate(ABC, BCD, t);
	return result;
}

float ComputeQuinticBezier1D(float A, float B, float C, float D, float E, float F, float t)
{
	float AB = Interpolate(A, B, t);
	float BC = Interpolate(B, C, t);
	float CD = Interpolate(C, D, t);
	float DE = Interpolate(D, E, t);
	float EF = Interpolate(E, F, t);

	float ABC = Interpolate(AB, BC, t);
	float BCD = Interpolate(BC, CD, t);
	float CDE = Interpolate(CD, DE, t);
	float DEF = Interpolate(DE, EF, t);

	float ABCD = Interpolate(ABC, BCD, t);
	float BCDE = Interpolate(BCD, CDE, t);
	float CDEF = Interpolate(CDE, DEF, t);

	float ABCDE = Interpolate(ABCD, BCDE, t);
	float BCDEF = Interpolate(BCDE, CDEF, t);

	float ABCDEF = Interpolate(ABCDE, BCDEF, t);
	return ABCDEF;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
CubicBezierCurve2D::CubicBezierCurve2D(CubicHermiteCurve2D const& fromHermite)
	: m_a(fromHermite.m_startPos)
	, m_d(fromHermite.m_endPos)
{
	m_b = m_a + fromHermite.m_startVel * (1.f / 3.f);
	m_c = m_d - fromHermite.m_endVel * (1.f / 3.f);
}

CubicBezierCurve2D::CubicBezierCurve2D(Vec2 startPos, Vec2 guidePos1, Vec2 guidePos2, Vec2 endPos)
	: m_a(startPos)
	, m_b(guidePos1)
	, m_c(guidePos2)
	, m_d(endPos)
{

}

Vec2 CubicBezierCurve2D::EvaluateAtParametric(float parametricZeroToOne) const
{
	Vec2 ab = Interpolate(m_a, m_b, parametricZeroToOne);
	Vec2 bc = Interpolate(m_b, m_c, parametricZeroToOne);
	Vec2 cd = Interpolate(m_c, m_d, parametricZeroToOne);

	Vec2 abc = Interpolate(ab, bc, parametricZeroToOne);
	Vec2 bcd = Interpolate(bc, cd, parametricZeroToOne);

	Vec2 result = Interpolate(abc, bcd, parametricZeroToOne);
	return result;
}

float CubicBezierCurve2D::GetApproximateLength(int numSubdivisions /*= 64*/) const
{
	float totalLength = 0.f;
	float step = 1.f / numSubdivisions;
	for (int i = 0; i < numSubdivisions; ++i)
	{
		Vec2 disp = EvaluateAtParametric((i + 1) * step)- EvaluateAtParametric(i * step);
		float dist = disp.GetLength();
		totalLength += dist;
	}

	return totalLength;
}

Vec2 CubicBezierCurve2D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions /*= 64*/) const
{
	float step = 1.f / numSubdivisions;
	std::vector<Vec2> samplePts;
	
	for (int i = 0; i < numSubdivisions; ++i)
	{
		float currentFraction = step * i;
		float nextFraction = step * (i + 1);
		Vec2 thisPos = EvaluateAtParametric(currentFraction);
		Vec2 nextPos = EvaluateAtParametric(nextFraction);
		samplePts.push_back(thisPos);
		// get the last point
		if (i == (numSubdivisions - 1))
		{
			samplePts.push_back(nextPos);
		}
	}
	
	int sectionIndex = 0;
	float sectionLength = 0.f;
	float distFromSectionStart = 0.f;
	for (int i = 0; i < numSubdivisions; ++i)
	{
		Vec2 disp = samplePts[i + 1] - samplePts[i];
		float dist = disp.GetLength();
		distanceAlongCurve -= dist;
		if (distanceAlongCurve <= 0.f)
		{
			sectionIndex = i;
			sectionLength = dist;
			distFromSectionStart = sectionLength + distanceAlongCurve;
			break;
		}
	}
	float fraction = distFromSectionStart / sectionLength;
	Vec2 ptPos = Interpolate(samplePts[sectionIndex], samplePts[sectionIndex + 1], fraction);
	return ptPos;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
CubicBezierCurve3D::CubicBezierCurve3D(CubicHermiteCurve3D const& fromHermite)
	: m_a(fromHermite.m_startPos)
	, m_d(fromHermite.m_endPos)
{
	m_b = m_a + fromHermite.m_startVel * (1.f / 3.f);
	m_c = m_d - fromHermite.m_endVel * (1.f / 3.f);
}

CubicBezierCurve3D::CubicBezierCurve3D(Vec3 startPos, Vec3 guidePos1, Vec3 guidePos2, Vec3 endPos)
	: m_a(startPos)
	, m_b(guidePos1)
	, m_c(guidePos2)
	, m_d(endPos)
{

}

Vec3 CubicBezierCurve3D::EvaluateAtParametric(float parametricZeroToOne) const
{
	Vec3 ab = Interpolate(m_a, m_b, parametricZeroToOne);
	Vec3 bc = Interpolate(m_b, m_c, parametricZeroToOne);
	Vec3 cd = Interpolate(m_c, m_d, parametricZeroToOne);

	Vec3 abc = Interpolate(ab, bc, parametricZeroToOne);
	Vec3 bcd = Interpolate(bc, cd, parametricZeroToOne);

	Vec3 result = Interpolate(abc, bcd, parametricZeroToOne);
	return result;
}

float CubicBezierCurve3D::GetApproximateLength(int numSubdivisions /*= 64*/) const
{
	float totalLength = 0.f;
	float step = 1.f / numSubdivisions;
	for (int i = 0; i < numSubdivisions; ++i)
	{
		Vec3 disp = EvaluateAtParametric((i + 1) * step) - EvaluateAtParametric(i * step);
		float dist = disp.GetLength();
		totalLength += dist;
	}

	return totalLength;
}

Vec3 CubicBezierCurve3D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions /*= 64*/) const
{
	float step = 1.f / numSubdivisions;
	std::vector<Vec3> samplePts;

	for (int i = 0; i < numSubdivisions; ++i)
	{
		float currentFraction = step * i;
		float nextFraction = step * (i + 1);
		Vec3 thisPos = EvaluateAtParametric(currentFraction);
		Vec3 nextPos = EvaluateAtParametric(nextFraction);
		samplePts.push_back(thisPos);
		// get the last point
		if (i == (numSubdivisions - 1))
		{
			samplePts.push_back(nextPos);
		}
	}

	int sectionIndex = 0;
	float sectionLength = 0.f;
	float distFromSectionStart = 0.f;
	for (int i = 0; i < numSubdivisions; ++i)
	{
		Vec3 disp = samplePts[i + 1] - samplePts[i];
		float dist = disp.GetLength();
		distanceAlongCurve -= dist;
		if (distanceAlongCurve <= 0.f)
		{
			sectionIndex = i;
			sectionLength = dist;
			distFromSectionStart = sectionLength + distanceAlongCurve;
			break;
		}
	}
	float fraction = distFromSectionStart / sectionLength;
	Vec3 ptPos = Interpolate(samplePts[sectionIndex], samplePts[sectionIndex + 1], fraction);
	return ptPos;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
CubicHermiteCurve2D::CubicHermiteCurve2D(CubicBezierCurve2D const& fromBezier)
	: m_startPos(fromBezier.m_a)
	, m_endPos(fromBezier.m_d)
{
	m_startVel = 3.f * (fromBezier.m_b - fromBezier.m_a);
	m_endVel = 3.f * (fromBezier.m_d - fromBezier.m_c);
}

CubicHermiteCurve2D::CubicHermiteCurve2D(Vec2 startPos, Vec2 startVel, Vec2 endPos, Vec2 endVel)
	: m_startPos(startPos)
	, m_startVel(startVel)
	, m_endPos(endPos)
	, m_endVel(endVel)
{

}

Vec2 CubicHermiteCurve2D::EvaluateAtParametric(float parametricZeroToOne) const
{
	// todo: optimize this by working it out on paper
	Vec2 bezierGuidePos1 = m_startPos + m_startVel * (1.f / 3.f);
	Vec2 bezierGuidePos2 = m_endPos - m_endVel * (1.f / 3.f);
	return ComputeCubicBezier2D(m_startPos, bezierGuidePos1, bezierGuidePos2, m_endPos, parametricZeroToOne);
}

float CubicHermiteCurve2D::GetApproximateLength(int numSubdivisions /*= 64*/) const
{
	CubicBezierCurve2D asBezier(*this);
	return asBezier.GetApproximateLength(numSubdivisions);
}

Vec2 CubicHermiteCurve2D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdividions /*= 64*/)
{
	CubicBezierCurve2D asBezier(*this);
	return asBezier.EvaluateAtApproximateDistance(distanceAlongCurve, numSubdividions);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
CubicHermiteCurve3D::CubicHermiteCurve3D(CubicBezierCurve3D const& fromBezier)
	: m_startPos(fromBezier.m_a)
	, m_endPos(fromBezier.m_d)
{
	m_startVel = 3.f * (fromBezier.m_b - fromBezier.m_a);
	m_endVel = 3.f * (fromBezier.m_d - fromBezier.m_c);
}

CubicHermiteCurve3D::CubicHermiteCurve3D(Vec3 startPos, Vec3 startVel, Vec3 endPos, Vec3 endVel)
	: m_startPos(startPos)
	, m_startVel(startVel)
	, m_endPos(endPos)
	, m_endVel(endVel)
{

}

Vec3 CubicHermiteCurve3D::EvaluateAtParametric(float parametricZeroToOne) const
{
	// todo: optimize this by working it out on paper
	Vec3 bezierGuidePos1 = m_startPos + m_startVel * (1.f / 3.f);
	Vec3 bezierGuidePos2 = m_endPos - m_endVel * (1.f / 3.f);
	return ComputeCubicBezier3D(m_startPos, bezierGuidePos1, bezierGuidePos2, m_endPos, parametricZeroToOne);
}

float CubicHermiteCurve3D::GetApproximateLength(int numSubdivisions /*= 64*/) const
{
	CubicBezierCurve3D asBezier(*this);
	return asBezier.GetApproximateLength(numSubdivisions);
}

Vec3 CubicHermiteCurve3D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdividions /*= 64*/)
{
	CubicBezierCurve3D asBezier(*this);
	return asBezier.EvaluateAtApproximateDistance(distanceAlongCurve, numSubdividions);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
CatmullRomSpline2D::CatmullRomSpline2D(std::vector<Vec2> const& positions, Vec2 startVel /*= Vec2::ZERO*/, Vec2 endVel /*= Vec2::ZERO*/)
{
	SetPositions(positions, startVel, endVel);
}

void CatmullRomSpline2D::SetPositions(std::vector<Vec2> const& positions, Vec2 startVel /*= Vec2::ZERO*/, Vec2 endVel /*= Vec2::ZERO*/)
{
	// copy positions and initialize velocities to match size; start & end velocity are zero
	m_positions = positions;
	m_velocities.resize(m_positions.size());
	if (positions.empty())
	{
		return;
	}
	m_velocities[0] = startVel;
	m_velocities.back() = endVel;

	// Catmull-Rom algorithm: set velocity at each interior point to the average of the displacement
	for (int i = 1; i < ((int)m_positions.size() - 1); ++i)
	{
		Vec2 prevPos = m_positions[i - 1];
		Vec2 thisPos = m_positions[i];
		Vec2 nextPos = m_positions[i + 1];
		Vec2 previousDisp = thisPos - prevPos;
		Vec2 nextDisp = nextPos - thisPos;
		m_velocities[i] = (previousDisp + nextDisp) * 0.5f;
	}
}

// todo: given a start velocity, this algorithm still exist?
//void CatmullRomSpline2D::SetPositions(std::vector<Vec2> const& positions, )
//{
//	// copy positions and initialize velocities to match size; start & end velocity are zero
//	m_positions = positions;
//	m_velocities.resize(m_positions.size());
//	if (positions.empty())
//	{
//		return;
//	}
//	m_velocities[0] = Vec2::ZERO;
//	m_velocities.back() = Vec2::ZERO;
//
//	// Catmull-Rom algorithm: set velocity at each interior point to the average of the displacement
//	for (int i = 1; i < ((int)m_positions.size() - 1);  ++i)
//	{
//		Vec2 prevPos = m_positions[i - 1];
//		Vec2 thisPos = m_positions[i];
//		Vec2 nextPos = m_positions[i + 1];
//		Vec2 previousDisp = thisPos - prevPos;
//		Vec2 nextDisp = nextPos - thisPos;
//		m_velocities[i] = (previousDisp + nextDisp) * 0.5f;
//	}
//}

Vec2 CatmullRomSpline2D::EvaluateAtParametric(float parametricZeroToNumCurveSections) const
{
	// calculate which curve the input is on
	int sectionStartIndex = (int)floor(parametricZeroToNumCurveSections);
	if (sectionStartIndex >= ((int)m_positions.size() - 1)) // if this is the last point, we are going to use the last section to calculate
	{
		sectionStartIndex = (int)m_positions.size() - 2;
	}
	int SectionEndIndex = sectionStartIndex + 1;
	float fraction = parametricZeroToNumCurveSections - (float)sectionStartIndex;

	CubicHermiteCurve2D sectionHermiteCurve(m_positions[sectionStartIndex], m_velocities[sectionStartIndex],
		m_positions[SectionEndIndex], m_velocities[SectionEndIndex]);
	CubicBezierCurve2D sectionBezierCurve(sectionHermiteCurve);

	return sectionBezierCurve.EvaluateAtParametric(fraction);
}

// very curve has numSubdivisions
float CatmullRomSpline2D::GetApproximateLength(int numSubdivisions /*= 64*/) const
{
	float totalLength = 0.f;
	float step = 1.f / numSubdivisions;
	for (int i = 0; i < (numSubdivisions * (m_positions.size() - 1)); ++i)
	{
		Vec2 disp = EvaluateAtParametric((i + 1) * step) - EvaluateAtParametric(i * step);
		float dist = disp.GetLength();
		totalLength += dist;
	}

	return totalLength;
}

Vec2 CatmullRomSpline2D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions /*= 64*/) const
{
	// base on the subdivision and this Catmull-Rom spline generate a spline
	float step = 1.f / numSubdivisions;
	std::vector<Vec2> samplePts;

	for (int i = 0; i < ((numSubdivisions * (m_positions.size() - 1))); ++i)
	{
		float currentFraction = step * i;
		float nextFraction = step * (i + 1);
		Vec2 thisPos = EvaluateAtParametric(currentFraction);
		Vec2 nextPos = EvaluateAtParametric(nextFraction);

		samplePts.push_back(thisPos);
		// get the last point
		if (i == ((numSubdivisions * (m_positions.size() - 1))) - 1 )
		{
			samplePts.push_back(nextPos);
		}
	}

	// get the wanted point
	int sectionIndex = 0;
	float sectionLength = 0.f;
	float distFromSectionStart = 0.f;
	for (int i = 0; i < ((numSubdivisions * (m_positions.size() - 1))); ++i)
	{
		Vec2 disp = samplePts[i + 1] - samplePts[i];
		float dist = disp.GetLength();
		distanceAlongCurve -= dist;
		if (distanceAlongCurve <= 0.f)
		{
			sectionIndex = i;
			sectionLength = dist;
			distFromSectionStart = sectionLength + distanceAlongCurve;
			break;
		}
	}
	float fraction = distFromSectionStart / sectionLength;
	Vec2 ptPos = Interpolate(samplePts[sectionIndex], samplePts[sectionIndex + 1], fraction);
	return ptPos;
}

CatmullRomSpline3D::CatmullRomSpline3D(std::vector<Vec3> const& positions, Vec3 startVel /*= Vec3::ZERO*/, Vec3 endVel /*= Vec3::ZERO*/)
{
	SetPositions(positions, startVel, endVel);
}

void CatmullRomSpline3D::SetPositions(std::vector<Vec3> const& positions, Vec3 startVel /*= Vec3::ZERO*/, Vec3 endVel /*= Vec3::ZERO*/)
{
	// copy positions and initialize velocities to match size; start & end velocity are zero
	m_positions = positions;
	m_velocities.resize(m_positions.size());
	if (positions.empty())
	{
		return;
	}
	m_velocities[0] = startVel;
	m_velocities.back() = endVel;

	// Catmull-Rom algorithm: set velocity at each interior point to the average of the displacement
	for (int i = 1; i < ((int)m_positions.size() - 1); ++i)
	{
		Vec3 prevPos = m_positions[i - 1];
		// Vec3 thisPos = m_positions[i];
		Vec3 nextPos = m_positions[i + 1];
		m_velocities[i] = (nextPos - prevPos) * 0.5f;
	}
}

Vec3 CatmullRomSpline3D::EvaluateAtParametric(float parametricZeroToNumCurveSections) const
{
	// calculate which curve the input is on
	int sectionStartIndex = (int)floor(parametricZeroToNumCurveSections);
	if (sectionStartIndex >= ((int)m_positions.size() - 1)) // if this is the last point, we are going to use the last section to calculate
	{
		sectionStartIndex = (int)m_positions.size() - 2;
	}
	int SectionEndIndex = sectionStartIndex + 1;
	float fraction = parametricZeroToNumCurveSections - (float)sectionStartIndex;

	CubicHermiteCurve3D sectionHermiteCurve(m_positions[sectionStartIndex], m_velocities[sectionStartIndex],
		m_positions[SectionEndIndex], m_velocities[SectionEndIndex]);
	CubicBezierCurve3D sectionBezierCurve(sectionHermiteCurve);

	return sectionBezierCurve.EvaluateAtParametric(fraction);
}

float CatmullRomSpline3D::GetApproximateLength(int numSubdivisions /*= 64*/) const
{
	float totalLength = 0.f;
	float step = 1.f / numSubdivisions;
	for (int i = 0; i < (numSubdivisions * (m_positions.size() - 1)); ++i)
	{
		Vec3 disp = EvaluateAtParametric((i + 1) * step) - EvaluateAtParametric(i * step);
		float dist = disp.GetLength();
		totalLength += dist;
	}

	return totalLength;
}

Vec3 CatmullRomSpline3D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions /*= 64*/) const
{
	// base on the subdivision and this Catmull-Rom spline generate a spline
	float step = 1.f / numSubdivisions;
	std::vector<Vec3> samplePts;

	for (int i = 0; i < ((numSubdivisions * (m_positions.size() - 1))); ++i)
	{
		float currentFraction = step * i;
		float nextFraction = step * (i + 1);
		Vec3 thisPos = EvaluateAtParametric(currentFraction);
		Vec3 nextPos = EvaluateAtParametric(nextFraction);

		samplePts.push_back(thisPos);
		// get the last point
		if (i == ((numSubdivisions * (m_positions.size() - 1))) - 1)
		{
			samplePts.push_back(nextPos);
		}
	}

	// get the wanted point
	int sectionIndex = 0;
	float sectionLength = 0.f;
	float distFromSectionStart = 0.f;
	for (int i = 0; i < ((numSubdivisions * (m_positions.size() - 1))); ++i)
	{
		Vec3 disp = samplePts[i + 1] - samplePts[i];
		float dist = disp.GetLength();
		distanceAlongCurve -= dist;
		if (distanceAlongCurve <= 0.f)
		{
			sectionIndex = i;
			sectionLength = dist;
			distFromSectionStart = sectionLength + distanceAlongCurve;
			break;
		}
	}
	float fraction = distFromSectionStart / sectionLength;
	Vec3 ptPos = Interpolate(samplePts[sectionIndex], samplePts[sectionIndex + 1], fraction);
	return ptPos;
}

Vec3 CatmullRomSpline3D::GetTangentDirectionAtApproximateDistance(float distanceAlongCurve, float step /*= 0.001f*/, int numSubdivisions /*= 64*/) const
{
	if (m_positions.empty() || m_velocities.empty())
	{
		ERROR_AND_DIE("The spline is invalid");
	}

	// Ensure t + delta is within valid range
	Vec3 point = EvaluateAtApproximateDistance(distanceAlongCurve);
	Vec3 nextPoint = Vec3();
	Vec3 tangent = Vec3();
	if (distanceAlongCurve + step <= GetApproximateLength(numSubdivisions))
	{
		nextPoint = EvaluateAtApproximateDistance(distanceAlongCurve + step);
		tangent = nextPoint - point;
	}
	else
	{
		return m_velocities.back().GetNormalized();
	}

	tangent = tangent.GetNormalized();
	return tangent;
}

SamplePointsCurve3D::SamplePointsCurve3D(std::vector<Vec3>& samplePoints)
				: m_samplePoints(samplePoints)
{
	
}
