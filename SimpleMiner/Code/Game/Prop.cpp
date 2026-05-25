#include "Engine/Renderer/Renderer.hpp"
#include "Engine/core/Clock.hpp"
#include "Game/Prop.hpp"

extern Renderer* g_theRenderer;
extern Game* g_theGame;
extern Clock* g_theColorChangingClock;
extern Clock* g_theGameClock;

Prop::Prop()
	:Entity(g_theGame, Vec3())
{

}

Prop::~Prop()
{

}

void Prop::Update()
{
	// float cubeRotatingSpeed = 30.f;
	// float SphereRotatingSpeed = 45.f;
	// float deltaSeconds = g_theGameClock->GetDeltaSeconds();
	m_angularVelocity = EulerAngles(0.f, 0.f, 0.f);

	m_orientation += m_angularVelocity;
}

void Prop::Render() const
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);

	// bind UV textures to the sphere
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetModelConstants(GetModelMatrix(), m_color);	
	g_theRenderer->DrawVertexArray((int)m_vertexes.size(), m_vertexes.data());
}

