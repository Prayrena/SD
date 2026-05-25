#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Math/Easing.hpp"
#include "Engine/Math/Splines.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Game/Curves2DMode.hpp"
#include "Game/GameCommon.hpp"
#include "Game/ReferencePoint.hpp"
#include "Game/Entity.hpp"
#include "Game/App.hpp"

extern App* g_theApp;
extern InputSystem* g_theInput;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;
extern Window* g_theWindow;
extern Clock* g_theGameClock;
extern BitmapFont* g_consoleFont;

Curves2DMode::Curves2DMode()
{

}

Curves2DMode::~Curves2DMode()
{

}

void Curves2DMode::Startup()
{
	g_theInput->SetCursorMode(false, false);

	m_controlPtTimer = new Timer(2.f, g_theGameClock);
	m_controlPtTimer->Start();

	// set the cameras
	AABB2 worldBox(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	//cameraStart.SetDimensions(Vec2(100.f, 50.f));
	m_worldCamera.SetOrthoView(worldBox);
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	// get three box areas
	m_easingBox = worldBox.GetBoxAtUVs(Vec2(0.f, 0.5f), Vec2(0.5f, 0.9f));
	m_CubicBezierBox = worldBox.GetBoxAtUVs(Vec2(0.5f, 0.5f), Vec2(1.f, 0.9f));
	m_CatmullRomBox = worldBox.GetBoxAtUVs(Vec2(0.f, 0.f), Vec2(1.f, 0.5f));

	m_easingBox.AddPadding(0.09f, 0.09f);
	m_CubicBezierBox.AddPadding(0.09f, 0.09f);
	m_CatmullRomBox.AddPadding(0.045f, 0.09f);
	AddVertsForAABB2D(m_debugVerts, m_easingBox, m_debugBoxColor);
	AddVertsForAABB2D(m_debugVerts, m_CubicBezierBox, m_debugBoxColor);
	AddVertsForAABB2D(m_debugVerts, m_CatmullRomBox, m_debugBoxColor);

	// for the easing diagram
	AddVertsForEasingDiagram();

	// get the size for the easing diagram text box
	m_diagramNameBox = m_easingBox;
	m_diagramNameBox.Translate(Vec2(0.f, m_stringHeight * -1.f));
	m_diagramNameBox.m_maxs.y = m_diagramNameBox.m_mins.y + m_stringHeight;

	CreateRandomShapes();

	// green lines
	AddVertsForAllSubdivisionCurves();

}

void Curves2DMode::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	UpdateInput();
	UpdateModeInfo();

	m_parametricallyMovingPtVerts.clear();
	if (!m_controlPtTimer->HasPeroidElapsed())
	{
		float t = m_controlPtTimer->GetElapsedTime() / m_movingDuration;
		UpdateMovingPointsOnEasingDiagram(t);
		UpdateCubicBezierDiagram(t);
		UpdateCatmullRomDiagram();
	}
	else
	{
		float t = 1.f;
		UpdateMovingPointsOnEasingDiagram(t);
		UpdateCubicBezierDiagram(t);
		UpdateCatmullRomDiagram();

		m_controlPtTimer->Restart();
	}

}

void Curves2DMode::UpdateInput()
{
	if (g_theInput->WasKeyJustPressed('W'))
	{
		m_easingDiagramIndex -= 1;

		if (m_easingDiagramIndex == -1)// switch to the last diagram
		{
			m_easingDiagramIndex = (int)(DiagramIndex::NumEasingDiagrams)-1;
		}

		AddVertsForEasingDiagram();
		AddVertsForAllSubdivisionCurves();
	}
	if (g_theInput->WasKeyJustPressed('E'))
	{
		m_easingDiagramIndex += 1;

		if (m_easingDiagramIndex == (int)(DiagramIndex::NumEasingDiagrams)) // switch to the first diagram
		{
			m_easingDiagramIndex = 0;
		}

		AddVertsForEasingDiagram();
		AddVertsForAllSubdivisionCurves();
	}

	if (g_theInput->WasKeyJustPressed('N'))
	{
		if (m_numSubdivisionSections > 1)
		{
			m_numSubdivisionSections /= 2;
			AddVertsForAllSubdivisionCurves();
		}
	}
	if (g_theInput->WasKeyJustPressed('M'))
	{
		m_numSubdivisionSections *= 2;
		AddVertsForAllSubdivisionCurves();
	}
}

