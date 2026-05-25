#include "Engine/UI/HealthBar.hpp"
#include "Engine/Renderer/Renderer.hpp"

extern Renderer* g_theRenderer;

HealthBar::HealthBar(Actor* trackingActor, Vec2 const& size, Vec2 const& frameOffset /*= Vec2(0.05f, 0.05f)*/, Vec3 dispToActor /*= Vec3::ZERO*/, 
					Rgba8 backgroundColor /*= Rgba8::WHITE*/, Rgba8 healthColor /*= Rgba8::WHITE*/, 
					Texture* texture /*= nullptr*/, BillboardType m_billboardType /*= BillboardType::FULL_CAMERA_FACING*/)
	: m_trackingActor(trackingActor)
	, m_size(size)
	, m_frameOffset(frameOffset)
	, m_dispToActor(dispToActor)
	, m_BGColor(backgroundColor)
	, m_BGTexture(texture)
	, m_healthColor(healthColor)
	, m_billboardType(m_billboardType)
{

}

void HealthBar::Update(Mat44 const& trackingMat)
{
	m_modelMatrix = GetBillboardMatrix(m_billboardType, trackingMat, m_trackingActor->m_position + m_dispToActor);

	m_healthBarVertexes.clear();
	m_healthBarVertexes.reserve(12);

	float BG_halfSizeX = m_size.x * 0.5f;
	float BG_halfSizeY = m_size.y * 0.5f;

	Vec3 BG_BL = Vec3(0.f, -BG_halfSizeX, -BG_halfSizeY);
	Vec3 BG_BR = Vec3(0.f, BG_halfSizeX, -BG_halfSizeY);
	Vec3 BG_TR = Vec3(0.f, BG_halfSizeX, BG_halfSizeY);
	Vec3 BG_TL = Vec3(0.f, -BG_halfSizeX, BG_halfSizeY);

	AddVertsForQuad3D(m_healthBarVertexes, BG_BL, BG_BR, BG_TR, BG_TL, m_BGColor);

	float healthFraction = m_trackingActor->m_currentHealth / m_trackingActor->m_maxHealth;

	float HB_halfSizeX = m_size.x * 0.5f * (1.f - m_frameOffset.x);
	float HB_halfSizeY = m_size.y * 0.5f * (1.f - m_frameOffset.y);

	Vec3 HB_BL = Vec3(0.f, -HB_halfSizeX, -HB_halfSizeY);
	Vec3 HB_BR = HB_BL + Vec3(0.f, HB_halfSizeX * 2.f, 0.f) * healthFraction;
	Vec3 HB_TL = Vec3(0.f, -HB_halfSizeX, HB_halfSizeY);
	Vec3 HB_TR = HB_TL + Vec3(0.f, HB_halfSizeX * 2.f, 0.f) * healthFraction;

	AddVertsForQuad3D(m_healthBarVertexes, HB_BL, HB_BR, HB_TR, HB_TL, m_healthColor);
}

void HealthBar::Render() const
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(m_BGTexture);

	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::READ_ONLY_LESS_EQUAL);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants(m_modelMatrix);
	g_theRenderer->DrawVertexArray((int)m_healthBarVertexes.size(), m_healthBarVertexes.data());
}

