#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <Mutex>

struct DebugRenderGeometry;

enum class DebugRenderMode
{
	ALWAYS,
	USE_DEPTH,
	X_RAY
};

struct DebugRenderConfig
{
	DebugRenderConfig() {};
	~DebugRenderConfig() {};
	DebugRenderConfig( DebugRenderConfig const& copyFrom)
	{
		m_renderer = copyFrom.m_renderer;
		m_fontName = copyFrom.m_fontName;
	}

	Renderer*	m_renderer = nullptr;
	Camera*		m_camera = nullptr;
	BitmapFont* m_font = nullptr;
	std::string m_fontName = "SquirrelFixedFont";

	int			m_numMessageOnScreen = 32;
	float		m_fontAspect = 0.6f;
	float		m_lineHeightAndTextBoxRatio = 0.9f;
};

extern std::vector<DebugRenderGeometry*>	g_debugRenderGeometries;
extern std::mutex							g_debugRenderGeometriesMutex;

extern DebugRenderConfig					g_theDebugRenderConfig;

// Setup
void DebugRenderSystemStartup(DebugRenderConfig const& config);
void DebugRenderSystemShutDown();

// Control
void SetDebugRenderVisibility( bool newVisibilityStatus );
bool GetDebugRenderVisibility();
void DebugRenderSetHidden();

// Output
void DebugRenderBeginFrame();
void DebugRenderEndFrame();
void DebugRenderWorld(const Camera& camera);
void DebugRenderWorldForVR();
void DebugRenderScreen(const Camera& camera);

// Geometry
void DebugAddWorldPoint(Vec3 const& pos,
	float radius, float duration,
	Rgba8 const& startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldLine(Vec3 const& start, Vec3 const& end,
	float radius, float duration,
	Rgba8 const& startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldWireCylinder(Vec3 const& base, Vec3 const& top,
	float radius, float duration,
	Rgba8 const& startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldWireSphere(Vec3 const& center,
	float radius, float duration,
	Rgba8 const& startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldArrow(Vec3 const& start, Vec3 const& end,
	float radius, float duration,
	Rgba8 const& startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

// VFX
// changing cone
void AddRicochetVFX(Vec3 const& impactPos, Vec3 const& reflectNormal,
	float radius, float maxConeHeight, float duration = 0.2f,
	Rgba8 const& startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
// multiple random direction cylinder
void AddShrapnelVFX(Vec3 const& impactPos, Vec3 const& reflectNormal, float speed,
	float radius, float maxConeHeight, float duration = 0.2f,
	Rgba8 const& startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

// fixed text box in the world
void DebugAddWorldText(std::string const& text,
	Mat44 const& transform, float textHeight,
	float duration, Vec2 const& alignment = Vec2(0.5f, 0.5f),
	Rgba8 const startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
// a self rotation text box in the world
void DebugAddWorldBillboardText(std::string const& text,
	Vec3 const& origin, float textHeight,
	Vec2 const& alignment, float duration,
	BillboardType const& type = BillboardType::FULL_CAMERA_FACING,
	Rgba8 const startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldBasis(Mat44 const& transform, float duration, float arrowRadius = 0.12f,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH, float length = 1.f);

void DebugAddScreenText(std::string const& text,
	Vec2 const& position, float size,
	Vec2 const& alignment, float duration,
	Rgba8 const& startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE);
void DebugAddMessage(std::string const& text,
	float duration,
	float size = 10.f,
	Rgba8 const& startColor = Rgba8::WHITE,
	Rgba8 const& endColor = Rgba8::WHITE);

// console commands
bool Command_DebugRenderClear(EventArgs& args);
bool Command_DebugRenderToggle(EventArgs& args);



