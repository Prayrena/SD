#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/core/Clock.hpp"
#include "Game/PlayerTank.hpp"
#include "Game/Entity.hpp"
#include "Game/UI.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/Map.hpp"
#include "Game/Tile.hpp"

PlayerTank* g_playerTank = nullptr;
Clock* g_theGameClock = nullptr;

extern App* g_theApp;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;

Game::Game()
{

}

Game::~Game()
{

}

void Game::Startup()
{
	g_theGameClock = new Clock();

	DefineAnimation();
	TileTypeDefinition::InitializeTileDefs();
	GenerateAllMaps();

	// initialize the player
	Vec2 playerStartPos = Vec2(1.5f, 1.5f);
	m_playerTank = new PlayerTank(m_allMaps[0], ENTITY_TYPE_GOOD_PLAYER, FACTION_GOOD, playerStartPos, 45.f);// Spawn the player ship
	m_currentMap->AddEntityToMap(*m_playerTank);

	//set the cameras
	Vec2 worldSize;
	worldSize.x = g_gameConfigBlackboard.GetValue("world_size_x", 1600.f);
	worldSize.y = g_gameConfigBlackboard.GetValue("world_size_y", 800.f);

	AABB2 cameraStart(Vec2(0.f, 0.f), worldSize);
	//cameraStart.SetDimensions(Vec2(100.f, 50.f));
	//cameraStart.SetCenter(playerStartPos);
	//m_worldCamera.SetOrthoView(Vec2(cameraStart.m_mins.x, cameraStart.m_mins.y), Vec2(cameraStart.m_maxs.x, cameraStart.m_maxs.y));
	m_worldCamera.SetOrthoView(Vec2::ZERO, worldSize);
	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));
}

void Game::DefineAnimation()
{
	SpriteSheet* explosionSpriteSheet = new SpriteSheet( *g_theApp->g_textureID[SPRITESHEET_EXPLOSION], IntVec2(5, 5) );
	m_tankExplosionAnim		= new SpriteAnimDefinition(*explosionSpriteSheet, 0, 24, 2.f);
	m_bulletExplosionAnim	= new SpriteAnimDefinition(*explosionSpriteSheet, 0, 24, 1.f);
	m_muzzleFlashAnim		= new SpriteAnimDefinition(*explosionSpriteSheet, 0, 24, 0.5f);
}

void Game::Update(float deltaSeconds)
{
	if (m_UIisDisplayed)
	{
		UpdateUI(deltaSeconds);
	}

	CheckPlayerLives(deltaSeconds);		

	if (m_currentMap)
	{
		if (m_gameState != VICTORY)
		{
			m_currentMap->Update(deltaSeconds);
		}
		SwitchBetweenMaps();
	}

	UpdateCameras(deltaSeconds);
	CheckIfPlayerHasGotToExit();
}
/// <render>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Game::Render() const
{
	switch (m_gameState)
	{
	case PLAYING:
	{
		RenderPlayingMode();
	} break;
	case PERISHED:
	{
		RenderPerishedMode();
	} break;
	case VICTORY:
	{
		RenderVictoryMode();
	}
		break;
	}
}

void Game::RenderPlayingMode() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);
	if (m_currentMap)
	{
		m_currentMap->Render();
	}
	if (g_theApp->m_debugMode)
	{
		if (m_currentMap)
		{
			m_currentMap->RenderDebug();
		}
	}
	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render in game UI
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderUIElements();
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderPerishedMode() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);
	if (m_currentMap)
	{
		m_currentMap->Render();
	}
	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderUIElements();
	RenderPlayerDeathMenuUI();
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderPlayerDeathMenuUI() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 testingTexture = m_screenCamera.GetCameraBounds();
	AddVertsForAABB2D(verts, testingTexture, Rgba8::WHITE);
	g_theRenderer->BindTexture(g_theApp->g_textureID[T_PERISHED_MENU]);
	g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
}

void Game::RenderVictoryMode() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderUIElements();
	RenderVictoryMenuUI();
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderVictoryMenuUI() const
{
	std::vector<Vertex_PCU> verts;
	AABB2 testingTexture = m_screenCamera.GetCameraBounds();
	AddVertsForAABB2D(verts, testingTexture, Rgba8::WHITE);
	g_theRenderer->BindTexture(g_theApp->g_textureID[T_VICTORY_MENU]);
	g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

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
	for ( int mapIndex = 0; mapIndex < NUM_MAP; ++mapIndex )
	{
		delete m_allMaps[mapIndex];
	}
	m_allMaps.clear();
	m_playerTank = nullptr;
}

