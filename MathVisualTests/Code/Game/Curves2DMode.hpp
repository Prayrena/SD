#pragma once
#include "Engine/Math/Splines.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameMode.hpp"

enum class DiagramIndex
{
	ElevateBounce,
	Indentity,
	EaseInQuadratic,
	EaseInCubic,
	EaseInQuartic,
	EaseInQuintic,
	EaseInHexic,
	EaseOutQuadratic,
	EaseOutCubic,
	EaseOutQuartic,
	EaseOutQuintic,
	EaseOutHexic,
	SmoothStep3,
	SmoothStep5,
	Hesitate3,
	Hesitate5,
	CustomFunkyEasingFunction,
	NumEasingDiagrams
};

class Curves2DMode : public GameMode
{
public:
	Curves2DMode();
	~Curves2DMode();
	void Startup() override;

	void Update(float deltaSeconds) override;
	void UpdateInput();
	void UpdateMovingPointsOnEasingDiagram(float t);
	void UpdateCubicBezierDiagram(float t);
	void UpdateCatmullRomDiagram();

	void Render() const override;
	void RenderDebug() const override;
	void RenderEasingDiagram() const;
	void RenderCubicBezierDiagram() const;
	void RenderCatmullRomDiagram() const;
	void RenderMovingPoints() const;
	void RenderSubdivisionSpline() const;
	void RenderControlPoints() const;
	void RenderDiagramName() const;

	void Shutdown() override;

	// adding shapes
	virtual void CreateRandomShapes() override;
	void AddVertsForEasingDiagram();
	void AddVertsForAllSubdivisionCurves();

	std::vector<Vec2> AddVertsForEasingDiagram(std::vector<Vertex_PCU>& listToAddVerts, DiagramIndex diagramIndex, Vec2 start, Vec2 end,
		Rgba8 color = Rgba8::GRAY, int numSection = 64);
	std::vector<Vec2> AddVertsForCubicBezierCurve(std::vector<Vertex_PCU>& listToAddVerts, Rgba8 color = Rgba8::GRAY, int numSection = 64);
	std::vector<Vec2> AddVertsForCubicCatmullRomCurve(std::vector<Vertex_PCU>& listToAddVerts, int numControlPts, Rgba8 color = Rgba8::GRAY, int numSection = 64);
	void AddVertsForControlPointsAndControlLines(std::vector<Vec2> pts, std::vector<Vertex_PCU>& listToAddVerts);
	void AddVertsForVelocityAtControlledPoints();

	// display info
	virtual void UpdateModeInfo() override;

	int m_cubicBezierSections = 64;
	CubicBezierCurve2D* m_cubicBezierCurve = nullptr;
	int m_CatmullRamSections = 128;
	CatmullRomSpline2D* m_CatmullRomCurve = nullptr;

	AABB2 m_easingBox;
	AABB2 m_CubicBezierBox;
	AABB2 m_CatmullRomBox;
	AABB2 m_diagramNameBox;

	int m_easingDiagramIndex = 0;
	int m_numCurveSections = 64;
	int m_numSubdivisionSections = 2;
	float m_smoothLineThickness = 0.45f;
	float m_controlLineThickness = 0.3f;
	float m_closestDist = 3.f;
	float m_controlPtRadius = 0.6f;
	float m_movingPtRadius = 0.9f;
	float m_arrowSize = 1.2f;
	float m_arrowLineThickness = 0.3f;
	float m_fractionOnCatmullRom = 0.f;
	float m_pastTimeOnCatmullRom = 0.f;

	// the green dot speed
	float m_fixedSpeedOnEasing = 0.f;
	float m_fixedSpeedOnCubicBezier = 0.f;
	float m_fixedSpeedOnCatmull = 0.f;

	Rgba8 m_smoothCurveColor = Rgba8::GRAY;
	Rgba8 m_polyLineColor = Rgba8::GREEN;
	Rgba8 m_debugBoxColor = Rgba8::RED;
	Rgba8 m_controlPtColor = Rgba8::BLUE_MVTHL;
	Rgba8 m_controlLineColor = Rgba8::BLUE_MVT;
	Rgba8 m_arrowColor = Rgba8::RED;
	Rgba8 m_parametricallyMovingPtColor = Rgba8::WHITE;

	Rgba8 m_fixedSpeedMovingPtColor = Rgba8::GREEN;
	Rgba8 m_subdivisionSplineColor = Rgba8::GREEN;

	std::vector<Vertex_PCU> m_easingVerts;
	std::vector<Vertex_PCU> m_cubicBezierVerts;
	std::vector<Vertex_PCU> m_CatmullRomVerts;
	std::vector<Vertex_PCU> m_debugVerts;
	std::vector<Vertex_PCU> m_subdivisionSplineVerts;
	std::vector<Vertex_PCU> m_easingFunctionVerts;
	std::vector<Vertex_PCU> m_parametricallyMovingPtVerts;
	std::vector<Vertex_PCU> m_controlPtVerts;

	// the points that green dots need to run through
	std::vector<Vec2> m_samplePtsForEasing;
	std::vector<Vec2> m_samplePtsForCubicBezier;
	std::vector<Vec2> m_samplePtsForCatmullRom;

	float m_stringHeight = 3.9f;
	std::string m_easingDiagramName;

	Timer* m_controlPtTimer = nullptr;
	float m_movingDuration = 2.f; // for each section
};