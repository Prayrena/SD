#include "Game/RaycastVSLineSegmentsMode.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/App.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"

extern App* g_theApp;
extern InputSystem* g_theInput;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;

extern Clock* g_theGameClock;

RaycastVsLineSegmentMode::RaycastVsLineSegmentMode()
	:GameMode()
{

}

RaycastVsLineSegmentMode::~RaycastVsLineSegmentMode()
{

}

void RaycastVsLineSegmentMode::Startup()
{
	g_theInput->SetCursorMode(false, false);

	CreateRandomShapes();

	// set the cameras
	AABB2 cameraStart(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	//cameraStart.SetDimensions(Vec2(100.f, 50.f));
	m_worldCamera.SetOrthoView(cameraStart);
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	m_tailPos = 0.3f * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);
	m_tipPos = 0.7f * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);
}

void RaycastVsLineSegmentMode::Update(float deltaSeconds)
{
	// reset list
	m_rayVerts.clear();
	m_hitLine = nullptr;
	m_hasHitLine = false;

	// update ray cast
	ControlTheReferenceRay(deltaSeconds);
	UpdateRaycastResultsForAllLineSegments();

	AddVertsForLines();
	AddVertsForRay();

	UpdateModeInfo();
}

void RaycastVsLineSegmentMode::Render() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	RenderAllLines();
	RenderRaycastResults();

	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void RaycastVsLineSegmentMode::Shutdown()
{

}

void RaycastVsLineSegmentMode::CreateRandomShapes()
{
	m_lineSegVerts.clear();
	m_lineRayPtrList.clear();
	int numLines = g_rng->RollRandomIntInRange(10, 12);
	float maxLength = WORLD_SIZE_Y * 0.6f;
	float minLength = WORLD_SIZE_Y * 0.05f;

	m_lineRayPtrList.resize(numLines);
	m_lineSegVerts.resize(numLines * 4);

	for (int i = 0; i < numLines; ++i)
	{
		Vec2 lineStart;
		lineStart.x = g_rng->RollRandomFloatInRange(0, WORLD_SIZE_X);
		lineStart.y = g_rng->RollRandomFloatInRange(0, WORLD_SIZE_Y);
		
		float lineLength = g_rng->RollRandomFloatInRange(minLength, maxLength);
		float orientationDegree = g_rng->RollRandomFloatInRange(0.f, 360.f);
		float orientationRadian = ConvertDegreesToRadians(orientationDegree);
		Vec2 lineEnd = Vec2(lineLength * cosf(orientationRadian), lineLength * sinf(orientationRadian)) + lineStart;

		LineSegment2* line = new LineSegment2(lineStart, lineEnd);
		Line_RaycastResult* newLine_Ray = new Line_RaycastResult(*line);
		m_lineRayPtrList[i] = newLine_Ray;
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void RaycastVsLineSegmentMode::AddVertsForRay()
{
	m_rayVerts.clear();

	if (m_hasHitLine)
	{
		Vec2 impactPos = m_hitLine->m_result.m_impactPos;
		Vec2 reflectNormalEndPoint = impactPos + (m_hitLine->m_result.m_impactNormal) * WORLD_SIZE_Y * 0.0015f;

		AddVertsForArrow2D(m_rayVerts, m_tailPos, m_tipPos, m_arrowSize, m_arrowLineThickness, Rgba8::GRAY);
		AddVertsForArrow2D(m_rayVerts, m_tailPos, impactPos, m_arrowSize, m_arrowLineThickness, Rgba8::RED);
		AddVertsForArrow2D(m_rayVerts, impactPos, reflectNormalEndPoint, m_arrowSize * 0.6f, m_arrowLineThickness, Rgba8::YELLOW);

		AddVertesForDisc2D(m_rayVerts, impactPos, WORLD_SIZE_Y * 0.005f, Rgba8::WHITE, 12);
	}
	else
	{
		// when there is no impact on disc, just show a green arrow
		AddVertsForArrow2D(m_rayVerts, m_tailPos, m_tipPos, m_arrowSize, m_arrowLineThickness, Rgba8::GREEN);
	}
}

void RaycastVsLineSegmentMode::AddVertsForLines()
{
	m_lineSegVerts.clear();

	for (int lineIndex = 0; lineIndex < (int)m_lineRayPtrList.size(); lineIndex++)
	{
		Line_RaycastResult*& line = m_lineRayPtrList[lineIndex];
		if (m_hasHitLine)
		{
			if (line->m_result.m_impactDist == m_shortestImpactDist)
			{
				AddVertsForLineSegment2D(m_lineSegVerts, line->m_line, m_lineThickness, Rgba8::BLUE_MVTHL);
				m_hitLine = line;
				continue;
			}
			else AddVertsForLineSegment2D(m_lineSegVerts, line->m_line, m_lineThickness, Rgba8::BLUE_MVT);
		}
		else AddVertsForLineSegment2D(m_lineSegVerts, line->m_line, m_lineThickness, Rgba8::BLUE_MVT);
	}
}

void RaycastVsLineSegmentMode::UpdateRaycastResultsForAllLineSegments()
{
	m_shortestImpactDist = 99999999.f;
	Vec2 rayForwardNormal = (m_tipPos - m_tailPos).GetNormalized();
	float rayDist = (m_tipPos - m_tailPos).GetLength();

	for (int lineIndex = 0; lineIndex < (int)m_lineRayPtrList.size(); lineIndex++)
	{
		Line_RaycastResult*& line = m_lineRayPtrList[lineIndex];		
		RaycastResult2D result = RaycastVSLineSegment2D(m_tailPos, rayForwardNormal, rayDist, line->m_line.m_start, line->m_line.m_end);
		line->m_result = result;
		// if hit, try to get the shortest record
		if (line->m_result.m_didImpact)
		{
			m_hasHitLine = true;
			if (result.m_impactDist < m_shortestImpactDist)
			{
				m_shortestImpactDist = result.m_impactDist;
			}
		}
	}
}

void RaycastVsLineSegmentMode::UpdateModeInfo()
{
	m_modeName = "Mode (F6 / F7 for prev / next): Raycast VS. Line Segments";
	m_controlInstruction = "LMB/RMB set ray start/end; ESDF move raycast start position, IJKL move raycast end position.\nArrow keys move the whole raycast. F8 to randomize shapes; Hold T to slow down movement";
}

void RaycastVsLineSegmentMode::RenderAllLines() const
{
	g_theRenderer->DrawVertexArray((int)m_lineSegVerts.size(), m_lineSegVerts.data());
}

void RaycastVsLineSegmentMode::RenderRaycastResults() const
{
	g_theRenderer->DrawVertexArray((int)m_rayVerts.size(), m_rayVerts.data());
}

