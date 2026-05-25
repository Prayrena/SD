#include "Engine/Renderer/Renderer.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Game/Prop.hpp"
#include "Game/App.hpp"

extern Renderer* g_theRenderer;
extern Game* g_theGame;
extern Clock* g_theColorChangingClock;
extern Clock* g_theGameClock;
extern App*   g_theApp;

Prop::Prop()
	:Entity(g_theGame, Vec3())
{

}

Prop::~Prop()
{

}

void Prop::Update()
{
	float cubeRotatingSpeed = 30.f;
	float SphereRotatingSpeed = 45.f;
	float deltaSeconds = g_theGameClock->GetDeltaSeconds();
	m_angularVelocity = EulerAngles(0.f, 0.f, 0.f);

	// first cube is rotating around X and Y axis
	if (m_name == "First Big Cube")
	{
		// m_angularVelocity.m_rollDegrees = (cubeRotatingSpeed * deltaSeconds);
		// m_angularVelocity.m_pitchDegrees = (cubeRotatingSpeed * deltaSeconds);
	}

	// second cube is changing color from white to black
	else if (m_name == "Second Big Cube")
	{
		unsigned char colorValue = (unsigned char)(255.f * abs(sinf(g_theColorChangingClock->GetTotalSeconds())));
		m_color.r = colorValue;
		m_color.g = colorValue;
		m_color.b = colorValue;
		m_color.a = colorValue;
	}

	// the sphere is rotating about z-axis at 45 degrees per second
	else if (m_name == "Sphere")
	{
		m_orientation.m_yawDegrees += SphereRotatingSpeed * deltaSeconds;
	}

	// the sphere is rotating about X-axis at 45 degrees per second
	else if (m_name == "Cylinder")
	{
		m_orientation.m_pitchDegrees += SphereRotatingSpeed * deltaSeconds;
	}

	// cone is rotating around X and Y axis
	if (m_name == "Cone")
	{
		m_angularVelocity.m_rollDegrees = (cubeRotatingSpeed * deltaSeconds);
		m_angularVelocity.m_pitchDegrees = (cubeRotatingSpeed * deltaSeconds);
	}

	m_orientation += m_angularVelocity;
}

void Prop::ShutDown()
{
	delete m_indexBuffer;
	m_indexBuffer = nullptr;

	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;
}

void Prop::Render() const
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);

	// bind UV textures to the sphere
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetModelConstants(GetModelMatrix(), m_color);	

	if (m_name == "First Big Cube")
	{
		g_theRenderer->BindShader(g_theApp->g_shaders[DIFFUSE]);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		// g_theRenderer->SetLightingConstants(g_theRenderer->m_currentLightingSettings);
		g_theRenderer->DrawVertexArrayWithIndexArray(m_vertexBuffer, m_indexBuffer, (int)(m_indexArray.size()));
	}
	else
	{
		g_theRenderer->BindShader(g_theApp->g_shaders[DEFAULT]);
		g_theRenderer->DrawVertexArray((int)m_vertexes.size(), m_vertexes.data());
	}
}