void Curves2DMode::UpdateMovingPointsOnEasingDiagram(float t)
{
	// for the fixed speed moving dot
// get the speed
	float totalLength = 0.f;
	int numSamplePoints = (int)m_samplePtsForEasing.size();
	for (int i = 0; i < numSamplePoints - 1; ++i)
	{
		Vec2 disp = m_samplePtsForEasing[i + 1] - m_samplePtsForEasing[i];
		float dist = disp.GetLength();
		totalLength += dist;
	}
	float fixedSpeed = totalLength / m_movingDuration;
	float pastTime = m_movingDuration * t;
	float pastLength = fixedSpeed * pastTime;
	// get which section it is on
	int sectionIndex = 0;
	float sectionLength = 0.f;
	float distFromSectionStart = 0.f;
	for (int i = 0; i < numSamplePoints - 1; ++i)
	{
		Vec2 disp = m_samplePtsForEasing[i + 1] - m_samplePtsForEasing[i];
		float dist = disp.GetLength();
		pastLength -= dist;
		if (pastLength <= 0.f)
		{
			sectionIndex = i;
			sectionLength = dist;
			distFromSectionStart = sectionLength + pastLength;
			break;
		}
	}
	float fraction = distFromSectionStart / sectionLength;
	Vec2 ptPos = Interpolate(m_samplePtsForEasing[sectionIndex], m_samplePtsForEasing[sectionIndex + 1], fraction);
	AddVertesForDisc2D(m_parametricallyMovingPtVerts, ptPos, m_movingPtRadius, m_fixedSpeedMovingPtColor);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
		// easing functions input update
	// first get the drawing diagram background, which is a square
	// the height is the height of the easing box
	float height = m_easingBox.GetDimensions().y;
	float width = height;
	float halfHeight = height * 0.5f;
	Vec2 center = m_easingBox.GetCenter();
	Vec2 start = center - Vec2(halfHeight, halfHeight);
	Vec2 end = center + Vec2(halfHeight, halfHeight);
	AABB2 BG(start, end);

	// draw the curve according to the diagram index
	// get numSection points, and draw numSection lines 
	Vec2 dot;
	dot.x = start.x + width * t;

	switch (static_cast<DiagramIndex>(m_easingDiagramIndex))
	{
	case DiagramIndex::Indentity:
	{
		dot.y = start.y + height * t;
	}break;	
	case DiagramIndex::ElevateBounce:
	{
		dot.y = start.y + height * ElevateBounce(t);
	}break;
	case DiagramIndex::EaseInQuadratic:
	{
		dot.y = start.y + height * EaseInQuadratic(t);
	}break;
	case DiagramIndex::EaseInCubic:
	{
		dot.y = start.y + height * EaseInCubic(t);
	}break;
	case DiagramIndex::EaseInQuartic:
	{
		dot.y = start.y + height * EaseInQuartic(t);
	}break;
	case DiagramIndex::EaseInQuintic:
	{
		dot.y = start.y + height * EaseInQuintic(t);
	}break;
	case DiagramIndex::EaseInHexic:
	{
		dot.y = start.y + height * EaseInHexic(t);

	}break;
	case DiagramIndex::EaseOutQuadratic:
	{
		dot.y = start.y + height * EaseOutQuadratic(t);
	}break;
	case DiagramIndex::EaseOutCubic:
	{
		dot.y = start.y + height * EaseOutCubic(t);
	}break;
	case DiagramIndex::EaseOutQuartic:
	{
		dot.y = start.y + height * EaseOutQuartic(t);
	}break;
	case DiagramIndex::EaseOutQuintic:
	{
		dot.y = start.y + height * EaseOutQuintic(t);
	}break;
	case DiagramIndex::EaseOutHexic:
	{
		dot.y = start.y + height * EaseOutHexic(t);
	}break;
	case DiagramIndex::SmoothStep3:
	{
		dot.y = start.y + height * SmoothStep3(t);
	}break;
	case DiagramIndex::SmoothStep5:
	{
		dot.y = start.y + height * SmoothStep5(t);
	}break;
	case DiagramIndex::Hesitate3:
	{
		dot.y = start.y + height * Hesitate3(0.f, 1.f, t);
	}break;
	case DiagramIndex::Hesitate5:
	{
		dot.y = start.y + height * Hesitate5(0.f, 1.f, t);
	}break;
	case DiagramIndex::CustomFunkyEasingFunction:
	{
		dot.y = start.y + height * CustomEasing(t);
	}break;
	}

	AddVertesForDisc2D(m_parametricallyMovingPtVerts, dot, m_movingPtRadius, m_parametricallyMovingPtColor);

	// add vertical and horizontal lines
	Vec2 ptOnX(dot.x, start.y);
	Vec2 ptOnY(start.x, dot.y);
	AddVertsForLineSegment2D(m_parametricallyMovingPtVerts, ptOnX, dot, m_controlLineThickness, m_parametricallyMovingPtColor);
	AddVertsForLineSegment2D(m_parametricallyMovingPtVerts, ptOnY, dot, m_controlLineThickness, m_parametricallyMovingPtColor);
}

