#include "Engine/Math/Vec3.hpp"
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Math/OpenXRMathUtils.hpp"
#include "Engine/Math/PhysXMathUtils.hpp"
#include "Engine/Physics/ThePhysX.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/UI/HealthBar.hpp"
#include "Game/Game.hpp"
#include "Game/HUD.hpp"
#include "Game/VRPlayer.hpp"
#include "Game/WinPlayer.hpp"
#include "Game/Crawler.hpp"	
#include "Game/openxr_program.h"

static const int k_lightingFogConstantsSlot = 8;

Clock* g_theGameClock = nullptr;
RandomNumberGenerator* g_rng = nullptr;
HUD* g_theHUD = nullptr;
VRPlayer* g_theVRPlayer = nullptr;
WinPlayer* g_theWinPlayer = nullptr;

extern BitmapFont*		g_consoleFont;
extern Renderer*		g_theRenderer;
extern AudioSystem*		g_theAudio;
extern OpenXrProgram*	g_theApp;
extern ThePhysX*		g_thePhysX;
extern InputSystem*		g_theInput;

using namespace DirectX;
using namespace physx;

constexpr float ONE_NIGHT_FIVE_BEATS = (1.f / (95.f * 0.696f));
constexpr float ONE_ONE_THIRTYFIVE_BEATS = (1.f / (135.f * 0.696f));
constexpr float ONE_TWO_ZEROFIVE_BEATS = (1.f / (205.f * 0.696f));
constexpr float ONE_TWO_ONEONE_BEATS = (1.f / (211.f * 0.696f));

Game::Game()
{

}

Game::~Game()
{

}

void Game::Startup()
{
	LoadAudioAssets();

	g_rng = new RandomNumberGenerator();

	g_theGameClock = new Clock();

	g_theVRPlayer = new VRPlayer();
	g_theVRPlayer->Startup();


	g_theWinPlayer = new WinPlayer();
	g_theWinPlayer->Startup();

	LoadingCrawlerModelParts();
	// m_crawlerTemplate = new Crawler(Vec3(99.f, 0.f, 0.f));
	// m_crawlerTemplate->Startup();

	g_theHUD = new HUD();

	m_grabToStartTimer = new Timer(DURATION_GRAB_TO_START);

	m_recenterTimer = new Timer(5.f);
	m_recenterTimer->Start();

	AddVertsForWorldAxis();
	AddVertsForGround();

	g_thePhysX->m_scene->setSimulationEventCallback(this);

	m_fog_CBO = g_theRenderer->CreateConstantBuffer(sizeof(FogGPUData));
	m_gpuShaderData = new FogGPUData;
	m_gpuShaderData->m_fogColor = Vec4(m_noonSkyFogColor);
}

void Game::LoadingCrawlerModelParts()
{
	// load models
	m_footModel = new ObjModel();
	m_footModel->LoadXml("Data/Models/Crawler_Foot.xml");
	m_footModel->m_color = Rgba8::WHITE;

	m_legModel = new ObjModel();
	m_legModel->LoadXml("Data/Models/Crawler_Leg.xml");
	m_legModel->m_color = Rgba8::WHITE;

	m_baseModel = new ObjModel();
	m_baseModel->LoadXml("Data/Models/Crawler_Base.xml");
	m_baseModel->m_color = Rgba8::WHITE;

	m_headModel = new ObjModel();
	m_headModel->LoadXml("Data/Models/Crawler_Head.xml");
	m_headModel->m_color = Rgba8::WHITE;

	m_gunModel = new ObjModel();
	m_gunModel->LoadXml("Data/Models/Crawler_Gun.xml");
	m_gunModel->m_color = Rgba8::WHITE;
}

void Game::UpdateAllCrawlersHealthBarTrackingMat(Mat44 const& trackingMat)
{
	for (auto crawler : m_crawlers)
	{
		if (crawler->m_healthBar)
		{
			crawler->m_healthBar->Update(trackingMat);
		}
	}
}

