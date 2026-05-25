#include "Game/RaycastVSConvex2Mode.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/App.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/core/Time.hpp"

extern App* g_theApp;
extern InputSystem* g_theInput;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;
extern Clock* g_theGameClock;

using namespace std;

// for bit bucket, we are doing division by 8 x 4
constexpr float BIT_BUCKET_REGION_SIZE = 25.f; // 200/8 = 100/4 = 25

ConvexObj::ConvexObj(Disc2& disc, int convexIndex)
{
	float totalAngle = 360.f;
	float startAngle = 0.f;
	vector<Vec2> tempConvexVertices;
	
	while (totalAngle > MAX_ANGLESTEP)
	{
		float angle = g_rng->RollRandomFloatInRange(MIN_ANGLESTEP, MAX_ANGLESTEP);
		totalAngle -= angle;
		startAngle += angle;

		Vec2 offset = Vec2::MakeFromPolarDegrees(startAngle, disc.m_radius);
		Vec2 convexVertice = disc.m_center + offset;

		tempConvexVertices.push_back(convexVertice);
	}

	RaycastVSConvex2Mode* scene = static_cast<RaycastVSConvex2Mode*>(g_theApp->m_currentGameMode);

	// use emplace_back so we don't need new and delete
	// ConvexPoly2* convexPoly = new ConvexPoly2(tempConvexVertices);
	// ConvexHull2* convexHull = new ConvexHull2(*convexPoly);
	scene->m_convexPolys.emplace_back(ConvexPoly2(tempConvexVertices));
	scene->m_convexHulls.emplace_back(scene->m_convexPolys.back());

	m_convexPolyIndex = convexIndex;
	m_convexHullIndex = convexIndex;
	m_discIndex = convexIndex;

	// record how many num vertices we have accumulated from the first convex
	// easy for later render part of the convexes on screen
	int numVertices = (int)tempConvexVertices.size();
	if (convexIndex != 0)
	{
		scene->m_numConvexVertices[convexIndex] = scene->m_numConvexVertices[convexIndex-1] + (numVertices - 2) * 3;
		scene->m_numLineSegmentsVertices[convexIndex] = scene->m_numLineSegmentsVertices[convexIndex-1] + numVertices * 6;
		scene->m_numDiscVertices[convexIndex] = scene->m_numDiscVertices[convexIndex-1] + scene->m_discNumSegments * 6;
	}
	else
	{
		scene->m_numConvexVertices[0] = (numVertices - 2) * 3;
		scene->m_numLineSegmentsVertices[0] = numVertices * 6;
		scene->m_numDiscVertices[0] = scene->m_discNumSegments * 6;
	}
}

ConvexObj::ConvexObj(int polyIndex, int hullIndex, int discIndex)
		: m_convexPolyIndex(polyIndex)
		, m_convexHullIndex(hullIndex)
		, m_discIndex(discIndex)
{

}

RaycastVSConvex2Mode::RaycastVSConvex2Mode()
	:GameMode()
{

}

RaycastVSConvex2Mode::~RaycastVSConvex2Mode()
{

}

void RaycastVSConvex2Mode::Startup()
{
	g_theInput->SetCursorMode(false, false);

	CreateRandomShapes();
	UpdateModeInfo();

	// set the cameras
	AABB2 cameraStart(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	//cameraStart.SetDimensions(Vec2(100.f, 50.f));
	m_worldCamera.SetOrthoView(cameraStart);
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	m_tailPos = 0.3f * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);
	m_tipPos = 0.7f * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);

	m_modeNameLineColor = Rgba8::NEON_BLUE;
	m_instructionLineColor = Rgba8::GREENISH_GRAY;
	m_testLineColor = Rgba8::GREENISH_GRAY;

	// TestBufferWriterAndParser();
	// TestLoadingExampleGHCSSaveFile();
}

void RaycastVSConvex2Mode::Update(float deltaSeconds)
{
	UpdateMouseInfo();
	UpdateModeInfo();

	ControlConvexNumsOnScreen();

	// reset ray
	UpdateRaycast2D();
	m_rayVerts.clear();
	m_hitConvex = nullptr;
	// m_hasHitConvex = false;

	if (!m_convexObjs.empty())
	{
		AddVertsForConvexHullBoundingLines();
	}

	// update ray cast
	UpdateCursorState();
	ControlTheReferenceRay(deltaSeconds);

	ResetGlobalRaycastTobeMissingResult();
	UpdateRaycastResultsForAllConvex();
	// AddVertsForLines();
	AddVertsForRay();
}

void RaycastVSConvex2Mode::Render() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8::IVORY_YELLOW);

	RenderAllConvex();
	RenderRaycastResults();
	RenderBoundingLineAndImpactPos();

	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void RaycastVSConvex2Mode::Shutdown()
{
	if (m_convexVertexBuffer)
	{
		delete m_convexVertexBuffer;
	}
}

void RaycastVSConvex2Mode::CreateRandomShapes()
{
	// clear shapes if the old version exists
	if (!m_convexObjs.empty())
	{
		m_convexObjs.clear();
		m_convexObjs.shrink_to_fit();	// might not work based on compability issue
		m_discs.clear();
		m_discs.shrink_to_fit();
		m_convexPolys.clear();
		m_convexPolys.shrink_to_fit();
		m_convexHulls.clear();
		m_convexHulls.shrink_to_fit();
	}

	for (int i = 0; i < NUM_TOTALCONVEX; ++i)
	{
		Vec2 discCenter;
		discCenter.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.05f, WORLD_SIZE_X * 0.9f);
		discCenter.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.05f, WORLD_SIZE_Y * 0.9f);

		float radius = g_rng->RollRandomFloatInRange(MIN_RADIUS, MAX_RADIUS);
		m_discs.emplace_back(Disc2(radius, discCenter));

		m_convexObjs.emplace_back(ConvexObj(m_discs.back(), i));
	}

	if (!m_discVerts.empty())
	{
		m_discVerts.clear();
	}

	if (m_discVertexBuffer)
	{
		delete m_discVertexBuffer;
		m_discVertexBuffer = nullptr;
	}

	AddVertsForAllConvexAndCreateVertexBuffer();
}

void RaycastVSConvex2Mode::UpdateModeInfo()
{
	m_modeName = "Raycast VS. Convex2D Mode (F6 / F7 for prev / next): W/R=Rotate, L/K=Scale, F2=DrawMode, F4=ShowDiscs, F8=Randomize";
	m_controlInstruction = Stringf("%i convex shapes(N/M to halve/double); T = Test with %i random rays (Y/U to halve/double)", m_numConvexShownOnScreen, m_numRays);

	if (m_bitMaskDuration != 0)
	{
		m_testString = Stringf("%i test rays vs. %i objects \nBit Bucket: took %.2fms (avg impact dist= %.2f) \nCompare: took %.2fms(avg impact dist= %.2f)", m_numRays, m_numConvexShownOnScreen, m_bitMaskDuration * 100.f, m_bitBucketAverageImpactDist, m_discCheckDuration * 100.f, m_discCheckAverageImpactDist);
	}
}

