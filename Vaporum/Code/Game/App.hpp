#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/Entity.hpp"
#include "Game/UI.hpp"
#include "Game/EngineBuildPreferences.hpp"

#ifdef ENGINE_ENABLE_NETSYSTEM
struct NetSystemConfig;
#endif // ENGINE_ENABLE_NETSYSTEM

//g_theApp owns(create, manage, and destroy) the g_theRender, but every else class could use the g_render
struct WindowConfig;
class Game;
class Renderer;
class Material;

enum class SoundEffectID
{
	EXPLOSION,
	HIT,
	FIRING,

	NUM_SOUNDEFFECTS
};


enum TextureID
{
	TESTUV,
	LOGO,
	ARROW_L_ICON,
	ARROW_R_ICON,
	LMB_ICON,
	RMB_ICON,
	Y_ICON,
	SPRITESHEET_EXPLOSION,
	SPRITESHEET_MUZZLEFIRE,
	SPRITESHEET_SMOKE,
	NUM_TEXTURES
}; 

enum SpriteAnimID
{
	EXPLOSION,
	MUZZLEFIRE,
	SMOKE,
	NUM_SPRITEANIMS
};

enum MaterialType
{
	MOON,
	NUM_MATERIALS
};

class App
{
public:
	App();//called when the class is instanced, don't return
	~App();//called when the class is dead, don't return

	void Startup(std::string commandLine);
	void Shutdown();
	void RunFrame();

	bool IsQuitting() const;
	bool HandleQuitRequested();

	// sound ids
	SoundPlaybackID m_openningBgm;

	bool   m_singleFrameMode = false;
	bool   m_debugMode = false;

	bool   m_isQuitting = false;
	bool   m_permitToStartTimer = false;// when press start, the timer start to count time for transition to game
	Timer* m_transformFromAttractToGameTimer;
	bool   m_attractModeIsOn = false;
	float  m_transitionToGameTimer = 0.f;
	float  m_ringThickness = 0.f;

	float  m_windowAspectRatio = 2.f;

	// event system functions
	static bool Event_Quit(EventArgs& args);
	static bool Event_LoadAssignedGameConfig(EventArgs& args); // this read the "GameConfig_Client.xml"
	bool		LoadGameConfig(); // this read the "GameConfig.xml"

	WindowConfig*		m_windowsConfig = nullptr;

#ifdef ENGINE_ENABLE_NETSYSTEM
	NetSystemConfig* m_netConfig = nullptr;
#endif // ENGINE_ENABLE_NETSYSTEM

private:
	void BeginFrame();
	void Update();
	void Render();
	void EndFrame();

	void LoadGamefig();
	void LoadAudioAssets();
	void LoadTextureAssets();
	void GenerateSpriteAnimationAssets();
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
