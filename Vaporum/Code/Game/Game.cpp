#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/Plane3.hpp"
#include "Engine/Model/Material.hpp"
#include "Engine/UI/Widget.hpp"
#include "Engine/Net/NetSystem.hpp"
#include "Engine/VFX/EffectSystem.hpp"
#include "Game/Player.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Model.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Game/Prop.hpp"
#include "Game/UI.hpp"
#include "Game/App.hpp"
#include "Game/unit.hpp"
#include <iostream>
#include <Windows.h>

#undef OPAQUE // this is cause because we have OPAQUE for blend mode and it is defined by windows.h

RandomNumberGenerator* g_rng = nullptr; // always initialize the global variable in the cpp file
Clock* g_theGameClock = nullptr;
Clock* g_theColorChangingClock = nullptr;

extern App* g_theApp;
extern InputSystem* g_theInput; 
extern AudioSystem* g_theAudio;
extern Renderer* g_theRenderer;
extern Window* g_theWindow;
extern EffectSystem* g_theEffectSystem;
extern Game* g_theGame;
extern NetSystem* g_theNetSystem;

extern Texture* g_textures[NUM_TEXTURES];
extern Material* g_materials[NUM_MATERIALS];
extern SpriteAnimDefinition* g_spriteAnims[NUM_SPRITEANIMS];
extern BitmapFont* g_consoleFont;

Game::Game()
{
}

Game::~Game()
{

}

void Game::Startup()
{
	g_rng = new RandomNumberGenerator;

	SubscribeEventCallbackFunction("LoadMap", Game::Event_LoadMapByInputName);

	// create a player and set it up
	Player* player = new Player();
	player->Startup();
	m_players.push_back(player);	
	g_theDebugRenderConfig.m_camera = &player->m_playerCamera;
	
	Player* player2 = new Player();
	player2->Startup();
	m_players.push_back(player2);

	IntVec2 appResolution = g_theWindow->GetWindowDimensions();
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2((float)appResolution.x, (float)appResolution.y));

	// set up the clock for the game 
	g_theGameClock = new Clock();

	SpawnProps();

	m_phongLighinting = new PhongLightingConstants();

	m_FPSTimer = new Timer(0.15f);
	m_FPSTimer->Start();

	// DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(m_testSceneIndex));

	// read XML info
	TileTypeDefinition::InitializeTileDefs();
	MapDefinition::InitializeMapDefs();
	UnitDefinition::InitializeUnitDefs();

	CreateVertexIndexBufferForGroundAndCopyFromCPUtoGPU();

	m_playerUnitColorMap[player] = Rgba8::PURPLE_BLUE;
	m_playerUnitColorMap[player2] = Rgba8::DEEP_ORANGE;		
	
	m_playerSelectedUnitColorMap[player] = Rgba8::CYSTAL_BLUE;
	m_playerSelectedUnitColorMap[player2] = Rgba8::BRIGHT_ORANGE;
	
	m_playerHexColorMap[player] = Rgba8::TEAL_BLUE;
	m_playerHexColorMap[player2] = Rgba8::LIGHT_ORANGE;

	ChangeGameState(GameState::SPLASH_SCREEN);

	CreatePauseMenuWidget();
	CreatePlayingInstructionWidget();

	CreateEndTurnWidget();
	m_endTurnWidget->m_enabled = false;

	CreateWinnerWidget();

	CreateWaitingForPlayersWidget();
	m_waitingWidget->m_enabled = false;

	ScribeNetworkCommands();
}

void Game::Update()
{
	switch (m_currentGameState)
	{
	case GameState::NONE:
		break;
	case GameState::SPLASH_SCREEN:
	{
		UpdateSplashScreen();
	}
		break;
	case GameState::MAIN_MENU:
	{
		UpdateMainMenu();
	}break;	
	case GameState::PAUSED:
	{
		UpdatePauseMenu();
	}break;
	case GameState::GAME_START:
	{		
	}break;
	case GameState::WAITING_FOR_PLAYERS:
	{
		UpdateWaitingForPlayers();
	}break;
	case GameState::TURN_START:
	{
		UpdateTurnStart();
	}break;
	case GameState::PLAYING:
	{
		UpdatePlaying();
	}break;
	case GameState::GAMEOVER:
	{
		UpdateGameOver();
	}break;
	case GameState::NUM_GAMESTATE:
		break;
	default:
		break;
	}

	if (GetDebugRenderVisibility())
	{
		UpdateDebugRenderMessages();
	}
}

void Game::UpdateDebugRenderMessages()
{
	Vec2  textPos = Vec2(1.f, 0.f);
	Vec2  spacing = Vec2(0.f, 0.04f);

	Vec2  fpsAlignment = Vec2(0.99f, 0.98f);
	Vec2  sunOrientAlignment = fpsAlignment - spacing;
	Vec2  sceneAlignment = sunOrientAlignment - spacing;
	Vec2  sunDirectionAlignment = sceneAlignment - spacing;
	Vec2  sunIntensityAlignment = sunDirectionAlignment - spacing;
	Vec2  ambientIntensityAlignment = sunIntensityAlignment - spacing;

	Vec2  renderAmbientAlignment = ambientIntensityAlignment - spacing;
	Vec2  renderDiffuseAlignment = renderAmbientAlignment - spacing;
	Vec2  renderSpecularAlignment = renderDiffuseAlignment - spacing;
	Vec2  renderEmissiveAlignment = renderSpecularAlignment - spacing;

	Vec2  useDiffuseMapAlignment = renderEmissiveAlignment - spacing;
	Vec2  useNormalMapAlignment = useDiffuseMapAlignment - spacing;
	Vec2  useSpecularMapAlignment = useNormalMapAlignment - spacing;
	Vec2  useGlossinessMapAlignment = useSpecularMapAlignment - spacing;
	Vec2  useEmissiveMapAlignment = useGlossinessMapAlignment - spacing;

	// float fontSize = 24.f;

	// if (m_FPSTimer->DecrementPeroidIfElapsed())
	// {
	// 	m_FPSString = Stringf("FPS: %s", std::to_string(Clock::GetSystemClock().GetFrameRatePerSecond()).c_str());
	// }
	// DebugAddScreenText(m_FPSString, Vec2(), fontSize, fpsAlignment, -1.f);

	auto iter = m_entities.find("Sun");
	if (iter != m_entities.end())
	{
		Entity* sun = iter->second;

	// sun orientation
	float yaw = sun->m_orientation.m_yawDegrees;
	float pitch = sun->m_orientation.m_pitchDegrees;
	float roll = sun->m_orientation.m_rollDegrees;
	if (yaw < 0.f)
	{
		yaw += 360.f;
	}	
	if (pitch < 0.f)
	{
		pitch += 360.f;
	}	
	if (roll < 0.f)
	{
		roll += 360.f;
	}
	if (yaw > 360.f)
	{
		yaw += 360.f;
	}
	if (pitch > 360.f)
	{
		pitch += 360.f;
	}
	if (roll > 360.f)
	{
		roll += 360.f;
	}
	// std::string sunOrientation = Stringf("Sun orientation (Arrows): ( %.1f, %.1f, %.1f )", yaw, pitch, roll);
	// DebugAddScreenText(sunOrientation, Vec2(), fontSize, sunOrientAlignment, -1.f);	
	
	// slot 2
	// if (m_currentMap)
	// {
	// 	// std::string pathFinding = Stringf("Movement Path Size: %i", (int)m_currentMap->m_unitMovementPath.size());
	// 	std::string pathFinding = Stringf("Movement Path Size: %i", m_currentMap->m_moveTargetTileIndexLastTime);
	// 	DebugAddScreenText(pathFinding, Vec2(), fontSize, sunOrientAlignment, -1.f);
	// }
	
	// scene
	// std::string sceneIndex = Stringf("Scene ([ / ]): %i", m_testSceneIndex);
	// DebugAddScreenText(sceneIndex, Vec2(), fontSize, sceneAlignment, -1.f);
	 
	// Vec3 raycastPos = m_playerMouseRaycastResult.m_impactPos; 
	// std::string mouseHitPos = Stringf("Mouse Raycast Position: ( %.2f, %.2f, %.2f )", raycastPos.x, raycastPos.y, raycastPos.z);
	// DebugAddScreenText(mouseHitPos, Vec2(), fontSize, sceneAlignment, -1.f, Rgba8::RED, Rgba8::RED);

	// // sun direction
	// std::string sunDirection = Stringf("Sun Direction (Arrows): ( %.1f, %.1f, %.1f )", m_phongLighinting->SunDirection.x, m_phongLighinting->SunDirection.y, m_phongLighinting->SunDirection.z);
	// DebugAddScreenText(sunDirection, Vec2(), fontSize, sunDirectionAlignment, -1.f);	
	
	// std::string rayForwardNormal = Stringf("Mouse Raycast Normal: ( %.2f, %.2f, %.2f )", m_rayForwardNormal.x, m_rayForwardNormal.y, m_rayForwardNormal.z);
	// DebugAddScreenText(rayForwardNormal, Vec2(), fontSize, sunDirectionAlignment, -1.f, Rgba8::RED, Rgba8::RED);	
	// 
	// // sun intensity
	// std::string sunIntensity = Stringf("Sun Intensity (< / >): %.1f", m_phongLighinting->SunIntensity);
	// DebugAddScreenText(sunIntensity, Vec2(), fontSize, sunIntensityAlignment, -1.f);	
	// 
	// // ambient intensity
	// std::string ambientIntensity = Stringf("Ambient Intensity (< / >): %.1f", m_phongLighinting->AmbientIntensity);
	// DebugAddScreenText(ambientIntensity, Vec2(), fontSize, ambientIntensityAlignment, -1.f);	
	// 
	// // render ambient 
	// std::string renderAmbient = Stringf("Render Ambient [1]: %s", m_phongLighinting->lighingDebug.RenderAmbient? "On" : "Off");
	// DebugAddScreenText(renderAmbient, Vec2(), fontSize, renderAmbientAlignment, -1.f);	
	// 
	// // render Diffuse 
	// std::string renderDiffuse = Stringf("Render Diffuse [2]: %s", m_phongLighinting->lighingDebug.RenderDiffuse? "On" : "Off");
	// DebugAddScreenText(renderDiffuse, Vec2(), fontSize, renderDiffuseAlignment, -1.f);	
	// 
	// // render Specular
	// std::string renderSpecular = Stringf("Render Specular [3]: %s", m_phongLighinting->lighingDebug.RenderSpecular? "On" : "Off");
	// DebugAddScreenText(renderSpecular, Vec2(), fontSize,renderSpecularAlignment, -1.f);
	// 
	// // render Emissive
	// std::string renderEmissive = Stringf("Render Emissive [4]: %s", m_phongLighinting->lighingDebug.RenderEmissive? "On" : "Off");
	// DebugAddScreenText(renderEmissive, Vec2(), fontSize,renderEmissiveAlignment, -1.f);	
	// 
	// // Diffuse Map
	// std::string diffuseMap = Stringf("Diffuse Map [5]: %s", m_phongLighinting->lighingDebug.UseDiffuseMap? "On" : "Off");
	// DebugAddScreenText(diffuseMap, Vec2(), fontSize, useDiffuseMapAlignment, -1.f);	
	// 
	// normal Map
	// std::string normalMap = Stringf("Normal Map [6]: %s", m_phongLighinting->lighingDebug.UseNormalMap? "On" : "Off");
	// DebugAddScreenText(normalMap, Vec2(), fontSize, useNormalMapAlignment, -1.f);	
	// 
	// // Specular Map
	// std::string specularMap = Stringf("Specular Map [7]: %s", m_phongLighinting->lighingDebug.UseSpecularMap? "On" : "Off");
	// DebugAddScreenText(specularMap, Vec2(), fontSize, useSpecularMapAlignment, -1.f);	
	// 
	// // Glossiness Map
	// std::string glossinessMap = Stringf("Glossiness Map [8]: %s", m_phongLighinting->lighingDebug.UseGlossinessMap? "On" : "Off");
	// DebugAddScreenText(glossinessMap, Vec2(), fontSize, useGlossinessMapAlignment, -1.f);	
	// 
	// // Emissive Map
	// std::string emissiveMap = Stringf("Emissive Map [9]: %s", m_phongLighinting->lighingDebug.UseEmissiveMap? "On" : "Off");
	// DebugAddScreenText(emissiveMap, Vec2(), fontSize, useEmissiveMapAlignment, -1.f);
	}
}