void Curves2DMode::UpdateCubicBezierDiagram(float t)
{
	// for the fixed speed moving dot
	// get the speed
	float totalLength = m_cubicBezierCurve->GetApproximateLength(m_numSubdivisionSections);
	float fixedSpeed = totalLength / m_movingDuration;
	float pastTime = m_movingDuration * t;
	float pastLength = fixedSpeed * pastTime;
	Vec2 ptPos = m_cubicBezierCurve->EvaluateAtApproximateDistance(pastLength, m_numSubdivisionSections);
	AddVertesForDisc2D(m_parametricallyMovingPtVerts, ptPos, m_movingPtRadius, m_fixedSpeedMovingPtColor);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// white dot
	Vec2 dot;
	dot = m_cubicBezierCurve->EvaluateAtParametric(t);
	AddVertesForDisc2D(m_parametricallyMovingPtVerts, dot, m_movingPtRadius, m_parametricallyMovingPtColor);
}

void Curves2DMode::UpdateCatmullRomDiagram()
{
	// for the fixed speed moving dot
	// get the speed
	float totalLength = m_CatmullRomCurve->GetApproximateLength(m_numSubdivisionSections);
	float fixedSpeed = totalLength / (((float)m_CatmullRomCurve->m_positions.size() - 1) * m_movingDuration);
	float pastLength = fixedSpeed * m_pastTimeOnCatmullRom;
	Vec2 ptPos = m_CatmullRomCurve->EvaluateAtApproximateDistance(pastLength, m_numSubdivisionSections);
	AddVertesForDisc2D(m_parametricallyMovingPtVerts, ptPos, m_movingPtRadius, m_fixedSpeedMovingPtColor);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	Vec2 dot;
	if (m_fractionOnCatmullRom <= float((int)m_CatmullRomCurve->m_positions.size() - 1))
	{
		m_fractionOnCatmullRom += (g_theGameClock->GetDeltaSeconds() / m_movingDuration);
		m_pastTimeOnCatmullRom += g_theGameClock->GetDeltaSeconds();
	}
	else
	{
		m_fractionOnCatmullRom = 0.f;
		m_pastTimeOnCatmullRom = 0.f;
	}

	dot = m_CatmullRomCurve->EvaluateAtParametric(m_fractionOnCatmullRom);
	AddVertesForDisc2D(m_parametricallyMovingPtVerts, dot, m_movingPtRadius, m_parametricallyMovingPtColor);
}

