#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Game/GetNearestPointsMode.hpp"
#include "Game/App.hpp"
#include "Game/ShiningTriangle.hpp"
#include "Game/GameMode.hpp"
#include <iostream>
#include <math.h>
 
Renderer* g_theRenderer = nullptr;
InputSystem* g_theInput = nullptr;
Window* g_theWindow = nullptr;
BitmapFont* g_consoleFont = nullptr;
DevConsole* g_theDevConsole = nullptr;
RandomNumberGenerator* g_rng = nullptr;
Clock* g_theGameClock = nullptr;

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
	windowConfig.m_windowTitle = "Math Visual Tests";
	windowConfig.m_aspectRatio = m_windowAspectRatio;
	g_theWindow = new Window(windowConfig);

	RenderConfig renderConfig;
	renderConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer(renderConfig);

	// set up development console
	DevConsoleConfig consoleConfig;
	consoleConfig.m_font = g_consoleFont;
	consoleConfig.m_renderer = g_theRenderer;
	consoleConfig.m_camera = &m_attractModeCamera;
	g_theDevConsole = new DevConsole(consoleConfig);

	g_theGameClock = new Clock();
	m_transformFromAttractToGameTimer = new Timer(0.1f); // 2.f

	g_theEventSystem->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	// call devConsole before the input system because of subscription sequence is prior
	g_theDevConsole->Startup();
	g_theInput->Startup();
	g_theInput->SetCursorMode(false, false);

	// the bit front needed to be created after the renderer start up function
	g_consoleFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont.png");
	g_theDevConsole->m_config.m_renderer = g_theRenderer;
	g_theDevConsole->m_config.m_font = g_consoleFont;

	// m_openningBgm = g_theAudio->StartSound(g_soundEffectsID[ATTRACTMODE_BGM], false, 1.0f, 0.f, 1.f, false);
	//set the 200x100 orthographic (2D) world and drawing coordinate system, 
	m_attractModeCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	// m_attractModeCamera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_theRenderer;
	debugRenderConfig.m_font = g_consoleFont;
	DebugRenderSystemStartup(debugRenderConfig);

	LoadTextureAssets();

	g_theDevConsole->AddInstruction("Pause Game: Key P / Controller View Button");
	g_theDevConsole->AddInstruction("Slow Mode: Key T / Controller Right Shoulder Button");
	g_theDevConsole->AddInstruction("Draw Debug Line and Disc: Key F1");
	g_theDevConsole->AddInstruction("Game Restart: Key F8");
	g_theDevConsole->AddInstruction("Toggle DevConsole Mode: Key F11");
	g_theDevConsole->AddInstruction("Close Menu/Quit: Key ESC");
	g_theDevConsole->AddInstruction("Input \"help\" for registered commands");
	g_theDevConsole->AddInstruction("quit");

	// set up event system subscription
	SubscribeEventCallbackFunction("quit", App::Event_Quit);
	// show helper commands at the start when the console is turned on
	FireEvent("ControlInstructions");

	// camera view might change according to the mechanics in the future
	//set the 200x100 orthographic (2D) world and drawing coordinate system, 
	m_attractModeCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));

	DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(0));
}

bool App :: IsQuitting()const
{
	return m_isQuitting;
}

//-----------------------------------------------------------------------------------------------
void App :: Shutdown()
{
	if (m_currentGameMode)
	{
		m_currentGameMode->Shutdown();
	}
	g_theRenderer->Shutdown();
	delete g_theRenderer;
}

