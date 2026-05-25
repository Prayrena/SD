#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/GameMode.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/UI.hpp"


//g_theApp owns(create, manage, and destroy) the g_theRender, but every else class could use the g_render

class GetNearestPointsMode;
class Renderer;

enum SoundEffectID
{
	ATTRACTMODE_BGM,

	BUTTON_CLICK,

	NUM_SOUNDEFFECTS
};

enum TextureID
{
	TESTUV,

	NUM_TEXTURES
};

class App
{
public:
	App();//called when the class is instanced, don't return
	~App();//called when the class is dead, don't return

	void Startup();
	void Shutdown();
	void RunFrame();

	bool IsQuitting() const;
	bool HandleQuitRequested();

	// sound ids
	SoundID g_soundEffectsID[NUM_SOUNDEFFECTS];
	SoundPlaybackID m_openningBgm;

	float  m_windowAspectRatio = 2.f;

	bool   m_singleFrameMode = false;
	bool   m_newTest = false;
	bool   m_debugMode = false;

	bool   m_isQuitting = false;
	bool   m_isPaused = false;
	bool   m_isSlowMo = false;
	bool   m_speedIsBoosted = false;
	bool   m_permitToStartTimer = true;// when press start, the timer start to count time for transition to game
	bool   m_attractModeIsOn = false;
	float  m_transitionToGameTimer = 0.f;

	GameMode* m_currentGameMode = nullptr;
	int		  m_GameModeIndex = 0;

	Texture* g_textures[NUM_TEXTURES];
	void LoadTextureAssets();

	// event system functions
	static bool Event_Quit(EventArgs& args);

private:
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

	// void LoadAudioAssets();

	void CheckKeyAndButtonStates( );
	void UpdateGameMode();
	void UpdateAttractMode();
	void RenderAttractMode() const;
	void InitializeAttractMode();
	void DeleteCurrentTestMode_And_CreateNewTestMode( TestingScene type );
	void RestartGame();

	void RenderUIElements() const;


private:
	Vec2	m_shipPos = Vec2(0.f, 50.f); // the start position of the spaceship
	float	m_ringThickness = 0.f;
	Camera	m_attractModeCamera;

	Timer*	m_transformFromAttractToGameTimer;
};
