#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Game/Shapes3DMode.hpp"
#include "Game/GameMode.hpp"
#include "Game/Player.hpp"
#include "Game/Prop.hpp"
#include "Game/App.hpp"

extern Renderer* g_theRenderer;
extern Clock* g_theGameClock;
extern RandomNumberGenerator* g_rng;
extern InputSystem* g_theInput;
extern App* g_theApp;

//----------------------------------------------------------------------------------------------------------------------------------------------------
void TestShape::Startup()
{
	// 50% will render in wire, 50% will render in texture
	float possibility = g_rng->RollRandomFloatInRange(0.f, 1.f);
	if (possibility <= 0.5f)
	{
		m_renderInWireframe = true;
	}
	AddVertsForShape();
}

void TestShape::Update()
{

}

void TestShape::Render() const
{
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->BindShader(nullptr);

	g_theRenderer->SetModelConstants(GetModelMatrix(), m_color);
	if(m_renderInWireframe)
	{
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_NONE);
	}
	else
	{
		if (m_type == GeometryType::PLANE)
		{
			g_theRenderer->BindTexture(nullptr);
		}
		else
		{
			g_theRenderer->BindTexture(g_theApp->g_textures[TESTUV]);
		}
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	}
	g_theRenderer->DrawVertexArray((int)m_vertices.size(), m_vertices.data());
}

void TestShape::ChangeColorAccordingToStatus()
{
	if (m_grabbedByPlayer && !m_isOverlaped)
	{
		m_color = Rgba8::RED;
		return;
	}

	if (m_isTheCloestImpactedShape && m_isOverlaped && !m_grabbedByPlayer) // lerp from shinning black - red
	{
		float shiningPeriod = 0.5f;
		float colorFraction = abs(sinf(g_theGameClock->GetTotalSeconds() / shiningPeriod));
		m_color = InterpolateRGBA(Rgba8::BLUE, Rgba8::GRAY, colorFraction);
		return;
	}

	if (m_grabbedByPlayer && m_isOverlaped) // lerp from shinning black - red
	{
		float shiningPeriod = 0.5f;
		float colorFraction = abs(sinf(g_theGameClock->GetTotalSeconds() / shiningPeriod));
		m_color = InterpolateRGBA(Rgba8::RED, Rgba8::GRAY, colorFraction);
		return;
	}

	if (!m_grabbedByPlayer && m_isOverlaped) // lerp from shinning black - white
	{
		float shiningPeriod = 0.5f;
		float colorFraction = abs(sinf(g_theGameClock->GetTotalSeconds() / shiningPeriod));
		m_color = InterpolateRGBA(Rgba8::WHITE, Rgba8::GRAY_Dark, colorFraction);
		return;
	}

	if (m_isTheCloestImpactedShape)
	{
		m_color = Rgba8::BLUE;
	}
	else
	{
		m_color = Rgba8::WHITE;
	}
}

void AABB3_RaycastResult::Update()
{
	Shapes3DMode* m_scene = static_cast<Shapes3DMode*>(g_theApp->m_currentGameMode);

	if (m_grabbedByPlayer)
	{
		Vec3& disp = m_scene->m_player->m_deltaMovement;
		m_position += disp;
	}

	ChangeColorAccordingToStatus();

	if (!IsPointInsideAABB3(m_scene->m_player->m_position, m_AABB3InWorld))
	{
		Vec3 nearestPt = GetNearestPointOnAABB3(m_scene->m_player->m_position, m_AABB3InWorld);
		DebugAddWorldPoint(nearestPt, 0.25f, 0.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);
	}
}

Mat44 TestShape::GetModelMatrix() const
{
	if (m_type == GeometryType::OBB3)
	{
		// because we draw an OBB3 at the center, there is no need to rotate again, do not add rotation
		Mat44 transformMat;
		transformMat.SetTranslation3D(m_position);
		return transformMat;
	}
	if (m_type == GeometryType::PLANE) // we draw world space plane directly
	{
		return Mat44();
	}
	else
	{
		Mat44 transformMat;
		transformMat.SetTranslation3D(m_position);
		Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
		transformMat.Append(orientationMat);
		return transformMat;
	}
}

void AABB3_RaycastResult::AddVertsForShape()
{
	AddVertsForAABB3D(m_vertices, m_AABB3Info, Rgba8::WHITE, AABB2::ZERO_TO_ONE);
}


