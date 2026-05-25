#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Game/Pachinko2D.hpp"
#include "Game/GameCommon.hpp"
#include "Game/ReferencePoint.hpp"
#include "Game/Entity.hpp"
#include "Game/UI.hpp"
#include "Game/App.hpp"

Rgba8 const Pachinko2D::s_hardElasticityColor = Rgba8(144, 220, 198, 255);
Rgba8 const Pachinko2D::s_softElasticityColor = Rgba8(255, 160, 132, 200);
Rgba8 const Pachinko2D::s_deepBallColor = Rgba8(58, 122, 153, 200);
Rgba8 const Pachinko2D::s_lightBallColor = Rgba8(164, 221, 240, 200);
Rgba8 const Pachinko2D::s_backgroundColor = Rgba8(42, 57, 80, 255);

extern App* g_theApp;
extern InputSystem* g_theInput; 
// extern AudioSystem* g_theAudio;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;
extern Window* g_theWindow;

extern Clock* g_theGameClock;

Pachinko2D::Pachinko2D()
	:GameMode()
{
}

Pachinko2D::~Pachinko2D()
{

}

PachinkoBall::PachinkoBall(Vec2 pos, float radius, Vec2 vel, Rgba8 color, float elasticity)
	: m_discCenter(pos)
	, m_discRadius(radius)
	, m_velocity(vel)
	, m_color(color)
	, m_elasticity(elasticity)
{

}

