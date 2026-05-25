#include "Game/PlayerShip.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Game/EnergyBar.hpp"
#include "Game/EnergySelectionRing.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern RandomNumberGenerator* g_rng;
extern Clock* g_theGameClock;

// the children's constructor function need to initialize its parent class constructor first
PlayerShip::PlayerShip(Game* owner, Vec2 const& startPos)
	:Entity(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_physicsRadius = PLAYERSHIP_PHYSICS_RADIUS;
	m_cosmeticRadius = PLAYERSHIP_COSMETIC_RADIUS;
	InitializeLocalVerts();
	InitializeFlameVerts();
	m_health = 1;
	m_fireRateInterval = PLAYERSHIP_FIRE_RATE_LONG;
}

PlayerShip::~PlayerShip()
{
}

void PlayerShip::Update(float deltaSeconds)
{
	m_thrustFraction = 0.f;
	m_angularVelocity = 0.f;	// angular velocity will reset to 0 when key released
	float acceleration = 0.f;	// ship will keep linear velocity even when key released 
	m_game->m_energyBar->m_energyConsuming = 0.f;
	// controller controlling
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	XboxController const& playerController = g_theInput->GetController(0);

	// Respawn
	if (m_isDead)
	{
		if (playerController.WasButtonJustPressed(XboxButtonID::XBOX_BUTTON_START))
		{
			m_game->RespawnPlayer();
		}
	}

	// Pause
	if (playerController.WasButtonJustPressed(XboxButtonID::XBOX_BUTTON_BACK))
	{
		g_theInput->HandleKeyPressed('P');
	}

	// Drive
	float leftStickMagnitude = playerController.GetLeftstick().GetMagnitude();
	if (leftStickMagnitude > 0.f)
	{
		m_thrustFraction = leftStickMagnitude;
		if (m_speedBooster)
		{
			acceleration = PLAYERSHIP_ACCERATION_HIGH * m_thrustFraction * deltaSeconds;
			m_game->m_energyBar->m_energyConsuming = ENERGY_CONSUMING_SPEEDBURST_RATE;
		}
		else
		{
			acceleration = PLAYERSHIP_ACCERATION_LOW * m_thrustFraction * deltaSeconds;
		}

		// only orient if the
		m_orientationDegrees = playerController.GetLeftstick().GetOrientationDegrees();
	}

	// shoot
	if (m_fireRateBooster)
	{
		m_fireRateInterval = PLAYERSHIP_FIRE_RATE_SHORT;
	}
	if (!m_fireRateBooster)
	{
		m_fireRateInterval = PLAYERSHIP_FIRE_RATE_LONG;
	}
	m_fireTimer += deltaSeconds;
	if (!m_isDead)
	{
		if (playerController.IsButtonDown(XboxButtonID::XBOX_BUTTON_A) || g_theInput->IsKeyDown(' '))
		{
			// if the fire booster if on and there is no
			if (m_game->m_energyBar->m_energyRemain > 0.f && m_fireRateBooster)
			{
				m_game->m_energyBar->m_energyConsuming = ENERGY_CONSUMING_FIRERATE_RATE;
			}
			if (m_fireTimer >= m_fireRateInterval)
			{
				Vec2 bulletRelativePos = Vec2(1, 0); // bullet's relative position to the ship
				Vec2 bulletPos = bulletRelativePos + m_position;
				m_game->SpawnBullet(bulletPos, m_orientationDegrees);
				m_fireTimer = 0.f;
			}
		}
	}

	// slow mode
	if (playerController.IsButtonDown(XboxButtonID::XBOX_BUTTON_RSHOULDER) || g_theInput->IsKeyDown('M'))
	{
		g_theGameClock->SetTimeScale(0.1f);
	}
	if (playerController.WasButtonJustReleased(XboxButtonID::XBOX_BUTTON_RSHOULDER) || g_theInput->IsKeyDown('M'))
	{
		g_theGameClock->SetTimeScale(1.f);
	}

	// energy selection has slow motion effect
	// could use when player is not dead
	if (!m_isDead)
	{
		if (playerController.IsButtonDown(XboxButtonID::XBOX_BUTTON_LSHOULDER) || g_theInput->IsKeyDown('K'))
		{
			g_theGameClock->SetTimeScale(0.1f);
			if (m_game->m_energySelectionRing == nullptr)
			{
				m_game->SpawnEnergySelectionRing();
			}

			if (m_game->m_energySelectionRing != nullptr)
			{
				m_game->UpdateEnergySelectionRing(deltaSeconds);
			}
		}

		if (playerController.WasButtonJustReleased(XboxButtonID::XBOX_BUTTON_LSHOULDER) || g_theInput->WasKeyJustReleased('K'))
		{
			g_theGameClock->SetTimeScale(1.f);
			if (m_game->m_energySelectionRing != nullptr)
			{
				m_game->DeleteEnergySelectionRing();
			}
		}
	}

	if (m_shieldGenerated)
	{
		m_physicsRadius = PLAYERSHIP_SHIELD_RADIUS;
		m_game->m_energyBar->m_energyConsuming = ENERGY_CONSUMING_SHIELD_RATE;
	}

	if (!m_shieldGenerated)
	{
		m_physicsRadius = PLAYERSHIP_PHYSICS_RADIUS;
	}


	//keyboard controlling
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// thrusting control
	if (g_theInput->IsKeyDown('E'))
	{
		// deltaVelocity is the change in velocity in deltaSecond that the PlayerShip has
		acceleration += PLAYERSHIP_ACCERATION_LOW * deltaSeconds;
		m_thrustFraction = 1.0f;
	}

	// orientation calculation according to player's control
	if (g_theInput->IsKeyDown('S'))
	{
		m_angularVelocity += PLAYERSHIP_TURNRATE;
	}

	if (g_theInput->IsKeyDown('F'))
	{
		m_angularVelocity -= PLAYERSHIP_TURNRATE;
	}

	// if the player holds both s and f, the ship will not turn
	if (g_theInput->IsKeyDown('S') && g_theInput->IsKeyDown('F'))
	{
		m_angularVelocity = 0.f;
	}

	if (g_theInput->IsKeyDown('N'))
	{
		m_game->RespawnPlayer();
	}


	deltaVelocity = deltaVelocity.MakeFromPolarDegrees(m_orientationDegrees, acceleration);// acceleration is the pointing to the orientation degrees
	m_velocity += deltaVelocity;

	m_orientationDegrees += m_angularVelocity * deltaSeconds;

	BounceOffWalls();

	// recalculate player ship's position based on velocity
	m_position += m_velocity * deltaSeconds;

	// update the vertex of the thrust flame
	UpdateThrustFlameVerts();

	if (m_isDead)
	{
		if (!m_debrisPlayed)
		{
			m_game->SpawnDebris(this, 0.f, PLAYERSHIP_DEBRIS_MIN_NUM, PLAYERSHIP_DEBRIS_MAX_NUM, Rgba8(PLAYERSHIP_COLOR_R, PLAYERSHIP_COLOR_G, PLAYERSHIP_COLOR_B, DEBRIS_COLOR_A));
			m_debrisPlayed = true;
		}
	}
}

