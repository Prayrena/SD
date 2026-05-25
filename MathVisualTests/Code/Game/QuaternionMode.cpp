#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Game/QuaternionMode.hpp"
#include "Game/GameMode.hpp"
#include "Game/Player.hpp"
#include "Game/Prop.hpp"
#include "Game/App.hpp"

using namespace std;

extern Renderer* g_theRenderer;
extern Clock* g_theGameClock;
extern RandomNumberGenerator* g_rng;
extern InputSystem* g_theInput;
extern App* g_theApp;

QuaternionMode* g_theQuaternionMode = nullptr;

//----------------------------------------------------------------------------------------------------------------------------------------------------
QuaternionMode::QuaternionMode()
	:GameMode()
{

}

QuaternionMode::~QuaternionMode()
{

}

void QuaternionMode::Startup()
{
	g_theInput->SetCursorMode(true, true);

	m_player = new Player(Vec3(-2.5f, -2.5f, 4.f));
	m_player->Startup();

	UpdateModeInfo();

	CreateRandomShapes();
	GenerateWorldGridsAndAxes();

	for (int i = 0; i < static_cast<int>(QUAT::NUM_QUAT); ++i)
	{
		m_quatArray[i] = Quat();
	}	
	Quat testQuat = ConvertEulerAnglesToQuat(EulerAngles(-30.f, 0.f, 0.f));
	m_quatArray[0] = testQuat;

	ScribeQuatModeCommands();

	m_splineFlowingTimer = new Timer(m_splineFlowingInterval, g_theGameClock);
	m_splineFlowingTimer->Start();

	m_blendingTimer = new Timer(1.f);
}

void QuaternionMode::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	// g_theInput->SetCursorMode(true, true);

	m_player->Update();
	ControlQuaternion();
	InputNumKeyToSetQuat();

	ControlTrajectory();
	UpdateQuatShortestPathSpline();
	AddingVertsForMovingPathSpline();

	UpdateQuatBlending();

	UpdateDebugMessages();
}

void QuaternionMode::ClickToGrabAndReleaseShape()
{
	//// if player has grabbed nothing and press left mouse key, pick up the item
	//if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE) && !m_hasGrabbedAShape && m_impactShape && !m_grabbedShape)
	//{
	//	m_hasGrabbedAShape = true;
	//	m_impactShape->m_grabbedByPlayer = true;
	//	m_grabbedShape = m_impactShape;
	//}

	//// if player has grabbed an object and press left mouse key, release the item
	//else if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE) && m_hasGrabbedAShape && m_grabbedShape)
	//{
	//	m_hasGrabbedAShape = false;
	//	m_grabbedShape->m_grabbedByPlayer = false;
	//	m_grabbedShape = nullptr;
	//}

	//// space to lock raycast
	//if (g_theInput->WasKeyJustPressed(' ') && !m_raycastIsLock)
	//{
	//	m_raycastIsLock = true;

	//	float radius = 0.05f;
	//	Vec3 arrowStartPos = m_rayStart + m_rayFwdNormal * m_closetRaycastResult.m_impactDist * 0.9f;
	//	if (m_closetRaycastResult.m_didImpact)
	//	{
	//		// for raycast
	//		AddVertsForCylinder3D(m_raycastVerts, m_rayStart, m_rayEnd, radius * 0.5f, Rgba8::GRAY);
	//		AddVertsForCone3D(m_raycastVerts, arrowStartPos, m_closetRaycastResult.m_impactPos, radius * 2.5f, Rgba8::RED);
	//		AddVertsForCylinder3D(m_raycastVerts, m_rayStart, arrowStartPos, radius * 1.5f, Rgba8::RED);
	//		AddVertsForSphere3D(m_raycastVerts, m_closetRaycastResult.m_impactPos, radius * 2.f, Rgba8::WHITE, AABB2::ZERO_TO_ONE);

	//		// for impact normal
	//		Vec3 impactNormalTip = m_closetRaycastResult.m_impactPos + m_closetRaycastResult.m_impactNormal * 1.f;
	//		AddVertsForCylinder3D( m_raycastVerts, m_closetRaycastResult.m_impactPos, impactNormalTip, radius, Rgba8::YELLOW);
	//		AddVertsForCone3D(m_raycastVerts, impactNormalTip, (impactNormalTip + m_closetRaycastResult.m_impactNormal * 0.5f), radius * 2.5f, Rgba8::YELLOW);
	//	}
	//	else
	//	{
	//		AddVertsForCylinder3D(m_raycastVerts, m_rayStart, arrowStartPos, radius * 1.5f, Rgba8::GREEN);
	//		AddVertsForCone3D(m_raycastVerts, arrowStartPos, m_closetRaycastResult.m_impactPos, radius * 2.5f, Rgba8::GREEN);
	//	}
	//}

	//// space again to unlock raycast
	//else if (g_theInput->WasKeyJustPressed(' ') && m_raycastIsLock)
	//{
	//	m_raycastIsLock = false;
	//	m_raycastVerts.clear();
	//}
}