void Game::Update()
{
	// if (g_theInput->WasKeyJustPressed(' '))
	// {
	// 	// PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
	// 	// dynamicActor->addForce(PxVec3(0, 0, 10), PxForceMode::eIMPULSE);
	// 	// WasHitByPlayer(Vec3(), Vec3(), 0.1f);
	// }	
	
	if (g_theInput->WasKeyJustPressed('O'))
	{
		g_theGameClock->StepSingleFrame();
	}

	if (g_theInput->WasKeyJustPressed('R'))
	{
		g_thePhysX->m_PhysXConfig.m_debugRender = !g_thePhysX->m_PhysXConfig.m_debugRender;
	}	
	
	if (g_theInput->WasKeyJustPressed('L'))
	{
		m_debugSkeleton = !m_debugSkeleton;

		if (m_debugSkeleton)
		{
			g_theVRPlayer->m_skeleton->m_drawSkeletonDebug = true;

			for (auto crawler : m_crawlers)
			{
				crawler->m_skeleton->m_drawSkeletonDebug = true;
			}
		}
		else
		{
			g_theVRPlayer->m_skeleton->m_drawSkeletonDebug = false;

			for (auto crawler : m_crawlers)
			{
				crawler->m_skeleton->m_drawSkeletonDebug = false;
			}
		}
	}

	if (g_theInput->WasKeyJustPressed('M'))
	{
		if (m_numCrawlers < 3)
		{
			++m_numCrawlers;
		}
	}
	if (g_theInput->WasKeyJustPressed('N'))
	{
		if (m_numCrawlers > 0)
		{
			--m_numCrawlers;
		}
	}

	switch (m_currentState)
	{
	case GameState::ATTRACT:
	{
		UpdateAttract();
	}break;
	case GameState::LOBBY:
	{
		UpdateLobby();
	}break;
	case GameState::PLAYING:
	{
		UpdatePlaying();
	}break;
	case GameState::SHOWSCORES:
	{
		UpdateShowScores();
	}break;
	}
}

void Game::Render() const
{
	switch (m_currentState)
	{
	case GameState::ATTRACT:
	{
		RenderAttract();
	}break;
	case GameState::LOBBY:
	{
		RenderLobby();
	}break;
	case GameState::PLAYING:
	{
		RenderPlaying();
	}break;
	case GameState::SHOWSCORES:
	{
		RenderShowScores();
	}break;
	}
}

void Game::Shutdown()
{
	if (m_axisVertexBuffer)
	{
		delete m_axisVertexBuffer;
	}
	if (m_groundVertexBuffer)
	{
		delete m_groundVertexBuffer;
	}

	if (m_footModel) {
		delete m_footModel;
		m_footModel = nullptr;
	}
	if (m_legModel) {
		delete m_legModel;
		m_legModel = nullptr;
	}
	if (m_baseModel) {
		delete m_baseModel;
		m_baseModel = nullptr;
	}
	if (m_headModel) {
		delete m_headModel;
		m_headModel = nullptr;
	}
	if (m_gunModel) {
		delete m_gunModel;
		m_gunModel = nullptr;
	}

	if (m_grabToStartTimer)
	{
		delete m_grabToStartTimer;
		m_grabToStartTimer = nullptr;
	}

	if (g_theVRPlayer)
	{
		delete g_theVRPlayer;
		g_theVRPlayer = nullptr;
	}

	if (g_theWinPlayer)
	{
		delete g_theWinPlayer;
		g_theWinPlayer = nullptr;
	}

	for (Crawler* c : m_crawlers)
	{
		delete c;
	}
}

void Game::SetModelMatrix() const
{

}