bool PachinkoBall::VelocityIsConvergentToFixedPoint(Vec2 fixedPos)
{
	Vec2 fixedPointToBallCenter = (m_discCenter - fixedPos).GetNormalized();
	if (DotProduct2D(fixedPointToBallCenter, m_velocity) < 0.f)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void PachinkoBall::AddVertsForBalls(std::vector<Vertex_PCU>& verts)
{
	AddVertesForDisc2D(verts, m_discCenter, m_discRadius, m_color, 18);
}

PachinkoBumper::PachinkoBumper(float elasticity)
	: m_elasticity(elasticity)
{

}

Rgba8 PachinkoBumper::GetBumperColorForElasticity()
{
	Rgba8 bumperColor = InterpolateRGBA(Pachinko2D::s_softElasticityColor, Pachinko2D::s_hardElasticityColor, m_elasticity);
	return bumperColor;
}

OBBPachinkoBumper::OBBPachinkoBumper(OBB2* shape, float elasticity)
	: PachinkoBumper(elasticity)
	, m_OBB2(*shape)
{

}

void OBBPachinkoBumper::AddVerts(std::vector<Vertex_PCU>& verts)
{
	AddVertsForOBB2D(verts, m_OBB2, GetBumperColorForElasticity());
}

void OBBPachinkoBumper::BounceBallOffOfBumper(PachinkoBall& ball)
{
	float elasticity = m_elasticity * ball.m_elasticity;
	Vec2 bouncePos = GetNearestPointOnOBB2D(ball.m_discCenter, m_OBB2);

	if (!IsPointInsideDisc2D(bouncePos, ball.m_discCenter, ball.m_discRadius))
	{
		return;
	}
	
	// only reflect the speed when the ball velocity is going into the bumper
	if (ball.VelocityIsConvergentToFixedPoint(bouncePos))
	{
		// get two sub - velocity
		Vec2 impactNormal = (ball.m_discCenter - bouncePos).GetNormalized();
		Vec2 velAlongNormal = GetProjectedOnto2D(ball.m_velocity, impactNormal);
		Vec2 velAlongTangent = ball.m_velocity - velAlongNormal;

		// revert and get new velocity
		velAlongNormal *= (-1.f * elasticity);
		Vec2 newVelocity = velAlongNormal + velAlongTangent;
		ball.m_velocity = newVelocity;
	}

	// push disc out of point
	PushDiscOutOfFixedPoint2D(ball.m_discCenter, ball.m_discRadius, bouncePos);
}

DiscPachinkoBumper::DiscPachinkoBumper(Vec2 center, float radius, float elasticity)
	: PachinkoBumper(elasticity)
	, m_center(center)
	, m_radius(radius)
{

}

void DiscPachinkoBumper::AddVerts(std::vector<Vertex_PCU>& verts)
{
	AddVertesForDisc2D(verts, m_center, m_radius, GetBumperColorForElasticity(), 24);
}

void DiscPachinkoBumper::BounceBallOffOfBumper(PachinkoBall& ball)
{
	float elasticity = m_elasticity * ball.m_elasticity;

	// no overlap, return
	if (!DoDiscsOverlap(ball.m_discCenter, ball.m_discRadius, m_center, m_radius))
	{
		return;
	}

	// correct(reverse and dampen) velocity into the bumper
	Vec2 fixedCenterToBallCenter = (ball.m_discCenter - m_center).GetNormalized();
	if (DotProduct2D(fixedCenterToBallCenter, ball.m_velocity) < 0.f)
	{
		Vec2 velAlongNormal = GetProjectedOnto2D(ball.m_velocity, fixedCenterToBallCenter);
		Vec2 velAlongTangent = ball.m_velocity - velAlongNormal;
		velAlongNormal *= (-1.f * elasticity);
		Vec2 newVelocity = velAlongTangent + velAlongNormal;
		ball.m_velocity = newVelocity;
	}

	PushDiscOutOfFixedDisc2D(ball.m_discCenter, ball.m_discRadius, m_center, m_radius);
}

CapsulePachinkoBumper::CapsulePachinkoBumper(Capsule2* capsule, float elasticity)
	: PachinkoBumper(elasticity)
	, m_capsule(*capsule)
{

}

void CapsulePachinkoBumper::AddVerts(std::vector<Vertex_PCU>& verts)
{
	AddVertsForCapsule2D(verts, m_capsule, GetBumperColorForElasticity());
}

void CapsulePachinkoBumper::BounceBallOffOfBumper(PachinkoBall& ball)
{
	float elasticity = m_elasticity * ball.m_elasticity;
	Vec2 bouncePos = GetNearestPointOnCapsule2D(ball.m_discCenter, m_capsule);

	if (!IsPointInsideDisc2D(bouncePos, ball.m_discCenter, ball.m_discRadius))
	{
		return;
	}

	// only reflect the speed when the ball velocity is going into the bumper
	if (ball.VelocityIsConvergentToFixedPoint(bouncePos))
	{
		// get two sub - velocity
		Vec2 impactNormal = (ball.m_discCenter - bouncePos).GetNormalized();
		Vec2 velAlongNormal = GetProjectedOnto2D(ball.m_velocity, impactNormal);
		Vec2 velAlongTangent = ball.m_velocity - velAlongNormal;

		// revert and get new velocity
		velAlongNormal *= (-1.f * elasticity);
		Vec2 newVelocity = velAlongNormal + velAlongTangent;
		ball.m_velocity = newVelocity;
	}

	// push disc out of point
	PushDiscOutOfFixedPoint2D(ball.m_discCenter, ball.m_discRadius, bouncePos);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// Game mode code
void Pachinko2D::Startup()
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


void Pachinko2D::Update(float deltaSeconds)
{
	UpdateFixedTimeStepControl();

	// control fixed time step
	if (m_isUsingFixedPhysicsTimestep)
	{
		m_physicsTimeOwed += deltaSeconds;
		while (m_physicsTimeOwed >= m_physicsFixedTimestep)
		{
			m_physicsTimeOwed -= m_physicsFixedTimestep;
			UpdatePhysics(m_physicsFixedTimestep);
		}
	}
	else
	{
		UpdatePhysics(deltaSeconds);
	}

	// reset list
	m_ballVerts.clear();
	m_ballVerts.reserve(100000);

	// update ray cast
	ControlTheReferenceRay(deltaSeconds);
	SpawnNewBallAtRayByInput();

	AddVertsForBalls();
	AddVertsForGeneratingRay();

	UpdateModeInfo();
}

void Pachinko2D::UpdateFixedTimeStepControl()
{
	// control time step
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTBRACKET))
	{
		m_physicsFixedTimestep *= 0.9f;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTBRACKET))
	{
		m_physicsFixedTimestep *= 1.1f;
	}

	if (g_theInput->WasKeyJustPressed('F'))
	{
		if (m_isUsingFixedPhysicsTimestep)
		{
			m_isUsingFixedPhysicsTimestep = false;
		}
		else
		{
			m_isUsingFixedPhysicsTimestep = true;
		}
	}
}

void Pachinko2D::Render() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);

	g_theRenderer->ClearScreen(Pachinko2D::s_backgroundColor);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	RenderAllBumpers();
	RenderBallsAndRay();

	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderUIElements();
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void Pachinko2D::RenderAllBumpers() const
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindShader(nullptr);

	g_theRenderer->DrawVertexArray((int)m_bumperVerts.size(), m_bumperVerts.data());
}

