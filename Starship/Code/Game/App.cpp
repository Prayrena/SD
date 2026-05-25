#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/core/Time.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/PlayerShipIcon.hpp"
#include "Game/ShiningTriangle.hpp"
#include <iostream>
#include <math.h>

extern App*		g_theApp;// global variable must be define in the cpp
extern Clock*	g_theGameClock;

Game*			g_theGame = nullptr;
Renderer*		g_theRenderer = nullptr;
InputSystem*	g_theInput = nullptr;
AudioSystem*	g_theAudio = nullptr;
Window*			g_theWindow = nullptr;
BitmapFont*		g_consoleFont = nullptr;
DevConsole*		g_theDevConsole = nullptr;

STATIC bool App::Event_Quit(EventArgs& args)
{
	UNUSED(args);
	g_theApp->HandleQuitRequested();
	return true;
}

App::App()
{

}

App :: ~App()
{

}

void App :: Startup ()
{   
	
	// Create engine subsystems and game
	EventSystemConfig eventConfig;
	g_theEventSystem = new EventSystem(eventConfig);

	InputConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);


	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_windowTitle = "SD1-A4: Starship Gold";
	windowConfig.m_aspectRatio = 2.f;
	g_theWindow = new Window(windowConfig);

	RenderConfig renderConfig;
	renderConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer( renderConfig );


	// set up development console
	DevConsoleConfig consoleConfig;
	consoleConfig.m_font = g_consoleFont;
	consoleConfig.m_renderer = g_theRenderer;
	consoleConfig.m_camera = &m_attractModeCamera;
	g_theDevConsole = new DevConsole(consoleConfig);

	AudioConfig audioConfig;
	g_theAudio = new AudioSystem(audioConfig);
	 
	g_theGame = new Game();

	//unsigned char testingChar = 'X';// unsigned char is a number from 0 to 255
	g_theEventSystem->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	// call devConsole before the input system because of subscription sequence is prior
	g_theDevConsole->Startup();
	g_theInput->Startup();
	g_theAudio->Startup();
	g_theGame->Startup();

	// the bit front needed to be created after the renderer start up function
	g_consoleFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont.png");
	g_theDevConsole->m_config.m_renderer = g_theRenderer;
	g_theDevConsole->m_config.m_font = g_consoleFont;

	LoadAudioAssets();
	m_openningBgm = g_theAudio->StartSound(g_soundEffectsID[ATTRACTMODE_BGM], false, 0.8f, 0.f, 0.6f, false);
	InitializeAttractMode();
	m_attractModeCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(200.f, 100.f));

	// set up event system subscription
	SubscribeEventCallbackFunction("quit", App::Event_Quit);

	// write in game instruction and help command to the g_theBlackBoard
// g_theDevConsole->AddInstruction("Game Start: Space Bar / Controller Menu Button");
// g_theDevConsole->AddInstruction("Quit: Key ESC / Controller View Button");
// g_theDevConsole->AddInstruction("Move: WASD / Controller Left Joystick");
// g_theDevConsole->AddInstruction("Fire Bullet: SpaceBar / Controller Button A");
	g_theDevConsole->AddInstruction("Slow Mode: Key T / Controller Right Shoulder Button");
	g_theDevConsole->AddInstruction("Pause Game: Key P / Controller View Button");
	// g_theDevConsole->AddInstruction("Open Energy Distribution Menu: Key K / Controller Left Shoulder Button");
	// g_theDevConsole->AddInstruction("Select Energy Distribution: Key IJL / Controller Right Joystick");
	// g_theDevConsole->AddInstruction("Player Respawn: Key N / Controller Menu Button");
	g_theDevConsole->AddInstruction("Clear Current Level Enemy: Key C");
	g_theDevConsole->AddInstruction("Spawn Random Asteroid: Key X");
	g_theDevConsole->AddInstruction("Draw Debug Line and Disc: Key F1");
	g_theDevConsole->AddInstruction("Game Restart: Key F8");
	g_theDevConsole->AddInstruction("Toggle DevConsole Mode: Key F11");
	g_theDevConsole->AddInstruction("Close Menu/Quit: Key ESC");
	g_theDevConsole->AddInstruction("Input \"help\" for registered commands");
	g_theDevConsole->AddInstruction("quit");

	// show helper commands at the start when the console is turned on
	FireEvent("ControlInstructions");
}

bool App :: IsQuitting()const
{
	return m_isQuitting;
}