void Game::LoadAudioAssets()
{
	m_soundEffectsID[int(SoundEffectID::GAMEMUSIC)] = g_theAudio->CreateOrGetSound("Data/Audio/Signals_LazerBoomerang.mp3");
	m_soundEffectsID[int(SoundEffectID::SCORESMUSIC)] = g_theAudio->CreateOrGetSound("Data/Audio/SClass_ZZZ.mp3");
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// State machine
void Game::EnterState(GameState state)
{
	// start new music
	switch (state)
	{
	case GameState::ATTRACT: // Draw the game start instruction on HUD
	{
		EnterAttract();
	}break;
	case GameState::LOBBY: // draw count down on the HUD
	{
		EnterLobby();
	}break;
	case GameState::PLAYING:
	{
		EnterPlaying();
	}break;	
	case GameState::SHOWSCORES: // show the final score and restart instruction
	{
		EnterShowScores();
	}break;
	}
}

void Game::ExitState(GameState state)
{
	// start new music
	switch (state)
	{
	case GameState::ATTRACT: // Draw the game start instruction on HUD
	{
		ExitAttract();
	}break;
	case GameState::LOBBY: // draw count down on the HUD
	{
		ExitLobby();
	}break;
	case GameState::PLAYING:
	{
		ExitPlaying();
	}break;
	case GameState::SHOWSCORES: // show the final score and restart instruction
	{
		ExitShowScores();
	}break;
	}
}

void Game::EnterAttract()
{
	m_currentScores = 0;
	m_activatedCubesCounter = 0;

	g_theRenderer->SetClearScreenColor(Rgba8::BLACK);
}

void Game::EnterLobby()
{
	// m_gameMusic = g_theAudio->StartSound(m_soundEffectsID[int(SoundEffectID::GAMEMUSIC)], false, 1.f);
	// Crawler* newCrawler = m_crawlerTemplate->CloneThisCrawler();
	// Crawler* newCrawler = new Crawler(Vec3(6.f, 4.f, 0.f), EulerAngles(150.f, 0.f, 0.f));
	// m_crawlers.push_back(newCrawler);

	g_theRenderer->SetClearScreenColor(Rgba8::BLUE_LIGHT);

	// Crawler* newCrawler2 = new Crawler(Vec3(6.f, -4.f, 0.f), EulerAngles(150.f, 0.f, 0.f));
	// m_crawlers.push_back(newCrawler2);
	// newCrawler2 = new Crawler(Vec3(9.f, 4.f, 0.f), EulerAngles(150.f, 0.f, 0.f));
	// m_crawlers.push_back(newCrawler2);
	// newCrawler2 = new Crawler(Vec3(12.f, -4.f, 0.f), EulerAngles(150.f, 0.f, 0.f));
	// m_crawlers.push_back(newCrawler2);
	// newCrawler2 = new Crawler(Vec3(15.f, -4.f, 0.f), EulerAngles(150.f, 0.f, 0.f));
	// m_crawlers.push_back(newCrawler2);
	// newCrawler2 = new Crawler(Vec3(18.f, -4.f, 0.f), EulerAngles(150.f, 0.f, 0.f));
	// m_crawlers.push_back(newCrawler2);

	for (auto crawler : m_crawlers)
	{
		crawler->Startup();
	}

	m_currentState = GameState::LOBBY;

	g_theRenderer->SetPhongLightingConstants(*m_phongLighinting);
}

void Game::EnterPlaying()
{
}

void Game::EnterShowScores()
{
	m_scoresMusic = g_theAudio->StartSound(m_soundEffectsID[int(SoundEffectID::SCORESMUSIC)], false, 1.f);
	g_theRenderer->SetClearScreenColor(Rgba8::BLACK);
}

void Game::ExitAttract()
{

}

void Game::ExitLobby()
{

}

void Game::ExitPlaying()
{
	g_theAudio->SetSoundPlaybackSpeed(m_gameMusic, 0.f);
}

void Game::ExitShowScores()
{
	g_theAudio->SetSoundPlaybackSpeed(m_scoresMusic, 0.f);
}

void Game::UpdateAttract()
{
	// both hands to need to grab and hold over 50% for a period of time to start the game
	//if (g_theVRPlayer->m_hands[Side::LEFT].m_grabValue < 0.5f || g_theVRPlayer->m_hands[Side::LEFT].m_grabValue < 0.5f)
	//{
	//	m_grabToStartTimer->Restart();
	//}
	//else
	//{
	//	if (m_grabToStartTimer->IsStopped())
	//	{
	//		m_grabToStartTimer->Start();
	//	}
	//	else
	//	{
	//		if (m_grabToStartTimer->HasPeroidElapsed())
	//		{
	//			m_grabToStartTimer->Stop();

	//			m_currentState = GameState::LOBBY;
	//			ExitState(GameState::ATTRACT);
	//			EnterState(GameState::LOBBY);
	//		}
	//	}
	//}
	 
	if (m_recenterTimer->HasPeroidElapsed() && g_theApp->m_openXRAvaible)
	{
		g_theApp->RecenterPlayer();
		m_recenterTimer->Stop();
	}
}

//Vec3 Game::GetElbowPoleVectorPosByInputHand(Vec3 const& handWorldpos)
//{
//	Joint* shoulderJoint = m_armSkeleton->GetJointByName("upperArm_r");
//	Vec3 shoulderPos = shoulderJoint->m_worldPos;
//	float boneLength = shoulderJoint->m_boneLength;
//
//	float height = (handWorldpos - shoulderPos).z;
//	if ()
//	{
//	}
//}

void Game::UpdateLobby()
{
	g_theVRPlayer->Update();

#ifndef SHIPPING
	g_theWinPlayer->Update();
#endif
	int numMech = 0;
	if (!m_crawlers.empty())
	{
		for (auto it = m_crawlers.begin(); it != m_crawlers.end(); )
		{
			Crawler* crawler = *it;
			if (crawler == nullptr)
			{
				it = m_crawlers.erase(it); // Remove nullptr
				continue;
			}

			if (crawler->m_deathClearTimer->HasPeroidElapsed())
			{
				delete crawler;
				crawler = nullptr;
				it = m_crawlers.erase(it);   // Remove from vector and dont advance the it
			}
			else if(crawler)
			{
				crawler->Update();
				++it;					// Only increment if this crawl is still alive
				++numMech;
			}
			else if (crawler && crawler->m_isDead)
			{
				++it;
				++numMech;
			}
		}
	}

	if (numMech > m_numCrawlers) // 3, 1
	{
		for (int i = m_numCrawlers; i < numMech; ++i) // 1, 2
		{
			m_crawlers[i]->m_isDead = true;
		}
	}

	while (numMech < m_numCrawlers)
	{
		float pos_x = g_rng->RollRandomFloatInRange(6.f, 9.f);
		float pos_y = g_rng->RollRandomFloatInRange(-4.f, 4.f);
		Vec3 birthPosition(pos_x, pos_y, 0.f);

		Mat44 rotationMat = g_theVRPlayer->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
		birthPosition = g_theVRPlayer->m_position + rotationMat.TransformPosition3D(birthPosition);

		Crawler* newCrawler = new Crawler(birthPosition, EulerAngles(150.f, 0.f, 0.f));
		newCrawler->Startup();
		m_crawlers.push_back(newCrawler);
		++numMech;
	}
}

void Game::UpdatePlaying()
{
}

void Game::UpdateShowScores()
{
	// if player grab both hands to start again, we'll renter the playing state
	if (g_theVRPlayer->m_hands[Side::LEFT].m_grabValue < 0.5f || g_theVRPlayer->m_hands[Side::LEFT].m_grabValue < 0.5f)
	{
		m_grabToStartTimer->Restart();
	}
	else
	{
		if (m_grabToStartTimer->IsStopped())
		{
			m_grabToStartTimer->Start();
		}
		else
		{
			if (m_grabToStartTimer->HasPeroidElapsed())
			{
				m_grabToStartTimer->Stop();

				ExitState(GameState::SHOWSCORES);
				m_currentState = GameState::ATTRACT;
				EnterState(GameState::ATTRACT);
			}
		}
	}
}

void Game::RenderAttract() const
{
	g_theHUD->RenderCountDownToStartTutorial();

	g_theVRPlayer->RenderHands();
}

void Game::RenderLobby() const
{
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	BindFogShaderData();
	g_theRenderer->BindShader(g_theApp->m_shaders[WORLD]);
	g_theRenderer->DrawVertexBuffer(m_groundVertexBuffer, (int)m_groundVerts.size());
	g_theRenderer->DrawVertexBuffer(m_axisVertexBuffer, (int)m_axisVerts.size());

	g_theVRPlayer->Render();

	if (!m_crawlers.empty())
	{
		for (auto crawler : m_crawlers)
		{
			if (crawler)
			{
				crawler->Render();
			}
		}
	}
}

void Game::RenderPlaying() const
{
	SetModelMatrix();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->DrawVertexArray((int)(m_axisVerts.size()), m_axisVerts.data());

	g_theRenderer->BindTexture(&g_consoleFont->GetTexture());
	g_theRenderer->DrawVertexArray((int)m_scoreVerts.size(), m_scoreVerts.data());
}

void Game::RenderShowScores() const
{
	SetModelMatrix();
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->DrawVertexArray((int)(m_axisVerts.size()), m_axisVerts.data());

	g_theRenderer->BindTexture(&g_consoleFont->GetTexture());
	g_theRenderer->DrawVertexArray((int)m_scoreVerts.size(), m_scoreVerts.data());

}

void Game::AddVertsForWorldAxis()
{
	float spacingXYZ = 1.f;
	int numOfXYZ = (int)(100.f / spacingXYZ) + 1;
	float dimensionRed = 0.025f;
	float dimensionGreen = 0.025f;
	// int numOfGrid = 1000 + 1;
	// float dimensionGray = 0.01f;
	// // gray grid
	// for (int i = 0; i < numOfGrid; ++i)
	// {
	// 	AABB3 pipe(Vec3(-50.f + (float)i - (dimensionGray * 0.5f), -50.f, -(dimensionGray * 0.5f)),
	// 		Vec3(-50.f + (float)i + (dimensionGray * 0.5f), 50.f, (dimensionGray * 0.5f)));
	// 	AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	// }
	// for (int i = 0; i < numOfGrid; ++i)
	// {
	// 	AABB3 pipe(Vec3(-50.f, -50.f + (float)i - (dimensionGray * 0.5f), -(dimensionGray * 0.5f)),
	// 		Vec3(50.f, -50.f + (float)i + (dimensionGray * 0.5f), (dimensionGray * 0.5f)));
	// 	AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	// }
	// // GREEN lane
	// for (int i = 0; i < numOfXYZ; ++i)
	// {
	// 	if (i == (numOfXYZ / 2))
	// 	{
	// 		AABB3 pipe(Vec3(-50.f + (float)i * spacingXYZ - (dimensionGreen * 1.2f), -50.f, -(dimensionGreen * 2.f)),
	// 			Vec3(-50.f + (float)i * spacingXYZ + (dimensionGreen * 2.f), 50.f, (dimensionGreen * 2.f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
	// 	}
	// 	else
	// 	{
	// 		AABB3 pipe(Vec3(-50.f + (float)i * spacingXYZ - (dimensionGreen * 0.5f), -50.f, -(dimensionGreen * 0.5f)),
	// 			Vec3(-50.f + (float)i * spacingXYZ + (dimensionGreen * 0.5f), 50.f, (dimensionGreen * 0.5f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
	// 	}
	// }
	// // RED lane
	// for (int i = 0; i < numOfXYZ; ++i)
	// {
	// 	if (i == (numOfXYZ / 2))
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 2.f), (dimensionRed * 2.f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// 	else
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// }
	// // BLUE lane
	// for (int i = 0; i < numOfXYZ; ++i)
	// {
	// 	if (i == (numOfXYZ / 2))
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 2.f), (dimensionRed * 2.f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// 	else
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// }

	// gray grid
	// for (int i = 0; i < numOfGrid; ++i)
	// {
	// 	AABB3 pipe(Vec3(-50.f + (float)i * 0.1f - (dimensionGray * 0.5f), -50.f, -(dimensionGray * 0.5f)),
	// 		Vec3(-50.f + (float)i * 0.1f + (dimensionGray * 0.5f), 50.f, (dimensionGray * 0.5f)));
	// 	AddVertsForAABB3D(m_axisVerts, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	// }
	// for (int i = 0; i < numOfGrid; ++i)
	// {
	// 	AABB3 pipe(Vec3(-50.f, -50.f + (float)i * 0.1f - (dimensionGray * 0.5f), -(dimensionGray * 0.5f)),
	// 		Vec3(50.f, -50.f + (float)i * 0.1f + (dimensionGray * 0.5f), (dimensionGray * 0.5f)));
	// 	AddVertsForAABB3D(m_axisVerts, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	// }
	// BLUE line
	for (int i = 0; i < numOfXYZ; ++i)
	{
		// if (i == (numOfXYZ / 2))
		// {
		// 	AABB3 pipe(Vec3(-50.f + (float)i * spacingXYZ - (dimensionGreen * 1.2f), -50.f, -(dimensionGreen * 2.f)),
		// 		Vec3(-50.f + (float)i * spacingXYZ + (dimensionGreen * 2.f), 50.f, (dimensionGreen * 2.f)));
		// 	AddVertsForAABB3D(m_axisVerts, pipe, Rgba8::WHITE, AABB2::ZERO_TO_ONE);
		// }
		// else
		// {
		// }
		AABB3 pipe(Vec3(-50.f + (float)i * spacingXYZ - (dimensionGreen * 0.5f), -50.f, -(dimensionGreen * 0.5f)),
			Vec3(-50.f + (float)i * spacingXYZ + (dimensionGreen * 0.5f), 50.f, (dimensionGreen * 0.5f)));
		AddVertsForAABB3D(m_axisVerts, pipe, Rgba8::WHITE, AABB2::ZERO_TO_ONE);
	}
	// PINK line
	for (int i = 0; i < numOfXYZ; ++i)
	{
		// if (i == (numOfXYZ / 2))
		// {
		// 	AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
		// 		Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 2.f), (dimensionRed * 2.f)));
		// 	AddVertsForAABB3D(m_axisVerts, pipe, Rgba8::WHITE, AABB2::ZERO_TO_ONE);
		// }
		// else
		// {
		// }
			AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
				Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
			AddVertsForAABB3D(m_axisVerts, pipe, Rgba8::WHITE, AABB2::ZERO_TO_ONE);
	}

	AABB3 pipeY(Vec3(-50.f, -dimensionRed * 2.f, -dimensionRed * 1.5f),
				Vec3(50.f, dimensionRed * 2.f, dimensionRed * 1.5f ));
	AddVertsForAABB3D(m_axisVerts, pipeY, Rgba8::BLUSH_PINK, AABB2::ZERO_TO_ONE);

	AABB3 pipeX(
		Vec3((dimensionGreen * 1.5f), -50.f, (dimensionGreen * 1.5f)), Vec3(- (dimensionGreen * 1.5f), 50.f, -(dimensionGreen * 1.5f)) );
	AddVertsForAABB3D(m_axisVerts, pipeX, Rgba8::NEON_GREEN, AABB2::ZERO_TO_ONE);


	// BLUE lane
	// for (int i = 0; i < numOfXYZ; ++i)
	// {
	// 	if (i == (numOfXYZ / 2))
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 2.f), (dimensionRed * 2.f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// 	else
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// }

	AddVertsForArrow3D(m_axisVerts, Vec3(), Vec3(1.f, 0.f, 0.f), 0.045f, Rgba8::RED, Rgba8::RED);
	AddVertsForArrow3D(m_axisVerts, Vec3(), Vec3(0.f, 1.f, 0.f), 0.045f, Rgba8::GREEN, Rgba8::GREEN);
	AddVertsForArrow3D(m_axisVerts, Vec3(), Vec3(0.f, 0.f, 1.f), 0.045f, Rgba8::BLUE, Rgba8::BLUE);

	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArrayDataSize = (m_axisVerts.size()) * vertexSize;
	m_axisVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_axisVerts.size()), vertexSize);
	g_theRenderer->CopyCPUToGPU(m_axisVerts.data(), vertexArrayDataSize, m_axisVertexBuffer);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// arrow for the debugging of the sun
	m_phongLighinting = new PhongLightingConstants();

	m_sun = new Actor();
	m_sun->m_name = "Sun";
	m_sun->m_position = Vec3(0.f, 0.f, 2.5f);
	m_sun->m_orientation = EulerAngles(35.f, 45.f, 0.f);
	AddVertsForArrow3D(m_sun->m_unlitVertexes, Vec3(), Vec3(1.f, 0.f, 0.f), 0.15f, Rgba8::YELLOW_TRANSPARENT, Rgba8::YELLOW_TRANSPARENT);

	m_phongLighinting->SunIntensity = 0.9f;
	m_phongLighinting->SunDirection = m_sun->m_orientation.GetForwardIBasis().GetNormalized();
	m_phongLighinting->AmbientIntensity = 1.f - m_phongLighinting->SunIntensity;

	g_theRenderer->m_sunDirection = m_phongLighinting->SunDirection;
}