void Pachinko2D::RenderBallsAndRay() const
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindShader(nullptr);

	g_theRenderer->DrawVertexArray((int)m_ballVerts.size(), m_ballVerts.data());
}

void Pachinko2D::Shutdown()
{

}

/// <UI Settings>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Vec2 Pachinko2D::GetRandomPosInWorld(Vec2 worldSize)
{
	float posX = g_rng->RollRandomFloatInRange(0, worldSize.x);
	float posY = g_rng->RollRandomFloatInRange(0, worldSize.y);
	return Vec2(posX, posY);
}

void Pachinko2D::CreateRandomShapes()
{
	m_bumpers.clear();
	m_balls.clear();

	// disc bumper
	for (int i = 0; i < m_numEachTypeBumper; ++i)
	{
		Vec2 discCenter;
		discCenter.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * .1f, WORLD_SIZE_X * .9f);
		discCenter.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .1f, WORLD_SIZE_Y * .9f);

		float discRadius = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .03f, WORLD_SIZE_Y * .15f);
		float elasticity = g_rng->RollRandomFloatInRange(0.f, 1.f);
		while (elasticity == 0.f || elasticity == 1.f)
		{
			elasticity = g_rng->RollRandomFloatInRange(0.f, 1.f);
		}

		PachinkoBumper* bumper = new DiscPachinkoBumper(discCenter, discRadius, elasticity);
		m_bumpers.push_back(bumper);
	}

	// capsule bumper
	for (int i = 0; i < m_numEachTypeBumper; ++i)
	{
		// random select one point on the world screen
		Vec2 start;
		start.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.1f, WORLD_SIZE_X * 0.9f);
		start.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.1f, WORLD_SIZE_Y * 0.9f);
		// random select one point on the world screen
		Vec2 end;
		end.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.1f, WORLD_SIZE_X * 0.9f);
		end.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.1f, WORLD_SIZE_Y * 0.9f);

		float length = (end - start).GetLength();
		while (length > m_maxCapsuleBumperLength || length < m_minCapsuleBumperLength)
		{
			start.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.1f, WORLD_SIZE_X * 0.9f);
			start.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.1f, WORLD_SIZE_Y * 0.9f);

			end.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.1f, WORLD_SIZE_X * 0.9f);
			end.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.1f, WORLD_SIZE_Y * 0.9f);

			length = (end - start).GetLength();
		}

		float radius = g_rng->RollRandomFloatInRange(m_minCircleBumperRadius, m_maxCircleBumperRadius);

		float elasticity = g_rng->RollRandomFloatInRange(0.f, 1.f);
		while (elasticity == 0.f || elasticity == 1.f)
		{
			elasticity = g_rng->RollRandomFloatInRange(0.f, 1.f);
		}

		Capsule2* capsule = new Capsule2(start, end, radius);
		PachinkoBumper* bumper = new CapsulePachinkoBumper(capsule, elasticity);
		m_bumpers.push_back(bumper);
	}
	
	// OBB2 bumper
	for (int i = 0; i < m_numEachTypeBumper; ++i)
	{
		Vec2 worldCenter;
		worldCenter.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * .1f, WORLD_SIZE_X * .9f);
		worldCenter.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .1f, WORLD_SIZE_Y * .9f);
		Vec2 iBasisNormal;
		iBasisNormal.x = g_rng->RollRandomFloatInRange(-1.f, 1.f);
		iBasisNormal.y = sqrtf(1.f - (iBasisNormal.x * iBasisNormal.x));
	
		Vec2 halfDimensions;
		halfDimensions.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .03f, WORLD_SIZE_Y * .09f);
		halfDimensions.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * .03f, WORLD_SIZE_Y * .09f);
	
		float elasticity = g_rng->RollRandomFloatInRange(0.f, 1.f);
		while (elasticity == 0.f || elasticity == 1.f)
		{
			elasticity = g_rng->RollRandomFloatInRange(0.f, 1.f);
		}
	
		OBB2* OBB2Shape = new OBB2(worldCenter, iBasisNormal, halfDimensions);
		PachinkoBumper* bumper = new OBBPachinkoBumper(OBB2Shape, elasticity);
		m_bumpers.push_back(bumper);
	}

	AddVertsForBumpers();
}

