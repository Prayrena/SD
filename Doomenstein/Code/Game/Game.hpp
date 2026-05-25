#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/GameCommon.hpp"
#include <vector>
#include <string>

class App;
class Entity;
class UI;
class RandomNumberGenerator;
class Boid;
class EnergyBar;
class EnergySelectionRing;
class Map;
class Player;

enum class GameState
{
	NONE,
	ATTRACT,
	LOBBY,
	PLAYING,
	VICTORY,
	COUNT
};

enum GameMap
{
	TESTMAP,
	NUM_MAP
};

struct GameTextDisplayConfig
{
	int			m_numMessageOnScreen = 32;
	float		m_lineSpacingMultiplier = 0.1f;
	float		m_titleFontMultipler = 0.06f;
	float		m_middleFontMultipler = 0.045f;
	float		m_smallFontMultiplier = 0.03f;
	float		m_cellAspect = 0.6f;
	BitmapFont* m_font = nullptr;
};

class Game
{
public:
	Game();
	~Game();
	void Startup();
	void Shutdown();
	void ShutDownUIElementList(int arraySize, UI** m_entityArrayPointer);

	void RenderWorldInPlayerCameras();
	void SpawnProps();

	// Game state machine
	void EnterState(GameState state);
	void ExitState(GameState state);
	void Update();
	void Render();
	
	void EnterAttract();
	void EnterLobby();
	void EnterPlaying();
	void EnterVictory();

	void ExitAttract();
	void ExitLobby();
	void ExitPlaying();
	void ExitVictory();

	void UpdateAttract();
	void UpdateLobby();
	void UpdatePlaying();
	void UpdateVictory();

	void RenderAttract();
	void RenderLobby();
	void RenderPlaying();
	void RenderVictory();

	GameState m_currentState = GameState::PLAYING;
	GameTextDisplayConfig m_textConfig;
	// attract mode
	bool   m_permitToStartTimer = false;// when press start, the timer start to count time for transition to game
	Timer* m_transformFromAttractToGameTimer;
	float  m_attractModeTransititionDuration = 1.f;
	float  m_ringThickness = 0.f;

	// map
	std::vector<Map*>	m_allMaps;
	Map* m_currentMap = nullptr;

	void GenerateAllMaps();

	// winning condition
	bool m_playerWins = false;


	// UI functions
	void InitializePlayerLives();
	void InitializeEnergyBar();
	void DrawIconShield(Vec2 centerPos) const;
	void DrawIconVelocity(Vec2 centerPos) const;
	void DrawIconWeapon(Vec2 centerPos) const;
	void RenderUIElements() const;
	void RenderUIElementList(int arraySize, UI* const* m_UIElementArrayPointer) const;
	void UpdateUIElementList(int arraySize, UI* const* m_UIElementArrayPointer, float deltaSeconds) const;
	void UpdateUI();
	void CheckPlayerLives();
	void UpdateEnergyBar(float deltaSeconds);
	void SpawnEnergySelectionRing();
	void DeleteEnergySelectionRing();
	void UpdateEnergySelectionRing(float deltaSeconds);
	EnergyBar* m_energyBar = nullptr;
	EnergySelectionRing* m_energySelectionRing = nullptr;
	bool m_speedBoost		= true;
	bool m_fireRateBoost	= false;
	bool m_shieldGenerate	= false;
	Rgba8 m_ringColor = Rgba8(Rgba8(255, 160, 133, 175));

	Vec2 GetRandomPosInWorld(Vec2 worldSize);

	// camera functions
	void UpdateCameras(float deltaSeconds);
	void TurnCameraShakeOnForPlayerDeath();
	void TurnCameraShakeOnForExplosion();
	float m_screenShakeAmount = 0.f;
	bool  m_cameraShakeIsDisplayed = true;
	Camera m_worldCamera;
	Camera m_sharedScreenCamera;

	bool  m_gameIsOver					 = false;
	float m_returnToStartTimer			 = 0.f;
	bool  m_UIisDisplayed				 = true;
	int	  m_playerLivesNum				 = PLAYER_LIVES_NUM;
	float m_introTimer = 0.f;

	// Player
	std::vector<Player*> m_playersList;
	Player*  m_currentRenderPlayerController = nullptr;
	float m_playerCameraAspectRatio = 4.f;

	void RespawnPlayer();

	void DetectPlayerJoiningAndLeavingLobby();
	bool RegisterPlayer(int playerIndex, int controllerIndex);
	void KickingOutPlayer(int controllerIndex);
	void DetectFirstJoiningPlayerInAttractMode();
	int  GetNumberOfJoiningPlayer() const;
	void RenderPlayerIndexAndInstruction() const;
	void RenderTitleAndStartInstruction() const;

	// update the camera settings for the players
	void UpdatePlayerCamerasSettings();

	std::vector<Entity*> m_entities;

	// lighting controls
	void ControlLightingSettings();

	// sounds
	SoundPlaybackID m_mainMenuPlaybackID = MISSING_SOUND_ID;
	SoundPlaybackID m_gameMusicPlaybackID = MISSING_SOUND_ID;
	SoundPlaybackID m_victoryMusicPlaybackID = MISSING_SOUND_ID;

private:
	bool CheckUIEnabled(UI* const UI) const;
	bool m_OpenScene = true;

	// Collision Detecting

	void UpdateDebugRenderMessages();
};