void RaycastVSConvex2Mode::ControlConvexNumsOnScreen()
{
	// half/double the number of convex shown on screen
	if (g_theInput->WasKeyJustPressed('N') && m_numConvexShownOnScreen != 1)
	{
		m_numConvexShownOnScreen = m_numConvexShownOnScreen >> 1;
	}

	if (g_theInput->WasKeyJustPressed('M') && m_numConvexShownOnScreen != NUM_TOTALCONVEX)
	{
		m_numConvexShownOnScreen = m_numConvexShownOnScreen << 1;
	}

	if (g_theInput->WasKeyJustPressed('Y') && m_numRays != 1)
	{
		m_numRays = m_numRays >> 1;
	}

	if (g_theInput->WasKeyJustPressed('U') && m_numRays != NUM_MAXNUMRAYS)
	{
		m_numRays = m_numRays << 1;
	}

	// change display mode
	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
	{
		if (m_opaqueMode)
		{
			m_opaqueMode = false;
		}
		else
		{
			m_opaqueMode = true;
		}
	}

	// show discs
	if (g_theInput->WasKeyJustPressed(KEYCODE_F4))
	{
		if (m_showDiscs)
		{
			m_showDiscs = false;
		}
		else
		{
			m_showDiscs = true;
		}
	}

	// randomize
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		CreateRandomShapes();
	}

	// randomize
	if (g_theInput->WasKeyJustPressed('T'))
	{
		TestGitBucketSpacePartition();
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void RaycastVSConvex2Mode::AddVertsForRay()
{
	m_rayVerts.clear();

	if (m_raycastResult.m_didImpact)
	{
		Vec2 impactPos = m_raycastResult.m_impactPos;
		Vec2 exitPos = m_raycastResult.m_exitPos;
		Vec2 reflectNormalEndPoint = impactPos + (m_raycastResult.m_impactNormal) * WORLD_SIZE_Y * 0.05f;
		Vec2 exitNormalEndPoint = exitPos + (m_raycastResult.m_exitNormal) * WORLD_SIZE_Y * 0.05f;

		// for the impact pos
		AddVertsForArrow2D(m_rayVerts, m_tailPos, m_tipPos, m_arrowSize, m_arrowLineThickness, Rgba8::GRAY_Dark);

		if (shortestImpactDist != 0.f)
		{
			AddVertsForArrow2D(m_rayVerts, m_tailPos, impactPos, m_arrowSize * 0.6f, m_arrowLineThickness * 2.f, Rgba8::GREEN);
			AddVertesForDisc2D(m_rayVerts, impactPos, WORLD_SIZE_Y * 0.009f, Rgba8::RED, 12);
		}
		AddVertsForArrow2D(m_rayVerts, impactPos, reflectNormalEndPoint, m_arrowSize * 0.6f, m_arrowLineThickness, Rgba8::RED);
		AddVertesForDisc2D(m_rayVerts, impactPos, WORLD_SIZE_Y * 0.005f, Rgba8::WHITE, 12);

		// for the exit pos
		if (m_raycastResult.m_didExit)
		{
			AddVertesForDisc2D(m_rayVerts, exitPos, WORLD_SIZE_Y * 0.005f, Rgba8::GREEN, 12);
			AddVertsForArrow2D(m_rayVerts, exitPos, exitNormalEndPoint, m_arrowSize * 0.6f, m_arrowLineThickness, Rgba8::LIGHT_ORANGE);
		}
		// else
		// {
		// 	AddVertesForDisc2D(m_rayVerts, exitPos, WORLD_SIZE_Y * 0.005f, Rgba8::RED, 12);
		// }
	}
	else
	{
		// when there is no impact, just show a gray arrow
		AddVertsForArrow2D(m_rayVerts, m_tailPos, m_tipPos, m_arrowSize, m_arrowLineThickness, Rgba8::GRAY_Dark);
	}
}

void RaycastVSConvex2Mode::AddVertsForAllConvexAndCreateVertexBuffer()
{
	// store all the convex vertices
	if (!m_convexVerts.empty())
	{
		m_convexVerts.clear();
		m_convexVerts.shrink_to_fit();
		m_lineSegmentVerts.clear();
		m_lineSegmentVerts.shrink_to_fit();
		// m_discVerts.clear();
		// m_discVerts.shrink_to_fit();
	}
	m_convexVerts.reserve(m_numConvexVertices[NUM_TOTALCONVEX - 1]);
	m_lineSegmentVerts.reserve(m_numLineSegmentsVertices[NUM_TOTALCONVEX - 1]);

	if (m_discVerts.empty())
	{
		m_discVerts.reserve(m_numDiscVertices[NUM_TOTALCONVEX - 1]);
	}

	// add verts for all convex
	for (ConvexObj const& convex : m_convexObjs) // need to use &, otherwise convex is a copy of each element in m_convexObjs
	{
		if (m_touchingConvex == &convex)	// then &convex will be the address of the temporary copy, not the actual object stored in m_convexObjs
		{
			AddVertsForConvexPoly2(m_convexVerts, m_convexPolys[convex.m_convexPolyIndex], Rgba8::GRAPEFRUIT_PINK_TRANSPARENT);
		}
		else
		{
			AddVertsForConvexPoly2(m_convexVerts, m_convexPolys[convex.m_convexPolyIndex], Rgba8::PALE_PINK_TRANSPARENT);
		}

		// for each edges add line segments
		int  numVertices = (int)m_convexPolys[convex.m_convexPolyIndex].GetVertexPos().size();
		vector<Vec2> vertices = m_convexPolys[convex.m_convexPolyIndex].GetVertexPos();
		for (int lineIndex = 0; lineIndex < numVertices; ++lineIndex)
		{
			if (lineIndex == (numVertices - 1))
			{
				AddVertsForLineSegment2D(m_lineSegmentVerts, vertices[lineIndex], vertices[0], m_convexEdgeThickness, m_convexEdgeColor);
			}
			else
			{
				AddVertsForLineSegment2D(m_lineSegmentVerts, vertices[lineIndex], vertices[lineIndex+1], m_convexEdgeThickness, m_convexEdgeColor);
			}
		}

		// for each disc, add verts
		if (!m_discVertexBuffer)
		{
			AddVertesForRing2D(m_discVerts, m_discs[convex.m_discIndex].m_center, m_discs[convex.m_discIndex].m_radius, m_boundingLineThickness * 0.5f, Rgba8::BRIGHT_ORANGE, m_discNumSegments);
		}
	}


	// create the vertex buffer for all the convex
	if (m_convexVertexBuffer)
	{
		delete m_convexVertexBuffer;
		m_convexVertexBuffer = nullptr;

		delete m_lineSegmentVertexBuffer;
		m_lineSegmentVertexBuffer = nullptr;		
		
		delete m_discVertexBuffer;
		m_discVertexBuffer = nullptr;
	}
	m_convexVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_convexVerts.size()), sizeof(Vertex_PCU));

	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArrayDataSize = (m_convexVerts.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_convexVerts.data(), vertexArrayDataSize, m_convexVertexBuffer);

	m_lineSegmentVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_lineSegmentVerts.size()), sizeof(Vertex_PCU));
	vertexArrayDataSize = (m_lineSegmentVerts.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_lineSegmentVerts.data(), vertexArrayDataSize, m_lineSegmentVertexBuffer);


	m_discVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_discVerts.size()), sizeof(Vertex_PCU));
	vertexArrayDataSize = (m_discVerts.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_discVerts.data(), vertexArrayDataSize, m_discVertexBuffer);
}

void RaycastVSConvex2Mode::AddVertsForConvexHullBoundingLines()
{
	// Show ConvexHull planes when there is only one convex shown in scene
	if (m_numConvexShownOnScreen == 1)
	{
		m_boundingLinesVerts.clear();

		// from every neighbor vertices on the convex, we are going to add verts for a infinite line
		vector<Vec2> const& vertices = m_convexPolys[0].GetVertexPos();
		int numLines = (int)vertices.size();

		Vec2 raycastDir = m_ray.rayFwdNormal;
		Raycast2D extendRay = m_ray;
		extendRay.rayDist += 9999.f;

		for (int lineIndex = 0; lineIndex < numLines; ++lineIndex)
		{
			Vec2 v0 = vertices[lineIndex];
			Vec2 v1;
			if (lineIndex + 1 == numLines)
			{
				v1 = vertices[0];
			}
			else
			{
				v1 = vertices[lineIndex + 1];
			}

			// we are going to extend the start and end of the line
			Vec2 disp = v1 - v0;
			float radian_V0_v1 = atan2f(disp.y, disp.x);
			float radian_v1_v0 = radian_V0_v1 + PI;
			Vec2 inf_end = Vec2::MakeFromPolarRadians(radian_V0_v1, 999.f) + v1;
			Vec2 inf_start = Vec2::MakeFromPolarRadians(radian_v1_v0, 999.f) + v0;

			// see if the plane is in the same direction with ray
			// opposite direction with ray, whether it is then show impact pos
			// or is hit by ray
			Plane2* plane = &m_convexHulls[0].m_boundingPlanes[lineIndex];
			Rgba8 lineColor;
			RaycastResult2D raycastResult = RaycastVSPlane2(extendRay, *plane);
			if (DotProduct2D(plane->m_normal, raycastDir) < 0)	
			{
				if (raycastResult.m_didImpact)
				{
					lineColor = m_boundingLine_hit;
				}
				else
				{
					lineColor = m_boundingLine_oppDir;
				}
			}
			else
			{
				lineColor = m_boundingLine_sameDir;
			}

			// add verts for line and disc
			AddVertsForLineSegment2D(m_boundingLinesVerts, inf_start, inf_end, m_boundingLineThickness, lineColor);

			// we add arrow on the mid point to show if we messed up with normal direction
			Vec2 midPt = (inf_start + inf_end) * 0.5f;
			AddVertsForArrow2D(m_boundingLinesVerts, midPt, midPt + plane->m_normal * 3.f, m_arrowSize * 0.3f, m_arrowLineThickness * 0.5f, lineColor);

			if (raycastResult.m_didImpact)
			{
				if (DotProduct2D(plane->m_normal, raycastDir) < 0)
				{
					AddVertesForDisc2D(m_boundingLinesVerts, raycastResult.m_impactPos, m_impactPtRadius, m_boundingLine_hit, 12);
				}
				else
				{
					AddVertesForDisc2D(m_boundingLinesVerts, raycastResult.m_impactPos, m_impactPtRadius, m_boundingLine_sameDir, 12);
				}
			}
		}
	}
}

