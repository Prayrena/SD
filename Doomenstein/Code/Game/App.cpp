#include "Engine/core/EngineCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/core/Time.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/core/DevConsole.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/ShiningTriangle.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/Weapon.hpp"
#include "Game/Actor.hpp"
#include <iostream>
#include <math.h>
 
extern App* g_theApp;// global variable must be define in the cpp
extern Clock* g_theGameClock;

Game* g_theGame = nullptr;
Renderer* g_theRenderer = nullptr;
InputSystem* g_theInput = nullptr;
AudioSystem* g_theAudio = nullptr;
Window* g_theWindow = nullptr;
BitmapFont* g_consoleFont = nullptr;
DevConsole* g_theDevConsole = nullptr;

App::App()
{

}

App :: ~App()
{

}

void App :: Startup ()
{   
	LoadGameConfigXml(); // will tell which path to load different music
	SetGameConfigByLoadedXml();

	// Create engine subsystems and game
	EventSystemConfig eventConfig;
	g_theEventSystem = new EventSystem(eventConfig);

	InputConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);


	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_windowTitle = "Doomenstein";
	windowConfig.m_aspectRatio = m_windowAspectRatio;
	g_theWindow = new Window(windowConfig);

	RenderConfig renderConfig;
	renderConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer(renderConfig);

	// set up development console
	DevConsoleConfig consoleConfig;
	consoleConfig.m_camera = &m_devConsoleCamera;
	g_theDevConsole = new DevConsole(consoleConfig);

	AudioConfig audioConfig;
	g_theAudio = new AudioSystem(audioConfig);

	g_theGame = new Game();

	g_theEventSystem->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	// call devConsole before the input system because of subscription sequence is prior
	g_theDevConsole->Startup();
	g_theInput->Startup();
	g_theAudio->Startup();

	// the bit front needed to be created after the renderer start up function
	g_consoleFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont.png");
	g_theGame->m_textConfig.m_font = g_consoleFont;
	g_theDevConsole->m_config.m_renderer = g_theRenderer;
	g_theDevConsole->m_config.m_font = g_consoleFont;

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_theRenderer;
	debugRenderConfig.m_font = g_consoleFont;
	DebugRenderSystemStartup(debugRenderConfig);

	LoadAudioAssets();
	LoadTextureAssets();
	LoadAllShaders();

	//set the 200x100 orthographic (2D) world and drawing coordinate system, 
	m_devConsoleCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	// m_attractModeCamera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	g_theDevConsole->AddInstruction("Type help for a list of commands");
	g_theDevConsole->AddInstruction("Key Board Controls", DevConsole::INFO_MINOR);
	g_theDevConsole->AddInstruction("Mouse  - Aim");
	g_theDevConsole->AddInstruction("W / A  - Move");
	g_theDevConsole->AddInstruction("S / D  - Strafe");
	g_theDevConsole->AddInstruction("Shift  - Sprint");
	g_theDevConsole->AddInstruction("Left arrow  - Select previous weapon");
	g_theDevConsole->AddInstruction("Right arrow  - Select next weapon");
	g_theDevConsole->AddInstruction("R  - Listen Mode");
	g_theDevConsole->AddInstruction("1  - Select weapon 1");
	g_theDevConsole->AddInstruction("2  - Select weapon 2");
	g_theDevConsole->AddInstruction("3  - Select weapon 3");
	g_theDevConsole->AddInstruction("Mouse left button	- Fire current weapon while held");

	g_theDevConsole->AddInstruction("X box Controller Controls", DevConsole::INFO_MINOR);
	g_theDevConsole->AddInstruction("Left stick x-axis  - Move left or right, relative camera orientation");
	g_theDevConsole->AddInstruction("Left stick y-axis  - Move forward or back, relative camera orientation");
	g_theDevConsole->AddInstruction("Right Trigger	- Fire current weapon while held");
	g_theDevConsole->AddInstruction("A button  - Sprint");
	g_theDevConsole->AddInstruction("D-pad Up  - Select previous weapon");
	g_theDevConsole->AddInstruction("D-pad Down  - Select next weapon");
	g_theDevConsole->AddInstruction("X button  - Select weapon 1");
	g_theDevConsole->AddInstruction("Y button  - Select weapon 2");

	g_theDevConsole->AddInstruction("F  - Toggle free-fly / first-person camera mode");
	g_theDevConsole->AddInstruction("N  - Possess next actor in the map that can be possessed");
	g_theDevConsole->AddInstruction("P  - Pause the game");

	// g_theDevConsole->AddInstruction("~      - Open Dev Console");
	g_theDevConsole->AddInstruction("Escape - Exit Game");
	// g_theDevConsole->AddInstruction("Space  - Start Game");

	// set up event system subscription
	SubscribeEventCallbackFunction("quit", App::Event_Quit);
	// show helper commands at the start when the console is turned on
	FireEvent("ControlInstructions");

	InitializeDefinitions();
	g_theGame->Startup();
}

