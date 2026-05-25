#include "Game/App.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/Time.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/core/NamedStrings.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/core/Image.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/core/Clock.hpp"
#include "Game/Game.hpp"
#include "Game/PlayerTank.hpp"
#include "Game/ShiningTriangle.hpp"
#include <math.h>
#include <iostream>
 
extern App*				g_theApp;// global variable must be define in the cpp
Game*					g_theGame = nullptr;
Renderer*				g_theRenderer = nullptr;
InputSystem*			g_theInput = nullptr;
AudioSystem*			g_theAudio = nullptr;
Window*					g_theWindow = nullptr;
RandomNumberGenerator*	g_rng = nullptr;
SpriteSheet*			g_terrainSprites = nullptr;
BitmapFont*				g_consoleFont = nullptr;
DevConsole*				g_theDevConsole = nullptr;

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
	windowConfig.m_windowTitle = "SD1-A8: Libra Epilogue";
	windowConfig.m_aspectRatio = 2.f;
	g_theWindow = new Window(windowConfig);

	RenderConfig renderConfig;
	renderConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer( renderConfig );

	AudioConfig audioConfig;
	g_theAudio = new AudioSystem(audioConfig);

	g_rng = new RandomNumberGenerator;

	g_theGame = new Game();


	//----------------------------------------------------------------------------------------------------------------------------------------------------
	//unsigned char testingChar = 'X';// unsigned char is a number from 0 to 255
	g_theEventSystem->Startup();
	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	g_theAudio->Startup();

	LoadAudioAssets();
	LoadTextureAssets();
	LoadGameConfigXml();
	g_consoleFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont.png");
	m_openning_Bgm = g_theAudio->StartSound(g_soundEffectsID[ATTRACTMODE_BGM], false, 0.6f, 0.f, 1.f, false);

	g_theGame->Startup();

	// set up development console
	DevConsoleConfig consoleConfig;
	consoleConfig.m_font = g_consoleFont;
	consoleConfig.m_numLinesOnScreen = 12;
	g_theDevConsole = new DevConsole(consoleConfig);

	// testing the event system
	g_gameConfigBlackboard.SetValue("playerHealth", "999");
	g_theEventSystem->SubscribeEventCallbackFunction("playerInvincible", EventSystemTesting_SetPlayerInvincible); // todo: I could only assign standalone functions to event system?

	// testing the development console
	g_theDevConsole->AddLine("Testing", DevConsole::INFO_ERROR);
}

bool App::EventSystemTesting_SetPlayerInvincible(EventArgs& eventArgs)
{
	int health = eventArgs.GetValue("playerHealth", 1);
	g_theGame->m_playerTank->m_health = health;
	return false;
}

void App::Shutdown()
{
	g_theGame->Shutdown();
	g_theRenderer->Shutdown();
	g_theWindow->ShutDown();
	g_theInput->Shutdown();
	g_theEventSystem->Shutdown();

	// destroy game and engine subsystems
	delete g_theGame;
	g_theGame = nullptr;

	delete g_theRenderer;
	g_theRenderer = nullptr;

	delete g_theWindow;
	g_theWindow = nullptr;

	delete g_theInput;
	g_theInput = nullptr;

	delete g_theEventSystem;
	g_theEventSystem = nullptr;
}

void App::BeginFrame()
{
	Clock::TickSystemClock();
	g_theEventSystem->BeginFrame();
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theAudio->BeginFrame();
}

void App::EndFrame()
{
	g_theEventSystem->EndFrame();
	g_theInput->EndFrame();
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
	g_theAudio->EndFrame();
}

