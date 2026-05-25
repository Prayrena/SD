#include "Engine/Input/AnalogJoystick.hpp"
#include "Engine/Math/MathUtils.hpp"

Vec2 AnalogJoystick::GetPosition() const
{
	return m_correctedPosition;
}

float AnalogJoystick::GetMagnitude() const
{
	return m_correctedPosition.GetLength();
}

float AnalogJoystick::GetOrientationDegrees() const
{
	return m_correctedPosition.GetOrientationDegrees();
}

Vec2 AnalogJoystick::GetRawUncorrectedPosition() const
{
	return m_rawPosition;
}

float AnalogJoystick::GetInnerDeadZoneFraction() const
{
	return m_innerDeadZoneFraction;
}

float AnalogJoystick::GetOuterDeadZoneFraction() const
{
	return m_outerDeadZoneFraction;
}

void AnalogJoystick::Reset()
{
	m_rawPosition = Vec2(0.f, 0.f);
	m_correctedPosition = Vec2(0.f, 0.f);
}

void AnalogJoystick::SetDeadZoneThresholds(float normalizedInnerDeadzoneThreshold, float normalizedOuterDeadzoneThreshold)
{
	m_innerDeadZoneFraction = normalizedInnerDeadzoneThreshold;
	m_outerDeadZoneFraction = normalizedOuterDeadzoneThreshold;
}

void AnalogJoystick::UpdatePosition(float rawNormalizedX, float rawNormalizedY)
{
	m_rawPosition = Vec2(rawNormalizedX, rawNormalizedY);

	// transform to polar system
	float rawThetaDegrees = m_rawPosition.GetOrientationDegrees();// could not be 0
	float rawLength = m_rawPosition.GetLength();

	// remap the value of the raw length
	float correctedR = RangeMapClamped(rawLength, m_innerDeadZoneFraction, m_outerDeadZoneFraction, 0.f, 1.f);	
	Vec2 correctedPolarPos = Vec2(rawThetaDegrees, correctedR);

	// transform the corrected polar coordinate to Cartesian coordinate
	m_correctedPosition = correctedPolarPos.MakeFromPolarDegrees(rawThetaDegrees, correctedR);
	// return;
}

