#include "Game/Asteroid.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;
extern RandomNumberGenerator* g_rng;



// the children's constructor function need to initialize its parent class constructor first
Asteroid::Asteroid(Game* owner, Vec2 const& startPos)
	:Entity(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_physicsRadius = ASTEROID_PHYSICS_RADIUS;
	m_cosmeticRadius = ASTEROID_COSMETIC_RADIUS;
	InitializeLocalVerts();

	m_health = ASTEROID_HEALTH;

	// reassign my debris
	m_debrisColor = Rgba8(ASTEROID_COLOR_R, ASTEROID_COLOR_G, ASTEROID_COLOR_B, DEBRIS_COLOR_A);

}

Asteroid::~Asteroid()
{
}


void Asteroid::Update(float deltaSeconds)
{
	//calculate *this health
	if (m_health < 1)
	{
		m_isDead = true;
		return;			// cut the update if *this is already dead
	}

	// position and orientation update
	m_position += m_velocity * deltaSeconds;
	m_orientationDegrees += m_angularVelocity * deltaSeconds;

	TransformAsteroidAcrossScreen();
	
}

void Asteroid::TakeDamage()
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
		m_game->SpawnDebris(this, 0.f, ENTITY_DEBRIS_MIN_NUM, ENTITY_DEBRIS_MAX_NUM, Rgba8(ASTEROID_COLOR_R, ASTEROID_COLOR_G, ASTEROID_COLOR_B, DEBRIS_COLOR_A));
	}

	// play asteroid splitting sound
}

void Asteroid::Render() const
{
	// if the current asteroid is define as dead
	// or it is off the screen, it all don't need for further rendering
	if (m_isDead || m_isGarbage)
		return;

	Vertex_PCU tempWorldVerts[ASTEROID_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;
	
	for (vi; vi < ASTEROID_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(ASTEROID_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(ASTEROID_NUM_VERT, tempWorldVerts);
}

void Asteroid::DebugRender() const
{
	// Y axis
	Vec2 debugLineEnd_Y = Vec2(1.f, 0.f);
	debugLineEnd_Y.SetOrientationDegrees(m_orientationDegrees);
	debugLineEnd_Y.SetLength(ASTEROID_COSMETIC_RADIUS);
	debugLineEnd_Y += m_position;
	DebugDrawLine(m_position, debugLineEnd_Y, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISY_COLOR_R, DEBUGLINE_AXISY_COLOR_G, DEBUGLINE_AXISY_COLOR_B, DEBUGLINE_AXISY_COLOR_A));
	// X axis
	Vec2 debugLineEnd_X = Vec2(0.f, 1.f);
	debugLineEnd_X.SetOrientationDegrees(m_orientationDegrees + 90.f);
	debugLineEnd_X.SetLength(ASTEROID_COSMETIC_RADIUS);
	debugLineEnd_X += m_position;
	DebugDrawLine(m_position, debugLineEnd_X, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISX_COLOR_R, DEBUGLINE_AXISX_COLOR_G, DEBUGLINE_AXISX_COLOR_B, DEBUGLINE_AXISX_COLOR_A));
	// collision debug ring
	DebugDrawRing(m_position, ASTEROID_COSMETIC_RADIUS, DEBUGRING_THICKNESS, Rgba8(DEBUGRING_COSMETIC_COLOR_R, DEBUGRING_COSMETIC_COLOR_G, DEBUGRING_COSMETIC_COLOR_B, DEBUGRING_COSMETIC_COLOR_A));
	DebugDrawRing(m_position, ASTEROID_PHYSICS_RADIUS, DEBUGRING_THICKNESS, Rgba8(DEBUGRING_PHYSICS_COLOR_R, DEBUGRING_PHYSICS_COLOR_G, DEBUGRING_PHYSICS_COLOR_B, DEBUGRING_PHYSICS_COLOR_A));
	// velocity debug line
	Vec2 debugLineEnd_Vel = m_position + m_velocity; // m_velocity is how far we move in a second
	DebugDrawLine(m_position, debugLineEnd_Vel, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_VELOCITY_COLOR_R, DEBUGLINE_VELOCITY_COLOR_G, DEBUGLINE_VELOCITY_COLOR_B, DEBUGLINE_VELOCITY_COLOR_A));

}