//-----------------------------------------------------------------------------------------------
void App :: Shutdown()
{
	// shut down game and engine subsystem
	g_theGame->Shutdown();
	g_theAudio->Shutdown();
	g_theRenderer->Shutdown();
	g_theWindow->ShutDown();
	g_theInput->Shutdown();
	g_theDevConsole->Shutdown();
	g_theEventSystem->Shutdown();

	delete g_theAudio;
	g_theAudio = nullptr;

	delete g_theDevConsole;
	g_theDevConsole = nullptr;

	delete g_theRenderer;
	g_theRenderer = nullptr;

	delete g_theWindow;
	g_theWindow = nullptr;

	delete g_theInput;
	g_theInput = nullptr;

	delete g_theEventSystem;
	g_theEventSystem = nullptr;

	// Delete UI entities
	for (int i = 0; i < UI_POSITION_ATTRACTMODE_ICON_NUM; ++i)
	{
		delete m_playerShipIcon[i];
		m_playerShipIcon[i] = nullptr;
	}
	delete m_shiningTriangle;
	m_shiningTriangle = nullptr;
}

void App :: BeginFrame()
{
	g_theEventSystem->BeginFrame();
	g_theInput->BeginFrame();
	g_theDevConsole->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theAudio->BeginFrame();
}

bool App::HandleQuitRequested()
{
	m_isQuitting = true;
	return true;
}

void App::LoadAudioAssets()
{
	g_soundEffectsID[SHOOTINGBULLETS] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PlayerShoot.wav");

	g_soundEffectsID[ATTRACTMODE_BGM] = g_theAudio->CreateOrGetSound("Data/AttractMode.wav");
	g_soundEffectsID[GAMEMODE_BGM] = g_theAudio->CreateOrGetSound("Data/Bgm/GameModeBgm.wav");
	g_soundEffectsID[NEW_WAVE] = g_theAudio->CreateOrGetSound("Data/SoundEffects/NewWaveComing.mp3");

	g_soundEffectsID[SPEED_BOOST] = g_theAudio->CreateOrGetSound("Data/EngineBoost.wav");
	g_soundEffectsID[FIRERATE_BOOST] = g_theAudio->CreateOrGetSound("Data/FireRateBoost.wav");
	g_soundEffectsID[ENERGY_RINGTURNEDON] = g_theAudio->CreateOrGetSound("Data/SoundEffects/ShieldTurnedOn.mp3");
	g_soundEffectsID[ENERGY_DEPLETED] = g_theAudio->CreateOrGetSound("Data/SoundEffects/EngineBoost.wav");

	g_soundEffectsID[ENEMY_DESTROYED] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PointsGained.mp3");
	g_soundEffectsID[ENEMY_WASHIT] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PlayerHit.wav");

	g_soundEffectsID[PLAYER_DEAD] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PlayerDeath.mp3");
	g_soundEffectsID[PLAYER_RESPAWN] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PlayerRespawn.mp3");
	g_soundEffectsID[GAMEOVER_VICTORY] = g_theAudio->CreateOrGetSound("Data/SoundEffects/Victory.mp3");
	g_soundEffectsID[GAMEOVER_DEFEATE] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PlayerIsSpotted.mp3");
}

void App::ManageAudio()
{
	XboxController const& controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed('P') || controller.WasButtonJustPressed(XBOX_BUTTON_BACK) || g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{

		// in game mode and game was playing
		if (!m_attractModeIsOn && g_theGameClock->IsPaused())
		{
			g_theAudio->SetSoundPlaybackSpeed(m_gameModeBgm, 0.f);
		}

		// in game mode and it was paused
		if (!m_attractModeIsOn && !g_theGameClock->IsPaused())
		{
			g_theAudio->SetSoundPlaybackSpeed(m_gameModeBgm, 1.f);
		}
	}
}

/// <Update per frame functions>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void App::Update()
{	
	float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();

	if (m_permitToStartTimer)
	{
		m_transitionToGameTimer += deltaSeconds;
	}

	// takes a few seconds before entering the game mode
	if (m_transitionToGameTimer >= TIME_TRANSITION_GAME)
	{
		m_attractModeIsOn = false;
	}

	if (m_attractModeIsOn)
	{
		UpdateAttractMode(deltaSeconds);
	}

	// otherwise the change of button could happen at anytime when key is pressed and the order is not guaranteed
	CheckKeyAndButtonStates();
	UpdateGameMode();

	RestartGame();
}