void Game::Render()
{	
	switch (m_currentGameState)
	{
	case GameState::NONE:
		break;
	case GameState::SPLASH_SCREEN:
	{
		RenderSplashScreen();
	}break;	
	case GameState::WAITING_FOR_PLAYERS:
	{
		RenderWaitingForPlayers();
	}break;
	case GameState::MAIN_MENU:
	{
		RenderMainMenu();
	}break;
	case GameState::GAME_START:
		break;

	case GameState::TURN_START:
	{
		RenderWorldInPlayerCamera();

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// use screen camera to render all UI elements
		g_theRenderer->BeginCamera(m_screenCamera);

		RenderWidgetIfIsValidAndEnabled(m_turnStartWidget);
		RenderWidgetIfIsValidAndEnabled(m_edgeTurnWidget);

		if (GetDebugRenderVisibility())
		{
			// render the messages on the screen
			DebugRenderScreen(m_screenCamera);
		}
		g_theRenderer->EndCamera(m_screenCamera);

	}break;
	case GameState::PLAYING:
	{
		RenderWorldInPlayerCamera();

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// Render Overlay on top Hexagons
		m_selectedHexagonsVertexs.clear();
		if (m_selectedHexIndex != INVALID_HEX_INDEX)
		{
			AddToVertsForPlayerSelectedHexagons(m_selectedHexIndex);
		}

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// use screen camera to render all UI elements
		g_theRenderer->BeginCamera(m_screenCamera);

		RenderWidgetIfIsValidAndEnabled(m_edgeTurnWidget);
		RenderWidgetIfIsValidAndEnabled(m_unitWidget_player1);
		RenderWidgetIfIsValidAndEnabled(m_unitWidget_player2);
		RenderWidgetIfIsValidAndEnabled(m_endTurnWidget);

		// render the playing instruction widget when 
		if (m_currentTurnPlayerIndex != INVALID_PLAYER_INDEX)
		{
			if (m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
			{
				RenderWidgetIfIsValidAndEnabled(m_playInstructionWidget);
			}
		}

		if (GetDebugRenderVisibility())
		{
			// render the messages on the screen
			DebugRenderScreen(m_screenCamera);
		}
		g_theRenderer->EndCamera(m_screenCamera);
	}break;
	case GameState::PAUSED:
	{
		g_theRenderer->BeginCamera(m_screenCamera);

		RenderWidgetIfIsValidAndEnabled(m_pauseMenuWidget);

		g_theRenderer->EndCamera(m_screenCamera);

	}break;
	case GameState::GAMEOVER:
	{
		RenderWorldInPlayerCamera();

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// use screen camera to render all UI elements
		g_theRenderer->BeginCamera(m_screenCamera);

		RenderWidgetIfIsValidAndEnabled(m_winnerWidget_0);
		RenderWidgetIfIsValidAndEnabled(m_winnerWidget_1);

	}break;
	case GameState::NUM_GAMESTATE:
		break;
	default:
		break;
	}
}

// use player camera to render entities in the world
void Game::RenderWorldInPlayerCamera()
{
	if (m_currentTurnPlayerIndex != INVALID_PLAYER_INDEX)
	{
		int playerIndex = m_currentTurnPlayerIndex;

		// we are always controlling the local player camera in network mode
		if (IsNetworkGameMode())
		{
			if (m_players[0]->m_netState == NetState::LOCAL)
			{
				playerIndex = 0;
			}
			else if (m_players[1]->m_netState == NetState::LOCAL)
			{
				playerIndex = 1;
			}
		}

			Camera& playerCamera = m_players[playerIndex]->m_playerCamera;

			g_theEffectSystem->UpdateTrackingCamera(playerCamera);
			g_theEffectSystem->Update();

			g_theRenderer->BeginCamera(playerCamera);
			g_theRenderer->ClearScreen(Rgba8::GRAY_Dark);//the background color setting of the window

			RenderGroundPlane();

			// render game world
			// if (!m_testSceneProps.empty())
			// {
			// 	for (Entity* entity : m_testSceneProps)
			// 	{
			// 		entity->Render();
			// 	}
			// }
			// if (!m_loadedModels.empty())
			// {
			// 	for (auto model : m_loadedModels)
			// 	{
			// 		model->Render();
			// 	}
			// }
			if (!m_entities.empty())
			{
				for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
				{
					std::string name = it->first;
					Entity* entity = it->second;
					entity->Render();
				}
			}


			if (m_currentMap)
			{
				m_currentMap->Render();
			}

			RenderSelectedHex();

			if (m_currentMap)
			{
				m_currentMap->RenderUnits();
			}

			g_theEffectSystem->Render();

			// debug render
			DebugRenderWorld(playerCamera);
			g_theRenderer->EndCamera(playerCamera);
	}
}

void Game::RenderGroundPlane()
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetModelConstants(Mat44());

	g_theRenderer->BindDiffuseSpecularNormalTextures(g_materials[MOON]->m_diffuseTexture, g_materials[MOON]->m_specGlossTexture, g_materials[MOON]->m_normalTexture);
	g_theRenderer->BindShader(g_materials[MOON]->m_shader);
	g_theRenderer->SetPhongLightingConstants(*m_phongLighinting);

	g_theRenderer->DrawVertexArrayWithIndexArray(m_groundVertexBuffer, m_groundIndexBuffer, (int)(m_groundIndexArray.size()));
}

void Game::SpawnProps()
{
	g_theColorChangingClock = new Clock(*g_theGameClock);
	g_theColorChangingClock->SetTimeScale(0.5f);

	// // add a cube prop
	// Prop* bigCubeProp = new Prop();
	// bigCubeProp->m_name = "First Big Cube";
	// AABB3 bigCube = AABB3(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f));
	// AABB3 smallCube = bigCube * 0.25f;
	// bigCubeProp->m_position = Vec3(2.f, 2.f, 0.f);
	// AddVertsForAABB3D(bigCubeProp->m_unlitVertexes, bigCube,
	// 	Rgba8::RED, Rgba8::CYAN, Rgba8::GREEN, Rgba8::MAGENTA, Rgba8::BLUE, Rgba8::YELLOW);
	// m_entities.push_back(bigCubeProp);
	// 
	// // a copy of the cube
	// Prop* bigCubeCopy = new Prop();
	// bigCubeCopy->m_name = "Second Big Cube";
	// bigCubeCopy->m_position = Vec3(-2.f, -2.f, 0.f);
	// AddVertsForAABB3D(bigCubeCopy->m_unlitVertexes, bigCube,
	// 	Rgba8::RED, Rgba8::CYAN, Rgba8::GREEN, Rgba8::MAGENTA, Rgba8::BLUE, Rgba8::YELLOW);
	// m_entities.push_back(bigCubeCopy);
	// 
	// // add a sphere of rotating about its world axis
	// Prop* sphere = new Prop();
	// sphere->m_name = "Sphere";
	// sphere->m_position = Vec3(10.f, -5.f, 1.f);
	// sphere->m_unlitTexture = g_theApp->g_textures[TESTUV];
	// AddVertsForSphere3D(sphere->m_unlitVertexes, Vec3(), 1.f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 16, 8);
	// m_entities.push_back(sphere);
	// 
	// // add a cylinder of rotating about world z axis
	// Prop* cylinder = new Prop();
	// cylinder->m_name = "Cylinder";
	// cylinder->m_position = Vec3(10.f, 5.f, 1.f);
	// cylinder->m_unlitTexture = g_theApp->g_textures[TESTUV];
	// AddVertsForCylinder3D(cylinder->m_unlitVertexes, Vec3(0.f, 0.f, -1.5f), Vec3(0.f, 0.f, 1.5f), 1.5f);
	// m_entities.push_back(cylinder);
	// 
	// // add a cone of rotating about world z axis
	// Prop* cone = new Prop();
	// cone->m_name = "Cone";
	// cone->m_position = Vec3(10.f, 0.f, 1.f);
	// cone->m_unlitTexture = g_theApp->g_textures[TESTUV];
	// AddVertsForCone3D(cone->m_unlitVertexes, Vec3(-1.5f, 0.f, 0.f), Vec3(1.5f, 0.f, 0.f), 1.5f);
	// m_entities.push_back(cone);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add a grid for the world
	//Prop* grid = new Prop();
	//grid->m_name = "Grid";
	//float spacingXY = 5.f;
	//int numOfXY = (int)(100.f / spacingXY) + 1;
	//int numOfGrid = 100 + 1;
	//float dimensionRed = 0.02f;
	//float dimensionGreen = 0.02f;
	//float dimensionGray = 0.01f;
	//// gray grid
	//for (int i = 0; i < numOfGrid; ++i)
	//{
	//	AABB3 pipe(Vec3(-50.f + (float)i - (dimensionGray * 0.5f), -50.f, -(dimensionGray * 0.5f)),
	//		Vec3( -50.f + (float)i + (dimensionGray * 0.5f), 50.f, (dimensionGray * 0.5f)));
	//	AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	//}
	//for (int i = 0; i < numOfGrid; ++i)
	//{
	//	AABB3 pipe(Vec3(-50.f, -50.f + (float)i - (dimensionGray * 0.5f), -(dimensionGray * 0.5f)),
	//		Vec3(50.f, -50.f + (float)i + (dimensionGray * 0.5f), (dimensionGray * 0.5f)));
	//	AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	//}
	//// GREEN lane
	//for (int i = 0; i < numOfXY; ++i)
	//{
	//	if ( i == (numOfXY / 2))
	//	{
	//		AABB3 pipe(Vec3(-50.f + (float)i * spacingXY - (dimensionGreen * 1.2f), -50.f, -(dimensionGreen * 2.f)),
	//			Vec3(-50.f + (float)i * spacingXY + (dimensionGreen * 2.f), 50.f, (dimensionGreen * 2.f)));
	//		AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
	//	}
	//	else 
	//	{
	//		AABB3 pipe(Vec3(-50.f + (float)i * spacingXY - (dimensionGreen * 0.5f), -50.f, -(dimensionGreen * 0.5f)),
	//			Vec3(-50.f + (float)i * spacingXY + (dimensionGreen * 0.5f), 50.f, (dimensionGreen * 0.5f)));
	//		AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
	//	}
	//}
	//// RED lane
	//for (int i = 0; i < numOfXY; ++i)
	//{
	//	if (i == (numOfXY / 2))
	//	{
	//		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXY - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
	//			Vec3(50.f, -50.f + (float)i * spacingXY + (dimensionRed * 2.f), (dimensionRed * 2.f)));
	//		AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	//	}
	//	else
	//	{
	//		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXY - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
	//			Vec3(50.f, -50.f + (float)i * spacingXY + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
	//		AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	//	}
	//}
	//m_entities["Grid"] = grid;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add world basis
	// Mat44 worldBasis;
	// DebugAddWorldBasis(worldBasis, -1.f);
	// add world text
	Mat44 XTransform;
	XTransform.Append(Mat44::CreateZRotationDegrees(90.f));
	XTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, .15f)));
	DebugAddWorldText(" X - Forward", XTransform, 0.3f, -1.f, Vec2(0.f, 0.0f), Rgba8::RED, Rgba8::RED);
	Mat44 YTransform;
	YTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, .15f)));
	DebugAddWorldText("Y - Left ", YTransform, 0.3f, -1.f, Vec2(1.f, 0.0f), Rgba8::GREEN, Rgba8::GREEN);

	Mat44 ZTransform;
	ZTransform.Append(Mat44::CreateXRotationDegrees(-90.f));
	ZTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, -.15f)));
	DebugAddWorldText(" Z - Up", ZTransform, 0.3f, -1.f, Vec2(0.f, 1.0f), Rgba8::BLUE, Rgba8::BLUE);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Billboard Testing
	// Vec3 billboardPosition(15.f, 0.f, 4.f);
	// DebugAddWorldBillboardText("The Big Brother is watching YOU", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::WORLD_UP_CAMERA_FACING, Rgba8::CYAN);
	// billboardPosition += Vec3(0.f, 0.f, 1.5f);
	// DebugAddWorldBillboardText("The Big Brother is watching YOU", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::FULL_CAMERA_FACING, Rgba8::CYAN);
	// billboardPosition += Vec3(0.f, 0.f, 1.5f);
	// DebugAddWorldBillboardText("The Big Brother is watching YOU", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::WORLD_UP_CAMERA_OPOSSING, Rgba8::CYAN);
	// billboardPosition += Vec3(0.f, 0.f, 1.5f);
	// DebugAddWorldBillboardText("The Big Brother is watching YOU", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::FULL_CAMERA_OPPOSING, Rgba8::CYAN);
	
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// arrow for the debugging of the sun
	Prop* sun = new Prop();
	sun->m_name = "Sun";
	sun->m_position = Vec3(0.f, 0.f, 2.5f);
	sun->m_orientation = EulerAngles(120.f, 45.f, 0.f);
	sun->m_unlitTexture = g_textures[TESTUV];
	AddVertsForArrow3D(sun->m_unlitVertexes, Vec3(), Vec3(1.f, 0.f, 0.f), 0.15f, Rgba8::YELLOW_TRANSPARENT, Rgba8::YELLOW_TRANSPARENT);
	m_entities["Sun"] = sun;
	 
	// Model* model = new Model(this);
	// model->LoadXml("Data/Models/Cube_Emissive.xml");
	// m_loadedModels.push_back(model);
}

bool Game::Event_LoadMapByInputName(EventArgs& args)
{
	std::string mapName = args.GetValue("name", "Map not defined");

	if (mapName == "Map not defined")
	{
		std::string loadingError = Stringf("Input format error, use: LoadMap Name = name");
		g_theDevConsole->AddLine(loadingError, DevConsole::INFO_ERROR);
	}
	else
	{
		// if the map is loaded before, we'll just return what we had
		Map* requestedMap = g_theGame->CheckIfTheMapIsLoadedBefore(mapName);
		if (requestedMap)
		{
			g_theGame->m_currentMap = requestedMap;
			requestedMap->UpdateCameraBoundsByMapDef(); // tell the player camera use map world bounds, this is in startup
			return true;
		}
		else
		{
			// we create a new one
			Map* newMap = g_theGame->CreateAndStartupNewMap(mapName);

			if (!newMap)
			{
				std::string loadingError = Stringf("Did not find map definition of: \"%s\", choose one below", mapName.c_str());
				g_theDevConsole->AddLine(loadingError, DevConsole::INFO_ERROR);

				for (int i = 0; i < (int)MapDefinition::s_mapDefs.size(); ++i)
				{
					std::string mapDef = Stringf("%s", MapDefinition::s_mapDefs[i].m_name.c_str());
					g_theDevConsole->AddLine(mapDef, DevConsole::INFO_MINOR);
				}
			}
			else
			{
				g_theGame->m_currentMap = newMap;
			}
		}
	}

	return true;
}

void Game::ChangeGameState(GameState state)
{
	ExitState(m_currentGameState);
	m_currentGameState = state;
	EnterState(state);
}

void Game::EnterState(GameState state)
{
	switch (state)
	{
	case GameState::SPLASH_SCREEN:
	{
		EnterSplashScreen();
	}break;		
	case GameState::WAITING_FOR_PLAYERS:
	{
		EnterWaiitingForPlayers();
	}break;
	case GameState::MAIN_MENU:
	{
		EnterMainMenu();
	}break;	
	case GameState::PAUSED:
	{
		EnterPauseMenu();
	}break;
	case GameState::GAME_START:
	{
		EnterGameStart();
	}break;	
	case GameState::TURN_START:
	{
		EnterTurnStart();
	}break;
	case GameState::PLAYING:
	{
		EnterPlaying();
	}break;
	}
}

void Game::ExitState(GameState state)
{
	switch (state)
	{
	case GameState::SPLASH_SCREEN:
	{
		ExitSplashScreen();
	}break;	
	case GameState::WAITING_FOR_PLAYERS:
	{
		ExitWaitingForPlayers();
	}break;
	case GameState::MAIN_MENU:
	{
		ExitMainMenu();
	}break;
	case GameState::PAUSED:
	{
		ExitMainMenu();
	}break;
	case GameState::PLAYING:
	{
		ExitPlaying();
	}break;
	}
}

