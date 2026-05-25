#pragma once
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameMode.hpp"

struct Line_RaycastResult
{
public:
	Line_RaycastResult(LineSegment2 line)
		:m_line(line)
	{}
	LineSegment2	m_line;
	RaycastResult2D m_result;
};

class RaycastVsLineSegmentMode : public GameMode
{
public:
	RaycastVsLineSegmentMode();
	~RaycastVsLineSegmentMode();
	void Startup() override;
	void Update(float deltaSeconds) override;
	void Render() const override;//mark for that the render is not going to change the variables
	void Shutdown() override;

	virtual void CreateRandomShapes() override;

	// verts management
	void AddVertsForRay();
	void AddVertsForLines();

	// line and raycast
	float m_lineThickness = 0.3f;
	float m_shortestImpactDist = 0.f;
	Line_RaycastResult* m_hitLine = nullptr;
	void UpdateRaycastResultsForAllLineSegments();

	virtual void UpdateModeInfo() override;

	// ray properties
	bool m_hasHitLine = false;
	float m_arrowSize = WORLD_SIZE_Y * 0.02f;
	float m_arrowLineThickness = 0.15f;

	std::vector<Line_RaycastResult*> m_lineRayPtrList; // store every testing discs' information
	std::vector<Vertex_PCU> m_lineSegVerts;// contain all the shapes verts

	void RenderAllLines() const;
	void RenderRaycastResults() const;
};