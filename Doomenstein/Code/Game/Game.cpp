#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/core/Timer.hpp"
#include "Game/Map.hpp"
#include "Game/Player.hpp"
#include "Game/Tile.hpp"
#include "Game/Player.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Weapon.hpp"
#include "Game/Entity.hpp"
#include "Game/Prop.hpp"
#include "Game/UI.hpp"
#include "Game/EnergyBar.hpp"
#include "Game/EnergySelectionRing.hpp"
#include "Game/App.hpp"

RandomNumberGenerator* g_rng = nullptr; // always initialize the global variable in the cpp file
Clock* g_theGameClock = nullptr;
Clock* g_theColorChangingClock = nullptr;

extern App* g_theApp;
extern InputSystem* g_theInput; 
extern AudioSystem* g_theAudio;
extern Renderer* g_theRenderer;
extern Window* g_theWindow;

Game::Game()
{
}

Game::~Game()
{

}

// TASK:
// spawn PlayerShip in the dead center of the world
// randomly spawn asteroids in the world
void Game::Startup()
{
	// initialize the lives bar
	// InitializePlayerLives();
	// InitializeEnergyBar();

	// initialize a randomNumberGenerator
	g_rng = new RandomNumberGenerator;

	m_sharedScreenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	// set up the clock for the game 
	g_theGameClock = new Clock();
	m_transformFromAttractToGameTimer = new Timer(m_attractModeTransititionDuration);

	EnterState(GameState::ATTRACT);

	m_gameIsOver = false;
	m_playerWins = false;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Game::Update()
{
	switch (m_currentState)
	{
	case GameState::ATTRACT:
	{
		UpdateAttract();
	}break;
	case GameState::LOBBY:
	{
		UpdateLobby();
	}break;
	case GameState::PLAYING:
	{
		UpdatePlaying();
	}break;	
	case GameState::VICTORY:
	{
		UpdateVictory();
	}break;
	}
}

void Game::UpdateAttract()
{
	AABB2 bounds = m_sharedScreenCamera.GetCameraBounds();

	// change ring thickness according to time
	if (m_transformFromAttractToGameTimer->GetElapsedTime() > 0.f)
	{
		m_ringThickness *= (1.f + m_transformFromAttractToGameTimer->GetElapsedTime() * 0.1f);
	}
	else
	{
		m_ringThickness = bounds.m_maxs.x * 0.02f * fabsf(sinf(2.f * (float)Clock::GetSystemClock().GetTotalSeconds()));
	}

	DetectFirstJoiningPlayerInAttractMode();

	// if the timer ends, enter playing mode
	if (m_transformFromAttractToGameTimer->HasPeroidElapsed())
	{
		ExitState(GameState::ATTRACT);
		EnterState(GameState::LOBBY);
	}
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
// lobby functions
void Game::UpdateLobby()
{
	DetectPlayerJoiningAndLeavingLobby();
	UpdatePlayerCamerasSettings();
	RenderPlayerIndexAndInstruction();
}

void Game::UpdatePlayerCamerasSettings()
{
	// change each player's normalized viewport settings and aspect ratio based on number of players
	if (GetNumberOfJoiningPlayer() == 1)
	{
		for (int i = 0; i < (int)m_playersList.size(); ++i)
		{
			if (m_playersList[i])
			{
				AABB2 normalizedViewport1;
				normalizedViewport1.m_mins = Vec2(0.f, 0.0f);
				normalizedViewport1.m_maxs = Vec2(1.f, 1.f);
				m_playersList[i]->SetPlayerNormalizedViewport(normalizedViewport1);
			}
		}
		m_playerCameraAspectRatio = 2.f;
		return;
	}
	if (GetNumberOfJoiningPlayer() == 2)
	{
		for (int i = 0; i < (int)m_playersList.size(); ++i)
		{
			if (m_playersList[i])
			{
				if (m_playersList[i]->m_playerIndex == 1)
				{
					AABB2 normalizedViewport1;
					normalizedViewport1.m_mins = Vec2(0.f, 0.5f);
					normalizedViewport1.m_maxs = Vec2(1.f, 1.f);
					m_playersList[i]->SetPlayerNormalizedViewport(normalizedViewport1);	
				}
				else if (m_playersList[i]->m_playerIndex == 2)
				{
					AABB2 normalizedViewport1;
					normalizedViewport1.m_mins = Vec2(0.f, 0.0f);
					normalizedViewport1.m_maxs = Vec2(1.f, 0.5f);
					m_playersList[i]->SetPlayerNormalizedViewport(normalizedViewport1);
				}
			}
		}

		m_playerCameraAspectRatio = 4.f;
	}
}

void Game::DetectPlayerJoiningAndLeavingLobby()
{
	// connect two controller and see if the controller disconnect and reconnect will remember the original slot in array
	// the player index is to memorize which shown on top of screen, which is shown below, when the first player quit, the second player will still be marked as 2nd player
	// the controller index is to know who is using key board, who is using controller
	
	// leaving
	// space bar to exit the attract mode to the lobby and become the first player with keyboard
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);

		KickingOutPlayer(-1);
	}
	// space bar to exit the attract mode to the lobby and become the first player with controller
	for (int i = 0; i < NUM_XBOX_CONTROLLERS; ++i)
	{
		XboxController const& controller = g_theInput->GetController(i);
		if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
		{
			g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
			m_transformFromAttractToGameTimer->Start();

			KickingOutPlayer(i);
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// joining
	// if (GetNumberOfJoiningPlayer() == 0)
	// {
	// 	// space bar to exit the attract mode to the lobby and become the first player with keyboard
	// 	if (g_theInput->WasKeyJustPressed(' '))
	// 	{
	// 		g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
	// 		m_transformFromAttractToGameTimer->Start();
	// 		if (!RegisterPlayer(1, -1)) // means this controller is controlling a player and pressed start
	// 		{
	// 			ExitState(GameState::LOBBY);
	// 			EnterState(GameState::ATTRACT);
	// 			return;
	// 		}
	// 	}
	// 	// space bar to exit the attract mode to the lobby and become the first player with controller
	// 	for (int i = 0; i < NUM_XBOX_CONTROLLERS; ++i)
	// 	{
	// 		XboxController const& controller = g_theInput->GetController(i);
	// 		if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
	// 		{
	// 			g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
	// 			m_transformFromAttractToGameTimer->Start();
	// 
	// 			if (!RegisterPlayer(1, -1)) // means this controller is controlling a player and pressed start
	// 			{
	// 				ExitState(GameState::LOBBY);
	// 				EnterState(GameState::ATTRACT);
	// 				return;
	// 			}
	// 
	// 			break; // if multiple controller pressing together, we only guarantee only the first one create new player
	// 		}
	// 	}
	// }
	if (GetNumberOfJoiningPlayer() == 1)
	{
		// first find if we are missing player 1 or 2
		int newPlayerIndex = 0;
		int existPlayerIndex = 0;
		for (int i = 0; i < (int)m_playersList.size(); ++i)
		{
			if (m_playersList[i] != nullptr)
			{
				existPlayerIndex = m_playersList[i]->m_playerIndex;
				break;
			}
		}
		if (existPlayerIndex == 1)
		{
			newPlayerIndex = 2;
		}
		else
		{
			newPlayerIndex = 1;
		}
		
		// space bar to exit the attract mode to the lobby and become the first player with keyboard
		if (g_theInput->WasKeyJustPressed(' ') || g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
		{
			g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, .2f, 0.f, 1.f, false);
			m_transformFromAttractToGameTimer->Start();
			if (!RegisterPlayer(newPlayerIndex, -1)) // means this controller is controlling a player and pressed start
			{
				ExitState(GameState::LOBBY);
				EnterState(GameState::PLAYING);
				return;
			}	
		}
		// space bar to exit the attract mode to the lobby and become the first player with controller
		for (int i = 0; i < NUM_XBOX_CONTROLLERS; ++i)
		{
			XboxController const& controller = g_theInput->GetController(i);
			if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
			{
				g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, .2f, 0.f, 1.f, false);
				m_transformFromAttractToGameTimer->Start();

				if (!RegisterPlayer(newPlayerIndex, i)) // means this controller is controlling a player and pressed start
				{
					ExitState(GameState::LOBBY);
					EnterState(GameState::PLAYING);
					return;
				}

				break; // if multiple controller pressing together, we only guarantee only the first one create new player
			}
		}
	} 
	else if (GetNumberOfJoiningPlayer() == 2) // if any of two controller pressed start, game starts
	{
		for (int i = 0; i < (int)m_playersList.size(); ++i)
		{
			if (m_playersList[i] != nullptr)
			{
				// check if any player is using keyboard and space bar is pressed to start game
				if (m_playersList[i]->m_controllerIndex == -1 && g_theInput->WasKeyJustPressed(' '))
				{
					g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, .2f, 0.f, 1.f, false);
					ExitState(GameState::LOBBY);
					EnterState(GameState::PLAYING);
				}
				else
				{
					// find the player's controller and check if start is pressed
					XboxController const& controller = g_theInput->GetController(m_playersList[i]->m_controllerIndex);
					if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
					{
						g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, .2f, 0.f, 1.f, false);
						ExitState(GameState::LOBBY);
						EnterState(GameState::PLAYING);
					}
				}
			}
		}
	}
	else if (GetNumberOfJoiningPlayer() == 0 && m_currentState == GameState::LOBBY)
	{
		// if there is no joining player at the lobby, quit to attract mode
		m_transformFromAttractToGameTimer->Stop();
		ExitState(GameState::LOBBY);
		EnterState(GameState::ATTRACT);
		return;
	}
}

