#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/core/JobSystem.hpp"
#include "Game/Entity.hpp"
#include "Game/UI.hpp"
#include <deque>

//g_theApp owns(create, manage, and destroy) the g_theRender, but every else class could use the g_render

class Game;
class Renderer;
class Shader;
class Job;

enum SoundEffectID
{
	ATTRACTMODE_BGM,

	BUTTON_CLICK,

	NUM_SOUNDEFFECTS
};

enum TextureID
{
	TESTUV,

	NUM_TEXTURES
};

enum ShaderID
{
	WORLD,
	NUM_SHADERS
};

constexpr int NUM_SQUARE_X = 40;
constexpr int NUM_SQUARE_Y = NUM_SQUARE_X / 2;
constexpr int SQUARES_MAX = 800;
constexpr float SQUARE_SIZE = 3.f;
constexpr float SPACING_X = (HUD_SIZE_X - SQUARE_SIZE * float(NUM_SQUARE_X)) / (NUM_SQUARE_X + 1);
constexpr float SPACING_Y = (HUD_SIZE_Y - SQUARE_SIZE * float(NUM_SQUARE_Y)) / (NUM_SQUARE_Y + 1);

class TestJob : public Job
{
public:
	TestJob()
		: Job()
	{}

	void Execute() override;
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
	Shader* g_shaders[NUM_SHADERS];

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

private:
	void BeginFrame();
	void Update();
	void Render();
	void EndFrame();

	void LoadAudioAssets();
	void LoadTextureAssets();
	void LoadGameConfigXml();
	void LoadGameShader();
	void SetGameConfigByLoadedXml();

	void CheckKeyAndButtonStates( );
	void UpdateGameMode();
	void UpdateAttractMode(float deltaSeconds);
	void RenderAttractMode() const;
	void InitializeAttractMode();
	void RestartGame();
	
	void GenerateAndQueueOneJobForJobSystem();
	void GenerateAndQueueNumJobsForJobSystem(int numJobs);
	void RetrieveOneCompletedJob();
	void RetrieveAllCompletedJobs();

private:
	Vec2   m_shipPos = Vec2(0.f, 50.f); // the start position of the spaceship
	Camera m_attractModeCamera;

	std::vector<Vertex_PCU> m_jobVerts;
	std::deque<TestJob*> m_jobs;
};
