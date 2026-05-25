#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Math/Easing.hpp"
#include "Engine/Math/Splines.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/SecondOrderDynamics.hpp"
#include "Game/SecondOrderDynamicsMode.hpp"
#include "Game/GameCommon.hpp"
#include "Game/ReferencePoint.hpp"
#include "Game/Entity.hpp"
#include "Game/App.hpp"

extern App* g_theApp;
extern InputSystem* g_theInput;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;
extern Window* g_theWindow;
extern Clock* g_theGameClock;
extern BitmapFont* g_consoleFont;

SecondOrderDynamicsMode::SecondOrderDynamicsMode()
{}

SecondOrderDynamicsMode::~SecondOrderDynamicsMode()
{}

void SecondOrderDynamicsMode::Startup()
{
	g_theInput->SetCursorMode(false, false);

	m_inputTimer = new Timer(20.f, g_theGameClock);
	m_samplerTimer = new Timer(m_sampleDuration, g_theGameClock);

	// set the cameras
	AABB2 worldBox(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	//cameraStart.SetDimensions(Vec2(100.f, 50.f));
	m_worldCamera.SetOrthoView(worldBox);
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	// get three box areas
	m_diagramBox = worldBox.GetBoxAtUVs(Vec2(0.12f, 0.1f), Vec2(0.88f, 0.9f));

	// for the easing diagram
	AddVertsForDiagramBox();

	// get the size for the diagram text box for printing out f r z
	// m_diagramBox = worldBox.GetBoxAtUVs(Vec2(0.1f, 0.1f), Vec2(0.9f, 0.9f));
	// m_diagramNameBox = m_easingBox;
	// m_diagramNameBox.Translate(Vec2(0.f, m_stringHeight * -1.f));
	// m_diagramNameBox.m_maxs.y = m_diagramNameBox.m_mins.y + m_stringHeight;

	m_secondOrderModifier = new SecondOrderDynamics(0.6f, 0.05f, -.5f, Vec3::ZERO); 
}

void SecondOrderDynamicsMode::AddVertsForDiagramBox()
{
	AddVertsForAABB2D(m_debugVerts, m_diagramBox, m_debugBoxColor);

	Vec2 BL = m_diagramBox.GetPointAtUV(Vec2(0.f, 0.f));
	Vec2 BR = m_diagramBox.GetPointAtUV(Vec2(1.f, 0.f));
	Vec2 TL = m_diagramBox.GetPointAtUV(Vec2(0.f, 1.f));

	AddVertsForArrow2D(m_debugVerts, BL, BR, m_arrowSize, m_arrowLineThickness, m_arrowColor);
	AddVertsForArrow2D(m_debugVerts, BL, TL, m_arrowSize, m_arrowLineThickness, m_arrowColor);
}

void SecondOrderDynamicsMode::Update(float deltaSeconds)
{
	(void) deltaSeconds;

	UpdateInput();
	UpdateModeInfo();
	UpdateInputCurve();
	UpdateSecondOrderCurve();
	UpdateDebugMessages();

	if (!m_inputTimer->IsStopped() && m_samplerTimer->HasPeroidElapsed())
	{
		m_samplerTimer->Restart();
	}

	if (m_inputTimer->HasPeroidElapsed())
	{
		m_inputTimer->Stop();
		m_samplerTimer->Stop();
	}

	// m_parametricallyMovingPtVerts.clear();
	// if (!m_inputTimer->HasPeroidElapsed())
	// {
	// 	float t = m_inputTimer->GetElapsedTime() / m_movingDuration;
	// 	UpdateSecondOrderDynamicsDiagram(t);
	// 	UpdateCubicBezierDiagram(t);
	// 	UpdateCatmullRomDiagram();
	// }
	// else
	// {
	// 	float t = 1.f;
	// 	UpdateSecondOrderDynamicsDiagram(t);
	// 	UpdateCubicBezierDiagram(t);
	// 	UpdateCatmullRomDiagram();
	// 
	// 	m_inputTimer->Restart();
	// }

}

void SecondOrderDynamicsMode::UpdateDebugMessages()
{
	// Vec2  textPos = Vec2(1.f, 0.f);
	Vec2  spacing = Vec2(0.f, 0.04f);

	Vec2  slot3 = Vec2(0.f, 0.1f);
	Vec2  slot2 = slot3 + spacing;
	Vec2  slot1 = slot2 + spacing;
	Vec2  slot4 = slot3 - spacing;

	float fontSize = 24.f;

	std::string curve_FValue = Stringf("f: %.2f(H/Y)", m_secondOrderModifier->m_f);
	std::string curve_ZValue = Stringf("z: %.2f(U/J)", m_secondOrderModifier->m_z);
	std::string curve_RValue = Stringf("r: %.2f(I/K)", m_secondOrderModifier->m_r);
	DebugAddScreenText(curve_FValue, Vec2(), fontSize, slot1, -1.f);
	DebugAddScreenText(curve_ZValue, Vec2(), fontSize, slot2, -1.f);
	DebugAddScreenText(curve_RValue, Vec2(), fontSize, slot3, -1.f);

	Vec2  xAxisNamePos = Vec2(0.92f, 0.1f);
	Vec2  yAxisNamePos = Vec2(0.04f, 0.9f);

	std::string xAxisName = "Time";
	std::string yAxisName = "Position";

	DebugAddScreenText( xAxisName, Vec2(), fontSize, xAxisNamePos, -1.f);
	DebugAddScreenText( yAxisName, Vec2(), fontSize, yAxisNamePos, -1.f);
}

void SecondOrderDynamicsMode::UpdateInput()
{
	float deltatime = m_inputTimer->GetDeltaSecondsFromClock();

	// if the timer is stopped and player start to input
	// start all curves over
	if (g_theInput->IsKeyDown('W') || g_theInput->IsKeyDown('S'))
	{
		if (m_inputTimer->IsStopped())
		{
			m_inputTimer->Start();
			m_samplerTimer->Start();
			m_deltatime_velocity_recording.clear();

			m_playerInputCurveVerts.clear();
			m_secondOrderCurveVerts.clear();

			for (auto& curve : m_curves)
			{
				curve.m_samplePoints.clear();
			}

			m_currentPos = Vec3::ZERO;

			RecordRFZValueAndRecreateSecondOrderDynamics();
		}	
	}

	// record the input by time step
	m_currentVelocity = 0.f;
	if (g_theInput->IsKeyDown('W'))
	{
		// m_inputValue += g_theGameClock->GetDeltaSeconds() * m_moveSpeed;
		m_currentVelocity = m_moveSpeed;
	}	
	if (g_theInput->IsKeyDown('S'))
	{
		// m_inputValue -= g_theGameClock->GetDeltaSeconds() * m_moveSpeed;
		m_currentVelocity = -m_moveSpeed;
	}

	if (g_theInput->WasKeyJustPressed('R'))
	{
		m_currentVelocity = 0.f;
		m_inputTimer->Stop();
		m_samplerTimer->Stop();

		m_playerInputCurveVerts.clear();
		m_secondOrderCurveVerts.clear();

		m_deltatime_velocity_recording.clear();

		for (auto& curve : m_curves)
		{
			curve.m_samplePoints.clear();
		}

		m_currentPos = Vec3::ZERO;

		delete 	m_secondOrderModifier;
		m_secondOrderModifier = new SecondOrderDynamics(1.f, 0.25f, -0.05f, Vec3::ZERO);
	}

	// record the input when input timer is on
	if (!m_inputTimer->IsStopped())
	{
		float currentVelocity = m_currentVelocity;
		m_deltatime_velocity_recording.emplace_back(deltatime, currentVelocity);
	}

	float rate = m_changeRate;
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		rate *= 30.f;
	}

	if (g_theInput->WasKeyJustPressed('Y') || g_theInput->IsKeyDown('Y'))
	{
		m_secondOrderModifier->m_f += rate;
		m_secondOrderModifier->UpdateParameters(m_secondOrderModifier->m_f, m_secondOrderModifier->m_z, m_secondOrderModifier->m_r, Vec3::ZERO);
		RebuildSecondOrderCurve();
	}
	if (g_theInput->WasKeyJustPressed('H') || g_theInput->IsKeyDown('H'))
	{
		m_secondOrderModifier->m_f -= rate;
		m_secondOrderModifier->UpdateParameters(m_secondOrderModifier->m_f, m_secondOrderModifier->m_z, m_secondOrderModifier->m_r, Vec3::ZERO);
		RebuildSecondOrderCurve();
	}
	if (g_theInput->WasKeyJustPressed('U') || g_theInput->IsKeyDown('U'))
	{
		m_secondOrderModifier->m_z += rate;
		m_secondOrderModifier->UpdateParameters(m_secondOrderModifier->m_f, m_secondOrderModifier->m_z, m_secondOrderModifier->m_r, Vec3::ZERO);
		RebuildSecondOrderCurve();
	}
	if (g_theInput->WasKeyJustPressed('J') || g_theInput->IsKeyDown('J'))
	{
		m_secondOrderModifier->m_z -= rate;
		m_secondOrderModifier->UpdateParameters(m_secondOrderModifier->m_f, m_secondOrderModifier->m_z, m_secondOrderModifier->m_r, Vec3::ZERO);
		RebuildSecondOrderCurve();
	}
	if (g_theInput->WasKeyJustPressed('I') || g_theInput->IsKeyDown('I'))
	{
		m_secondOrderModifier->m_r += rate;
		m_secondOrderModifier->UpdateParameters(m_secondOrderModifier->m_f, m_secondOrderModifier->m_z, m_secondOrderModifier->m_r, Vec3::ZERO);
		RebuildSecondOrderCurve();
	}
	if (g_theInput->WasKeyJustPressed('K') || g_theInput->IsKeyDown('K'))
	{
		m_secondOrderModifier->m_r -= rate;
		m_secondOrderModifier->UpdateParameters(m_secondOrderModifier->m_f, m_secondOrderModifier->m_z, m_secondOrderModifier->m_r, Vec3::ZERO);
		RebuildSecondOrderCurve();
	}

	// control time step
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTARROW))
	{
		RecordRFZValueAndRecreateSecondOrderDynamics();

		m_secondOrderModifier->m_physicsFixedTimestep *= 0.9f;
		RebuildSecondOrderCurve();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTARROW))
	{
		RecordRFZValueAndRecreateSecondOrderDynamics();

		m_secondOrderModifier->m_physicsFixedTimestep *= 1.1f;
		RebuildSecondOrderCurve();
	}

	if (g_theInput->IsKeyDown(KEYCODE_LEFTBRACKET))
	{
		m_moveSpeed -= m_speedChangeRate * deltatime;
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHTBRACKET))
	{
		m_moveSpeed += m_speedChangeRate * deltatime;
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM1))
	{
		float f = 1.f;
		float z = .5f;
		float r = 2.f;

		delete 	m_secondOrderModifier;
		m_secondOrderModifier = new SecondOrderDynamics(f, z, r, Vec3::ZERO);
		RebuildSecondOrderCurve();
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM2))
	{
		float f = 1.f;
		float z = 0.5f;
		float r = -2.f;

		delete 	m_secondOrderModifier;
		m_secondOrderModifier = new SecondOrderDynamics(f, z, r, Vec3::ZERO);
		RebuildSecondOrderCurve();
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM3))
	{
		float f = 1.f;
		float z = 1.f;
		float r = 0.f;

		delete 	m_secondOrderModifier;
		m_secondOrderModifier = new SecondOrderDynamics(f, z, r, Vec3::ZERO);
		RebuildSecondOrderCurve();
	}
	// if (g_theInput->WasKeyJustPressed(KEYCODE_NUM4))
	// {
	// 	q = Quat(0.f, 0.f, 0.f, 1.f);
	// }
	// if (g_theInput->WasKeyJustPressed(KEYCODE_NUM5))
	// {
	// 	q = Quat(0.707f, 0.f, 0.f, 0.707f);
	// }
	// if (g_theInput->WasKeyJustPressed(KEYCODE_NUM6))
	// {
	// 	q = Quat(0.f, 0.707f, 0.f, 0.707f);
	// }
	// if (g_theInput->WasKeyJustPressed(KEYCODE_NUM7))
	// {
	// 	q = Quat(0.f, 0.f, 0.707f, 0.707f);
	// }
}

