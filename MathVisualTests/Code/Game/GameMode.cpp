#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameMode.hpp"
#include "Game/HeatmapAssessmentMode.hpp"
#include "Game/GetNearestPointsMode.hpp"
#include "Game/RaycastVSDiscsMode.hpp"
#include "Game/RaycastVSLineSegmentsMode.hpp"
#include "Game/RaycastVSAABB2Mode.hpp"
#include "Game/RaycastVSConvex2Mode.hpp"
#include "Game/Shapes3DMode.hpp"
#include "Game/QuaternionMode.hpp"
#include "Game/Curves2DMode.hpp"
#include "Game/Pachinko2D.hpp"
#include "Game/SecondOrderDynamicsMode.hpp"

extern InputSystem* g_theInput;

GameMode::GameMode()
{
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));
}

GameMode::~GameMode()
{

}

void GameMode::RenderDebug() const
{

}

void GameMode::CreateRandomShapes()
{

}

void GameMode::RenderScreenMessage() const
{
	AABB2 bounds = m_screenCamera.GetCameraBounds();
	float screenHeigt = bounds.m_maxs.y - bounds.m_mins.y;
	float textLineHeight = screenHeigt / m_gameModeConfig.m_numMessageOnScreen;
	float fontHeight = m_gameModeConfig.m_lineHeightAndTextBoxRatio * textLineHeight;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	std::vector<Vertex_PCU> messageVerts;
	// int linesLeftToDrawOnScreen = m_gameModeConfig.m_numMessageOnScreen;
	// get the bounds for fist interface text line
	AABB2 currentLineBox(Vec2(bounds.m_mins.x, (bounds.m_maxs.y - textLineHeight)), Vec2(bounds.m_maxs.x, bounds.m_maxs.y));
	m_gameModeConfig.m_font->AddVertsForTextInBox2D(messageVerts, m_modeName, currentLineBox, fontHeight, Vec2(0.f, 0.5f),
		m_modeNameLineColor, 0.3f, m_gameModeConfig.m_cellAspect, TextDrawMode::OVERRUN);

	currentLineBox.Translate(Vec2(0.f, (textLineHeight * -2.f)));
	currentLineBox.m_maxs.y = currentLineBox.m_mins.y + textLineHeight * 2.f;
	m_gameModeConfig.m_font->AddVertsForTextInBox2D(messageVerts, m_controlInstruction, currentLineBox, fontHeight, Vec2(0.f, 0.5f),
		m_instructionLineColor, 0.2f, m_gameModeConfig.m_cellAspect, TextDrawMode::OVERRUN);	
		
	currentLineBox.Translate(Vec2(0.f, (textLineHeight * -2.f)));
	currentLineBox.m_maxs.y = currentLineBox.m_mins.y + textLineHeight * 2.f;
	m_gameModeConfig.m_font->AddVertsForTextInBox2D(messageVerts, m_testString, currentLineBox, fontHeight, Vec2(0.f, 0.5f),
		m_testLineColor, 0.2f, m_gameModeConfig.m_cellAspect, TextDrawMode::OVERRUN);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	m_gameModeConfig.m_renderer->BindTexture(&m_gameModeConfig.m_font->GetTexture());
	m_gameModeConfig.m_renderer->DrawVertexArray((int)messageVerts.size(), messageVerts.data());
}

GameMode* GameMode::CreateNewGame(TestingScene type)
{
	switch (type)
	{
	case HEATMAP_ASSESSMENT: return new HeatmapAssessmentMode();
	case PACHINKO: return new Pachinko2D();
	case CURVES2D: return new Curves2DMode();
	case QUATERNION: return new QuaternionMode();
	case SECOND_ORDER_DYNAMICS: return new SecondOrderDynamicsMode();
	case RAYCAST_VS_3DSHAPES: return new Shapes3DMode();
	case RAYCAST2D_VS_AABB2S: return new RaycastVSAABB2Mode();
	case RAYCAST2D_VS_CONVEX2S: return new RaycastVSConvex2Mode();
	case RAYCAST2D_VS_LINESEGMENTS: return new RaycastVsLineSegmentMode();
	case GET_NEARESTPOINT: return new GetNearestPointsMode();
	case RAYCAST2D_VS_DISCS: return new RaycastVSDiscsMode();
	}
	return nullptr;
}