void RaycastVSConvex2Mode::RenderBoundingLineAndImpactPos() const
{
	if (!m_boundingLinesVerts.empty() && m_numConvexShownOnScreen == 1)
	{
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);

		g_theRenderer->SetModelConstants(Mat44());
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray((int)m_boundingLinesVerts.size(), m_boundingLinesVerts.data());
	}
}

void RaycastVSConvex2Mode::UpdateRaycast2D()
{
	m_ray.rayStartPos = m_tailPos;
	m_ray.rayDist = GetDistance2D(m_tipPos, m_tailPos);
	m_ray.rayFwdNormal = (m_tipPos - m_tailPos).GetNormalized();
}

void RaycastVSConvex2Mode::ResetGlobalRaycastTobeMissingResult()
{
	Vec2& S = m_ray.rayStartPos;
	Vec2& i = m_ray.rayFwdNormal;
	float& d = m_ray.rayDist;
	Vec2 E = S + i * d;

	RaycastResult2D missResult;
	missResult.m_didImpact = false;
	missResult.m_impactDist = 0.f;
	missResult.m_impactNormal = m_ray.rayFwdNormal;
	missResult.m_rayFwdNormal = m_ray.rayFwdNormal;
	missResult.m_rayDist = m_ray.rayDist;
	missResult.m_rayStartPos = m_ray.rayStartPos;
	missResult.m_didExit = false;
	missResult.m_travelDistInShape = 0.f;
	missResult.m_exitPos = E;
	missResult.m_exitNormal = m_ray.rayFwdNormal;

	m_raycastResult = missResult;
}

bool RaycastVSConvex2Mode::IsSceneHasAHitConvex()
{
	if (m_raycastResult.m_didImpact)
	{
		return true;
	}
	else return false;
}

//
//void RaycastVSConvex2Mode::AddVertsForLines()
//{
//	m_convexVerts.clear();
//
//	for (int lineIndex = 0; lineIndex < (int)m_convexObjs.size(); lineIndex++)
//	{
//		ConvexObj*& AABB2 = m_convexObjs[lineIndex];
//		if (m_hasHitAABB2)
//		{
//			if (AABB2->m_result.m_impactDist == m_shortestImpactDist)
//			{
//				AddVertsForAABB2D(m_convexVerts, AABB2->m_AABB2, Rgba8::BLUE_MVTHL);
//				m_hitConvex = AABB2;
//				continue;
//			}
//			else AddVertsForAABB2D(m_convexVerts, AABB2->m_AABB2, Rgba8::BLUE_MVT);
//		}
//		else AddVertsForAABB2D(m_convexVerts, AABB2->m_AABB2, Rgba8::BLUE_MVT);
//	}
//}

void RaycastVSConvex2Mode::UpdateRaycastResultsForAllConvex()
{
	// reset the current hitting convex
	m_hitConvex = nullptr;

	// find the closest raycast result
	Vec2 rayForwardNormal = (m_tipPos - m_tailPos).GetNormalized();
	float rayDist = (m_tipPos - m_tailPos).GetLength();
	float cloestDist = rayDist;
	for (int convexIndex = 0; convexIndex < m_numConvexShownOnScreen; convexIndex++)
	{
		RaycastResult2D discRaycastResult = RaycastVsDisc2D(m_ray, m_discs[convexIndex]);
		if (discRaycastResult.m_didImpact)
		{
			RaycastResult2D result = RaycastVSConvexHull2(m_ray, m_convexHulls[convexIndex]);

			// if hit,update the shortest raycast hit record
			if (result.m_didImpact)
			{
				if (result.m_impactDist < cloestDist)
				{
					m_raycastResult = result;
					m_hitConvex = &m_convexObjs[convexIndex];
					cloestDist = result.m_impactDist;
				}
			}
		}
	}
}

void RaycastVSConvex2Mode::TestGitBucketSpacePartition()
{
	// assign each convex with bit bucket index
	for (auto& convex : m_convexObjs)
	{
		AssignBitBucketIndexForConvex(convex);
	}

	// generate random ray and mark their bit bucket mask
	vector<RayWithBitBucketMask> rays;
	for (int i = 0; i < m_numRays; ++i)
	{
		float rayStartX = g_rng->RollRandomFloatInRange(0.f, WORLD_SIZE_X);
		float rayStartY = g_rng->RollRandomFloatInRange(0.f, WORLD_SIZE_Y);
		
		float rayEndX = g_rng->RollRandomFloatInRange(0.f, WORLD_SIZE_X);
		float rayEndY = g_rng->RollRandomFloatInRange(0.f, WORLD_SIZE_Y);

		Raycast2D* newRay = new Raycast2D();
		newRay->rayStartPos = Vec2(rayStartX, rayStartY);
		newRay->rayFwdNormal = (Vec2(rayEndX, rayEndY) - newRay->rayStartPos).GetNormalized();
		newRay->rayDist = (Vec2(rayEndX, rayEndY) - newRay->rayStartPos).GetLength();

		Vec2& I = newRay->rayFwdNormal;
		float d = newRay->rayDist;
		Vec2& S = newRay->rayStartPos;

		// Convert start position to grid coordinates
		int grid_x = static_cast<int>(S.x / BIT_BUCKET_REGION_SIZE);
		int grid_y = static_cast<int>(S.y / BIT_BUCKET_REGION_SIZE);

		// Ray direction steps (+1 or -1)
		int stepX = (I.x > 0) ? 1 : -1;
		int stepY = (I.y > 0) ? 1 : -1;

		// todo: visual test this:
		// Compute tMax and tDelta
		// (stepX > 0) evaluates to :
		//	true (or 1) when stepX == +1 (moving right).
		// 	false (or 0) when stepX == -1 (moving left).
		// If moving right(stepX = +1), we want the right boundary of the current cell (grid_x + 1).
		// If moving left(stepX = -1), we want the left boundary of the current cell (grid_x + 0), which simplifies to just grid_x.
		float tMaxX = ((grid_x + (stepX > 0)) * BIT_BUCKET_REGION_SIZE - S.x) / I.x;
		float tMaxY = ((grid_y + (stepY > 0)) * BIT_BUCKET_REGION_SIZE - S.y) / I.y;

		float tDeltaX = abs(BIT_BUCKET_REGION_SIZE / I.x);
		float tDeltaY = abs(BIT_BUCKET_REGION_SIZE / I.y);

		uint32_t mask = 0;

		// Start the ray traversal
		while (d > 0) {
			// update bit mask
			int bitIndex = grid_x + grid_y * m_numColumn;
			mask |= (1u << bitIndex);

			// Determine next step and decrease d
			if (tMaxX < tMaxY) {
				if (tMaxX > d) break; // Exit if the next step exceeds raycast overall distance
				grid_x += stepX;
				tMaxX += tDeltaX;
			}
			else {
				if (tMaxY > d) break;
				grid_y += stepY;
				tMaxY += tDeltaY;
			}

			// Exit if out of bounds
			if (grid_x < 0 || grid_x >= m_numColumn || grid_y < 0 || grid_y >= m_numRow) break;
		}

		rays.emplace_back(*newRay, mask);
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	double timeStart_bitBucket = GetCurrentTimeSeconds();

	double impactDist = 0;
	for (int i = 0; i < m_numConvexShownOnScreen; ++i)
	{
		for (int rayIndex = 0; rayIndex < m_numRays; ++rayIndex)
		{
			RaycastResult2D result = CheckRaycastVSConvexByBitBucket(rays[rayIndex], m_convexObjs[i]);
			if (result.m_didImpact)
			{
				impactDist += result.m_impactDist;
			}
		}
	}
	m_bitBucketAverageImpactDist = impactDist / (double)m_numRays;

	double timeEnd_bitBucket = GetCurrentTimeSeconds();
	m_bitMaskDuration = timeEnd_bitBucket - timeStart_bitBucket;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	double timeStart_DiscCheck = GetCurrentTimeSeconds();

	impactDist = 0.f;
	for (int i = 0; i < m_numConvexShownOnScreen; ++i)
	{
		for (int rayIndex = 0; rayIndex < m_numRays; ++rayIndex)
		{
			RaycastResult2D discRaycastResult = RaycastVsDisc2D(rays[rayIndex].m_ray, m_discs[m_convexObjs[i].m_discIndex]);
			if (discRaycastResult.m_didImpact)
			{
				 RaycastResult2D result = RaycastVSConvexHull2(rays[rayIndex].m_ray, m_convexHulls[m_convexObjs[i].m_convexHullIndex]);
				 if (result.m_didImpact)
				 {
					 impactDist += result.m_impactDist;
				 }
			}
		}
	}
	m_discCheckAverageImpactDist = impactDist / (double)m_numRays;

	double timeEnd_DiscCheck = GetCurrentTimeSeconds();
	m_discCheckDuration = timeEnd_DiscCheck - timeStart_DiscCheck;
}

RaycastResult2D RaycastVSConvex2Mode::CheckRaycastVSConvexByBitBucket(RayWithBitBucketMask& ray, ConvexObj& convex)
{
	// first, check two bit mask to see if the ray pass the area where the convex is located
	if ((ray.m_bitBucketMask & convex.m_bitBucketMask) !=0 )
	{
		RaycastResult2D discRaycastResult = RaycastVsDisc2D(ray.m_ray, m_discs[convex.m_discIndex]);
		if (discRaycastResult.m_didImpact)
		{
			return RaycastVSConvexHull2(ray.m_ray, m_convexHulls[convex.m_convexHullIndex]);
		}
	}
	RaycastResult2D missResult;
	return missResult;
}

void RaycastVSConvex2Mode::AssignBitBucketIndexForConvex(ConvexObj& convex)
{
	Vec2 const& center = m_discs[convex.m_discIndex].m_center;
	float const& radius = m_discs[convex.m_discIndex].m_radius;

	// loop through all four corner for each area of 32
	// if even one corner is inside the radius, then mark the convex bit by the index
	for (int i = 0; i < 32; ++i)
	{
		int x = i % m_numColumn;
		int y = i / m_numColumn;
		float xMin = x * BIT_BUCKET_REGION_SIZE;
		float xMax = (x + 1) * BIT_BUCKET_REGION_SIZE;		
		float yMin = y * BIT_BUCKET_REGION_SIZE;
		float yMax = (y + 1) * BIT_BUCKET_REGION_SIZE;

		// we cant check the four corners only but check overlap
		// if (IsPointInsideDisc2D(Vec2(xMin, yMin), center, radius) ||
		// 	IsPointInsideDisc2D(Vec2(xMin, yMax), center, radius) ||
		// 	IsPointInsideDisc2D(Vec2(xMax, yMin), center, radius) ||
		// 	IsPointInsideDisc2D(Vec2(xMax, yMax), center, radius))
		// {
		// 	convex.m_bitBucketMask |= (1u << i);
		// }

		AABB2 region(Vec2(xMin, yMin), Vec2(xMax, yMax));
		if (DoDiscOverlapAABB2(center, radius, region))
		{
			convex.m_bitBucketMask |= (1u << i);
		}
	}
}

void RaycastVSConvex2Mode::RenderAllConvex() const
{
	if (!m_convexVerts.empty())
	{
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);

		g_theRenderer->SetModelConstants(Mat44());
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);

		if (m_opaqueMode)
		{
			g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
			g_theRenderer->DrawVertexBuffer(m_lineSegmentVertexBuffer, m_numLineSegmentsVertices[m_numConvexShownOnScreen - 1]);
			g_theRenderer->DrawVertexBuffer(m_convexVertexBuffer, m_numConvexVertices[m_numConvexShownOnScreen - 1]);
		}
		else
		{
			g_theRenderer->SetBlendMode(BlendMode::ALPHA);
			g_theRenderer->DrawVertexBuffer(m_convexVertexBuffer, m_numConvexVertices[m_numConvexShownOnScreen - 1]);
			g_theRenderer->DrawVertexBuffer(m_lineSegmentVertexBuffer, m_numLineSegmentsVertices[m_numConvexShownOnScreen - 1]);
		}

		if (m_showDiscs)
		{
			g_theRenderer->DrawVertexBuffer(m_discVertexBuffer, m_numDiscVertices[m_numConvexShownOnScreen - 1]);
		}
	}
}

