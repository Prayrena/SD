#include "Engine/core/EngineCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/core/Time.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/VFX/EffectSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/core/DevConsole.hpp"
#include "Engine/Model/Material.hpp"
#include "Engine/Net/NetSystem.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include <iostream>
#include <math.h>

#ifdef ENGINE_ENABLE_NETSYSTEM
#include "Engine/Net/NetSystem.hpp"
NetSystem* g_theNetSystem = nullptr;
#endif // ENGINE_ENABLE_NETSYSTEM
 
extern App* g_theApp;// global variable must be define in the cpp
extern Clock* g_theGameClock;

Game* g_theGame = nullptr;
Renderer* g_theRenderer = nullptr;
InputSystem* g_theInput = nullptr;
AudioSystem* g_theAudio = nullptr;
Window* g_theWindow = nullptr;
BitmapFont* g_consoleFont = nullptr;
DevConsole* g_theDevConsole = nullptr;
EffectSystem* g_theEffectSystem = nullptr;

Texture* g_textures[NUM_TEXTURES];
SpriteAnimDefinition* g_spriteAnims[NUM_SPRITEANIMS];
Material* g_materials[NUM_MATERIALS];
SoundID g_soundEffectsID[static_cast<unsigned long long>(SoundEffectID::NUM_SOUNDEFFECTS)];

App::App()
{

}

App :: ~App()
{

}

void App :: Startup (std::string commandLine)
{   
	// Create engine subsystems and game
	EventSystemConfig eventConfig;
	g_theEventSystem = new EventSystem(eventConfig);

	// set up development console
	DevConsoleConfig consoleConfig;
	g_theDevConsole = new DevConsole(consoleConfig);

	InputConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);
	g_theInput->SetDesiredCursorMode(false, false);

	// get windows config from the XML
	// GameConfig.xml in the root folder
	LoadGameConfig(); 

	// sometimes we'll use the command line to override some of the windows settings
	// loading "GameConfig_Client"
	SubscribeEventCallbackFunction("LoadGameConfig", App::Event_LoadAssignedGameConfig);
	if (!commandLine.empty())
	{
		g_theDevConsole->Execute(commandLine);
	}

	if (!m_windowsConfig) // if we don't have GameConfig.xml in the root folder or command line to load specific game setting
	{
		WindowConfig windowsConfig;
		windowsConfig.m_windowTitle = "Unnamed Application";
	}
	m_windowsConfig->m_inputSystem = g_theInput;
	g_theWindow = new Window(*m_windowsConfig);

#ifdef ENGINE_ENABLE_NETSYSTEM
	if (m_netConfig)
	{
		g_theNetSystem = new NetSystem(*m_netConfig);
	}
	else
	{
		NetSystemConfig config;
		g_theNetSystem = new NetSystem(config);
	}
	g_theNetSystem->Startup();
#endif // ENGINE_ENABLE_NETSYSTEM

	RenderConfig renderConfig;
	renderConfig.m_window = g_theWindow;
#ifdef USE_EMISSIVE_MATERIAL
	renderConfig.m_emissiveEnabled = true; // default is false