void App::LoadAudioAssets()
{
	g_soundEffectsID[ATTRACTMODE_BGM] = g_theAudio->CreateOrGetSound("Data/BGM/InterstellarMainTheme.mp3");
	g_soundEffectsID[LEVEL1_BGM] = g_theAudio->CreateOrGetSound("Data/BGM/level1_BGM.wav");
	g_soundEffectsID[BUTTON_CLICK] = g_theAudio->CreateOrGetSound("Data/SoundEffects/Click.mp3");
	g_soundEffectsID[PAUSE] = g_theAudio->CreateOrGetSound("Data/SoundEffects/Pause.mp3");
	g_soundEffectsID[UNPAUSE] = g_theAudio->CreateOrGetSound("Data/SoundEffects/Unpause.mp3");
	g_soundEffectsID[PLAYER_SHOOT] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PlayerFireBullet.wav");
	g_soundEffectsID[PLAYER_HIT] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PlayerHit.wav");
	g_soundEffectsID[PLAYER_DEFEAT] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PlayerDeath.mp3");
	g_soundEffectsID[PLAYER_VICTORY] = g_theAudio->CreateOrGetSound("Data/SoundEffects/Victory.mp3");
	g_soundEffectsID[PLAYER_ISSPOTTED] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PlayerIsSpotted.mp3");
	g_soundEffectsID[ENEMY_SHOOT] = g_theAudio->CreateOrGetSound("Data/SoundEffects/EnemyShoot.wav");
	g_soundEffectsID[ENEMY_HIT] = g_theAudio->CreateOrGetSound("Data/SoundEffects/EnemyHit.wav");
	g_soundEffectsID[ENEMY_DEATH] = g_theAudio->CreateOrGetSound("Data/SoundEffects/EnemyDied.wav");
	g_soundEffectsID[BULLET_BOUNCE] = g_theAudio->CreateOrGetSound("Data/SoundEffects/BulletBounce.wav");
	g_soundEffectsID[POINTS_GAINED] = g_theAudio->CreateOrGetSound("Data/SoundEffects/PointsGained.mp3");
	g_soundEffectsID[BULLET_EXPLOSION] = g_theAudio->CreateOrGetSound("Data/SoundEffects/Explosion.mp3");
	g_soundEffectsID[CURRENT_MAP_COMPLETE] = g_theAudio->CreateOrGetSound("Data/SoundEffects/CurrentMapComplete.mp3");
}


void App::LoadTextureAssets()
{
	// Image* defaultImage = new Image(IntVec2(2, 2), Rgba8::MAGENTA);
	// g_textureID[ATTRACTMODE_WALLPAPER] = g_theRenderer->CreateTextureFromImage(*defaultImage);
	g_textureID[ATTRACTMODE_WALLPAPER] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/AttractScreen.png");
	g_textureID[PLAYERTANK_BASE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PlayerTankBase.png");
	g_textureID[PLAYERTANK_TURRET] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PlayerTankTop.png");
	g_textureID[SCORPIO_BASE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/ScorpioBase.png");
	g_textureID[SCORPIO_TURRET] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/ScorpioTurret.png");
	g_textureID[T_LEO] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Leo.png");
	g_textureID[T_ARIES] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Aries.png");
	g_textureID[T_CAPRICORN] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Capricorn.png");
	g_textureID[T_GOOD_BULLET] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyBullet.png");
	g_textureID[T_GOOD_BOLT] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyBolt.png");
	g_textureID[T_EVIL_BULLET] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyBullet.png");
	g_textureID[T_EVIL_BOLT] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyBolt.png");
	g_textureID[T_EVIL_MISSILE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyShell.png");
	g_textureID[T_PERISHED_MENU] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/YouDiedScreen.png");
	g_textureID[T_VICTORY_MENU] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/VictoryScreen.jpg");
	g_textureID[SPRITESHEET_TILES] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Terrain_8x8.png");
	g_textureID[SPRITESHEET_EXPLOSION] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Explosion_5x5.png");

	g_terrainSprites = new SpriteSheet(*g_textureID[SPRITESHEET_TILES], IntVec2(8, 8));
}

void App::LoadGameConfigXml()
{
	XmlDocument GameConfigXml;
	char const* filePath = "Data/LibraGameConfig.xml";
	XmlResult result = GameConfigXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("failed to load xml file"));

	XmlElement* rootElement = GameConfigXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "rootElement is nullPtr");

	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);
}

bool App :: IsQuitting()const
{
	return m_isQuitting;
}

//-----------------------------------------------------------------------------------------------

bool App::HandleQuitRequested()
{
	m_isQuitting = true;
	return true;
}

void App::ManageAudio()
{
	XboxController const& controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed('P') || controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
	{

		// in game mode and game was playing
		if (!m_attractModeIsOn && m_isPaused)
		{
			g_theAudio->SetSoundPlaybackSpeed(m_level1_Bgm, 0.f);
		}
		
		// in game mode and it was paused
		if (!m_attractModeIsOn && !m_isPaused)
		{
			g_theAudio->SetSoundPlaybackSpeed(m_level1_Bgm, 1.f);
		}
	}
}

/// <Update per frame functions>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void App::Update(float deltaSeconds)
{	
	ManageAudio();

	if (m_permitToStartTimer)
	{
		m_transitionToGameTimer += deltaSeconds;
	}

	if (m_transitionToGameTimer >= TIME_TRANSITION_GAME)
	{
		m_attractModeIsOn = false;
	}

	if (m_attractModeIsOn)
	{
		UpdateAttractMode(deltaSeconds);
	}

	UpdateGameMode(deltaSeconds);
	CheckKeyAndButtonStates(); // have functions that resetart the game, therefore it needs to be at the bottom
}