void SecondOrderDynamicsMode::UpdateInputCurve()
{
	if (!m_inputTimer->IsStopped())
	{
		Vec2 BL = m_diagramBox.GetPointAtUV(Vec2(0.f, 0.f));
		Vec2 BR = m_diagramBox.GetPointAtUV(Vec2(1.f, 0.f));

		float timeFraction = m_inputTimer->GetElapsedFraction();
		Vec2 xOnAxis = Interpolate(BL, BR, timeFraction);

		m_currentPos.y += m_currentVelocity * g_theGameClock->GetDeltaSeconds();
		// m_currentPos.x = xOnAxis.x;
		// m_recordedPositions.push_back(m_currentPos);

		if (m_samplerTimer->HasPeroidElapsed())
		{
			Vec3 samplePoint = m_currentPos;
			samplePoint.y += BL.y;
			samplePoint.x =  xOnAxis.x;

			m_curves[(int)Curve::PLAYER_INPUT].m_samplePoints.push_back(samplePoint);

			m_playerInputCurveVerts.clear();
			AddVertsForSamplePointsCurve(m_playerInputCurveVerts, m_curves[(int)Curve::PLAYER_INPUT], m_smoothLineThickness, m_smoothCurveColor, false, m_numCurveSections);
		}
	}
	// else
	// {
	// 	if (!m_playerInputCurveVerts.empty())
	// 	{
	// 		AddVertsForSamplePointsCurve(m_playerInputCurveVerts, m_curves[(int)Curve::PLAYER_INPUT], m_smoothLineThickness, m_smoothCurveColor, false, m_numCurveSections);
	// 	}
	// }
}

