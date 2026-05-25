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
#include "Engine/Model/Material.hpp"
#include "Game/ShiningTriangle.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include <algorithm>
#include <iostream>
#include <math.h>
 
 using namespace std;

extern App* g_theApp;// global variable must be define in the cpp
extern Clock* g_theGameClock;

Game* g_theGame = nullptr;
Renderer* g_theRenderer = nullptr;
InputSystem* g_theInput = nullptr;
AudioSystem* g_theAudio = nullptr;
Window* g_theWindow = nullptr;
BitmapFont* g_consoleFont = nullptr;
DevConsole* g_theDevConsole = nullptr;

Texture* g_textures[NUM_TEXTURES];
Material* g_materials[NUM_MATERIALS];

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
	g_theInput->SetDesiredCursorMode(false, false);

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_windowTitle = "Procedural Animation Blending";
	windowConfig.m_aspectRatio = m_windowAspectRatio;
	g_theWindow = new Window(windowConfig);

	RenderConfig renderConfig;
	renderConfig.m_window = g_theWindow;
#ifdef USE_EMISSIVE_MATERIAL
	renderConfig.m_emissiveEnabled = true; // default is false
#endif
	g_theRenderer = new Renderer(renderConfig);

	// set up development console
	DevConsoleConfig consoleConfig;
	consoleConfig.m_font = g_consoleFont;
	consoleConfig.m_renderer = g_theRenderer;
	consoleConfig.m_camera = &m_attractModeCamera;
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
	g_theDevConsole->m_config.m_renderer = g_theRenderer;
	g_theDevConsole->m_config.m_font = g_consoleFont;

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_theRenderer;
	debugRenderConfig.m_font = g_consoleFont;
	DebugRenderSystemStartup(debugRenderConfig);

	LoadAudioAssets();
	LoadTextureAssets();
	LoadMaterialAssets();

	// m_openningBgm = g_theAudio->StartSound(g_soundEffectsID[ATTRACTMODE_BGM], false, 1.0f, 0.f, 1.f, false);
	//set the 200x100 orthographic (2D) world and drawing coordinate system, 
	m_attractModeCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	// m_attractModeCamera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	g_theDevConsole->AddInstruction("Type help for a list of commands");
	g_theDevConsole->AddInstruction("Controls", DevConsole::INFO_MINOR);
	g_theDevConsole->AddInstruction("Mouse  - Aim");
	g_theDevConsole->AddInstruction("W / A  - Move");
	g_theDevConsole->AddInstruction("S / D  - Strafe");
	g_theDevConsole->AddInstruction("Q / E  - Roll");
	g_theDevConsole->AddInstruction("Z / C  - Elevate");
	g_theDevConsole->AddInstruction("Shift  - Sprint");
	g_theDevConsole->AddInstruction("H	    - Set Camera to Origin");
	g_theDevConsole->AddInstruction("1      - Spawn Line");
	g_theDevConsole->AddInstruction("2      - Spawn Point");
	g_theDevConsole->AddInstruction("3      - Spawn Wireframe Sphere");
	g_theDevConsole->AddInstruction("4      - Spawn Basis");
	g_theDevConsole->AddInstruction("5      - Spawn Billboard Text");
	g_theDevConsole->AddInstruction("6      - Spawn Wireframe");
	g_theDevConsole->AddInstruction("7      - Add Message");
	g_theDevConsole->AddInstruction("~      - Open Dev Console");
	g_theDevConsole->AddInstruction("Escape - Exit Game");
	g_theDevConsole->AddInstruction("Space  - Start Game");

	// set up event system subscription
	SubscribeEventCallbackFunction("quit", App::Event_Quit);
	// show helper commands at the start when the console is turned on
	FireEvent("ControlInstructions");

	m_transformFromAttractToGameTimer = new Timer(0.5f);
	InitializeAttractMode();

	g_theGame->Startup();
}

void App::LoadAudioAssets()
{
	g_soundEffectsID[ATTRACTMODE_BGM] = g_theAudio->CreateOrGetSound("Data/InterstellarMainTheme.mp3");
	g_soundEffectsID[BUTTON_CLICK] = g_theAudio->CreateOrGetSound("Data/Click.mp3");
	return;
}