void Game::EnterSplashScreen()
{
	CreateSplashScreenWidget();
}

void Game::EnterMainMenu()
{
	CreateMainMenuWidgets();

	if (IsNetworkGameMode())
	{
		PlayerQuit();
	}
}

void Game::EnterGameStart()
{
	GenerateAllMaps();

	if (IsNetworkGameMode())
	{
		ChangeGameState(GameState::WAITING_FOR_PLAYERS);
	}
	else
	{
		ChangeGameState(GameState::TURN_START);
	}
}

void Game::EnterPlaying()
{
	DeleteWidget(m_turnStartWidget);
	m_playInstructionWidget->m_enabled = true;

	m_winnerWidget_0->m_enabled = false;
	m_winnerWidget_1->m_enabled = false;
}

void Game::EnterTurnStart()
{
	if (IsNetworkGameMode())
	{
		m_theOtherPlayerIsReady = true;
	}
	
	DeleteWidget(m_edgeTurnWidget);

	if (m_currentTurnPlayerIndex == INVALID_PLAYER_INDEX)
	{
		m_currentTurnPlayerIndex = 0;
	}
	else if (m_currentTurnPlayerIndex == 0)
	{
		m_currentTurnPlayerIndex = 1;
	}
	else if (m_currentTurnPlayerIndex == 1)
	{
		m_currentTurnPlayerIndex = 0;
	}

	CreateCenterAndEdgePlayerTurnWidgets();

	m_currentMap->UpdatePlayerUnitsAtTurnStart(m_currentTurnPlayerIndex);

	m_currentTurnState = TurnState::NO_SELECTION;
}

void Game::EnterGameOver()
{

}

void Game::EnterPauseMenu()
{
	m_pauseMenuWidget->m_enabled = true;
}

void Game::EnterWaiitingForPlayers()
{
	m_waitingWidget->m_enabled = true;
	m_theOtherPlayerIsReady = false;
}

void Game::ExitSplashScreen()
{

}

void Game::ExitMainMenu()
{
	m_menuVerts.clear();
}

void Game::ExitPauseMenu()
{
	m_pauseMenuWidget->m_enabled = false;

	delete m_currentMap;
	m_currentMap = nullptr;
}

void Game::ExitPlaying()
{

}

void Game::ExitGameOver()
{
	delete m_currentMap;
	m_currentMap = nullptr;
}

void Game::ExitWaitingForPlayers()
{
	m_waitingWidget->m_enabled = false;
	m_theOtherPlayerIsReady = false;
}

void Game::UpdateSplashScreen()
{
	if (m_splashScreenWidget)
	{
		m_splashScreenWidget->Update();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ENTER) || g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		ChangeGameState(GameState::MAIN_MENU);
	}
}

void Game::UpdateMainMenu()
{
	if (m_mainMenuWidget)
	{
		m_mainMenuWidget->Update();
	}

	if (m_mainMenuWidget->GetTextButtonByName("LocalGame")->WasJustReleased())
	{
		m_players[0]->m_netState = NetState::LOCAL;
		m_players[1]->m_netState = NetState::LOCAL;
		m_currentTurnPlayerIndex = INVALID_PLAYER_INDEX; // the enter turn will deal it to 0, so we always start player 1
		ChangeGameState(GameState::GAME_START);
	}	
	
	if (m_mainMenuWidget->GetTextButtonByName("Quit")->WasJustReleased())
	{
		FireEvent("quit");
	}

#ifdef ENGINE_ENABLE_NETSYSTEM
	if (g_theApp->m_netConfig->m_mode != NetSystemMode::NONE && m_mainMenuWidget->GetTextButtonByName("NetworkGame")->WasJustReleased())
	{
		if (g_theNetSystem->m_config.m_mode == NetSystemMode::SERVER)
		{
			m_players[0]->m_netState = NetState::LOCAL;
			m_players[1]->m_netState = NetState::REMOTE;
			m_currentTurnPlayerIndex = INVALID_PLAYER_INDEX; // the enter turn will deal it to 0, so we always start player 1
			ChangeGameState(GameState::GAME_START);
		}
		else if (g_theNetSystem->m_config.m_mode == NetSystemMode::CLIENT && m_mainMenuWidget->GetTextButtonByName("NetworkGame")->WasJustReleased())
		{
			m_players[0]->m_netState = NetState::REMOTE;
			m_players[1]->m_netState = NetState::LOCAL;
			m_currentTurnPlayerIndex = INVALID_PLAYER_INDEX; // the enter turn will deal it to 0, so we always start player 1
			ChangeGameState(GameState::GAME_START);
		}
	}
#endif
	
	// when pressing enter, it will execute the widget's current button
	if (g_theInput->WasKeyJustPressed(KEYCODE_ENTER))
	{
		TextLine* selectedButton = m_mainMenuWidget->GetTextButtonByCurrentButtonIndex();
		if (selectedButton)
		{
			if (selectedButton->m_buttonName == "LocalGame")
			{
				m_players[0]->m_netState = NetState::LOCAL;
				m_players[1]->m_netState = NetState::LOCAL;
				m_currentTurnPlayerIndex = INVALID_PLAYER_INDEX; // the enter turn will deal it to 0, so we always start player 1
				ChangeGameState(GameState::GAME_START);
			}
			else if (selectedButton->m_buttonName == "NetworkGame")
			{
				m_players[0]->m_netState = NetState::REMOTE;
				m_players[1]->m_netState = NetState::LOCAL;
				m_currentTurnPlayerIndex = INVALID_PLAYER_INDEX; // the enter turn will deal it to 0, so we always start player 1
				ChangeGameState(GameState::GAME_START);
			}
			else if (selectedButton->m_buttonName == "Quit")
			{
				FireEvent("quit");
			}
		}
	}

	UseArrowKeysToControlWidgetButtons(m_mainMenuWidget);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		ChangeGameState(GameState::SPLASH_SCREEN);
	}
}

void Game::UpdateWaitingForPlayers()
{
	if (g_theNetSystem)
	{
		// if we are connected, tell the other player we are ready
		if (g_theNetSystem->IsConnected())
		{
			PlayerReady();
		}

		// must wait until the other player is ready
		if (g_theNetSystem->IsConnected() && m_theOtherPlayerIsReady)
		{
			ChangeGameState(GameState::TURN_START);
		}

		if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			ChangeGameState(GameState::MAIN_MENU);
		}
	}
}

void Game::UpdatePauseMenu()
{
	if (m_pauseMenuWidget)
	{
		m_pauseMenuWidget->Update();
	}

	if (m_pauseMenuWidget->m_enabled)
	{
		if (m_pauseMenuWidget->GetTextButtonByName("Main Menu")->WasJustReleased())
		{
			ChangeGameState(GameState::MAIN_MENU);
		}

		if (m_pauseMenuWidget->GetTextButtonByName("Resume Game")->WasJustReleased())
		{
			ChangeGameState(GameState::PLAYING);
		}
	}
	 
	UseArrowKeysToControlWidgetButtons(m_pauseMenuWidget);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ENTER))
	{
		TextLine* selectedButton = m_pauseMenuWidget->GetTextButtonByCurrentButtonIndex();
		if (selectedButton)
		{
			if (selectedButton->m_buttonName == "Main Menu")
			{
				if (IsNetworkGameMode())
				{
					PlayerQuit();
				}
				ChangeGameState(GameState::MAIN_MENU);
			}
			else if (selectedButton->m_buttonName == "Resume Game")
			{
				ChangeGameState(GameState::PLAYING);
			}
		}
	}

}

void Game::UpdatePlaying()
{
	if (!IsNetworkGameMode())
	{
		m_players[m_currentTurnPlayerIndex]->Update();
	}
	else
	{
		int playerIndex = m_currentTurnPlayerIndex;


		// always update local player camera in network game mode
		if (m_players[0]->m_netState == NetState::LOCAL)
		{
			playerIndex = 0;
		}
		else if (m_players[1]->m_netState == NetState::LOCAL)
		{
			playerIndex = 1;
		}

		m_players[playerIndex]->Update();

		if (!g_theNetSystem->IsConnected() || !m_theOtherPlayerIsReady)
		{
			ChangeGameState(GameState::GAMEOVER);

			if (playerIndex == 0)
			{
				m_winnerWidget_0->m_enabled = true;
			}
			else
			{
				m_winnerWidget_1->m_enabled = true;
			}
		}
	}

	if (m_selectedUnit)
	{
		if (m_selectedUnit->m_unitHealth > 0)
		{
			m_selectedUnit->Update();
		}
	}
	if (m_defensingUnit)
	{
		if (m_defensingUnit->m_unitHealth > 0)
		{
			m_defensingUnit->Update();
		}
	}
	m_currentMap->Update();

	// update testing entities
	if (!m_entities.empty())
	{
		for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
		{
			std::string name = it->first;
			Entity* entity = it->second;
			entity->Update();
		}
	}

	if (!m_testSceneProps.empty())
	{
		for (Entity* entity : m_testSceneProps)
		{
			if (m_rotationMode)
			{
				entity->m_angularVelocity = EulerAngles(45.f, 0.f, 0.f);
			}
			else
			{
				entity->m_angularVelocity = EulerAngles(0.f, 0.f, 0.f);
			}
			entity->Update();
		}
	}
	if (!m_loadedModels.empty())
	{
		for (Entity* model : m_loadedModels)
		{
			if (m_rotationMode)
			{
				model->m_angularVelocity = EulerAngles(45.f, 0.f, 0.f);
			}
			else
			{
				model->m_angularVelocity = EulerAngles(0.f, 0.f, 0.f);
			}
			model->Update();
		}
	}

	UpdatePlayingInput();

	UpdateUnitInfoWidget();
	UpdatePlayingInstructionWidget();
}

void Game::UpdateTurnStart()
{
	if (m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		if (g_theInput->WasKeyJustPressed(KEYCODE_ENTER) || g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
		{
			StartTurn();
		}
	}

	if (IsNetworkGameMode())
	{
		int playerIndex = m_currentTurnPlayerIndex;

		// always update local player camera in network game mode
		if (m_players[0]->m_netState == NetState::LOCAL)
		{
			playerIndex = 0;
		}
		else if (m_players[1]->m_netState == NetState::LOCAL)
		{
			playerIndex = 1;
		}

		if (!g_theNetSystem->IsConnected()  || !m_theOtherPlayerIsReady)
		{
			if (playerIndex == 0)
			{
				m_winnerWidget_0->m_enabled = true;
			}
			else
			{
				m_winnerWidget_1->m_enabled = true;
			}
			ChangeGameState(GameState::GAMEOVER);

		}
	}
}

void Game::UpdateGameOver()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ENTER) || g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		ChangeGameState(GameState::MAIN_MENU);
	}
}

void Game::RenderSplashScreen() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	m_splashScreenWidget->Render();
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderMainMenu() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	m_mainMenuWidget->Render();
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderWaitingForPlayers() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderWidgetIfIsValidAndEnabled(m_waitingWidget);
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::ScribeNetworkCommands()
{
	SubscribeEventCallbackFunction("Command_StartTurn", Command_StartTurn);
	SubscribeEventCallbackFunction("Command_EndTurn", Command_EndTurn);
	SubscribeEventCallbackFunction("Command_SetFocusedHex", Command_SetFocusedHex);

	SubscribeEventCallbackFunction("Command_PlayerReady", Command_PlayerReady);
	SubscribeEventCallbackFunction("Command_PlayerQuit", Command_PlayerQuit);

	SubscribeEventCallbackFunction("Command_SelectFocusedUnit", Command_SelectFocusedUnit);
	SubscribeEventCallbackFunction("Command_SelectPreviousUnit", Command_SelectPreviousUnit);
	SubscribeEventCallbackFunction("Command_SelectNextUnit", Command_SelectNextUnit);

	SubscribeEventCallbackFunction("Command_Move", Command_Move);
	SubscribeEventCallbackFunction("Command_Stay", Command_Stay);
	SubscribeEventCallbackFunction("Command_HoldFire", Command_HoldFire);
	SubscribeEventCallbackFunction("Command_Attack", Command_Attack);
	SubscribeEventCallbackFunction("Command_Cancel", Command_Cancel);
}

bool Game::Command_StartTurn(EventArgs& args)
{
	UNUSED(args);
	g_theGame->StartTurn();
	return true;
}

bool Game::Command_EndTurn(EventArgs& args)
{
	UNUSED(args);
	g_theGame->EndTurn();
	return true;
}

bool Game::Command_SetFocusedHex(EventArgs& args)
{
	int tileIndex = args.GetValue("tileindex", INVALID_HEX_INDEX);
	g_theGame->SetFocusedHex(tileIndex);
	return true;
}


bool Game::Command_PlayerReady(EventArgs& args)
{
	UNUSED(args);
	g_theGame->m_theOtherPlayerIsReady = true;
	return true;
}

bool Game::Command_PlayerQuit(EventArgs& args)
{
	UNUSED(args);
	g_theGame->m_theOtherPlayerIsReady = false;
	return true;
}

void Game::PlayerReady()
{
	if (IsNetworkGameMode())
	{
		std::string mesg = "Command_PlayerReady";
		g_theNetSystem->Send(mesg);
	}
}

void Game::PlayerQuit()
{
	m_theOtherPlayerIsReady = false;

	if (IsNetworkGameMode())
	{
		std::string mesg = "Command_PlayerQuit";
		g_theNetSystem->Send(mesg);
	}
}

void Game::StartTurn()
{
	// only send out the message when we are local
	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string mesg = "Command_StartTurn";
		g_theNetSystem->Send(mesg);
	}

	ChangeGameState(GameState::PLAYING);
}

void Game::EndTurn()
{
	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string mesg = "Command_EndTurn";
		g_theNetSystem->Send(mesg);
	}

	// if player is selecting any unit, cancel the selection
	if (m_selectedUnit)
	{
		m_selectedUnit = nullptr;
	}

	ChangeGameState(GameState::TURN_START);

	m_endTurnWidget->m_enabled = false;
	m_currentTurnState = TurnState::NO_SELECTION;
}

void Game::SetFocusedHex(int tileIndex)
{
	m_selectedHexIndex = tileIndex;

	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string mesg = "Command_SetFocusedHex";
		std::string tileIndexString = Stringf(" tileindex = %i", tileIndex);
		mesg += tileIndexString;
		g_theNetSystem->Send(mesg);
	}
}

