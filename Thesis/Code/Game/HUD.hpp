#pragma once
#include "ThirdParty/OpenXR/include/openxr/openxr.h"
#include "Game/GameCommon.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Math/Mat44.hpp"

class HUD
{
public:
	HUD() {}
	~HUD() {}

	void Update();
	void UpdateHUDViewSpacePose(XrPosef updatePose);

	Mat44 SetModelMatrix();

	void RenderInstructionToStartGame();
	void RenderCountDownToStartTutorial();

	bool m_countDownStarts = false;
	Timer* m_countDownTimer = nullptr;
	int m_timesCountingDown = 0;

	Timer*	m_glyphTimer = nullptr;
	float   m_timePeroidToShowAWord = 0.05f;
	int		m_glyphCounter = 0;

	Vec3    m_displayPos = Vec3(3.5f, 0.f, 0.f);
	Vec3    m_pos;
	Quat    m_orientation;
	XrPosef m_HUDViewSpacePose;

	float m_calibrationDuration = 10.f;
};