void App::CheckKeyAndButtonStates()
{
	XboxController const& controller = g_theInput->GetController(0);

	// T for slow mode
	if (g_theInput->IsKeyDown('T'))
	{
		g_theGameClock->SetTimeScale(0.1f);
	}
	if (g_theInput->WasKeyJustReleased('T'))
	{
		g_theGameClock->SetTimeScale(1.f);
	}

	// open up the single frame mode
	// discuss four situations about the pause mode and single frame mode
	if (g_theInput->WasKeyJustPressed('O'))
	{
		g_theGameClock->StepSingleFrame();
	}
	if (g_theInput->WasKeyJustPressed('P'))
	{
		g_theGameClock->TogglePause();
	}


	// space bar to exit the attract mode
	if (g_theInput->WasKeyJustPressed(' ') || controller.WasButtonJustPressed(XBOX_BUTTON_START))
	{
		m_permitToStartTimer = true;
		g_theAudio->SetSoundPlaybackSpeed(m_openningBgm, 0.f);
		if (m_attractModeIsOn)
		{
			m_gameModeBgm = g_theAudio->StartSound(g_soundEffectsID[GAMEMODE_BGM], true, 0.5f);
		}
	}

	// F1 for entering debug mode
	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
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
//----------------------------------------------------------------------------------------------------------------------------------------------------
// 	ESC and pause logic
	// Quit Application
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && m_attractModeIsOn)
	{
		m_isQuitting = true;
	}
	// pause the game when in game
	else if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && !m_attractModeIsOn && !g_theGameClock->IsPaused())
	{
		g_theGameClock->Pause();
		return;
	}

	if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK) && m_attractModeIsOn)
	{
		m_isQuitting = true;
	}
	else if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK) && !m_attractModeIsOn && !g_theGameClock->IsPaused())
	{
		g_theGameClock->Pause();
		return;
	}

	// in game when pause and press ESC, quit to attract mode
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && g_theGameClock->IsPaused())
	{
		g_theGameClock->TogglePause();
		m_attractModeIsOn = true;
		m_permitToStartTimer = false;
		m_transitionToGameTimer = 0.f;
		g_theGame->m_gameIsOver = true;
	}
	if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK) && g_theGameClock->IsPaused())
	{
		g_theGameClock->TogglePause();
		m_attractModeIsOn = true;
		m_permitToStartTimer = false;
		m_transitionToGameTimer = 0.f;
		g_theGame->m_gameIsOver = true;
	}
//----------------------------------------------------------------------------------------------------------------------------------------------------
// 
	// Restart Game
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		g_theGame->m_gameIsOver = true;
		g_theAudio->SetSoundPlaybackSpeed(m_gameModeBgm, 0.f);
		RestartGame();
	}
}

void App::UpdateGameMode()
{
	if (!m_attractModeIsOn)
	{
		g_theGame->Update();
	}
}

void App::RestartGame()
{
	if (g_theGame->m_gameIsOver)
	{
		g_theGame->Shutdown();
		delete g_theGame;

		m_attractModeIsOn = true;
		m_permitToStartTimer = false;
		m_transitionToGameTimer = 0.f;

		for (int i = 0; i < UI_POSITION_ATTRACTMODE_ICON_NUM; ++i)
		{
			delete m_playerShipIcon[i];
		}

		delete m_shiningTriangle;

		InitializeAttractMode();
		g_theGame = new Game();
		g_theGame->Startup();
		m_openningBgm = g_theAudio->StartSound(g_soundEffectsID[ATTRACTMODE_BGM], false, 0.8f, 0.f, 0.6f, false);
	}
}

// because this function is const, it will use not any "set XXXXXX" functions
void App::Render() const
{
	// clear screen
	g_theRenderer->ClearScreen(Rgba8(WORLD_COLOR_R, WORLD_COLOR_G, WORLD_COLOR_B, 255));//the background color setting of the window

	// attract mode render
	g_theRenderer->BeginCamera(m_attractModeCamera);
	if (m_attractModeIsOn)
	{
		RenderAttractMode();
	}
	g_theRenderer->EndCamera(m_attractModeCamera);

	// game render
	if (!m_attractModeIsOn)
	{
		g_theGame->Render();
	}

	// dev console render
	g_theRenderer->BeginCamera(*g_theDevConsole->m_config.m_camera);
	AABB2 screenBounds = g_theDevConsole->m_config.m_camera->GetCameraBounds();
	g_theDevConsole->Render(screenBounds);
	g_theRenderer->EndCamera(*g_theDevConsole->m_config.m_camera);
}