void RaycastVSConvex2Mode::RenderRaycastResults() const
{
	g_theRenderer->DrawVertexArray((int)m_rayVerts.size(), m_rayVerts.data());
}

void RaycastVSConvex2Mode::UpdateCursorState()
{
	switch (m_cursorState)
	{
	case CursorState_ConvexScene::RESEASED: {	// check if left button is clicked while inside a convex
		if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE) && m_touchingConvex)
		{
			m_cursorState = CursorState_ConvexScene::DRAGGING;
		}
		else
		{
			UpdateConvexTouchByMouseCursor();
			RotateAndScaleTouchingConvex();
			AddVertsForAllConvexAndCreateVertexBuffer();
		}
	}break;
	case CursorState_ConvexScene::DRAGGING:	{	// detect whether the left button is still unreleased
		if (g_theInput->WasKeyJustReleased(KEYCODE_LEFT_MOUSE))
		{
			m_cursorState = CursorState_ConvexScene::RESEASED;
		}
		else // then we need to move the dragging convex
		{
			MoveDraggingConvex();
		}
	}break;
	case CursorState_ConvexScene::NUM_STATE:
		break;
	default:
		break;
	}
}

bool RaycastVSConvex2Mode::UpdateConvexTouchByMouseCursor()
{
	Vec2 mousePos = GetMousePositionInWorld();
	for (int convexIndex = 0; convexIndex < m_numConvexShownOnScreen; ++convexIndex)
	{
		if (m_convexHulls[convexIndex].IsPointInside(mousePos))
		{
			m_touchingConvex = &m_convexObjs[convexIndex];
			return true;
		}
	}
	m_touchingConvex = nullptr;
	return false;
}

void RaycastVSConvex2Mode::MoveDraggingConvex()
{
	Vec2 disp = GetMouseDispThisFrame();

	m_convexPolys[m_touchingConvex->m_convexPolyIndex].MoveAllVerticesByDisplacement(disp);

	// update hull and disc
	m_convexHulls[m_touchingConvex->m_convexHullIndex] = ConvexHull2(m_convexPolys[m_touchingConvex->m_convexPolyIndex]);
	m_discs[m_touchingConvex->m_discIndex].m_center += disp;

	// we also going to scale the ring vertices directly because they are wasting too much if we adding again every frame
	int discIndex = m_touchingConvex->m_discIndex;
	int verticeStartIndex = m_numDiscVertices[discIndex - 1];
	int verticeEndIndex = m_numDiscVertices[discIndex];
	if (discIndex == 0)
	{
		verticeStartIndex = 0;
		verticeEndIndex = m_numDiscVertices[0];
	}
	for (int i = verticeStartIndex; i < verticeEndIndex; ++i)
	{
		Vec3& p = m_discVerts[i].m_position;
		TransformPositionAroundPosOnXY(p, 1.f, m_mousePosCurrentFrame, 0.f, disp);
	}

	AddVertsForAllConvexAndCreateVertexBuffer();
}