void SecondOrderDynamicsMode::UpdateSecondOrderCurve()
{
	if (!m_inputTimer->IsStopped())
	{
		m_secondOrderModifier->Update(g_theGameClock->GetDeltaSeconds(), m_currentPos);

		if (m_samplerTimer->HasPeroidElapsed())
		{
			Vec3 modifiedPosition = m_secondOrderModifier->m_goal_position;

			Vec2 BL = m_diagramBox.GetPointAtUV(Vec2(0.f, 0.f));
			Vec2 BR = m_diagramBox.GetPointAtUV(Vec2(1.f, 0.f));
			float timeFraction = m_inputTimer->GetElapsedFraction();
			Vec2 xOnAxis = Interpolate(BL, BR, timeFraction);

			modifiedPosition.x = xOnAxis.x;
			modifiedPosition.y += BL.y;

			m_curves[(int)Curve::SECOND_ORDER_DYNAMICS].m_samplePoints.push_back(modifiedPosition);

			m_secondOrderCurveVerts.clear();
			AddVertsForSamplePointsCurve(m_secondOrderCurveVerts, m_curves[(int)Curve::SECOND_ORDER_DYNAMICS], m_smoothLineThickness, m_secondOrderCurveColor, false, m_numCurveSections);
		}
	}
	// else
	// {
	// 	if (!m_secondOrderCurveVerts.empty())
	// 	{
	// 		AddVertsForSamplePointsCurve(m_secondOrderCurveVerts, m_curves[(int)Curve::SECOND_ORDER_DYNAMICS], m_smoothLineThickness, m_secondOrderCurveColor, false, m_numCurveSections);
	// 	}
	// }
}