void QuaternionMode::Render() const
{
	RenderWorldInPlayerCamera(); // player's view
	RenderDebugRenderSystem();

	g_theRenderer->BeginCamera(m_screenCamera);
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void QuaternionMode::Shutdown()
{

}

void QuaternionMode::GenerateWorldGridsAndAxes()
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

void QuaternionMode::CreateRandomShapes()
{
	CreateVerticeAndVertexBufferForSphere();
	AddVertsAndCreateVertexBufferForAllDebugArrows();
}

void QuaternionMode::CreateVerticeAndVertexBufferForSphere()
{
	AddVertsForSphere3D(m_sphereVerts, Vec3::ZERO, m_sphereRadius, m_sphereDefaultColor, AABB2::ZERO_TO_ONE, 24, 12);

	// create the vertex buffer for all the convex
	m_sphereVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_sphereVerts.size()), sizeof(Vertex_PCU));

	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArrayDataSize = (m_sphereVerts.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_sphereVerts.data(), vertexArrayDataSize, m_sphereVertexBuffer);
}

Mat44 QuaternionMode::GetSphereModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_sphereCenter);
	Mat44 orientationMat = m_sphereOrientation.GetAsMatrix_XFwd_YLeft_ZUp();
	transformMat.Append(orientationMat);
	return transformMat;
}

void QuaternionMode::RenderSphere() const
{
	if (!m_sphereVerts.empty())
	{
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);

		g_theRenderer->SetModelConstants(GetSphereModelMatrix());
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexBuffer(m_sphereVertexBuffer, (int)m_sphereVerts.size());
	}
}