bool Game::Command_SelectFocusedUnit(EventArgs& args)
{
	UNUSED(args);
	g_theGame->SelectFocusedUnit();
	return true;
}

bool Game::Command_SelectPreviousUnit(EventArgs& args)
{
	UNUSED(args);
	g_theGame->SelectPreviousUnit();
	return true;
}

bool Game::Command_SelectNextUnit(EventArgs& args)
{
	UNUSED(args);
	g_theGame->SelectNextUnit();
	return true;
}

bool Game::Command_Move(EventArgs& args)
{
	UNUSED(args);
	g_theGame->Move();
	return true;
}

bool Game::Command_Stay(EventArgs& args)
{
	UNUSED(args);
	g_theGame->Stay();
	return true;
}

bool Game::Command_HoldFire(EventArgs& args)
{
	UNUSED(args);
	g_theGame->HoldFire();
	return true;
}

bool Game::Command_Attack(EventArgs& args)
{
	UNUSED(args);
	g_theGame->Attack();
	return true;
}

bool Game::Command_Cancel(EventArgs& args)
{
	UNUSED(args);
	g_theGame->Cancel();
	return true;
}

void Game::SelectFocusedUnit()
{
	m_selectedUnit = m_currentMap->GetUnitCurrentlyOnThisTile(m_selectedHexIndex);
	m_currentMap->m_moveTargetTileIndexLastTime = INVALID_HEX_INDEX;
	m_currentMap->UpdateSelectedUnitMovementRange();

	m_currentTurnState = TurnState::UNIT_SELECTED_MOVE;

	//Emitter* smokeEmitter = new Emitter(*g_spriteAnims[SMOKE], m_selectedUnit->GetExhaustPosition(), Vec3(0.f, 0.f, 1.f), 10.f, 30, 999.f);
	//smokeEmitter->SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
	//			SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 1.5f))->
	//			SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange(0.f, 0.f))->
	//			SetParticle_Alpha_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
	//			SetParticle_Color_FromStartToEnd(Rgba8::GRAY_Dark, Rgba8::WHITE)->
	//			SetParticle_Size_StartRange(FloatRange(0.3f, 1.2f))->
	//			SetParticle_MovingSpeed_StartRange(FloatRange(0.45f, 1.2f))->
	//			SetParticle_Rotation_StartRange(FloatRange(0.f, 360.f))->
	//			SetParticle_LifeTime_Range(FloatRange(2.4f, 3.6f))->
	//			SetParticle_Offset_StartRange(FloatRange(0.f, 0.3f));

	//g_theEffectSystem->AddEmitter(smokeEmitter);

	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string msg = "Command_SelectFocusedUnit";
		g_theNetSystem->Send(msg);
	}
}

void Game::SelectPreviousUnit()
{
	Player* currentPlayer = m_players[m_currentTurnPlayerIndex];
	int unitIndex = INVALID_UNIT_INDEX;
	int newTileIndex = INVALID_HEX_INDEX;

	if (m_selectedUnit)
	{
		unitIndex = m_currentMap->GetUnitIndex(m_selectedUnit);
	}
	else
	{
		unitIndex = 0;
	}

	bool findUnit = false;

	for (int i = unitIndex - 1; i >= 0; --i)
	{
		if (m_currentMap->m_units[i]->m_player == currentPlayer && i != unitIndex && !m_currentMap->m_units[i]->HasFinishedMoveAndAttackThisTurn())
		{
			newTileIndex = m_currentMap->m_units[i]->m_currentHexIndex;
			findUnit = true;
			break;
		}
	}

	if (!findUnit)
	{
		// if decreasing is not finding it, we will decrease from the end to find it
		for (int i = (int)m_currentMap->m_units.size() - 1; i >= 0; --i)
		{
			if (m_currentMap->m_units[i]->m_player == currentPlayer && i != unitIndex && !m_currentMap->m_units[i]->HasFinishedMoveAndAttackThisTurn())
			{
				newTileIndex = m_currentMap->m_units[i]->m_currentHexIndex;
				break;
			}
		}
	}

	m_selectedHexIndex = newTileIndex;
	SelectFocusedUnit();
	m_currentTurnState = TurnState::UNIT_SELECTED_MOVE;

	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string mesg = "Command_SelectPreviousUnit";
		g_theNetSystem->Send(mesg);
	}
}

void Game::SelectNextUnit()
{
	Player* currentPlayer = m_players[m_currentTurnPlayerIndex];
	int unitIndex = INVALID_UNIT_INDEX;
	int newTileIndex = INVALID_HEX_INDEX;

	if (m_selectedUnit)
	{
		unitIndex = m_currentMap->GetUnitIndex(m_selectedUnit);
	}
	else
	{
		unitIndex = 0;
	}

	bool findUnit = false;
	// the unit must not be current unit, nor could it already finished move and attack
	for (int i = unitIndex + 1; i < (int)m_currentMap->m_units.size(); ++i)
	{
		if (m_currentMap->m_units[i]->m_player == currentPlayer && i != unitIndex && !m_currentMap->m_units[i]->HasFinishedMoveAndAttackThisTurn())
		{
			newTileIndex = m_currentMap->m_units[i]->m_currentHexIndex;
			findUnit = true;
			break;
		}
	}

	if (!findUnit)
	{
		// if decreasing is not finding it, we will decrease from the end to find it
		for (int i = 0; i < (int)m_currentMap->m_units.size(); ++i)
		{
			if (m_currentMap->m_units[i]->m_player == currentPlayer && i != unitIndex && !m_currentMap->m_units[i]->HasFinishedMoveAndAttackThisTurn())
			{
				newTileIndex = m_currentMap->m_units[i]->m_currentHexIndex;
				break;
			}
		}
	}

	m_selectedHexIndex = newTileIndex;
	SelectFocusedUnit();
	m_currentTurnState = TurnState::UNIT_SELECTED_MOVE;

	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string mesg = "Command_SelectNextUnit";
		g_theNetSystem->Send(mesg);
	}
}


void Game::UpdatePlayingInput()
{
	// model viewer stuff
	//LoadSelectedFile();
	//SwitchSceneBasedOnInput();

	// ControlLightingSettings();
	//ControlTextureMapDebug();

	//ToggleToShowDebugVertexes();
	//ToggleToRotateModelAndProp();

	if (m_currentTurnPlayerIndex != INVALID_PLAYER_INDEX && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		m_playerMouseRaycastResult = RaycastFromCameraToMouseToMap(m_currentMap);
		int tileIndex = CheckMouseRaycastImpactIsInWhichHexagon(m_currentMap);
		SetFocusedHex(tileIndex);

		if (m_currentTurnState == TurnState::NO_SELECTION)
		{
			// select focused hex and unit
			Unit* raycastedUnit = m_currentMap->GetUnitCurrentlyOnThisTile(m_selectedHexIndex);

			if (raycastedUnit)
			{
				if (m_players[m_currentTurnPlayerIndex] == raycastedUnit->m_player && !raycastedUnit->HasFinishedMoveAndAttackThisTurn() &&
					g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
				{
					SelectFocusedUnit();
				}
			}

			if (m_playInstructionWidget->CheckIfTextOrImageOfThisNameIsShown("End Turn")
				&& g_theInput->WasKeyJustPressed('Y'))
			{
				m_currentTurnState = TurnState::END_TURN;
			}

			if ((m_playInstructionWidget->CheckIfTextOrImageOfThisNameIsShown("Previous") || m_playInstructionWidget->CheckIfTextOrImageOfThisNameIsShown("Next")) &&
					!CheckIfPlayerHasNoUnitsToMove())
			{
				UseLeftAndRightArrowsToSelectUnit();
			}

			if (g_theInput->WasKeyJustPressed('P') || g_theInput->WasKeyJustPressed(KEYCODE_ESC))
			{
				ChangeGameState(GameState::PAUSED);
			}
		}
		else if (m_currentTurnState == TurnState::END_TURN)
		{
			if (m_endTurnWidget->m_enabled &&
				(g_theInput->WasKeyJustPressed(KEYCODE_ENTER) || g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE)))
			{
				EndTurn();
			}
			else if (m_endTurnWidget->m_enabled && g_theInput->WasKeyJustPressed(KEYCODE_ESC))
			{
				m_currentTurnState = TurnState::NO_SELECTION;
				m_endTurnWidget->m_enabled = false;
				m_playInstructionWidget->m_enabled = true;
			}
		}
		// deselect unit
		else if (m_currentTurnState == TurnState::UNIT_SELECTED_MOVE)
		{
			if (m_selectedUnit && g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))	// cancel
			{
				m_selectedUnit->m_moving = false;
				m_selectedUnit = nullptr;
				m_currentTurnState = TurnState::NO_SELECTION;
			}

			// move/stay unit
			if (m_selectedUnit && m_currentMap->MoveTargetIsInRange() && g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
			{
				if (m_selectedUnit->m_startHexIndex != m_selectedHexIndex && !m_currentMap->GetUnitCurrentlyOnThisTile(m_selectedHexIndex))
				{
					Move();
				}
				else if (m_selectedUnit->m_startHexIndex == m_selectedHexIndex)
				{
					Stay();
				}
			}

			// player has no unit to move, only able to end turn
			// if (g_theInput->WasKeyJustPressed('Y'))
			// {
			// 	m_currentTurnState = TurnState::END_TURN;
			// }

			UseLeftAndRightArrowsToSelectUnit();
		}
		else if (m_currentTurnState == TurnState::UNIT_SELECTED_ATTACK && !m_selectedUnit->m_moving)	// unit can only attack after move in place
		{
			// cancel movement order
			if (!m_selectedUnit->m_hasAttacked && m_selectedUnit->m_hasMoved
					&& g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
			{
				Cancel();
			}

			// holdfire
			if (CheckIfSelectedHexPointsToSelectedUnit()
					&& g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
			{
				HoldFire();
			}

			// attack
			if (!m_currentMap->m_unitAttackTiles.empty() 
					&& g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
			{
				if (CheckIfSelectedHexIsInAttackTiles())
				{
					Attack();
				}
			}
		}
	}
}

void Game::UseLeftAndRightArrowsToSelectUnit()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTARROW))
	{
		SelectPreviousUnit();
	}
	else if(g_theInput->WasKeyJustPressed(KEYCODE_RIGHTARROW))
	{
		SelectNextUnit();
	}
}

bool Game::CheckIfSelectedHexIsInAttackTiles() const
{
	for (int i = 0; i < (int)m_currentMap->m_unitAttackTiles.size(); ++i)
	{
		if (m_selectedHexIndex == m_currentMap->GetTileIndex_For_TileCoordinates(m_currentMap->m_unitAttackTiles[i]->m_tileCoords))
		{
			return true;
		}
	}

	return false;
}

bool Game::CheckIfSelectedHexPointsToSelectedUnit() const
{
	return m_selectedHexIndex == m_selectedUnit->m_currentHexIndex;
}

bool Game::CheckIfPlayerHasNoUnitsToMove() const
{
	for (int i = 0; i < (int)m_currentMap->m_units.size(); ++i)
	{
		if (!m_currentMap->m_units[i]->m_hasAttacked &&
				!m_currentMap->m_units[i]->m_hasMoved &&
				m_currentMap->m_units[i]->m_player == g_theGame->m_players[m_currentTurnPlayerIndex])
		{
			return false;
		}	
	}

	return true;
}

bool Game::CheckForWinner()
{
	int player1_Units = 0;
	int player2_Units = 0;

	for (int i = 0; i < (int)m_currentMap->m_units.size(); ++i)
	{
		if (m_currentMap->m_units[i]->m_unitHealth >0 && m_currentMap->m_units[i]->m_player == m_players[0])
		{
			++player1_Units;
		}
		else if (m_currentMap->m_units[i]->m_unitHealth >0 && m_currentMap->m_units[i]->m_player == m_players[1])
		{
			++player2_Units;
		}
	}

	if (player1_Units == 0)
	{
		m_winnerWidget_1->m_enabled = true;
		ChangeGameState(GameState::GAMEOVER);
		return true;
	}
	else if (player2_Units == 0)
	{
		m_winnerWidget_0->m_enabled = true;
		ChangeGameState(GameState::GAMEOVER);
		return true;
	}

	return false;
}

 
void Game::Move()
{
	if (m_selectedHexIndex == INVALID_HEX_INDEX)
	{
		return;
	}

	m_selectedUnit->m_currentHexIndex = m_selectedHexIndex;
	m_selectedUnit->m_hasMoved = true;
	m_selectedUnit->m_moving = true;
	m_selectedUnit->m_movingTimer->Restart();

	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string mesg = "Command_Move";
		g_theNetSystem->Send(mesg);
	}
}

void Game::Stay()
{
	m_selectedUnit->m_currentHexIndex = m_selectedUnit->m_startHexIndex;
	m_selectedUnit->m_hasMoved = true;

	m_currentTurnState = TurnState::UNIT_SELECTED_ATTACK;
	m_currentMap->UpdateTilesInAttackRange();

	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string mesg = "Command_Stay";
		g_theNetSystem->Send(mesg);
	}
}

void Game::Attack()
{
	m_defensingUnit = m_currentMap->GetUnitCurrentlyOnThisTile(m_selectedHexIndex);
	m_defensingUnit->m_isAttacking = true;

	int selectedUnitAttackDamage = m_selectedUnit->m_unitDef->m_groundAttackDamage * 2 / m_defensingUnit->m_unitDef->m_defense;
	int defenseUnitAttackDamage = m_defensingUnit->m_unitDef->m_groundAttackDamage * 2 / m_defensingUnit->m_unitDef->m_defense;

	// both unit will try to attack
	// but if it is out of range, the damage is zero
	int range = (int)m_currentMap->AStarPathfinding(&m_currentMap->m_tiles[m_selectedUnit->m_currentHexIndex], &m_currentMap->m_tiles[m_defensingUnit->m_currentHexIndex]).size() - 1;
	if ( range > m_defensingUnit->m_unitDef->m_groundAttackRangeMin ||
		range < m_defensingUnit->m_unitDef->m_groundAttackRangeMin )
	{
		defenseUnitAttackDamage = 0;
		m_defensingUnit->m_isAttacking = false;
	}
	else
	{ 
		m_defensingUnit->m_attackingAtHexIndex = m_selectedUnit->m_currentHexIndex;
	}

	m_selectedUnit->m_damage = selectedUnitAttackDamage;
	m_defensingUnit->m_damage = defenseUnitAttackDamage;

	m_selectedUnit->m_isAttacking = true;
	m_selectedUnit->m_attackingAtHexIndex = m_defensingUnit->m_currentHexIndex;

	m_selectedUnit->m_hasAttacked = true;

	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string mesg = "Command_Attack";
		g_theNetSystem->Send(mesg);
	}
}

