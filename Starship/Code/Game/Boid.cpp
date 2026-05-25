#include "Game/Boid.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/PlayerShip.hpp"
#include "Game/Game.hpp"// in order to use the 
#include "Game/App.hpp"
#include "Engine/core/EngineCommon.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;
extern PlayerShip* g_playerShip;

// the children's constructor function need to initialize its parent class constructor first
Boid::Boid(Game* owner, Vec2 const& startPos)
	:Entity(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_physicsRadius  = BOID_PHYSICS_RADIUS;
	m_cosmeticRadius = BOID_COSMETIC_RADIUS;
	InitializeLocalVerts();
	m_health = 1;

	m_position = startPos;
}

Boid::~Boid()
{
}

void Boid::Update(float deltaSeconds)
{
	//m_thrustingTimer += deltaSeconds;
	//
	//if (m_thrustingTimer < BOID_TRUST_DURATION)
	//{
	//	Vec2 deltaVelocity = deltaVelocity.MakeFromPolarDegrees(m_orientationDegrees, BOID_ACCERATION * deltaSeconds);
	//}
	//else
	//{
	//	deltaVelocity = Vec2(0.f, 0.f);
	//}


	// Separation : steer away from nearby boids
	// get acceleration opposite towards other boids
	m_nearestFriend = m_game->GetTheNearestBoid(m_position);
	Vec2 disp = m_position - (m_nearestFriend->m_position);
	if (disp.GetLength() < BOID_SOCIAL_DISTANCE)
	{
		// use range map instead
		m_seperateVelocity = ( 1 / (disp.GetLength() + 1.f) ) * disp.GetNormalized() * BOID_SEPERATION_MULTIPLIER * deltaSeconds;
	}

	else
	{
		//m_nearestFriend = nullptr;
		m_seperateVelocity = Vec2(0.f, 0.f);
	}

	// Alignment : steer the velocity as nearby boids
	// get the average velocity of nearby friends
	m_alignVelocity = m_game->GetAverageFriendVelocityInRange(m_position, m_normal);
	m_alignVelocity *= BOID_ALIGNMENT_MULTIPLIER * deltaSeconds;


	// Cohesion : steer towards the center of the boids
	// get acceleration opposite towards the center of nearby boids
	Vec2 friendsCenterPos = m_game->GetCenterPositionofFriendsInRange(m_position);
	m_cohereVelocity = (friendsCenterPos - m_position);
	m_cohereVelocity *= BOID_COHESION_MULTIPLIER * deltaSeconds;

	// Aggression : have a acceleration towards the player, 
	// when go faraway, this part of velocity start to dominate
	if (!m_game->m_playerShip->m_isDead)
	{
		Vec2 dispToPlayer = (m_game->m_playerShip->m_position - m_position);
		m_playerAttractionVelocity = dispToPlayer * BOID_PLAYERATTRACTION_MULTIPLER * deltaSeconds;
	}
	else
	{
		m_playerAttractionVelocity = Vec2(0.f, 0.f);
	}

	m_velocity += m_seperateVelocity + m_alignVelocity + m_cohereVelocity + m_playerAttractionVelocity ;
	m_velocity = m_velocity.GetClamped(BOID_MAXSPEED);

	m_orientationDegrees = m_velocity.GetOrientationDegrees();

	TransformBoidAcrossScreen();

	// recalculate player ship's position based on velocity
	m_position += m_velocity * deltaSeconds;

	UpdateOrientation();
}

void Boid::TakeDamage()
{
	if (m_health > 0)
	{
		m_health -= 1;
	}

	if (m_health == 0)
	{
		m_isDead = true;
		m_isGarbage = true;
		m_game->TurnCameraShakeOnForExplosion();
		m_game->SpawnDebris(this, 0.f, ENTITY_DEBRIS_MIN_NUM, ENTITY_DEBRIS_MAX_NUM, Rgba8(WASP_COLOR_R, WASP_COLOR_G, WASP_COLOR_B, DEBRIS_COLOR_A));
	}

	// play crash sound
}

void Boid::Render() const //all called functions must be const
{
	RenderShip();
}


