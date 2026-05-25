#include "Game/StarField.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/ReferenceInput.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/GetNearestPointsScene.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;

// the children's constructor function need to initialize its parent class constructor first
StarField::StarField(GetNearestPointsScene* owner, Vec2 const& startPos)
	:UI(owner, startPos)// constructing the entity before the constructing the bullet
{
	//m_UIColor = Rgba8(BEETLE_COLOR_R, BEETLE_COLOR_G, BEETLE_COLOR_B, 255);
	InitializeLocalVerts();
}

StarField::~StarField()
{
	
}

void StarField::Update(float deltaSeconds)
{
	UpdateAlphaColor(deltaSeconds);
	InitializeLocalVerts();
}


void StarField::UpdateAlphaColor(float deltaSeconds)
{
	// shinning color
	m_age += deltaSeconds;	
	m_UIColor.a = static_cast<unsigned char>(   RangeMap(sinf(m_age * m_shinningFrequency), -1.f, 1.f, 60.f, 255.f)  );

	//every frame reset the moving speed of the star to 0
	m_velocity = Vec2(0.f, 0.f);
	// if the player is not dead, then the star moving counter direction to the player
	if (!m_game->m_testingPoint->m_isDead)
	{
		m_velocity = m_counterPlayerVelocityScale * (m_game->m_testingPoint->m_velocity);
		m_velocity *= -1.f;
	}

	m_position += m_velocity * deltaSeconds;

	//float shinningDuration = 2.0f;
	//if (!m_brightnessIncreasing)
	//{
	//	m_shiningTimer -= deltaSeconds;
	//	if (m_shiningTimer <= 0.f)
	//	{
	//		m_brightnessIncreasing = true;
	//		m_shiningTimer = 0.f;
	//	}
	//	m_debrisColor.a = static_cast<unsigned char>( Interpolate(0.f, 255.f, m_shiningTimer / shinningDuration)  );
	//}																							  																							  
	//else																	  
	//{																							  
	//	m_shiningTimer += deltaSeconds;														  
	//	if (m_shiningTimer >= shinningDuration)
	//	{																						  
	//		m_brightnessIncreasing = false;	
	//		m_shiningTimer = shinningDuration;
	//	}																						  
	//	m_debrisColor.a = static_cast<unsigned char>(  Interpolate(0.f, 255.f, m_shiningTimer / shinningDuration)  );
	//}
}


void StarField::Render() const //all called functions must be const
{
	// is current ship is destroyed, do not render
	if (!m_isEnabled)
	{
		return;
	}

	Vertex_PCU tempWorldVerts[STARFIELD_TRI_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < STARFIELD_TRI_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(STARFIELD_TRI_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(STARFIELD_TRI_NUM_VERT, tempWorldVerts);
}


// the shape of the ship
void StarField::InitializeLocalVerts()
{
	m_localVerts[0].m_position = Vec3(-1.5f, 0.f, 0.f);
	m_localVerts[1].m_position = Vec3(1.5f, 0.f, 0.f);
	m_localVerts[2].m_position = Vec3(0.f, .5f, 0.f);

	// left wing
	m_localVerts[3].m_position = Vec3(-1.5f, 0.f, 0.f);
	m_localVerts[4].m_position = Vec3(0.f, -.5f, 0.f);
	m_localVerts[5].m_position = Vec3(1.5f, 0.f, 0.f);

	// right wing
	m_localVerts[6].m_position = Vec3(0.f, -2.f, 0.f);
	m_localVerts[7].m_position = Vec3(0.5f, 0.f, 0.f);
	m_localVerts[8].m_position = Vec3(0.f, 2.f, 0.f);
	// body (tri 1 of 2)
	m_localVerts[9].m_position = Vec3(0.f, -2.f, 0.f);
	m_localVerts[10].m_position = Vec3(0.f, 2.f, 0.f);
	m_localVerts[11].m_position = Vec3(-.5f, 0.f, 0.f);

	// change color according to vertex index
	for (int vertIndex = 0; vertIndex < STARFIELD_TRI_NUM_VERT; ++vertIndex)
	{
		m_localVerts[vertIndex].m_position *= m_uniformScale;
		m_localVerts[vertIndex].m_color = m_UIColor;
	}
}