void Game::DetectFirstJoiningPlayerInAttractMode()
{
	if (GetNumberOfJoiningPlayer() == 0)
	{
		// space bar to exit the attract mode to the lobby and become the first player with keyboard
		if (g_theInput->WasKeyJustPressed(' '))
		{
			g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, .2f, 0.f, 1.f, false);
			m_transformFromAttractToGameTimer->Start();
			if (!RegisterPlayer(1, -1)) // means this controller is controlling a player and pressed start
			{
				ExitState(GameState::LOBBY);
				EnterState(GameState::ATTRACT);
				return;
			}
		}
		// space bar to exit the attract mode to the lobby and become the first player with controller
		for (int i = 0; i < NUM_XBOX_CONTROLLERS; ++i)
		{
			XboxController const& controller = g_theInput->GetController(i);
			if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
			{
				g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, .2f, 0.f, 1.f, false);
				m_transformFromAttractToGameTimer->Start();

				if (!RegisterPlayer(1, i)) // means this controller is controlling a player and pressed start
				{
					ExitState(GameState::LOBBY);
					EnterState(GameState::ATTRACT);
					return;
				}

				break; // if multiple controller pressing together, we only guarantee only the first one create new player
			}
		}
	}
}

// if find this controller has already register a player, return false
// successfully register a new player will return true
bool Game::RegisterPlayer(int playerIndex, int controllerIndex)
{
	// first check if the controller index is controlling any player, if so, do not register new player and return bool
	for (int i = 0; i < (int)m_playersList.size(); ++i)
	{
		if (m_playersList[i])
		{
			if (m_playersList[i]->m_controllerIndex == controllerIndex)
			{
				return false;
			}
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// if the controller is not controlling any player, creat a new player and assign this controller index
	Player* newPlayer = new Player(Vec3(2.5f, 8.5, 0.5f), m_currentMap);
	newPlayer->m_playerIndex = playerIndex;
	newPlayer->m_controllerIndex = controllerIndex; // tell the player controller to find input from which controller

	// if there is a nullptr in list, fill in
	bool emptySpaceInList = false;
	for (int i = 0; i < (int)m_playersList.size(); ++i)
	{
		if (!m_playersList[i])
		{
			m_playersList[i] = newPlayer;
			emptySpaceInList = true;
			break;
		}
	}

	// if there is no empty space, push back the new player
	if (!emptySpaceInList)
	{
		m_playersList.push_back(newPlayer);
	}
	return true;
}

// find which player has chosen to quit, then remove it from player list
void Game::KickingOutPlayer(int controllerIndex)
{
	for (int i = 0; i < (int)m_playersList.size(); ++i)
	{
		if (m_playersList[i])
		{
			if (m_playersList[i]->m_controllerIndex == controllerIndex)
			{
				delete m_playersList[i];
				m_playersList[i] = nullptr;
			}
		}
	}
}

int Game::GetNumberOfJoiningPlayer() const
{
	int numPlayers = 0;
	for (int i = 0; i < (int)m_playersList.size(); ++i)
	{
		if (m_playersList[i])
		{
			++numPlayers;
		}
	}
	return numPlayers;
}

void Game::RenderPlayerIndexAndInstruction() const
{
	AABB2 bounds = g_theApp->m_devConsoleCamera.GetCameraBounds();
	float screenHeigt = bounds.m_maxs.y - bounds.m_mins.y;

	std::string player1 = "PLAYER 1";
	std::string player2 = "PLAYER 2";
	std::string controller = "Controller";
	std::string keyboard = "Mouse and Keyboard";
	std::string instruction = "Press START to start game\nPress BACK to leave game";

	std::string useSpear = "Press down Number 3 to equip the new Spear auto-locking weapon. \nPut enemies in the aiming circle and wait to shoot";
	std::string listenMode = "Press down \'Q\' to enter listen mode. \nNearby enemies will show on the screen when you enter listen mode";

	std::vector<Vertex_PCU> textVerts;
	if (GetNumberOfJoiningPlayer() == 1)
	{
		// get the player index and controller info
		std::string playerIndexText;
		bool controlledByKeyboard = false;
		for (int i = 0; i < (int)m_playersList.size(); ++i)
		{
			if (m_playersList[i] != nullptr)
			{
				m_playersList[i]->m_playerIndex == 1 ? playerIndexText = player1 : playerIndexText = player2;
				m_playersList[i]->m_controllerIndex == -1 ? controlledByKeyboard = true : controlledByKeyboard = false;
			}
		}

		m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, playerIndexText, bounds, m_textConfig.m_titleFontMultipler * screenHeigt, Vec2(0.5f, 0.5f), m_ringColor);
		if (controlledByKeyboard)
		{
			m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, keyboard, bounds, m_textConfig.m_middleFontMultipler * screenHeigt, Vec2(0.5f, 0.45f), m_ringColor, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);
			m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, useSpear, bounds, m_textConfig.m_middleFontMultipler * screenHeigt, Vec2(0.5f, 0.72f), Rgba8::DEEP_ORANGE, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);
			m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, listenMode, bounds, m_textConfig.m_middleFontMultipler * screenHeigt, Vec2(0.5f, 0.87f), Rgba8::DEEP_ORANGE, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);
		}
		else
		{
			m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, controller, bounds, m_textConfig.m_middleFontMultipler * screenHeigt, Vec2(0.5f, 0.3f), m_ringColor, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);
		}
		m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, instruction, bounds, m_textConfig.m_smallFontMultiplier * screenHeigt, Vec2(0.5f, 0.1f), m_ringColor, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);
	}
	else if (GetNumberOfJoiningPlayer() == 2)
	{
		// get the player index
		std::string player1IndexText;
		std::string player2IndexText;
		bool controlledByKeyboard1 = false;
		bool controlledByKeyboard2 = false;
		int numPlayer = 1;

		// know which is player 1 and 2, which player is controlled by keyboard or controller
		for (int i = 0; i < (int)m_playersList.size(); ++i)
		{
			if (m_playersList[i] != nullptr)
			{
				if (numPlayer == 1)
				{
					m_playersList[i]->m_playerIndex == 1 ? player1IndexText = player1 : player2IndexText = player2;
					m_playersList[i]->m_controllerIndex == -1 ? controlledByKeyboard1 = true : controlledByKeyboard1 = false;
					++numPlayer;
				}
				else
				{
					m_playersList[i]->m_playerIndex == 1 ? player2IndexText = player1 : player2IndexText = player2;
					m_playersList[i]->m_controllerIndex == -1 ? controlledByKeyboard2 = true : controlledByKeyboard2 = false;
					break;
				}
			}
		}

		// player 1
		m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, player1IndexText, bounds, m_textConfig.m_titleFontMultipler * screenHeigt, Vec2(0.5f, 0.75f), m_ringColor);
		if (controlledByKeyboard1)
		{
			m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, keyboard, bounds, m_textConfig.m_middleFontMultipler * screenHeigt, Vec2(0.5f, 0.7f), m_ringColor, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);
		}
		else
		{
			m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, controller, bounds, m_textConfig.m_middleFontMultipler * screenHeigt, Vec2(0.5f, 0.7f), m_ringColor, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);
		}
		m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, instruction, bounds, m_textConfig.m_smallFontMultiplier * screenHeigt, Vec2(0.5f, 0.62f), m_ringColor, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);
		// player 2
		m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, player2IndexText, bounds, m_textConfig.m_titleFontMultipler * screenHeigt, Vec2(0.5f, 0.25f), m_ringColor);
		if (controlledByKeyboard2)
		{
			m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, keyboard, bounds, m_textConfig.m_middleFontMultipler * screenHeigt, Vec2(0.5f, 0.2f), m_ringColor, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);
		}
		else
		{
			m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, controller, bounds, m_textConfig.m_middleFontMultipler * screenHeigt, Vec2(0.5f, 0.2f), m_ringColor, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);
		}
		m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, instruction, bounds, m_textConfig.m_smallFontMultiplier * screenHeigt, Vec2(0.5f, 0.12f), m_ringColor, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);

	}

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->BindTexture(&m_textConfig.m_font->GetTexture());
	g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());

}