void AABB3_RaycastResult::UpdateShapeInfoInWorldSpace()
{
	m_AABB3InWorld.m_mins = m_AABB3Info.m_mins + GetModelMatrix().TransformVectorQuantity3D(m_position);
	m_AABB3InWorld.m_maxs = m_AABB3Info.m_maxs + GetModelMatrix().TransformVectorQuantity3D(m_position);
}

void AABB3_RaycastResult::RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist)
{
	m_raycastResult = RaycastVsAABB3D(rayStart, rayFwdNormal, rayDist, m_AABB3InWorld);
}

void OBB3_RaycastResult::Update()
{
	Shapes3DMode* m_scene = static_cast<Shapes3DMode*>(g_theApp->m_currentGameMode);

	if (m_grabbedByPlayer)
	{
		Vec3& disp = m_scene->m_player->m_deltaMovement;
		m_position += disp;
	}

	ChangeColorAccordingToStatus();

	if (!IsPointInsideOBB3(m_scene->m_player->m_position, m_OBB3InWorld))
	{
		Vec3 nearestPt = GetNearestPointOnOBB3(m_scene->m_player->m_position, m_OBB3InWorld);
		DebugAddWorldPoint(nearestPt, 0.25f, 0.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);
	}
}

void OBB3_RaycastResult::AddVertsForShape()
{
	AddVertsForOBB3(m_vertices, m_OBB3Info);
}


void OBB3_RaycastResult::UpdateShapeInfoInWorldSpace()
{
	m_OBB3InWorld = m_OBB3Info;
	m_OBB3InWorld.m_center = m_OBB3Info.m_center + m_position;
}

void OBB3_RaycastResult::RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist)
{
	m_raycastResult = RaycastVsOBB3(rayStart, rayFwdNormal, rayDist, m_OBB3InWorld);
}

void Plane_RaycastResult::Update()
{
	Shapes3DMode* m_scene = static_cast<Shapes3DMode*>(g_theApp->m_currentGameMode);

	if (m_grabbedByPlayer)
	{
		Vec3& disp = m_scene->m_player->m_deltaMovement;
		m_position += disp;
	}

	// ChangeColorAccordingToStatus();

	Vec3 nearestPt = GetNearestPointOnPlane(m_scene->m_player->m_position, m_planeInWorld);
	DebugAddWorldPoint(nearestPt, 0.25f, 0.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);
}

void Plane_RaycastResult::AddVertsForShape()
{
	UpdateShapeInfoInWorldSpace();
	AddVertsForPlane3(m_vertices, m_planeInWorld);

	// // line from origin to plane center to world origin
	// Vec3 planeOrigin = m_planeInWorld.m_normal * m_planeInWorld.m_distAlongNormalFromOrigin;
	// AddVertsForCylinder3D(m_vertices, planeOrigin, Vec3(), 0.06f * 0.5f, Rgba8::GRAY_TRANSPARENT, AABB2::ZERO_TO_ONE, 4);
}

void Plane_RaycastResult::UpdateShapeInfoInWorldSpace()
{
	m_planeInWorld = m_planeInfo;
	m_planeInWorld.m_distAlongNormalFromOrigin = GetDistance3D(m_position, Vec3());
}

void Plane_RaycastResult::RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist)
{
	m_raycastResult = RaycastVsPlane3D(rayStart, rayFwdNormal, rayDist, m_planeInWorld);
}

void Sphere_RaycastResult::Update()
{
	Shapes3DMode* m_scene = static_cast<Shapes3DMode*>(g_theApp->m_currentGameMode);

	if (m_grabbedByPlayer)
	{
		Vec3& disp = m_scene->m_player->m_deltaMovement;
		m_position += disp;
	}

	ChangeColorAccordingToStatus();

	if (!IsPointInsideSphere(m_scene->m_player->m_position, m_sphereInWorld))
	{
		Vec3 nearestPt = GetNearestPointOnSphere(m_scene->m_player->m_position, m_sphereInWorld);
		DebugAddWorldPoint(nearestPt, 0.25f, 0.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);
	}
}

void Sphere_RaycastResult::AddVertsForShape()
{
	AddVertsForSphere3D(m_vertices, m_sphereInfo.Center, m_sphereInfo.Radius);
}

void Sphere_RaycastResult::UpdateShapeInfoInWorldSpace()
{
	m_sphereInWorld.Center = m_sphereInfo.Center + m_position;
	m_sphereInWorld.Radius = m_sphereInfo.Radius;
}

