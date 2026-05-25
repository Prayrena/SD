#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/core/DevConsole.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Game/Entity.hpp"
#include "Game/UI.hpp"
#include <vector>

//g_theApp owns(create, manage, and destroy) the g_theRender, but every else class could use the g_render
class Game;
class Renderer;
class Texture;

enum SoundEffectID
{
	// BGM
	ATTRACTMODE_BGM,
	LEVEL1_BGM,

	// sound effects
	BUTTON_CLICK,
	PAUSE,
	UNPAUSE,
	PLAYER_SHOOT,
	PLAYER_HIT,
	PLAYER_DEFEAT,
	PLAYER_VICTORY,

	ENEMY_SHOOT,
	ENEMY_HIT,
	ENEMY_DEATH,
	PLAYER_ISSPOTTED,

	BULLET_BOUNCE,
	BULLET_EXPLOSION,
	POINTS_GAINED,

	CURRENT_MAP_COMPLETE,

	NUM_SOUNDEFFECTS	
};

enum TextureID
{
	ATTRACTMODE_WALLPAPER,

	PLAYERTANK_BASE,
	PLAYERTANK_TURRET,
	SCORPIO_BASE,
	SCORPIO_TURRET,
	T_LEO,
	T_ARIES,
	T_CAPRICORN,

	T_GOOD_BULLET,
	T_GOOD_BOLT,
	T_EVIL_BULLET,
	T_EVIL_BOLT,
	T_EVIL_MISSILE,

	T_PERISHED_MENU,
	T_VICTORY_MENU,

	SPRITESHEET_TILES,

	SPRITESHEET_EXPLOSION,

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
	static bool EventSystemTesting_SetPlayerInvincible(EventArgs& eventArgs); // currently the event system need the member function to be static

	void ManageAudio();
	// todo: create function switch music: stop current music and start another one

	// sound ids
	SoundID g_soundEffectsID[NUM_SOUNDEFFECTS];
	SoundPlaybackID m_openning_Bgm;
	SoundPlaybackID m_level1_Bgm;

	Texture* g_textureID[NUM_TEXTURES];

	bool   m_singleFrameMode = false;
	bool   m_debugMode = false;

	bool   m_isQuitting = false;
	bool   m_isPaused = false;
	bool   m_isSlowMo = false;
	bool   m_speedIsBoosted = false;
	bool   m_noClipMode = false;
	bool   m_permitToStartTimer = false;// when press start, the timer start to count time for transition to game
	bool   m_attractModeIsOn = true;
	float  m_transitionToGameTimer = 0.f;

	// create a counter to see how many times 'o' is pressed
	int m_OPressedCounter = 0;
	int m_OCounterUpdate = 0;

private:
	void BeginFrame();
	void Update(float deltaSeconds);
	void Render() const;
	void RenderDevConsole() const;
	void EndFrame();

	void LoadGameConfigXml();
	void LoadAudioAssets();
	void LoadTextureAssets();

	void CheckKeyAndButtonStates( );
	void UpdateGameMode(float deltaSeconds);
	void UpdateAttractMode(float deltaSeconds);
	void RenderAttractMode() const;
	void RenderGameModeInstruction() const;
	void RestartGame();

private:
	Vec2			m_shipPos = Vec2(0.f, 50.f); // the start position of the spaceship
	Camera			m_attractModeCamera;

protected:
	double m_timeLastFrame = 0.f;
	double m_timeThisFrame = 0.f;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
// eventSystem testing