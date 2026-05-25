#include "Game/GetNearestPointsScene.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Game/ReferenceInput.hpp"
#include "Game/GameMode.hpp"
#include "Game/UI.hpp"
#include "Game/EnergyBar.hpp"
#include "Game/EnergySelectionRing.hpp"
#include "Game/App.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/OBB2.hpp"

extern App* g_theApp;
extern InputSystem* g_theInput; 
extern AudioSystem* g_theAudio;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;

GetNearestPointsScene::GetNearestPointsScene()
{
}

GetNearestPointsScene::~GetNearestPointsScene()
{

}

// TASK:
// spawn PlayerShip in the dead center of the world
// randomly spawn asteroids in the world
void GetNearestPointsScene::Startup()
{
	// generate the testing point
	Vec2 startPos(WORLD_SIZE_X * .5f, WORLD_SIZE_Y * .5f);
	m_testingPoint = new ReferenceInput(startPos);

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
}

void GetNearestPointsScene::Update(float deltaSeconds)
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

	if (m_UIisDisplayed)
	{
		UpdateUI(deltaSeconds);
	}
}

void GetNearestPointsScene::Render() const
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
	g_theRenderer->EndCamera(m_screenCamera);
}

void GetNearestPointsScene::ShutDownUIElementList(int arraySize, UI** m_UIElementArrayPointer)
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

void GetNearestPointsScene::Shutdown()
{
}
/// <UI Settings>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// <health bar>

void GetNearestPointsScene::InitializeEnergyBar()
{
	m_energyBar = new EnergyBar(this, Vec2(0.f, 0.f));
}

Vec2 GetNearestPointsScene::GetRandomPosInWorld(Vec2 worldSize)
{
	float posX = g_rng->RollRandomFloatInRange(0, worldSize.x);
	float posY = g_rng->RollRandomFloatInRange(0, worldSize.y);
	return Vec2(posX, posY);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void GetNearestPointsScene::CreateRandomOBB2()
{
	Vec2 worldCenter;
	worldCenter.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * .3f, WORLD_SIZE_X * .7f);
	worldCenter.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .3f, WORLD_SIZE_Y * .7f);
	Vec2 iBasisNormal;
	iBasisNormal.x = g_rng->RollRandomFloatInRange(-1.f, 1.f);
	iBasisNormal.y = sqrtf( 1.f - (iBasisNormal.x * iBasisNormal.x) );

	Vec2 halfDimensions;
	halfDimensions.x = g_rng->RollRandomFloatInRange(2.f, WORLD_SIZE_X * .1f);
	halfDimensions.y = g_rng->RollRandomFloatInRange(2.f, WORLD_SIZE_Y * .3f);

	m_OBB2 = new OBB2(worldCenter, iBasisNormal, halfDimensions);

	AddVertsForOBB2D(m_testingShapesVerts, *m_OBB2, Rgba8::GRAY_TRANSPARENT);
}

void GetNearestPointsScene::CreateRandomLineSegment2()
{
	// random select one point on the world screen
	Vec2 start;
	start.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.3f, WORLD_SIZE_X * 0.7f);
	start.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.3f, WORLD_SIZE_Y * 0.7f);
	// random select one point on the world screen
	Vec2 end;
	end.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.3f, WORLD_SIZE_X * 0.7f);
	end.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.3f, WORLD_SIZE_Y * 0.7f);

	m_lineSegment = new LineSegment2(start, end);
	AddVertsForLineSegment2D(m_testingShapesVerts, *m_lineSegment, .5f, Rgba8::GRAY_TRANSPARENT);
}

void GetNearestPointsScene::CreateRandomInfiniteLine2()
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

void GetNearestPointsScene::CreateRandomCapsule2()
{
	// random select one point on the world screen
	Vec2 start;
	start.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.3f, WORLD_SIZE_X * 0.7f);
	start.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.3f, WORLD_SIZE_Y * 0.7f);
	// random select one point on the world screen
	Vec2 end;
	end.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.3f, WORLD_SIZE_X * 0.7f);
	end.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.3f, WORLD_SIZE_Y * 0.7f);

	float radius = g_rng->RollRandomFloatInRange(2.f, WORLD_SIZE_Y * 0.1f);

	m_capsule = new Capsule2(start, end, radius);

	AddVertsForCapsule2D(m_testingShapesVerts, *m_capsule, Rgba8::GRAY_TRANSPARENT);
}

void GetNearestPointsScene::CreateRandomAABB2()
{
	Vec2 mins;
	mins.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.3f, WORLD_SIZE_X * 0.7f);
	mins.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.3f, WORLD_SIZE_Y * 0.7f);
	// random select one point on the world screen
	Vec2 maxs;
	maxs.x = g_rng->RollRandomFloatInRange(mins.x + 3.f, mins.x + WORLD_SIZE_X * 0.3f);
	maxs.y = g_rng->RollRandomFloatInRange(mins.y + 3.f, mins.y + WORLD_SIZE_Y * 0.3f);
	m_AABB2 = new AABB2(mins, maxs);

	AddVertsForAABB2D(m_testingShapesVerts, *m_AABB2, Rgba8::GRAY_TRANSPARENT);
}