void Sphere_RaycastResult::RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist)
{
	m_raycastResult = RaycastVsSphere3D(rayStart, rayFwdNormal, rayDist, m_sphereInWorld);
}

void ZCylinder_RaycastResult::Update()
{
	Shapes3DMode* m_scene = static_cast<Shapes3DMode*>(g_theApp->m_currentGameMode);

	if (m_grabbedByPlayer)
	{
		Vec3& disp = m_scene->m_player->m_deltaMovement;
		m_position += disp;
	}

	ChangeColorAccordingToStatus();

	if (!IsPointInsideZCylinder(m_scene->m_player->m_position, m_cylinderInWorld))
	{
		Vec3 nearestPt = GetNearestPointOnZCylinder(m_scene->m_player->m_position, m_cylinderInWorld);
		DebugAddWorldPoint(nearestPt, 0.25f, 0.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);
	}
}

void ZCylinder_RaycastResult::AddVertsForShape()
{
	AddVertsForCylinder3D(m_vertices, Vec3(m_ZCylinderInfo.CenterXY, m_ZCylinderInfo.MinMaxZ.m_min), Vec3(m_ZCylinderInfo.CenterXY, m_ZCylinderInfo.MinMaxZ.m_max), m_ZCylinderInfo.Radius);
}

void ZCylinder_RaycastResult::UpdateShapeInfoInWorldSpace()
{
	float cylinderHeight = m_ZCylinderInfo.MinMaxZ.GetRangeLength();
	m_cylinderInWorld.CenterXY = m_ZCylinderInfo.CenterXY + Vec2(m_position);
	m_cylinderInWorld.Radius = m_ZCylinderInfo.Radius;
	m_cylinderInWorld.MinMaxZ = FloatRange(m_position.z, (m_position + Vec3(0.f, 0.f, cylinderHeight)).z);
}

void ZCylinder_RaycastResult::RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist)
{
	m_raycastResult = RaycastVsCylinderZ3D(rayStart, rayFwdNormal, rayDist, m_cylinderInWorld);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
Shapes3DMode::Shapes3DMode()
	:GameMode()
{

}

Shapes3DMode::~Shapes3DMode()
{

}

void Shapes3DMode::Startup()
{
	g_theInput->SetCursorMode(true, true);

	m_player = new Player(Vec3(2.5f, 8.5, 0.5f));
	m_player->Startup();

	UpdateModeInfo();

	CreateRandomShapes();
	GenerateWorldGridsAndAxes();
}

void Shapes3DMode::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	m_player->Update();

	for (size_t i = 0; i < (int)m_shapes.size(); i++)
	{
		m_shapes[i]->Update();
	}

	for (size_t i = 0; i < (int)m_shapes.size(); i++)
	{
		m_shapes[i]->UpdateShapeInfoInWorldSpace(); // this will update test shape info saved in their struct		
	}

	// whether the shape is overlapped is updated every frame
	CheckIfShapesAreOverlapped();

	ShootRaycastForCollisionTest(); 
	ClickToGrabAndReleaseShape();
	GenerateWorldAxesInfrontOfCamera();
}

