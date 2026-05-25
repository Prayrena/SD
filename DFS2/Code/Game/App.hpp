#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/Entity.hpp"
#include "Game/UI.hpp"


//g_theApp owns(create, manage, and destroy) the g_theRender, but every else class could use the g_render

class Game;
class Renderer;
class Material;

enum SoundEffectID
{
	ATTRACTMODE_BGM,

	BUTTON_CLICK,

	NUM_SOUNDEFFECTS
};

enum TextureID
{
	RETICLE,

	NUM_TEXTURES
};

enum MaterialType
{
	CRAWLER,
	NUM_MATERIALS
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

	bool   m_singleFrameMode = false;
	bool   m_debugMode = false;

	bool   m_isQuitting = false;
	bool   m_permitToStartTimer = false;// when press start, the timer start to count time for transition to game
	Timer* m_transformFromAttractToGameTimer;
	bool   m_attractModeIsOn = true;
	float  m_transitionToGameTimer = 0.f;
	float  m_ringThickness = 0.f;

	float  m_windowAspectRatio = 2.f;

	// event system functions
	static bool Event_Quit(EventArgs& args);

	double m_maxRenderDuration;
	double m_maxUpdateDuration;
	double m_maxBeginFrameDuration;

private:
	void BeginFrame();
	void Update();
	void Render();
	void EndFrame();

	void LoadAudioAssets();
	void LoadTextureAssets();
	void LoadMaterialAssets();

	void CheckKeyAndButtonStates( );
	void UpdateGameMode();
	void UpdateAttractMode(float deltaSeconds);
	void RenderAttractMode() const;
	void InitializeAttractMode();
	void RestartGame();
	
private:
	Camera m_attractModeCamera;

};