void GetNearestPointsScene::CreateRandomSector2()
{
	m_sectorApertureDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
	m_sectorRadius = g_rng->RollRandomFloatInRange(3.f, WORLD_SIZE_Y * 0.3f);

	m_sectorTip.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * .2f, WORLD_SIZE_X * .8f);
	m_sectorTip.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .2f, WORLD_SIZE_Y * .8f);
	m_sectorForwardNormal = m_sectorForwardNormal.MakeFromPolarDegrees(m_sectorApertureDegrees * 0.5f, 1.f);
	AddVertesForSector2D(m_testingShapesVerts, m_sectorTip, m_sectorForwardNormal, m_sectorApertureDegrees, m_sectorRadius, Rgba8::GRAY_TRANSPARENT, 32);
}

void GetNearestPointsScene::CreateRandomDisk2()
{
	m_diskCenter.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * .3f, WORLD_SIZE_X * .7f);
	m_diskCenter.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .3f, WORLD_SIZE_Y * .7f);
	m_diskRadius = g_rng->RollRandomFloatInRange(2.f, WORLD_SIZE_Y * 0.3f);

	AddVertesForDisc2D(m_testingShapesVerts, m_diskCenter, m_diskRadius, Rgba8::GRAY_TRANSPARENT, 36);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void GetNearestPointsScene::CreateTestingPoint()
{
	// create player control point
	AddVertesForDisc2D(m_testingPointsVerts, m_testingPoint->m_position, .3f, Rgba8::WHITE, 12);
}

void GetNearestPointsScene::CreateNearestPointOnOBB2()
{
	Vec2 NearestPoint = GetNearestPointOnOBB2D(m_testingPoint->m_position, *m_OBB2);
	if (IsPointInsideOBB2D(m_testingPoint->m_position, *m_OBB2))
	{
		AddVertsForOBB2D(m_testingPointsVerts, *m_OBB2, Rgba8::CYAN);
	}
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsScene::CreateNearestPointOnLineSegment2()
{
	Vec2 NearestPoint = GetNearestPointOnLineSegment2D( m_testingPoint->m_position, *m_lineSegment);
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsScene::CreateNearestPointOnCapsule2()
{

	Vec2 NearestPoint = GetNearestPointOnCapsule2D(m_testingPoint->m_position, *m_capsule);
	if (IsPointInsideCapsule2D(m_testingPoint->m_position, *m_capsule))
	{
		AddVertsForCapsule2D(m_testingPointsVerts, *m_capsule, Rgba8::CYAN);
	}
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsScene::CreateNearestPointOnAABB2()
{
	Vec2 NearestPoint = GetNearestPointOnAABB2D(m_testingPoint->m_position, *m_AABB2);
	if (IsPointInsideAABB2D(m_testingPoint->m_position, *m_AABB2))
	{
		AddVertsForAABB2D(m_testingPointsVerts, *m_AABB2, Rgba8::CYAN);
	}
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsScene::CreateNearestPointOnDisk2()
{
	Vec2 NearestPoint = GetNearestPointOnDisc2D(m_testingPoint->m_position, m_diskCenter, m_diskRadius);
	if (IsPointInsideDisc2D(m_testingPoint->m_position, m_diskCenter, m_diskRadius))
	{
		AddVertesForDisc2D(m_testingPointsVerts, m_diskCenter, m_diskRadius, Rgba8::CYAN, 36);
	}
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsScene::CreateNearestPointOnSector2()
{
	Vec2 NearestPoint = GetNearestPointOnSector2D(m_testingPoint->m_position, m_sectorTip, m_sectorForwardNormal, m_sectorApertureDegrees, m_sectorRadius);
	if (IsPointInsideOrientedSector2D(m_testingPoint->m_position, m_sectorTip, m_sectorForwardNormal, m_sectorApertureDegrees, m_sectorRadius))
	{
		AddVertesForSector2D(m_testingPointsVerts, m_sectorTip, m_sectorForwardNormal, m_sectorApertureDegrees, m_sectorRadius, Rgba8::CYAN, 32);
	}
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsScene::CreateNearestPointOnInifiniteLine2()
{
	Vec2 NearestPoint = GetNearestPointOnInfiniteLine2D(m_testingPoint->m_position, *m_infiniteLine);
	AddVertesForDisc2D(m_testingPointsVerts, NearestPoint, .5f, Rgba8::YELLOW, 12);
	AddVertsForLineSegment2D(m_testingPointsVerts, NearestPoint, m_testingPoint->m_position, m_thicknessLinkLine, Rgba8::WHITE_TRANSPARENT);
}

void GetNearestPointsScene::RenderAllShapes() const
{
	g_theRenderer->DrawVertexArray((int)m_testingShapesVerts.size(), m_testingShapesVerts.data());
	g_theRenderer->DrawVertexArray((int)m_testingPointsVerts.size(), m_testingPointsVerts.data());
}

// add camera shake when player is dead
void GetNearestPointsScene::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

void GetNearestPointsScene::UpdateUI(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

void GetNearestPointsScene::RenderUIElements() const
{
}

bool GetNearestPointsScene::CheckUIEnabled(UI* const UI) const
{
	UNUSED(UI);
	return false;
}

bool GetNearestPointsScene::DoEntitiesOverlap(Entity const& a, Entity const& b)
{
	UNUSED(a);
	UNUSED(b);
	return false;
}

void GetNearestPointsScene::RenderUIElementList(int arraySize, UI* const* m_UIElementArrayPointer) const
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

void GetNearestPointsScene::UpdateUIElementList(int arraySize, UI* const* m_UIElementArrayPointer, float deltaSeconds) const
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