#endif
	g_theRenderer = new Renderer(renderConfig);

	AudioConfig audioConfig;
	g_theAudio = new AudioSystem(audioConfig);

	g_theEffectSystem = new EffectSystem();

	g_theGame = new Game();

	g_theEventSystem->Startup();
	g_theEffectSystem->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	// call devConsole before the input system because of subscription sequence is prior
	g_theDevConsole->Startup();
	g_theInput->Startup();
	g_theAudio->Startup();

	// reset the setting of the dev console
	// the bit front needed to be created after the renderer start up function
	// g_consoleFont = g_theRenderer->CreateBitmapFont("Data/Fonts/font_Roboto.png");
	g_consoleFont = g_theRenderer->CreateBitmapFont("Data/Fonts/RobotoMonoSemiBold128.png");
	// g_consoleFont = g_theRenderer->CreateBitmapFont("Data/Fonts/SquirrelFixedFont.png");
	g_theDevConsole->m_config.m_renderer = g_theRenderer;
	g_theDevConsole->m_config.m_font = g_consoleFont;
	g_theDevConsole->m_config.m_camera = &m_attractModeCamera;

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_theRenderer;
	debugRenderConfig.m_font = g_consoleFont;
	DebugRenderSystemStartup(debugRenderConfig);

	LoadAudioAssets();
	LoadTextureAssets();
	GenerateSpriteAnimationAssets();
	LoadMaterialAssets();

	// m_openningBgm = g_theAudio->StartSound(g_soundEffectsID[ATTRACTMODE_BGM], false, 1.0f, 0.f, 1.f, false);
	//set the 200x100 orthographic (2D) world and drawing coordinate system 
	if ((WORLD_SIZE_X / WORLD_SIZE_Y) != g_theWindow->GetAspect())
	{
		m_attractModeCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_Y * g_theWindow->GetAspect(), WORLD_SIZE_Y));
	}
	else
	{
		m_attractModeCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	}

	// m_attractModeCamera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	g_theDevConsole->AddInstruction("Type help for a list of commands");
	g_theDevConsole->AddInstruction("Controls", DevConsole::INFO_MINOR);
	g_theDevConsole->AddInstruction("Mouse			- Select");
	g_theDevConsole->AddInstruction("W / A / S / D  - Move");
	g_theDevConsole->AddInstruction("Q / E			- Zoom Camera");
	g_theDevConsole->AddInstruction("H				- Set Camera to Origin");
	g_theDevConsole->AddInstruction("Arrow Keys     - Control Light Direction");
	g_theDevConsole->AddInstruction("~				- Open Dev Console");
	g_theDevConsole->AddInstruction("Escape			- Exit Game");
	g_theDevConsole->AddInstruction("Space			- Start Game");

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
	g_soundEffectsID[static_cast<unsigned long long>(SoundEffectID::EXPLOSION)] = g_theAudio->CreateOrGetSound("Data/Audio/Explosion.wav");
	g_soundEffectsID[static_cast<unsigned long long>(SoundEffectID::HIT)] = g_theAudio->CreateOrGetSound("Data/Audio/Hit.wav");
	g_soundEffectsID[static_cast<unsigned long long>(SoundEffectID::FIRING)] = g_theAudio->CreateOrGetSound("Data/Audio/TankShot.wav");
}

