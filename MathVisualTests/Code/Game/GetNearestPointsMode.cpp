#include "Game/GetNearestPointsMode.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Game/ReferencePoint.hpp"
#include "Game/GameMode.hpp"
#include "Game/UI.hpp"
#include "Game/App.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/OBB2.hpp"

extern App* g_theApp;
extern InputSystem* g_theInput; 
// extern AudioSystem* g_theAudio;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;

extern Clock* g_theGameClock;

GetNearestPointsMode::GetNearestPointsMode()
{
}

GetNearestPointsMode::~GetNearestPointsMode()
{

}

// TASK:
// spawn PlayerShip in the dead center of the world
// randomly spawn asteroids in the world
void GetNearestPointsMode::Startup()
{
	g_theInput->SetCursorMode(false, false);

	// generate the testing point
	Vec2 startPos = Vec2(WORLD_SIZE_X * .5f, WORLD_SIZE_Y * .5f);
	m_testingPoint = new ReferencePoint(startPos);

	// respawn new testing shapes
	m_testingShapesVerts.clear();
	CreateRandomOBB2();
	CreateRandomAABB2();
	CreateRandomLineSegment2();
	CreateRandomInfiniteLine2();
	CreateRandomDisk2();
	CreateRandomCapsule2();
	CreateRandomSector2();

	// set the cameras
	AABB2 cameraStart(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	//cameraStart.SetDimensions(Vec2(100.f, 50.f));
	m_worldCamera.SetOrthoView(cameraStart);
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	UpdateModeInfo();
}

void GetNearestPointsMode::CreateRandomShapes()
{
	// respawn new testing shapes
	m_testingShapesVerts.clear();
	CreateRandomOBB2();
	CreateRandomAABB2();
	CreateRandomLineSegment2();
	CreateRandomInfiniteLine2();
	CreateRandomDisk2();
	CreateRandomCapsule2();
	CreateRandomSector2();
}


void GetNearestPointsMode::Update(float deltaSeconds)
{
	// clear up the verts array
	m_testingPointsVerts.clear();// is there better way to reset?
	m_testingPoint->Update(deltaSeconds);

	CreateNearestPointOnOBB2();
	CreateNearestPointOnAABB2();
	CreateNearestPointOnLineSegment2();
	CreateNearestPointOnInifiniteLine2();
	CreateNearestPointOnDisk2();
	CreateNearestPointOnCapsule2();
	CreateNearestPointOnSector2();
	CreateTestingPoint();
}

void GetNearestPointsMode::Render() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);

	RenderAllShapes();

	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderUIElements();
	if (g_theApp->m_debugMode)
	{

	}
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void GetNearestPointsMode::ShutDownUIElementList(int arraySize, UI** m_UIElementArrayPointer)
{
	for (int i = 0; i < arraySize; ++i)
	{
		UI*& UIPtr = m_UIElementArrayPointer[i];// the first const says that the entity instance is const, the second const say that the entity pointer is const

		// tell every existing asteroid to draw analysis
		if (CheckUIEnabled(UIPtr)) // the function that inside a const function calls must be const
		{
			delete UIPtr;
			UIPtr = nullptr;
		}
	}

}

void GetNearestPointsMode::Shutdown()
{
}
/// <UI Settings>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// <health bar>


Vec2 GetNearestPointsMode::GetRandomPosInWorld(Vec2 worldSize)
{
	float posX = g_rng->RollRandomFloatInRange(0, worldSize.x);
	float posY = g_rng->RollRandomFloatInRange(0, worldSize.y);
	return Vec2(posX, posY);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void GetNearestPointsMode::CreateRandomOBB2()
{
	Vec2 worldCenter;
	worldCenter.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * .2f, WORLD_SIZE_X * .8f);
	worldCenter.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .2f, WORLD_SIZE_Y * .8f);
	Vec2 iBasisNormal;
	iBasisNormal.x = g_rng->RollRandomFloatInRange(-1.f, 1.f);
	iBasisNormal.y = sqrtf( 1.f - (iBasisNormal.x * iBasisNormal.x) );

	Vec2 halfDimensions;
	halfDimensions.x = g_rng->RollRandomFloatInRange(2.f, WORLD_SIZE_X * .1f);
	halfDimensions.y = g_rng->RollRandomFloatInRange(2.f, WORLD_SIZE_Y * .3f);

	m_OBB2 = new OBB2(worldCenter, iBasisNormal, halfDimensions);

	AddVertsForOBB2D(m_testingShapesVerts, *m_OBB2, Rgba8::GRAY_TRANSPARENT);
}

