#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <vector>
#include <string>

enum class DebugRenderGeometryType
{
	SPHERE,
	LINE,
	TEXT,
	CYLINDER,
	CONE,
	CHANGINGCONE,
	BILLBOARD,
	COUNT
};

struct DebugRenderGeometry
{
public:
	DebugRenderGeometry(DebugRenderMode renderMode, float duration, 
		Rgba8 const startColor, Rgba8 const endColor, Vec3 const position,
		DebugRenderGeometryType type);

	DebugRenderGeometry( Vec3 const position, Vec3 direction, 
		float coneRadius, float coneHeight, float duration = 0.2f,
		Rgba8 const startColor = Rgba8::WHITE, Rgba8 const endColor = Rgba8::WHITE,
		DebugRenderMode renderMode = DebugRenderMode::USE_DEPTH,
		DebugRenderGeometryType type = DebugRenderGeometryType::CHANGINGCONE);

	DebugRenderGeometry(std::string message, Vec2 position, float size, Vec2 alignment,
		float duration, Rgba8 const startColor, Rgba8 const endColor); // for on screen message

	DebugRenderGeometry(std::string message, float duration, float size, Rgba8 const startColor, Rgba8 const endColor); // for on screen debug message

	~DebugRenderGeometry() = default;

	std::vector<Vertex_PCU> m_vertices;
	Timer* m_timer;
	bool  m_showOnlyOneFrame = false; // for marking geometry that exist only one frame to delete next frame
	int	  m_framesLeft = 999999;

	Rgba8 m_startColor = Rgba8::WHITE;
	Rgba8 m_endColor = Rgba8::WHITE;
	Rgba8 m_currentColor = Rgba8::WHITE;

	DebugRenderMode m_debugRendermode = DebugRenderMode::USE_DEPTH;
	RasterizerMode  m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;

	Texture* m_texture = nullptr;

	DebugRenderGeometryType m_type = DebugRenderGeometryType::LINE;
	BillboardType m_billboardType = BillboardType::NONE;

	// Ricochet
	Vec3 m_direction;
	float m_height = 0.f;
	float m_radius = 0.f;

	Vec3 m_position;
	Vec3 velocity;
	Mat44 m_modelMatrix;

	// for screen message
	Vec2 m_textPosition;
	Vec2 m_alignment;
	float m_textSize;
	std::string m_text;


	Mat44 m_trackingTargetTransform;

	void UpdateGeometry();
};