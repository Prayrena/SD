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
class Shader;

enum SoundEffectID
{
	MAINMENUMUSIC,
	GAMEMUSIC,
	BUTTON_CLICK,
	PLAYER_VICTORY,

	NUM_SOUNDEFFECTS
};

enum TextureID
{
	TESTUV,
	VICTORY_MENU,
	NUM_TEXTURES
};

enum ShaderID
{
	DEFAULT,
	DIFFUSE,
	NUM_SHADERS
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

	Texture* g_textures[NUM_TEXTURES];
	Shader*  g_shaders[NUM_SHADERS];

	// music load path defined by game config xml
	std::string m_mainMenuMusicLoadPath;
	std::string m_gameMusicLoadPath;
	std::string m_buttonClickSoundLoadPath;

	bool   m_singleFrameMode = false;
	bool   m_debugMode = false;

	bool   m_isQuitting = false;

	float  m_windowAspectRatio = 1.f;
	float  m_musicVolume = 1.f;

	// event system functions
	static bool Event_Quit(EventArgs& args);

	Camera m_devConsoleCamera;
private:
	void BeginFrame();
	void Update();
	void Render();
	void EndFrame();

	void RenderDevConsole();

	void LoadAudioAssets();
	void LoadTextureAssets();
	void LoadGameConfigXml();
	void InitializeDefinitions();
	void SetGameConfigByLoadedXml();
	void LoadAllShaders();

	void CheckKeyAndButtonStates( );
	void UpdateGameMode();

	void RestartGame();
};