void Curves2DMode::CreateRandomShapes()
{
	// for Cubic Bezier curve
	m_cubicBezierVerts.clear();
	m_cubicBezierVerts.clear();
	m_controlPtVerts.clear();

	std::vector <Vec2> controlPts;
	Vec2 pt1 = g_rng->GetRandomPointInsideAABB2(m_CubicBezierBox);
	controlPts.push_back(pt1);
	Vec2 pt2 = g_rng->GetRandomPointInsideAABB2(m_CubicBezierBox);
	controlPts.push_back(pt2);
	Vec2 pt3 = g_rng->GetRandomPointInsideAABB2(m_CubicBezierBox);
	controlPts.push_back(pt3);
	Vec2 pt4 = g_rng->GetRandomPointInsideAABB2(m_CubicBezierBox);
	controlPts.push_back(pt4);

	delete m_cubicBezierCurve;
	m_cubicBezierCurve = new CubicBezierCurve2D(pt1, pt2, pt3, pt4);

	// for the white line
	AddVertsForCubicBezierCurve(m_cubicBezierVerts, m_smoothCurveColor, m_numCurveSections);
	AddVertsForControlPointsAndControlLines(controlPts, m_controlPtVerts);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// for the Catmull Rom Spline
	std::vector <Vec2> controlPts2;
	m_CatmullRomVerts.clear();

	// get random control points
	int numControlPts = g_rng->RollRandomIntInRange(8, 12);
	float sectionFraction = 1.f / numControlPts;
	for (int i = 0; i < numControlPts; ++i)
	{
		AABB2 box = m_CatmullRomBox.GetBoxAtUVs(Vec2(i * sectionFraction, 0.f), Vec2((i + 1) * sectionFraction, 1.f));
		Vec2 pt = g_rng->GetRandomPointInsideAABB2(box);
		controlPts2.push_back(pt);
	}

	// get random start velocity and end velocity
	Vec2 startVel;
	Vec2 endVel;
	startVel.x = g_rng->RollRandomFloatInFloatRange(FloatRange(-20.f, 20.f));
	startVel.y = g_rng->RollRandomFloatInFloatRange(FloatRange(-20.f, 20.f));
	endVel.x = g_rng->RollRandomFloatInFloatRange(FloatRange(-20.f, 20.f));
	endVel.y = g_rng->RollRandomFloatInFloatRange(FloatRange(-20.f, 20.f));

	delete m_CatmullRomCurve;
	m_CatmullRomCurve = new CatmullRomSpline2D(controlPts2, startVel, endVel);

	AddVertsForCubicCatmullRomCurve(m_CatmullRomVerts, numControlPts, m_smoothCurveColor, m_numCurveSections);

	AddVertsForAllSubdivisionCurves();

	AddVertsForControlPointsAndControlLines(controlPts2, m_controlPtVerts);
	AddVertsForVelocityAtControlledPoints();

}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// add verts
void Curves2DMode::AddVertsForEasingDiagram() // white line
{
	// easing functions input update
	// first get the drawing diagram background, which is a square
	// the height is the height of the easing box
	float height = m_easingBox.GetDimensions().y;
	float halfHeight = height * 0.5f;
	Vec2 center = m_easingBox.GetCenter();
	Vec2 BL = center - Vec2(halfHeight, halfHeight);
	Vec2 TR = center + Vec2(halfHeight, halfHeight);
	AABB2 BG(BL, TR);

	// clear last diagram drawing
	m_easingVerts.clear();

	AddVertsForAABB2D(m_easingVerts, BG, Rgba8::GRAY_TRANSPARENT);
	AddVertsForEasingDiagram(m_easingVerts, static_cast<DiagramIndex>(m_easingDiagramIndex), BL, TR, m_smoothCurveColor, m_numCurveSections);
}

void Curves2DMode::AddVertsForAllSubdivisionCurves() // three green curves
{
	float height = m_easingBox.GetDimensions().y;
	float halfHeight = height * 0.5f;
	Vec2 center = m_easingBox.GetCenter();
	Vec2 BL = center - Vec2(halfHeight, halfHeight);
	Vec2 TR = center + Vec2(halfHeight, halfHeight);
	AABB2 BG(BL, TR);

	// green line
	m_subdivisionSplineVerts.clear();
	m_samplePtsForEasing.clear();
	m_samplePtsForEasing = AddVertsForEasingDiagram(m_subdivisionSplineVerts, static_cast<DiagramIndex>(m_easingDiagramIndex), BL, TR, m_subdivisionSplineColor, m_numSubdivisionSections);
	m_samplePtsForCubicBezier.clear();
	m_samplePtsForCubicBezier = AddVertsForCubicBezierCurve(m_subdivisionSplineVerts, m_subdivisionSplineColor, m_numSubdivisionSections);
	m_samplePtsForCatmullRom.clear();
	m_samplePtsForCatmullRom = AddVertsForCubicCatmullRomCurve(m_subdivisionSplineVerts, (int)m_CatmullRomCurve->m_positions.size(), m_subdivisionSplineColor, m_numSubdivisionSections);
}