void QuaternionMode::ControlQuaternion()
{
	// H/Y - X
	// U/J - Y
	// I/K - Z
	// O/L - W
	Quat& q = m_quatArray[m_quatIndexInControl];

	// use N/M to control which quat in control
	if (g_theInput->WasKeyJustPressed('N'))
	{
		--m_quatIndexInControl;
		if (m_quatIndexInControl == -1)
		{
			m_quatIndexInControl = 1;
		}
	}
	if (g_theInput->WasKeyJustPressed('M'))
	{
		++m_quatIndexInControl;
		if (m_quatIndexInControl == 2)
		{
			m_quatIndexInControl = 0;
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// change blend weight
	if (g_theInput->IsKeyDown(KEYCODE_LEFTBRACKET))
	{
		m_blendingWeight -= m_blendingWeightChangeSpeed;
		if (m_blendingWeight < 0.f)
		{
			m_blendingWeight = 0.f;
		}

		m_blendingTimer->Stop();
	}
	if (g_theInput->IsKeyDown(KEYCODE_RIGHTBRACKET))
	{
		m_blendingWeight += m_blendingWeightChangeSpeed;
		if (m_blendingWeight > 1.f )
		{
			m_blendingWeight = 1.f;
		}

		m_blendingTimer->Stop();
	}	

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (g_theInput->IsKeyDown(KEYCODE_LESSTHAN))
	{
		if (m_blendingTimer->m_period > m_blendingTimerChangeSpeed * 2.f)
		{
			m_blendingTimer->m_period -= m_blendingTimerChangeSpeed;
		}
	}
	if (g_theInput->IsKeyDown(KEYCODE_GREATERTHAN))
	{
		m_blendingTimer->m_period += m_blendingTimerChangeSpeed;
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (g_theInput->WasKeyJustPressed(' '))
	{
		if (m_blendingTimer->IsStopped())
		{
			m_blendingTimer->Start();
		}
		else
		{
			m_blendingTimer->Stop();
		}
	}

	// control the euler angle only when euler angle is showed
	if (m_showEulerAngle)
	{
		if (g_theInput->WasKeyJustPressed('Y') || g_theInput->IsKeyDown('Y'))
		{
			m_eulerAngle.m_yawDegrees += m_quaternionChangeRate;
		}
		if (g_theInput->WasKeyJustPressed('H') || g_theInput->IsKeyDown('H'))
		{
			m_eulerAngle.m_yawDegrees -= m_quaternionChangeRate;
		}
		if (g_theInput->WasKeyJustPressed('U') || g_theInput->IsKeyDown('U'))
		{
			m_eulerAngle.m_pitchDegrees += m_quaternionChangeRate;
		}
		if (g_theInput->WasKeyJustPressed('J') || g_theInput->IsKeyDown('J'))
		{
			m_eulerAngle.m_pitchDegrees -= m_quaternionChangeRate;
		}
		if (g_theInput->WasKeyJustPressed('I') || g_theInput->IsKeyDown('I'))
		{
			m_eulerAngle.m_rollDegrees += m_quaternionChangeRate;
		}
		if (g_theInput->WasKeyJustPressed('K') || g_theInput->IsKeyDown('K'))
		{
			m_eulerAngle.m_rollDegrees -= m_quaternionChangeRate;
		}

		m_quatArray[m_quatIndexInControl] = ConvertEulerAnglesToQuat(m_eulerAngle); 
	}

	if (g_theInput->WasKeyJustPressed('L'))
	{
		m_showEulerAngle = !m_showEulerAngle;
	}

	if (g_theInput->WasKeyJustPressed('I'))
	{
		q = q.GetInversed();
	}		
	if (g_theInput->WasKeyJustPressed('G'))
	{
		q = q.GetNegated();
	}

	if (g_theInput->WasKeyJustPressed('R'))
	{
		q = Quat();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		q.x = g_rng->RollRandomFloatInRange(-1.f, 1.f);
		q.y = g_rng->RollRandomFloatInRange(-1.f, 1.f);
		q.z = g_rng->RollRandomFloatInRange(-1.f, 1.f);
		q.w = g_rng->RollRandomFloatInRange(-1.f, 1.f);
	}

	// after changing the quaternion, normalize it
	q = q.GetNormalized();
}

void QuaternionMode::InputNumKeyToSetQuat()
{
	if (!g_theDevConsole->IsOpen())
	{
		Quat& q = m_quatArray[m_quatIndexInControl];
		if (g_theInput->WasKeyJustPressed(KEYCODE_NUM1))
		{
			q = Quat(1.f, 0.f, 0.f, 0.f);
		}		
		if (g_theInput->WasKeyJustPressed(KEYCODE_NUM2))
		{
			q = Quat(0.f, 1.f, 0.f, 0.f);
		}
		if (g_theInput->WasKeyJustPressed(KEYCODE_NUM3))
		{
			q = Quat(0.f, 0.f, 1.f, 0.f);
		}		
		if (g_theInput->WasKeyJustPressed(KEYCODE_NUM4))
		{
			q = Quat(0.f, 0.f, 0.f, 1.f);
		}
		if (g_theInput->WasKeyJustPressed(KEYCODE_NUM5))
		{
			q = Quat(0.707f, 0.f, 0.f, 0.707f);
		}		
		if (g_theInput->WasKeyJustPressed(KEYCODE_NUM6))
		{
			q = Quat(0.f, 0.707f, 0.f, 0.707f);
		}		
		if (g_theInput->WasKeyJustPressed(KEYCODE_NUM7))
		{
			q = Quat(0.f, 0.f, 0.707f, 0.707f);
		}
	}
}

void QuaternionMode::AddVertsAndCreateVertexBufferForAllDebugArrows()
{
	AddVertsForArrow3D(m_quat1_Verts, Vec3::ZERO, Vec3(m_sphereRadius * 1.5f, 0.f, 0.f), m_arrowRadius * 0.5f, m_quatColorArray[0], m_quatColorArray[0], 0.8f);
	AddVertsForArrow3D(m_quat1_Verts, Vec3::ZERO, Vec3( 0.f, m_sphereRadius * 0.5f, 0.f), m_arrowRadius * 0.5f, m_quatColorArray[0], m_quatColorArray[0], 0.8f);
	AddVertsForArrow3D(m_quat1_Verts, Vec3::ZERO, Vec3( 0.f, 0.f, m_sphereRadius * 0.5f), m_arrowRadius * 0.5f, m_quatColorArray[0], m_quatColorArray[0], 0.8f);

	// create the vertex buffer for all the convex
	m_quat1_VertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_quat1_Verts.size()), sizeof(Vertex_PCU));

	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArrayDataSize = (m_quat1_Verts.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_quat1_Verts.data(), vertexArrayDataSize, m_quat1_VertexBuffer);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	AddVertsForArrow3D(m_quat2_Verts, Vec3::ZERO, Vec3(m_sphereRadius * 1.4f, 0.f, 0.f), m_arrowRadius * 0.5f, m_quatColorArray[1], m_quatColorArray[1], 0.8f);
	AddVertsForArrow3D(m_quat2_Verts, Vec3::ZERO, Vec3(0.f, m_sphereRadius * 0.5f, 0.f), m_arrowRadius * 0.5f, m_quatColorArray[1], m_quatColorArray[1], 0.8f);
	AddVertsForArrow3D(m_quat2_Verts, Vec3::ZERO, Vec3(0.f, 0.f, m_sphereRadius * 0.5f), m_arrowRadius * 0.5f, m_quatColorArray[1], m_quatColorArray[1], 0.8f);

	// create the vertex buffer for all the convex
	m_quat2_VertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_quat2_Verts.size()), sizeof(Vertex_PCU));

	vertexArrayDataSize = (m_quat2_Verts.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_quat2_Verts.data(), vertexArrayDataSize, m_quat2_VertexBuffer);	
	
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	AddVertsForArrow3D(m_quat3_Verts, Vec3::ZERO, Vec3(m_sphereRadius * 1.3f, 0.f, 0.f), m_arrowRadius * 0.75f, m_quatColorArray[2], m_quatColorArray[2], 0.8f);
	AddVertsForArrow3D(m_quat3_Verts, Vec3::ZERO, Vec3(0.f, m_sphereRadius * 0.5f, 0.f), m_arrowRadius * 0.75f, m_quatColorArray[2], m_quatColorArray[2], 0.8f);
	AddVertsForArrow3D(m_quat3_Verts, Vec3::ZERO, Vec3(0.f, 0.f, m_sphereRadius * 0.5f), m_arrowRadius * 0.75f, m_quatColorArray[2], m_quatColorArray[2], 0.8f);

	// create the vertex buffer for all the convex
	m_quat3_VertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_quat3_Verts.size()), sizeof(Vertex_PCU));

	vertexArrayDataSize = (m_quat3_Verts.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_quat3_Verts.data(), vertexArrayDataSize, m_quat3_VertexBuffer);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	AddVertsForArrow3D(m_quat4_Verts, Vec3::ZERO, Vec3(m_sphereRadius * 1.5f, 0.f, 0.f), m_arrowRadius * 0.6f, m_quatColorArray[3], m_quatColorArray[3], 0.8f);
	AddVertsForArrow3D(m_quat4_Verts, Vec3::ZERO, Vec3(0.f, m_sphereRadius * 0.6f, 0.f), m_arrowRadius * 0.6f, m_quatColorArray[3], m_quatColorArray[3], 0.8f);
	AddVertsForArrow3D(m_quat4_Verts, Vec3::ZERO, Vec3(0.f, 0.f, m_sphereRadius * 0.6f), m_arrowRadius * 0.6f, m_quatColorArray[3], m_quatColorArray[3], 0.8f);

	// create the vertex buffer for all the convex
	m_quat4_VertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_quat4_Verts.size()), sizeof(Vertex_PCU));

	vertexArrayDataSize = (m_quat4_Verts.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_quat4_Verts.data(), vertexArrayDataSize, m_quat4_VertexBuffer);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	AddVertsForArrow3D(m_eulerAngle_Verts, Vec3::ZERO, Vec3(m_sphereRadius * 1.1f, 0.f, 0.f), m_arrowRadius, m_eulerAngleColor, m_eulerAngleColor, 0.8f);
	AddVertsForArrow3D(m_eulerAngle_Verts, Vec3::ZERO, Vec3(0.f, m_sphereRadius *  0.5f, 0.f), m_arrowRadius, m_eulerAngleColor, m_eulerAngleColor, 0.8f);
	AddVertsForArrow3D(m_eulerAngle_Verts, Vec3::ZERO, Vec3(0.f, 0.f, m_sphereRadius *  0.5f), m_arrowRadius, m_eulerAngleColor, m_eulerAngleColor, 0.8f);

	// create the vertex buffer for all the convex
	m_eulerAngle_VertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_eulerAngle_Verts.size()), sizeof(Vertex_PCU));

	vertexArrayDataSize = (m_eulerAngle_Verts.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_eulerAngle_Verts.data(), vertexArrayDataSize, m_eulerAngle_VertexBuffer);
}