void Game::HoldFire()
{
	m_selectedUnit->m_hasAttacked = true;
	m_selectedUnit = nullptr;
	m_currentTurnState = TurnState::NO_SELECTION;

	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string mesg = "Command_HoldFire";
		g_theNetSystem->Send(mesg);
	}
}

void Game::Cancel()
{
	m_selectedUnit->m_currentHexIndex = m_selectedUnit->m_startHexIndex;
	m_selectedUnit->m_hasMoved = false;
	m_selectedUnit->m_position = g_theGame->m_currentMap->m_tiles[m_selectedUnit->m_startHexIndex].m_centerWorldPos;

	m_currentTurnState = TurnState::UNIT_SELECTED_MOVE;

	if (IsNetworkGameMode() && m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
	{
		std::string mesg = "Command_Cancel";
		g_theNetSystem->Send(mesg);
	}
}

void Game::ToggleToShowDebugVertexes()
{
	if (g_theInput->WasKeyJustPressed('N'))
	{
		if (m_debugMode)
		{
			m_debugMode = false;
		}
		else
		{
			m_debugMode = true;
		}
	}
}

void Game::ToggleToRotateModelAndProp()
{
	if (g_theInput->WasKeyJustPressed('R'))
	{
		if (m_rotationMode)
		{
			m_rotationMode = false;
		}
		else
		{
			m_rotationMode = true;
		}
	}
}

void Game::CreateVertexIndexBufferForGroundAndCopyFromCPUtoGPU()
{
	AddVertsForQuad3D(m_groundVertexs, m_groundIndexArray, Vec3(-20.f, -10.f, 0.f), Vec3(60.f, -10.f, 0.f), Vec3(60.f, 30.f, 0.f), Vec3(-20.f, 30.f, 0.f) );

	// create vertex buffer and index buffer
	m_groundVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_groundVertexs.size()), sizeof(Vertex_PCUTBN));
	m_groundIndexBuffer = g_theRenderer->CreateIndexBuffer((size_t)(m_groundIndexArray.size()));

	size_t vertexSize = sizeof(Vertex_PCUTBN);
	size_t vertexArrayDataSize = (m_groundVertexs.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_groundVertexs.data(), vertexArrayDataSize, m_groundVertexBuffer);

	size_t indexSize = sizeof(int);
	size_t indexArrayDataSize = m_groundIndexArray.size() * indexSize;
	g_theRenderer->CopyCPUToGPU(m_groundIndexArray.data(), indexArrayDataSize, m_groundIndexBuffer);
}

void Game::GenerateAllMaps()
{
	// make sure all maps are cleared up before generating because player might restart
	m_allMaps.clear();

	m_currentMap = CreateAndStartupNewMap("Grid12x12"); // default map


	m_players[0]->ClampCameraBasedOnHeightAndProjectedScreenCenter();
	m_players[1]->ClampCameraBasedOnHeightAndProjectedScreenCenter();
}

Map* Game::CheckIfTheMapIsLoadedBefore(std::string mapName)
{
	for (int i = 0; i < (int)m_allMaps.size(); ++i)
	{
		if (m_allMaps[i])
		{
			Map& loadedMap = *m_allMaps[i];

			if (loadedMap.m_mapDefinition->m_name == mapName || ToLower(loadedMap.m_mapDefinition->m_name) == mapName) // input case-insensitive
			{
				return &loadedMap;
			}
		}
	}

	return nullptr;
}

Map* Game::CreateAndStartupNewMap(std::string mapName)
{
	for (int i = 0; i < (int)MapDefinition::s_mapDefs.size(); ++i)
	{
		// put into the map list by names
		if (MapDefinition::s_mapDefs[i].m_name == mapName || ToLower(MapDefinition::s_mapDefs[i].m_name) == mapName)
		{
			Map* map = new Map(MapDefinition::s_mapDefs[i]);
			map->Startup();
			m_allMaps.push_back(map);
			return map;
		}
	}

	return nullptr;
}

RaycastResult3D Game::RaycastFromCameraToMouseToMap(Map const* map)
{
	RaycastResult3D result;
	if (m_currentTurnPlayerIndex != INVALID_PLAYER_INDEX)
	{
		if (m_players[m_currentTurnPlayerIndex]->m_netState == NetState::LOCAL)
		{
			Vec3 nearClipPos = m_players[m_currentTurnPlayerIndex]->m_playerCamera.GetCursorPosOnCameraNearClipPlane(*g_theRenderer->m_config.m_window);
			Vec3 cameraPos = m_players[m_currentTurnPlayerIndex]->m_playerCamera.m_position;
			m_rayForwardNormal = (nearClipPos - cameraPos).GetNormalized();

			// based on the map, generate a 3D plane
			const Vec3 planeNormal(0.f, 0.f, 1.f);
			Plane3 mapPlane(planeNormal, map->m_mapOrigin.z); 

			result = RaycastVsPlane3D(cameraPos, m_rayForwardNormal, 999.f, mapPlane);

			DebugAddWorldPoint(result.m_impactPos, 0.06f, 0.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			Vec3 arrowTip = result.m_impactPos + result.m_impactNormal * 0.3f;
			DebugAddWorldArrow(result.m_impactPos, arrowTip, 0.03f, 0.f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);

			return result;
		}
	}

	return result;
}

void Game::AddToVertsForPlayerSelectedHexagons(int index)
{
	if (index != INVALID_HEX_INDEX)
	{
		// according to the index, if there is unit on it change its color
		Player* player = m_currentMap->CheckWhichPlayerUnitIsOnTile(index);
		Rgba8 frameColor;
		if (player)
		{
			frameColor = m_playerHexColorMap[player];
		}
		else
		{
			frameColor = m_selectedHexColor;
		}	 
		AddVertsForHexagonFrame(m_selectedHexagonsVertexs, m_currentMap->m_tiles[index].m_centerWorldPos, m_selectedHexInRadius, 0.05f, 0.f, true, frameColor);
	}
}

void Game::RenderSelectedHex() const
{
	if (!m_selectedHexagonsVertexs.empty())
	{
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetModelConstants(Mat44());
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray((int)(m_selectedHexagonsVertexs.size()), m_selectedHexagonsVertexs.data());
	}
}

int Game::CheckMouseRaycastImpactIsInWhichHexagon(Map* map)
{
	// mouse did not hit map plane at all
	if (!m_playerMouseRaycastResult.m_didImpact)
	{
		return INVALID_HEX_INDEX;
	}

	Vec3& p = m_playerMouseRaycastResult.m_impactPos;

	for (int tileIndex = 0; tileIndex < (int)map->m_tiles.size(); ++ tileIndex)
	{
		Tile& tile = map->m_tiles[tileIndex];

		// first do not consider hexes that are not rendered due to world bounds or are blocked
		Vec3& c = tile.m_centerWorldPos;
		if (c < m_currentMap->m_mapDefinition->m_worldBoundsMin || c > m_currentMap->m_mapDefinition->m_worldBoundsMax || tile.m_tileDef->m_isBlocked)
		{
			continue;
		}

		// then check if the center of the hex is too far away from the impact pos
		float distSqrt = GetDistanceSquared3D(p, c);
		if (distSqrt > (map->m_tileInRadius * 2.f) * (map->m_tileInRadius * 2.f) )
		{
			continue;
		}

		// if not, check in detail if the hit position in the hexagon
		if (IsPointInHexagonXY(p, tile.m_boundaries))
		{
			return tileIndex;
		}
	}

	return INVALID_HEX_INDEX;
}

bool Game::LoadSelectedFile()
{
	if (g_theInput->WasKeyJustPressed('L'))
	{
		char currentDir[MAX_PATH];
		DWORD result = GetCurrentDirectoryA(MAX_PATH, currentDir);

		if (result > 0 && result <= MAX_PATH) // get current directory successfully
		{
			m_appFilePath = std::string(currentDir); // remember the application address

			char openFileName[MAX_PATH]; // Buffer for file name
			openFileName[0] = '\0';
			OPENFILENAMEA data = {};  // Common dialog box structure

			// Initialize OPENFILENAME structure
			data.lStructSize = sizeof(data);
			data.hwndOwner = (HWND)g_theRenderer->m_config.m_window->GetHwnd();
			data.lpstrFile = openFileName;
			data.nMaxFile = sizeof(openFileName);
			data.lpstrFile = openFileName;

			// Filter to allow only .obj and .xml files
			// data.lpstrFilter = "OBJ files\0*.obj\0XML files\0*.xml\0All files\0*.*\0"; // either type
			data.lpstrFilter = "OBJ and XML files\0*.obj;*.xml\0All files\0*.*\0";
			data.nFilterIndex = 1;
			data.lpstrFileTitle = NULL;
			data.nMaxFileTitle = 0;

			// Set the initial directory to the current working directory
			data.lpstrInitialDir = currentDir;

			// Set flags for the dialog
			data.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			g_theInput->m_cursorState.m_desiredHiddenMode = false;
			g_theInput->ShowOrHideCursorBasedOnCursorMode();

			// Display the Open dialog box
			if (GetOpenFileNameA(&data) == TRUE) // the currentDir may changed after loading open a new file
			{
				// The file path selected by the user
				m_modelFilePath = openFileName;
				std::string filePath = Stringf("Loading file: %s", openFileName);
				DebugAddMessage(filePath, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
				// ::ShowCursor(false);

				m_loadedModels.clear();

				//----------------------------------------------------------------------------------------------------------------------------------------------------
				// reset our current directory because the open file might have change teh working directory
				char newDir[MAX_PATH];
				GetCurrentDirectoryA(MAX_PATH, newDir);

				// Find the position of the last backslash
				char* lastSlashPos = strrchr(newDir, '\\');

				if (lastSlashPos != nullptr) {
					// Null-terminate the string at the last backslash to remove the last folder
					*lastSlashPos = '\0';

					// Find the position of the second-to-last backslash
					char* secondLastSlashPos = strrchr(newDir, '\\');

					if (secondLastSlashPos != nullptr) {
						// Null-terminate the string at the second-to-last backslash to remove two folders
						*secondLastSlashPos = '\0';
					}
				}
				SetCurrentDirectoryA(newDir);
				//----------------------------------------------------------------------------------------------------------------------------------------------------

				LoadNewModel();

				// the dialog window will interfere with windows key released fire event 
				EventArgs args;
				args.SetValue("KeyCode", Stringf("%d", 'L'));
				FireEvent("KeyReleased", args);

				return true;
			}
			else
			{
				GetWindowsLastErrorAndDisplayOnScreen();
			}

			// the dialog window will interfere with windows key released fire event 
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", "L"));
			FireEvent("KeyReleased", args);

			// ::ShowCursor(false);
			return false;
		}
		else if (result > MAX_PATH) // the name of the current directory is too long
		{
			std::string filePath = "Current application file path is too long";
			DebugAddMessage(filePath, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
			::ShowCursor(false);
			return false;
		}
		else // == 0, failed
		{
			GetWindowsLastErrorAndDisplayOnScreen();
			::ShowCursor(false);
			return false;
		}
	}
	else
	{
		::ShowCursor(false);
		return false;
	}
}

void Game::ControlTextureMapDebug()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM1))
	{
		if (m_phongLighinting->lighingDebug.RenderAmbient)
		{
			m_phongLighinting->lighingDebug.RenderAmbient = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.RenderAmbient = true;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM2))
	{
		if (m_phongLighinting->lighingDebug.RenderDiffuse)
		{
			m_phongLighinting->lighingDebug.RenderDiffuse = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.RenderDiffuse = true;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM3))
	{
		if (m_phongLighinting->lighingDebug.RenderSpecular)
		{
			m_phongLighinting->lighingDebug.RenderSpecular = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.RenderSpecular = true;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM4))
	{
		if (m_phongLighinting->lighingDebug.RenderEmissive)
		{
			m_phongLighinting->lighingDebug.RenderEmissive = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.RenderEmissive = true;
		}
	}	
	
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM5))
	{
		if (m_phongLighinting->lighingDebug.UseDiffuseMap)
		{
			m_phongLighinting->lighingDebug.UseDiffuseMap = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.UseDiffuseMap = true;
		}
	}	

	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM6))
	{
		if (m_phongLighinting->lighingDebug.UseNormalMap)
		{
			m_phongLighinting->lighingDebug.UseNormalMap = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.UseNormalMap = true;
		}	
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM7))
	{
		if (m_phongLighinting->lighingDebug.UseSpecularMap)
		{
			m_phongLighinting->lighingDebug.UseSpecularMap = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.UseSpecularMap = true;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM8))
	{
		if (m_phongLighinting->lighingDebug.UseGlossinessMap)
		{
			m_phongLighinting->lighingDebug.UseGlossinessMap = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.UseGlossinessMap = true;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM9))
	{
		if (m_phongLighinting->lighingDebug.UseEmissiveMap)
		{
			m_phongLighinting->lighingDebug.UseEmissiveMap = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.UseEmissiveMap = true;
		}
	}
}

