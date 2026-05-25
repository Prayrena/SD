#pragma once
#include "Engine/core/EngineCommon.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Entity.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/UIElement.hpp"

class NamedStrings;
typedef NamedStrings EventArgs;

enum SoundEffectID
{
	ATTRACTMODE_BGM,
	GAMEMODE_BGM,

	SHOOTINGBULLETS,
	PLAYER_DEAD,
	PLAYER_RESPAWN,
	ENEMY_WASHIT,
	ENEMY_DESTROYED,
	NEW_WAVE,

	GAMEOVER_VICTORY,
	GAMEOVER_DEFEATE,

	ENERGY_DEPLETED,
	ENERGY_RINGTURNEDON,
	SPEED_BOOST,
	FIRERATE_BOOST,


	NUM_SOUNDEFFECTS
};


//g_theApp owns(create, manage, and destroy) the g_theRender, but every else class could use the g_render

class Game;
class Renderer;

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

	void LoadAudioAssets();
	void ManageAudio();

	// event system functions
	static bool Event_Quit(EventArgs& args);

	// sound ids
	SoundID m_blasterSoundID;
	SoundID m_openningBgmID;

	SoundPlaybackID m_openningBgm;
	SoundPlaybackID m_gameModeBgm;
	SoundPlaybackID g_soundEffectsID[NUM_SOUNDEFFECTS];

	bool   m_debugMode = false;

	bool   m_isQuitting = false;
	bool   m_permitToStartTimer = false;// when press start, the timer start to count time for transition to game
	bool   m_attractModeIsOn = true;
	float  m_transitionToGameTimer = 0.f;

	// create a counter to see how many times 'o' is pressed
	int m_OPressedCounter = 0;
	int m_OCounterUpdate = 0;

private:
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

	void CheckKeyAndButtonStates( );
	void UpdateGameMode();
	void UpdateAttractMode(float deltaSeconds);
	UIElement* m_playerShipIcon[UI_POSITION_ATTRACTMODE_ICON_NUM] = {};
	void RenderAttractMode() const;
	void InitializeAttractMode();
	void RestartGame();

	UIElement* m_shiningTriangle = nullptr;

private:
	Vec2   m_shipPos = Vec2(0.f, 50.f); // the start position of the spaceship
	Camera m_attractModeCamera;
	double m_timeLastFrame = 0.f;
};