Mat44 QuaternionMode::GetQuat1ModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_sphereCenter);
	Mat44 orientationMat(m_quatArray[0]);
	transformMat.Append(orientationMat);
	return transformMat;
}

Mat44 QuaternionMode::GetQuat2ModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_sphereCenter);
	Mat44 orientationMat(m_quatArray[1]);
	transformMat.Append(orientationMat);
	return transformMat;
}

Mat44 QuaternionMode::GetQuat3ModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_sphereCenter);
	Mat44 orientationMat(m_quatArray[2]);
	transformMat.Append(orientationMat);
	return transformMat;
}

Mat44 QuaternionMode::GetQuat4ModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_sphereCenter);
	Mat44 orientationMat(m_quatArray[3]);
	transformMat.Append(orientationMat);
	return transformMat;
}

void QuaternionMode::RenderAllArrows() const
{
	if (!m_sphereVerts.empty())
	{
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);

		g_theRenderer->SetModelConstants(GetQuat1ModelMatrix());
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexBuffer(m_quat1_VertexBuffer, (int)m_quat1_Verts.size());

		g_theRenderer->SetModelConstants(GetQuat2ModelMatrix());
		g_theRenderer->DrawVertexBuffer(m_quat2_VertexBuffer, (int)m_quat2_Verts.size());		
		
		if (m_showTrajectory)
		{
			g_theRenderer->SetDepthMode(DepthMode::DISABLED);

			g_theRenderer->SetModelConstants(GetQuat3ModelMatrix());
			g_theRenderer->DrawVertexBuffer(m_quat3_VertexBuffer, (int)m_quat3_Verts.size());

			g_theRenderer->SetModelConstants(GetQuat4ModelMatrix());
			g_theRenderer->DrawVertexBuffer(m_quat4_VertexBuffer, (int)m_quat4_Verts.size());
		}

		if (m_showEulerAngle)
		{
			g_theRenderer->SetModelConstants(GetEulerAngleModelMatrix());
			g_theRenderer->DrawVertexBuffer(m_eulerAngle_VertexBuffer, (int)m_eulerAngle_Verts.size());
		}
	}
}