void Boid::RenderShip() const
{
	// is current ship is destroyed, do not render
	if (m_isDead || m_isGarbage)
	{
		return;
	}

	Vertex_PCU tempWorldVerts[Boid_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < Boid_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(Boid_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(Boid_NUM_VERT, tempWorldVerts);
}

void Boid::UpdateOrientation()
{
	m_normal = Vec2(1.f, 0.f);
	m_normal.SetOrientationDegrees(m_orientationDegrees);
}

// draw debug mode
void Boid::DebugRender() const
{
	//// Y axis
	//Vec2 debugLineEnd_Y = Vec2(1.f, 0.f);
	//debugLineEnd_Y.SetOrientationDegrees(m_orientationDegrees);
	//debugLineEnd_Y.SetLength(PLAYERSHIP_COSMETIC_RADIUS);
	//debugLineEnd_Y += m_position;
	//DebugDrawLine(m_position, debugLineEnd_Y, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISY_COLOR_R, DEBUGLINE_AXISY_COLOR_G, DEBUGLINE_AXISY_COLOR_B, DEBUGLINE_AXISY_COLOR_A));
	//// X axis
	//Vec2 debugLineEnd_X = Vec2(0.f, 1.f);
	//debugLineEnd_X.SetOrientationDegrees(m_orientationDegrees + 90.f); 
	//debugLineEnd_X.SetLength(PLAYERSHIP_COSMETIC_RADIUS);
	//debugLineEnd_X += m_position;
	//DebugDrawLine(m_position, debugLineEnd_X, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISX_COLOR_R, DEBUGLINE_AXISX_COLOR_G, DEBUGLINE_AXISX_COLOR_B, DEBUGLINE_AXISX_COLOR_A));
	
	// debug ring
	// show the friend in range
	DebugDrawRing(m_position, BOID_TOWARDSCENTER_RANGE, DEBUGRING_THICKNESS, Rgba8(DEBUGRING_COSMETIC_COLOR_R, DEBUGRING_COSMETIC_COLOR_G, DEBUGRING_COSMETIC_COLOR_B, DEBUGRING_COSMETIC_COLOR_A));
	DebugDrawRing(m_position, BOID_SOCIAL_DISTANCE, DEBUGRING_THICKNESS, Rgba8(DEBUGRING_PHYSICS_COLOR_R, DEBUGRING_PHYSICS_COLOR_G, DEBUGRING_PHYSICS_COLOR_B, DEBUGRING_PHYSICS_COLOR_A));

	// velocity debug line
	//Vec2 debug_Velocity= m_position + m_velocity; 
	//DebugDrawLine(m_position, debug_Velocity, DEBUGLINE_THICKNESS, Rgba8(255, 0, 0, DEBUGLINE_VELOCITY_COLOR_A));
	Vec2 debug_AlignVelocity = m_position + m_alignVelocity ; 
	DebugDrawLine(m_position, debug_AlignVelocity, DEBUGLINE_THICKNESS, Rgba8(0, 0, 255, DEBUGLINE_VELOCITY_COLOR_A));
	Vec2 debug_SeperateVelocity = m_position + m_seperateVelocity; 
	DebugDrawLine(m_position, debug_SeperateVelocity, DEBUGLINE_THICKNESS, Rgba8(255, 0, 0, DEBUGLINE_VELOCITY_COLOR_A));
	Vec2 debug_CohereVelocity = m_position + m_cohereVelocity; 
	DebugDrawLine(m_position, debug_CohereVelocity, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISX_COLOR_R, DEBUGLINE_AXISX_COLOR_G, DEBUGLINE_AXISX_COLOR_B, DEBUGLINE_AXISX_COLOR_A));

	// debug line to nearest frind
	if (m_nearestFriend != nullptr)
	{
		DebugDrawLine(m_position, m_nearestFriend->m_position, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISX_COLOR_R, DEBUGLINE_AXISX_COLOR_G, DEBUGLINE_AXISX_COLOR_B, DEBUGLINE_AXISX_COLOR_A));
	}
}

bool Boid::IsOffScreen() const
{
	return false;
}

// the shape of the ship
void Boid::InitializeLocalVerts()
{
	// nose
	m_localVerts[0].m_position = Vec3(0.f, -.5f, 0.f);
	m_localVerts[1].m_position = Vec3(2.f, 0.f, 0.f);
	m_localVerts[2].m_position = Vec3(0.f, .5f, 0.f);

	// change color according to vertex index
	for (int vertIndex = 0; vertIndex < Boid_NUM_VERT; ++vertIndex)
	{
		m_localVerts[vertIndex].m_color = Rgba8(WASP_COLOR_R, WASP_COLOR_G, WASP_COLOR_B, WASP_COLOR_A);
	}
}

void Boid::TransformBoidAcrossScreen()
{
	// when asteroid is about off the screen from the top edge
	if (m_position.y > (WORLD_SIZE_Y + BOID_COSMETIC_RADIUS))
	{
		m_position.y = -BOID_COSMETIC_RADIUS;
	}
	// when asteroid is about off the screen from the right edge
	if (m_position.x > (WORLD_SIZE_X + BOID_COSMETIC_RADIUS))
	{
		m_position.x = -BOID_COSMETIC_RADIUS;
	}
	// when asteroid is about off the screen from the bottom edge
	if (m_position.y < -BOID_COSMETIC_RADIUS)
	{
		m_position.y = WORLD_SIZE_Y + BOID_COSMETIC_RADIUS;
	}
	// when asteroid is about off the screen from the right edge
	if (m_position.x < -BOID_COSMETIC_RADIUS)
	{
		m_position.x = WORLD_SIZE_X + BOID_COSMETIC_RADIUS;
	}
}