void PlayerShip::TakeDamage()
{
	if (m_shieldGenerated)
	{
		return;
	}
	else
	{
		m_isDead = true;
		if (m_game->m_energySelectionRing!=nullptr)
		{
			m_game->DeleteEnergySelectionRing();
		}
		g_theGameClock->SetTimeScale(1.f);
		m_shieldGenerated = false;
		m_speedBooster = false;
		m_fireRateBooster = false;
	}
	// play crash sound
}

void PlayerShip::Render() const //all called functions must be const
{
	RenderFlame();
	RenderShip();
	RenderShield();
}

void PlayerShip::RenderShield() const
{
	Rgba8 shieldRingColor = Rgba8(ENERGY_REMAIN_COLOR_R, ENERGY_REMAIN_COLOR_G, ENERGY_REMAIN_COLOR_B, ENERGY_REMAIN_COLOR_A);

	if (m_shieldGenerated)
	{
		DrawShieldRing(m_position, shieldRingColor, Rgba8(0, 0, 0, 0));
	}

	else return;
}

void PlayerShip::RenderShip() const
{
	// is current ship is destroyed, do not render
	if (m_isDead || m_isGarbage)
	{
		return;
	}

	Vertex_PCU tempWorldVerts[PLAYERSHIP_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < PLAYERSHIP_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(PLAYERSHIP_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(PLAYERSHIP_NUM_VERT, tempWorldVerts);
}

void PlayerShip::RenderFlame() const
{
	// is current ship is destroyed, do not render
	if (m_isDead || m_isGarbage)
	{
		return;
	}

	Vertex_PCU tempWorldVerts[3];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < 3; ++vi)
	{
		tempWorldVerts[vi] = m_thrustFlameVerts[vi];
	}

	TransformVertexArrayXY3D(3, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(3, tempWorldVerts);
}

void PlayerShip::UpdateThrustFlameVerts()
{
	float flameFlickingScale = g_rng->RollRandomFloatInRange(0.8f, 1.2f);

	// is it is in slow mode or is paused or the engine is killed, the flame would not be updated
	if (g_theGameClock->GetTimeScale() != 1.f || Clock::GetSystemClock().IsPaused() || m_thrustFraction == 0.f)
	{
		flameFlickingScale = 0.f;
	}

	// update vertex position
	m_thrustFlameVerts[0].m_position = Vec3(-1.8f, -1.f, 0.f);
	m_thrustFlameVerts[1].m_position = Vec3(-1.8f, 1.f, 0.f);
	if (m_speedBooster)
	{
		m_thrustFlameVerts[2].m_position = Vec3(  ( -PLAYERSHIP_FLAME_LENGTH_LONG * m_thrustFraction * flameFlickingScale ), 0.f, 0.f);// the x could larger than the 0
	}
	else
	{
		m_thrustFlameVerts[2].m_position = Vec3((-PLAYERSHIP_FLAME_LENGTH_SHORT * m_thrustFraction * flameFlickingScale), 0.f, 0.f);// the x could larger than the 0
	}

	// change vertex color
	for (int i = 0; i < 2; ++i)
	{
		m_thrustFlameVerts[i].m_color = Rgba8(PLAYERSHIP_FLAME_COLOR_R, PLAYERSHIP_FLAME_COLOR_R, PLAYERSHIP_FLAME_COLOR_R, 255);
	}
	m_thrustFlameVerts[2].m_color = Rgba8(PLAYERSHIP_FLAME_COLOR_R, PLAYERSHIP_FLAME_COLOR_G, PLAYERSHIP_FLAME_COLOR_B, 0);
}

void PlayerShip::InitializeFlameVerts()
{
	m_thrustFlameVerts[0].m_position = Vec3(-1.8f, -1.f, 0.f);
	m_thrustFlameVerts[1].m_position = Vec3(-1.8f, 1.f, 0.f);
	m_thrustFlameVerts[2].m_position = Vec3((-PLAYERSHIP_FLAME_LENGTH_LONG), 0.f, 0.f);

	for (int i = 0; i < 2; ++i)
	{
		m_thrustFlameVerts[i].m_color = Rgba8(PLAYERSHIP_FLAME_COLOR_R, PLAYERSHIP_FLAME_COLOR_G, PLAYERSHIP_FLAME_COLOR_B, 255);
	}

	m_thrustFlameVerts[2].m_color = Rgba8(PLAYERSHIP_FLAME_COLOR_R, PLAYERSHIP_FLAME_COLOR_G, PLAYERSHIP_FLAME_COLOR_B, 0);
}

// draw debug mode
void PlayerShip::DebugRender() const
{
	// Y axis
	Vec2 debugLineEnd_Y = Vec2(m_thrustFraction * 5.0f, 0.f);
	debugLineEnd_Y.SetOrientationDegrees(m_orientationDegrees);
	//debugLineEnd_Y.SetLength(PLAYERSHIP_COSMETIC_RADIUS);
	debugLineEnd_Y += m_position;
	DebugDrawLine(m_position, debugLineEnd_Y, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISY_COLOR_R, DEBUGLINE_AXISY_COLOR_G, DEBUGLINE_AXISY_COLOR_B, DEBUGLINE_AXISY_COLOR_A));
	// X axis
	Vec2 debugLineEnd_X = Vec2(0.f, 1.f);
	debugLineEnd_X.SetOrientationDegrees(m_orientationDegrees + 90.f); 
	debugLineEnd_X.SetLength(PLAYERSHIP_COSMETIC_RADIUS);
	debugLineEnd_X += m_position;
	DebugDrawLine(m_position, debugLineEnd_X, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISX_COLOR_R, DEBUGLINE_AXISX_COLOR_G, DEBUGLINE_AXISX_COLOR_B, DEBUGLINE_AXISX_COLOR_A));
	// collision debug ring
	DebugDrawRing(m_position, PLAYERSHIP_COSMETIC_RADIUS, DEBUGRING_THICKNESS, Rgba8(DEBUGRING_COSMETIC_COLOR_R, DEBUGRING_COSMETIC_COLOR_G, DEBUGRING_COSMETIC_COLOR_B, DEBUGRING_COSMETIC_COLOR_A));
	DebugDrawRing(m_position, PLAYERSHIP_PHYSICS_RADIUS, DEBUGRING_THICKNESS, Rgba8(DEBUGRING_PHYSICS_COLOR_R, DEBUGRING_PHYSICS_COLOR_G, DEBUGRING_PHYSICS_COLOR_B, DEBUGRING_PHYSICS_COLOR_A));
	// velocity debug line
	Vec2 debugLineEnd_Vel = m_position + m_velocity; // m_velocity is how far we move in a second
	DebugDrawLine(m_position, debugLineEnd_Vel, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_VELOCITY_COLOR_R, DEBUGLINE_VELOCITY_COLOR_G, DEBUGLINE_VELOCITY_COLOR_B, DEBUGLINE_VELOCITY_COLOR_A));
}