void QuaternionMode::UpdateQuatBlending()
{
	// if the blending timer is not stop
	// we use the time elapsed fraction to overwrite blend weight
	if (!m_blendingTimer->IsStopped())
	{
		if (m_blendingTimer->HasPeroidElapsed())
		{
			m_blendingTimer->Restart();
		}
		m_blendingWeight = m_blendingTimer->GetElapsedFraction();				
	}

	// m_quatArray[NUM_QUAT - 1] = Quat::Slerp(m_quatArray[0], m_quatArray[1], m_blendingWeight);
	m_quatArray[static_cast<int>(QUAT::NLERP)] = Quat::Nlerp(m_quatArray[0], m_quatArray[1], m_blendingWeight);
	m_quatArray[static_cast<int>(QUAT::SLERP)] = Quat::Slerp(m_quatArray[0], m_quatArray[1], m_blendingWeight);
}

Mat44 QuaternionMode::GetEulerAngleModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_sphereCenter);
	Mat44 orientationMat = m_eulerAngle.GetAsMatrix_XFwd_YLeft_ZUp();
	transformMat.Append(orientationMat);
	return transformMat;
}

void QuaternionMode::UpdateQuatShortestPathSpline()
{
	for (int splineIndex = 0; splineIndex < static_cast<int>(QUAT::NLERP); ++splineIndex)
	{
		if (m_quatArray[splineIndex] == Quat())
		{
			continue;
		}

		SamplePointsCurve3D& curve = m_quatTrajectory[splineIndex];
		curve.m_samplePoints.clear();
		curve.m_samplePoints.reserve(m_numTrajectorySegments);

		Vec3 axis;
		float angle;
		m_quatArray[splineIndex].GetRotationAxisAndAngle(axis, angle);

		// move the axis by the pos of the sphere
		float oneDivideNumSegments = 1.f / m_numTrajectorySegments;
		float quaterAngle = angle * oneDivideNumSegments;

		for (int i = 0; i < m_numTrajectorySegments; ++i)
		{
			float theta = quaterAngle * i;
			Quat q = Quat::CreateQuatByAngleAndAxis(theta, axis);
			Vec3 controlPt = q.RotateVector(Vec3(m_arrowLength, 0.f, 0.f));
			controlPt += m_sphereCenter;

			curve.m_samplePoints.push_back(controlPt);
		}
	}

	for (int splineIndex = static_cast<int>(QUAT::NLERP); splineIndex < static_cast<int>(QUAT::NUM_QUAT); ++splineIndex)
	{
		if (m_quatArray[splineIndex] == Quat())
		{
			continue;
		}

		SamplePointsCurve3D& curve = m_quatTrajectory[splineIndex];
		curve.m_samplePoints.clear();
		curve.m_samplePoints.reserve(m_numTrajectorySegments);

		Quat step;

		Quat start = m_quatArray[0];
		Quat end = m_quatArray[1];
		
		// move the axis by the pos of the sphere
		float oneDivideNumSegments = 1.f / m_numTrajectorySegments;

		for (int i = 0; i < m_numTrajectorySegments; ++i)
		{
			Quat q;
			Vec3 controlPt;
			if (splineIndex == static_cast<int>(QUAT::NLERP))
			{
				q = Quat::Nlerp(start, end, oneDivideNumSegments * i);
				controlPt = q.RotateVector(Vec3(m_sphereRadius * 1.3f, 0.f, 0.f));
			}
			else if (splineIndex == static_cast<int>(QUAT::SLERP))
			{
				q = Quat::Slerp(start, end, oneDivideNumSegments * i);
				controlPt = q.RotateVector(Vec3(m_sphereRadius * 1.5f, 0.f, 0.f));
			}
			controlPt += m_sphereCenter;
			
			curve.m_samplePoints.push_back(controlPt);
		}
	}
}

