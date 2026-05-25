#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Camera.hpp"

class App;
class PlayerShip;
class Beetle;
class Bullet;
class Asteroid;
class Entity;
class UIElement;
class Debris;
class RandomNumberGenerator;
class Boid;
class EnergyBar;
class EnergySelectionRing;

class Game
{
public:
	Game();
	~Game();
	void Startup();
	void Update();
	void Render() const;//mark for that the render is not going to change the variables
	void Shutdown();
	void ShutDownEntityList(int arraySize, Entity** m_entityArrayPointer);
	void ShutDownUIElementList(int arraySize, UIElement** m_entityArrayPointer);

	// UI functions
	void InitializePlayerLives();
	void InitializeEnergyBar();
	void DrawIconShield(Vec2 centerPos) const;
	void DrawIconVelocity(Vec2 centerPos) const;
	void DrawIconWeapon(Vec2 centerPos) const;
	void RenderUIElements() const;
	void RenderUIElementList(int arraySize, UIElement* const* m_UIElementArrayPointer) const;
	void UpdateUIElementList(int arraySize, UIElement* const* m_UIElementArrayPointer, float deltaSeconds) const;
	void UpdateUI(float deltaSeconds);
	void CheckPlayerLives(float deltaSeconds);

	// Energy game design
	void UpdateEnergyBar(float deltaSeconds);
	void SpawnEnergySelectionRing();
	void DeleteEnergySelectionRing();
	void UpdateEnergySelectionRing(float deltaSeconds);
	EnergyBar* m_energyBar = nullptr;
	EnergySelectionRing* m_energySelectionRing = nullptr;
	bool m_speedBoost		= true;
	bool m_fireRateBoost	= false;
	bool m_shieldGenerate	= false;

	void CheckIfLevelIsClear(float deltaSeconds);
	bool CheckEntityListIsClear(int arraySize, Entity** entityList);
	bool m_currentLevelIsClear = false;
	float m_introTimer = TIME_INTRO_ZOOMOUT;

	void InitializeStarField();
	void UpdateStarField(float deltaSeconds);
	void RenderStarField() const;
	Vec2 GetRandomPosInWorld(Vec2 worldSize);
	UIElement* m_starField_Near[UI_STARFIELD_NEAR_NUM] = {};
	UIElement* m_starField_Far[UI_STARFIELD_FAR_NUM] = {};

	// camera functions
	void UpdateCameras(float deltaSeconds);
	void TurnCameraShakeOnForPlayerDeath();
	void TurnCameraShakeOnForExplosion();
	float m_screenShakeAmount = 0.f;
	bool  m_cameraShakeIsDisplayed = true;
	Camera m_worldCamera;
	Camera m_screenCamera;
	UIElement* m_brightUp;

	bool  m_gameIsOver					 = false;
	float m_returnToStartTimer			 = 0.f;
	bool  m_UIisDisplayed				 = true;
	int	  m_playerLivesNum				 = PLAYER_LIVES_NUM;

	UIElement* m_UI_lives[PLAYER_LIVES_NUM] = {};

	// Entity children special power
	void RespawnPlayer();
	void SpawnRandomAsteroid( );
	void SpawnRandomWasp( );
	void SpawnRandomBoid();
	void SpawnRandomBeetle();
	void SpawnBullet(Vec2 const& pos, float& forwardDegrees);
	void SpawnDebris(Entity* EntityPtr, float deflectRotationDegrees, int debris_min_num, int debris_max_num, Rgba8 debrisColor);
	//void UpdateBoidController(float deltaSeconds);

	PlayerShip* m_playerShip = nullptr;// in order to let game's owned class use player ship's info

	int m_asteroidWave[30] = { ASTEROID_WAVE_1, ASTEROID_WAVE_2, ASTEROID_WAVE_3, ASTEROID_WAVE_4, ASTEROID_WAVE_5 };
	int m_waspWave[30] = { WASP_WAVE_1, WASP_WAVE_2, WASP_WAVE_3, WASP_WAVE_4, WASP_WAVE_5 };
	int m_beetleWave[30] = { BEETLE_WAVE_1, BEETLE_WAVE_2, BEETLE_WAVE_3, BEETLE_WAVE_4, BEETLE_WAVE_5 };
	int m_boidWave[MAX_BOIDS] = { BOID_WAVE_1, BOID_WAVE_2, BOID_WAVE_3, BOID_WAVE_4, BOID_WAVE_5 };

	float m_boidLaunchingTimer = 0.f;

	// flocking simulation for boids
	// void AlignBoids(float deltaSeconds);
	// void CohereBoids(float deltaSeconds);
	// void SeperateBoids(float deltaSeconds);
	Vec2 GetCenterPositionofFriendsInRange(Vec2 referencePosition);
	Vec2 GetAverageFriendVelocityInRange(Vec2 referencePosition, Vec2 normal);
	Boid* GetTheNearestBoid(Vec2 referencePosition);

private:
	// collision detections
	void CheckEntityCollision();
	void CheckEntityListCollision(int A_size, Entity** A, int B_size, Entity** B);
	void CheckEntityListWithPlayerShipCollision(int A_size, Entity** A);

	void CheckBulletsVSAsteroids();
	void CheckBulletsVSBeetles();
	void CheckBulletsVSWasps();
	void CheckBulletsVSBoids();

	void CheckAsteroidsVSShips();
	void CheckBeetlesVSShips();
	void CheckWaspsVSShips();
	void CheckBoidsVSShips();

	bool CheckEntityIsAlive(Entity* const entity) const;
	bool CheckUIElementEnabled(UIElement* const UIElement) const;

	// random spawning off screen function
	Vec2 EnemySpawnOffScreenPos(float radius);
	Vec2 EntitySpawnOffScreenPos(float radius);

	// Processing entities
	void UpdateEntities(float deltaSeconds);
	void UpdateEntityList(int arraySize, Entity** m_entityArrayPointer, float deltaSeconds);

	// level progression
	void spawnNewLevelEntities(float deltaSeconds);
	void SpawnAsteroidForLevel(int levelIndex);
	void SpawnWaspForLevel	  (int levelIndex);
	void SpawnBoidsForLevel(int levelIndex);
	void SpawnBeetleForLevel(int levelIndex);
	void ClearLevelEntityForTesting();
	int m_currentLevelIndex = 1;

	// Cleaning functions
	void DeleteGarbageEntities();
	void DeleteGarbageEntityList(int arraySize, Entity** m_entityArrayPointer);

	// Render functions
	void RenderEntities() const;
	void RenderEntityList(int arraySize, Entity* const* m_entityArrayPointer) const;
	void DebugRenderEntities() const;
	void DebugRenderEntityList(int arraySize, Entity* const* m_entityArrayPointer) const;

	//Entities Management
	Entity*  m_bullets[MAX_BULLETS]			= {};
	Entity*  m_debris[MAX_DEBRIS_NUMBERS]	= {};
	Entity*  m_asteroids[MAX_ASTEROIDS]		= {};// array of Asteroid pointers, save memory and easy management for asteroid
	Entity*  m_beetles[MAX_BEETLES]			= {};
	Entity*  m_wasps[MAX_WASPS]				= {};
	Entity*  m_boids[MAX_BOIDS]				= {};

	bool m_OpenScene = true;

	// Collision Detecting
	bool DoEntitiesOverlap(Entity const& a, Entity const& b);
	void RenderEnergyBoosterStatus() const;

};
