#include "Game/RaycastVSAABB2Mode.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/App.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/RaycastUtils.hpp"

extern App* g_theApp;
extern InputSystem* g_theInput;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;

extern Clock* g_theGameClock;

RaycastVSAABB2Mode::RaycastVSAABB2Mode()
	:GameMode()
{

}

RaycastVSAABB2Mode::~RaycastVSAABB2Mode()
{

}

void RaycastVSAABB2Mode::Startup()
{
	g_theInput->SetCursorMode(false, false);

	CreateRandomShapes();
	UpdateModeInfo();

	// set the cameras
	AABB2 cameraStart(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	//cameraStart.SetDimensions(Vec2(100.f, 50.f));
	m_worldCamera.SetOrthoView(cameraStart);
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	m_tailPos = 0.3f * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);
	m_tipPos = 0.7f * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);
}

void RaycastVSAABB2Mode::Update(float deltaSeconds)
{
	// reset list
	m_rayVerts.clear();
	m_hitAABB2 = nullptr;
	m_hasHitAABB2 = false;

	// update ray cast
	ControlTheReferenceRay(deltaSeconds);
	UpdateRaycastResultsForAllAABB2s();

	AddVertsForLines();
	AddVertsForRay();
}

void RaycastVSAABB2Mode::Render() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	RenderAllAABB2s();
	RenderRaycastResults();

	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void RaycastVSAABB2Mode::Shutdown()
{

}

void RaycastVSAABB2Mode::CreateRandomShapes()
{
	m_AABB2Verts.clear();
	m_AABB2ResultPtrList.clear();
	int numLines = g_rng->RollRandomIntInRange(9, 18);
	// float maxLength = WORLD_SIZE_Y * 0.6f;
	// float minLength = WORLD_SIZE_Y * 0.05f;

	m_AABB2ResultPtrList.resize(numLines);
	m_AABB2Verts.resize(numLines * 6);

	for (int i = 0; i < numLines; ++i)
	{
		Vec2 mins;
		mins.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.05f, WORLD_SIZE_X * 0.9f);
		mins.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.05f, WORLD_SIZE_Y * 0.9f);
		// random select one point on the world screen
		Vec2 maxs;
		maxs.x = g_rng->RollRandomFloatInRange(mins.x + 1.5f, mins.x + 1.5f + WORLD_SIZE_X * 0.2f);
		maxs.y = g_rng->RollRandomFloatInRange(mins.y + 1.5f, mins.y + 1.5f + WORLD_SIZE_Y * 0.2f);
		AABB2* newAABB2 = new AABB2(mins, maxs);
		AABB2_RaycastResult* newAABB2_Ray = new AABB2_RaycastResult(*newAABB2);

		m_AABB2ResultPtrList[i] = newAABB2_Ray;
	}
}