// use this function to initialize the verts info inside m_localVerts
void Asteroid::InitializeLocalVerts()
{
	// calculate the random radii for each triangle, so could get (R, theta_radian) for each vertex
	float astrdRadii[ASTEROID_NUM_VERT];
	for (int sideNum = 0; sideNum < ASTEROID_NUM_VERT; ++sideNum)
	{
		astrdRadii[sideNum] = g_rng->RollRandomFloatInRange(ASTEROID_PHYSICS_RADIUS, ASTEROID_COSMETIC_RADIUS);
	}

	// get the Polar coordinate for the vertex then transform into Cartesian coordinate system
	float astrdPerPieDegrees = 360.f / static_cast<float>( ASTEROID_NUM_VERT );// for doing operation, two variable need to be the same type
	Vec2 astrdVertPos[ASTEROID_NUM_VERT];
	for (int sideNum = 0; sideNum < ASTEROID_NUM_VERT; ++sideNum)
	{
		float degrees = astrdPerPieDegrees * static_cast<float>( sideNum );
		float radii = astrdRadii[sideNum];
		astrdVertPos[sideNum].x = radii * cosf(degrees);
		astrdVertPos[sideNum].y = radii * sinf(degrees);
	}

	// build triangles according to the vertex array
	for (int triNum = 0; triNum < ASTEROID_NUM_TRI; ++triNum)
	{
		// get the Index of the astrdVertsPos to draw tri
		int StartVertIndex = triNum;
		int EndVertIndex = (triNum + 1) % ASTEROID_NUM_TRI; // make that the last tri includes the first vertex
		Vec3 center = Vec3(0, 0, 0); // we use these three points to draw tri

		// calculate the Index for each tri vertex sending to render
		int firstVertIndex = (triNum * 3) + 0;
		int SecondVertIndex = (triNum * 3) + 1;
		int ThirdVertIndex = (triNum * 3) + 2;

		// write Vec3 info into m_localVerts 
		m_localVerts[firstVertIndex].m_position = center; // 1st vertex of each tri will always be the center
		m_localVerts[SecondVertIndex].m_position = Vec3(astrdVertPos[StartVertIndex].x, astrdVertPos[StartVertIndex].y, 0.f);
		m_localVerts[ThirdVertIndex].m_position = Vec3(astrdVertPos[EndVertIndex].x, astrdVertPos[EndVertIndex].y, 0.f);

		// rewrite the color to each asteroid
		m_localVerts[firstVertIndex].m_color = Rgba8(ASTEROID_COLOR_R, ASTEROID_COLOR_G, ASTEROID_COLOR_B, ASTEROID_COLOR_A);
		m_localVerts[SecondVertIndex].m_color = Rgba8(ASTEROID_COLOR_R, ASTEROID_COLOR_G, ASTEROID_COLOR_B, ASTEROID_COLOR_A);
		m_localVerts[ThirdVertIndex].m_color = Rgba8(ASTEROID_COLOR_R, ASTEROID_COLOR_G, ASTEROID_COLOR_B, ASTEROID_COLOR_A);
	}
}

void Asteroid::TransformAsteroidAcrossScreen()
{
	// when asteroid is about off the screen from the top edge
	if (m_position.y > (WORLD_SIZE_Y + ASTEROID_COSMETIC_RADIUS))
	{
		m_position.y = -ASTEROID_COSMETIC_RADIUS;
	}
	// when asteroid is about off the screen from the right edge
	if (m_position.x > (WORLD_SIZE_X + ASTEROID_COSMETIC_RADIUS))
	{
		m_position.x = -ASTEROID_COSMETIC_RADIUS;
	}
	// when asteroid is about off the screen from the bottom edge
	if (m_position.y < -ASTEROID_COSMETIC_RADIUS)
	{
		m_position.y = WORLD_SIZE_Y + ASTEROID_COSMETIC_RADIUS;
	}
	// when asteroid is about off the screen from the right edge
	if (m_position.x < -ASTEROID_COSMETIC_RADIUS)
	{
		m_position.x = WORLD_SIZE_X + ASTEROID_COSMETIC_RADIUS;
	}
}

bool Asteroid::IsOffScreen() const
{
	// if the asteroid is completely could not be seen over the window, return true
	if (m_position.x - ASTEROID_PHYSICS_RADIUS > WORLD_SIZE_X ||
		m_position.x + ASTEROID_PHYSICS_RADIUS < 0.f)
	{
		return true;
	}

	if (m_position.y - ASTEROID_PHYSICS_RADIUS > WORLD_SIZE_Y ||
		m_position.y + ASTEROID_PHYSICS_RADIUS < 0.f)
	{
		return true;
	}

	else
	{
		return false;
	}
}