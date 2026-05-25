#include "Game/PlayerShipIcon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;

// the children's constructor function need to initialize its parent class constructor first
PlayerShipIcon::PlayerShipIcon(Game* owner, Vec2 const& startPos)
	:UIElement(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_uniformScale = 0.9f;
	InitializeLocalVerts();
}

PlayerShipIcon::~PlayerShipIcon()
{
}

void PlayerShipIcon::Update(float deltaSeconds)
{

}

void PlayerShipIcon::Render() const //all called functions must be const
{
	Vertex_PCU tempWorldVerts[PLAYERSHIPICON_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < PLAYERSHIPICON_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(PLAYERSHIPICON_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(PLAYERSHIPICON_NUM_VERT, tempWorldVerts);
}

// the shape of the ship
void PlayerShipIcon::InitializeLocalVerts()
{
	// nose
	m_localVerts[0].m_position = Vec3(1.f, 0.f, 0.f) ;
	m_localVerts[1].m_position = Vec3(0.f, 1.f, 0.f) ;
	m_localVerts[2].m_position = Vec3(0.f, -1.f, 0.f);
	//m_localVerts[0].m_color = Rgba8(255, 255, 255, 255);
	//m_localVerts[1].m_color = Rgba8(255, 255, 0, 255);
	//m_localVerts[2].m_color = Rgba8(255, 255, 0, 255);

	// left wing
	m_localVerts[3].m_position = Vec3(2.f, 1.f, 0.f)   ;
	m_localVerts[4].m_position = Vec3(0.f, 2.f, 0.f)   ;
	m_localVerts[5].m_position = Vec3(-2.f, 1.f, 0.f)  ;

	// right wing
	m_localVerts[6].m_position = Vec3(2.f, -1.f, 0.f)  ;
	m_localVerts[7].m_position = Vec3(0.f, -2.f, 0.f)  ;
	m_localVerts[8].m_position = Vec3(-2.f, -1.f, 0.f) ;
	// body (tri 1 of 2)
	m_localVerts[9].m_position = Vec3(0.f, 1.f, 0.f)   ;
	m_localVerts[10].m_position = Vec3(-2.f, -1.f, 0.f);
	m_localVerts[11].m_position = Vec3(0.f, -1.f, 0.f) ;

	// body (tri 2 of 2)
	m_localVerts[12].m_position = Vec3(0.f, 1.f, 0.f)  ;
	m_localVerts[13].m_position = Vec3(-2.f, 1.f, 0.f) ;
	m_localVerts[14].m_position = Vec3(-2.f, -1.f, 0.f);

	// change color according to vertex index
	for (int vertIndex = 0; vertIndex < PLAYERSHIPICON_NUM_VERT; ++vertIndex)
	{
		m_localVerts[vertIndex].m_position *= m_uniformScale;
		m_localVerts[vertIndex].m_color = Rgba8(PLAYERSHIP_COLOR_R, PLAYERSHIP_COLOR_G, PLAYERSHIP_COLOR_B, PLAYERSHIP_COLOR_A);
	}
}