void Shapes3DMode::ShootRaycastForCollisionTest()
{
	if (!m_raycastIsLock)
	{
		m_rayStart = m_player->m_position;
		Vec3 rayDisp = Vec3(m_rayDist, 0.f, 0.f);
		m_rayFwdNormal = m_player->GetModelMatrix().TransformVectorQuantity3D(rayDisp).GetNormalized();
		m_rayEnd = m_rayStart + m_rayFwdNormal * m_rayDist;
		m_rayFwdNormal = (m_rayEnd - m_rayStart).GetNormalized();
		// DebugAddWorldLine(m_rayStart, m_rayEnd, 0.01f, 10.f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::X_RAY);

		m_closetRaycastResult = RaycastAllShapes(m_rayStart, m_rayFwdNormal, m_rayDist);
		if (m_closetRaycastResult.m_didImpact)
		{
			DebugAddWorldPoint(m_closetRaycastResult.m_impactPos, 0.15f, 0.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			Vec3 arrowTip = m_closetRaycastResult.m_impactPos + m_closetRaycastResult.m_impactNormal;
			DebugAddWorldArrow(m_closetRaycastResult.m_impactPos, arrowTip, 0.1f, 0.f, Rgba8::YELLOW, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);
		}
	}
}

RaycastResult3D Shapes3DMode::RaycastAllShapes(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist)
{
	float shortestDist = rayDist;

	// update each shape's raycast result and get the shortest dist
	for (size_t i = 0; i < (int)m_shapes.size(); i++)
	{
		m_shapes[i]->m_isTheCloestImpactedShape = false;
		m_shapes[i]->RaycastVSShape(rayStart, rayFwdNormal, rayDist);
		if (m_shapes[i]->m_raycastResult.m_impactDist < shortestDist)
		{
			shortestDist = m_shapes[i]->m_raycastResult.m_impactDist;
		}
		else
		{
			continue;
		}
	}

	// according to the shortest dist, find the shape
	if (shortestDist < rayDist)
	{
		for (size_t i = 0; i < (int)m_shapes.size(); i++)
		{

			if (m_shapes[i]->m_raycastResult.m_impactDist == shortestDist)
			{
				m_impactShape = m_shapes[i];
				m_shapes[i]->m_isTheCloestImpactedShape = true;
				return m_shapes[i]->m_raycastResult;
			}
			else
			{
				continue;
			}
		}

		ERROR_AND_DIE("Could not find the one that has the shortest impact dist");
	}
	else
	{
		m_impactShape = nullptr;

		RaycastResult3D missResult;
		missResult.m_didImpact = false;
		missResult.m_impactDist = rayDist;
		missResult.m_impactPos = m_rayEnd;
		missResult.m_impactNormal = m_rayFwdNormal;

		missResult.m_didExit = false;
		missResult.m_travelDistInShape = 0.f;
		missResult.m_exitPos = rayStart;
		missResult.m_exitNormal = m_rayFwdNormal;

		missResult.m_rayFwdNormal = m_rayFwdNormal;
		missResult.m_rayStartPos = rayStart;
		missResult.m_rayDist = rayDist;

		return missResult;
	}
}

void Shapes3DMode::GenerateWorldAxesInfrontOfCamera()
{
	Vec3 rayStart = m_player->m_position;
	Vec3 rayDisp = Vec3(0.2f, 0.f, 0.f);
	Vec3 rayFwdNormal = m_player->GetModelMatrix().TransformVectorQuantity3D(rayDisp).GetNormalized();
	Vec3 rayEnd = rayStart + rayFwdNormal * m_rayDist;

	Mat44 identity;
	Mat44 worldBasis = Mat44::CreateTranslation3D(rayEnd);

	// generate the reference at the pos of the ray start
	DebugAddWorldBasis(worldBasis, 0.f, 0.09f, DebugRenderMode::X_RAY, 1.f);
}

void Shapes3DMode::ClickToGrabAndReleaseShape()
{
	// if player has grabbed nothing and press left mouse key, pick up the item
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE) && !m_hasGrabbedAShape && m_impactShape && !m_grabbedShape)
	{
		m_hasGrabbedAShape = true;
		m_impactShape->m_grabbedByPlayer = true;
		m_grabbedShape = m_impactShape;
	}

	// if player has grabbed an object and press left mouse key, release the item
	else if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE) && m_hasGrabbedAShape && m_grabbedShape)
	{
		m_hasGrabbedAShape = false;
		m_grabbedShape->m_grabbedByPlayer = false;
		m_grabbedShape = nullptr;
	}

	// space to lock raycast
	if (g_theInput->WasKeyJustPressed(' ') && !m_raycastIsLock)
	{
		m_raycastIsLock = true;

		float radius = 0.05f;
		Vec3 arrowStartPos = m_rayStart + m_rayFwdNormal * m_closetRaycastResult.m_impactDist * 0.9f;
		if (m_closetRaycastResult.m_didImpact)
		{
			// for raycast
			AddVertsForCylinder3D(m_raycastVerts, m_rayStart, m_rayEnd, radius * 0.5f, Rgba8::GRAY);
			AddVertsForCone3D(m_raycastVerts, arrowStartPos, m_closetRaycastResult.m_impactPos, radius * 2.5f, Rgba8::RED);
			AddVertsForCylinder3D(m_raycastVerts, m_rayStart, arrowStartPos, radius * 1.5f, Rgba8::RED);
			AddVertsForSphere3D(m_raycastVerts, m_closetRaycastResult.m_impactPos, radius * 2.f, Rgba8::WHITE, AABB2::ZERO_TO_ONE);

			// for impact normal
			Vec3 impactNormalTip = m_closetRaycastResult.m_impactPos + m_closetRaycastResult.m_impactNormal * 1.f;
			AddVertsForCylinder3D( m_raycastVerts, m_closetRaycastResult.m_impactPos, impactNormalTip, radius, Rgba8::YELLOW);
			AddVertsForCone3D(m_raycastVerts, impactNormalTip, (impactNormalTip + m_closetRaycastResult.m_impactNormal * 0.5f), radius * 2.5f, Rgba8::YELLOW);
		}
		else
		{
			AddVertsForCylinder3D(m_raycastVerts, m_rayStart, arrowStartPos, radius * 1.5f, Rgba8::GREEN);
			AddVertsForCone3D(m_raycastVerts, arrowStartPos, m_closetRaycastResult.m_impactPos, radius * 2.5f, Rgba8::GREEN);
		}
	}

	// space again to unlock raycast
	else if (g_theInput->WasKeyJustPressed(' ') && m_raycastIsLock)
	{
		m_raycastIsLock = false;
		m_raycastVerts.clear();
	}
}