std::vector<Vec2> Curves2DMode::AddVertsForEasingDiagram(std::vector<Vertex_PCU>& listToAddVerts, DiagramIndex diagramIndex, Vec2 start, Vec2 end,
	Rgba8 color, int numSection)
{
	std::vector<Vec2> samplePts;
	float step = 1.f / (float)numSection;
	float height = end.y - start.y;
	float width = end.x - start.x;

	// draw the curve according to the diagram index
	// get numSection points, and draw numSection lines 
	for (int i = 0; i < numSection; ++i)
	{
		float currentFraction = step * i;
		float nextFraction = step * (i + 1);
		Vec2 thisPos;
		Vec2 nextPos;
		thisPos.x = start.x + width * currentFraction;
		nextPos.x = start.x + width * nextFraction;

		switch (diagramIndex)
		{
		case DiagramIndex::Indentity:
		{
			m_easingDiagramName = "Indentity";
			thisPos.y = start.y + height * currentFraction;
			nextPos.y = start.y + height * nextFraction;
		}break;		
		case DiagramIndex::ElevateBounce:
		{
			m_easingDiagramName = "ElevateBounce";
			thisPos.y = start.y + height * ElevateBounce(currentFraction);
			nextPos.y = start.y + height * ElevateBounce(nextFraction);
		}break;
		case DiagramIndex::EaseInQuadratic:
		{
			m_easingDiagramName = "SmoothStart2";
			thisPos.y = start.y + height * EaseInQuadratic(currentFraction);
			nextPos.y = start.y + height * EaseInQuadratic(nextFraction);
		}break;
		case DiagramIndex::EaseInCubic:
		{
			m_easingDiagramName = "SmoothStart3";
			thisPos.y = start.y + height * EaseInCubic(currentFraction);
			nextPos.y = start.y + height * EaseInCubic(nextFraction);
		}break;
		case DiagramIndex::EaseInQuartic:
		{
			m_easingDiagramName = "SmoothStart4";
			thisPos.y = start.y + height * EaseInQuartic(currentFraction);
			nextPos.y = start.y + height * EaseInQuartic(nextFraction);
		}break;
		case DiagramIndex::EaseInQuintic:
		{
			m_easingDiagramName = "SmoothStart5";
			thisPos.y = start.y + height * EaseInQuintic(currentFraction);
			nextPos.y = start.y + height * EaseInQuintic(nextFraction);
		}break;
		case DiagramIndex::EaseInHexic:
		{
			m_easingDiagramName = "SmoothStart6";
			thisPos.y = start.y + height * EaseInHexic(currentFraction);
			nextPos.y = start.y + height * EaseInHexic(nextFraction);
		}break;
		case DiagramIndex::EaseOutQuadratic:
		{
			m_easingDiagramName = "SmoothStop2";
			thisPos.y = start.y + height * EaseOutQuadratic(currentFraction);
			nextPos.y = start.y + height * EaseOutQuadratic(nextFraction);
		}break;
		case DiagramIndex::EaseOutCubic:
		{
			m_easingDiagramName = "SmoothStop3";
			thisPos.y = start.y + height * EaseOutCubic(currentFraction);
			nextPos.y = start.y + height * EaseOutCubic(nextFraction);
		}break;
		case DiagramIndex::EaseOutQuartic:
		{
			m_easingDiagramName = "SmoothStop4";
			thisPos.y = start.y + height * EaseOutQuartic(currentFraction);
			nextPos.y = start.y + height * EaseOutQuartic(nextFraction);
		}break;
		case DiagramIndex::EaseOutQuintic:
		{
			m_easingDiagramName = "SmoothStop5";
			thisPos.y = start.y + height * EaseOutQuintic(currentFraction);
			nextPos.y = start.y + height * EaseOutQuintic(nextFraction);
		}break;
		case DiagramIndex::EaseOutHexic:
		{
			m_easingDiagramName = "SmoothStop6";
			thisPos.y = start.y + height * EaseOutHexic(currentFraction);
			nextPos.y = start.y + height * EaseOutHexic(nextFraction);
		}break;
		case DiagramIndex::SmoothStep3:
		{
			m_easingDiagramName = "SmoothStep3";
			thisPos.y = start.y + height * SmoothStep3(currentFraction);
			nextPos.y = start.y + height * SmoothStep3(nextFraction);
		}break;
		case DiagramIndex::SmoothStep5:
		{
			m_easingDiagramName = "SmoothStep5";
			thisPos.y = start.y + height * SmoothStep5(currentFraction);
			nextPos.y = start.y + height * SmoothStep5(nextFraction);
		}break;
		case DiagramIndex::Hesitate3:
		{
			m_easingDiagramName = "Hesitate3";
			thisPos.y = start.y + height * Hesitate3(0.f, 1.f, currentFraction);
			nextPos.y = start.y + height * Hesitate3(0.f, 1.f, nextFraction);
		}break;
		case DiagramIndex::Hesitate5:
		{
			m_easingDiagramName = "Hesitate5";
			thisPos.y = start.y + height * Hesitate5(0.f, 1.f, currentFraction);
			nextPos.y = start.y + height * Hesitate5(0.f, 1.f, nextFraction);
		}break;
		case DiagramIndex::CustomFunkyEasingFunction:
		{
			m_easingDiagramName = "CustomFunkyEasingFunction";
			thisPos.y = start.y + height * CustomEasing(currentFraction);
			nextPos.y = start.y + height * CustomEasing(nextFraction);
		}break;
		}

		samplePts.push_back(thisPos);
		// get the last point
		if (i == (numSection-1))
		{
			samplePts.push_back(nextPos);
		}

		// each each line segments
		AddVertsForLineSegment2D(listToAddVerts, thisPos, nextPos, m_smoothLineThickness, color);
	}

	return samplePts;
}


