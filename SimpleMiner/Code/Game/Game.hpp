#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>
#include <string>

class App;
class Entity;
class UI;
class RandomNumberGenerator;
class Boid;
class EnergyBar;
class EnergySelectionRing;
class Player;

class Game
{
public:
	Game();
	~Game();
	void Startup();
	void Update();
	void Render();//mark for that the render is not going to change the variables
	void Shutdown();
	void ShutDownUIElementList(int arraySize, UI** m_entityArrayPointer);

	void RenderWorldInPlayerCamera();
	void SpawnProps();

	// UI functions
	void RenderUIElements() const;
	void RenderUIElementList(int arraySize, UI* const* m_UIElementArrayPointer) const;
	void UpdateUIElementList(int arraySize, UI* const* m_UIElementArrayPointer, float deltaSeconds) const;
	void UpdateUI();

	Vec2 GetRandomPosInWorld(Vec2 worldSize);

	// camera functions
	void UpdateCameras(float deltaSeconds);
	void TurnCameraShakeOnForPlayerDeath();
	void TurnCameraShakeOnForExplosion();
	float m_screenShakeAmount = 0.f;
	bool  m_cameraShakeIsDisplayed = true;
	Camera m_worldCamera;
	Camera m_screenCamera;

	bool  m_gameIsOver					 = false;
	float m_returnToStartTimer			 = 0.f;
	bool  m_UIisDisplayed				 = true;
	int	  m_playerLivesNum				 = PLAYER_LIVES_NUM;
	float m_introTimer = 0.f;

	// Entity children special power
	void RespawnPlayer();

	Player* m_player = nullptr;

	std::vector<Entity*> m_entities;

private:
	bool CheckUIEnabled(UI* const UI) const;
	bool m_OpenScene = true;

	// Collision Detecting
	bool DoEntitiesOverlap(Entity const& a, Entity const& b);

	void UpdateDebugRenderMessages();
};
////
//std::atomic<int> g_x = 0; // happen one at a time
//
//// use them together
//int g_x = 0;
//std::mutex<int> g_x_mutex;
//
//void ThreadMain()
//{
//	for (int i = 0; i < 1'000'000; ++i)
//	{
//		//g_x_mutex.lock(); // asking to get the pumpkin
//		//++g_x;
//		//g_x_mutex.unlock(); // returning the pumpkin
//
//		// if thread B is checking the condition, A might checking the condition or ++g_x, therefore is uncertain?
//		if (g_x > 10)
//		{
//			++g_x;
//		}
//	}
//}