void Game::RenderTitleAndStartInstruction() const
{
	AABB2 bounds = g_theApp->m_devConsoleCamera.GetCameraBounds();
	float screenHeigt = bounds.m_maxs.y - bounds.m_mins.y;

	std::string title = "Doomenstein";
	std::string instruction = "Press SPACE to join with mouse and keyboard\nPress START to join with controller\nPress ESCAPE or BACK to exit";

	std::vector<Vertex_PCU> textVerts;

	m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, title, bounds, m_textConfig.m_titleFontMultipler * screenHeigt, Vec2(0.5f, 0.2f), m_ringColor);
	m_textConfig.m_font->AddVertsForTextInBox2D(textVerts, instruction, bounds, m_textConfig.m_smallFontMultiplier * screenHeigt, Vec2(0.5f, 0.06f), m_ringColor, m_textConfig.m_lineSpacingMultiplier, m_textConfig.m_cellAspect);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->BindTexture(&m_textConfig.m_font->GetTexture());
	g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Game::UpdatePlaying()
{
	// if (m_playersList.size() >= 1 && g_theApp->m_debugMode)
	// {
	// 	g_debugPosition = m_playersList[0]->m_position; // send the info to the debug render system to show on screen
	// }

	if (m_UIisDisplayed)
	{
		UpdateUI();
	}

	if (m_currentMap)
	{
		m_currentMap->Update();

		// control lighting settings
		ControlLightingSettings();
	}

	// update testing entities
	// if (!m_entities.empty())
	// {
	// 	for (int i = 0; i < m_entities.size(); ++i)
	// 	{
	// 		m_entities[i]->Update();
	// 	}
	// }
	// 

	if (GetDebugRenderVisibility())
	{
		UpdateDebugRenderMessages();
	}

	if (m_playerWins)
	{
		ExitState(GameState::PLAYING);
		EnterState(GameState::VICTORY);
	}
}