void Game::UpdateUnitInfoWidget()
{
	bool showWidget_l = false;
	bool showWidget_r = false;
	if (m_players[0] == m_currentMap->CheckWhichPlayerUnitIsOnTile(m_selectedHexIndex))
	{
		showWidget_l = true;
		m_unitWidget_player1 = CreateUnitInfoWidget(m_players[0], m_currentMap->GetUnitCurrentlyOnThisTile(m_selectedHexIndex));
	}	
	else if (m_selectedUnit)
	{
		if (m_players[0] == m_selectedUnit->m_player)
		{
			showWidget_l = true;
			m_unitWidget_player1 = CreateUnitInfoWidget(m_players[0], m_selectedUnit);
		}
	}

	if (m_players[1] == m_currentMap->CheckWhichPlayerUnitIsOnTile(m_selectedHexIndex))
	{
		showWidget_r = true;
		m_unitWidget_player2 = CreateUnitInfoWidget(m_players[1], m_currentMap->GetUnitCurrentlyOnThisTile(m_selectedHexIndex));
	}	
	else if (m_selectedUnit)
	{
		if (m_players[1] == m_selectedUnit->m_player)
		{
			showWidget_r = true;
			m_unitWidget_player2 = CreateUnitInfoWidget(m_players[1], m_selectedUnit);
		}
	}

	// clear widget
	if (!showWidget_l && m_unitWidget_player1)
	{
		delete m_unitWidget_player1;
		m_unitWidget_player1 = nullptr;
	}
	
	if (!showWidget_r && m_unitWidget_player2)
	{
		delete m_unitWidget_player2;
		m_unitWidget_player2 = nullptr;
	}

}

Widget* Game::CreateUnitInfoWidget(Player* unitOwner, Unit* unit)
{
	Vec2 widgetAlignment;
	if (unitOwner == m_players[0])
	{
		widgetAlignment = Vec2(0.02f, 0.1f);
	}
	else if (unitOwner == m_players[1])
	{
		widgetAlignment = Vec2(0.98f, 0.1f);
	}

	float fontSize_s = 24.f;

	Vec2 unitInfoSize(0.15f, 0.f);
	Widget* widget = new Widget(unitInfoSize, 4.f / 7.f, widgetAlignment, &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK_TRANSPARENT, 0.02f, Rgba8::WHITE);
	widget->m_designedByHeight = false;

	AABB2 unitAttributesBox(Vec2(0.02f, 0.02f), Vec2(0.98f, 0.4f));
	std::string unitStaticsKeys("Attack\nDefense\nRange\nMove\nHealth");
	TextLine* unitAttributes_key = new TextLine(unitStaticsKeys, Vec2(0.f, 0.f), fontSize_s, unitAttributesBox);
	widget->AddTextLineToWidget(unitAttributes_key);

	UnitDefinition const* unitDef = unit->m_unitDef;
	std::string unitStaticsValue = Stringf("%i\n%i\n%i - %i\n%i\n%i", unitDef->m_groundAttackDamage, unitDef->m_defense, unitDef->m_groundAttackRangeMin, unitDef->m_groundAttackRangeMax, unitDef->m_movementRange, unit->m_unitHealth);
	TextLine* unitAttributes_value  = new TextLine(unitStaticsValue, Vec2(1.f, 0.f), fontSize_s, unitAttributesBox);
	widget->AddTextLineToWidget(unitAttributes_value);

	AABB2 unitTitleBounds(Vec2(0.02f, 0.9f), Vec2(0.98f, 0.98f));
	std::string unitName(Stringf("%s", unitDef->m_actorName.c_str()));
	TextLine* unitTitle = new TextLine(unitName, Vec2(0.5f, 1.f), 32.f, unitTitleBounds);
	widget->AddTextLineToWidget(unitTitle);

	Vec2 unitOutlookPos(0.5f, 0.65f);
	ImageBox unitOutlook(unitOutlookPos, 0.f, 0.4f, unitDef->m_texture_UI);
	widget->m_imageBoxes.push_back(unitOutlook);

	return widget;
}

void Game::CreatePlayingInstructionWidget()
{
	Vec2 widgetSize(0.6f, 0.f);
	m_playInstructionWidget = new Widget(widgetSize, 9.f, Vec2(0.5f, 0.02f), &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK_TRANSPARENT, 0.02f, Rgba8::WHITE);
	m_playInstructionWidget->m_designedByHeight = false;

	float fontSize = 32.f;
	float firstRowIconHeight = 0.7f;
	float SecondRowIconHeight = 0.2f;	
	float firstRowTextStartHeight = 0.55f;
	// float SecondRowIconHeight = 0.2f;
	float IconHeight = 0.3f;
	float spacing = 0.3f;

	// second row
	Vec2 previousIconPos(0.05f, SecondRowIconHeight);
	ImageBox previousImageBox(previousIconPos, 0.f, IconHeight, g_textures[ARROW_L_ICON], "Previous");
	m_playInstructionWidget->m_imageBoxes.push_back(previousImageBox);	
	
	Vec2 nextIconPos = previousIconPos + Vec2(spacing, 0.f);
	ImageBox nextImageBox(nextIconPos, 0.f, IconHeight, g_textures[ARROW_R_ICON], "Next");
	m_playInstructionWidget->m_imageBoxes.push_back(nextImageBox);	
	
	Vec2 YIconPos = nextIconPos + Vec2(spacing, 0.f);
	ImageBox YImageBox(YIconPos, 0.f, IconHeight, g_textures[Y_ICON], "End Turn");
	m_playInstructionWidget->m_imageBoxes.push_back(YImageBox);

	AABB2 previousTextBounds = AABB2(Vec2(previousIconPos.x + 0.02f, 0.05f), Vec2(previousIconPos.x + 0.12f, 0.9f));
	std::string previousText = "Previous";
	TextLine* previousUnit = new TextLine(previousText, previousTextBounds, Vec2(0.f, 0.f), fontSize, "Previous");
	m_playInstructionWidget->AddTextLineToWidget(previousUnit);	
	
	AABB2 nextTextBounds = AABB2(Vec2(nextIconPos.x + 0.02f, 0.05f), Vec2(nextIconPos.x + 0.12f, 0.9f));
	std::string nextText = "Next";
	TextLine* nextUnit = new TextLine(nextText, nextTextBounds, Vec2(0.f, 0.f), fontSize, "Next");
	m_playInstructionWidget->AddTextLineToWidget(nextUnit);	
	
	AABB2 endTextBounds = AABB2(Vec2(YIconPos.x + 0.02f, 0.05f), Vec2(YIconPos.x + 0.12f, 0.9f));
	std::string endTurnText = "End Turn";
	TextLine* endText = new TextLine(endTurnText, endTextBounds, Vec2(0.f, 0.f), fontSize, "End Turn");
	m_playInstructionWidget->AddTextLineToWidget(endText);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// first row
	Vec2 firstRowFirstSlotIconPos(0.05f, firstRowIconHeight);
	ImageBox firstRowFirstSlotImageBox(firstRowFirstSlotIconPos, 0.f, IconHeight, g_textures[LMB_ICON], "LMB");
	m_playInstructionWidget->m_imageBoxes.push_back(firstRowFirstSlotImageBox);

	AABB2 firstRowFirstSlotTextBounds = AABB2(Vec2(firstRowFirstSlotIconPos.x + 0.02f, firstRowTextStartHeight), Vec2(firstRowFirstSlotIconPos.x + 0.12f, 0.9f));
	std::string selectText = "Select";
	TextLine* selectTextLine = new TextLine(selectText, firstRowFirstSlotTextBounds, Vec2(0.f, 0.f), fontSize,  "Select");
	m_playInstructionWidget->AddTextLineToWidget(selectTextLine);	
	std::string moveText = "Move";
	TextLine* moveTextLine = new TextLine(moveText, firstRowFirstSlotTextBounds, Vec2(0.f, 0.f), fontSize, "Move");
	m_playInstructionWidget->AddTextLineToWidget(moveTextLine);
	std::string stayText = "Stay";
	TextLine* stayTextLine = new TextLine(stayText, firstRowFirstSlotTextBounds, Vec2(0.f, 0.f), fontSize,  "Stay");
	m_playInstructionWidget->AddTextLineToWidget(stayTextLine);	
	std::string holdFireText = "Hold Fire";
	TextLine* holdFireLine = new TextLine(holdFireText, firstRowFirstSlotTextBounds, Vec2(0.f, 0.f), fontSize,  "Hold Fire");
	m_playInstructionWidget->AddTextLineToWidget(holdFireLine);	
	std::string attackText = "Attack";
	TextLine* attackLine = new TextLine(attackText, firstRowFirstSlotTextBounds, Vec2(0.f, 0.f), fontSize,  "Attack");
	m_playInstructionWidget->AddTextLineToWidget(attackLine);
	
	Vec2 firstRowSecondSlotIconPos = Vec2(0.05f, firstRowIconHeight) + Vec2(spacing, 0.f);
	ImageBox firstRowSecondSlotImageBox(firstRowSecondSlotIconPos, 0.f, IconHeight, g_textures[RMB_ICON], "RMB");
	m_playInstructionWidget->m_imageBoxes.push_back(firstRowSecondSlotImageBox);

	AABB2 firstRowSecondSlotTextBounds = AABB2(Vec2(firstRowSecondSlotIconPos.x + 0.02f, firstRowTextStartHeight), Vec2(firstRowSecondSlotIconPos.x + 0.12f, 0.9f));
	std::string deselectText = "Deselect";
	TextLine* deselectTextLine = new TextLine(deselectText, firstRowSecondSlotTextBounds, Vec2(0.f, 0.f), fontSize,  "Deselect");
	m_playInstructionWidget->AddTextLineToWidget(deselectTextLine);	
	std::string cancelText = "Cancel";
	TextLine* cancelTextLine = new TextLine(cancelText, firstRowSecondSlotTextBounds, Vec2(0.f, 0.f), fontSize,  "Cancel");
	m_playInstructionWidget->AddTextLineToWidget(cancelTextLine);


	m_playInstructionWidget->HideTextAndImageOfThisName("Select");
	m_playInstructionWidget->HideTextAndImageOfThisName("Move");
	m_playInstructionWidget->HideTextAndImageOfThisName("Hold Fire");
	m_playInstructionWidget->HideTextAndImageOfThisName("Stay");
	m_playInstructionWidget->HideTextAndImageOfThisName("Attack");

	m_playInstructionWidget->HideTextAndImageOfThisName("Deselect");
	m_playInstructionWidget->HideTextAndImageOfThisName("Cancel");

	m_playInstructionWidget->HideTextAndImageOfThisName("LMB");
	m_playInstructionWidget->HideTextAndImageOfThisName("RMB");
}

void Game::UpdatePlayingInstructionWidget()
{
	if (m_currentTurnState == TurnState::NO_SELECTION)
	{
		// if player have not select a unit but the raycast hit a unit, show select icon
		if (m_players[m_currentTurnPlayerIndex] == m_currentMap->CheckWhichPlayerUnitIsOnTile(m_selectedHexIndex) &&
				!m_currentMap->GetUnitCurrentlyOnThisTile(m_selectedHexIndex)->HasFinishedMoveAndAttackThisTurn())
		{
			m_playInstructionWidget->ShowTextAndImageOfThisName("Select");
			m_playInstructionWidget->HideTextAndImageOfThisName("Move");
			m_playInstructionWidget->HideTextAndImageOfThisName("Hold Fire");
			m_playInstructionWidget->HideTextAndImageOfThisName("Stay");
			m_playInstructionWidget->HideTextAndImageOfThisName("Attack");

			m_playInstructionWidget->HideTextAndImageOfThisName("Deselect");
			m_playInstructionWidget->HideTextAndImageOfThisName("Cancel");

			m_playInstructionWidget->ShowTextAndImageOfThisName("LMB");
			m_playInstructionWidget->HideTextAndImageOfThisName("RMB");
		}
		else // hide all
		{
			m_playInstructionWidget->HideTextAndImageOfThisName("Select");
			m_playInstructionWidget->HideTextAndImageOfThisName("Move");
			m_playInstructionWidget->HideTextAndImageOfThisName("Hold Fire");
			m_playInstructionWidget->HideTextAndImageOfThisName("Stay");
			m_playInstructionWidget->HideTextAndImageOfThisName("Attack");

			m_playInstructionWidget->HideTextAndImageOfThisName("Deselect");
			m_playInstructionWidget->HideTextAndImageOfThisName("Cancel");

			m_playInstructionWidget->HideTextAndImageOfThisName("LMB");
			m_playInstructionWidget->HideTextAndImageOfThisName("RMB");
		}

		// able to end turn 
		m_playInstructionWidget->ShowTextAndImageOfThisName("End Turn");
	}

	if (m_currentTurnState == TurnState::UNIT_SELECTED_MOVE)
	{
		// showing movement instruction
		if (m_selectedUnit)
		{
			if (!m_selectedUnit->m_hasMoved)
			{
				if (m_currentMap->m_mouseInUnitMovementRange)
				{
					if (CheckIfSelectedHexPointsToSelectedUnit())
					{
						m_playInstructionWidget->ShowTextAndImageOfThisName("Stay");
						m_playInstructionWidget->HideTextAndImageOfThisName("Move");
					}
					else
					{
						m_playInstructionWidget->HideTextAndImageOfThisName("Stay");
						m_playInstructionWidget->ShowTextAndImageOfThisName("Move");
					}
					m_playInstructionWidget->HideTextAndImageOfThisName("Select");
					m_playInstructionWidget->HideTextAndImageOfThisName("Hold Fire");
					m_playInstructionWidget->HideTextAndImageOfThisName("Attack");

					m_playInstructionWidget->ShowTextAndImageOfThisName("Deselect");
					m_playInstructionWidget->HideTextAndImageOfThisName("Cancel");

					m_playInstructionWidget->ShowTextAndImageOfThisName("LMB");
					m_playInstructionWidget->ShowTextAndImageOfThisName("RMB");

					m_playInstructionWidget->HideTextAndImageOfThisName("End Turn");
				}
				else
				{
					m_playInstructionWidget->HideTextAndImageOfThisName("Select");
					m_playInstructionWidget->HideTextAndImageOfThisName("Move");
					m_playInstructionWidget->HideTextAndImageOfThisName("Hold Fire");
					m_playInstructionWidget->HideTextAndImageOfThisName("Stay");
					m_playInstructionWidget->HideTextAndImageOfThisName("Attack");

					m_playInstructionWidget->ShowTextAndImageOfThisName("Deselect");
					m_playInstructionWidget->HideTextAndImageOfThisName("Cancel");

					m_playInstructionWidget->HideTextAndImageOfThisName("LMB");
					m_playInstructionWidget->ShowTextAndImageOfThisName("RMB");

					m_playInstructionWidget->HideTextAndImageOfThisName("End Turn");
				}

				// no able to end turn 
				m_playInstructionWidget->HideTextAndImageOfThisName("End Turn");
		}
		}
	}

	// previous and next unit
	if (m_currentTurnState == TurnState::UNIT_SELECTED_MOVE || m_currentTurnState == TurnState::NO_SELECTION)
	{
		if (CheckIfPlayerHasNoUnitsToMove())
		{
			m_playInstructionWidget->HideTextAndImageOfThisName("Previous");
			m_playInstructionWidget->HideTextAndImageOfThisName("Next");

			// m_currentTurnState = TurnState::NO_SELECTION;
		}
		else
		{
			m_playInstructionWidget->ShowTextAndImageOfThisName("Previous");
			m_playInstructionWidget->ShowTextAndImageOfThisName("Next");
		}
	}


	if (m_currentTurnState == TurnState::UNIT_SELECTED_ATTACK)
	{
		if (CheckIfSelectedHexIsInAttackTiles()) // attack
		{
			m_playInstructionWidget->HideTextAndImageOfThisName("Select");
			m_playInstructionWidget->HideTextAndImageOfThisName("Move");
			m_playInstructionWidget->HideTextAndImageOfThisName("Hold Fire");
			m_playInstructionWidget->HideTextAndImageOfThisName("Stay");
			m_playInstructionWidget->ShowTextAndImageOfThisName("Attack");

			m_playInstructionWidget->ShowTextAndImageOfThisName("LMB");
		}
		else if (CheckIfSelectedHexPointsToSelectedUnit())	// hold fire
		{
			m_playInstructionWidget->HideTextAndImageOfThisName("Select");
			m_playInstructionWidget->HideTextAndImageOfThisName("Move");
			m_playInstructionWidget->ShowTextAndImageOfThisName("Hold Fire");
			m_playInstructionWidget->HideTextAndImageOfThisName("Stay");
			m_playInstructionWidget->HideTextAndImageOfThisName("Attack");

			m_playInstructionWidget->ShowTextAndImageOfThisName("LMB");
		}
		else  // no LMB option
		{
			m_playInstructionWidget->HideTextAndImageOfThisName("Select");
			m_playInstructionWidget->HideTextAndImageOfThisName("Move");
			m_playInstructionWidget->HideTextAndImageOfThisName("Hold Fire");
			m_playInstructionWidget->HideTextAndImageOfThisName("Stay");
			m_playInstructionWidget->HideTextAndImageOfThisName("Attack");

			m_playInstructionWidget->HideTextAndImageOfThisName("LMB");
		}

		// show cancel all the time
		m_playInstructionWidget->ShowTextAndImageOfThisName("Cancel");
		m_playInstructionWidget->ShowTextAndImageOfThisName("RMB");

		m_playInstructionWidget->HideTextAndImageOfThisName("Deselect");

		// no able to end turn 
		m_playInstructionWidget->HideTextAndImageOfThisName("End Turn");

		// hide selection when attacking
		m_playInstructionWidget->HideTextAndImageOfThisName("Previous");
		m_playInstructionWidget->HideTextAndImageOfThisName("Next");
	}

	if (m_currentTurnState == TurnState::NO_SELECTION)
	{
		if (CheckIfPlayerHasNoUnitsToMove()) // only show end turn option
		{
			m_playInstructionWidget->HideTextAndImageOfThisName("Select");
			m_playInstructionWidget->HideTextAndImageOfThisName("Move");
			m_playInstructionWidget->HideTextAndImageOfThisName("Hold Fire");
			m_playInstructionWidget->HideTextAndImageOfThisName("Stay");
			m_playInstructionWidget->HideTextAndImageOfThisName("Attack");

			m_playInstructionWidget->HideTextAndImageOfThisName("Deselect");
			m_playInstructionWidget->HideTextAndImageOfThisName("Cancel");

			m_playInstructionWidget->HideTextAndImageOfThisName("LMB");
			m_playInstructionWidget->HideTextAndImageOfThisName("RMB");

			m_playInstructionWidget->ShowTextAndImageOfThisName("End Turn");
		}
	}

	if (m_currentTurnState == TurnState::END_TURN)
	{
		m_playInstructionWidget->m_enabled = false;
		m_endTurnWidget->m_enabled = true;
	}
}