void Shapes3DMode::Render() const
{
	RenderWorldInPlayerCamera(); // player's view
	RenderDebugRenderSystem();

	g_theRenderer->BeginCamera(m_screenCamera);
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void Shapes3DMode::Shutdown()
{

}

void Shapes3DMode::GenerateWorldGridsAndAxes()
{
	 // add a grid for the world
	 Prop* grid = new Prop();
	 grid->m_name = "Grid";
	 float spacingXY = 5.f;
	 int numOfXY = (int)(100.f / spacingXY) + 1;
	 int numOfGrid = 100 + 1;
	 float dimensionRed = 0.05f;
	 float dimensionGreen = 0.04f;
	 float dimensionGray = 0.02f;
	 // gray grid
	 for (int i = 0; i < numOfGrid; ++i)
	 {
	 	AABB3 pipe(Vec3(-50.f + (float)i - (dimensionGray * 0.5f), -50.f, -(dimensionGray * 0.5f)),
	 		Vec3( -50.f + (float)i + (dimensionGray * 0.5f), 50.f, (dimensionGray * 0.5f)));
	 	AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	 }
	 for (int i = 0; i < numOfGrid; ++i)
	 {
	 	AABB3 pipe(Vec3(-50.f, -50.f + (float)i - (dimensionGray * 0.5f), -(dimensionGray * 0.5f)),
	 		Vec3(50.f, -50.f + (float)i + (dimensionGray * 0.5f), (dimensionGray * 0.5f)));
	 	AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	 }
	 // GREEN lane
	 for (int i = 0; i < numOfXY; ++i)
	 {
	 	if ( i == (numOfXY / 2))
	 	{
	 		AABB3 pipe(Vec3(-50.f + (float)i * spacingXY - (dimensionGreen * 1.2f), -50.f, -(dimensionGreen * 2.f)),
	 			Vec3(-50.f + (float)i * spacingXY + (dimensionGreen * 2.f), 50.f, (dimensionGreen * 2.f)));
	 		AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
	 	}
	 	else 
	 	{
	 		AABB3 pipe(Vec3(-50.f + (float)i * spacingXY - (dimensionGreen * 0.5f), -50.f, -(dimensionGreen * 0.5f)),
	 			Vec3(-50.f + (float)i * spacingXY + (dimensionGreen * 0.5f), 50.f, (dimensionGreen * 0.5f)));
	 		AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
	 	}
	 }
	 // RED lane
	 for (int i = 0; i < numOfXY; ++i)
	 {
	 	if (i == (numOfXY / 2))
	 	{
	 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXY - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
	 			Vec3(50.f, -50.f + (float)i * spacingXY + (dimensionRed * 2.f), (dimensionRed * 2.f)));
	 		AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	 	}
	 	else
	 	{
	 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXY - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
	 			Vec3(50.f, -50.f + (float)i * spacingXY + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
	 		AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	 	}
	 }
	 m_gridsAndAxes.push_back(grid);
	 // ----------------------------------------------------------------------------------------------------------------------------------------------------
	 // add world basis
	 Mat44 worldBasis;
	 DebugAddWorldBasis(worldBasis, -1.f);
	 // add world text
	 Mat44 XTransform;
	 XTransform.Append(Mat44::CreateZRotationDegrees(90.f));
	 XTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, .15f)));
	 DebugAddWorldText(" X - Forward", XTransform, 0.3f, -1.f, Vec2(0.f, 0.0f), Rgba8::RED, Rgba8::RED);
	 Mat44 YTransform;
	 YTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, .15f)));
	 DebugAddWorldText("Y - Left ", YTransform, 0.3f, -1.f, Vec2(1.f, 0.0f), Rgba8::GREEN, Rgba8::GREEN);
	 
	 Mat44 ZTransform;
	 ZTransform.Append(Mat44::CreateXRotationDegrees(-90.f));
	 ZTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, -.15f)));
	 DebugAddWorldText(" Z - Up", ZTransform, 0.3f, -1.f, Vec2(0.f, 1.0f), Rgba8::BLUE, Rgba8::BLUE);
}