void App::CheckKeyAndButtonStates()
{
	XboxController const& controller = g_theInput->GetController(0);

	// slow mode
	if (g_theInput->IsKeyDown('T') || controller.IsButtonDown(XboxButtonID::XBOX_BUTTON_RSHOULDER))
	{
		m_isSlowMo = true;
	}
	else      
	{
		m_isSlowMo = false;
	}

	// speed boost mode
	if (g_theInput->IsKeyDown('Y') || controller.IsButtonDown(XboxButtonID::XBOX_BUTTON_LSHOULDER))
	{
		m_speedIsBoosted = true;
	}
	else
	{
		m_speedIsBoosted = false;
	}

	// discuss four situations about the pause mode and single frame mode
	// only when enter the game mode
	if (!m_attractModeIsOn)
	{
		if (g_theInput->WasKeyJustPressed('P') || controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
		{
			if (m_isPaused == false && m_singleFrameMode == false)
			{
				m_isPaused = true;
				g_theAudio->StartSound(g_soundEffectsID[PAUSE], false, 1.0f, 0.f, 1.f, false);
				g_theAudio->SetSoundPlaybackSpeed(m_level1_Bgm, 0.f);
			}
			// pressed P under single frame mode make the game back on normal
			else if (m_isPaused == false && m_singleFrameMode == true)
			{
				m_OPressedCounter = 0;
				m_OCounterUpdate = 0;
				m_singleFrameMode = false;
			}
			// under pause mode, if player press p again, game will resume
			else if (m_isPaused == true && m_singleFrameMode == false)
			{
				m_isPaused = false;
				g_theAudio->StartSound(g_soundEffectsID[UNPAUSE], false, 1.0f, 0.f, 1.f, false);
				g_theAudio->SetSoundPlaybackSpeed(m_level1_Bgm, 1.f);
			}
			// under pause mode + analysis mode, another p being pressed will put game back on normal
			else if (m_isPaused == true && m_singleFrameMode == true)
			{
				m_OPressedCounter = 0;
				m_OCounterUpdate = 0;

				m_singleFrameMode = false;
				m_isPaused = false;
				g_theAudio->StartSound(g_soundEffectsID[UNPAUSE], false, 1.0f, 0.f, 1.f, false);
				g_theAudio->SetSoundPlaybackSpeed(m_level1_Bgm, 1.f);
			};
		}

		// open up the single frame mode
		// discuss four situations about the pause mode and single frame mode
		if (g_theInput->WasKeyJustPressed('O'))
		{
			if (m_isPaused == false && m_singleFrameMode == false)
			{
				m_OPressedCounter++;
				m_singleFrameMode = true;
				return;
			}
			// under single frame mode, if player press o, the game will only run next frame
			if (m_isPaused == false && m_singleFrameMode == true)
			{
				m_OPressedCounter++;
				return;
			}
			// under pause mode, if player press o, the game will only run next frame
			if (m_isPaused == true && m_singleFrameMode == false)
			{
				m_singleFrameMode = true;
				m_OPressedCounter++;
				return;
			};
			// under pause mode + analysis mode, if player press o, the game will only run next frame
			if (m_isPaused == true && m_singleFrameMode == true)
			{
				m_singleFrameMode = true;
				m_OPressedCounter++;
				return;
			};
		}
	}

	if ( g_theInput->WasKeyJustPressed(' ') || controller.WasButtonJustPressed(XBOX_BUTTON_START) || g_theInput->WasKeyJustPressed('P'))
	{
		g_theAudio->StartSound(g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
		// space bar or start to exit the attract mode
		if (m_attractModeIsOn)
		{
			m_permitToStartTimer = true;
			g_theAudio->StopSound(m_openning_Bgm);
			m_level1_Bgm = g_theAudio->StartSound(g_soundEffectsID[LEVEL1_BGM], true, 0.2f);
		}
		if (!m_attractModeIsOn)
		{
			switch (g_theGame->m_gameState)
			{
			case PERISHED: // when player is dead, press start to respawn
			{
				g_theGame->RespawnPlayer();
			} break;
			case VICTORY: // when player win the game, press start to restart game
			{
				RestartGame();
			} break;
			}
		}
	}

	// when showing perished menu, press N could also respawn player
	// if (g_theGame->m_gameState == PERISHED)
	// {
	// 	if (g_theInput->WasKeyJustPressed('N'))
	// 	{
	// 		g_theGame->RespawnPlayer();
	// 	}
	// }

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

	// F3 for no clip mode
	if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
	{
		if (!m_noClipMode)
		{
			m_noClipMode = true;
			g_theDevConsole->AddLine("m_noClipMode = true", DevConsole::INFO_MAJOR);
			return;
		}
		else 
		{
			g_theDevConsole->AddLine("m_noClipMode = false", DevConsole::INFO_MAJOR);
			m_noClipMode = false;
		}
	}


	// F4 for showing the whole map
	if (g_theInput->WasKeyJustPressed(KEYCODE_F4))
	{
		if (!g_theGame->m_debugCamerIsOn)
		{
			g_theGame->m_debugCamerIsOn = true;
		}
		else
		{
			g_theGame->m_debugCamerIsOn = false;
		}
	}

	// F2 for toggling the mode of the development console
	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
	{
		g_theDevConsole->ToggleMode();
	}

	// F5 for testing the event system and devConsole
	if (g_theInput->WasKeyJustPressed(KEYCODE_F5))
	{
		g_theDevConsole->Execute("playerInvincible playerHealth = 999");
		g_theDevConsole->AddLine("playerInvincible playerHealth = 999", DevConsole::INFO_MAJOR);
	}

	// Quit Application
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustReleased(XboxButtonID::XBOX_BUTTON_BACK))
	{
		g_theAudio->StartSound(g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
		if (m_attractModeIsOn) // when press back at the attract mode, the application quits
		{
			m_isQuitting = true;
			return;
		}
		if (!m_attractModeIsOn && m_isPaused) // when enter the game and is paused, press back will restart to attract mode
		{
			RestartGame();
			return;
		};
		if (g_theGame->m_gameState == PERISHED || g_theGame->m_gameState == VICTORY)// when player is dead or win, press back will restart to attract mode
		{
			RestartGame();
			return;
		}
		m_isPaused = true;
		m_singleFrameMode = false;
		m_debugMode = false;
		g_theAudio->SetSoundPlaybackSpeed(m_level1_Bgm, 0.f);
		g_theAudio->StartSound(g_soundEffectsID[PAUSE], false, 1.0f, 0.f, 1.f, false);
	}
	// Restart Game
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		if (g_theGame)
		{
			g_theAudio->StartSound(g_soundEffectsID[BUTTON_CLICK], false, 1.0f, 0.f, 1.f, false);
			RestartGame();
		}
	}
}

void App::UpdateGameMode(float deltaSeconds)
{
	if (!m_attractModeIsOn)
	{
		// enter slow mode will deltaSecond down to 1/10 or its original
		if (m_isSlowMo)
		{
			deltaSeconds *= 0.1f;
		}

		// pause mode will make deltaSeconds to o so nobody moves
		if (m_isPaused)
		{
			deltaSeconds = 0.f;
		}

		// boost speed mode 
		if (m_speedIsBoosted)
		{
			deltaSeconds *= 4.f;
		}

		//this define which object's logic will be controlled by the single frame mode
		if (m_OPressedCounter != 0)
		{
			if (m_OPressedCounter > m_OCounterUpdate)
			{
				// functions thats run one frame when o is pressed
				// UpdateShip(deltaSeconds);
				// use m_game -> update(); to substitude all of within
				m_OCounterUpdate += 2;
				return;
			}
			if (m_OPressedCounter == m_OCounterUpdate)
			{
				g_theGame->Update(deltaSeconds);
				m_OCounterUpdate++;
				return;
			}
			return;
		}

		g_theGame->Update(deltaSeconds);
	}
}

void App::RestartGame()
{
	g_theAudio->StopSound(m_level1_Bgm);
	g_theGame->Shutdown();
	delete g_theGame;
	
	m_attractModeIsOn = true;
	m_permitToStartTimer = false;
	m_transitionToGameTimer = 0.f;
	m_isPaused = false;
	m_isSlowMo = false;
	m_debugMode = false;
	g_theGame = new Game();
	g_theGame->Startup();
	m_openning_Bgm = g_theAudio->StartSound(g_soundEffectsID[ATTRACTMODE_BGM], false, 1.0f, 0.f, 1.f, false);
}

// because this function is const, it will use not any "set XXXXXX" functions
void App::Render() const
{
	g_theRenderer->ClearScreen(Rgba8(WORLD_COLOR_R, WORLD_COLOR_G, WORLD_COLOR_B, 255));//the background color setting of the window
	if (!m_attractModeIsOn)
	{
		g_theGame->Render();
	}

	g_theRenderer->BeginCamera(m_attractModeCamera);
	if (m_attractModeIsOn)
	{
		RenderAttractMode();
	}
	g_theRenderer->EndCamera(m_attractModeCamera);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	float screenAspect = g_theWindow->GetAspect();
	AABB2 fullSreenBounds(Vec2(0.f, 0.f), Vec2(800.f * screenAspect, 800.f));
	Camera consoleCamera(fullSreenBounds);
	g_theRenderer->BeginCamera(consoleCamera);
	RenderDevConsole(); // the development console is render above everything else
	g_theRenderer->EndCamera(consoleCamera);

}

void App::RenderDevConsole() const
{
	AABB2 consoleBounds = m_attractModeCamera.GetCameraBounds();
	g_theDevConsole->Render(consoleBounds, g_theRenderer);
}

/// <Attract Mode>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void App::UpdateAttractMode(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

void App::RenderAttractMode() const
{
	// draw the attract wall paper
	std::vector<Vertex_PCU> verts;
	AABB2 testingTexture = m_attractModeCamera.GetCameraBounds();
	AddVertsForAABB2D(verts, testingTexture, Rgba8::WHITE);
	g_theRenderer->BindTexture(g_textureID[ATTRACTMODE_WALLPAPER]);
	g_theRenderer->DrawVertexArray( (int)verts.size(), verts.data() );

	// draw breathing ring
	Vec2 screenSize;
	screenSize.x = g_gameConfigBlackboard.GetValue("screen_size_x", 999.f);
	screenSize.y = g_gameConfigBlackboard.GetValue("screen_size_y", 999.f);

	Vec2 center = screenSize * 0.5f;
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	Rgba8 ringColor = Rgba8(255, 160, 133, 175);
	float ringThickness = 0.05f * screenSize.y * sinf((float)m_timeThisFrame);
	g_theRenderer->BindTexture(nullptr);
	DebugDrawRing(center, screenSize.y * 0.2f, ringThickness, ringColor);

	// title font
	std::vector<Vertex_PCU> textVerts;
	AABB2 screenBounds = m_attractModeCamera.GetCameraBounds();
	g_consoleFont->AddVertsForTextInBox2D(textVerts, "Libra\nEpilogue", screenBounds, 0.06f * screenSize.y, Vec2(0.5f, 0.5f), Rgba8::CYAN);
	g_consoleFont->AddVertsForTextInBox2D(textVerts, "Programmed by:\nChengxiang Li", screenBounds, 0.03f * screenSize.y, Vec2(1.f, 0.5f), Rgba8::CYAN);
	g_consoleFont->AddVertsForTextInBox2D(textVerts, "Designed by:\nProfessor Eiserloh", screenBounds, 0.03f * screenSize.y, Vec2(0.f, 0.5f), Rgba8::CYAN);
	g_theRenderer->BindTexture(&g_consoleFont->GetTexture());
	g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());

}

