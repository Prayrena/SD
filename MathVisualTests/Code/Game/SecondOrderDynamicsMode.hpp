#pragma once
#include "Engine/Math/Splines.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameMode.hpp"
#include <unordered_map>

class SecondOrderDynamics;

enum class Curve: unsigned char
{
	SECOND_ORDER_DYNAMICS,
	PLAYER_INPUT,
	NUM_CURVE
};

constexpr int NUM_CURVES = 2;

class SecondOrderDynamicsMode : public GameMode
{
public:
	SecondOrderDynamicsMode();
	~SecondOrderDynamicsMode();
	void Startup() override;
	void AddVertsForDiagramBox();

	void Update(float deltaSeconds) override;
	void UpdateDebugMessages();
	
	void UpdateInput();
	void UpdateInputCurve();
	void UpdateSecondOrderCurve();

	void RecordRFZValueAndRecreateSecondOrderDynamics();
	void RebuildSecondOrderCurve();

	void Render() const override;
	void RenderDebug() const override;

	void RenderMovingPoints() const;
	void RenderCurves() const;

	void Shutdown() override;

	SecondOrderDynamics*	    m_secondOrderModifier = nullptr;
	float						m_changeRate = 0.02f;

	SamplePointsCurve3D			m_curves[NUM_CURVES] = {};
	std::vector<Vertex_PCU> m_playerInputCurveVerts;
	std::vector<Vertex_PCU> m_secondOrderCurveVerts;

	float	m_moveSpeed = 5.f;
	float	m_speedChangeRate = 1.f;
	float	m_currentVelocity = 0.f;
	Vec3	m_currentPos = Vec3::ZERO;
	std::vector<Vec3> m_recordedPositions;

	// display info
	virtual void UpdateModeInfo() override;

	AABB2 m_diagramBox;
	AABB2 m_diagramNameBox;

	int m_numCurveSections = 64;
	int m_numSubdivisionSections = 2;
	float m_smoothLineThickness = 0.25f;
	float m_controlLineThickness = 0.3f;
	float m_closestDist = 3.f;
	float m_controlPtRadius = 0.6f;
	float m_movingPtRadius = 0.9f;
	float m_arrowSize = 1.2f;
	float m_arrowLineThickness = 0.3f;

	// the green dot speed
	float m_fixedSpeedOnEasing = 0.f;
	float m_fixedSpeedOnCubicBezier = 0.f;
	float m_fixedSpeedOnCatmull = 0.f;

	Rgba8 m_smoothCurveColor = Rgba8::GREEN_TRANSPARENT;
	Rgba8 m_secondOrderCurveColor = Rgba8::CANDLE_YELLOW_TRANSPARENT;
	Rgba8 m_polyLineColor = Rgba8::GREEN;
	Rgba8 m_debugBoxColor = Rgba8::GRAY;
	Rgba8 m_controlPtColor = Rgba8::BLUE_MVTHL;
	Rgba8 m_controlLineColor = Rgba8::BLUE_MVT;
	Rgba8 m_arrowColor = Rgba8::RED;
	Rgba8 m_parametricallyMovingPtColor = Rgba8::WHITE;

	Rgba8 m_fixedSpeedMovingPtColor = Rgba8::GREEN;
	Rgba8 m_subdivisionSplineColor = Rgba8::GREEN;

	std::vector<Vertex_PCU> m_debugVerts;

	std::vector<Vertex_PCU> m_parametricallyMovingPtVerts;
	std::vector<Vertex_PCU> m_controlPtVerts;

	// the points that green dots need to run through
	std::vector<Vec2> m_samplePtsForEasing;
	std::vector<Vec2> m_samplePtsForCubicBezier;
	std::vector<Vec2> m_samplePtsForCatmullRom;

	float m_stringHeight = 3.9f;
	std::string m_easingDiagramName;

	Timer* m_inputTimer = nullptr;
	Timer* m_samplerTimer = nullptr;

	std::vector<std::pair<float, float>> m_deltatime_velocity_recording;
	float	m_recordingTimeOwned = 0.f;
	float	m_recordingFixedTimestep = (1.f / 60.f);

	float m_movingDuration = 2.f; // for each section
	float m_sampleDuration = 0.03f; // for each section
};