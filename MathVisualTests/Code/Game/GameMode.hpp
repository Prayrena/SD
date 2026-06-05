#pragma once
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include <string>

class Renderer;

enum TestingScene
{
	// App starts static_cast<TestingScene>(0), so keep the default startup mode first.
	JOB_SYSTEM_AND_THREADS,
	HEATMAP_ASSESSMENT,
	RAYCAST2D_VS_CONVEX2S,
	SECOND_ORDER_DYNAMICS,
	QUATERNION,
	RAYCAST_VS_3DSHAPES,
	PACHINKO,
	CURVES2D,
	RAYCAST2D_VS_AABB2S,
	RAYCAST2D_VS_LINESEGMENTS,
	RAYCAST2D_VS_DISCS,
	GET_NEARESTPOINT,
	NUM_TESTINGMODES
};

struct GameModeConfig
{
	int			m_numMessageOnScreen = 32;
	float		m_lineHeightAndTextBoxRatio = 0.8f;
	float		m_cellAspect = 0.6f;
	BitmapFont* m_font = nullptr;
	Renderer* m_renderer = nullptr;
};

class GameMode
{
public:
	GameMode();
	virtual ~GameMode();
	virtual void Startup() = 0;
	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;
	virtual void RenderDebug() const;
	virtual void Shutdown() = 0;

	virtual void CreateRandomShapes();

	virtual void UpdateModeInfo() = 0;
	virtual void RenderScreenMessage() const;

	static GameMode* CreateNewGame(TestingScene type);

public:
	// reference raycast control
	void ControlTheReferenceRay(float deltaSeconds);

	void UpdateMouseInfo();
	Vec2 GetMousePositionInWorld();
	Vec2 GetMouseDispThisFrame();
	Vec2 m_mousePosLastFrame = Vec2::ZERO;
	Vec2 m_mousePosCurrentFrame = Vec2::ZERO;

	// ray properties
	Vec2 m_tailPos;		// the other end
	Vec2 m_tipPos;		// the arrow head

	// camera setting
	Camera m_worldCamera;
	Camera m_screenCamera;

	// mode info
	std::string m_modeName;
	std::string m_controlInstruction;
	std::string m_testString;
	Rgba8	m_modeNameLineColor = Rgba8::Naples_Yellow;
	Rgba8	m_instructionLineColor = Rgba8::CYAN;
	Rgba8	m_testLineColor = Rgba8::CYAN;
	GameModeConfig m_gameModeConfig;

	// verts
	std::vector<Vertex_PCU> m_rayVerts;// contain all the reference point and nearest points
};