void SecondOrderDynamicsMode::RecordRFZValueAndRecreateSecondOrderDynamics()
{
	// record current f, z, r value
	float f = m_secondOrderModifier->m_f;
	float z = m_secondOrderModifier->m_z;
	float r = m_secondOrderModifier->m_r;

	float timeStep = m_secondOrderModifier->m_physicsFixedTimestep ;

	delete 	m_secondOrderModifier;
	m_secondOrderModifier = new SecondOrderDynamics(f, z, r, Vec3::ZERO);
	m_secondOrderModifier->m_physicsFixedTimestep = timeStep;
}

void SecondOrderDynamicsMode::RebuildSecondOrderCurve()
{
	m_curves[(int)Curve::SECOND_ORDER_DYNAMICS].m_samplePoints.clear();

	Vec3 pos = Vec3::ZERO;
	float currentTime = 0.f;
	float wholeDuration = m_inputTimer->m_period;

	Vec2 BL = m_diagramBox.GetPointAtUV(Vec2(0.f, 0.f));
	Vec2 BR = m_diagramBox.GetPointAtUV(Vec2(1.f, 0.f));

	for (auto const& recording : m_deltatime_velocity_recording) 
	{
		float deltaTime = recording.first;
		float velocity = recording.second;

		pos.y += velocity * deltaTime;
		m_secondOrderModifier->Update(deltaTime, pos);
		
		Vec3 modifiedPosition = m_secondOrderModifier->m_goal_position;

		currentTime += deltaTime;
		float timeFraction = currentTime / wholeDuration;
		Vec2 xOnAxis = Interpolate(BL, BR, timeFraction);

		modifiedPosition.x = xOnAxis.x;
		modifiedPosition.y += BL.y;

		m_curves[(int)Curve::SECOND_ORDER_DYNAMICS].m_samplePoints.push_back(modifiedPosition);
	}

	m_secondOrderCurveVerts.clear();
	AddVertsForSamplePointsCurve(m_secondOrderCurveVerts, m_curves[(int)Curve::SECOND_ORDER_DYNAMICS], m_smoothLineThickness, m_secondOrderCurveColor, false, m_numCurveSections);
}
 