void Pachinko2D::AddVertsForBumpers()
{
	m_bumperVerts.clear();

	for (int i = 0; i < (int)m_bumpers.size(); ++i)
	{
		m_bumpers[i]->AddVerts(m_bumperVerts);
	}
}

void Pachinko2D::SpawnNewBallAtRayByInput()
{
	if (g_theInput->WasKeyJustPressed(' ') || g_theInput->IsKeyDown('N'))
	{
		// get random radius
		FloatRange radiusRange(m_minBallRadius, m_maxBallRadius);
		float radius = g_rng->RollRandomFloatInFloatRange(radiusRange);

		Vec2 velocity = (m_tipPos - m_tailPos) * m_initialSpeedMultiplier;

		// spawn the ball and give it velocity
		float chance = g_rng->RollRandomFloatInRange(0.f, 1.f);
		Rgba8 ballColor = InterpolateRGBA(Pachinko2D::s_lightBallColor, Pachinko2D::s_deepBallColor, chance);
		PachinkoBall* ball = new PachinkoBall(m_tailPos, radius, velocity, ballColor, 0.9f); // task: todo 4.21 every ball is 0.9 elasticity?
		m_balls.push_back(ball);
	}
}

void Pachinko2D::UpdateModeInfo()
{
	std::string buttomWrap;
	if (g_theInput->WasKeyJustPressed('B'))
	{
		if (m_buttomWrap)
		{
			m_buttomWrap = false;
		}
		else // false
		{
			m_buttomWrap = true;
		}
	}

	if (m_buttomWrap)
	{
		buttomWrap = "On";
	}
	else // false
	{
		buttomWrap = "Off";
	}

	float frameTimeMs = 1000.f * m_physicsFixedTimestep;
	float deltaSeconds = g_theGameClock->GetDeltaSeconds() * 1000.f;

	m_modeName = "Mode (F6 / F7 for prev / next): Pachinko Machine (2D)";
	if (m_isUsingFixedPhysicsTimestep)
	{
		m_controlInstruction = Stringf("F8 to reset; LMB/RMB/ESDF/IJKL move, T= slow, space/N= ball(%i), B= ButtomWarp %s, timeStep= %.02fms (F [,]) dt= %.02fms", (int)m_balls.size(), buttomWrap.c_str(), frameTimeMs, deltaSeconds);
	}
	else
	{
		m_controlInstruction = Stringf("F8 to reset; LMB/RMB/ESDF/IJKL move, T= slow, space/N= ball(%i), B= Buttom warp %s, variable timeStep (F), dt= %.02fms", (int)m_balls.size(), buttomWrap.c_str(), frameTimeMs, deltaSeconds);
	}
}

void Pachinko2D::AddVertsForBalls()
{
	for (int i = 0; i < (int)m_balls.size(); ++i)
	{
		m_balls[i]->AddVertsForBalls(m_ballVerts);
	}
}

void Pachinko2D::AddVertsForGeneratingRay()
{
	// arrow
	AddVertsForArrow2D(m_ballVerts, m_tailPos, m_tipPos, m_arrowSize, m_arrowLineThickness, Rgba8::GREEN);

	// ring
	AddVertesForRing2D(m_ballVerts, m_tailPos, m_minBallRadius, m_spawnRingThickness, Rgba8::BLUE_MVTHL, 36);
	AddVertesForRing2D(m_ballVerts, m_tailPos, m_maxBallRadius, m_spawnRingThickness, Rgba8::BLUE_MVTHL, 36);
}

// add camera shake when player is dead
void Pachinko2D::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}


void Pachinko2D::RenderUIElements() const
{
	if (g_theApp->m_isPaused)
	{
		std::vector<Vertex_PCU> verts;
		verts.reserve(6);
		AABB2 testingTexture = AABB2(0.f, 0.f, 1600.f, 800.f);
		AddVertsForAABB2D(verts, testingTexture, Rgba8(0, 0, 0, 100));
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
	}
}

bool Pachinko2D::CheckUIEnabled(UI* const UI) const
{
	UNUSED(UI);
	return false;
}