void Game::CreateMainMenuWidgets()
{
	float ratio = g_theWindow->GetCurrentAspectRatio();
	m_mainMenuWidget = new Widget(Vec2(0.f, 1.f), ratio, Vec2(0.5f, 0.5f), &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK, 0.f, Rgba8::WHITE);
	m_mainMenuWidget->m_designedByHeight = true;

	AABB2 titleBounds(Vec2(0.3f, 0.3f), Vec2(.7f, .7f));
	std::string title("Main Menu");
	TextLine* titleLine = new TextLine(title, Vec2(0.5f, 0.5f), 128.f, titleBounds, 90.f, Vec2(-0.3f, 0.f), Rgba8::WHITE, Rgba8::BLACK, "undefined", INVALID_BUTTON_INDEX, Rgba8::BLACK, Rgba8::WHITE);
	m_mainMenuWidget->AddTextLineToWidget(titleLine);

	// if net system is not enabled, we only have local game option
	int buttonIndex = 0;

	AABB2 buttonBounds(Vec2(0.25f, 0.55f), Vec2(.8f, .65f));
	std::string localGameOption("Local Game"); 
	TextLine* localGameBotton = new TextLine(localGameOption, Vec2(0.f, 0.5f), 64.f, buttonBounds, 0.f, Vec2(0.f, 0.f), Rgba8::WHITE, Rgba8::BLACK, "LocalGame",buttonIndex, Rgba8::TOTAL_TRANSPARENT, Rgba8::WHITE);
	m_mainMenuWidget->AddTextLineToWidget(localGameBotton);	

	Vec2 spacing(0.f, 0.12f);
	// if net system is enabled, we have Network game option
	#ifdef ENGINE_ENABLE_NETSYSTEM
	if (g_theApp->m_netConfig->m_mode != NetSystemMode::NONE)
	{
		++buttonIndex;

		buttonBounds.m_mins -= spacing;
		buttonBounds.m_maxs -= spacing;
		std::string networkGameOption("Network Game");
		TextLine* networkGameBotton = new TextLine(networkGameOption, Vec2(0.f, 0.5f), 64.f, buttonBounds, 0.f, Vec2(0.f, 0.f), Rgba8::WHITE, Rgba8::BLACK, "NetworkGame", buttonIndex, Rgba8::TOTAL_TRANSPARENT, Rgba8::WHITE);
		m_mainMenuWidget->AddTextLineToWidget(networkGameBotton);
	}
	#endif

	// quit option is always here
	++buttonIndex;
	buttonBounds.m_mins -= spacing;
	buttonBounds.m_maxs -= spacing;
	std::string quitOption("Quit");
	TextLine* quitBotton = new TextLine(quitOption, Vec2(0.f, 0.5f), 64.f, buttonBounds, 0.f, Vec2(0.f, 0.f), Rgba8::WHITE, Rgba8::BLACK, "Quit", buttonIndex, Rgba8::TOTAL_TRANSPARENT, Rgba8::WHITE);
	m_mainMenuWidget->AddTextLineToWidget(quitBotton);

	// logo
	Vec2 logoPos(0.5f, 0.5f);
	ImageBox logoBox(logoPos, 0.f, 0.6f, g_textures[LOGO]);
	m_mainMenuWidget->m_imageBoxes.push_back(logoBox);	
	
	Vec2 stripPos(0.24f, 0.5f);
	ImageBox stripBox(stripPos, 0.01f, 1.f, nullptr);
	m_mainMenuWidget->m_imageBoxes.push_back(stripBox);
}

void Game::CreateSplashScreenWidget()
{
	float ratio = g_theWindow->GetCurrentAspectRatio();
	m_splashScreenWidget = new Widget(Vec2(0.f, 1.f), ratio, Vec2(0.5f, 0.5f), &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK, 0.f, Rgba8::WHITE);
	m_splashScreenWidget->m_designedByHeight = true;

	Vec2 logoPos(0.5f, 0.5f);
	ImageBox logoBox(logoPos, 0.f, 0.6f, g_textures[LOGO]);
	m_splashScreenWidget->m_imageBoxes.push_back(logoBox);

	AABB2 instructionBounds(Vec2(0.f, 0.f), Vec2(1.f, 1.f));
	std::string instruction("Press Enter or Click to start\nEscape to quit");
	TextLine* instructionLine = new TextLine(instruction, Vec2(0.5f, 0.06f), 64.f, instructionBounds);
	m_splashScreenWidget->AddTextLineToWidget(instructionLine);
	
	AABB2 titleBounds(Vec2(0.f, 0.f), Vec2(1.f, 1.f));
	std::string title("VAPORUM");
	TextLine* titleLine = new TextLine(title, Vec2(0.5f, 0.95f), 128.f, titleBounds, 0.f, Vec2(), Rgba8::WHITE);
	m_splashScreenWidget->AddTextLineToWidget(titleLine);
}

void Game::CreatePauseMenuWidget()
{
	float ratio = g_theWindow->GetCurrentAspectRatio();
	m_pauseMenuWidget = new Widget(Vec2(0.f, 1.f), ratio, Vec2(0.5f, 0.5f), &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK, 0.f, Rgba8::WHITE);
	m_pauseMenuWidget->m_designedByHeight = true;

	AABB2 titleBounds(Vec2(0.3f, 0.3f), Vec2(.7f, .7f));
	std::string title("Pause Menu");
	TextLine* titleLine = new TextLine(title, Vec2(0.5f, 0.5f), 128.f, titleBounds, 90.f, Vec2(-0.3f, 0.f), Rgba8::WHITE, Rgba8::BLACK, "undefined", INVALID_BUTTON_INDEX, Rgba8::BLACK, Rgba8::WHITE);
	m_pauseMenuWidget->AddTextLineToWidget(titleLine);

	// if net system is not enabled, we only have local game option
	int buttonIndex = 0;

	AABB2 buttonBounds(Vec2(0.25f, 0.55f), Vec2(.8f, .65f));
	std::string ResumeOption("Resume Game");
	TextLine* resumeBotton = new TextLine(ResumeOption, Vec2(0.f, 0.5f), 64.f, buttonBounds, 0.f, Vec2(0.f, 0.f), Rgba8::WHITE, Rgba8::BLACK, "Resume Game", buttonIndex, Rgba8::TOTAL_TRANSPARENT, Rgba8::WHITE);
	m_pauseMenuWidget->AddTextLineToWidget(resumeBotton);

	Vec2 spacing(0.f, 0.12f);

	// quit option is always here
	++buttonIndex;
	buttonBounds.m_mins -= spacing;
	buttonBounds.m_maxs -= spacing;
	std::string mainMenuOption("Main Menu");
	TextLine* mainMenuBotton = new TextLine(mainMenuOption, Vec2(0.f, 0.5f), 64.f, buttonBounds, 0.f, Vec2(0.f, 0.f), Rgba8::WHITE, Rgba8::BLACK, "Main Menu", buttonIndex, Rgba8::TOTAL_TRANSPARENT, Rgba8::WHITE);
	m_pauseMenuWidget->AddTextLineToWidget(mainMenuBotton);

	// logo
	Vec2 logoPos(0.5f, 0.5f);
	ImageBox logoBox(logoPos, 0.f, 0.6f, g_textures[LOGO]);
	m_pauseMenuWidget->m_imageBoxes.push_back(logoBox);

	Vec2 stripPos(0.24f, 0.5f);
	ImageBox stripBox(stripPos, 0.01f, 1.f, nullptr);
	m_pauseMenuWidget->m_imageBoxes.push_back(stripBox);
}

void Game::CreateEndTurnWidget()
{
	Vec2 turnInfoSize(0.f, 0.3f);
	m_endTurnWidget = new Widget(turnInfoSize, 4.f, Vec2(0.5f, 0.5f), &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK_TRANSPARENT, 0.02f, Rgba8::WHITE);
	m_endTurnWidget->m_designedByHeight = true;

	std::string endTurn = "End Turn?";
	AABB2 turnInfoBounds(Vec2(0.02f, 0.6f), Vec2(0.98f, 0.95f));
	TextLine* playerTurnTextLine = new TextLine(endTurn, Vec2(0.5f, 0.f), 64.f, turnInfoBounds);
	m_endTurnWidget->AddTextLineToWidget(playerTurnTextLine);

	std::string instruction = "ENTER or click to continue\nESCAPE to cancel";
	AABB2 instructionBounds(Vec2(0.02f, 0.02f), Vec2(0.98f, 0.5f));
	TextLine* instructionTextLine = new TextLine(instruction, Vec2(0.5f, 0.f), 32.f, instructionBounds);
	m_endTurnWidget->AddTextLineToWidget(instructionTextLine);
}

void Game::CreateWaitingForPlayersWidget()
{
	Vec2 turnInfoSize(0.f, 0.3f);
	m_waitingWidget = new Widget(turnInfoSize, 4.f, Vec2(0.5f, 0.5f), &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK_TRANSPARENT, 0.02f, Rgba8::WHITE);
	m_waitingWidget->m_designedByHeight = true;

	std::string endTurn = "Waiting for players";
	AABB2 turnInfoBounds(Vec2(0.02f, 0.6f), Vec2(0.98f, 0.95f));
	TextLine* playerTurnTextLine = new TextLine(endTurn, Vec2(0.5f, 0.f), 64.f, turnInfoBounds);
	m_waitingWidget->AddTextLineToWidget(playerTurnTextLine);

	std::string instruction = "ESCAPE to cancel";
	AABB2 instructionBounds(Vec2(0.02f, 0.02f), Vec2(0.98f, 0.5f));
	TextLine* instructionTextLine = new TextLine(instruction, Vec2(0.5f, 0.f), 32.f, instructionBounds);
	m_waitingWidget->AddTextLineToWidget(instructionTextLine);
}

