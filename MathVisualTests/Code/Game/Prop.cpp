#include "Engine/Renderer/Renderer.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Game/Prop.hpp"
#include "Game/App.hpp"

extern Renderer* g_theRenderer;
extern Clock* g_theColorChangingClock;
extern Clock* g_theGameClock;
extern App*   g_theApp;

Prop::Prop()
{

}

Prop::~Prop()
{

}

void Prop::Update()
{
}

void Prop::ShutDown()
{

}

void Prop::Render() const
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);

	// bind UV textures to the sphere
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetModelConstants(GetModelMatrix(), m_color);	
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray((int)m_vertexes.size(), m_vertexes.data());
}

Mat44 Prop::GetModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);
	Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	transformMat.Append(orientationMat);
	return transformMat;
}