void QuaternionMode::AddingVertsForMovingPathSpline()
{
	if (m_splineFlowingTimer)
	{
		if (m_splineFlowingTimer->HasPeroidElapsed())
		{
			++m_drawingSectionIndex;
			if (m_drawingSectionIndex == m_drawingSectionNum)
			{
				m_drawingSectionIndex = 0;
			}
			m_splineFlowingTimer->Restart();
		}

		m_pathSplineVertexs.clear();
		for (int splineIndex = 0; splineIndex < static_cast<int>(QUAT::NUM_QUAT); ++splineIndex)
		{
			// if the quat is default value, no need to add the shortest path
			if (m_quatArray[splineIndex] == Quat())
			{
				continue;
			}
			else
			{
				if ((splineIndex == static_cast<int>(QUAT::NLERP) || splineIndex == static_cast<int>(QUAT::SLERP)) && m_showTrajectory)
				{
					AddVertsForSamplePointsCurve(m_pathSplineVertexs, m_quatTrajectory[splineIndex], m_drawingSplineRadius, m_quatColorArray[splineIndex], true, m_drawingSectionNum, m_drawingSectionIndex);
				}
				if(!m_showTrajectory && splineIndex != static_cast<int>(QUAT::NLERP) && splineIndex != static_cast<int>(QUAT::SLERP))
				{
					AddVertsForSamplePointsCurve(m_pathSplineVertexs, m_quatTrajectory[splineIndex], m_drawingSplineRadius, m_quatColorArray[splineIndex], false, m_drawingSectionNum, m_drawingSectionIndex);

					Vec3 axis;
					float angle;
					m_quatArray[splineIndex].GetRotationAxisAndAngle(axis, angle);
					AddVertsForCylinder3D(m_pathSplineVertexs, m_sphereCenter + axis * m_sphereRadius * 2.f, m_sphereCenter - axis * m_sphereRadius * 2.f, m_quatAxisRadius, m_quatColorArray[splineIndex]);
				}
			}
		}
	}
}

void QuaternionMode::ControlTrajectory()
{
	if (g_theInput->WasKeyJustPressed('T'))
	{
		m_showTrajectory = !m_showTrajectory;
	}
}

void QuaternionMode::RenderMovingPath() const
{
	if (!m_pathSplineVertexs.empty())
	{
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		g_theRenderer->SetModelConstants(Mat44());
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray((int)m_pathSplineVertexs.size(), m_pathSplineVertexs.data());
	}

}

void QuaternionMode::UpdateModeInfo()
{
	m_modeName = "Mode (F6 / F7 for prev / next): Quaternion";
	m_controlInstruction = "WASD & QE = fly, R-Reset, I-Inverse, G-Negate, NUM Keys-Set Quat \nT-Turn on/off Nlerp & Slerp";
	if (!m_showEulerAngle)
	{
		m_testString = "Space - Pause / Continue Blending\nL - Show Euler Angle Conversion";
	}
	else
	{
		m_testString = "Space-Pause/Continue Blending\nL-Close Euler Angle Conversion\nY/H-Yaw, U/J-Pitch, I/K-Roll";
	}
}