void Pachinko2D::UpdatePhysics(float timeStep)
{
	int numBalls = (int)m_balls.size();
	int numBumpers = (int)m_bumpers.size();

	for (int i = 0; i < numBalls; ++i)
	{
		MoveBallByGravityAndVelocity(*m_balls[i], timeStep);
	}


	for (int i = 0; i < numBalls; ++i)
	{
		for (int ballIndex = (i + 1); ballIndex < numBalls; ++ballIndex)
		{
			CheckBallVsBall(*m_balls[i], *m_balls[ballIndex]);
		}
	}

	for (int i = 0; i < numBalls; ++i)
	{
		for (int j = 0; j < numBumpers; ++j)
		{
			CheckBallVsBumper(*m_balls[i], *m_bumpers[j]);
		}
	}

	for (int i = 0; i < numBalls; ++i)
	{
		CheckBallVsWalls(*m_balls[i]);
	}
}

void Pachinko2D::MoveBallByGravityAndVelocity(PachinkoBall& ball, float deltaSeconds)
{
	ball.m_velocity.y -= m_gravityScale * deltaSeconds;
	ball.m_discCenter += ball.m_velocity * deltaSeconds;
}

void Pachinko2D::CheckBallVsBall(PachinkoBall& a, PachinkoBall& b)
{
	// if two discs not overlap, skip
	if (!DoDiscsOverlap(a.m_discCenter, a.m_discRadius, b.m_discCenter, b.m_discRadius))
	{
		return;
	}

	float elasticity = a.m_elasticity * b.m_elasticity;
	PushDiscOutOfEachOther2D(a.m_discCenter, a.m_discRadius, b.m_discCenter, b.m_discRadius);

	// check if two ball are converge or divide
	Vec2 normal = (b.m_discCenter - a.m_discCenter).GetNormalized();
	float BSpeed = DotProduct2D(b.m_velocity, normal);
	float ASpeed = DotProduct2D(a.m_velocity, normal);
	if ((BSpeed - ASpeed) < 0.f) // converge
	{
		Vec2 Bn = GetProjectedOnto2D(b.m_velocity, normal);
		Vec2 An = GetProjectedOnto2D(a.m_velocity, normal);
		Vec2 Bt = b.m_velocity - Bn;
		Vec2 At = a.m_velocity - An;

		Vec2 BnExchanged = Bn * elasticity;
		Vec2 AnExchanged = An * elasticity;

		Vec2 AVel = BnExchanged + At;
		Vec2 BVel = AnExchanged + Bt;

		a.m_velocity = AVel;
		b.m_velocity = BVel;
	}
	else // divide
	{
		return;
	}
}

void Pachinko2D::CheckBallVsWalls(PachinkoBall& ball)
{
  	float elasticity = ball.m_elasticity * m_wallElasticity;

	// bottom wall
	if (m_buttomWrap)
	{
		if (ball.m_discCenter.y < ball.m_discRadius)
		{
			Vec2 bouncePos = Vec2(ball.m_discCenter.x, 0.f);
			// only reflect the speed when the ball velocity is going into the wall
			if (ball.VelocityIsConvergentToFixedPoint(bouncePos))
			{
				ball.m_velocity.y *= -1.f * elasticity;
			}

			ball.m_discCenter.y = ball.m_discRadius;
		}
	}
	else 
	{
		// teleport 10% of total screen-height higher than the top of the screen 
		// plus the ball’ s radius 
		if (ball.m_discCenter.y < (ball.m_discRadius * (-1.05f)))
		{
			ball.m_discCenter.y = WORLD_SIZE_Y * 1.1f + ball.m_discRadius;
		}
	}

	// right wall
	if (ball.m_discCenter.x + ball.m_discRadius > WORLD_SIZE_X)
	{
		Vec2 bouncePos = Vec2(WORLD_SIZE_X, ball.m_discCenter.y);
		// only reflect the speed when the ball velocity is going into the wall
		if (ball.VelocityIsConvergentToFixedPoint(bouncePos))
		{
			ball.m_velocity.x *= -1.f * elasticity;
		}

		ball.m_discCenter.x = WORLD_SIZE_X - ball.m_discRadius;
	}

	// left wall
	if (ball.m_discCenter.x - ball.m_discRadius < 0.f)
	{
		Vec2 bouncePos = Vec2(0.f, ball.m_discCenter.y);
		// only reflect the speed when the ball velocity is going into the wall
		if (ball.VelocityIsConvergentToFixedPoint(bouncePos))
		{
			ball.m_velocity.x *= -1.f * elasticity;
		}

		ball.m_discCenter.x = ball.m_discRadius;
	}
}

void Pachinko2D::CheckBallVsBumper(PachinkoBall& ball, PachinkoBumper& bumper)
{
	bumper.BounceBallOffOfBumper(ball);
}