void App :: BeginFrame()
{
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
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
	if (m_transformFromAttractToGameTimer->HasPeroidElapsed())
	{
		m_attractModeIsOn = false;
		DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(0));
		m_transformFromAttractToGameTimer->Stop();
	}

	if (m_attractModeIsOn)
	{
		UpdateAttractMode();
	}

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

	// Y for speed boost mode
	if (g_theInput->IsKeyDown('Y'))
	{
		g_theGameClock->SetTimeScale(4.f);
	}
	if (g_theInput->WasKeyJustReleased('Y'))
	{
		g_theGameClock->SetTimeScale(1.f);
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// F6 and F7 for switching different testing scene
	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		m_GameModeIndex -= 1;

		if (m_GameModeIndex == -1)
		{
			m_GameModeIndex = NUM_TESTINGMODES -1;

			DeleteCurrentTestMode_And_CreateNewTestMode( static_cast<TestingScene>(m_GameModeIndex) );
		}
		else
		{
			DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(m_GameModeIndex));
		}
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
		m_GameModeIndex += 1;

		if (m_GameModeIndex == NUM_TESTINGMODES)
		{
			m_GameModeIndex = 0;

			DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(m_GameModeIndex));
		}
		else
		{
			DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(m_GameModeIndex));
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
	if (m_attractModeIsOn)
	{
		if (g_theInput->WasKeyJustPressed(' ') || controller.WasButtonJustPressed(XBOX_BUTTON_START))
		{
			// g_theAudio->StartSound(g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
			m_permitToStartTimer = true;
			if (m_transformFromAttractToGameTimer->IsStopped())
			{
				m_transformFromAttractToGameTimer->Start();
			}
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

	// in game when pause and press ESC, quit to attract mode
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && g_theGameClock->IsPaused())
	{
		g_theGameClock->TogglePause();
		m_attractModeIsOn = true;
		m_transformFromAttractToGameTimer->Stop();
	}
	if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK) && g_theGameClock->IsPaused())
	{
		g_theGameClock->TogglePause();
		m_attractModeIsOn = true;
		m_transformFromAttractToGameTimer->Stop();
	}

	// Restart Game
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		if (m_currentGameMode)
		{
			// the game will delete all its children first
			m_currentGameMode->CreateRandomShapes();
		}
	}
}

void App::UpdateGameMode()
{
	if (!m_attractModeIsOn && m_currentGameMode)
	{
		float deltaSeconds = g_theGameClock->GetDeltaSeconds();
		m_currentGameMode->Update(deltaSeconds);
	}
}

void App::RestartGame()
{
	m_currentGameMode->Shutdown();
	m_currentGameMode->Startup();
	m_transformFromAttractToGameTimer->Stop();
	// m_attractModeIsOn = true;
	// m_permitToStartTimer = false;
	// m_transitionToGameTimer = 0.f;
	// m_isPaused = false;
	// m_isSlowMo = false;
	// m_debugMode = false;
	// InitializeAttractMode();
	// m_currentGameMode = new GetNearestPointsMode();
}

void App::RenderUIElements() const
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

void App::LoadTextureAssets()
{
	g_textures[TESTUV] = g_theRenderer->CreateOrGetTextureFromFile("Data/Textures/TestUV.png");
}

// because this function is const, it will use not any "set XXXXXX" functions
void App::Render() const
{
	g_theRenderer->ClearScreen(Rgba8(WORLD_COLOR_R, WORLD_COLOR_G, WORLD_COLOR_B, 255));//the background color setting of the window
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->BeginCamera(m_attractModeCamera);
	
	if (m_attractModeIsOn)
	{
		RenderAttractMode();
	}
	g_theRenderer->EndCamera(m_attractModeCamera);

	if (!m_attractModeIsOn && m_currentGameMode)
	{
		if (m_debugMode)
		{
			m_currentGameMode->RenderDebug();
		}

		m_currentGameMode->Render();
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

void App::DeleteCurrentTestMode_And_CreateNewTestMode(TestingScene type)
{
	if (m_currentGameMode)
	{
		m_currentGameMode->Shutdown();
		delete m_currentGameMode;
		m_currentGameMode = nullptr;
	}

	m_currentGameMode = GameMode::CreateNewGame(type);
	m_currentGameMode->Startup();

	GameModeConfig config;
	config.m_font = g_consoleFont;
	config.m_renderer = g_theRenderer;
	m_currentGameMode->m_gameModeConfig = config;
}

void App::UpdateAttractMode()
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

void App::RenderAttractMode() const
{	
	// draw breathing ring
	Vec2 center = Vec2(WORLD_SIZE_X * 0.5f, WORLD_SIZE_Y * 0.5f);
	Rgba8 ringColor = Rgba8(Rgba8(255, 160, 133, 175));
	DebugDrawRing(center, WORLD_SIZE_X * 0.1f, m_ringThickness, ringColor);
}

void App::EndFrame()
{
	g_theInput->EndFrame();
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
}

//-----------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
void App::RunFrame()
{
	while (!m_isQuitting)
	{
		Clock::GetSystemClock().TickSystemClock();

		g_theRenderer->BeginCamera(m_attractModeCamera);

		BeginFrame();
		Update();
		Render();
		EndFrame();
	}
}