std::vector<Vec2> Curves2DMode::AddVertsForCubicBezierCurve(std::vector<Vertex_PCU>& listToAddVerts, Rgba8 color /*= Rgba8::GRAY*/, int numSection /*= 64*/)
{
	float step = 1.f / numSection;
	std::vector<Vec2> samplePts;

	for (int i = 0; i < numSection; ++i)
	{
		float currentFraction = step * i;
		float nextFraction = step * (i + 1);
		Vec2 thisPos = m_cubicBezierCurve->EvaluateAtParametric(currentFraction);
		Vec2 nextPos = m_cubicBezierCurve->EvaluateAtParametric(nextFraction);
		samplePts.push_back(thisPos);
		// get the last point
		if (i == (numSection - 1))
		{
			samplePts.push_back(nextPos);
		}
		AddVertsForLineSegment2D(listToAddVerts, thisPos, nextPos, m_smoothLineThickness, color);
	}

	return samplePts;
}

std::vector<Vec2> Curves2DMode::AddVertsForCubicCatmullRomCurve(std::vector<Vertex_PCU>& listToAddVerts, int numControlPts, Rgba8 color /*= Rgba8::GRAY*/, int numSection /*= 64*/)
{
	float step = 1.f / numSection;
	std::vector<Vec2> samplePts;

	for (int i = 0; i < ((numControlPts - 1) * numSection); ++i)
	{
		float currentFraction = step * i;
		float nextFraction = step * (i + 1);
		Vec2 thisPos = m_CatmullRomCurve->EvaluateAtParametric(currentFraction);
		Vec2 nextPos = m_CatmullRomCurve->EvaluateAtParametric(nextFraction);

		samplePts.push_back(thisPos);
		// get the last point
		if (i == ((numControlPts - 1) * numSection - 1))
		{
			samplePts.push_back(nextPos);
		}

		AddVertsForLineSegment2D(listToAddVerts, thisPos, nextPos, m_smoothLineThickness, color);
	}

	return samplePts;
}

