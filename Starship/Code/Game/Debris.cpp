#include "Game/Debris.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;
extern RandomNumberGenerator* g_rng;

// the children's constructor function need to initialize its parent class constructor first
Debris::Debris(Game* owner, Vec2 const& startPos)
	:Entity(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_isVFX = true;
}

Debris::~Debris()
{
}

bool Debris :: DebrisDeadDueToAge()
{
	if (m_age < 0)
	{
		return true;
	}

	else
	{
		return false;
	}
}

void Debris::Update(float deltaSeconds)
{
	// the debris could die due to the life span ends
	m_age -= deltaSeconds;
	if (DebrisDeadDueToAge())
	{
		m_isDead = true;
		return;
	}

	if (IsOffScreen())
	{
		m_isGarbage = true;
		return;
	}

	UpdateDebrisColorAlpha();

	InitializeLocalVerts();

	// the debris' alpha color decrease to 0 in 2 seconds
	// translate the unsigned char into float first
	// do the interpolate then, and translate the float back to unsigned char
	// m_debrisColor.a -= static_cast<unsigned char>((static_cast<float>m_debrisColor.a / 2) * deltaSeconds);

	// position and orientation update
	m_position += m_velocity * deltaSeconds;
	m_orientationDegrees += m_angularVelocity * deltaSeconds;
}

void Debris::Render() const
{
	// if the current asteroid is define as dead
	// or it is off the screen, it all don't need for further rendering
	if (m_isDead || m_isGarbage)
	{
		return;
	}

	Vertex_PCU tempWorldVerts[DEBRIS_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;
	
	for (vi; vi < DEBRIS_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(DEBRIS_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(DEBRIS_NUM_VERT, tempWorldVerts);
}

void Debris::DebugRender() const
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

void Debris::UpdateDebrisColorAlpha()
{
	float colorAlpha;
	colorAlpha = static_cast<float>(m_debrisColor.a);
	colorAlpha = Interpolate(0.f, 127.f, m_age / DEBRIS_LIFESPAN);
	m_debrisColor.a = static_cast<unsigned char>(colorAlpha);
}

// use this function to initialize the verts info inside m_localVerts
void Debris::InitializeLocalVerts()
{
	// calculate the random radii for each triangle, so could get (R, theta_radian) for each vertex
	float astrdRadii[DEBRIS_NUM_VERT];
	for (int sideNum = 0; sideNum < DEBRIS_NUM_VERT; ++sideNum)
	{
		astrdRadii[sideNum] = g_rng->RollRandomFloatInRange(debris_inner_radius, debris_outer_radius);
	}

	// get the Polar coordinate for the vertex then transform into Cartesian coordinate system
	float astrdPerPieDegrees = 360.f / static_cast<float>( DEBRIS_NUM_VERT );// for doing operation, two variable need to be the same type
	Vec2 astrdVertPos[DEBRIS_NUM_VERT];
	for (int sideNum = 0; sideNum < DEBRIS_NUM_VERT; ++sideNum)
	{
		float degrees = astrdPerPieDegrees * static_cast<float>( sideNum );
		float radii = astrdRadii[sideNum];
		astrdVertPos[sideNum].x = radii * cosf(degrees);
		astrdVertPos[sideNum].y = radii * sinf(degrees);
	}

	// build triangles according to the vertex array
	for (int triNum = 0; triNum < DEBRIS_NUM_TRI; ++triNum)
	{
		// get the Index of the astrdVertsPos to draw tri
		int StartVertIndex = triNum;
		int EndVertIndex = (triNum + 1) % DEBRIS_NUM_TRI; // make that the last tri includes the first vertex
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
		m_localVerts[firstVertIndex].m_color  = m_debrisColor;
		m_localVerts[SecondVertIndex].m_color = m_debrisColor;
		m_localVerts[ThirdVertIndex].m_color  = m_debrisColor;
	}
}

bool Debris::IsOffScreen() const
{
	// if the asteroid is completely could not be seen over the window, return true
	if (m_position.x - debris_outer_radius > WORLD_SIZE_X ||
		m_position.x + debris_outer_radius < 0.f)
	{
		return true;
	}

	if (m_position.y - debris_outer_radius > WORLD_SIZE_Y ||
		m_position.y + debris_outer_radius < 0.f)
	{
		return true;
	}

	else
	{
		return false;
	}
}