void Game::CreateCenterAndEdgePlayerTurnWidgets()
{
	Vec2 turnInfoSize(0.f, 0.3f);
	m_turnStartWidget = new Widget(turnInfoSize, 4.f, Vec2(0.5f, 0.5f), &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK_TRANSPARENT, 0.02f, Rgba8::WHITE);
	m_turnStartWidget->m_designedByHeight = true;

	std::string whoseTurn;
	if (m_currentTurnPlayerIndex == 0)
	{
		whoseTurn = "Player 1's Turn";
	}
	else
	{
		whoseTurn = "Player 2's Turn";
	}
	AABB2 turnInfoBounds(Vec2(0.02f, 0.6f), Vec2(0.98f, 0.95f));
	TextLine* playerTurnTextLine = new TextLine(whoseTurn, Vec2(0.5f, 0.f), 64.f, turnInfoBounds);
	m_turnStartWidget->AddTextLineToWidget(playerTurnTextLine);

	std::string instruction = "ENTER or click to continue";
	AABB2 instructionBounds(Vec2(0.02f, 0.02f), Vec2(0.98f, 0.04f));
	TextLine* instructionTextLine = new TextLine(instruction, Vec2(0.5f, 0.f), 32.f, instructionBounds);
	m_turnStartWidget->AddTextLineToWidget(instructionTextLine);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// edge widget
	Vec2 widgetAlignment;
	if (m_currentTurnPlayerIndex == 0)
	{
		widgetAlignment = Vec2(0.01f, 0.98f);
	}
	else if (m_currentTurnPlayerIndex == 1)
	{
		widgetAlignment = Vec2(0.99f, 0.98f);
	}

	Vec2 edgeTurnInfoSize(0.f, 0.09f);
	m_edgeTurnWidget = new Widget(edgeTurnInfoSize, 6.f, widgetAlignment, &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK_TRANSPARENT, 0.02f, Rgba8::WHITE);
	m_edgeTurnWidget->m_designedByHeight = true;

	AABB2 playerTurnBounds(Vec2(0.02f, 0.02f), Vec2(0.98f, 0.98f));
	TextLine* edgePlayerTurnTextLine = new TextLine(whoseTurn, Vec2(0.5f, 0.5f), 32.f, playerTurnBounds, 0.f, Vec2(), Rgba8::GREEN);
	m_edgeTurnWidget->AddTextLineToWidget(edgePlayerTurnTextLine);
}


void Game::CreateWinnerWidget()
{
	Vec2 turnInfoSize(0.f, 0.3f);
	m_winnerWidget_0 = new Widget(turnInfoSize, 4.f, Vec2(0.5f, 0.5f), &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK_TRANSPARENT, 0.02f, Rgba8::WHITE);
	m_winnerWidget_0->m_designedByHeight = true;

	std::string whoWins = "Player 1 Wins";
	AABB2 turnInfoBounds(Vec2(0.02f, 0.6f), Vec2(0.98f, 0.95f));
	TextLine* player1TurnTextLine = new TextLine(whoWins, Vec2(0.5f, 0.f), 64.f, turnInfoBounds);
	m_winnerWidget_0->AddTextLineToWidget(player1TurnTextLine);

	std::string instruction = "ENTER or click to continue";
	AABB2 instructionBounds(Vec2(0.02f, 0.02f), Vec2(0.98f, 0.04f));
	TextLine* instructionTextLine = new TextLine(instruction, Vec2(0.5f, 0.f), 32.f, instructionBounds);
	m_winnerWidget_0->AddTextLineToWidget(instructionTextLine);	
	
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	m_winnerWidget_1 = new Widget(turnInfoSize, 4.f, Vec2(0.5f, 0.5f), &m_screenCamera, g_theWindow, g_consoleFont, nullptr, true, Rgba8::BLACK_TRANSPARENT, 0.02f, Rgba8::WHITE);
	m_winnerWidget_1->m_designedByHeight = true;

	whoWins = "Player 2 Wins";
	TextLine* player2TurnTextLine = new TextLine(whoWins, Vec2(0.5f, 0.f), 64.f, turnInfoBounds);
	m_winnerWidget_1->AddTextLineToWidget(player2TurnTextLine);

	m_winnerWidget_1->AddTextLineToWidget(instructionTextLine);
}

void Game::UseArrowKeysToControlWidgetButtons(Widget* widget)
{
	// when pressing arrow up and down, the widget's button index will increase or decrease
	if (g_theInput->WasKeyJustPressed(KEYCODE_DOWNARROW))
	{
		++widget->m_currentButtonIndex;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_UPARROW))
	{
		--widget->m_currentButtonIndex;
	}
}

void Game::ControlLightingSettings()
{
	auto iter = m_entities.find("Sun");
	if (iter != m_entities.end())
	{
		Entity* sun = iter->second;

		if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW))
		{
			sun->m_orientation.m_yawDegrees -= 0.5f;
		}		
		if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW))
		{
			sun->m_orientation.m_yawDegrees += 0.5f;
		}		
		if (g_theInput->IsKeyDown(KEYCODE_UPARROW))
		{
			sun->m_orientation.m_pitchDegrees += 0.5f;
		}
		if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW))
		{
			sun->m_orientation.m_pitchDegrees -= 0.5f;
		}		
		
		//----------------------------------------------------------------------------------------------------------------------------------------------------
		if (g_theInput->WasKeyJustPressed(KEYCODE_LESSTHAN))
		{
			if (m_phongLighinting->SunIntensity > 0.f)
			{
				m_phongLighinting->SunIntensity -= 0.1f;
			}
		}		
		if (g_theInput->WasKeyJustPressed(KEYCODE_GREATERTHAN))
		{
			if (m_phongLighinting->SunIntensity < 1.f)
			{
				m_phongLighinting->SunIntensity += 0.1f;
			}
		}

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		m_phongLighinting->SunDirection = sun->m_orientation.GetForwardIBasis().GetNormalized();
		m_phongLighinting->WorldEyePosition = m_players[0]->m_playerCamera.m_position;
		m_phongLighinting->AmbientIntensity = 1.f - m_phongLighinting->SunIntensity;
	}
}

void Game::GetWindowsLastErrorAndDisplayOnScreen()
{
	// Retrieves the calling thread's last-error code value
	DWORD errorCode = GetLastError();

	// translate the error code to a readable message
	LPSTR messageBuffer = nullptr;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	FormatMessageA(
		flags,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&messageBuffer,
		0, NULL);

	// show the fail message on screen
	std::string errorMessage(messageBuffer);
	DebugAddMessage(errorMessage, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

	LocalFree(messageBuffer);  // Free the buffer allocated by FormatMessage
}

void Game::LoadNewModel()
{
	// check if there is same name XML, is yes, load the transform info
	// Buffers for split the file path
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	// Split the file path into components
	_splitpath_s(m_modelFilePath.c_str(), drive, dir, fname, ext);

	Model* model = new Model(this);
	// use xml to load a model
	if (_stricmp(ext, ".xml") == 0)
	{
		std::string loadingInfo = "Identify xml file, loading... Please wait...";
		DebugAddMessage(loadingInfo, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

		if (model->LoadXml(m_modelFilePath))
		{
			std::string errorMsg = "File loading succeeded";
			DebugAddMessage(errorMsg, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

			model->m_objectFileName = m_modelFilePath;
			m_loadedModels.push_back(model);
		}
		else
		{
			std::string errorMsg = "File loading failed";
			DebugAddMessage(errorMsg, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
		}
	}
	// use obj to load a model
	else if (_stricmp(ext, ".obj") == 0)
	{
		std::string loadingInfo = "Identify obj file, loading... Please wait...";
		DebugAddMessage(loadingInfo, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

		if (model->LoadObj(m_modelFilePath))
		{
			std::string errorMsg = "File loading succeeded";
			DebugAddMessage(errorMsg, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

			model->m_objectFileName = m_modelFilePath;
			m_loadedModels.push_back(model);
		}
		else
		{
			std::string errorMsg = "File loading failed";
			DebugAddMessage(errorMsg, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
		}

	}
	else
	{
		std::string errorMsg = "File type do not support. Sorry.";
		DebugAddMessage(errorMsg, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}
}

void Game::ShutDownUIElementList(int arraySize, UI** m_UIElementArrayPointer)
{
	for (int i = 0; i < arraySize; ++i)
	{
		UI*& UIPtr = m_UIElementArrayPointer[i];// the first const says that the entity instance is const, the second const say that the entity pointer is const

		// tell every existing asteroid to draw analysis
		if (CheckUIEnabled(UIPtr)) // the function that inside a const function calls must be const
		{
			delete UIPtr;
			UIPtr = nullptr;
		}
	}

}

void Game::Shutdown()
{
	if (IsNetworkGameMode())
	{
		PlayerQuit();
	}

	for (int i = 0; i < (int)m_allMaps.size(); ++i)
	{
		if (m_allMaps[i])
		{
			delete m_allMaps[i];
		}
	}

	if (m_groundVertexBuffer)
	{
		delete m_groundVertexBuffer;
	}

	if (m_groundIndexBuffer)
	{
		delete m_groundIndexBuffer;
	}

	delete m_FPSTimer;
	m_FPSTimer = nullptr;

	for (int i = 0; i < (int)m_players.size(); ++i)
	{
		delete m_players[i];
		m_players[i] = nullptr;
	}

	for (int i = 0; i < (int)UnitDefinition::s_unitDefs.size(); ++i)
	{
		UnitDefinition::s_unitDefs[i].ShutDown();
	}

	delete g_theGameClock;
}

Vec2 Game::GetRandomPosInWorld(Vec2 worldSize)
{
	float posX = g_rng->RollRandomFloatInRange(0, worldSize.x);
	float posY = g_rng->RollRandomFloatInRange(0, worldSize.y);
	return Vec2(posX, posY);
}

// add camera shake when player is dead
void Game::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

bool Game::IsNetworkGameMode() const
{
	return !(m_players[0]->m_netState == NetState::LOCAL && m_players[1]->m_netState == NetState::LOCAL);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Game::ClearTestScene()
{
	m_testSceneProps.clear();
	m_loadedModels.clear();
}

void Game::SwitchSceneBasedOnInput()
{
	// F6 and F7 for switching different testing scene
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTBRACKET))
	{
		m_testSceneIndex -= 1;

		if (m_testSceneIndex == -1)
		{
			m_testSceneIndex = NUM_TESTSCENE - 1;

			DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(m_testSceneIndex));
		}
		else
		{
			DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(m_testSceneIndex));
		}
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTBRACKET))
	{
		m_testSceneIndex += 1;

		if (m_testSceneIndex == NUM_TESTSCENE)
		{
			m_testSceneIndex = 0;

			DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(m_testSceneIndex));
		}
		else
		{
			DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(m_testSceneIndex));
		}
	}
}

void Game::DeleteCurrentTestMode_And_CreateNewTestMode(TestingScene type)
{
	ClearTestScene();

	switch (type)
	{
	case EMISSIVE_CUBES:
	{
		Model* model_textured = new Model(this);
		model_textured->LoadXml("Data/Models/Cube_Textured.xml");
		model_textured->m_position = Vec3(0.f, 0.f, 0.5f);
		m_loadedModels.push_back(model_textured);
		
		Model* model_W = new Model(this);
		model_W->LoadXml("Data/Models/Cube_Emissive.xml");
		model_W->m_position = Vec3(-1.f, -1.f, 0.5f);
		m_loadedModels.push_back(model_W);
		
		Model* model_R = new Model(this);
		model_R->LoadXml("Data/Models/Cube_Emissive.xml");
		model_R->m_position = Vec3(1.f, -1.f, 0.5f);
		model_R->m_color = Rgba8::RED;
		m_loadedModels.push_back(model_R);
		
		Model* model_G = new Model(this);
		model_G->LoadXml("Data/Models/Cube_Emissive.xml");
		model_G->m_position = Vec3(1.f, 1.f, 0.5f);
		model_G->m_color = Rgba8::GREEN;
		m_loadedModels.push_back(model_G);
		
		Model* model_B = new Model(this);
		model_B->LoadXml("Data/Models/Cube_Emissive.xml");
		model_B->m_position = Vec3(-1.f, 1.f, 0.5f);
		model_B->m_color = Rgba8::BLUE;
		m_loadedModels.push_back(model_B);
	}
		break;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	case TEXTURED_PROPS:
	{
		// Prop* Grass_Cube = new Prop();
		// Grass_Cube->m_name = "Grass_Cube";
		// Grass_Cube->m_position = Vec3(-2.f, -2.f, 0.f);
		// Grass_Cube->m_material = g_materials[GRASS];
		// Grass_Cube->CreateCube();
		// Grass_Cube->CreateVertexAndIndexBuffer();		
		// m_testSceneProps.push_back(Grass_Cube);
		// 
		// Prop* Grass_Sphere = new Prop();
		// Grass_Sphere->m_name = "Grass_Sphere";
		// Grass_Sphere->m_position = Vec3(2.f, -2.f, 0.f);
		// Grass_Sphere->m_material = g_materials[GRASS];
		// Grass_Sphere->CreateSphere();
		// Grass_Sphere->CreateVertexAndIndexBuffer();
		// m_testSceneProps.push_back(Grass_Sphere);
		// 
		// Prop* Brick_Cube = new Prop();
		// Brick_Cube->m_name = "Brick_Cube";
		// Brick_Cube->m_position = Vec3(2.f, 2.f, 0.f);
		// Brick_Cube->m_material = g_materials[BRICK];
		// Brick_Cube->CreateCube();
		// Brick_Cube->CreateVertexAndIndexBuffer();
		// m_testSceneProps.push_back(Brick_Cube);
		// 
		// Prop* Brick_Sphere = new Prop();
		// Brick_Sphere->m_name = "Brick_Sphere";
		// Brick_Sphere->m_position = Vec3(-2.f, 2.f, 0.f);
		// Brick_Sphere->m_material = g_materials[BRICK];
		// Brick_Sphere->CreateSphere();
		// Brick_Sphere->CreateVertexAndIndexBuffer();
		// m_testSceneProps.push_back(Brick_Sphere);
	}
		break;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	case TUTORIAL_BOX:
	{
		Model* tutorialBox = new Model(this);
		tutorialBox->LoadXml("Data/Models/Tutorial_Box.xml");
		m_loadedModels.push_back(tutorialBox);
	}
		break;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	case HADRIAN_TANK:
	{
		Model* tutorialBox = new Model(this);
		tutorialBox->LoadXml("Data/Models/Hadrian.xml");
		m_loadedModels.push_back(tutorialBox);
	}
		break;
	
	default:
		return;
		break;
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

bool Game::CheckUIEnabled(UI* const UI) const
{
	UNUSED(UI);
	return false;
}

bool Game::DoEntitiesOverlap(Entity const& a, Entity const& b)
{
	UNUSED(a);
	UNUSED(b);
	return false;
}