void RaycastVSAABB2Mode::UpdateModeInfo()
{
	m_modeName = "Mode (F6 / F7 for prev / next): Raycast VS. AABBs(2D)";
	m_controlInstruction = "LMB/RMB set ray start/end; ESDF move raycast start position, IJKL move raycast end position.\nArrow keys move the whole raycast. F8 to randomize shapes; Hold T to slow down movement";
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void RaycastVSAABB2Mode::AddVertsForRay()
{
	m_rayVerts.clear();

	if (m_hasHitAABB2)
	{
		Vec2 impactPos = m_hitAABB2->m_result.m_impactPos;
		Vec2 exitPos = m_hitAABB2->m_result.m_exitPos;
		Vec2 reflectNormalEndPoint = impactPos + (m_hitAABB2->m_result.m_impactNormal) * WORLD_SIZE_Y * 0.05f;
		Vec2 exitNormalEndPoint = exitPos + (m_hitAABB2->m_result.m_exitNormal) * WORLD_SIZE_Y * 0.05f;

		// for the impact pos
		AddVertsForArrow2D(m_rayVerts, m_tailPos, m_tipPos, m_arrowSize, m_arrowLineThickness, Rgba8::GRAY);

		if (m_shortestImpactDist != 0.f)
		{
			AddVertsForArrow2D(m_rayVerts, m_tailPos, impactPos, m_arrowSize, m_arrowLineThickness, Rgba8::RED);
		}
		AddVertsForArrow2D(m_rayVerts, impactPos, reflectNormalEndPoint, m_arrowSize * 0.6f, m_arrowLineThickness, Rgba8::YELLOW);
		AddVertesForDisc2D(m_rayVerts, impactPos, WORLD_SIZE_Y * 0.005f, Rgba8::WHITE, 12);

		// for the exit pos
		if (m_hitAABB2->m_result.m_didExit)
		{
			AddVertesForDisc2D(m_rayVerts, exitPos, WORLD_SIZE_Y * 0.005f, Rgba8::GREEN, 12);
			AddVertsForArrow2D(m_rayVerts, exitPos, exitNormalEndPoint, m_arrowSize * 0.6f, m_arrowLineThickness, Rgba8::LIGHT_ORANGE);
		}
		else
		{
			AddVertesForDisc2D(m_rayVerts, exitPos, WORLD_SIZE_Y * 0.005f, Rgba8::RED, 12);
		}
	}
	else
	{
		// when there is no impact on disc, just show a green arrow
		AddVertsForArrow2D(m_rayVerts, m_tailPos, m_tipPos, m_arrowSize, m_arrowLineThickness, Rgba8::GREEN);
	}
}

void RaycastVSAABB2Mode::AddVertsForLines()
{
	m_AABB2Verts.clear();

	for (int lineIndex = 0; lineIndex < (int)m_AABB2ResultPtrList.size(); lineIndex++)
	{
		AABB2_RaycastResult*& AABB2 = m_AABB2ResultPtrList[lineIndex];
		if (m_hasHitAABB2)
		{
			if (AABB2->m_result.m_impactDist == m_shortestImpactDist)
			{
				AddVertsForAABB2D(m_AABB2Verts, AABB2->m_AABB2, Rgba8::BLUE_MVTHL);
				m_hitAABB2 = AABB2;
				continue;
			}
			else AddVertsForAABB2D(m_AABB2Verts, AABB2->m_AABB2, Rgba8::BLUE_MVT);
		}
		else AddVertsForAABB2D(m_AABB2Verts, AABB2->m_AABB2, Rgba8::BLUE_MVT);
	}
}

void RaycastVSAABB2Mode::UpdateRaycastResultsForAllAABB2s()
{
	m_shortestImpactDist = 99999999.f;
	Vec2 rayForwardNormal = (m_tipPos - m_tailPos).GetNormalized();
	float rayDist = (m_tipPos - m_tailPos).GetLength();

	for (int lineIndex = 0; lineIndex < (int)m_AABB2ResultPtrList.size(); lineIndex++)
	{
		AABB2_RaycastResult*& AABB2 = m_AABB2ResultPtrList[lineIndex];
		RaycastResult2D result = RaycastVSAABB2(m_tailPos, rayForwardNormal, rayDist, AABB2->m_AABB2);
		AABB2->m_result = result;
		// if hit, try to get the shortest record
		if (AABB2->m_result.m_didImpact)
		{
			m_hasHitAABB2 = true;
			if (result.m_impactDist < m_shortestImpactDist)
			{
				m_shortestImpactDist = result.m_impactDist;
			}
		}
	}
}

void RaycastVSAABB2Mode::RenderAllAABB2s() const
{
	g_theRenderer->DrawVertexArray((int)m_AABB2Verts.size(), m_AABB2Verts.data());
}

void RaycastVSAABB2Mode::RenderRaycastResults() const
{
	g_theRenderer->DrawVertexArray((int)m_rayVerts.size(), m_rayVerts.data());
}

