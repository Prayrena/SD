#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"

struct EulerAngles
{
public:
	EulerAngles();
	EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees);

	// get three axis
	void GetAsVectors_IFwd_JLeft_KUp(Vec3& out_forwardIBasis, Vec3& out_forwardJBasis, Vec3& out_forwardKBasis);
	Vec3 GetForwardIBasis();

	//	void GetAsVectors_XRight_YUp_ZLeft() this support for unity

	Mat44 GetAsMatrix_XFwd_YLeft_ZUp() const; // get a matrix based on the euler angles

	bool  SetFromText(char const* text);

	// those are degrees
	float m_yawDegrees = 0.f;
	float m_pitchDegrees = 0.f;
	float m_rollDegrees = 0.f;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// operators overloading
	// const
	bool operator ==(EulerAngles const& compare) const; // EulerAngles == EulerAngles
	bool operator !=(EulerAngles const& compare) const; // EulerAngles != vEulerAngles
	EulerAngles const operator+(EulerAngles const& angleToAdd) const;// EulerAngles + EulerAngles
	EulerAngles const operator-(EulerAngles const& angleToSubtract) const;// EulerAngles - EulerAngles
	EulerAngles const operator/(float inverseEulerAngles) const;// vec3 / vec3

	// self-mutating
	void operator += (EulerAngles const& eulerAnglesToAdd);//Vec3 += Vec3
	void operator -= (EulerAngles const& eulerAnglesToSubtract);//Vec3 -= Vec3
	void operator =  (EulerAngles const& copyFrom); //Vec3 = Vec3
};