// world settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#ToDo:generate all the maps
void Game::GenerateAllMaps()
{
	// make sure all maps are cleared up before generating because player might restart
	m_allMaps.reserve(3);
	for (int i = 0; i < m_allMaps.size(); ++i)
	{
		delete m_allMaps[i];
	}

	// get three map definition from xml
	MapDefinition::InitializeMapDefs();
	for (int i = 0; i < MapDefinition::s_mapDefs.size(); ++i)
	{
		// put into the map list by names
		if (MapDefinition::s_mapDefs[i].m_name == "highland")
		{
			Map* map1 = new Map(MapDefinition::s_mapDefs[i]);
			m_allMaps.push_back(map1);
		}
		if (MapDefinition::s_mapDefs[i].m_name == "northPole")
		{
			Map* map2 = new Map(MapDefinition::s_mapDefs[i]);
			m_allMaps.push_back(map2);
		}
		if (MapDefinition::s_mapDefs[i].m_name == "insideVolcano")
		{
			Map* map3 = new Map(MapDefinition::s_mapDefs[i]);
			m_allMaps.push_back(map3);
		}
	}

	m_currentMap = m_allMaps[0];
}

void Game::TeleportPlayersBetweenDifferentMaps()
{
	EntityPtrList const& playersToMove = m_currentMap->GetEntitiesByType(ENTITY_TYPE_GOOD_PLAYER);
	for (int playerIndex = 0; playerIndex < (int)playersToMove.size(); ++playerIndex)
	{
		// remove the player tank from current map and add it to the next map
		m_currentMap->RemoveEntityFromMap(*playersToMove[playerIndex]);
		Map* nextMap = m_allMaps[m_nextMapIndex];
		playersToMove[playerIndex]->m_position = Vec2(1.5f, 1.5f);//define the position of the player on new map
		playersToMove[playerIndex]->m_orientationDegrees = 45.f;
		nextMap->AddEntityToMap(*playersToMove[playerIndex]);
	}
}

bool Game::FindPlayerReferenceOnCurrentMap(Entity*& playerPtr)
{
	if (m_currentMap)
	{
		if (m_currentMap->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0])
		{
			playerPtr = m_currentMap->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0];
			return true;
		}
	}

	return false;
}

void Game::CheckIfPlayerHasGotToExit()
{
	IntVec2 exitTileCoords = IntVec2((m_currentMap->m_dimensions.x - 2), (m_currentMap->m_dimensions.y - 2));
	int exitTileIndex = m_currentMap->GetTileIndex_For_TileCoordinates(exitTileCoords);
	AABB2 exitBounds = m_currentMap->m_tiles[exitTileIndex].GetBounds();

	if (m_playerTank)
	{
		if (exitBounds.IsPointInside(m_playerTank->m_position))
		{
			// if player reaches the exit point on the last map, show victory
			if ( m_currentMapIndex == (m_allMaps.size() - 1) )
			{
				m_gameState = VICTORY;
				g_theAudio->SetSoundPlaybackSpeed(g_theApp->m_level1_Bgm, 0.f);
				if (!m_victorySoundIsPlayed)
				{
					g_theAudio->StartSound(g_theApp->g_soundEffectsID[PLAYER_VICTORY], false, 1.0f, 0.f, 1.f, false);
					m_victorySoundIsPlayed = true;
				}
			}
			else
			{
				m_nextMapIndex += 1;
				TeleportPlayersBetweenDifferentMaps();
				m_currentMap = m_allMaps[m_nextMapIndex];
				m_currentMapIndex = m_nextMapIndex;
				g_theAudio->StartSound(g_theApp->g_soundEffectsID[CURRENT_MAP_COMPLETE], false, 1.0f, 0.f, 1.f, false);
			}
		}
	}
}