void Shapes3DMode::CreateRandomShapes()
{
	// because we are calling F8 to regenerate, we have to clear the array at the start of the function
	for (int i = 0; i < (int)m_shapes.size(); ++i)
	{
		delete m_shapes[i];
	}
	m_shapes.clear();

	float range = 20.f;
	FloatRange worldRangeX(range * (-1.f), range);
	FloatRange worldRangeY(range * (-1.f), range);
	FloatRange worldRangeZ(range * (-1.f), range);

	// AABB3
	int numAABB = g_rng->RollRandomIntInRange(4, 6);
	float minBoxLength = 5.f;

 	for (int i = 0; i < numAABB; ++i)
	{
		Vec3 center;
		center.x = g_rng->RollRandomFloatInRange(range * (-1.f), range);
		center.y = g_rng->RollRandomFloatInRange(range * (-1.f), range);
		center.z = g_rng->RollRandomFloatInRange(range * (-1.f), range);

		float depth = g_rng->RollRandomFloatInRange(minBoxLength, minBoxLength * 1.5f);
		float width = g_rng->RollRandomFloatInRange(minBoxLength, minBoxLength * 1.5f);
		float height = g_rng->RollRandomFloatInRange(minBoxLength, minBoxLength * 1.5f);

		Vec3 mins;
		mins.x = (-1.f) * depth * 0.5f;
		mins.y = (-1.f) * width * 0.5f;
		mins.z = (-1.f) * height * 0.5f;
		Vec3 maxs;
		maxs.x = depth * 0.5f;
		maxs.y = width * 0.5f;
		maxs.z = height * 0.5f;

		AABB3* newAABB3 = new AABB3(mins, maxs);
		TestShape* newShape = new AABB3_RaycastResult(GeometryType::AABB3D, center, *newAABB3);

		// DebugAddWorldPoint(center, 0.25f, -1.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);

		newShape->Startup();
		m_shapes.push_back(newShape);
	}

	// OBB3
	int numOBB = g_rng->RollRandomIntInRange(2, 4);

	for (int i = 0; i < numOBB; ++i)
	{
		Vec3 center;
		center.x = g_rng->RollRandomFloatInRange(range * (-1.f), range);
		center.y = g_rng->RollRandomFloatInRange(range * (-1.f), range);
		center.z = g_rng->RollRandomFloatInRange(range * (-1.f), range);

		float depth = g_rng->RollRandomFloatInRange(minBoxLength, minBoxLength * 1.5f);
		float width = g_rng->RollRandomFloatInRange(minBoxLength, minBoxLength * 1.5f);
		float height = g_rng->RollRandomFloatInRange(minBoxLength, minBoxLength * 1.5f);

		Vec3 halfDimensions;
		halfDimensions.x = depth * 0.5f;
		halfDimensions.y = width * 0.5f;
		halfDimensions.z = height * 0.5f;

		Vec3 iBasis;
		EulerAngles direction1;
		direction1.m_yawDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
		direction1.m_pitchDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
		direction1.m_rollDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
		iBasis = direction1.GetForwardIBasis().GetNormalized();		

		Mat44 modelMat = direction1.GetAsMatrix_XFwd_YLeft_ZUp();

		OBB3* newOBB3 = new OBB3(Vec3(), modelMat.GetIBasis3D(), modelMat.GetJBasis3D(), halfDimensions);
		TestShape* newShape = new OBB3_RaycastResult(GeometryType::OBB3, center, *newOBB3);
		newShape->m_orientation = newOBB3->m_iBasis.GetOrientation();

		newShape->Startup();
		m_shapes.push_back(newShape);
	}

	// Plane
	Vec3 planeNormal;
	EulerAngles planeDirection;
	planeDirection.m_yawDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
	planeDirection.m_pitchDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
	planeDirection.m_rollDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
	planeNormal = planeDirection.GetForwardIBasis().GetNormalized();

	float dist = g_rng->RollRandomFloatInRange(0.f, range * 0.5f);
	
	Plane3* plane = new Plane3(planeNormal, 0.f);
	Vec3 planeCenter = planeNormal * dist;
	// DebugAddWorldPoint(planeCenter, 0.25f, -1.f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::X_RAY);
	TestShape* newPlane = new Plane_RaycastResult(GeometryType::PLANE, planeCenter, *plane);
	newPlane->m_orientation = plane->m_normal.GetOrientation();
	newPlane->Startup();
	m_shapes.push_back(newPlane);

	// sphere
	int numSphere = g_rng->RollRandomIntInRange(4, 6);
	float minRadius = 2.5f;

	for (size_t i = 0; i < numSphere; i++)
	{
		Vec3 center;
		center.x = g_rng->RollRandomFloatInRange(range * (-1.f), range);
		center.y = g_rng->RollRandomFloatInRange(range * (-1.f), range);
		center.z = g_rng->RollRandomFloatInRange(range * (-1.f), range);

		float radius = g_rng->RollRandomFloatInRange(minRadius, minRadius * 2.f);

		Sphere* newSphere = new Sphere(Vec3(), radius);
		TestShape* newShape = new Sphere_RaycastResult(GeometryType::SPHERE, center, *newSphere);

		// DebugAddWorldPoint(center, 0.25f, -1.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);

		newShape->Startup();
		m_shapes.push_back(newShape);
	}

	// cylinder
	int numCylinder = g_rng->RollRandomIntInRange(4, 6);
	float minHeight = 2.5f;
	float minCylinderRadius = 2.5f;

	for (size_t i = 0; i < numCylinder; i++)
	{
		Vec3 center;
		center.x = g_rng->RollRandomFloatInRange(range * (-1.f), range);
		center.y = g_rng->RollRandomFloatInRange(range * (-1.f), range);
		center.z = g_rng->RollRandomFloatInRange(range * (-1.f), range);

		float radius = g_rng->RollRandomFloatInRange(minCylinderRadius, minCylinderRadius * 2.f);
		float height = g_rng->RollRandomFloatInRange(minHeight, minHeight * 2.f);
		FloatRange minMaxZ(0.f, height);

		ZCylinder* newZCylinder = new ZCylinder(Vec2(), radius, minMaxZ);
		TestShape* newShape = new ZCylinder_RaycastResult(GeometryType::ZCYLINDER, center, *newZCylinder);

		// DebugAddWorldPoint(center, 0.25f, -1.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);

		newShape->Startup();
		m_shapes.push_back(newShape);
	}
}

