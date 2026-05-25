#pragma once
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameMode.hpp"

struct AABB2_RaycastResult
{
public:
	AABB2_RaycastResult(AABB2 line)
		:m_AABB2(line)
	{}
	AABB2			m_AABB2;
	RaycastResult2D m_result;
};

class RaycastVSAABB2Mode : public GameMode
{
public:
	RaycastVSAABB2Mode();
	~RaycastVSAABB2Mode();
	void Startup() override;
	void Update(float deltaSeconds) override;
	void Render() const override;//mark for that the render is not going to change the variables
	void Shutdown() override;

	virtual void CreateRandomShapes() override;

	virtual void UpdateModeInfo() override;

	// verts management
	void AddVertsForRay();
	void AddVertsForLines();

	// line and raycast
	float m_lineThickness = 0.5f;
	float m_shortestImpactDist = 0.f;
	AABB2_RaycastResult* m_hitAABB2 = nullptr;
	void UpdateRaycastResultsForAllAABB2s();

	// ray properties
	bool  m_hasHitAABB2 = false;
	float m_arrowSize = WORLD_SIZE_Y * 0.02f;
	float m_arrowLineThickness = 0.15f;

	std::vector<AABB2_RaycastResult*> m_AABB2ResultPtrList; // store every testing discs' information
	std::vector<Vertex_PCU> m_AABB2Verts;// contain all the shapes verts

	void RenderAllAABB2s() const;
	void RenderRaycastResults() const;
};