void RaycastVSConvex2Mode::RotateAndScaleTouchingConvex()
{
	if (m_touchingConvex)
	{
		// rotate
		if (g_theInput->WasKeyJustPressed('W') || g_theInput->IsKeyDown('W') || g_theInput->WasKeyJustPressed('R') || g_theInput->IsKeyDown('R'))
		{
			float rotatingSpeed = m_rotatingSpeed;
			if (g_theInput->WasKeyJustPressed('R') || g_theInput->IsKeyDown('R'))
			{
				rotatingSpeed *= -1.f;
			}
			
			m_convexPolys[m_touchingConvex->m_convexHullIndex].RotateAllVerticesByDegrees(rotatingSpeed, m_mousePosCurrentFrame);

			// update hull
			m_convexHulls[m_touchingConvex->m_convexHullIndex] = ConvexHull2(m_convexPolys[m_touchingConvex->m_convexPolyIndex]);

			// we also going to scale the ring vertices directly because they are wasting too much if we adding again every frame
			int discIndex = m_touchingConvex->m_discIndex;
			if (discIndex >= 1)
			{
				for (int i = m_numDiscVertices[discIndex - 1]; i < m_numDiscVertices[discIndex]; ++i)
				{
					Vec3& p = m_discVerts[i].m_position;
					TransformPositionAroundPosOnXY(p, 1.f, m_mousePosCurrentFrame, rotatingSpeed, Vec2::ZERO);
				}
			}
			else
			{
				for (int i = 0; i < m_numDiscVertices[0]; ++i)
				{
					Vec3& p = m_discVerts[i].m_position;
					TransformPositionAroundPosOnXY(p, 1.f, m_mousePosCurrentFrame, rotatingSpeed, Vec2::ZERO);
				}
			}
		}

		// scale
		if (g_theInput->WasKeyJustPressed('L') || g_theInput->IsKeyDown('L') || g_theInput->WasKeyJustPressed('K') || g_theInput->IsKeyDown('K'))
		{
			float scaleSpeed = m_scaleSpeed;
			if (g_theInput->WasKeyJustPressed('K') || g_theInput->IsKeyDown('K'))
			{
				scaleSpeed *= -1.f;
			}

			float scale = 1.f + scaleSpeed;
			m_convexPolys[m_touchingConvex->m_convexHullIndex].ScaleAllVertices(scale, m_mousePosCurrentFrame);

			// update hull and discs
			m_convexHulls[m_touchingConvex->m_convexHullIndex] = ConvexHull2(m_convexPolys[m_touchingConvex->m_convexPolyIndex]);
			m_discs[m_touchingConvex->m_discIndex] = Disc2(m_discs[m_touchingConvex->m_discIndex].m_radius * scale, m_discs[m_touchingConvex->m_discIndex].m_center);

			// we also going to scale the ring vertices directly because they are wasting too much if we adding again every frame
			int discIndex = m_touchingConvex->m_discIndex;
			if (discIndex >= 1)
			{
				for (int i = m_numDiscVertices[discIndex - 1]; i < m_numDiscVertices[discIndex]; ++i)
				{
					Vec3& p = m_discVerts[i].m_position;
					TransformPositionAroundPosOnXY(p, scale, m_mousePosCurrentFrame, 0.f, Vec2::ZERO);
				}
			}
			else
			{
				for (int i = 0; i < m_numDiscVertices[0]; ++i)
				{
					Vec3& p = m_discVerts[i].m_position;
					TransformPositionAroundPosOnXY(p, scale, m_mousePosCurrentFrame, 0.f, Vec2::ZERO);
				}
			}
		}
	}
}

