#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>
#include <string>
#include <map>

class	App;
class	Entity;
class	UI;
class	RandomNumberGenerator;
class	Boid;
class	EnergyBar;
class	EnergySelectionRing;
class	Model;
struct	PhongLightingConstants;
class   Timer;
class	Crawler;
class	Player;

enum TestingScene
{
	EMISSIVE_CUBES,
	TEXTURED_PROPS,
	TUTORIAL_BOX,
	HADRIAN_TANK,

	NUM_TESTSCENE
};

class Game
{
public:
	Game();
	~Game();
	void Startup();
	void Update();
	void Render();//mark for that the render is not going to change the variables
	void Shutdown();

	void RenderWorldInPlayerCamera();
	void SpawnProps();

	// viewer control
	void UpdateInput();
	bool LoadSelectedFile();
	void GetWindowsLastErrorAndDisplayOnScreen();
	void LoadNewModel();
	void ControlTextureMapDebug();

	std::vector<Model*> m_loadedModels;
	float m_debugMsgFontSize = 30.f;

	// Lighting control
	void ControlLightingSettings();
	PhongLightingConstants* m_phongLighinting = nullptr;

	// UI functions
	void RenderHUD() const;

	Vec2 GetRandomPosInWorld(Vec2 worldSize);

	// camera functions
	void UpdateCameras(float deltaSeconds);

	Camera m_worldCamera;
	Camera m_screenCamera;

	bool  m_gameIsOver					 = false;
	float m_returnToStartTimer			 = 0.f;
	bool  m_UIisDisplayed				 = true;
	int	  m_playerLivesNum				 = PLAYER_LIVES_NUM;
	float m_introTimer = 0.f;

	// Entity children special power
	void RespawnPlayer();

	Player* m_player;

	std::map<std::string, Entity*> m_entities;

	std::vector<Entity*> m_testSceneProps;

	std::string m_modelFilePath;
	std::string m_appFilePath;

	// scene management
	void ClearTestScene();
	void DeleteCurrentTestMode_And_CreateNewTestMode(TestingScene type);

	int	 m_testSceneIndex = 0;

	// debug
	void ToggleToShowDebugVertexes();
	void ToggleToRotateModelAndProp();

	bool m_debugMode = false;
	bool m_rotationMode = false;

	std::vector<Crawler*> m_crawlers;

private:
	bool CheckUIEnabled(UI* const UI) const;
	bool m_OpenScene = true;

	// Collision Detecting
	bool DoEntitiesOverlap(Entity const& a, Entity const& b);

	void UpdateDebugRenderMessages();
	Timer* m_FPSTimer = nullptr;
	std::string m_FPSString;
};