void Shapes3DMode::UpdateModeInfo()
{
	m_modeName = "Mode (F6 / F7 for prev / next): 3D Shapes";
	m_controlInstruction = "ESDF = fly horizontal, IJKL move raycast end position, space = lock raycast\nF8 to randomize shapes; Hold T = slow, LMB = grab object";
}

void Shapes3DMode::CheckIfShapesAreOverlapped()
{
	for (size_t i = 0; i < (int)m_shapes.size(); i++)
	{
		// reset every shapes overlap num as 0 to start
		TestShape* A = m_shapes[i];
		A->m_numOverlap = 0;
	}

	for (size_t i = 0; i < (int)m_shapes.size(); i++)
	{
		// reset every shapes overlap num as 0 to start
		TestShape* A = m_shapes[i];

		for (size_t j = 0; j < (int)m_shapes.size(); j++)
		{
			TestShape* B = m_shapes[j];

			// only test overlap if two shapes pointer are pointing the same one
			if (A != B)
			{
				bool isOverlap = false;

				if (A->m_type == GeometryType::AABB3D && B->m_type == GeometryType::AABB3D)
				{
					isOverlap = DoAABB3sOverlap(dynamic_cast<AABB3_RaycastResult*>(A)->m_AABB3InWorld, dynamic_cast<AABB3_RaycastResult*>(B)->m_AABB3InWorld);
				}
				else if (A->m_type == GeometryType::AABB3D && B->m_type == GeometryType::ZCYLINDER)
				{
					isOverlap = DoZCylinderAndAABBOverlap3D(dynamic_cast<ZCylinder_RaycastResult*>(B)->m_cylinderInWorld, dynamic_cast<AABB3_RaycastResult*>(A)->m_AABB3InWorld);
				}
				if (A->m_type == GeometryType::AABB3D && B->m_type == GeometryType::SPHERE)
				{
					isOverlap = DoSphereAndAABBOverlap3D(dynamic_cast<Sphere_RaycastResult*>(B)->m_sphereInWorld, dynamic_cast<AABB3_RaycastResult*>(A)->m_AABB3InWorld);
				}
				else if (A->m_type == GeometryType::ZCYLINDER && B->m_type == GeometryType::ZCYLINDER)
				{
					isOverlap = DoZCylindersOverlap3D(dynamic_cast<ZCylinder_RaycastResult*>(A)->m_cylinderInWorld, dynamic_cast<ZCylinder_RaycastResult*>(B)->m_cylinderInWorld);
				}
				else if (A->m_type == GeometryType::ZCYLINDER && B->m_type == GeometryType::SPHERE)
				{
					isOverlap = DoZCylinderAndSphereOverlap3D(dynamic_cast<ZCylinder_RaycastResult*>(A)->m_cylinderInWorld, dynamic_cast<Sphere_RaycastResult*>(B)->m_sphereInWorld);
				}
				else if (A->m_type == GeometryType::SPHERE && B->m_type == GeometryType::SPHERE)
				{
					isOverlap = DoSpheresOverlap3D(dynamic_cast<Sphere_RaycastResult*>(A)->m_sphereInWorld, dynamic_cast<Sphere_RaycastResult*>(B)->m_sphereInWorld);
				}
				else if (A->m_type == GeometryType::PLANE && B->m_type == GeometryType::SPHERE)
				{
					isOverlap = DoPlaneIntersectSphere(dynamic_cast<Plane_RaycastResult*>(A)->m_planeInWorld, dynamic_cast<Sphere_RaycastResult*>(B)->m_sphereInWorld);
				}
				else if (A->m_type == GeometryType::PLANE && B->m_type == GeometryType::AABB3D)
				{
					isOverlap = DoPlaneIntersectAABB3(dynamic_cast<Plane_RaycastResult*>(A)->m_planeInWorld, dynamic_cast<AABB3_RaycastResult*>(B)->m_AABB3InWorld);
				}				
				else if (A->m_type == GeometryType::PLANE && B->m_type == GeometryType::OBB3)
				{
					isOverlap = DoPlaneIntersectOBB3(dynamic_cast<Plane_RaycastResult*>(A)->m_planeInWorld, dynamic_cast<OBB3_RaycastResult*>(B)->m_OBB3InWorld);
				}

				// is there is one overlap, we will add its overlap num
				if (isOverlap)
				{
					++A->m_numOverlap;
					++B->m_numOverlap;
				}
			}
		}
	}

	// check every shape whether it has any overlap shape
	for (size_t i = 0; i < (int)m_shapes.size(); i++)
	{
		if (m_shapes[i]->m_numOverlap == 0)
		{
			m_shapes[i]->m_isOverlaped = false;
		}
		else
		{
			m_shapes[i]->m_isOverlaped = true;
		}
	}
}