void RaycastVSConvex2Mode::TestBufferWriterAndParser()
{
	vector<uint8_t> inBuffer;
	BufferWriter writer = *(new BufferWriter(inBuffer));

	AppendTestFileBufferData(writer, eBufferEndian::LITTLE);		
	AppendTestFileBufferData(writer, eBufferEndian::BIG);

	// set up save file name
	string saveFolderPathName = Stringf("BufferWriterTest2");
	string fileName = Stringf("%s.binary", saveFolderPathName.c_str());

	if (!FileWriteFromBuffer(inBuffer, fileName))
	{
		ERROR_AND_DIE("Have problem saving the test binary file");
	}

	vector<uint8_t> correctResultBuffer;
	FileReadToBuffer(correctResultBuffer, "BufferWriterTest.binary");	
	vector<uint8_t> writeResultBuffer;
	FileReadToBuffer(writeResultBuffer, "BufferWriterTest2.binary");
	if (writeResultBuffer !=  correctResultBuffer)
	{
		ERROR_AND_DIE("Buffer writter is incorrect");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	vector<uint8_t> outBuffer;
	fileName = "Test.binary";
	FileReadToBuffer(outBuffer, fileName);
	BufferParser* parser = new BufferParser(outBuffer);
	ParseTestFileBufferData(*parser, GetPlatformNativeEndian());
}

void RaycastVSConvex2Mode::AppendTestFileBufferData(BufferWriter& bufWrite, eBufferEndian endianMode /*= eBufferEndian::NATIVE*/)
{
	bufWrite.SetEndianMode(endianMode);
	bufWrite.AppendChar('T');
	bufWrite.AppendChar('E');
	bufWrite.AppendChar('S');
	bufWrite.AppendChar('T');
	bufWrite.AppendByte(2); // Version 2
	bufWrite.AppendByte((unsigned char)bufWrite.GetEndianMode());
	bufWrite.AppendBool(false);
	bufWrite.AppendBool(true);
	bufWrite.AppendUint32(0x12345678);
	bufWrite.AppendInt32(-7); // signed 32-bit int
	bufWrite.AppendFloat(1.f); // in memory looks like hex: 00 00 80 3F (or 3F 80 00 00 in big endian)
	bufWrite.AppendDouble(3.1415926535897932384626433832795); // actually 3.1415926535897931 (best it can do)
	bufWrite.AppendStringZeroTerminated("Hello"); // written with a trailing 0 ('\0') after (6 bytes total)
	bufWrite.AppendStringAfter32BitLength("Is this thing on?"); // uint 17, then 17 chars (no zero-terminator after)
	bufWrite.AppendRgba(Rgba8(200, 100, 50, 255)); // four bytes in RGBA order (endian-independent)
	bufWrite.AppendByte(8); // 0x08 == 8 (byte)
	bufWrite.AppendRgb(Rgba8(238, 221, 204, 255)); // written as 3 bytes (RGB) only; ignores Alpha
	bufWrite.AppendByte(9); // 0x09 == 9 (byte)
	bufWrite.AppendIntVec2(IntVec2(1920, 1080));
	bufWrite.AppendVec2(Vec2(-0.6f, 0.8f));
	bufWrite.AppendVertexPCU(Vertex_PCU(Vec3(3.f, 4.f, 5.f), Rgba8(100, 101, 102, 103), Vec2(0.125f, 0.625f)));
}

void RaycastVSConvex2Mode::ParseTestFileBufferData(BufferParser& bufParse, eBufferEndian endianMode)
{
	// Parse known test file elements
	bufParse.SetEndianMode(endianMode);
	char fourCC0_T = bufParse.ParseChar(); // 'T' == 0x54 hex == 84 decimal
	char fourCC1_E = bufParse.ParseChar(); // 'E' == 0x45 hex == 84 decimal
	char fourCC2_S = bufParse.ParseChar(); // 'S' == 0x53 hex == 69 decimal
	char fourCC3_T = bufParse.ParseChar(); // 'T' == 0x54 hex == 84 decimal
	unsigned char version = bufParse.ParseByte(); // version 2
	eBufferEndian mode = (eBufferEndian)bufParse.ParseByte(); // 1 for little endian, or 2 for big endian
	bool shouldBeFalse = bufParse.ParseBool(); // written in buffer as byte 0 or 1
	bool shouldBeTrue = bufParse.ParseBool(); // written in buffer as byte 0 or 1
	unsigned int largeUint = bufParse.ParseUint32(); // 0x12345678
	int negativeSeven = bufParse.ParseInt32(); // -7 (as signed 32-bit int)
	float oneF = bufParse.ParseFloat(); // 1.0f
	double pi = bufParse.ParseDouble(); // 3.1415926535897932384626433832795 (or as best it can)

	std::string helloString, isThisThingOnString;
	bufParse.ParseStringZeroTerminated(helloString); // written with a trailing 0 ('\0') after (6 bytes total)
	bufParse.ParseStringAfter32BitLength(isThisThingOnString); // written as uint 17, then 17 characters (no zero-terminator after)

	Rgba8 rustColor = bufParse.ParseRgba(); // Rgba8( 200, 100, 50, 255 )
	unsigned char eight = bufParse.ParseByte(); // 0x08 == 8 (byte)
	Rgba8 seashellColor = bufParse.ParseRgb(); // Rgba8( 238, 221, 204) written as 3 bytes (RGB) only; assume alpha is 255
	unsigned char nine = bufParse.ParseByte(); // 0x09 == 9 (byte)
	IntVec2 highDefRes = bufParse.ParseIntVec2(); // IntVector2( 1920, 1080 )
	Vec2 normal2D = bufParse.ParseVec2(); // Vector2( -0.6f, 0.8f )
	Vertex_PCU vertex = bufParse.ParseVertexPCU(); // VertexPCU( 3.f, 4.f, 5.f, Rgba(100,101,102,103), 0.125f, 0.625f ) );

	// Validate actual values parsed
	GUARANTEE_OR_DIE(fourCC0_T == 'T', "Parse is incorrect");
	GUARANTEE_OR_DIE(fourCC1_E == 'E', "Parse is incorrect");
	GUARANTEE_OR_DIE(fourCC2_S == 'S', "Parse is incorrect");
	GUARANTEE_OR_DIE(fourCC3_T == 'T', "Parse is incorrect");
	GUARANTEE_OR_DIE(version == 2, "Parse is incorrect");
	GUARANTEE_OR_DIE(mode == endianMode, "Parse is incorrect"); // verify that we're receiving things in the endianness we expect
	GUARANTEE_OR_DIE(shouldBeFalse == false, "Parse is incorrect");
	GUARANTEE_OR_DIE(shouldBeTrue == true, "Parse is incorrect");
	GUARANTEE_OR_DIE(largeUint == 0x12345678, "Parse is incorrect");
	GUARANTEE_OR_DIE(negativeSeven == -7, "Parse is incorrect");
	GUARANTEE_OR_DIE(oneF == 1.f, "Parse is incorrect");
	GUARANTEE_OR_DIE(pi == 3.1415926535897932384626433832795, "Parse is incorrect");
	GUARANTEE_OR_DIE(helloString == "Hello", "Parse is incorrect");
	GUARANTEE_OR_DIE(isThisThingOnString == "Is this thing on?", "Parse is incorrect");
	GUARANTEE_OR_DIE(rustColor == Rgba8(200, 100, 50, 255), "Parse is incorrect");
	GUARANTEE_OR_DIE(eight == 8, "Parse is incorrect");
	GUARANTEE_OR_DIE(seashellColor == Rgba8(238, 221, 204), "Parse is incorrect");
	GUARANTEE_OR_DIE(nine == 9, "Parse is incorrect");
	GUARANTEE_OR_DIE(highDefRes == IntVec2(1920, 1080), "Parse is incorrect");
	GUARANTEE_OR_DIE(normal2D == Vec2(-0.6f, 0.8f), "Parse is incorrect");
	GUARANTEE_OR_DIE(vertex.m_position == Vec3(3.f, 4.f, 5.f), "Parse is incorrect");
	GUARANTEE_OR_DIE(vertex.m_color == Rgba8(100, 101, 102, 103), "Parse is incorrect");
	GUARANTEE_OR_DIE(vertex.m_uvTexCoords == Vec2(0.125f, 0.625f), "Parse is incorrect");
}

void RaycastVSConvex2Mode::TestLoadingExampleGHCSSaveFile()
{
	vector<uint8_t> outBuffer;
	string fileName = "Data/Saves/Smile.ghcs";
	// string fileName = "Data/Saves/C33_Empty.ghcs";
	FileReadToBuffer(outBuffer, fileName);
	BufferParser* parser = new BufferParser(outBuffer);

	// verify the header of the binary file to be correct convex scene save file
	if ('G' == parser->ParseByte() &&
		'H' == parser->ParseByte() &&
		'C' == parser->ParseByte() &&
		'S' == parser->ParseByte() &&
		33  == parser->ParseByte())	// cohort ID
	{
		int majorFileVersion = parser->ParseByte();
		int minorFileVersion = parser->ParseByte();

		eBufferEndian endianMode = (eBufferEndian)parser->ParseByte();
		parser->SetEndianMode((eBufferEndian)endianMode);
		uint32_t tableOffset = parser->ParseInt32();

		// check the end of the header part
		if ('E' != parser->ParseByte() ||
			'N' != parser->ParseByte() ||
			'D' != parser->ParseByte() ||
			'H' != parser->ParseByte()) {
			ERROR_AND_DIE("Error reading the header part");
		}

		// let's go to the table part and get the information
		parser->SetReadOffset(tableOffset);
		// check the start of table content
		if ('G' != parser->ParseByte() ||
			'H' != parser->ParseByte() ||
			'T' != parser->ParseByte() ||
			'C' != parser->ParseByte())
		{
			ERROR_AND_DIE("Error reading the header part");
		}
		// parse all the data store in the table
		uint8_t numsChunk = parser->ParseByte();
		vector<ChunkSaveData> chunks(numsChunk);
		if (numsChunk == 0)
		{
			EventArgs warning;
			warning.SetValue("message", "Save file do not have any chunk data");
			g_theDevConsole->Command_Echo(warning);
			return;
		}
		for (int chunkIndex = 0; chunkIndex < numsChunk; ++chunkIndex)
		{
			chunks[chunkIndex].m_chunkType = parser->ParseByte();
			chunks[chunkIndex].m_chunkHeaderStart = parser->ParseUint32();
			chunks[chunkIndex].m_chunkTotalSize = parser->ParseUint32();
		}
		// clear convex data
		if (!m_convexObjs.empty())
		{
			m_convexObjs.clear();
			m_convexObjs.shrink_to_fit();	// might not work based on compability issue
			m_discs.clear();
			m_discs.shrink_to_fit();
			m_convexPolys.clear();
			m_convexPolys.shrink_to_fit();
			m_convexHulls.clear();
			m_convexHulls.shrink_to_fit();
		}
		// check the end of the data table
		if ('E' != parser->ParseByte() ||
			'N' != parser->ParseByte() ||
			'D' != parser->ParseByte() ||
			'T' != parser->ParseByte()) {
			ERROR_AND_DIE("Error reading the header part");
		}

		// from the start to the end, read the data to the scene
		AABB2 worldBounds = AABB2::ZERO_TO_ONE;
		uint16_t numObjects = 0;
		float scaleX = 0.f;
		float scaleY = 0.f;
		for (int chunkIndex = 0; chunkIndex < numsChunk; ++chunkIndex)
		{
			// scene info chunk
			if (chunks[chunkIndex].m_chunkType == 0x01)
			{
				parser->SetReadOffset(chunks[chunkIndex].m_chunkHeaderStart);

				// check the start of table content
				if ('G' != parser->ParseByte() ||
					'H' != parser->ParseByte() ||
					'C' != parser->ParseByte() ||
					'K' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				if (0x01 != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				endianMode = (eBufferEndian)parser->ParseByte();
				parser->SetEndianMode((eBufferEndian)endianMode);

				if(18 != parser->ParseUint32())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				worldBounds = parser->ParseAABB2();
				// update the scale for updating later convex info in the save file
				scaleX = worldBounds.m_maxs.x / WORLD_SIZE_X;
				scaleY = worldBounds.m_maxs.y / WORLD_SIZE_Y;

				numObjects = parser->ParseUint16();
				if (numsChunk == 0)
				{
					EventArgs warning;
					warning.SetValue("message", "Save file do not have any convex object");
					g_theDevConsole->Command_Echo(warning);
					return;
				}

				if ('E' != parser->ParseByte() ||
					'N' != parser->ParseByte() ||
					'D' != parser->ParseByte() ||
					'C' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}
			}

			// convexPoly 
			else if (chunks[chunkIndex].m_chunkType == 0x02)
			{
				parser->SetReadOffset(chunks[chunkIndex].m_chunkHeaderStart);

				// check the start of table content
				if ('G' != parser->ParseByte() ||
					'H' != parser->ParseByte() ||
					'C' != parser->ParseByte() ||
					'K' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				if (0x02 != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				endianMode = (eBufferEndian)parser->ParseByte();
				parser->SetEndianMode((eBufferEndian)endianMode);

				uint32_t convexPolyChunkDataSize = parser->ParseUint32();
				UNUSED(convexPolyChunkDataSize);
				uint16_t numConvexPoly = parser->ParseUint16();
				 
				for (int i = 0; i < numConvexPoly; ++i)
				{ 
					uint8_t numVertex = parser->ParseByte();	// this is 66 for the second convex
					m_convexPolys.emplace_back(ConvexPoly2());
					for (int j = 0; j < numVertex; ++j)
					{
						Vec2 pos = parser->ParseVec2();
						pos.x *= scaleX;
						pos.y *= scaleY;
						m_convexPolys.back().AddVertexPosition(pos);
					}
				}

				// check the start of table content
				if ('E' != parser->ParseByte() ||
					'N' != parser->ParseByte() ||
					'D' != parser->ParseByte() ||
					'C' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}
			}
			
			// convexHull 
			else if (chunks[chunkIndex].m_chunkType == 0x80)
			{
				parser->SetReadOffset(chunks[chunkIndex].m_chunkHeaderStart);

				// check the start of table content
				if ('G' != parser->ParseByte() ||
					'H' != parser->ParseByte() ||
					'C' != parser->ParseByte() ||
					'K' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}
			}

			// Bounding discs
			else if (chunks[chunkIndex].m_chunkType == 0x81)
			{
				parser->SetReadOffset(chunks[chunkIndex].m_chunkHeaderStart);

				// check the start of table content
				if ('G' != parser->ParseByte() ||
					'H' != parser->ParseByte() ||
					'C' != parser->ParseByte() ||
					'K' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				unsigned char chunkType = parser->ParseByte();

				endianMode = (eBufferEndian)parser->ParseByte();
				parser->SetEndianMode((eBufferEndian)endianMode);

				uint32_t boundingDiscDataSize = parser->ParseUint32();
				UNUSED(boundingDiscDataSize);
				uint16_t numDiscs = parser->ParseUint16();

				for (int i = 0; i < numDiscs; ++i)
				{
					Vec2 center = parser->ParseVec2();
					center.x *= scaleX;
					center.y *= scaleY;
					float radius = parser->ParseFloat();
					m_discs.emplace_back( Disc2(radius, center) );
				}
			}
		}

		// after loading the data
		// if the convex hull data is empty, regenerate convex hull data
		int objIndex = 0;
		for (auto poly : m_convexPolys)
		{
			m_convexHulls.emplace_back(ConvexHull2(poly));
			m_convexObjs.emplace_back( ConvexObj(objIndex, objIndex, objIndex) );
			++objIndex;
			
			// update vertices num
			int numVertices = poly.GetNumsOfVertices();
			if (objIndex != 0)
			{
				m_numConvexVertices[objIndex] = m_numConvexVertices[objIndex - 1] + (numVertices - 2) * 3;
				m_numLineSegmentsVertices[objIndex] = m_numLineSegmentsVertices[objIndex - 1] + numVertices * 6;
				m_numDiscVertices[objIndex] = m_numDiscVertices[objIndex - 1] + m_discNumSegments * 6;
			}
			else
			{
				m_numConvexVertices[0] = (numVertices - 2) * 3;
				m_numLineSegmentsVertices[0] = numVertices * 6;
				m_numDiscVertices[0] = m_discNumSegments * 6;
			}
		}

		// based on the convex obj we read, we show them all on the screen
		m_numConvexShownOnScreen = objIndex + 1;

		// based on the loading info, reload the convex info
		// store all the convex vertices
		if (!m_convexVerts.empty())
		{
			m_convexVerts.clear();
			m_convexVerts.shrink_to_fit();
			m_lineSegmentVerts.clear();
			m_lineSegmentVerts.shrink_to_fit();
		}

		// add verts for all convex
		for (ConvexObj const& convex : m_convexObjs) // need to use &, otherwise convex is a copy of each element in m_convexObjs
		{
			// for each edges add line segments
			int  numVertices = (int)m_convexPolys[convex.m_convexPolyIndex].GetVertexPos().size();
			vector<Vec2> vertices = m_convexPolys[convex.m_convexPolyIndex].GetVertexPos();
			for (int lineIndex = 0; lineIndex < numVertices; ++lineIndex)
			{
				if (lineIndex == (numVertices - 1))
				{
					AddVertsForLineSegment2D(m_lineSegmentVerts, vertices[lineIndex], vertices[0], m_convexEdgeThickness, m_convexEdgeColor);
				}
				else
				{
					AddVertsForLineSegment2D(m_lineSegmentVerts, vertices[lineIndex], vertices[lineIndex + 1], m_convexEdgeThickness, m_convexEdgeColor);
				}
			}

			// for each disc, add verts
			if (!m_discVertexBuffer)
			{
				AddVertesForRing2D(m_discVerts, m_discs[convex.m_discIndex].m_center, m_discs[convex.m_discIndex].m_radius, m_boundingLineThickness * 0.5f, Rgba8::BRIGHT_ORANGE, m_discNumSegments);
			}
		}

		// create the vertex buffer for all the convex
		if (m_convexVertexBuffer)
		{
			delete m_convexVertexBuffer;
			m_convexVertexBuffer = nullptr;

			delete m_lineSegmentVertexBuffer;
			m_lineSegmentVertexBuffer = nullptr;

			delete m_discVertexBuffer;
			m_discVertexBuffer = nullptr;
		}
		m_convexVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_convexVerts.size()), sizeof(Vertex_PCU));

		size_t vertexSize = sizeof(Vertex_PCU);
		size_t vertexArrayDataSize = (m_convexVerts.size()) * vertexSize;
		g_theRenderer->CopyCPUToGPU(m_convexVerts.data(), vertexArrayDataSize, m_convexVertexBuffer);

		m_lineSegmentVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_lineSegmentVerts.size()), sizeof(Vertex_PCU));
		vertexArrayDataSize = (m_lineSegmentVerts.size()) * vertexSize;
		g_theRenderer->CopyCPUToGPU(m_lineSegmentVerts.data(), vertexArrayDataSize, m_lineSegmentVertexBuffer);


		m_discVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_discVerts.size()), sizeof(Vertex_PCU));
		vertexArrayDataSize = (m_discVerts.size()) * vertexSize;
		g_theRenderer->CopyCPUToGPU(m_discVerts.data(), vertexArrayDataSize, m_discVertexBuffer);
	}
	else
	{
		ERROR_AND_DIE(Stringf("Error reading %s, wrong format", fileName.c_str()));
	}
}