// controlling the map switching
void Game::SwitchBetweenMaps()
{
	// F6 for switching between different heat maps
	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		m_heatmapIndex += 1;
		if (m_heatmapIndex == NUM_HEATMAP) // when all the heat maps are shown, press again will turn it off
		{
			m_showHeatMap = false;
			m_heatmapIndex = -1;
		}
		else // will turn on showing the heat map and switching between different maps
		{
			m_showHeatMap = true;
			m_heatMapState = static_cast<HeatMapAnalysis>(m_heatmapIndex);
		}
	}

	// F7 and F9 for switching different testing scene
	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
			m_nextMapIndex -= 1;
		if (m_nextMapIndex == -1)
		{
			m_nextMapIndex = NUM_MAP -1;
			TeleportPlayersBetweenDifferentMaps();
			m_currentMap = m_allMaps[m_nextMapIndex];
			m_currentMapIndex = m_nextMapIndex;
		}
		else
		{
			TeleportPlayersBetweenDifferentMaps();
			m_currentMap = m_allMaps[m_nextMapIndex];
			m_currentMapIndex = m_nextMapIndex;
		}
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F9))
	{
		m_nextMapIndex += 1;
		if (m_nextMapIndex == NUM_MAP)
		{
			m_nextMapIndex = 0;
			TeleportPlayersBetweenDifferentMaps();
			m_currentMap = m_allMaps[m_nextMapIndex];
			m_currentMapIndex = m_nextMapIndex;

			// m_gameState = VICTORY;//game testing ends
		}
		else
		{
			TeleportPlayersBetweenDifferentMaps();
			m_currentMap = m_allMaps[m_nextMapIndex];
			m_currentMapIndex = m_nextMapIndex;
		}
	}
}

/// <UI Settings>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// <health bar>
void Game::InitializePlayerLives()
{
	//for ( int i = 0; i < m_playerLivesNum; ++i )
	//{
	//	Vec2 iconPos (UI_POSITION_PLAYER_LIVES_X + (i * UI_POSITION_PLAYER_LIVES_SPACING), UI_POSITION_PLAYER_LIVES_Y);
	//	m_UI_lives[i] = new PlayerShipIcon(this, iconPos);
	//	m_UI_lives[i]->m_orientationDegrees = 90.f;
	//}
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

	// if the player dies, do not update the camera
	if (!m_playerTank)
	{
		return;
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	//the camera follows the player and will not go out of bounds
	if (!m_playerTank->m_isDead)
	{
		Vec2 worldSize;
		worldSize.x = g_gameConfigBlackboard.GetValue("world_size_x", 1600.f);
		worldSize.y = g_gameConfigBlackboard.GetValue("world_size_y", 800.f);
		m_worldCamera.SetOrthoView(Vec2::ZERO, worldSize);

		followingCamera.m_mins = m_worldCamera.GetOrthoBottomLeft();
		followingCamera.m_maxs = m_worldCamera.GetOrthoTopRight();
		followingCamera.SetCenter(m_playerTank->m_position);
		//followingCamera.m_mins.x = GetClamped(followingCamera.m_mins.x, 0.f, WORLD_SIZE_X);
		//followingCamera.m_mins.y = GetClamped(followingCamera.m_mins.y, 0.f, WORLD_SIZE_Y);
		//followingCamera.m_maxs.x = GetClamped(followingCamera.m_maxs.x, 0.f, WORLD_SIZE_X);
		//followingCamera.m_maxs.y = GetClamped(followingCamera.m_maxs.y, 0.f, WORLD_SIZE_Y);

		if (followingCamera.m_mins.x < 0.f)
		{
			followingCamera.Translate(Vec2(-followingCamera.m_mins.x, 0));
		}
		if (followingCamera.m_mins.y < 0.f)
		{
			followingCamera.Translate(Vec2(0.f, -followingCamera.m_mins.y));
		}
		if (followingCamera.m_maxs.x > m_currentMap->m_dimensions.x)
		{
			followingCamera.Translate(Vec2(m_currentMap->m_dimensions.x - followingCamera.m_maxs.x, 0));
		}
		if (followingCamera.m_maxs.y > m_currentMap->m_dimensions.y)
		{
			followingCamera.Translate(Vec2(0.f, m_currentMap->m_dimensions.y - followingCamera.m_maxs.y));
		}


		m_worldCamera.SetOrthoView(Vec2(followingCamera.m_mins), Vec2(followingCamera.m_maxs));
		m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));
		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// if debug camera mode is on, override the world camera setting
		if (m_debugCamerIsOn)
		{
			// based on the mapSize scale, the world camera zoom to see the whole picture
			Vec2 mapSize = m_currentMap->GetMapDimensions();
			Vec2 cameraSize;
			if (mapSize.x / mapSize.y > 2)
			{
				cameraSize.x = mapSize.x;
				cameraSize.y = cameraSize.x * 0.5f;
			}
			else
			{
				cameraSize.y = mapSize.y;
				cameraSize.x = cameraSize.y * 2;
			}
			m_worldCamera.SetOrthoView(Vec2(0.f, 0.f), cameraSize);
		}
	}
}