void Curves2DMode::UpdateModeInfo()
{
	m_easingFunctionVerts.clear();
	g_consoleFont->AddVertsForTextInBox2D(m_easingFunctionVerts, m_easingDiagramName, m_diagramNameBox, m_stringHeight * 0.9f, Vec2(0.5f, 0.5f),
			Rgba8::GREEN, 0.f);

	m_modeName = "Mode (F6 / F7 for prev / next): Easing, Curves, Splines(2D)";
	m_controlInstruction = Stringf("F8 to randomize; W/E = prev/next Easing function; N/M = curve subdivisions (%i), Hold T to slow down movement", m_numSubdivisionSections);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// render
void Curves2DMode::Render() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	RenderEasingDiagram();
	RenderCubicBezierDiagram();
	RenderCatmullRomDiagram();

	RenderSubdivisionSpline();

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	RenderControlPoints();
	RenderMovingPoints();

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(&g_consoleFont->GetTexture());
	RenderDiagramName();

	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void Curves2DMode::RenderDebug() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	g_theRenderer->DrawVertexArray((int)m_debugVerts.size(), m_debugVerts.data());

	g_theRenderer->EndCamera(m_worldCamera);
}

void Curves2DMode::RenderEasingDiagram() const
{
	g_theRenderer->DrawVertexArray((int)m_easingVerts.size(), m_easingVerts.data());
}

void Curves2DMode::RenderCubicBezierDiagram() const
{
	g_theRenderer->DrawVertexArray((int)m_cubicBezierVerts.size(), m_cubicBezierVerts.data());
}

void Curves2DMode::RenderCatmullRomDiagram() const
{
	g_theRenderer->DrawVertexArray((int)m_CatmullRomVerts.size(), m_CatmullRomVerts.data());
}

void Curves2DMode::RenderMovingPoints() const
{
	g_theRenderer->DrawVertexArray((int)m_parametricallyMovingPtVerts.size(), m_parametricallyMovingPtVerts.data());
}

void Curves2DMode::RenderSubdivisionSpline() const
{
	g_theRenderer->DrawVertexArray((int)m_subdivisionSplineVerts.size(), m_subdivisionSplineVerts.data());
}

void Curves2DMode::RenderControlPoints() const
{
	g_theRenderer->DrawVertexArray((int)m_controlPtVerts.size(), m_controlPtVerts.data());
}

void Curves2DMode::RenderDiagramName() const
{
	g_theRenderer->DrawVertexArray((int)m_easingFunctionVerts.size(), m_easingFunctionVerts.data());
}

void Curves2DMode::Shutdown()
{

}

void Curves2DMode::AddVertsForControlPointsAndControlLines(std::vector<Vec2> pts, std::vector<Vertex_PCU>& listToAddVerts)
{
	for (int i = 0; i < (int)pts.size(); ++i)
	{
		// control lines
		if (i <= ((int)pts.size() - 2))
		{
			AddVertsForLineSegment2D(listToAddVerts, pts[i], pts[i + 1], m_controlLineThickness, m_controlLineColor);
		}
	}

	for (int i = 0; i < (int)pts.size(); ++i)
	{
		// control points
		AddVertesForDisc2D(listToAddVerts, pts[i], m_controlPtRadius, m_controlPtColor);		
	}
}

void Curves2DMode::AddVertsForVelocityAtControlledPoints()
{
	std::vector<Vec2>& velocities = m_CatmullRomCurve->m_velocities;
	std::vector<Vec2>& posList = m_CatmullRomCurve->m_positions;

	for (int i = 0; i < (int)posList.size(); ++i)
	{
		Vec2 arrowTipPos = posList[i] + velocities[i];
		AddVertsForArrow2D(m_CatmullRomVerts, posList[i], arrowTipPos, m_arrowSize, m_arrowLineThickness, m_arrowColor);
	}
}

