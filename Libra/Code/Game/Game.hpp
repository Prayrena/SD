#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>

class SpriteAnimDefinition;

//todo: render functions based on game state
enum GameState
{
	PLAYING,
	PERISHED,
	VICTORY,
	NUM_GAMESTATE
};

enum HeatMapAnalysis
{
	SOLVABILITY,
	FIRST_LEO_PATHING,
	NUM_HEATMAP
};

enum GameMap
{
	MAP_0,
	MAP_1,
	MAP_2,
	NUM_MAP
};

class App;
class PlayerTank;
class Entity;
class UI;
class RandomNumberGenerator;
class Boid;
class EnergyBar;
class EnergySelectionRing;
class World;
class Map;

class Game
{
public:
	Game();
	~Game();
	void Startup();
	void Update(float deltaSeconds);

	void Render() const;
	void RenderPlayingMode() const;
	void RenderPerishedMode() const;
	void RenderVictoryMode() const;
	void RenderPlayerDeathMenuUI() const;
	void RenderVictoryMenuUI() const;

	void Shutdown();
	void ShutDownUIElementList(int arraySize, UI** m_entityArrayPointer);

	// UI functions
	void InitializePlayerLives();
	void RenderUIElements() const;
	void RenderUIElementList(int arraySize, UI* const* m_UIElementArrayPointer) const;
	void UpdateUIElementList(int arraySize, UI* const* m_UIElementArrayPointer, float deltaSeconds) const;
	void UpdateUI(float deltaSeconds);
	void CheckPlayerLives(float deltaSeconds);
	
	Vec2 GetRandomPosInWorld(Vec2 worldSize);

	// camera functions
	void   UpdateCameras(float deltaSeconds);
	void   TurnCameraShakeOnForPlayerDeath();
	void   TurnCameraShakeOnForExplosion();
	float  m_screenShakeAmount = 0.f;
	bool   m_cameraShakeIsDisplayed = true;
	Camera m_worldCamera;
	Camera m_screenCamera;
	AABB2  followingCamera;

	// Map and world setting
	void GenerateAllMaps();
	void SwitchBetweenMaps();
	void TeleportPlayersBetweenDifferentMaps();

	Map*		m_currentMap = nullptr;
	int			m_currentMapIndex = 0;
	int			m_nextMapIndex = 0;
	GameState	m_gameState = PLAYING;
	std::vector<Map*>	m_allMaps;

	float m_transitToPerishTimer		 = 0.f;
	bool  m_UIisDisplayed				 = true;
	bool  m_debugCamerIsOn				 = false;
	int	  m_playerLivesNum				 = PLAYER_LIVES_NUM;
	float m_introTimer = 0.f;

	bool			m_showHeatMap = false;
	int				m_heatmapIndex = -1;
	HeatMapAnalysis m_heatMapState = SOLVABILITY;

	// Collision Detecting
	bool DoEntitiesOverlap(Entity const& a, Entity const& b);

	// player management
	void RespawnPlayer();
	bool FindPlayerReferenceOnCurrentMap(Entity*& playerPtr);
	void CheckIfPlayerHasGotToExit();

	PlayerTank* m_playerTank = nullptr;// in order to let game's owned class use player ship's info
	Vec2		m_playerCheckPoint;
	float		m_playerCheckPointOrientation;

	// audio management
	bool m_defeatSoundIsPlayed = false;
	bool m_victorySoundIsPlayed = false;

	// animation
	void DefineAnimation();

	SpriteAnimDefinition* m_tankExplosionAnim = nullptr;
	SpriteAnimDefinition* m_bulletExplosionAnim = nullptr;
	SpriteAnimDefinition* m_muzzleFlashAnim = nullptr;

private:
	bool CheckUIEnabled(UI* const UI) const;
	bool m_OpenScene = true;
};