void Game::TurnCameraShakeOnForPlayerDeath()
{
	// tell that camera shake has not been displayed yet(should start)
	m_cameraShakeIsDisplayed = false;
	m_screenShakeAmount = CAMERA_SHAKE_AMOUNT_PLAYERDEAD;
}

void Game::TurnCameraShakeOnForExplosion()
{
	// tell that camera shake has not been displayed yet(should start)
	m_cameraShakeIsDisplayed = false;
	m_screenShakeAmount = CAMERA_SHAKE_AMOUNT_EXPLOSION;
}

void Game::UpdateUI(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	// CheckPlayerLives(deltaSeconds);
	// UpdateEnergyBar(deltaSeconds);
	// UpdateEnergySelectionRing(deltaSeconds);
}


void Game::CheckPlayerLives(float deltaSeconds)
{
	Entity* playerTank = m_currentMap->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0];
	if (!playerTank->isAlive())// if the player is dead and cleared by the map class
	{
		m_transitToPerishTimer += deltaSeconds;

		// g_theApp->m_isSlowMo = true;
		// m_returnToStartTimer += deltaSeconds * 10.f;// make the delta second's value without the effect of the slow mode

		if (m_transitToPerishTimer >= TIME_RETURN_TO_PERISHMODE)
		{
			m_gameState = PERISHED;
			g_theAudio->SetSoundPlaybackSpeed(g_theApp->m_level1_Bgm, 0.f);
			if (!m_defeatSoundIsPlayed)
			{
				g_theAudio->StartSound(g_theApp->g_soundEffectsID[PLAYER_DEFEAT], false, 1.0f, 0.f, 1.f, false);
				m_defeatSoundIsPlayed = true;
			}
		}

		XboxController const& playerController = g_theInput->GetController(0);
		// respawn after defeat
		if (g_theInput->WasKeyJustReleased('N') || playerController.WasButtonJustReleased(XboxButtonID::XBOX_BUTTON_START))
		{
			RespawnPlayer();
		}
	}
	else
	{
		m_playerCheckPoint = m_playerTank->m_position;
		m_playerCheckPointOrientation = m_playerTank->m_orientationDegrees;

	}
}

void Game::RespawnPlayer()
{
	m_transitToPerishTimer = 0.f;
	g_theAudio->SetSoundPlaybackSpeed(g_theApp->m_level1_Bgm, 1.f);

	// only allow to respawn playerShip when player is dead
	if (m_playerTank->m_isDead)
	{
		// m_currentMap->AddEntityToMap( *dynamic_cast<Entity*>(m_playerTank) );
		m_playerTank->ResetToRespawn();
		m_gameState = PLAYING;
		m_defeatSoundIsPlayed = false;
		g_theApp->m_speedIsBoosted = false;
	}
}

void Game::RenderUIElements() const
{
	if (g_theApp->m_isPaused)
	{
		std::vector<Vertex_PCU> verts;
		AABB2 testingTexture = AABB2(0.f, 0.f, 1600.f, 800.f);
		AddVertsForAABB2D(verts, testingTexture, Rgba8(0, 0, 0, 100));
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
	}
}

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


void Game::RenderUIElementList(int arraySize, UI* const* m_UIElementArrayPointer) const
{
	for (int i = 0; i < arraySize; ++i)
	{
		UI* const UIptr = m_UIElementArrayPointer[i];// we only need to promise that the entity ptr will not be changed because the instance that the pointer point to do not own by the class
		if (CheckUIEnabled(UIptr))
		{
			UIptr->Render();
		}
	}
}

void Game::UpdateUIElementList(int arraySize, UI* const* m_UIElementArrayPointer, float deltaSeconds) const
{
	for (int i = 0; i < arraySize; ++i)
	{
		UI* const UIptr = m_UIElementArrayPointer[i];// we only need to promise that the entity ptr will not be changed because the instance that the pointer point to do not own by the class
		if (CheckUIEnabled(UIptr))
		{
			UIptr->Update(deltaSeconds);
		}
	}
}



