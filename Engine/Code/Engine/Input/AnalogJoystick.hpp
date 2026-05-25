#pragma once
#include "Engine/Math/Vec2.hpp"

class AnalogJoystick
{
public:
	Vec2  GetPosition()const;
	float GetMagnitude() const;
	float GetOrientationDegrees() const;

	Vec2  GetRawUncorrectedPosition() const;
	float GetInnerDeadZoneFraction() const;
	float GetOuterDeadZoneFraction() const;

	//For use by XboxController, et al
	void Reset();
	void SetDeadZoneThresholds(float normalizedInnerDeadzoneThreshold, float normalizedOuterDeadzoneThreshold);
	void UpdatePosition(float rawNormalizedX, float rawNormalizedY);

	Vec2  m_rawPosition;									// Flaky; doesn't rest at zero (or consistently snap to rest position)
	Vec2  m_correctedPosition = Vec2(0.f, 0.f);				// Deadzone-corrected position

protected:
	float m_innerDeadZoneFraction = 0.06f;	// if R < this%, R = 0; "input range start" for corrective range map
	float m_outerDeadZoneFraction = 0.95f;	// if R > this% ，R = 1; "inputrange end" for corrective range map	
};