bool PlayerShip::IsOffScreen() const
{
	return false;
}
 
// the shape of the ship
void PlayerShip::InitializeLocalVerts()
{
	// nose
	m_localVerts[0].m_position = Vec3(1.f, 0.f, 0.f);
	m_localVerts[1].m_position = Vec3(0.f, 1.f, 0.f);
	m_localVerts[2].m_position = Vec3(0.f, -1.f, 0.f);
	//m_localVerts[0].m_color = Rgba8(255, 255, 255, 255);
	//m_localVerts[1].m_color = Rgba8(255, 255, 0, 255);
	//m_localVerts[2].m_color = Rgba8(255, 255, 0, 255);
	
	// left wing
	m_localVerts[3].m_position = Vec3(2.f, 1.f, 0.f);
	m_localVerts[4].m_position = Vec3(0.f, 2.f, 0.f);
	m_localVerts[5].m_position = Vec3(-2.f, 1.f, 0.f);

	// right wing
	m_localVerts[6].m_position = Vec3(2.f, -1.f, 0.f);
	m_localVerts[7].m_position = Vec3(-2.f, -1.f, 0.f);
	m_localVerts[8].m_position = Vec3(0.f, -2.f, 0.f);
	// body (tri 1 of 2)
	m_localVerts[9].m_position = Vec3(0.f, 1.f, 0.f);
	m_localVerts[10].m_position = Vec3(-2.f, -1.f, 0.f);
	m_localVerts[11].m_position = Vec3(0.f, -1.f, 0.f);

	// body (tri 2 of 2)
	m_localVerts[12].m_position = Vec3(0.f, 1.f, 0.f);
	m_localVerts[13].m_position = Vec3(-2.f, 1.f, 0.f);
	m_localVerts[14].m_position = Vec3(-2.f, -1.f, 0.f);

	// change color according to vertex index
	for (int vertIndex = 0; vertIndex < PLAYERSHIP_NUM_VERT; ++vertIndex)
	{
		m_localVerts[vertIndex].m_color = Rgba8(PLAYERSHIP_COLOR_R, PLAYERSHIP_COLOR_G, PLAYERSHIP_COLOR_B, PLAYERSHIP_COLOR_A);
	}
}