void App::LoadAudioAssets()
{
	g_soundEffectsID[MAINMENUMUSIC] = g_theAudio->CreateOrGetSound(m_mainMenuMusicLoadPath);
	g_soundEffectsID[GAMEMUSIC] = g_theAudio->CreateOrGetSound(m_gameMusicLoadPath);
	g_soundEffectsID[BUTTON_CLICK] = g_theAudio->CreateOrGetSound(m_buttonClickSoundLoadPath);
	g_soundEffectsID[PLAYER_VICTORY] = g_theAudio->CreateOrGetSound("Data/Audio/Victory.mp3");
}

void App::LoadTextureAssets()
{
	g_textures[TESTUV] = g_theRenderer->CreateOrGetTextureFromFile("Data/Textures/TestUV.png");
	g_textures[VICTORY_MENU] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/VictoryScreen.jpg");
}

void App::LoadGameConfigXml()
{
	XmlDocument GameConfigXml;
	char const* filePath = "Data/GameConfig.xml";
	XmlResult result = GameConfigXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("failed to load xml file"));

	XmlElement* rootElement = GameConfigXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "rootElement is nullPtr");

	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);

}

void App::InitializeDefinitions()
{
	// because actor def need to know the weapon, and the weapon def needs to know the projectile actor
	// so we load projectile actor def first
	// then weapon def secondly
	// lastly is the actors def
	// notice that because projectile is also an actor, we use a function to load XML from different path
	char const* actorDefFilePath = "Data/Definitions/ActorDefinitions.xml";
	char const* projectileActorDefFilePath = "Data/Definitions/ProjectileActorDefinitions.xml";

	ActorDefinition::InitializeActorDefs(projectileActorDefFilePath);
	WeaponDefinition::InitializeWeaponDefs();
	ActorDefinition::InitializeActorDefs(actorDefFilePath);
}

void App::SetGameConfigByLoadedXml()
{
	m_musicVolume = g_gameConfigBlackboard.GetValue("musicVolume", 1.f);
	m_windowAspectRatio = g_gameConfigBlackboard.GetValue("windowAspect", 1.f);
	m_mainMenuMusicLoadPath = g_gameConfigBlackboard.GetValue("mainMenuMusic", "Did not designated");
	m_gameMusicLoadPath = g_gameConfigBlackboard.GetValue("gameMusic", "Did not designated");
	m_buttonClickSoundLoadPath = g_gameConfigBlackboard.GetValue("buttonClickSound", "Did not designated");
}

void App::LoadAllShaders()
{
	g_shaders[DEFAULT] = g_theRenderer->GetLoadedShader("Default");
	g_shaders[DIFFUSE] = g_theRenderer->CreateOrGetShader("Data/Shaders/Diffuse", VertexType::Vertex_PCUTBN);
}

bool App :: IsQuitting()const
{
	return m_isQuitting;
}

//-----------------------------------------------------------------------------------------------
//
void App :: Shutdown()
{
	// shut down game and engine subsystem
	g_theGame->Shutdown();
	g_theAudio->Shutdown();
	g_theWindow->ShutDown();
	g_theInput->Shutdown();
	g_theDevConsole->Shutdown();
	g_theEventSystem->Shutdown();
	DebugRenderSystemShutDown();
	g_theRenderer->Shutdown();

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

}

void App :: BeginFrame()
{
	g_theEventSystem->BeginFrame();
	g_theInput->BeginFrame();
	g_theDevConsole->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theAudio->BeginFrame();

	DebugRenderBeginFrame();
}

bool App::HandleQuitRequested()
{
	m_isQuitting = true;
	return true;
}