void GameMode::ControlTheReferenceRay(float deltaSeconds)
{
	if (g_theInput->IsKeyDown('E') || g_theInput->IsKeyDown(KEYCODE_UPARROW))
	{
		m_tailPos += Vec2(0.f, PLAYERSHIP_ACCERATION_LOW * deltaSeconds);
	}

	// orientation calculation according to player's control
	if (g_theInput->IsKeyDown('S') || g_theInput->IsKeyDown(KEYCODE_LEFTARROW))
	{
		m_tailPos += Vec2(-PLAYERSHIP_ACCERATION_LOW * deltaSeconds, 0.f);
	}

	if (g_theInput->IsKeyDown('F') || g_theInput->IsKeyDown(KEYCODE_RIGHTARROW))
	{
		m_tailPos += Vec2(PLAYERSHIP_ACCERATION_LOW * deltaSeconds, 0.f);
	}

	if (g_theInput->IsKeyDown('D') || g_theInput->IsKeyDown(KEYCODE_DOWNARROW))
	{
		m_tailPos += Vec2(0.f, -PLAYERSHIP_ACCERATION_LOW * deltaSeconds);
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------d

	if (g_theInput->IsKeyDown('I') || g_theInput->IsKeyDown(KEYCODE_UPARROW))
	{
		m_tipPos += Vec2(0.f, PLAYERSHIP_ACCERATION_LOW * deltaSeconds);
	}
	// orientation calculation according to player's control
	if (g_theInput->IsKeyDown('J') || g_theInput->IsKeyDown(KEYCODE_LEFTARROW))
	{
		m_tipPos += Vec2(-PLAYERSHIP_ACCERATION_LOW * deltaSeconds, 0.f);
	}

	if (g_theInput->IsKeyDown('L') || g_theInput->IsKeyDown(KEYCODE_RIGHTARROW))
	{
		m_tipPos += Vec2(PLAYERSHIP_ACCERATION_LOW * deltaSeconds, 0.f);
	}

	if (g_theInput->IsKeyDown('K') || g_theInput->IsKeyDown(KEYCODE_DOWNARROW))
	{
		m_tipPos += Vec2(0.f, -PLAYERSHIP_ACCERATION_LOW * deltaSeconds);
	}

	// mouse control
	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		Vec2 mouseUV = g_theInput->GetNormalizedCursorPos();
		Vec2 mousePosInWorld;
		mousePosInWorld.x = RangeMap(mouseUV.x, 0.f, 1.f, 0.f, WORLD_SIZE_X);
		mousePosInWorld.y = RangeMap(mouseUV.y, 0.f, 1.f, 0.f, WORLD_SIZE_Y);
		m_tailPos = mousePosInWorld;
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE))
	{
		Vec2 mouseUV = g_theInput->GetNormalizedCursorPos();
		Vec2 mousePosInWorld;
		mousePosInWorld.x = RangeMap(mouseUV.x, 0.f, 1.f, 0.f, WORLD_SIZE_X);
		mousePosInWorld.y = RangeMap(mouseUV.y, 0.f, 1.f, 0.f, WORLD_SIZE_Y);
		m_tipPos = mousePosInWorld;
	}

	if (g_theInput->IsKeyDown('X'))
	{
		m_tailPos.x = m_tipPos.x;
	}

	if (g_theInput->IsKeyDown('Y'))
	{
		m_tailPos.y = m_tipPos.y;
	}
}

void GameMode::UpdateMouseInfo()
{
	m_mousePosLastFrame = m_mousePosCurrentFrame;
	m_mousePosCurrentFrame = GetMousePositionInWorld();
}

Vec2 GameMode::GetMousePositionInWorld()
{
	Vec2 mouseUV = g_theInput->GetNormalizedCursorPos();
	Vec2 mousePosInWorld;
	mousePosInWorld.x = RangeMap(mouseUV.x, 0.f, 1.f, 0.f, WORLD_SIZE_X);
	mousePosInWorld.y = RangeMap(mouseUV.y, 0.f, 1.f, 0.f, WORLD_SIZE_Y);
	return mousePosInWorld;
}

Vec2 GameMode::GetMouseDispThisFrame()
{
	return (m_mousePosCurrentFrame - m_mousePosLastFrame);
}