void PlayerShip::DrawShieldRing(Vec2 const& center, Rgba8 const& colorA, Rgba8 const& colorB) const
{
	// the goal is to draw trapezoid for DEBUG_NUM_SIDES times in a circle

	// get the R for inner and outer ring
	float innerRadius = m_shieldRadius ;
	float outerRadius = m_shieldRadius + m_shieldThickness;

	//get each pie's degree

	// all the vertexes will be contained in an array
	Vertex_PCU verts[SHIELD_RING_NUM_VERTS] = {};

	// go over each pie of the ring
	for (int sideIndex = 0; sideIndex < SHIELD_RING_NUM_SIDES; ++sideIndex)
	{
		// get the theta_degrees
		float startDegrees = SHIELD_DEGREES_PERSIDE * static_cast<float>(sideIndex);
		float endDegrees = SHIELD_DEGREES_PERSIDE * static_cast<float>(sideIndex + 1);

		// use polar system( cos radian and sin radian) to get Cartesian system
		float cosStrtPt = CosDegrees(startDegrees);
		float sinStrdPt = SinDegrees(startDegrees);
		float cosEndPt = CosDegrees(endDegrees);
		float sinEndPt = SinDegrees(endDegrees);

		// the local space position of four vertex
		Vec3 innerStrtPos(innerRadius * cosStrtPt, innerRadius * sinStrdPt, 0.f);
		Vec3 outerStrtPos(outerRadius * cosStrtPt, outerRadius * sinStrdPt, 0.f);
		Vec3 innerEndPos(innerRadius * cosEndPt, innerRadius * sinEndPt, 0.f);
		Vec3 outerEndPos(outerRadius * cosEndPt, outerRadius * sinEndPt, 0.f);

		// transform pos from local to world
		Vec3 centerPos = Vec3(center.x, center.y, 0.f);
		innerStrtPos += centerPos;
		outerStrtPos += centerPos;
		innerEndPos += centerPos;
		outerEndPos += centerPos;

		// trapezoid is made of two tris: tri ABC & tri DEF
		// C and E share the same spot, B and D share the same spot
		// match up the tri vertex with render vert array Index
		int VertIndexA = (sideIndex * 6) + 0;
		int VertIndexB = (sideIndex * 6) + 1;
		int VertIndexC = (sideIndex * 6) + 2;
		int VertIndexD = (sideIndex * 6) + 3;
		int VertIndexE = (sideIndex * 6) + 4;
		int VertIndexF = (sideIndex * 6) + 5;

		// assign the vertex with pos info
		// A is inner start, B is outer Start, C is inner end
		// D is outer end, E is inner end, F is outer start
		verts[VertIndexA].m_position = innerStrtPos;
		verts[VertIndexB].m_position = outerStrtPos;
		verts[VertIndexC].m_position = innerEndPos;
		verts[VertIndexA].m_color = colorA;
		verts[VertIndexB].m_color = colorB;
		verts[VertIndexC].m_color = colorA;


		verts[VertIndexD].m_position = outerEndPos;
		verts[VertIndexE].m_position = innerEndPos;
		verts[VertIndexF].m_position = outerStrtPos;
		verts[VertIndexD].m_color = colorB;
		verts[VertIndexE].m_color = colorA;
		verts[VertIndexF].m_color = colorB;
	}
	g_theRenderer->DrawVertexArray(SHIELD_RING_NUM_VERTS, &verts[0]);
}