void Game::UpdateVictory()
{
	if (g_theInput->WasKeyJustPressed(' ') || g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		m_playerWins = false;
		ExitState(GameState::VICTORY);
		EnterState(GameState::LOBBY);
	}
}

void Game::UpdateDebugRenderMessages()
{
	Vec2  positionAlignment = Vec2(0.f, 1.f);
	Vec2  orientationAlignment = Vec2(0.f, 0.95f);
	Vec2  timeFpsAlignment = Vec2(1.f, 1.f);
	Vec2  lightingAlignment = Vec2(1.f, 0.95f);
	Vec2  controlModeAlignment = Vec2(0.5f, 0.f);
	float fontSize = 24.f;

	std::string gameGoal = Stringf("Gaol: Use listen mode to kill all enemies and find the exit");
	DebugAddScreenText(gameGoal, Vec2(0.f, 0.f), fontSize, positionAlignment, -1.f);

	// for the top of the line we are always drawing the position and orientation
	// player position
	// std::string playerPosition = Stringf("PlayerPosition: %.2f, %.2f, %.2f", g_debugPosition.x, g_debugPosition.y, g_debugPosition.z);
	// DebugAddScreenText(playerPosition, Vec2(0.f, 0.f), fontSize, positionAlignment, -1.f);
	// std::string playerOrientation = Stringf("PlayerOrientation: %.2f, %.2f, %.2f"
	// 	, m_player->m_orientation.m_yawDegrees
	// 	, m_player->m_orientation.m_pitchDegrees
	// 	, m_player->m_orientation.m_rollDegrees);
	// DebugAddScreenText(playerOrientation, Vec2(0.f, 0.f), fontSize, orientationAlignment, -1.f);

	// time, FPS, time scale
	float		time;
	std::string FPS;
	float		timeScale = g_theGameClock->GetTimeScale();
	if (g_theGameClock)
	{
		time = g_theGameClock->GetTotalSeconds();
		if (g_theGameClock->IsPaused())
		{
			FPS = "Paused";
			timeScale = 0.f;
		}
		else
		{
			FPS = Stringf("%.2f", (1.f / g_theGameClock->GetDeltaSeconds()));
		}
	}
	else
	{
		time = 0.f;
		timeScale = 0.f;
	}
	std::string gameTime_FPS_TimeScale = Stringf("[Game Clock] Time: %.2f FPS: %s TimeScale: %.2f", time, FPS.c_str(), timeScale);
	// DebugAddScreenText(gameTime_FPS_TimeScale, Vec2(0.f, 0.f), fontSize, timeFpsAlignment, -1.f);

	// show current lighting settings
	std::string lightingSettings = Stringf("Sun Direction X: %.2f [F2 / F3 to change]\nSun Direction Y: %.2f [F4 / F5 to change]\nSun Intensity: %.2f [F6 / F7 to change]\nAmbient Intensity: %.2f [F8 / F9 to change]",
		m_currentMap->m_mapLightingSettings->SunDirection.x, 
		m_currentMap->m_mapLightingSettings->SunDirection.y, 
		m_currentMap->m_mapLightingSettings->SunIntensity,
		m_currentMap->m_mapLightingSettings->AmbientIntensity);
	// DebugAddScreenText(lightingSettings, Vec2(0.f, 0.f), fontSize, lightingAlignment, -1.f);

	// show which actor our input is controlling
	//if (m_playersList[0])
	//{
	//	if (m_playersList[0]->m_freeFlyCamera)
	//	{
	//		std::string controlCamera = "Free Fly Camera Mode";
	//		DebugAddScreenText(controlCamera, Vec2(0.f, 0.5f), fontSize, controlModeAlignment, -1.f);
	//	}
	//	else
	//	{
	//		Actor* controllingActor = m_playersList[0]->m_map->GetActorByUID(m_playersList[0]->m_actorUID);
	//		if (controllingActor)
	//		{
	//			if (!controllingActor->m_isDead)
	//			{
	//				std::string FirstPersonCamera = Stringf("First Person Camera Mode: %s view", controllingActor->m_actorDef->m_actorName.c_str());
	//				DebugAddScreenText(FirstPersonCamera, Vec2(0.f, 0.5f), fontSize, controlModeAlignment, -1.f, Rgba8::DEEP_ORANGE, Rgba8::DEEP_ORANGE);
	//			}
	//			else
	//			{
	//				std::string playerIsDead = "Player is dead";
	//				DebugAddScreenText(playerIsDead, Vec2(0.f, 0.5f), fontSize, controlModeAlignment, -1.f, Rgba8::RED, Rgba8::RED);
	//			}
	//		}
	//	}
	//}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Game::Render()
{	
	switch (m_currentState)
	{
	case GameState::ATTRACT:
		{
			RenderAttract();
		}break;
	case GameState::LOBBY:
	{
		RenderLobby();
	}break;
	case GameState::PLAYING:
		{
			RenderPlaying();
		}break;
	case GameState::VICTORY:
	{
		RenderVictory();
	}break;
	}	
}

void Game::RenderAttract()
{
	g_theRenderer->BeginCamera(m_sharedScreenCamera);

	// draw breathing ring
	AABB2 bounds = m_sharedScreenCamera.GetCameraBounds();
	Vec2 center = Vec2(bounds.m_maxs.x * 0.5f, bounds.m_maxs.y * 0.5f);

	DebugDrawRing(center, bounds.m_maxs.x * 0.1f, m_ringThickness, m_ringColor);

	RenderTitleAndStartInstruction();

	g_theRenderer->EndCamera(m_sharedScreenCamera);
}

void Game::RenderLobby()
{
	RenderPlayerIndexAndInstruction();
}

void Game::RenderPlaying()
{
	RenderWorldInPlayerCameras(); // player's view

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_sharedScreenCamera);
	RenderUIElements();
	if (GetDebugRenderVisibility() && !m_playersList[0]->m_freeFlyCamera)
	{
		// render the messages on the screen
		DebugRenderScreen(m_sharedScreenCamera);
	}

	// show tint black screen when game is paused
	if (g_theGameClock->IsPaused())
	{
		std::vector<Vertex_PCU> backgroundVerts;
		AddVertsForAABB2D(backgroundVerts, m_sharedScreenCamera.GetCameraBounds(), Rgba8::BLACK_TRANSPARENT);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->DrawVertexArray((int)backgroundVerts.size(), backgroundVerts.data());

		// stop current game music
		g_theAudio->SetSoundPlaybackSpeed(m_gameMusicPlaybackID, 0.f);
	}
	else
	{
		// reset current game music play speed
		g_theAudio->SetSoundPlaybackSpeed(m_gameMusicPlaybackID, 1.f);
	}

	g_theRenderer->EndCamera(m_sharedScreenCamera);
}