void App::RenderGameModeInstruction() const
{

}

//-----------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
void App::RunFrame()
{
	while (!m_isQuitting)
	{
		// float deltaSeconds = 1.f / 60.f; // assuming 60 fps, keep the deltaSeconds a static number
		// know each deltaSeconds is calculate by the time difference
		m_timeThisFrame = GetCurrentTimeSeconds();
		float deltaSeconds = static_cast<float>(m_timeThisFrame - m_timeLastFrame);
		m_timeLastFrame = m_timeThisFrame;

		// camera view might change according to the mechanics in the future
		//set the 200x100 orthographic (2D) world and drawing coordinate system, 
		Vec2 screenSize;
		screenSize.x = g_gameConfigBlackboard.GetValue("screen_size_x", 1600.f);
		screenSize.y = g_gameConfigBlackboard.GetValue("screen_size_y", 800.f);

		m_attractModeCamera.SetOrthoView(Vec2(0.f, 0.f), screenSize);

		g_theRenderer->BeginCamera(m_attractModeCamera);

		BeginFrame();
		Update(deltaSeconds);
		Render();
		EndFrame();
	}
}

//void SquirrelsFunction()
//{
//	int x = 5;
//}
//void BobsFunction()
//{
//	int x = 5;
//}
//typedef void(*AnyVoidFunctionPtrType)();
//
//AnyVoidFunctionPtrType functionPtr = SquirrelsFunction;
//functionPtr();