void PlayerShip::BounceOffWalls()
{
	// if the ship reach the edge of the window, turn the speed around
	if (m_position.x + PLAYERSHIP_COSMETIC_RADIUS > WORLD_SIZE_X ||
		m_position.x - PLAYERSHIP_COSMETIC_RADIUS < 0.f)
	{

		// as well as teleport the ship back to the edge, where two kissing each other
		if (m_velocity.x > 0.f)
		{
			m_position.x = WORLD_SIZE_X - PLAYERSHIP_COSMETIC_RADIUS;
		}
		if (m_velocity.x < 0.f)
		{
			m_position.x = 0.f + PLAYERSHIP_COSMETIC_RADIUS;
		}
		// change the speed
		m_velocity.x *= (-1.f);
	}

	if (m_position.y + PLAYERSHIP_COSMETIC_RADIUS > WORLD_SIZE_Y ||
		m_position.y - PLAYERSHIP_COSMETIC_RADIUS < 0.f)
	{
		if (m_velocity.y > 0.f)
		{
			m_position.y = WORLD_SIZE_Y - PLAYERSHIP_COSMETIC_RADIUS;
		}
		if (m_velocity.y < 0.f)
		{
			m_position.y = PLAYERSHIP_COSMETIC_RADIUS;
		}
		m_velocity.y *= (-1.f);
	}
}
