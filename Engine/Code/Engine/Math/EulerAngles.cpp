#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

enum
{
	Ix, Iy, Iz, Iw,
	Jx, Jy, Jz, Jw,
	Kx, Ky, Kz, Kw,
	Tx, Ty, Tz, Tw
};

EulerAngles::EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees)
	:m_yawDegrees(yawDegrees), m_pitchDegrees(pitchDegrees), m_rollDegrees(rollDegrees)
{

}

EulerAngles::EulerAngles()
{
	m_yawDegrees = 0.f;
	m_pitchDegrees = 0.f;
	m_rollDegrees = 0.f;
}

void EulerAngles::GetAsVectors_IFwd_JLeft_KUp(Vec3& out_forwardIBasis, Vec3& out_forwardJBasis, Vec3& out_forwardKBasis)
{
	Mat44 eulerAnglesMat = GetAsMatrix_XFwd_YLeft_ZUp();

	out_forwardIBasis = eulerAnglesMat.GetIBasis3D();
	out_forwardJBasis = eulerAnglesMat.GetJBasis3D();
	out_forwardKBasis = eulerAnglesMat.GetKBasis3D();
}

// in the world which _XFwd_YLeft_ZUp
Vec3 EulerAngles::GetForwardIBasis()
{
	Mat44 eulerAnglesMat = GetAsMatrix_XFwd_YLeft_ZUp();
	return eulerAnglesMat.GetIBasis3D();
}

Mat44 EulerAngles::GetAsMatrix_XFwd_YLeft_ZUp() const
{
	// // what we record on paper is [yaw][pitch][roll][localPosition]
	// Mat44 eulerAnglesMat = Mat44();
	// eulerAnglesMat.AppendZRotation(m_yawDegrees);
	// eulerAnglesMat.AppendYRotation(m_pitchDegrees);
	// eulerAnglesMat.AppendXRotation(m_rollDegrees);
	// return eulerAnglesMat;

	// simplified way is to overlook the 0 * multiplication during the matrix calculation process
	Mat44 eulerAnglesMat = Mat44();
	
	// need to translate to radians before calculate cos and sin
	float yawCos = CosDegrees(m_yawDegrees);
	float yawSin = SinDegrees(m_yawDegrees);
	float pitchCos = CosDegrees(m_pitchDegrees);
	float pitchSin = SinDegrees(m_pitchDegrees);
	float rollCos = CosDegrees(m_rollDegrees);
	float rollSin = SinDegrees(m_rollDegrees);
	
	eulerAnglesMat.m_values[Ix] = yawCos * pitchCos;
	eulerAnglesMat.m_values[Iy] = yawSin * pitchCos;
	eulerAnglesMat.m_values[Iz] = -pitchSin;
	eulerAnglesMat.m_values[Iw] = 0.f;
	
	eulerAnglesMat.m_values[Jx] = -(yawSin * rollCos) + rollSin * yawCos * pitchSin;
	eulerAnglesMat.m_values[Jy] = yawCos * rollCos + yawSin * pitchSin * rollSin;
	eulerAnglesMat.m_values[Jz] = pitchCos * rollSin;
	eulerAnglesMat.m_values[Jw] = 0.f;
	
	eulerAnglesMat.m_values[Kx] = yawSin * rollSin + yawCos * pitchSin * rollCos;
	eulerAnglesMat.m_values[Ky] = -(yawCos * rollSin) + yawSin * pitchSin * rollCos;
	eulerAnglesMat.m_values[Kz] = rollCos * pitchCos;
	eulerAnglesMat.m_values[Kw] = 0.f;
	
	eulerAnglesMat.m_values[Tx] = 0.f;
	eulerAnglesMat.m_values[Ty] = 0.f;
	eulerAnglesMat.m_values[Tz] = 0.f;
	eulerAnglesMat.m_values[Tw] = 1.f;
	
	return eulerAnglesMat;
}

bool EulerAngles::SetFromText(char const* text)
{
	Strings asStrings = SplitStringOnDelimiter(text, ',');
	int numStrings = (int)asStrings.size();
	if (numStrings != 3)
	{
		return false;
	}
	else
	{
		m_yawDegrees = (float)atof(asStrings[0].c_str());
		m_pitchDegrees = (float)atof(asStrings[1].c_str());
		m_rollDegrees = (float)atof(asStrings[2].c_str());
		return true; // for check if the set is successful
	}
}

bool EulerAngles::operator==(EulerAngles const& compare) const
{
	if (m_yawDegrees == compare.m_yawDegrees &&
		m_pitchDegrees == compare.m_pitchDegrees &&
		m_rollDegrees == compare.m_rollDegrees)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool EulerAngles::operator!=(EulerAngles const& compare) const
{
	if (m_yawDegrees != compare.m_yawDegrees ||
		m_pitchDegrees != compare.m_pitchDegrees ||
		m_rollDegrees != compare.m_rollDegrees)
	{
		return true;
	}
	else
	{
		return false;
	}
}

EulerAngles const EulerAngles::operator+(EulerAngles const& angleToAdd) const
{
	return EulerAngles(
		m_yawDegrees + angleToAdd.m_yawDegrees,
		m_pitchDegrees + angleToAdd.m_pitchDegrees,
		m_rollDegrees + angleToAdd.m_rollDegrees);
}

EulerAngles const EulerAngles::operator-(EulerAngles const& angleToSubtract) const
{
	return EulerAngles(
		m_yawDegrees - angleToSubtract.m_yawDegrees,
		m_pitchDegrees - angleToSubtract.m_pitchDegrees,
		m_rollDegrees - angleToSubtract.m_rollDegrees);
}

EulerAngles const EulerAngles::operator/(float inverseEulerAngles) const
{
	return EulerAngles(
		m_yawDegrees / inverseEulerAngles,
		m_pitchDegrees / inverseEulerAngles,
		m_rollDegrees / inverseEulerAngles);
}

void EulerAngles::operator+=(EulerAngles const& eulerAnglesToAdd)
{
	m_yawDegrees += eulerAnglesToAdd.m_yawDegrees;
	m_pitchDegrees += eulerAnglesToAdd.m_pitchDegrees;
	m_rollDegrees += eulerAnglesToAdd.m_rollDegrees;
}

void EulerAngles::operator-=(EulerAngles const& eulerAnglesToSubtract)
{
	m_yawDegrees -= eulerAnglesToSubtract.m_yawDegrees;
	m_pitchDegrees -= eulerAnglesToSubtract.m_pitchDegrees;
	m_rollDegrees -= eulerAnglesToSubtract.m_rollDegrees;
}

void EulerAngles::operator=(EulerAngles const& copyFrom)
{
	m_yawDegrees = copyFrom.m_yawDegrees;
	m_pitchDegrees = copyFrom.m_pitchDegrees;
	m_rollDegrees = copyFrom.m_rollDegrees;
}
