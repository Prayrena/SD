#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Game/RaycastVSDiscsMode.hpp"
#include "Game/GameCommon.hpp"
#include "Game/ReferencePoint.hpp"
#include "Game/Entity.hpp"
#include "Game/UI.hpp"
#include "Game/App.hpp"

extern App* g_theApp;
extern InputSystem* g_theInput; 
// extern AudioSystem* g_theAudio;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;
extern Window* g_theWindow;

extern Clock* g_theGameClock;

RaycastVSDiscsMode::RaycastVSDiscsMode()
	:GameMode()
{
}

RaycastVSDiscsMode::~RaycastVSDiscsMode()
{

}

void RaycastVSDiscsMode::Startup()
{
	g_theInput->SetCursorMode(false, false);

	// respawn new testing shapes
	CreateRandomShapes();

	// set the cameras
	AABB2 cameraStart(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	//cameraStart.SetDimensions(Vec2(100.f, 50.f));
	m_worldCamera.SetOrthoView(cameraStart);
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	m_tailPos = 0.3f * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);
	m_tipPos = 0.7f * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);

	UpdateModeInfo();
}


void RaycastVSDiscsMode::Update(float deltaSeconds)
{
	// reset list
	m_rayVerts.clear();
	m_hitDisc = nullptr;

	// update ray cast
	ControlTheReferenceRay(deltaSeconds);
	UpdateRaycastResult_andHitDisc();

	AddvertsForDiscs();
	AddvertsForRay();
}

void RaycastVSDiscsMode::Render() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	RenderAllDiscs();
	RenderRaycastResults();

	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderUIElements();
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void RaycastVSDiscsMode::RenderAllDiscs() const
{
	g_theRenderer->DrawVertexArray((int)m_discsVerts.size(), m_discsVerts.data());
}

void RaycastVSDiscsMode::RenderRaycastResults() const
{
	g_theRenderer->DrawVertexArray((int)m_rayVerts.size(), m_rayVerts.data());
}

void RaycastVSDiscsMode::Shutdown()
{

}

/// <UI Settings>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Vec2 RaycastVSDiscsMode::GetRandomPosInWorld(Vec2 worldSize)
{
	float posX = g_rng->RollRandomFloatInRange(0, worldSize.x);
	float posY = g_rng->RollRandomFloatInRange(0, worldSize.y);
	return Vec2(posX, posY);
}

void RaycastVSDiscsMode::CreateRandomShapes()
{
	m_discsVerts.clear();
	m_discsPtrList.clear();
	int numDisc = g_rng->RollRandomIntInRange(9, 15);
	m_discsPtrList.resize(numDisc);

	for (int i = 0; i < numDisc; ++i)
	{
		m_discsPtrList[i] = new Disc;

		m_discsPtrList[i]->discCenter.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * .1f, WORLD_SIZE_X * .9f);
		m_discsPtrList[i]->discCenter.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .1f, WORLD_SIZE_Y * .9f);

		m_discsPtrList[i]->discRadius = g_rng->RollRandomFloatInRange(1.5f, WORLD_SIZE_Y * 0.15f);
	}

	m_discsVerts.reserve(numDisc * m_numDiscSides);
}

// raycast each disc in the list and store the ray cast information inside each disc struct
void RaycastVSDiscsMode::UpdateRaycastResult_andHitDisc()
{
	m_hasHitDisc = false;
	m_shortestImpactDist = 0.f;
	m_hasHitDisc = false;
	Vec2 raycastNormal = (m_tipPos - m_tailPos).GetNormalized();
	float rayLength = (m_tipPos - m_tailPos).GetLength();

	for (int discIndex = 0; discIndex < (int)m_discsPtrList.size(); discIndex++)
	{
		Disc*& disc = m_discsPtrList[discIndex];
		disc->result = RaycastVsDisc2D(m_tailPos, raycastNormal, rayLength, disc->discCenter, disc->discRadius);

		// if the start point is inside a disc, no need to check who is the first ray cast object
		if (IsPointInsideDisc2D(m_tailPos, disc->discCenter, disc->discRadius))
		{
			m_shortestImpactDist = 0.f;
			m_hasHitDisc = true;
			m_hitDisc = disc;
			return;
		}

		// get the nearest hit result
		if (disc->result.m_didImpact)
		{
			m_hasHitDisc = true;

			if (m_shortestImpactDist == 0.f)
			{
				m_shortestImpactDist = disc->result.m_impactDist;
			}
			else
			{
				if (disc->result.m_impactDist <= m_shortestImpactDist)
				{
					m_shortestImpactDist = disc->result.m_impactDist;
				}
			}
		}
	}
}

void RaycastVSDiscsMode::UpdateModeInfo()
{
	m_modeName = "Mode (F6 / F7 for prev / next): Test Shapes (3D)";
	m_controlInstruction = "LMB/RMB set ray start/end; ESDF move raycast start position, IJKL move raycast end position.\nArrow keys move the whole raycast. F8 to randomize shapes; Hold T to slow down movement";
}

