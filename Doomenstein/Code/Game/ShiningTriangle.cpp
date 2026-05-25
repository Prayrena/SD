#include "Game/ShiningTriangle.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;

// the children's constructor function need to initialize its parent class constructor first
ShiningTriangle::ShiningTriangle(Game* owner, Vec2 const& startPos)
	:UI(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_UIColor = Rgba8(ATTRACTMODE_PLAYBUTTON_COLOR_R, ATTRACTMODE_PLAYBUTTON_COLOR_G, ATTRACTMODE_PLAYBUTTON_COLOR_B, ATTRACTMODE_PLAYBUTTON_COLOR_A);
}

ShiningTriangle::~ShiningTriangle()
{
}

void ShiningTriangle::Update(float deltaSeconds)
{
	InitializeLocalVerts();

	UpdateAlphaColor(deltaSeconds);
}


void ShiningTriangle::UpdateAlphaColor(float deltaSeconds)
{
	m_age += deltaSeconds;

	m_UIColor.a = static_cast<unsigned char>(   RangeMap(sinf(m_age * colorChangingRate), -1.f, 1.f, 100.f, 255.f)  );

	// when player press start, the start button scaled up
	// if (g_theGame->m_permitToStartTimer)
	// {
	// 	m_scaleTimer += deltaSeconds;
	// 	float duration = TIME_TRANSITION_GAME;
	// 	m_uniformScale = Interpolate(1.0f, 50.f, m_scaleTimer / duration);
	// 	if (m_scaleTimer / duration > 0.5f)// if the screen is almost cover up, change the color into the background color
	// 	{
	// 		m_UIColor.r = static_cast<unsigned char>(Interpolate(m_UIColor.a, WORLD_COLOR_R, m_scaleTimer / duration));
	// 		m_UIColor.g = static_cast<unsigned char>(Interpolate(m_UIColor.a, WORLD_COLOR_G, m_scaleTimer / duration));
	// 		m_UIColor.b = static_cast<unsigned char>(Interpolate(m_UIColor.a, WORLD_COLOR_B, m_scaleTimer / duration));
	// 		m_UIColor.a = static_cast<unsigned char>(Interpolate(m_UIColor.a, 255.f, m_scaleTimer / duration));
	// 	}
	// }
}


void ShiningTriangle::Render() const //all called functions must be const
{
	// is current ship is destroyed, do not render
	if (!m_isEnabled)
	{
		return;
	}

	Vertex_PCU tempWorldVerts[ATTRACTMODE_TRI_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < ATTRACTMODE_TRI_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(ATTRACTMODE_TRI_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(ATTRACTMODE_TRI_NUM_VERT, tempWorldVerts);
}


// the shape of the ship
void ShiningTriangle::InitializeLocalVerts()
{
	m_localVerts[0].m_position = Vec3(-5.f, 4.f, 0.f);
	m_localVerts[1].m_position = Vec3(0.f, -6.f, 0.f);
	m_localVerts[2].m_position = Vec3(5.f, 4.f, 0.f);

	// change color according to vertex index
	for (int vertIndex = 0; vertIndex < ATTRACTMODE_TRI_NUM_VERT; ++vertIndex)
	{
		m_localVerts[vertIndex].m_position *= m_uniformScale;
		m_localVerts[vertIndex].m_color = m_UIColor;
	}
}