void GetNearestPointsMode::CreateRandomLineSegment2()
{
	// random select one point on the world screen
	Vec2 start;
	start.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.2f, WORLD_SIZE_X * 0.8f);
	start.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.2f, WORLD_SIZE_Y * 0.8f);
	// random select one point on the world screen
	Vec2 end;
	end.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.3f, WORLD_SIZE_X * 0.7f);
	end.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.3f, WORLD_SIZE_Y * 0.7f);

	m_lineSegment = new LineSegment2(start, end);
	AddVertsForLineSegment2D(m_testingShapesVerts, *m_lineSegment, .5f, Rgba8::GRAY_TRANSPARENT);
}

void GetNearestPointsMode::CreateRandomInfiniteLine2()
{
	// random select one point on the world screen
	Vec2 start;
	start.x = g_rng->RollRandomFloatInRange(0.f, WORLD_SIZE_X);
	start.y = g_rng->RollRandomFloatInRange(0.f, WORLD_SIZE_Y);
	// then randomly choose the orientation degree
	float orientationDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
	Vec2 disp = disp.MakeFromPolarDegrees(orientationDegrees, 999.f);
	Vec2 end = start + disp;
	disp = disp.GetRotatedDegrees(180.f);
	start = start + disp;

	m_infiniteLine = new LineSegment2(start, end);
	AddVertsForLineSegment2D(m_testingShapesVerts, *m_infiniteLine, .5f, Rgba8::GRAY_TRANSPARENT);
}

void GetNearestPointsMode::CreateRandomCapsule2()
{
	// random select one point on the world screen
	Vec2 start;
	start.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.2f, WORLD_SIZE_X * 0.8f);
	start.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.2f, WORLD_SIZE_Y * 0.8f);
	// random select one point on the world screen
	Vec2 end;
	end.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.2f, WORLD_SIZE_X * 0.8f);
	end.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.2f, WORLD_SIZE_Y * 0.8f);

	float radius = g_rng->RollRandomFloatInRange(2.f, WORLD_SIZE_Y * 0.1f);

	m_capsule = new Capsule2(start, end, radius);

	AddVertsForCapsule2D(m_testingShapesVerts, *m_capsule, Rgba8::GRAY_TRANSPARENT);
}

void GetNearestPointsMode::CreateRandomAABB2()
{
	Vec2 mins;
	mins.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.2f, WORLD_SIZE_X * 0.8f);
	mins.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.2f, WORLD_SIZE_Y * 0.8f);
	// random select one point on the world screen
	Vec2 maxs;
	maxs.x = g_rng->RollRandomFloatInRange(mins.x + 3.f, mins.x + WORLD_SIZE_X * 0.3f);
	maxs.y = g_rng->RollRandomFloatInRange(mins.y + 3.f, mins.y + WORLD_SIZE_Y * 0.3f);
	m_AABB2 = new AABB2(mins, maxs);

	AddVertsForAABB2D(m_testingShapesVerts, *m_AABB2, Rgba8::GRAY_TRANSPARENT);
}

void GetNearestPointsMode::CreateRandomSector2()
{
	m_sectorApertureDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
	m_sectorRadius = g_rng->RollRandomFloatInRange(3.f, WORLD_SIZE_Y * 0.3f);

	m_sectorTip.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * .2f, WORLD_SIZE_X * .8f);
	m_sectorTip.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .2f, WORLD_SIZE_Y * .8f);
	m_sectorForwardNormal = m_sectorForwardNormal.MakeFromPolarDegrees(m_sectorApertureDegrees * 0.5f, 1.f);
	AddVertesForSector2D(m_testingShapesVerts, m_sectorTip, m_sectorForwardNormal, m_sectorApertureDegrees, m_sectorRadius, Rgba8::GRAY_TRANSPARENT, 32);
}

void GetNearestPointsMode::CreateRandomDisk2()
{
	m_diskCenter.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * .2f, WORLD_SIZE_X * .8f);
	m_diskCenter.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .2f, WORLD_SIZE_Y * .8f);
	m_diskRadius = g_rng->RollRandomFloatInRange(2.f, WORLD_SIZE_Y * 0.3f);

	AddVertesForDisc2D(m_testingShapesVerts, m_diskCenter, m_diskRadius, Rgba8::GRAY_TRANSPARENT, 36);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void GetNearestPointsMode::CreateTestingPoint()
{
	// create player control point
	AddVertesForDisc2D(m_testingPointsVerts, m_testingPoint->m_position, .3f, Rgba8::WHITE, 12);
}