void Game::AddVertsForGround()
{
	float halfDimension = 1000.f;
	AddVertsForQuad3D(m_groundVerts, Vec3(-halfDimension, -halfDimension, 0.f), Vec3(halfDimension, -halfDimension, 0.f), 
					Vec3(halfDimension, halfDimension, 0.f), Vec3(-halfDimension, halfDimension, 0.f), Rgba8::GRAY_BLUE);

	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArrayDataSize = (m_groundVerts.size()) * vertexSize;
	m_groundVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_groundVerts.size()), vertexSize);
	g_theRenderer->CopyCPUToGPU(m_groundVerts.data(), vertexArrayDataSize, m_groundVertexBuffer);
}

int Game::GetIndexForCoords(IntVec2 coords)
{
	return (coords.y * NUM_SIDE_X + coords.x);
}

void Game::UpdateFogShaderDataWithNewCameraPos(Vec3 const& pos)
{
	m_gpuShaderData->m_cameraPos = Vec4(pos);
}

void Game::BindFogShaderData() const
{
	g_theRenderer->CopyCPUToGPU(m_gpuShaderData, sizeof(FogGPUData), m_fog_CBO);
	g_theRenderer->BindConstantBuffer(k_lightingFogConstantsSlot, m_fog_CBO);
}

