#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Engine/Math/Easing.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

DebugRenderGeometry::DebugRenderGeometry(DebugRenderMode renderMode, float duration,
	Rgba8 const startColor, Rgba8 const endColor, Vec3 const position,
	DebugRenderGeometryType type)
	: m_debugRendermode(renderMode)
	, m_startColor(startColor)
	, m_endColor(endColor)
	, m_position(position)
	, m_type(type)
{
	m_timer = new Timer();
	m_timer->m_period = duration;
	m_timer->Start();
	if (duration == 0.f)
	{
		m_showOnlyOneFrame = true;
		m_framesLeft = 1;
	}

	m_currentColor = m_startColor;
}

DebugRenderGeometry::DebugRenderGeometry(std::string message, Vec2 position, float size, Vec2 alignment, float duration, Rgba8 const startColor, Rgba8 const endColor)
	:m_debugRendermode(DebugRenderMode::ALWAYS)
	, m_text(message)
	, m_textPosition(position)
	, m_position(position)
	, m_textSize(size)
	, m_alignment(alignment)
	, m_startColor(startColor)
	, m_endColor(endColor)
{
	m_debugRendermode = DebugRenderMode::ALWAYS;
	m_type = DebugRenderGeometryType::TEXT;

	// timer
	m_timer = new Timer();
	m_timer->m_period = duration;
	m_timer->Start();
	if (duration == 0.f)
	{
		m_showOnlyOneFrame = true;
		m_framesLeft = 1;
	}

	m_currentColor = m_startColor;
}

DebugRenderGeometry::DebugRenderGeometry(std::string message, float duration, float size, Rgba8 const startColor, Rgba8 const endColor)
	: m_text(message)
	, m_textSize(size)
	, m_startColor(startColor)
	, m_endColor(endColor)
{
	m_debugRendermode = DebugRenderMode::ALWAYS;
	m_type = DebugRenderGeometryType::TEXT;

	// timer
	m_timer = new Timer();
	m_timer->m_period = duration;
	m_timer->Start();
	if (duration == 0.f)
	{
		m_showOnlyOneFrame = true;
		m_framesLeft = 1;
	}

	m_currentColor = m_startColor;
}

DebugRenderGeometry::DebugRenderGeometry(Vec3 const position, Vec3 direction, float coneRadius, float coneHeight, float duration /*= 0.2f*/, 
	Rgba8 const startColor /*= Rgba8::WHITE*/, Rgba8 const endColor /*= Rgba8::WHITE*/, DebugRenderMode renderMode /*= DebugRenderMode::USE_DEPTH*/, 
	DebugRenderGeometryType type /*= DebugRenderGeometryType::CHANGINGCONE*/)
	: m_position(position)
	, m_direction(direction)
	, m_radius(coneRadius)
	, m_height(coneHeight)
	, m_debugRendermode(renderMode)
	, m_startColor(startColor)
	, m_endColor(endColor)
	, m_type(type)
{
	m_timer = new Timer();
	m_timer->m_period = duration;
	m_timer->Start();
}

void DebugRenderGeometry::UpdateGeometry()
{
	float lifeTimeFraction;
	if (m_timer->m_period <= 0.f)
	{
		lifeTimeFraction = 0.f;
	}
	else
	{
		lifeTimeFraction = m_timer->GetElapsedFraction();
	}

	if (m_startColor != m_endColor)
	{
		m_currentColor = InterpolateRGBA(m_startColor, m_endColor, lifeTimeFraction);

		// color shifting
		for (int i = 0; i < (int)m_vertices.size(); ++ i)
		{
			m_vertices[i].m_color = m_currentColor;
		}
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (m_showOnlyOneFrame)
	{
		--m_framesLeft;
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// if the geometry has a billboard type, it is going to update all the vertices every update
	if (m_billboardType != BillboardType::NONE)
	{
		m_modelMatrix = GetBillboardMatrix(m_billboardType, m_trackingTargetTransform, m_position);
	}

	if (m_type == DebugRenderGeometryType::CHANGINGCONE)
	{
		m_vertices.clear();

		// get the height of the cone
		// the first half part is increasing
		// the second half part is decreasing
		float fraction = m_timer->GetElapsedFraction();
		fraction = EaseOutCubic(fraction);
		float coneHeight = Interpolate(0.f, m_height, fraction);
		float coneRadius = Interpolate(0.f, m_radius, fraction);

		Vec3 endPos = m_position + m_direction * coneHeight;
		AddVertsForCone3D(m_vertices, m_position, endPos, coneRadius, m_currentColor);
	}
}
