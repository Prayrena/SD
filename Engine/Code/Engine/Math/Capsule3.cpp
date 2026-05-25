#include "Engine/Math/Capsule3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/EulerAngles.hpp"

Capsule3::Capsule3(Capsule3 const& copyfrom)
{
	m_start = copyfrom.m_start;
	m_end = copyfrom.m_end;
	m_isFixedRadius = copyfrom.m_isFixedRadius;
	m_radius_start = copyfrom.m_radius_start;
	m_radius_end = copyfrom.m_radius_end;
}

Capsule3::Capsule3(Vec3 startPos, Vec3 endPos, bool isFixedRadius, float startRadius, float endRadius)
	: m_start(startPos)
	, m_end(endPos)
	, m_isFixedRadius(isFixedRadius)
	, m_radius_start(startRadius)
	, m_radius_end(endRadius)
{

}

void Capsule3::Translate(Vec3 const& translation)
{
	m_start += translation;
	m_end += translation;
}

void Capsule3::SetCenter(Vec3 const& newCenter)
{
	Vec3 center = (m_start + m_end) * 0.5f;
	Vec3 translation = newCenter - center;
	m_start += translation;
	m_end += translation;
}

Capsule3 Capsule3::RotateFromStart(float yaw, float pitch)
{
	Vec3 disp = m_end - m_start;
	EulerAngles orientation = disp.GetOrientation();
	orientation.m_yawDegrees += yaw;
	orientation.m_pitchDegrees += pitch;
	EulerAngles angle(yaw, pitch, 0.f);
	 
	Vec3 newDisp = Vec3::GetDirectionForYawPitch(orientation.m_yawDegrees, orientation.m_pitchDegrees);
	Capsule3 rotatedCapsule(*this);
	rotatedCapsule.m_end = rotatedCapsule.m_start + newDisp;
	return rotatedCapsule;
}