void RaycastVSDiscsMode::AddvertsForDiscs()
{
	m_discsVerts.clear();

	if (m_shortestImpactDist == 0.f)
	{
		for (int discIndex = 0; discIndex < (int)m_discsPtrList.size(); discIndex++)
		{
			Disc*& disc = m_discsPtrList[discIndex];
			// when the tail point is inside a disc
			// if it has the nearest impact dist, then highlight the disc
			if (IsPointInsideDisc2D(m_tailPos, disc->discCenter, disc->discRadius))
			{
				AddVertesForDisc2D(m_discsVerts, disc->discCenter, disc->discRadius, Rgba8::BLUE_MVTHL, m_numDiscSides);
			}
			else AddVertesForDisc2D(m_discsVerts, disc->discCenter, disc->discRadius, Rgba8::BLUE_MVT, m_numDiscSides);
		}
		if (m_hasHitDisc)
		{
			AddVertesForDisc2D(m_discsVerts, m_hitDisc->discCenter, m_hitDisc->discRadius, Rgba8::BLUE_MVTHL, m_numDiscSides);
		}
		return;
	}

	// when the tail point is not inside a disc
	for (int discIndex = 0; discIndex < (int)m_discsPtrList.size(); discIndex++)
	{
		Disc*& disc = m_discsPtrList[discIndex];
		// if it has the nearest impact dist, then highlight the disc
		if (m_hasHitDisc)
		{
			if (disc->result.m_impactDist == m_shortestImpactDist)
			{
				AddVertesForDisc2D(m_discsVerts, disc->discCenter, disc->discRadius, Rgba8::BLUE_MVTHL, m_numDiscSides);
				m_hitDisc = disc;
				continue;
			}
			else  AddVertesForDisc2D(m_discsVerts, disc->discCenter, disc->discRadius, Rgba8::BLUE_MVT, m_numDiscSides);
		}
		else AddVertesForDisc2D(m_discsVerts, disc->discCenter, disc->discRadius, Rgba8::BLUE_MVT, m_numDiscSides);
	}
	if (m_hasHitDisc)
	{
		AddVertesForDisc2D(m_discsVerts, m_hitDisc->discCenter, m_hitDisc->discRadius, Rgba8::BLUE_MVTHL, m_numDiscSides);
	}
}

void RaycastVSDiscsMode::AddvertsForRay()
{
	m_rayVerts.clear();

	if (m_hasHitDisc)
	{
		Vec2 impactPos = m_hitDisc->result.m_impactPos;
		Vec2 exitPos = m_hitDisc->result.m_exitPos;

		Vec2 reflectNormalEndPoint = impactPos + (m_hitDisc->result.m_impactNormal) * WORLD_SIZE_Y * 0.075f;
		Vec2 exitNormalEndPoint = exitPos + (m_hitDisc->result.m_exitNormal) * WORLD_SIZE_Y * 0.075f;

		// when the tail position is inside a disc, show the original arrow as well as the hit normal
		if (m_shortestImpactDist == 0.f)
		{
			AddVertsForArrow2D(m_rayVerts, m_tailPos, m_tipPos, m_arrowSize, m_arrowLineThickness, Rgba8::GRAY);
			AddVertsForArrow2D(m_rayVerts, impactPos, reflectNormalEndPoint, m_arrowSize * 0.8f, m_arrowLineThickness, Rgba8::YELLOW);
		}
		else
		{
			AddVertsForArrow2D(m_rayVerts, m_tailPos, m_tipPos, m_arrowSize, m_arrowLineThickness, Rgba8::GRAY);
			AddVertsForArrow2D(m_rayVerts, m_tailPos, impactPos, m_arrowSize, m_arrowLineThickness, Rgba8::RED);
			AddVertsForArrow2D(m_rayVerts, impactPos, reflectNormalEndPoint, m_arrowSize * 0.8f, m_arrowLineThickness, Rgba8::YELLOW);
		}
		AddVertesForDisc2D(m_rayVerts, impactPos, WORLD_SIZE_Y * 0.005f, Rgba8::WHITE, 12);

		if (m_hitDisc->result.m_didExit)
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

// add camera shake when player is dead
void RaycastVSDiscsMode::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}


void RaycastVSDiscsMode::RenderUIElements() const
{
	if (g_theApp->m_isPaused)
	{
		std::vector<Vertex_PCU> verts;
		AABB2 testingTexture = AABB2(0.f, 0.f, 1600.f, 800.f);
		AddVertsForAABB2D(verts, testingTexture, Rgba8(0, 0, 0, 100));
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
	}
}

bool RaycastVSDiscsMode::CheckUIEnabled(UI* const UI) const
{
	UNUSED(UI);
	return false;
}

bool RaycastVSDiscsMode::DoEntitiesOverlap(Entity const& a, Entity const& b)
{
	UNUSED(a);
	UNUSED(b);
	return false;
}