void Game::ResetGameWorldByHeadPose(Mat44 const& newMat)
{
	m_toNewResetGameWold = newMat;
}

void Game::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
	// for (physx::PxU32 i = 0; i < nbPairs; i++) 
	// {
	// 	const PxActor* actorA = pairHeader.actors[0];
	// 	const PxActor* actorB = pairHeader.actors[1];
	// 
	// 	// Sort the pair to ensure uniqueness regardless of order
	// 	auto key = std::minmax(actorA, actorB);
	// 
	// 	if (pairs[i].events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
	// 	{
	// 		// If this is the first time they're touching, handle collision
	// 		if (m_activeContacts.insert(key).second) // Only triggers if insert is new
	// 		{
	// 			physx::PxRigidDynamic* actorA = pairHeader.actors[0]->is<physx::PxRigidDynamic>();
	// 			physx::PxRigidDynamic* actorB = pairHeader.actors[1]->is<physx::PxRigidDynamic>();
	// 
	// 			if (actorA && actorB)
	// 			{
	// 				// Ensure both are dynamic actors
	// 				ApplyForceOnActorCollision(actorA, actorB, pairs[i]);
	// 			}
	// 		}
	// 	}
	// 	else if (pairs[i].events & PxPairFlag::eNOTIFY_TOUCH_LOST)
	// 	{
	// 		// Once they separate, allow future contact to trigger again
	// 		m_activeContacts.erase(key);
	// 	}
	// }

	for (physx::PxU32 i = 0; i < nbPairs; i++)
	{
		if (pairs[i].events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			// DebuggerPrintf(Stringf("Collision detected between: %s and %s", pairHeader.actors[0]->getName(), pairHeader.actors[1]->getName()).c_str());
	
			if (pairs[i].events & (physx::PxPairFlag::eNOTIFY_TOUCH_FOUND | physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS))
			{
				physx::PxRigidDynamic* actorA = pairHeader.actors[0]->is<physx::PxRigidDynamic>();
				physx::PxRigidDynamic* actorB = pairHeader.actors[1]->is<physx::PxRigidDynamic>();
	
				if (actorA && actorB)
				{
					// Ensure both are dynamic actors
					ApplyForceOnActorCollision(actorA, actorB, pairs[i]);
				}
			}
		}
	}
}

