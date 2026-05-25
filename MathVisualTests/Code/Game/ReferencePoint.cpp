#include "Game/ReferencePoint.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/GetNearestPointsMode.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;
extern InputSystem* g_theInput;
// extern AudioSystem* g_theAudio;
extern RandomNumberGenerator* g_rng;

// the children's constructor function need to initialize its parent class constructor first
ReferencePoint::ReferencePoint( Vec2 const& startPos)
	:Entity(startPos)// constructing the entity before the constructing the bullet
{
	m_physicsRadius = PLAYERSHIP_PHYSICS_RADIUS;
	m_cosmeticRadius = PLAYERSHIP_COSMETIC_RADIUS;
	InitializeLocalVerts();
}

ReferencePoint::~ReferencePoint()
{
}

void ReferencePoint::Update(float deltaSeconds)
{
	//keyboard controlling
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// thrusting control
	if (g_theInput->IsKeyDown('E'))
	{
		m_position += Vec2( 0.f, PLAYERSHIP_ACCERATION_LOW * deltaSeconds);
	}

	// orientation calculation according to player's control
	if (g_theInput->IsKeyDown('S'))
	{
		m_position += Vec2(-PLAYERSHIP_ACCERATION_LOW * deltaSeconds, 0.f);
	}

	if (g_theInput->IsKeyDown('F'))
	{
		m_position += Vec2( PLAYERSHIP_ACCERATION_LOW * deltaSeconds, 0.f);
	}

	if (g_theInput->IsKeyDown('D'))
	{
		m_position += Vec2(0.f, -PLAYERSHIP_ACCERATION_LOW * deltaSeconds);
	}
}

void ReferencePoint::Render() const //all called functions must be const
{
	RenderShip();
}

void ReferencePoint::RenderShip() const
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

// draw debug mode
void ReferencePoint::DebugRender() const
{
}

bool ReferencePoint::IsOffScreen() const
{
	return false;
}

// the shape of the ship
void ReferencePoint::InitializeLocalVerts()
{

}