void SecondOrderDynamicsMode::UpdateModeInfo()
{
	m_modeName = "Mode (F6 / F7 for prev / next): Second Order Dynamics(2D)";
	m_controlInstruction = Stringf("Press R to clear and reset curves. W/S to move back and forth");

	float frameTimeMs = 1000.f * m_secondOrderModifier->m_physicsFixedTimestep;

	m_controlInstruction = Stringf("Press R to clear and reset curves. W/S to move back and forth, timeStep= %.02fms(<- / ->), speed= %.02f([ / ])", frameTimeMs, m_moveSpeed);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// render
void SecondOrderDynamicsMode::Render() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	RenderDebug();
	RenderCurves();
	// RenderMovingPoints();

	// g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	// g_theRenderer->BindTexture(&g_consoleFont->GetTexture());
	// RenderDiagramName();

	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	DebugRenderScreen(m_screenCamera);
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void SecondOrderDynamicsMode::RenderDebug() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	g_theRenderer->DrawVertexArray((int)m_debugVerts.size(), m_debugVerts.data());

	g_theRenderer->EndCamera(m_worldCamera);
}

void SecondOrderDynamicsMode::RenderMovingPoints() const
{
	g_theRenderer->DrawVertexArray((int)m_parametricallyMovingPtVerts.size(), m_parametricallyMovingPtVerts.data());
}

void SecondOrderDynamicsMode::RenderCurves() const
{
	// g_theRenderer->DrawVertexArray((int)m_parametricallyMovingPtVerts.size(), m_parametricallyMovingPtVerts.data());
	g_theRenderer->DrawVertexArray((int)m_secondOrderCurveVerts.size(), m_secondOrderCurveVerts.data());
	g_theRenderer->DrawVertexArray((int)m_playerInputCurveVerts.size(), m_playerInputCurveVerts.data());
}

void SecondOrderDynamicsMode::Shutdown()
{

}