void Game::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 nbPairs)
{
	(void)pairs;
	(void)nbPairs;
}

void Game::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 nbConstraints)
{
	(void)constraints;
	(void)nbConstraints;
}

void Game::onWake(physx::PxActor** actors, physx::PxU32 count)
{
	(void)actors;
	(void)count;
}

void Game::onSleep(physx::PxActor** actors, physx::PxU32 count)
{
	(void)actors;
	(void)count;
}

void Game::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
{
	(void)bodyBuffer;
	(void)poseBuffer;
	(void)count;
}

void Game::ApplyForceOnActorCollision(physx::PxRigidDynamic* actorA, physx::PxRigidDynamic* actorB, const physx::PxContactPair& contactPair)
{
	// Extract contact points
	PxContactPairPoint contactPoints[10]; // Store up to 10 contacts, how many should I check?
	PxU32 contactCount = contactPair.extractContacts(contactPoints, 10);

	for (PxU32 j = 0; j < contactCount; j++) {
		PxVec3 contactPoint = contactPoints[j].position;
		PxVec3 normal = contactPoints[j].normal;

		DebuggerPrintf("actorA userData: %p, actorB userData: %p", actorA->userData, actorB->userData);

		// Convert PxRigidActor to Actor*
		// because PhysX only stores a void* in userData
		// dynamic_cast only works with polymorphic classes(i.e., classes with at least one virtual function).
		// userData is a void*, so the compiler does not know its type.
		// Since void* is not a class type, dynamic_cast cannot work.
		// will static_cast throw error?
		Actor* baseActor_A = static_cast<Actor*>(actorA->userData);
		Actor* baseActor_B = static_cast<Actor*>(actorB->userData);

		VRPlayer* AisPlayer = dynamic_cast<VRPlayer*>(baseActor_A);
		Crawler* BisEnemy = dynamic_cast<Crawler*>(baseActor_B);

		if (AisPlayer && BisEnemy)
		{
			// transform the info to game world
			Vec3 hittingNormal = GetPhysXToGameSpaceMat().TransformVectorQuantity3D(Vec3(normal));
			Vec3 hitPos = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(contactPoint));

			PxVec3 velocity = actorA->getLinearVelocity();
			// Vec3 velocityNormal = GetPhysXToGameSpaceMat().TransformVectorQuantity3D(Vec3(velocity)).GetNormalized();
			Vec3 attackActorVelocityDir = Vec3(velocity);
			float playerHittingSpeed = Vec3(velocity).GetLength();

			// BisEnemy->WasHitByPlayer(actorB, contactPoint, velocityNormal, playerHittingSpeed);
			if (!BisEnemy->m_isDead)
			{
				BisEnemy->WasHitByPlayer(actorA, actorB, contactPoint, attackActorVelocityDir, playerHittingSpeed);
			}
		}
		else
		{
			Crawler* AisEnemy = dynamic_cast<Crawler*>(baseActor_A);
			VRPlayer* BisPlayer = dynamic_cast<VRPlayer*>(baseActor_B);

			if (AisEnemy && BisPlayer)
			{
				// transform the info to game world
				Vec3 hittingNormal = GetPhysXToGameSpaceMat().TransformVectorQuantity3D(Vec3(normal));
				Vec3 hitPos = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(contactPoint));

				PxVec3 velocity = actorB->getLinearVelocity();
				Vec3 velocityNormal = GetPhysXToGameSpaceMat().TransformVectorQuantity3D(Vec3(velocity)).GetNormalized();
				float playerHittingSpeed = Vec3(velocity).GetLength();

				// AisEnemy->WasHitByPlayer(actorA, contactPoint, velocityNormal, playerHittingSpeed);
				if (!AisEnemy->m_isDead)
				{
					AisEnemy->WasHitByPlayer(actorB, actorA, contactPoint, velocityNormal, playerHittingSpeed);
				}
			}
			else
			{
				Crawler* AisCrawler = dynamic_cast<Crawler*>(baseActor_A);
				Crawler* BisCrawler = dynamic_cast<Crawler*>(baseActor_B);

				// if A and B are all Crawler, they are bumping into each other, roll new random desired position
				if (AisCrawler && BisCrawler && AisCrawler != BisCrawler)
				{
					AisCrawler->RollRandomDesiredPositionInFrontOfPlayer();
					BisCrawler->RollRandomDesiredPositionInFrontOfPlayer();
				}
			}
		}
	}
}