void Game::RenderVictory()
{
	g_theRenderer->BeginCamera(m_sharedScreenCamera);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	std::vector<Vertex_PCU> verts;
	AABB2 testingTexture = m_sharedScreenCamera.GetCameraBounds();
	AddVertsForAABB2D(verts, testingTexture, Rgba8::WHITE);
	g_theRenderer->BindTexture(g_theApp->g_textures[VICTORY_MENU]);
	g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());

	g_theRenderer->EndCamera(m_sharedScreenCamera);
}

// use player camera to render entities in the world
void Game::RenderWorldInPlayerCameras()
{
	g_theRenderer->ClearScreen(Rgba8::GRAY_Dark);//the background color setting of the window
	for (int i = 0; i < (int)m_playersList.size(); ++i)
	{
		if (m_playersList[i])
		{
			Camera& playerWorldCamera = m_playersList[i]->m_worldCamera;
			g_theRenderer->BeginCamera(playerWorldCamera);
			//// render game world for props
			//if (!m_entities.empty())
			//{
			//	for (int i = 0; i < m_entities.size(); ++i)
			//	{
			//		m_entities[i]->Render();
			//	}
			//}

			// render current map
			if (m_currentMap)
			{
				m_currentRenderPlayerController = m_playersList[i];
				m_currentMap->Render();
			}
			// debug render
			DebugRenderWorld(playerWorldCamera);
			g_theRenderer->EndCamera(playerWorldCamera);
			
			//----------------------------------------------------------------------------------------------------------------------------------------------------
			m_playersList[i]->RenderScreenCamera();
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Game::SpawnProps()
{
	g_theColorChangingClock = new Clock(*g_theGameClock);
	g_theColorChangingClock->SetTimeScale(0.5f);

	// // add a cube prop
	// Prop* bigCubeProp = new Prop();
	// bigCubeProp->m_name = "First Big Cube";
	// AABB3 bigCubeBounds = AABB3(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f));
	// AABB3 smallCube = bigCubeBounds * 0.25f;
	// bigCubeProp->m_position = Vec3(2.f, 2.f, 0.f);
	// 
	// AddVertsForAABB3D(bigCubeProp->m_vertexPCUTBNs, bigCubeProp->m_indexArray, bigCubeBounds);
	// m_entities.push_back(bigCubeProp);
	// 
	// // create vertex buffer and index buffer
	// bigCubeProp->m_vertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(bigCubeProp->m_vertexPCUTBNs.size()), sizeof(Vertex_PCUTBN));
	// bigCubeProp->m_indexBuffer = g_theRenderer->CreateIndexBuffer((size_t)(bigCubeProp->m_indexArray.size()));
	// 
	// size_t vertexSize = sizeof(Vertex_PCUTBN);
	// size_t vertexArrayDataSize = (bigCubeProp->m_vertexPCUTBNs.size()) * vertexSize;
	// g_theRenderer->CopyCPUToGPU(bigCubeProp->m_vertexPCUTBNs.data(), vertexArrayDataSize, bigCubeProp->m_vertexBuffer);
	// 
	// size_t indexSize = sizeof(int);
	// size_t indexArrayDataSize = bigCubeProp->m_indexArray.size() * indexSize;
	// g_theRenderer->CopyCPUToGPU(bigCubeProp->m_indexArray.data(), indexArrayDataSize, bigCubeProp->m_indexBuffer);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// // add a grid for the world
	// Prop* grid = new Prop();
	// grid->m_name = "Grid";
	// float spacingXY = 5.f;
	// int numOfXY = (int)(100.f / spacingXY) + 1;
	// int numOfGrid = 100 + 1;
	// float dimensionRed = 0.05f;
	// float dimensionGreen = 0.04f;
	// float dimensionGray = 0.02f;
	// // gray grid
	// for (int i = 0; i < numOfGrid; ++i)
	// {
	// 	AABB3 pipe(Vec3(-50.f + (float)i - (dimensionGray * 0.5f), -50.f, -(dimensionGray * 0.5f)),
	// 		Vec3( -50.f + (float)i + (dimensionGray * 0.5f), 50.f, (dimensionGray * 0.5f)));
	// 	AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	// }
	// for (int i = 0; i < numOfGrid; ++i)
	// {
	// 	AABB3 pipe(Vec3(-50.f, -50.f + (float)i - (dimensionGray * 0.5f), -(dimensionGray * 0.5f)),
	// 		Vec3(50.f, -50.f + (float)i + (dimensionGray * 0.5f), (dimensionGray * 0.5f)));
	// 	AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	// }
	// // GREEN lane
	// for (int i = 0; i < numOfXY; ++i)
	// {
	// 	if ( i == (numOfXY / 2))
	// 	{
	// 		AABB3 pipe(Vec3(-50.f + (float)i * spacingXY - (dimensionGreen * 1.2f), -50.f, -(dimensionGreen * 2.f)),
	// 			Vec3(-50.f + (float)i * spacingXY + (dimensionGreen * 2.f), 50.f, (dimensionGreen * 2.f)));
	// 		AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
	// 	}
	// 	else 
	// 	{
	// 		AABB3 pipe(Vec3(-50.f + (float)i * spacingXY - (dimensionGreen * 0.5f), -50.f, -(dimensionGreen * 0.5f)),
	// 			Vec3(-50.f + (float)i * spacingXY + (dimensionGreen * 0.5f), 50.f, (dimensionGreen * 0.5f)));
	// 		AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
	// 	}
	// }
	// // RED lane
	// for (int i = 0; i < numOfXY; ++i)
	// {
	// 	if (i == (numOfXY / 2))
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXY - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXY + (dimensionRed * 2.f), (dimensionRed * 2.f)));
	// 		AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// 	else
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXY - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXY + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
	// 		AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// }
	// m_entities.push_back(grid);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// // add world basis
	// Mat44 worldBasis;
	// DebugAddWorldBasis(worldBasis, -1.f);
	// // add world text
	// Mat44 XTransform;
	// XTransform.Append(Mat44::CreateZRotationDegrees(90.f));
	// XTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, .15f)));
	// DebugAddWorldText(" X - Forward", XTransform, 0.3f, -1.f, Vec2(0.f, 0.0f), Rgba8::RED, Rgba8::RED);
	// Mat44 YTransform;
	// YTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, .15f)));
	// DebugAddWorldText("Y - Left ", YTransform, 0.3f, -1.f, Vec2(1.f, 0.0f), Rgba8::GREEN, Rgba8::GREEN);
	// 
	// Mat44 ZTransform;
	// ZTransform.Append(Mat44::CreateXRotationDegrees(-90.f));
	// ZTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, -.15f)));
	// DebugAddWorldText(" Z - Up", ZTransform, 0.3f, -1.f, Vec2(0.f, 1.0f), Rgba8::BLUE, Rgba8::BLUE);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Billboard Testing
	// Vec3 billboardPosition(15.f, 0.f, 4.f);
	// DebugAddWorldBillboardText(m_currentMap->m_mapDefinition.m_name, billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::WORLD_UP_CAMERA_FACING, Rgba8::CYAN);
	// billboardPosition += Vec3(0.f, 0.f, 1.5f);
	// DebugAddWorldBillboardText("Test Using Different Shaders", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::FULL_CAMERA_FACING, Rgba8::CYAN);
	// billboardPosition += Vec3(0.f, 0.f, 1.5f);
	// DebugAddWorldBillboardText("The Big Brother is watching YOU", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::WORLD_UP_CAMERA_OPOSSING, Rgba8::CYAN);
	// billboardPosition += Vec3(0.f, 0.f, 1.5f);
	// DebugAddWorldBillboardText("The Big Brother is watching YOU", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::FULL_CAMERA_OPPOSING, Rgba8::CYAN);

}
//----------------------------------------------------------------------------------------------------------------------------------------------------
void Game::EnterState(GameState state)
{
	m_transformFromAttractToGameTimer->Stop();

	// start new music
	switch (state)
	{
	case GameState::ATTRACT:
	{
		EnterAttract();
	}break;
	case GameState::LOBBY:
	{
		EnterLobby();
	}break;
	case GameState::PLAYING:
	{
		EnterPlaying();
	}break;	
	case GameState::VICTORY:
	{
		EnterVictory();
	}break;
	}
}

void Game::EnterAttract()
{
	m_mainMenuPlaybackID = g_theAudio->StartSound(g_theApp->g_soundEffectsID[MAINMENUMUSIC], true, 0.02f, 0.f, 1.f, false);

 	m_currentState = GameState::ATTRACT;
	g_theInput->SetDesiredCursorMode(false, false);
	m_transformFromAttractToGameTimer->Stop();
}

void Game::EnterLobby()
{
	m_currentState = GameState::LOBBY;
	g_theInput->SetDesiredCursorMode(false, false);
}

void Game::EnterPlaying()
{
	m_gameMusicPlaybackID = g_theAudio->StartSound(g_theApp->g_soundEffectsID[GAMEMUSIC], true, 0.008f, 0.f, 1.f, false);
	 
	m_currentState = GameState::PLAYING;
	g_theInput->SetDesiredCursorMode(true, true);

	int numPlayer = GetNumberOfJoiningPlayer();
	g_theAudio->SetNumListeners(numPlayer);

	int listenerIndex = 0;
	for (int i = 0; i < (int)m_playersList.size(); ++i)
	{
		if (m_playersList[i] != nullptr)
		{
			m_playersList[i]->m_listenerIndex = listenerIndex;
			++listenerIndex;
			m_playersList[i]->Startup();
		}
	}	

	TileTypeDefinition::InitializeTileDefs();
	GenerateAllMaps();

	m_currentMap->Startup();
	// SpawnProps(); // used for visual debugging showing the name of the map
}

void Game::EnterVictory()
{
	m_victoryMusicPlaybackID = g_theAudio->StartSound(g_theApp->g_soundEffectsID[PLAYER_VICTORY], true, .2f, 0.f, 1.f, false);

	m_currentState = GameState::VICTORY;
	g_theInput->SetDesiredCursorMode(false, false);
}

void Game::ExitAttract()
{
	m_transformFromAttractToGameTimer->Stop();

}

void Game::ExitLobby()
{
	g_theAudio->SetSoundPlaybackSpeed(m_mainMenuPlaybackID, 0.f);

}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Game::ExitState(GameState state)
{
	switch (state)
	{
	case GameState::ATTRACT:
	{
		ExitAttract();
	}break;
	case GameState::LOBBY:
	{
		ExitLobby();
	}break;
	case GameState::PLAYING:
	{
		ExitPlaying();
	}break;
	case GameState::VICTORY:
	{
		ExitVictory();
	}break;
	}
}

void Game::ExitPlaying()
{
	g_theAudio->SetSoundPlaybackSpeed(m_gameMusicPlaybackID, 0.f);

	for (int i = 0; i < (int)m_allMaps.size(); ++i)
	{
		delete m_allMaps[i];
	}
	m_allMaps.clear();

	for (int i = 0; i < (int)m_entities.size(); ++i)
	{
		delete m_entities[i];
	}
	m_entities.clear();

	for (int i = 0; i < (int)m_playersList.size(); ++i)
	{
		delete m_playersList[i];
		m_playersList[i] = nullptr;
		m_playersList.clear();
	}
}

void Game::ExitVictory()
{
	g_theAudio->SetSoundPlaybackSpeed(m_victoryMusicPlaybackID, 0.f);

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
	MapDefinition::ClearDefinitions();

	for (int i = 0; i < (int)m_allMaps.size(); ++i)
	{
		delete m_allMaps[i];
	}
	m_allMaps.clear();

	// for (int i = 0; i < (int)m_entities.size(); ++i)
	// {
	// 	delete m_entities[i];
	// }
	// m_entities.clear();

	for (int i = 0; i < (int)m_playersList.size(); ++i)
	{
		delete m_playersList[i];
		m_playersList[i] = nullptr;
		m_playersList.clear();
	}

	delete g_theGameClock;

	MapDefinition::ClearDefinitions();
}

void Game::GenerateAllMaps()
{
	// make sure all maps are cleared up before generating because player might restart
	m_allMaps.reserve(3);
	 
	// get map definition from xml
	MapDefinition::InitializeMapDefs();
	for (int i = 0; i < MapDefinition::s_mapDefs.size(); ++i)
	{
		// put into the map list by names
		if (MapDefinition::s_mapDefs[i].m_name == "GoldMap")
		{
			Map* map1 = new Map(MapDefinition::s_mapDefs[i]);
			m_allMaps.push_back(map1);
		}		
		else if (MapDefinition::s_mapDefs[i].m_name == "TestMap")
		{
			Map* map1 = new Map(MapDefinition::s_mapDefs[i]);
			m_allMaps.push_back(map1);
		}		
		else if (MapDefinition::s_mapDefs[i].m_name == "MPMap")
		{
			Map* map1 = new Map(MapDefinition::s_mapDefs[i]);
			m_allMaps.push_back(map1);
		}
	}
	m_currentMap = m_allMaps[2];
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

void Game::InitializeEnergyBar()
{
	m_energyBar = new EnergyBar(this, Vec2(0.f, 0.f));
}

void Game::DrawIconShield(Vec2 centerPos) const
{
	Vec3 worldPos = Vec3(centerPos.x, centerPos.y, 0.f);
	Vertex_PCU verts[9] = {};
	Vec3 pointA = Vec3(-1.2f, 1.f, 0.f);
	Vec3 pointB = Vec3(-1.f, -1.f, 0.f);
	Vec3 pointC = Vec3(0.f, -2.f, 0.f);
	Vec3 pointD = Vec3(1.f, -1.f, 0.f);
	Vec3 pointE = Vec3(1.2f, 1.f, 0.f);

	verts[0].m_position = pointA + worldPos;
	verts[1].m_position = pointB + worldPos;
	verts[2].m_position = pointD + worldPos;

	verts[3].m_position = pointA + worldPos;
	verts[4].m_position = pointD + worldPos;
	verts[5].m_position = pointE + worldPos;

	verts[6].m_position = pointB + worldPos;
	verts[7].m_position = pointC + worldPos;
	verts[8].m_position = pointD + worldPos;

	g_theRenderer->DrawVertexArray(9, &verts[0]);
}

void Game::DrawIconVelocity(Vec2 centerPos) const
{
	Vec3 worldPos = Vec3(centerPos.x, centerPos.y, 0.f);
	Vertex_PCU verts[9] = {};
	Vec3 pointA = Vec3(0.f, 1.8f, 0.f);
	Vec3 pointB = Vec3(0.f, -1.8f, 0.f);
	Vec3 pointC = Vec3(2.f, -1.8f, 0.f);
	Vec3 pointD = Vec3(2.f, 1.8f, 0.f);

	verts[0].m_position = pointA + worldPos;
	verts[1].m_position = pointB + worldPos;
	verts[2].m_position = pointC + worldPos;

	verts[3].m_position = pointA + worldPos;
	verts[4].m_position = pointC + worldPos;
	verts[5].m_position = pointD + worldPos;

	verts[6].m_position = Vec3(-2.f, 0.f, 0.f) + worldPos;
	verts[7].m_position = Vec3(0.f, .3f, 0.f) + worldPos;
	verts[8].m_position = Vec3(0.f, -.3f, 0.f) + worldPos;

	g_theRenderer->DrawVertexArray(9, &verts[0]);
}

void Game::DrawIconWeapon(Vec2 centerPos) const
{
	Vec3 worldPos = Vec3(centerPos.x, centerPos.y, 0.f);
	Vertex_PCU verts[18] = {};
	Vec3 pointA = Vec3(-2.f, .6f, 0.f);
	Vec3 pointB = Vec3(-2.f, -.6f, 0.f);
	Vec3 pointC = Vec3(-.5f, -.6f, 0.f);
	Vec3 pointD = Vec3(-.5f, .6f, 0.f);
	Vec3 pointE = Vec3(-.5f, .3f, 0.f);
	Vec3 pointF = Vec3(-.5f, -.3f, 0.f);
	Vec3 pointG = Vec3(1.f, -.3f, 0.f);
	Vec3 pointH = Vec3(1.f, .3f, 0.f);
	Vec3 pointI = Vec3(1.f, .4f, 0.f);
	Vec3 pointJ = Vec3(1.f, -.4f, 0.f);
	Vec3 pointK = Vec3(1.8f, -0.4f, 0.f);
	Vec3 pointL = Vec3(1.8f, 0.4f, 0.f);

	verts[0].m_position = pointA + worldPos;
	verts[1].m_position = pointB + worldPos;
	verts[2].m_position = pointC + worldPos;

	verts[3].m_position = pointA + worldPos;
	verts[4].m_position = pointC + worldPos;
	verts[5].m_position = pointD + worldPos;

	verts[6].m_position = pointE + worldPos;
	verts[7].m_position = pointF + worldPos;
	verts[8].m_position = pointG + worldPos;

	verts[9].m_position = pointE + worldPos;
	verts[10].m_position = pointG + worldPos;
	verts[11].m_position = pointH + worldPos;

	verts[12].m_position = pointI + worldPos;
	verts[13].m_position = pointJ + worldPos;
	verts[14].m_position = pointK + worldPos;

	verts[15].m_position = pointI + worldPos;
	verts[16].m_position = pointK + worldPos;
	verts[17].m_position = pointL + worldPos;

	g_theRenderer->DrawVertexArray(18, &verts[0]);
}

Vec2 Game::GetRandomPosInWorld(Vec2 worldSize)
{
	float posX = g_rng->RollRandomFloatInRange(0, worldSize.x);
	float posY = g_rng->RollRandomFloatInRange(0, worldSize.y);
	return Vec2(posX, posY);
}

//// add camera shake when player is dead
//void Game::UpdateCameras(float deltaSeconds)
//{
//	// normally the camera follows the player
//	//if (!m_playerShip->m_isDead && !m_introTimer > 0.f)
//	//{
//	//	AABB2 followingCamera(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
//	//	followingCamera.SetDimensions(Vec2(100.f, 50.f));
//	//	followingCamera.SetCenter(m_playerShip->m_position);
//	//	m_worldCamera.SetOrthoView(Vec2(followingCamera.m_mins.x, followingCamera.m_mins.y), Vec2(followingCamera.m_maxs.x, followingCamera.m_maxs.y));
//
//	//	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));
//	//}
//
//	// the intro start with a zoom out
//	m_introTimer -= deltaSeconds;
//	if (m_introTimer > 0.f)
//	{
//		m_worldCamera.ZoomInOutCamera(deltaSeconds, AABB2(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y)), TIME_INTRO_ZOOMOUT);
//	}
//
//	// if the camera is going to shake
//	if (!m_cameraShakeIsDisplayed)
//	{
//		// if it is the final blow to the player, because the delta second is slow down, this is the compensation
//		if (m_playerLivesNum < 1)
//		{
//			deltaSeconds *= 8.f;
//		}
//
//		// reset camera back to player before shake
//		AABB2 followingCamera(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
//		followingCamera.SetDimensions(Vec2(200.f, 100.f));
//		followingCamera.SetCenter(Vec2(WORLD_SIZE_X * 0.5f, WORLD_SIZE_Y * 0.5f));
//		m_worldCamera.SetOrthoView(Vec2(followingCamera.m_mins.x, followingCamera.m_mins.y), Vec2(followingCamera.m_maxs.x, followingCamera.m_maxs.y));
//		//m_worldCamera.SetOrthoView(Vec2(0.f, 0.f) + m_playerShip->m_position, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
//
//		//m_worldCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
//		m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));
//
//		// calculate the shake amount for the world camera
//		m_screenShakeAmount -= CAMERA_SHAKE_REDUCTION * CAMERA_SHAKE_REDUCTION * deltaSeconds;
//		m_screenShakeAmount = GetClamped(m_screenShakeAmount, 0.f, CAMERA_SHAKE_AMOUNT_PLAYERDEAD);
//
//		float shakeX = g_rng->RollRandomFloatInRange(-m_screenShakeAmount, m_screenShakeAmount);
//		float shakeY = g_rng->RollRandomFloatInRange(-m_screenShakeAmount, m_screenShakeAmount);
//		m_worldCamera.Translate2D(Vec2(shakeX, shakeY));
//
//		// if the camera's shaking ends, camera shake has been displayed
//		if (m_screenShakeAmount <= 0.f)
//		{
//			m_cameraShakeIsDisplayed = true;
//		}
//		return;
//	}
//
//}

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

void Game::UpdateUI()
{
	// CheckPlayerLives(deltaSeconds);
	// UpdateEnergyBar(deltaSeconds);
	// UpdateEnergySelectionRing(deltaSeconds);
}

void Game::UpdateEnergyBar(float deltaSeconds)
{
	m_energyBar->Update(deltaSeconds);
}

void Game::SpawnEnergySelectionRing()
{
	// m_energySelectionRing = new EnergySelectionRing(this, Vec2(0.f, 0.f));
}

void Game::DeleteEnergySelectionRing()
{
	if (m_energySelectionRing != nullptr)
	{
		delete m_energySelectionRing;
		m_energySelectionRing = nullptr;
	}
}

void Game::UpdateEnergySelectionRing(float deltaSeconds)
{
	if (m_energySelectionRing != nullptr)
	{
		m_energySelectionRing->Update(deltaSeconds);
	}
}

void Game::CheckPlayerLives()
{	// 
	// if (m_playerShip->m_isDead && m_playerLivesNum < 1)
	// {
	// 	g_theGameClock->SetTimeScale(0.1f);
	// 
	// 	m_returnToStartTimer += deltaSeconds * 10.f;// make the delta second's value without the effect of the slow mode
	// 	if (m_returnToStartTimer >= TIME_RETURN_TO_ATTRACTMODE)
	// 	{
	// 		g_theGameClock->SetTimeScale(1.f);			
	// 		m_gameIsOver = true;
	// 	}
	// }
}

void Game::RespawnPlayer()
{
	// only allow to respawn playerShip when player is dead
	//if (m_playerShip->m_isDead && m_playerLivesNum > 0)
	//{
	//	// update the playerShip UI after use a life
	//	m_UI_lives[m_playerLivesNum - 1]->m_isEnabled = false;
	//	m_playerLivesNum -= 1;
	//
	//	// reinitialize the player
	//	Vec2 PlayerStart(WORLD_SIZE_X * .5f, WORLD_SIZE_Y * .5f);// Declare & define the pos of player start
	//	m_playerShip = new PlayerShip(this, PlayerStart);// Spawn the player ship
	//}
}

void Game::ControlLightingSettings()
{
	float debugMsgFontSize = 10.f;

	// sun direction controls
	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
	{
		--(m_currentMap->m_mapLightingSettings->SunDirection.x);

		std::string orientation = Stringf("Sun Direction X = %.2f", m_currentMap->m_mapLightingSettings->SunDirection.x);
		DebugAddMessage(orientation, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
	{
		++(m_currentMap->m_mapLightingSettings->SunDirection.x);

		std::string orientation = Stringf("Sun Direction X = %.2f", m_currentMap->m_mapLightingSettings->SunDirection.x);
		DebugAddMessage(orientation, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F4))
	{
		--(m_currentMap->m_mapLightingSettings->SunDirection.y);

		std::string orientation = Stringf("Sun Direction Y = %.2f", m_currentMap->m_mapLightingSettings->SunDirection.y);
		DebugAddMessage(orientation, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F5))
	{
		++(m_currentMap->m_mapLightingSettings->SunDirection.y);

		std::string orientation = Stringf("Sun Direction Y = %.2f", m_currentMap->m_mapLightingSettings->SunDirection.y);
		DebugAddMessage(orientation, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}

	// sun intensity controls
	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		m_currentMap->m_mapLightingSettings->SunIntensity -= 0.05f;

		std::string intensity = Stringf("Sun Intensity = %.2f", m_currentMap->m_mapLightingSettings->SunIntensity);
		DebugAddMessage(intensity, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
		m_currentMap->m_mapLightingSettings->SunIntensity += 0.05f;

		std::string intensity = Stringf("Sun Intensity = %.2f", m_currentMap->m_mapLightingSettings->SunIntensity);
		DebugAddMessage(intensity, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}

	// ambient light intensity
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		m_currentMap->m_mapLightingSettings->AmbientIntensity -= 0.05f;

		std::string intensity = Stringf("Ambient Intensity = %.2f", m_currentMap->m_mapLightingSettings->AmbientIntensity);
		DebugAddMessage(intensity, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F9))
	{
		m_currentMap->m_mapLightingSettings->AmbientIntensity += 0.05f;

		std::string intensity = Stringf("Ambient Intensity = %.2f", m_currentMap->m_mapLightingSettings->AmbientIntensity);
		DebugAddMessage(intensity, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}

	// The z-component of the direction should stay fixed at -1.
	m_currentMap->m_mapLightingSettings->SunDirection.z = -1.f;
	m_currentMap->m_mapLightingSettings->SunIntensity = GetClamped(m_currentMap->m_mapLightingSettings->SunIntensity, 0.f, 1.f);
	m_currentMap->m_mapLightingSettings->AmbientIntensity = GetClamped(m_currentMap->m_mapLightingSettings->AmbientIntensity, 0.f, 1.f);
}

void Game::RenderUIElements() const
{ 
	// dx11 testing
	// Vertex_PCU vertices[] = {
	// Vertex_PCU(Vec3(-0.5f, -0.5f, 0.f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
	// Vertex_PCU(Vec3(0.f, 0.5f, 0.f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
	// Vertex_PCU(Vec3(0.5f, -0.5f, 0.f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f))
	// };
	// g_theRenderer->DrawVertexArray(3, vertices);
}

bool Game::CheckUIEnabled(UI* const UI) const
{
	UNUSED(UI);
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