void GetNearestPointsMode::CreateNearestPointOnOBB2()
{
	Vec2 NearestPoint = GetNearestPointOnOBB2D(m_testingPoint->m_position, *m_OBB2);
	if (IsPointInsideOBB2D(m_testingPoint->m_position, *m_OBB2))
	{
		AddVertsForOBB2D(m_testingPointsVerts, *m_OBB2, Rgba8::CYAN);
	}
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsMode::CreateNearestPointOnLineSegment2()
{
	Vec2 NearestPoint = GetNearestPointOnLineSegment2D( m_testingPoint->m_position, *m_lineSegment);
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsMode::CreateNearestPointOnCapsule2()
{

	Vec2 NearestPoint = GetNearestPointOnCapsule2D(m_testingPoint->m_position, *m_capsule);
	if (IsPointInsideCapsule2D(m_testingPoint->m_position, *m_capsule))
	{
		AddVertsForCapsule2D(m_testingPointsVerts, *m_capsule, Rgba8::CYAN);
	}
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsMode::CreateNearestPointOnAABB2()
{
	Vec2 NearestPoint = GetNearestPointOnAABB2D(m_testingPoint->m_position, *m_AABB2);
	if (IsPointInsideAABB2D(m_testingPoint->m_position, *m_AABB2))
	{
		AddVertsForAABB2D(m_testingPointsVerts, *m_AABB2, Rgba8::CYAN);
	}
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsMode::CreateNearestPointOnDisk2()
{
	Vec2 NearestPoint = GetNearestPointOnDisc2D(m_testingPoint->m_position, m_diskCenter, m_diskRadius);
	if (IsPointInsideDisc2D(m_testingPoint->m_position, m_diskCenter, m_diskRadius))
	{
		AddVertesForDisc2D(m_testingPointsVerts, m_diskCenter, m_diskRadius, Rgba8::CYAN, 36);
	}
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsMode::CreateNearestPointOnSector2()
{
	Vec2 NearestPoint = GetNearestPointOnSector2D(m_testingPoint->m_position, m_sectorTip, m_sectorForwardNormal, m_sectorApertureDegrees, m_sectorRadius);
	if (IsPointInsideOrientedSector2D(m_testingPoint->m_position, m_sectorTip, m_sectorForwardNormal, m_sectorApertureDegrees, m_sectorRadius))
	{
		AddVertesForSector2D(m_testingPointsVerts, m_sectorTip, m_sectorForwardNormal, m_sectorApertureDegrees, m_sectorRadius, Rgba8::CYAN, 32);
	}
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsMode::UpdateModeInfo()
{
	m_modeName = "Mode (F6 / F7 for prev / next): Get nearest point";
	m_controlInstruction = "LMB/RMB set ray start/end; ESDF move raycast start position, IJKL move raycast end position.\nArrow keys move the whole raycast. F8 to randomize shapes; Hold T to slow down movement";
}

void GetNearestPointsMode::CreateNearestPointOnInifiniteLine2()
{
	Vec2 NearestPoint = GetNearestPointOnInfiniteLine2D(m_testingPoint->m_position, *m_infiniteLine);
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsMode::RenderAllShapes() const
{
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	g_theRenderer->DrawVertexArray((int)m_testingShapesVerts.size(), m_testingShapesVerts.data());
	g_theRenderer->DrawVertexArray((int)m_testingPointsVerts.size(), m_testingPointsVerts.data());
}

// add camera shake when player is dead
void GetNearestPointsMode::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

void GetNearestPointsMode::RenderUIElements() const
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

bool GetNearestPointsMode::CheckUIEnabled(UI* const UI) const
{
	UNUSED(UI);
	return false;
}

bool GetNearestPointsMode::DoEntitiesOverlap(Entity const& a, Entity const& b)
{
	UNUSED(a);
	UNUSED(b);
	return false;
}

void GetNearestPointsMode::RenderUIElementList(int arraySize, UI* const* m_UIElementArrayPointer) const
{
	for (int i = 0; i < arraySize; ++i)
	{
		UI* const UIptr = m_UIElementArrayPointer[i];// we only need to promise that the entity ptr will not be changed because the instance that the pointer point to do not own by the class
		if (CheckUIEnabled(UIptr))
		{
			UIptr->Render();
		}
	}
}

void GetNearestPointsMode::UpdateUIElementList(int arraySize, UI* const* m_UIElementArrayPointer, float deltaSeconds) const
{
	for (int i = 0; i < arraySize; ++i)
	{
		UI* const UIptr = m_UIElementArrayPointer[i];// we only need to promise that the entity ptr will not be changed because the instance that the pointer point to do not own by the class
		if (CheckUIEnabled(UIptr))
		{
			UIptr->Update(deltaSeconds);
		}
	}
}