void RaycastVSConvex2Mode::TestSavingExampleGHCSSaveFile()
{
	vector<uint8_t> outBuffer;
	string fileName = "Data/Saves/C33_TwoObjects.ghcs";
	// string fileName = "Data/Saves/C33_Empty.ghcs";
	FileReadToBuffer(outBuffer, fileName);
	BufferParser* parser = new BufferParser(outBuffer);

	// verify the header of the binary file to be correct convex scene save file
	if ('G' == parser->ParseByte() &&
		'H' == parser->ParseByte() &&
		'C' == parser->ParseByte() &&
		'S' == parser->ParseByte() &&
		33 == parser->ParseByte())	// cohort ID
	{
		int majorFileVersion = parser->ParseByte();
		int minorFileVersion = parser->ParseByte();

		eBufferEndian endianMode = (eBufferEndian)parser->ParseByte();
		parser->SetEndianMode((eBufferEndian)endianMode);
		uint32_t tableOffset = parser->ParseInt32();

		// check the end of the header part
		if ('E' != parser->ParseByte() ||
			'N' != parser->ParseByte() ||
			'D' != parser->ParseByte() ||
			'H' != parser->ParseByte()) {
			ERROR_AND_DIE("Error reading the header part");
		}

		// let's go to the table part and get the information
		parser->SetReadOffset(tableOffset);
		// check the start of table content
		if ('G' != parser->ParseByte() ||
			'H' != parser->ParseByte() ||
			'T' != parser->ParseByte() ||
			'C' != parser->ParseByte())
		{
			ERROR_AND_DIE("Error reading the header part");
		}
		// parse all the data store in the table
		uint8_t numsChunk = parser->ParseByte();
		vector<ChunkSaveData> chunks(numsChunk);
		if (numsChunk == 0)
		{
			EventArgs warning;
			warning.SetValue("message", "Save file do not have any chunk data");
			g_theDevConsole->Command_Echo(warning);
			return;
		}
		for (int chunkIndex = 0; chunkIndex < numsChunk; ++chunkIndex)
		{
			chunks[chunkIndex].m_chunkType = parser->ParseByte();
			chunks[chunkIndex].m_chunkHeaderStart = parser->ParseUint32();
			chunks[chunkIndex].m_chunkTotalSize = parser->ParseUint32();
		}
		// clear convex data
		if (!m_convexObjs.empty())
		{
			m_convexObjs.clear();
			m_convexObjs.shrink_to_fit();	// might not work based on compability issue
			m_discs.clear();
			m_discs.shrink_to_fit();
			m_convexPolys.clear();
			m_convexPolys.shrink_to_fit();
			m_convexHulls.clear();
			m_convexHulls.shrink_to_fit();
		}
		// check the end of the data table
		if ('E' != parser->ParseByte() ||
			'N' != parser->ParseByte() ||
			'D' != parser->ParseByte() ||
			'T' != parser->ParseByte()) {
			ERROR_AND_DIE("Error reading the header part");
		}

		// from the start to the end, read the data to the scene
		AABB2 worldBounds = AABB2::ZERO_TO_ONE;
		uint16_t numObjects = 0;
		float scaleX = 0.f;
		float scaleY = 0.f;
		for (int chunkIndex = 0; chunkIndex < numsChunk; ++chunkIndex)
		{
			// scene info chunk
			if (chunks[chunkIndex].m_chunkType == 0x01)
			{
				parser->SetReadOffset(chunks[chunkIndex].m_chunkHeaderStart);

				// check the start of table content
				if ('G' != parser->ParseByte() ||
					'H' != parser->ParseByte() ||
					'C' != parser->ParseByte() ||
					'K' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				if (0x01 != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				endianMode = (eBufferEndian)parser->ParseByte();
				parser->SetEndianMode((eBufferEndian)endianMode);

				if (18 != parser->ParseUint32())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				worldBounds = parser->ParseAABB2();
				// update the scale for updating later convex info in the save file
				scaleX = worldBounds.m_maxs.x / WORLD_SIZE_X;
				scaleY = worldBounds.m_maxs.y / WORLD_SIZE_Y;

				numObjects = parser->ParseUint16();
				if (numsChunk == 0)
				{
					EventArgs warning;
					warning.SetValue("message", "Save file do not have any convex object");
					g_theDevConsole->Command_Echo(warning);
					return;
				}

				if ('E' != parser->ParseByte() ||
					'N' != parser->ParseByte() ||
					'D' != parser->ParseByte() ||
					'C' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}
			}

			// convexPoly 
			else if (chunks[chunkIndex].m_chunkType == 0x02)
			{
				parser->SetReadOffset(chunks[chunkIndex].m_chunkHeaderStart);

				// check the start of table content
				if ('G' != parser->ParseByte() ||
					'H' != parser->ParseByte() ||
					'C' != parser->ParseByte() ||
					'K' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				if (0x02 != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				endianMode = (eBufferEndian)parser->ParseByte();
				parser->SetEndianMode((eBufferEndian)endianMode);

				uint32_t convexPolyChunkDataSize = parser->ParseUint32();
				UNUSED(convexPolyChunkDataSize);
				uint16_t numConvexPoly = parser->ParseUint16();

				for (int i = 0; i < numConvexPoly; ++i)
				{
					uint8_t numVertex = parser->ParseByte();	// this is 66 for the second convex
					m_convexPolys.emplace_back(ConvexPoly2());
					for (int j = 0; j < numVertex; ++j)
					{
						Vec2 pos = parser->ParseVec2();
						pos.x *= scaleX;
						pos.y *= scaleY;
						m_convexPolys.back().AddVertexPosition(pos);
					}
				}

				// check the start of table content
				if ('E' != parser->ParseByte() ||
					'N' != parser->ParseByte() ||
					'D' != parser->ParseByte() ||
					'C' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}
			}

			// convexHull 
			else if (chunks[chunkIndex].m_chunkType == 0x80)
			{
				parser->SetReadOffset(chunks[chunkIndex].m_chunkHeaderStart);

				// check the start of table content
				if ('G' != parser->ParseByte() ||
					'H' != parser->ParseByte() ||
					'C' != parser->ParseByte() ||
					'K' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}
			}

			// Bounding discs
			else if (chunks[chunkIndex].m_chunkType == 0x81)
			{
				parser->SetReadOffset(chunks[chunkIndex].m_chunkHeaderStart);

				// check the start of table content
				if ('G' != parser->ParseByte() ||
					'H' != parser->ParseByte() ||
					'C' != parser->ParseByte() ||
					'K' != parser->ParseByte())
				{
					ERROR_AND_DIE("Error reading the header part");
				}

				endianMode = (eBufferEndian)parser->ParseByte();
				parser->SetEndianMode((eBufferEndian)endianMode);

				uint32_t boundingDiscDataSize = parser->ParseUint32();
				UNUSED(boundingDiscDataSize);
				uint16_t numDiscs = parser->ParseUint16();

				for (int i = 0; i < numDiscs; ++i)
				{
					Vec2 center = parser->ParseVec2();
					center.x *= scaleX;
					center.y *= scaleY;
					float radius = parser->ParseFloat();
					m_discs.emplace_back(Disc2(radius, center));
				}
			}
		}

		// after loading the data
		// if the convex hull data is empty, regenerate convex hull data
		int objIndex = 0;
		// for (int i = 0; i < numObjects; ++i)
		// {
		// 	// m_convexHulls.emplace_back(ConvexHull2(poly));
		// 	m_convexObjs.emplace_back(ConvexObj(objIndex, objIndex, objIndex));
		// 	++objIndex;
		// 
		// 	// update vertices num
		// 	// int numVertices = poly.GetNumsOfVertices();
		// 	if (objIndex != 0)
		// 	{
		// 		m_numConvexVertices[objIndex] = m_numConvexVertices[objIndex - 1] + (numVertices - 2) * 3;
		// 		m_numLineSegmentsVertices[objIndex] = m_numLineSegmentsVertices[objIndex - 1] + numVertices * 6;
		// 		m_numDiscVertices[objIndex] = m_numDiscVertices[objIndex - 1] + m_discNumSegments * 6;
		// 	}
		// 	else
		// 	{
		// 		m_numConvexVertices[0] = (numVertices - 2) * 3;
		// 		m_numLineSegmentsVertices[0] = numVertices * 6;
		// 		m_numDiscVertices[0] = m_discNumSegments * 6;
		// 	}
		// }

		// based on the convex obj we read, we show them all on the screen
		m_numConvexShownOnScreen = objIndex + 1;

		// based on the loading info, reload the convex info
		// store all the convex vertices
		if (!m_convexVerts.empty())
		{
			m_convexVerts.clear();
			m_convexVerts.shrink_to_fit();
			m_lineSegmentVerts.clear();
			m_lineSegmentVerts.shrink_to_fit();
		}

		// for (int i = 0; i < numObjects; ++i)
		// {
		// 	Vec2 discCenter;
		// 	discCenter.x = g_rng->RollRandomFloatInRange(WORLD_SIZE_X * 0.05f, WORLD_SIZE_X * 0.9f);
		// 	discCenter.y = g_rng->RollRandomFloatInRange(WORLD_SIZE_Y * 0.05f, WORLD_SIZE_Y * 0.9f);
		// 
		// 	float radius = g_rng->RollRandomFloatInRange(MIN_RADIUS, MAX_RADIUS);
		// 	m_discs.emplace_back(Disc2(radius, discCenter));
		// 
		// 	m_convexObjs.emplace_back(ConvexObj(m_discs.back(), i));
		// }

		// add verts for all convex
		for (ConvexObj const& convex : m_convexObjs) // need to use &, otherwise convex is a copy of each element in m_convexObjs
		{
			// for each edges add line segments
			int  numVertices = (int)m_convexPolys[convex.m_convexPolyIndex].GetVertexPos().size();
			vector<Vec2> vertices = m_convexPolys[convex.m_convexPolyIndex].GetVertexPos();
			for (int lineIndex = 0; lineIndex < numVertices; ++lineIndex)
			{
				if (lineIndex == (numVertices - 1))
				{
					AddVertsForLineSegment2D(m_lineSegmentVerts, vertices[lineIndex], vertices[0], m_convexEdgeThickness, m_convexEdgeColor);
				}
				else
				{
					AddVertsForLineSegment2D(m_lineSegmentVerts, vertices[lineIndex], vertices[lineIndex + 1], m_convexEdgeThickness, m_convexEdgeColor);
				}
			}

			// for each disc, add verts
			if (!m_discVertexBuffer)
			{
				AddVertesForRing2D(m_discVerts, m_discs[convex.m_discIndex].m_center, m_discs[convex.m_discIndex].m_radius, m_boundingLineThickness * 0.5f, Rgba8::BRIGHT_ORANGE, m_discNumSegments);
			}
		}

		// create the vertex buffer for all the convex
		if (m_convexVertexBuffer)
		{
			delete m_convexVertexBuffer;
			m_convexVertexBuffer = nullptr;

			delete m_lineSegmentVertexBuffer;
			m_lineSegmentVertexBuffer = nullptr;

			delete m_discVertexBuffer;
			m_discVertexBuffer = nullptr;
		}
		m_convexVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_convexVerts.size()), sizeof(Vertex_PCU));

		size_t vertexSize = sizeof(Vertex_PCU);
		size_t vertexArrayDataSize = (m_convexVerts.size()) * vertexSize;
		g_theRenderer->CopyCPUToGPU(m_convexVerts.data(), vertexArrayDataSize, m_convexVertexBuffer);

		m_lineSegmentVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_lineSegmentVerts.size()), sizeof(Vertex_PCU));
		vertexArrayDataSize = (m_lineSegmentVerts.size()) * vertexSize;
		g_theRenderer->CopyCPUToGPU(m_lineSegmentVerts.data(), vertexArrayDataSize, m_lineSegmentVertexBuffer);


		m_discVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_discVerts.size()), sizeof(Vertex_PCU));
		vertexArrayDataSize = (m_discVerts.size()) * vertexSize;
		g_theRenderer->CopyCPUToGPU(m_discVerts.data(), vertexArrayDataSize, m_discVertexBuffer);
	}
	else
	{
		ERROR_AND_DIE(Stringf("Error reading %s, wrong format", fileName.c_str()));
	}
}