bool App::Event_Quit(EventArgs& args)
{
	UNUSED(args);
	g_theApp->HandleQuitRequested();
	return true;
}

/// <Update per frame functions>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void App::Update()
{	
	// otherwise the change of button could happen at anytime when key is pressed and the order is not guaranteed
	CheckKeyAndButtonStates();
	UpdateGameMode();
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
	if (g_theInput->WasKeyJustPressed('O'))
	{
		g_theGameClock->StepSingleFrame();
	}
	if (g_theInput->WasKeyJustPressed('P'))
	{
		g_theGameClock->TogglePause();
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

	// Restart Game
	//if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	//{
	//	if (g_theGame)
	//	{
	//		// g_theAudio->StartSound(g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
	//		// g_theAudio->StopSound(m_openningBgm);
	//		// the game will delete all its children first
	//		g_theGame->Shutdown();
	//		g_theGame = nullptr;
	//		Startup();
	//		m_attractModeIsOn = true;
	//		g_theInput->m_inAttractMode = true;
	//		m_transformFromAttractToGameTimer->Stop();
	//	}
	//}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// ESC and pause logic
	// Quit Application
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && g_theGame->m_currentState == GameState::ATTRACT)
	{
		m_isQuitting = true;
	}
	if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK) && g_theGame->m_currentState == GameState::ATTRACT)
	{
		m_isQuitting = true;
	}

	// pause the game when in game
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) 
		&& g_theGame->m_currentState == GameState::PLAYING 
		&& !g_theGameClock->IsPaused())
	{
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, .2f, 0.f, 1.f, false);

		g_theGameClock->Pause();
		return;
	}
	if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK)
		&& g_theGame->m_currentState == GameState::PLAYING
		&& !g_theGameClock->IsPaused())
	{
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, .2f, 0.f, 1.f, false);

		g_theGameClock->Pause();
		return;
	}

	// in game when pause and press ESC, quit to attract mode
	if (g_theGame->m_currentState == GameState::PLAYING)
	{
		if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && g_theGameClock->IsPaused())
		{
			g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, .2f, 0.f, 1.f, false);

			g_theGameClock->TogglePause();

			g_theGame->ExitState(GameState::PLAYING);
			g_theGame->EnterState(GameState::ATTRACT);
			g_theGame->m_gameIsOver = true;
		}
		if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK) && g_theGameClock->IsPaused())
		{
			g_theAudio->StartSound(g_theApp->g_soundEffectsID[BUTTON_CLICK], false, .2f, 0.f, 1.f, false);

			g_theGameClock->TogglePause();
			g_theGame->ExitState(GameState::PLAYING);
			g_theGame->EnterState(GameState::ATTRACT);
			g_theGame->m_gameIsOver = true;
		}
	}
}

void App::UpdateGameMode()
{
	g_theGame->Update();
}

void App::RestartGame()
{
	switch (g_theGame->m_currentState)
	{
	case GameState::ATTRACT:
	{
		return; // no need to restart
	}break;
	case GameState::PLAYING:
	{
		g_theGame->ExitState(GameState::PLAYING);

		g_theGame->Shutdown();
		delete g_theGame;
		m_debugMode = false;
		g_theGame = new Game();
		g_theGame->Startup();	
	}break;
	}
}

void App::Render()
{
	g_theRenderer->ClearScreen(Rgba8(255, 232, 189, 255));//the background color setting of the window

	g_theGame->Render();

	RenderDevConsole();
}


void App::EndFrame()
{
	g_theEventSystem->EndFrame();
 	g_theInput->EndFrame();
	g_theDevConsole->EndFrame();
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
	g_theAudio->EndFrame();

	DebugRenderEndFrame();
}

void App::RenderDevConsole()
{
	g_theRenderer->BeginCamera(*g_theDevConsole->m_config.m_camera);
	AABB2 screenBounds = g_theDevConsole->m_config.m_camera->GetCameraBounds();
	g_theDevConsole->Render(screenBounds);
	g_theRenderer->EndCamera(*g_theDevConsole->m_config.m_camera);
}

//-----------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
void App::RunFrame()
{
	while (!m_isQuitting)
	{
		Clock::GetSystemClock().TickSystemClock();

		BeginFrame();
		Update();
		Render();
		EndFrame();
	}
}