void App::LoadTextureAssets()
{
	g_textures[TESTUV] = g_theRenderer->CreateOrGetTextureFromFile("Data/Textures/TestUV.png");
	g_textures[LOGO] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Logo.png");
	g_textures[ARROW_L_ICON] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Icons/Left.png");
	g_textures[ARROW_R_ICON] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Icons/Right.png");
	g_textures[LMB_ICON] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Icons/LMB.png");
	g_textures[RMB_ICON] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Icons/RMB.png");
	g_textures[Y_ICON] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Icons/Y.png");

	g_textures[SPRITESHEET_EXPLOSION] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Particles/Explosion_5x5.png");
	g_textures[SPRITESHEET_MUZZLEFIRE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Particles/Muzzle_3x2.png");
	g_textures[SPRITESHEET_SMOKE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Particles/Smoke_5x2.png");
}

void App::GenerateSpriteAnimationAssets()
{
	SpriteSheet* explosionSpriteSheet = new SpriteSheet(*g_textures[SPRITESHEET_EXPLOSION], IntVec2(5, 5));
	g_spriteAnims[EXPLOSION] = new SpriteAnimDefinition(*explosionSpriteSheet, 0, 24, 2.f);

	SpriteSheet* muzzleFireSpriteSheet = new SpriteSheet(*g_textures[SPRITESHEET_MUZZLEFIRE], IntVec2(3, 2));
	g_spriteAnims[MUZZLEFIRE] = new SpriteAnimDefinition(*muzzleFireSpriteSheet, 0, 4, 0.5f);	
	
	SpriteSheet* smokeFireSpriteSheet = new SpriteSheet(*g_textures[SPRITESHEET_SMOKE], IntVec2(5, 2));
	g_spriteAnims[SMOKE] = new SpriteAnimDefinition(*smokeFireSpriteSheet, 0, 9, 1.f, SpriteAnimPlaybackType::PINGPONG);
}

void App::LoadMaterialAssets()
{
	g_materials[MOON] = new Material();
	g_materials[MOON]->Load("Data/Materials/Moon.xml");	
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

#ifdef ENGINE_ENABLE_NETSYSTEM
	g_theNetSystem->Shutdown();
	delete g_theNetSystem;
	g_theNetSystem = nullptr;
#endif // ENGINE_ENABLE_NETSYSTEM


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

#ifdef ENGINE_ENABLE_NETSYSTEM
	g_theNetSystem->BeginFrame();
#endif // ENGINE_ENABLE_NETSYSTEM

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

bool App::Event_LoadAssignedGameConfig(EventArgs& args)
{
	std::string filePathName = args.GetValue("file", "files undefined");

	XmlDocument configXml;
	char const* configXmlFilePath = filePathName.c_str();
	XmlResult result = configXml.LoadFile(configXmlFilePath);

	if (!result == tinyxml2::XML_SUCCESS) // can not open the XML
	{
		return false;
	}

	XmlElement* configElement = configXml.RootElement();
	if (!configElement && strcmp(configElement->Name(), "GameConfig") == 0) // the XML file does not have a root element or the name does not match up
	{
		return false;
	}

	// if the windows config already exist, we'll override some of the settings
	if (!g_theApp->m_windowsConfig)
	{
		g_theApp->m_windowsConfig = new WindowConfig();
	}

	// parse the content of the xml 
	g_theApp->m_windowsConfig->m_isFullscreen = ParseXmlAttribute(*configElement, "windowFullscreen", false);
	g_theApp->m_windowsConfig->m_size = ParseXmlAttribute(*configElement, "windowSize", IntVec2(-1, -1));
	g_theApp->m_windowsConfig->m_pos = ParseXmlAttribute(*configElement, "windowPosition", IntVec2(-1, -1));
	g_theApp->m_windowsConfig->m_windowTitle = ParseXmlAttribute(*configElement, "windowTitle", "Unnamed Application");

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (!g_theApp->m_netConfig)
	{
		g_theApp->m_netConfig = new NetSystemConfig();
	}

	std::string netModeName = ParseXmlAttribute(*configElement, "netMode", "net mode not defined");
	if (netModeName == "Client")
	{
		g_theApp->m_netConfig->m_mode = NetSystemMode::CLIENT;
	}
	if (netModeName == "Server")
	{
		g_theApp->m_netConfig->m_mode = NetSystemMode::SERVER;
	}

	return true;
}

bool App::LoadGameConfig()
{
	std::string filePathName = "Data/GameConfig.xml";

	XmlDocument configXml;
	char const* configXmlFilePath = filePathName.c_str();
	XmlResult result = configXml.LoadFile(configXmlFilePath);

	if (!result == tinyxml2::XML_SUCCESS) // can not open the XML
	{
		return false;
	}

	XmlElement* configElement = configXml.RootElement();
	if (!configElement && strcmp(configElement->Name(), "GameConfig") == 0) // the XML file does not have a root element or the name does not match up
	{
		return false;
	}

	g_theApp->m_windowsConfig = new WindowConfig();

	// parse the content of the xml 
	g_theApp->m_windowsConfig->m_windowTitle = ParseXmlAttribute(*configElement, "windowTitle", "Unnamed Application");
	g_theApp->m_windowsConfig->m_aspectRatio = ParseXmlAttribute(*configElement, "windowAspect", 2.f);

	g_theApp->m_windowsConfig->m_isFullscreen = ParseXmlAttribute(*configElement, "windowFullscreen", false);
	g_theApp->m_windowsConfig->m_size = ParseXmlAttribute(*configElement, "windowSize", IntVec2(-1, -1));
	g_theApp->m_windowsConfig->m_pos = ParseXmlAttribute(*configElement, "windowPosition", IntVec2(-1, -1));

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	g_theApp->m_netConfig = new NetSystemConfig();

	std::string netModeName = ParseXmlAttribute(*configElement, "netMode", "net node not found");
	if (netModeName == "Client")
	{
		g_theApp->m_netConfig->m_mode = NetSystemMode::CLIENT;
	}
	if (netModeName == "Server")
	{
		g_theApp->m_netConfig->m_mode = NetSystemMode::SERVER;
	}

	g_theApp->m_netConfig->m_sendBufferSize = ParseXmlAttribute(*configElement, "netSendBufferSize", 1024);
	g_theApp->m_netConfig->m_recvBufferSize = ParseXmlAttribute(*configElement, "netRecvBufferSize", 1024);
	g_theApp->m_netConfig->m_hostAddressString = ParseXmlAttribute(*configElement, "netHostAddress", "Address and port not defined");

	return true;
}

/// <Update per frame functions>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void App::Update()
{	
	// float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();

	// if (m_transformFromAttractToGameTimer->HasPeroidElapsed())
	// {
	// 	m_attractModeIsOn = false;
	// 	g_theInput->SetDesiredCursorMode(false, false);
	// }
	// 
	// if (m_attractModeIsOn)
	// {
	// 	UpdateAttractMode(deltaSeconds);
	// }

	// otherwise the change of button could happen at anytime when key is pressed and the order is not guaranteed
	CheckKeyAndButtonStates();
	UpdateGameMode();

	RestartGame();
}

void App::CheckKeyAndButtonStates()
{
	// XboxController const& controller = g_theInput->GetController(0);

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

	// // space bar to enter exit the attract mode
	// if (g_theInput->WasKeyJustPressed(' ') || controller.WasButtonJustPressed(XBOX_BUTTON_START))
	// {
	// 	g_theAudio->StartSound(g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
	// 	m_transformFromAttractToGameTimer->Start();
	// }
	// 
	// // F1 for entering debug mode
	// if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	// {
	// 	if (m_debugMode)
	// 	{
	// 		m_debugMode = false;
	// 	}
	// 	else
	// 	{
	// 		m_debugMode = true;
	// 	}
	// }

	// Restart Game
	// if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	// {
	// 	if (g_theGame)
	// 	{
	// 		// g_theAudio->StartSound(g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
	// 		// g_theAudio->StopSound(m_openningBgm);
	// 		// the game will delete all its children first
	// 		g_theGame->Shutdown();
	// 		g_theGame = nullptr;
	// 		Startup();
	// 		m_attractModeIsOn = true;
	// 		g_theInput->m_inAttractMode = true;
	// 		m_transformFromAttractToGameTimer->Stop();
	// 	}
	// }

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// ESC and pause logic
	// Quit Application
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && g_theGame->m_currentGameState == GameState::SPLASH_SCREEN)
	{
		m_isQuitting = true;
	}
	// // pause the game when in game
	// else if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && !m_attractModeIsOn && !g_theGameClock->IsPaused())
	// {
	// 	g_theGameClock->Pause();
	// 	return;
	// }
	// 
	// if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK) && m_attractModeIsOn)
	// {
	// 	m_isQuitting = true;
	// }
	// else if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK) && !m_attractModeIsOn && !g_theGameClock->IsPaused())
	// {
	// 	g_theGameClock->Pause();
	// 	return;
	// }

	// in game when pause and press ESC, quit to attract mode
	// if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && g_theGameClock->IsPaused())
	// {
	// 	g_theGameClock->TogglePause();
	// 	m_attractModeIsOn = true;
	// 	g_theInput->SetDesiredCursorMode(false, false);
	// 	m_transformFromAttractToGameTimer->Stop();
	// 	g_theGame->m_gameIsOver = true;
	// }
	// if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK) && g_theGameClock->IsPaused())
	// {
	// 	g_theGameClock->TogglePause();
	// 	m_attractModeIsOn = true;
	// 	g_theInput->SetDesiredCursorMode(false, false);
	// 	m_transformFromAttractToGameTimer->Stop();
	// 	g_theGame->m_gameIsOver = true;
	// }
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
		g_theInput->SetDesiredCursorMode(false, false);
		m_transformFromAttractToGameTimer->Stop();
		m_debugMode = false;
		InitializeAttractMode();
		g_theGame = new Game();
		g_theGame->Startup();
	}
}

void App::Render()
{
	// g_theRenderer->ClearScreen(Rgba8::GREENISH_GRAY);//the background color setting of the window
	g_theRenderer->ClearScreen(Rgba8::BLACK);//the background color setting of the window

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
	Vec2 center = Vec2(m_attractModeCamera.GetCameraBounds().m_maxs * 0.5f);
	Rgba8 ringColor = Rgba8::WHITE;
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

#ifdef ENGINE_ENABLE_NETSYSTEM
	g_theNetSystem->EndFrame();
#endif // ENGINE_ENABLE_NETSYSTEM

	DebugRenderEndFrame();
}

void App::LoadGamefig()
{

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