void Shapes3DMode::RenderAll3DShapes() const
{
	for (int i = 0; i < (int)m_shapes.size(); ++i)
	{
		m_shapes[i]->Render();
	}
}


void Shapes3DMode::RenderWorldInPlayerCamera() const
{
	Camera& playerCamera = dynamic_cast<Player*>(m_player)->m_playerCamera;
	g_theRenderer->BeginCamera(playerCamera);
	g_theRenderer->ClearScreen(Rgba8::GRAY_Dark);//the background color setting of the window
	// render game world for props
	if (!m_gridsAndAxes.empty())
	{
		for (int i = 0; i < m_gridsAndAxes.size(); ++i)
		{
			m_gridsAndAxes[i]->Render();
		}
	}
	RenderAll3DShapes();
	RenderLockRaycast();
	// debug render
	g_theRenderer->EndCamera(playerCamera);

	DebugRenderWorld(playerCamera);
}

void Shapes3DMode::RenderLockRaycast() const
{
	if (m_raycastIsLock)
	{
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetModelConstants();
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->DrawVertexArray((int)m_raycastVerts.size(), m_raycastVerts.data());
	}
}

void Shapes3DMode::RenderDebugRenderSystem() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	if (GetDebugRenderVisibility())
	{
		// render the messages on the screen
		DebugRenderScreen(m_screenCamera);
	}
	if (g_theGameClock->IsPaused())
	{
		std::vector<Vertex_PCU> backgroundVerts;
		AddVertsForAABB2D(backgroundVerts, m_screenCamera.GetCameraBounds(), Rgba8::BLACK_TRANSPARENT);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->DrawVertexArray((int)backgroundVerts.size(), backgroundVerts.data());
	}
	g_theRenderer->EndCamera(m_screenCamera);
}