/// <Attract Mode>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void App::InitializeAttractMode()
{	
	Vec2 iconAPos = Vec2(UI_POSITION_ATTRACTMODE_ICONA_X, UI_POSITION_ATTRACTMODE_ICONA_Y);
	m_playerShipIcon[0] = new PlayerShipIcon(g_theGame, iconAPos);
	m_playerShipIcon[0]->m_uniformScale = UI_ATTRACTMODE_ICONA_SCALE;
	m_playerShipIcon[0]->InitializeLocalVerts();
	m_playerShipIcon[0]->m_orientationDegrees = UI_ORIENTATION_ATTRACTMODE_ICONA;
	m_playerShipIcon[0]->m_originalPosition = iconAPos;
	m_playerShipIcon[0]->m_floatingCycle = 6.f;
	
	Vec2 iconBPos = Vec2(UI_POSITION_ATTRACTMODE_ICONB_X, UI_POSITION_ATTRACTMODE_ICONB_Y);
	m_playerShipIcon[1] = new PlayerShipIcon(g_theGame, iconBPos);
	m_playerShipIcon[1]->m_uniformScale = UI_ATTRACTMODE_ICONB_SCALE;
	m_playerShipIcon[1]->InitializeLocalVerts();
	m_playerShipIcon[1]->m_orientationDegrees = UI_ORIENTATION_ATTRACTMODE_ICONB;
	m_playerShipIcon[1]->m_originalPosition = iconBPos;
	m_playerShipIcon[1]->m_floatingCycle = 3.f;

	m_shiningTriangle = new ShiningTriangle(  g_theGame, Vec2(UI_POSITION_ATTRACTMODE_PLAYBUTTON_X, UI_POSITION_ATTRACTMODE_PLAYBUTTON_Y)  );
	m_shiningTriangle->m_uniformScale = UI_ATTRACTMODE_PLAYBUTTON_SCALE;
}

void App::UpdateAttractMode(float deltaSeconds)
{
	for (int i = 0; i < UI_POSITION_ATTRACTMODE_ICON_NUM; ++i)
	{
		m_playerShipIcon[i]->Update(deltaSeconds);
	}

	m_shiningTriangle->Update(deltaSeconds);
}

void App::RenderAttractMode() const
{
	// UI items
	for (int i = 0; i < UI_POSITION_ATTRACTMODE_ICON_NUM; ++i)
	{
		UIElement* const UIPtr = m_playerShipIcon[i];
		if (i == 2)
		{
			UIPtr->m_orientationDegrees = 180.f;
		}
		UIPtr->Render();
	}

	m_shiningTriangle->Render();

	// titles
	std::vector<Vertex_PCU> textVerts;
	AABB2 screenBounds = m_attractModeCamera.GetCameraBounds();
	static int maxGlyphsToDraw = 24;
	static float timer = 0.f;
	timer += 0.2f;
	maxGlyphsToDraw += (int)timer;
	if (timer >= 1.f)
	{
		timer = 0.f;
	}
	maxGlyphsToDraw %= 24;
	g_consoleFont->AddVertsForTextInBox2D(textVerts, "Starship\nGold", screenBounds * 0.9f, 0.09f * screenBounds.m_maxs.y, Vec2(1.f, 0.75f), Rgba8::CYAN, 0.5f, 0.6f, SHRINK_TO_FIT, maxGlyphsToDraw);
	g_consoleFont->AddVertsForTextInBox2D(textVerts, "Programmed by:\nChengxiang Li", screenBounds, 0.03f * screenBounds.m_maxs.y, Vec2(1.f, 0.f), Rgba8::CYAN);
	g_consoleFont->AddVertsForTextInBox2D(textVerts, "Designed by:\nProfessor Eiserloh", screenBounds, 0.03f * screenBounds.m_maxs.y, Vec2(0.f, 0.f), Rgba8::CYAN);
	g_theRenderer->BindTexture(&g_consoleFont->GetTexture());
	g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
	g_theRenderer->BindTexture(nullptr);
}

void App::EndFrame()
{
	g_theInput->EndFrame();
	g_theDevConsole->EndFrame();
	g_theEventSystem->EndFrame();
	g_theRenderer->EndFrame();
	g_theAudio->EndFrame();
	g_theWindow->EndFrame();
}

//-----------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
void App::RunFrame()
{
	while (!m_isQuitting)
	{
		Clock::GetSystemClock().TickSystemClock();
		// float deltaSeconds = 1.f / 60.f; // assuming 60 fps, keep the deltaSeconds a static number
		// know each deltaSeconds is calculate by the time difference
		// double timeThisFrame = GetCurrentTimeSeconds();
		// float deltaSeconds = static_cast<float>(timeThisFrame - m_timeLastFrame);
		// m_timeLastFrame = timeThisFrame;

		BeginFrame();
		Update();
		Render();
		EndFrame();
	}
}