void App::LoadTextureAssets()
{
	g_textures[RETICLE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Reticle.png");
}

void App::LoadMaterialAssets()
{
	g_materials[MaterialType::CRAWLER] = new Material();
	g_materials[MaterialType::CRAWLER]->Load("Data/Materials/M_Crawler.xml");
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
	g_theRenderer->Shutdown();
	g_theWindow->ShutDown();
	g_theInput->Shutdown();
	g_theDevConsole->Shutdown();
	g_theEventSystem->Shutdown();
	DebugRenderSystemShutDown();

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
	float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();

	if (m_transformFromAttractToGameTimer->HasPeroidElapsed())
	{
		m_attractModeIsOn = false;
		g_theInput->SetCursorMode(true, true);
	}

	if (m_attractModeIsOn)
	{
		UpdateAttractMode(deltaSeconds);
	}

	// otherwise the change of button could happen at anytime when key is pressed and the order is not guaranteed
	CheckKeyAndButtonStates();
	UpdateGameMode();

	// RestartGame();
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

	// space bar to enter exit the attract mode
	if (g_theInput->WasKeyJustPressed(' ') || controller.WasButtonJustPressed(XBOX_BUTTON_START))
	{
		g_theAudio->StartSound(g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
		m_transformFromAttractToGameTimer->Start();
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
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		if (g_theGame)
		{
			// g_theAudio->StartSound(g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
			// g_theAudio->StopSound(m_openningBgm);
			// the game will delete all its children first
			g_theGame->Shutdown();
			g_theGame = nullptr;
			Startup();
			m_attractModeIsOn = true;
// 			g_theInput->m_inAttractMode = true;
			m_transformFromAttractToGameTimer->Stop();
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// ESC and pause logic
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
		g_theInput->SetCursorMode(false, false);
		m_transformFromAttractToGameTimer->Stop();
		g_theGame->m_gameIsOver = true;
	}
	if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK) && g_theGameClock->IsPaused())
	{
		g_theGameClock->TogglePause();
		m_attractModeIsOn = true;
//		g_theInput->m_inAttractMode = true;
		m_transformFromAttractToGameTimer->Stop();
		g_theGame->m_gameIsOver = true;
	}
}

void App::UpdateGameMode()
{
	if (!m_attractModeIsOn)
	{
		g_theGame->Update();
	}
	else
	{
		// change ring thickness according to time
		if (m_transformFromAttractToGameTimer->GetElapsedTime() > 0.f)
		{
			m_ringThickness *= (1.f + m_transformFromAttractToGameTimer->GetElapsedTime() * 0.1f);
		}
		else
		{
			m_ringThickness = WORLD_SIZE_X * 0.02f * fabsf(sinf(2.f * (float)Clock::GetSystemClock().GetTotalSeconds()));
		}
	}
}

void App::RestartGame()
{
	if (g_theGame->m_gameIsOver)
	{
		g_theGame->Shutdown();
		delete g_theGame;

		m_attractModeIsOn = true;
		g_theInput->SetCursorMode(false, false);
		m_transformFromAttractToGameTimer->Stop();
		m_debugMode = false;
		InitializeAttractMode();
		g_theGame = new Game();
		g_theGame->Startup();
	}
}

void App::Render()
{
	g_theRenderer->ClearScreen(Rgba8(255, 232, 189, 255));//the background color setting of the window

	g_theRenderer->BeginCamera(m_attractModeCamera);
	if (m_attractModeIsOn)
	{
		RenderAttractMode();
	}
	g_theRenderer->EndCamera(m_attractModeCamera);

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

}

void App::UpdateAttractMode(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

void App::RenderAttractMode() const
{	
	// draw breathing ring
	Vec2 center = Vec2(WORLD_SIZE_X * 0.5f, WORLD_SIZE_Y * 0.5f);
	Rgba8 ringColor = Rgba8(Rgba8(255, 160, 133, 175));
	DebugDrawRing(center, WORLD_SIZE_X * 0.1f, m_ringThickness, ringColor);
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

//-----------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
void App::RunFrame()
{
	while (!m_isQuitting)
	{
		Clock::GetSystemClock().TickSystemClock();

		double timeAtStart = GetCurrentTimeSeconds();
		double timeAtEnd = GetCurrentTimeSeconds();
		BeginFrame();
		double timeElapsed = timeAtEnd - timeAtStart;
		m_maxBeginFrameDuration = max(m_maxBeginFrameDuration, timeElapsed);


		timeAtStart = GetCurrentTimeSeconds();
		Update();
		timeAtEnd = GetCurrentTimeSeconds();
		timeElapsed = timeAtEnd - timeAtStart;
		m_maxUpdateDuration = max(m_maxUpdateDuration, timeElapsed);

		timeAtStart = GetCurrentTimeSeconds();
		Render();
		timeAtEnd = GetCurrentTimeSeconds();
		timeElapsed = timeAtEnd - timeAtStart;
		m_maxRenderDuration = max(m_maxRenderDuration, timeElapsed);

		// std::string UMsg = Stringf(" begin frame duration: %.8f\n", m_maxBeginFrameDuration);
		// DebuggerPrintf(UMsg.c_str());
		// 
		// UMsg = Stringf(" total update duration: %.8f\n", m_maxUpdateDuration);
		// DebuggerPrintf(UMsg.c_str());
		// 
		// UMsg = Stringf(" total render duration: %.8f\n", m_maxRenderDuration);
		// DebuggerPrintf(UMsg.c_str());

		EndFrame();
	}
}