void QuaternionMode::UpdateDebugMessages()
{
	Vec2  textPos = Vec2(1.f, 0.f);
	Vec2  spacing = Vec2(0.f, 0.04f);

	Vec2  quat_1_Alignment = Vec2(0.99f, 0.98f);
	Vec2  quat_2_Alignment = quat_1_Alignment - spacing;
	Vec2  blended_quat_Alignment = quat_2_Alignment - spacing;
	Vec2  quatControlIndex_Alignment = blended_quat_Alignment - spacing * 2.f;
	Vec2  TValue_Alignment = quatControlIndex_Alignment - spacing;

	Vec2  duration_Alignment = TValue_Alignment - spacing;
	Vec2  renderDiffuseAlignment = duration_Alignment - spacing;
	Vec2  renderSpecularAlignment = renderDiffuseAlignment - spacing;
	Vec2  renderEmissiveAlignment = renderSpecularAlignment - spacing;

	Vec2  useDiffuseMapAlignment = renderEmissiveAlignment - spacing;
	Vec2  useNormalMapAlignment = useDiffuseMapAlignment - spacing;
	Vec2  useSpecularMapAlignment = useNormalMapAlignment - spacing;
	Vec2  useGlossinessMapAlignment = useSpecularMapAlignment - spacing;
	Vec2  eulerAngle_alignment = Vec2(0.5f, 0.03f);

	float fontSize = 24.f;

	std::string quat_1_string = Stringf("Quat_0: (X: %.2f, Y: %.2f, Z: %.2f, W: %.2f)", m_quatArray[0].x, m_quatArray[0].y, m_quatArray[0].z, m_quatArray[0].w);
	DebugAddScreenText(quat_1_string, Vec2(), fontSize, quat_1_Alignment, -1.f);

	// EulerAngles angle_1 = ConvertQuatToEulerAngles(m_quatArray[0]);
	// 
	// std::string quat_1_Euler = Stringf("EulerAngle_1: ( %.1f, %.1f, %.1f )", angle_1.m_yawDegrees, angle_1.m_pitchDegrees, angle_1.m_rollDegrees);
	// DebugAddScreenText(quat_1_Euler, Vec2(), fontSize, eulerAngle_1_Alignment, -1.f);

	std::string quat_2_str = Stringf("Quat_1: (X: %.2f, Y: %.2f, Z: %.2f, W: %.2f)", m_quatArray[1].x, m_quatArray[1].y, m_quatArray[1].z, m_quatArray[1].w);
	DebugAddScreenText(quat_2_str, Vec2(), fontSize, quat_2_Alignment, -1.f);	
	

	std::string quat_Blended_str = Stringf("Quat_Blended: (X: %.2f, Y: %.2f, Z: %.2f, W: %.2f)", m_quatArray[3].x, m_quatArray[3].y, m_quatArray[3].z, m_quatArray[3].w);
	DebugAddScreenText(quat_Blended_str, Vec2(), fontSize, blended_quat_Alignment, -1.f);

	// control quat index
	std::string quatIndex = Stringf("Quat Index in control (N / M): %i", m_quatIndexInControl);
	DebugAddScreenText(quatIndex, Vec2(), fontSize, quatControlIndex_Alignment, -1.f);
	
	// blending value
	std::string tValue = Stringf("Weight Value ([ / ]): %.2f", m_blendingWeight);
	DebugAddScreenText(tValue, Vec2(), fontSize, TValue_Alignment, -1.f);
	
	// blending value
	std::string blendingDuration = Stringf("Blending Duration (< / >): %.2f", m_blendingTimer->m_period);
	DebugAddScreenText(blendingDuration, Vec2(), fontSize, duration_Alignment, -1.f);

	if (m_showEulerAngle)
	{
		std::string eulerAngle_str = Stringf("EulerAngle: ( %.1f, %.1f, %.1f)", m_eulerAngle.m_yawDegrees, m_eulerAngle.m_pitchDegrees, m_eulerAngle.m_rollDegrees);
		DebugAddScreenText(eulerAngle_str, Vec2(), fontSize, eulerAngle_alignment, -1.f);
	}

	// 
	// // render ambient 
	// std::string renderAmbient = Stringf("Render Ambient [1]: %s", m_phongLighinting->lighingDebug.RenderAmbient ? "On" : "Off");
	// DebugAddScreenText(renderAmbient, Vec2(), fontSize, renderAmbientAlignment, -1.f);
	// 
	// // render Diffuse 
	// std::string renderDiffuse = Stringf("Render Diffuse [2]: %s", m_phongLighinting->lighingDebug.RenderDiffuse ? "On" : "Off");
	// DebugAddScreenText(renderDiffuse, Vec2(), fontSize, renderDiffuseAlignment, -1.f);
	// 
	// // render Specular
	// std::string renderSpecular = Stringf("Render Specular [3]: %s", m_phongLighinting->lighingDebug.RenderSpecular ? "On" : "Off");
	// DebugAddScreenText(renderSpecular, Vec2(), fontSize, renderSpecularAlignment, -1.f);
	// 
	// // render Emissive
	// std::string renderEmissive = Stringf("Render Emissive [4]: %s", m_phongLighinting->lighingDebug.RenderEmissive ? "On" : "Off");
	// DebugAddScreenText(renderEmissive, Vec2(), fontSize, renderEmissiveAlignment, -1.f);
	// 
	// // Diffuse Map
	// std::string diffuseMap = Stringf("Diffuse Map [5]: %s", m_phongLighinting->lighingDebug.UseDiffuseMap ? "On" : "Off");
	// DebugAddScreenText(diffuseMap, Vec2(), fontSize, useDiffuseMapAlignment, -1.f);
	// 
	// // normal Map
	// std::string normalMap = Stringf("Normal Map [6]: %s", m_phongLighinting->lighingDebug.UseNormalMap ? "On" : "Off");
	// DebugAddScreenText(normalMap, Vec2(), fontSize, useNormalMapAlignment, -1.f);
	// 
	// // Specular Map
	// std::string specularMap = Stringf("Specular Map [7]: %s", m_phongLighinting->lighingDebug.UseSpecularMap ? "On" : "Off");
	// DebugAddScreenText(specularMap, Vec2(), fontSize, useSpecularMapAlignment, -1.f);
	// 
	// // Glossiness Map
	// std::string glossinessMap = Stringf("Glossiness Map [8]: %s", m_phongLighinting->lighingDebug.UseGlossinessMap ? "On" : "Off");
	// DebugAddScreenText(glossinessMap, Vec2(), fontSize, useGlossinessMapAlignment, -1.f);
	// 
	// // Emissive Map
	// std::string emissiveMap = Stringf("Emissive Map [9]: %s", m_phongLighinting->lighingDebug.UseEmissiveMap ? "On" : "Off");
	// DebugAddScreenText(emissiveMap, Vec2(), fontSize, useEmissiveMapAlignment, -1.f);
}

