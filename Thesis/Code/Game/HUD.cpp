#include "Game/HUD.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/OpenXRMathUtils.hpp"
#include "Game/Game.hpp"
#include "Game/VRPlayer.hpp"

extern BitmapFont* g_consoleFont;
extern Renderer* g_theRenderer;
extern Game* g_theGame;
extern Clock* g_theGameClock;
extern VRPlayer* g_theVRPlayer;

void HUD::UpdateHUDViewSpacePose(XrPosef updatePose)
{
	m_HUDViewSpacePose = updatePose;

	Quat rotationInOpenXR(m_HUDViewSpacePose.orientation);
	Quat trackingRotationInGameWorld = GetOpenXRToGameMat().TransformQuaternion(rotationInOpenXR);

	m_orientation = trackingRotationInGameWorld;
	m_pos = GetOpenXRToGameMat().TransformPosition3D(Vec3(m_HUDViewSpacePose.position));
}

Mat44 HUD::SetModelMatrix()
{
	Mat44 transformMat(m_pos);
	Mat44 orientationMat(m_orientation);
	Mat44 modelMat = transformMat.MatMultiply(orientationMat);

	g_theRenderer->SetModelConstants(modelMat);
	return modelMat;
}

void HUD::RenderInstructionToStartGame()
{
	std::vector<Vertex_PCU> textVerts;
	std::string text = Stringf("Initialize calibration system, please keep your head looking forward");
	g_consoleFont->AddVertsForText3DAtOriginXForward(textVerts, TEXT_HEIGHT_DEFAULT, text, Rgba8::LIGHT_ORANGE, FONT_ASPECT);
	
	// yaw the billboard 180 degrees to face the player
	Mat44 rotationMat_1 = Mat44::CreateXRotationDegrees(-90.f);
	Mat44 rotationMat_2 = Mat44::CreateYRotationDegrees(-90.f);
	Mat44 translationMat = Mat44::CreateTranslation3D(m_displayPos);
	Mat44 transformMat = translationMat.MatMultiply(rotationMat_2.MatMultiply(rotationMat_1));
	
	SetModelMatrix();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(&g_consoleFont->GetTexture());

	// Vec3 cubeHalfSize = Vec3(1.f, 1.f, 1.f) * 0.5f;
	// AABB3 bounds(cubeHalfSize * -1.f, cubeHalfSize);
	// AddVertsForAABB3D(textVerts, bounds, Rgba8::RED, Rgba8::RED, Rgba8::GREEN, Rgba8::GREEN, Rgba8::BLUE, Rgba8::BLUE);

	TransformVertexArray3D(textVerts, transformMat);

	g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
}

void HUD::RenderCountDownToStartTutorial()
{
	if (!m_glyphTimer)
	{
		m_glyphTimer = new Timer(m_timePeroidToShowAWord);
		m_glyphTimer->Start();
	}
	if (m_glyphTimer->HasPeroidElapsed())
	{
		++m_glyphCounter;
		m_glyphTimer->Start();
	}
	if (((int)m_calibrationDuration - m_timesCountingDown) == 3)
	{
		m_glyphCounter = 0;
	}

	if (!m_countDownStarts)
	{
		m_countDownTimer = new Timer(1.f);
		m_countDownTimer->Start();
		m_countDownStarts = true;
		m_timesCountingDown = 0;
	}
	else
	{
		if (m_countDownTimer->HasPeroidElapsed())
		{
			if (m_timesCountingDown == (int)m_calibrationDuration)
			{
				m_countDownStarts = false;
				g_theGame->EnterState(GameState::LOBBY);
				m_countDownStarts = false;
				return;
			}
			else
			{
				++m_timesCountingDown;
				m_countDownTimer->Restart();
			}
		}

		// show the the 3, 2, 1 on screen
		std::vector<Vertex_PCU> textVerts;
		if (((int)m_calibrationDuration - m_timesCountingDown) > 3)
		{
			std::string text = "Initialize calibration\nKeep your head looking forward\n";
			// std::string text = "Initialize calibration ";
			text += Stringf("%i", ((int)m_calibrationDuration - m_timesCountingDown - 3));
			g_consoleFont->AddVertsForText3DAtOriginXForward(textVerts, TEXT_HEIGHT_SMALL, text, Rgba8::BLUSH_PINK, FONT_ASPECT, Vec2(0.f, 0.5f), m_glyphCounter);
		}
		else
		{
			std::string text = "System Ready\nWelcome Back";
			g_consoleFont->AddVertsForText3DAtOriginXForward(textVerts, TEXT_HEIGHT_SMALL, text, Rgba8::NEON_BLUE, FONT_ASPECT, Vec2(0.f, 0.5f), m_glyphCounter);
		}

		// yaw the billboard 180 degrees to face the player and move by the distance to see the words
		Mat44 rotationMat = Mat44::CreateZRotationDegrees(180.f);
		Mat44 translationMat = Mat44::CreateTranslation3D(m_displayPos);
		TransformVertexArray3D(textVerts, translationMat.MatMultiply(rotationMat));

		SetModelMatrix();
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->BindTexture(&g_consoleFont->GetTexture());
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);

		g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
	}
}