void QuaternionMode::RenderWorldInPlayerCamera() const
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

	RenderSphere();
	RenderAllArrows();
	RenderMovingPath();

	// debug render
	g_theRenderer->EndCamera(playerCamera);

	DebugRenderWorld(playerCamera);
}

void QuaternionMode::ScribeQuatModeCommands()
{
	SubscribeEventCallbackFunction("ChangeQuat", Command_SetQuatIndex);
	SubscribeEventCallbackFunction("SetQuat", Command_SetQuatValue);
}

bool QuaternionMode::Command_SetQuatIndex(EventArgs& args)
{
	// the input is like SetQuat index = 1;
	int quatIndex = args.GetValue("index", 0);

	// if user input SetQuat = 1
	if (quatIndex >=0 && quatIndex <= 3)
	{
		// we just change the quat index being controlled
		QuaternionMode* mode = static_cast<QuaternionMode*>(g_theApp->m_currentGameMode);
		mode->m_quatIndexInControl = quatIndex;
	}
	return true;
}

bool QuaternionMode::Command_SetQuatValue(EventArgs& args)
{
	// the input is like SetQuat = 0.7007, 0, 0, 0.7;
	string quatStr = args.GetValue("value", "no info found");
	if (quatStr == "no info found")
	{
		return true;
	}

	// cut the string to get quat info
	Strings quatInfo = SplitStringOnDelimiter(quatStr, ',', true);

	// set all four values
	int numInfo = (int)quatInfo.size();
	if (numInfo == 4)
	{
		QuaternionMode* mode = static_cast<QuaternionMode*>(g_theApp->m_currentGameMode);
		mode->m_quatArray[mode->m_quatIndexInControl].x = stof(quatInfo[0]);
		mode->m_quatArray[mode->m_quatIndexInControl].y = stof(quatInfo[1]);
		mode->m_quatArray[mode->m_quatIndexInControl].z = stof(quatInfo[2]);
		mode->m_quatArray[mode->m_quatIndexInControl].w = stof(quatInfo[3]);

		mode->m_quatArray[mode->m_quatIndexInControl].GetNormalized();
	}
	return true;
}

void QuaternionMode::RenderDebugRenderSystem() const
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
