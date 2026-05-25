#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/core/Timer.hpp"
#include "Game/Player.hpp"
#include "Game/Game.hpp"
#include "Game/Model.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Game/Prop.hpp"
#include "Game/Crawler.hpp"	
#include "Game/App.hpp"
#include <iostream>
#include <Windows.h>

using namespace std;

RandomNumberGenerator* g_rng = nullptr; // always initialize the global variable in the cpp file
Clock* g_theGameClock = nullptr;
Clock* g_theColorChangingClock = nullptr;

extern App* g_theApp;
extern InputSystem* g_theInput; 
extern AudioSystem* g_theAudio;
extern Renderer* g_theRenderer;
extern Window* g_theWindow;

extern Texture* g_textures[NUM_TEXTURES];
extern Material* g_materials[NUM_MATERIALS];

Game::Game()
{
}

Game::~Game()
{

}

void Game::Startup()
{
	// initialize a randomNumberGenerator
	g_rng = new RandomNumberGenerator;

	// create a player and set it up
	m_player = new Player();
	m_player->Startup();
	g_theDebugRenderConfig.m_camera = &dynamic_cast<Player*>(m_player)->m_playerCamera;

	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	// set up the clock for the game 
	g_theGameClock = new Clock();

	SpawnProps();

	m_phongLighinting = new PhongLightingConstants();

	m_FPSTimer = new Timer(0.15f);
	m_FPSTimer->Start();

	// DeleteCurrentTestMode_And_CreateNewTestMode(static_cast<TestingScene>(m_testSceneIndex));
	Crawler* newCrawler = new Crawler();
	newCrawler->Startup();
	m_crawlers.push_back(newCrawler);
}

void Game::Update()
{
	m_player->Update();

	//// update testing entities
	//if (!m_entities.empty())
	//{
	//	for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
	//	{
	//		std::string name = it->first;
	//		Entity* entity = it->second;
	//		entity->Update();
	//	}
	//}
	//if (!m_testSceneProps.empty())
	//{
	//	for (Entity* entity : m_testSceneProps)
	//	{
	//		if (m_rotationMode)
	//		{
	//			entity->m_angularVelocity = EulerAngles(45.f, 0.f, 0.f);
	//		}
	//		else
	//		{
	//			entity->m_angularVelocity = EulerAngles(0.f, 0.f, 0.f);
	//		}
	//		entity->Update();
	//	}
	//}	
	//if (!m_loadedModels.empty())
	//{
	//	for (Entity* model : m_loadedModels)
	//	{
	//		if (m_rotationMode)
	//		{
	//			model->m_angularVelocity = EulerAngles(45.f, 0.f, 0.f);
	//		}
	//		else
	//		{
	//			model->m_angularVelocity = EulerAngles(0.f, 0.f, 0.f);
	//		}
	//		model->Update();
	//	}
	//}

	// UpdateInput();

	for (auto crawler : m_crawlers)
	{
		crawler->Update();
	}

	if (GetDebugRenderVisibility())
	{
		UpdateDebugRenderMessages();
	}
}

void Game::UpdateDebugRenderMessages()
{
	Vec2  textPos = Vec2(1.f, 0.f);
	Vec2  spacing = Vec2(0.f, 0.04f);

	Vec2  fpsAlignment = Vec2(0.99f, 0.98f);
	// Vec2  sunOrientAlignment = fpsAlignment - spacing;
	// Vec2  sceneAlignment = sunOrientAlignment - spacing;
	// Vec2  sunDirectionAlignment = sceneAlignment - spacing;
	// Vec2  sunIntensityAlignment = sunDirectionAlignment - spacing;
	// Vec2  ambientIntensityAlignment = sunIntensityAlignment - spacing;
	// 
	// Vec2  renderAmbientAlignment = ambientIntensityAlignment - spacing;
	// Vec2  renderDiffuseAlignment = renderAmbientAlignment - spacing;
	// Vec2  renderSpecularAlignment = renderDiffuseAlignment - spacing;
	// Vec2  renderEmissiveAlignment = renderSpecularAlignment - spacing;
	// 
	// Vec2  useDiffuseMapAlignment = renderEmissiveAlignment - spacing;
	// Vec2  useNormalMapAlignment = useDiffuseMapAlignment - spacing;
	// Vec2  useSpecularMapAlignment = useNormalMapAlignment - spacing;
	// Vec2  useGlossinessMapAlignment = useSpecularMapAlignment - spacing;
	// Vec2  useEmissiveMapAlignment = useGlossinessMapAlignment - spacing;

	float fontSize = 24.f;

	if (m_FPSTimer->DecrementPeroidIfElapsed())
	{
		m_FPSString = Stringf("FPS: %s", std::to_string(Clock::GetSystemClock().GetFrameRatePerSecond()).c_str());
	}
	DebugAddScreenText(m_FPSString, Vec2(), fontSize, fpsAlignment, -1.f);

	//auto iter = m_entities.find("Sun");
	//if (iter != m_entities.end())
	//{
	//	Entity* sun = iter->second;

	//// sun orientation
	//float yaw = sun->m_orientation.m_yawDegrees;
	//float pitch = sun->m_orientation.m_pitchDegrees;
	//float roll = sun->m_orientation.m_rollDegrees;
	//if (yaw < 0.f)
	//{
	//	yaw += 360.f;
	//}	
	//if (pitch < 0.f)
	//{
	//	pitch += 360.f;
	//}	
	//if (roll < 0.f)
	//{
	//	roll += 360.f;
	//}
	//if (yaw > 360.f)
	//{
	//	yaw += 360.f;
	//}
	//if (pitch > 360.f)
	//{
	//	pitch += 360.f;
	//}
	//if (roll > 360.f)
	//{
	//	roll += 360.f;
	//}
	//std::string sunOrientation = Stringf("Sun orientation (Arrows): ( %.1f, %.1f, %.1f )", yaw, pitch, roll);
	//DebugAddScreenText(sunOrientation, Vec2(), fontSize, sunOrientAlignment, -1.f);			
	//
	//// scene
	//std::string sceneIndex = Stringf("Scene ([ / ]): %i", m_testSceneIndex);
	//DebugAddScreenText(sceneIndex, Vec2(), fontSize, sceneAlignment, -1.f);
	//
	//// sun direction
	//std::string sunDirection = Stringf("Sun Direction (Arrows): ( %.1f, %.1f, %.1f )", m_phongLighinting->SunDirection.x, m_phongLighinting->SunDirection.y, m_phongLighinting->SunDirection.z);
	//DebugAddScreenText(sunDirection, Vec2(), fontSize, sunDirectionAlignment, -1.f);	
	//
	//// sun intensity
	//std::string sunIntensity = Stringf("Sun Intensity (< / >): %.1f", m_phongLighinting->SunIntensity);
	//DebugAddScreenText(sunIntensity, Vec2(), fontSize, sunIntensityAlignment, -1.f);	
	//
	//// ambient intensity
	//std::string ambientIntensity = Stringf("Ambient Intensity (< / >): %.1f", m_phongLighinting->AmbientIntensity);
	//DebugAddScreenText(ambientIntensity, Vec2(), fontSize, ambientIntensityAlignment, -1.f);	
	//
	//// render ambient 
	//std::string renderAmbient = Stringf("Render Ambient [1]: %s", m_phongLighinting->lighingDebug.RenderAmbient? "On" : "Off");
	//DebugAddScreenText(renderAmbient, Vec2(), fontSize, renderAmbientAlignment, -1.f);	
	//
	//// render Diffuse 
	//std::string renderDiffuse = Stringf("Render Diffuse [2]: %s", m_phongLighinting->lighingDebug.RenderDiffuse? "On" : "Off");
	//DebugAddScreenText(renderDiffuse, Vec2(), fontSize, renderDiffuseAlignment, -1.f);	
	//
	//// render Specular
	//std::string renderSpecular = Stringf("Render Specular [3]: %s", m_phongLighinting->lighingDebug.RenderSpecular? "On" : "Off");
	//DebugAddScreenText(renderSpecular, Vec2(), fontSize,renderSpecularAlignment, -1.f);
	//
	//// render Emissive
	//std::string renderEmissive = Stringf("Render Emissive [4]: %s", m_phongLighinting->lighingDebug.RenderEmissive? "On" : "Off");
	//DebugAddScreenText(renderEmissive, Vec2(), fontSize,renderEmissiveAlignment, -1.f);	
	//
	//// Diffuse Map
	//std::string diffuseMap = Stringf("Diffuse Map [5]: %s", m_phongLighinting->lighingDebug.UseDiffuseMap? "On" : "Off");
	//DebugAddScreenText(diffuseMap, Vec2(), fontSize, useDiffuseMapAlignment, -1.f);	
	//
	//// normal Map
	//std::string normalMap = Stringf("Normal Map [6]: %s", m_phongLighinting->lighingDebug.UseNormalMap? "On" : "Off");
	//DebugAddScreenText(normalMap, Vec2(), fontSize, useNormalMapAlignment, -1.f);	
	//
	//// Specular Map
	//std::string specularMap = Stringf("Specular Map [7]: %s", m_phongLighinting->lighingDebug.UseSpecularMap? "On" : "Off");
	//DebugAddScreenText(specularMap, Vec2(), fontSize, useSpecularMapAlignment, -1.f);	
	//
	//// Glossiness Map
	//std::string glossinessMap = Stringf("Glossiness Map [8]: %s", m_phongLighinting->lighingDebug.UseGlossinessMap? "On" : "Off");
	//DebugAddScreenText(glossinessMap, Vec2(), fontSize, useGlossinessMapAlignment, -1.f);	
	//
	//// Emissive Map
	//std::string emissiveMap = Stringf("Emissive Map [9]: %s", m_phongLighinting->lighingDebug.UseEmissiveMap? "On" : "Off");
	//DebugAddScreenText(emissiveMap, Vec2(), fontSize, useEmissiveMapAlignment, -1.f);
	//}
}



void Game::Render()
{	
	RenderWorldInPlayerCamera();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// g_theRenderer->RenderEmissive();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderHUD();
	if (GetDebugRenderVisibility())
	{
		// render the messages on the screen
		DebugRenderScreen(m_screenCamera);
	}
	// if (g_theGameClock->IsPaused())
	// {
	// 	std::vector<Vertex_PCU> backgroundVerts;
	// 	AddVertsForAABB2D(backgroundVerts, m_screenCamera.GetCameraBounds(), Rgba8::BLACK_TRANSPARENT);
	// 	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	// 	g_theRenderer->BindTexture(nullptr);
	// 	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	// 	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	// 	g_theRenderer->DrawVertexArray((int)backgroundVerts.size(), backgroundVerts.data());
	// }
	g_theRenderer->EndCamera(m_screenCamera);
}

// use player camera to render entities in the world
void Game::RenderWorldInPlayerCamera()
{
	Camera& playerCamera = dynamic_cast<Player*>(m_player)->m_playerCamera;
	g_theRenderer->BeginCamera(playerCamera);
	g_theRenderer->ClearScreen(Rgba8::GRAY_Dark);//the background color setting of the window

	// render game world
	if (!m_testSceneProps.empty())
	{
		for (Entity* entity : m_testSceneProps)
		{
			entity->Render();
		}
	}
	if (!m_loadedModels.empty())
	{
		for (auto model : m_loadedModels)
		{
			model->Render();
		}
	}
	if (!m_entities.empty())
	{
		for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
		{
			std::string name = it->first;
			Entity* entity = it->second;
			entity->Render();
		}
	}

	g_theRenderer->SetPhongLightingConstants(*m_phongLighinting);
	for (auto crawler : m_crawlers)
	{
		crawler->Render();
	}

	// g_theRenderer->RenderEmissive();

	// debug render
	DebugRenderWorld(playerCamera);
	g_theRenderer->EndCamera(playerCamera);
}

void Game::SpawnProps()
{
	g_theColorChangingClock = new Clock(*g_theGameClock);
	g_theColorChangingClock->SetTimeScale(0.5f);

	// // add a cube prop
	// Prop* bigCubeProp = new Prop();
	// bigCubeProp->m_name = "First Big Cube";
	// AABB3 bigCube = AABB3(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f));
	// AABB3 smallCube = bigCube * 0.25f;
	// bigCubeProp->m_position = Vec3(2.f, 2.f, 0.f);
	// AddVertsForAABB3D(bigCubeProp->m_unlitVertexes, bigCube,
	// 	Rgba8::RED, Rgba8::CYAN, Rgba8::GREEN, Rgba8::MAGENTA, Rgba8::BLUE, Rgba8::YELLOW);
	// m_entities.push_back(bigCubeProp);
	// 
	// // a copy of the cube
	// Prop* bigCubeCopy = new Prop();
	// bigCubeCopy->m_name = "Second Big Cube";
	// bigCubeCopy->m_position = Vec3(-2.f, -2.f, 0.f);
	// AddVertsForAABB3D(bigCubeCopy->m_unlitVertexes, bigCube,
	// 	Rgba8::RED, Rgba8::CYAN, Rgba8::GREEN, Rgba8::MAGENTA, Rgba8::BLUE, Rgba8::YELLOW);
	// m_entities.push_back(bigCubeCopy);
	// 
	// // add a sphere of rotating about its world axis
	// Prop* sphere = new Prop();
	// sphere->m_name = "Sphere";
	// sphere->m_position = Vec3(10.f, -5.f, 1.f);
	// sphere->m_unlitTexture = g_theApp->g_textures[TESTUV];
	// AddVertsForSphere3D(sphere->m_unlitVertexes, Vec3(), 1.f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 16, 8);
	// m_entities.push_back(sphere);
	// 
	// // add a cylinder of rotating about world z axis
	// Prop* cylinder = new Prop();
	// cylinder->m_name = "Cylinder";
	// cylinder->m_position = Vec3(10.f, 5.f, 1.f);
	// cylinder->m_unlitTexture = g_theApp->g_textures[TESTUV];
	// AddVertsForCylinder3D(cylinder->m_unlitVertexes, Vec3(0.f, 0.f, -1.5f), Vec3(0.f, 0.f, 1.5f), 1.5f);
	// m_entities.push_back(cylinder);
	// 
	// // add a cone of rotating about world z axis
	// Prop* cone = new Prop();
	// cone->m_name = "Cone";
	// cone->m_position = Vec3(10.f, 0.f, 1.f);
	// cone->m_unlitTexture = g_theApp->g_textures[TESTUV];
	// AddVertsForCone3D(cone->m_unlitVertexes, Vec3(-1.5f, 0.f, 0.f), Vec3(1.5f, 0.f, 0.f), 1.5f);
	// m_entities.push_back(cone);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add a grid for the world
	Prop* grid = new Prop();
	grid->m_name = "Grid";
	float spacingXY = 5.f;
	int numOfXY = (int)(100.f / spacingXY) + 1;
	int numOfGrid = 100 + 1;
	float dimensionRed = 0.02f;
	float dimensionGreen = 0.02f;
	float dimensionGray = 0.01f;
	// gray grid
	for (int i = 0; i < numOfGrid; ++i)
	{
		AABB3 pipe(Vec3(-50.f + (float)i - (dimensionGray * 0.5f), -50.f, -(dimensionGray * 0.5f)),
			Vec3( -50.f + (float)i + (dimensionGray * 0.5f), 50.f, (dimensionGray * 0.5f)));
		AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	}
	for (int i = 0; i < numOfGrid; ++i)
	{
		AABB3 pipe(Vec3(-50.f, -50.f + (float)i - (dimensionGray * 0.5f), -(dimensionGray * 0.5f)),
			Vec3(50.f, -50.f + (float)i + (dimensionGray * 0.5f), (dimensionGray * 0.5f)));
		AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	}
	// GREEN lane
	for (int i = 0; i < numOfXY; ++i)
	{
		if ( i == (numOfXY / 2))
		{
			AABB3 pipe(Vec3(-50.f + (float)i * spacingXY - (dimensionGreen * 1.2f), -50.f, -(dimensionGreen * 2.f)),
				Vec3(-50.f + (float)i * spacingXY + (dimensionGreen * 2.f), 50.f, (dimensionGreen * 2.f)));
			AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
		}
		else 
		{
			AABB3 pipe(Vec3(-50.f + (float)i * spacingXY - (dimensionGreen * 0.5f), -50.f, -(dimensionGreen * 0.5f)),
				Vec3(-50.f + (float)i * spacingXY + (dimensionGreen * 0.5f), 50.f, (dimensionGreen * 0.5f)));
			AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
		}
	}
	// RED lane
	for (int i = 0; i < numOfXY; ++i)
	{
		if (i == (numOfXY / 2))
		{
			AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXY - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
				Vec3(50.f, -50.f + (float)i * spacingXY + (dimensionRed * 2.f), (dimensionRed * 2.f)));
			AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
		}
		else
		{
			AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXY - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
				Vec3(50.f, -50.f + (float)i * spacingXY + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
			AddVertsForAABB3D(grid->m_unlitVertexes, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
		}
	}
	m_entities["Grid"] = grid;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add world basis
	// Mat44 worldBasis;
	// DebugAddWorldBasis(worldBasis, -1.f);
	// add world text
	Mat44 XTransform;
	XTransform.Append(Mat44::CreateZRotationDegrees(90.f));
	XTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, .15f)));
	DebugAddWorldText(" X - Forward", XTransform, 0.3f, -1.f, Vec2(0.f, 0.0f), Rgba8::RED, Rgba8::RED);
	Mat44 YTransform;
	YTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, .15f)));
	DebugAddWorldText("Y - Left ", YTransform, 0.3f, -1.f, Vec2(1.f, 0.0f), Rgba8::GREEN, Rgba8::GREEN);

	Mat44 ZTransform;
	ZTransform.Append(Mat44::CreateXRotationDegrees(-90.f));
	ZTransform.Append(Mat44::CreateTranslation3D(Vec3(0.f, 0.f, -.15f)));
	DebugAddWorldText(" Z - Up", ZTransform, 0.3f, -1.f, Vec2(0.f, 1.0f), Rgba8::BLUE, Rgba8::BLUE);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Billboard Testing
	// Vec3 billboardPosition(15.f, 0.f, 4.f);
	// DebugAddWorldBillboardText("The Big Brother is watching YOU", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::WORLD_UP_CAMERA_FACING, Rgba8::CYAN);
	// billboardPosition += Vec3(0.f, 0.f, 1.5f);
	// DebugAddWorldBillboardText("The Big Brother is watching YOU", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::FULL_CAMERA_FACING, Rgba8::CYAN);
	// billboardPosition += Vec3(0.f, 0.f, 1.5f);
	// DebugAddWorldBillboardText("The Big Brother is watching YOU", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::WORLD_UP_CAMERA_OPOSSING, Rgba8::CYAN);
	// billboardPosition += Vec3(0.f, 0.f, 1.5f);
	// DebugAddWorldBillboardText("The Big Brother is watching YOU", billboardPosition, 1.f, Vec2(0.5f, 0.5f), -1.f, BillboardType::FULL_CAMERA_OPPOSING, Rgba8::CYAN);
	
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// arrow for the debugging of the sun
	Prop* sun = new Prop();
	sun->m_name = "Sun";
	sun->m_position = Vec3(0.f, 0.f, 2.5f);
	sun->m_orientation = EulerAngles(120.f, 45.f, 0.f);
	sun->m_unlitTexture = nullptr;
	AddVertsForArrow3D(sun->m_unlitVertexes, Vec3(), Vec3(1.f, 0.f, 0.f), 0.15f, Rgba8::YELLOW_TRANSPARENT, Rgba8::YELLOW_TRANSPARENT);
	m_entities["Sun"] = sun;
	 
	// Model* model = new Model(this);
	// model->LoadXml("Data/Models/Cube_Emissive.xml");
	// m_loadedModels.push_back(model);
}

void Game::UpdateInput()
{
	ControlLightingSettings();
	ControlTextureMapDebug();

	ToggleToShowDebugVertexes();
	ToggleToRotateModelAndProp();
}

void Game::ToggleToShowDebugVertexes()
{
	if (g_theInput->WasKeyJustPressed('N'))
	{
		if (m_debugMode)
		{
			m_debugMode = false;
		}
		else
		{
			m_debugMode = true;
		}
	}
}

void Game::ToggleToRotateModelAndProp()
{
	if (g_theInput->WasKeyJustPressed('R'))
	{
		if (m_rotationMode)
		{
			m_rotationMode = false;
		}
		else
		{
			m_rotationMode = true;
		}
	}
}

void Game::ControlTextureMapDebug()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM1))
	{
		if (m_phongLighinting->lighingDebug.RenderAmbient)
		{
			m_phongLighinting->lighingDebug.RenderAmbient = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.RenderAmbient = true;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM2))
	{
		if (m_phongLighinting->lighingDebug.RenderDiffuse)
		{
			m_phongLighinting->lighingDebug.RenderDiffuse = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.RenderDiffuse = true;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM3))
	{
		if (m_phongLighinting->lighingDebug.RenderSpecular)
		{
			m_phongLighinting->lighingDebug.RenderSpecular = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.RenderSpecular = true;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM4))
	{
		if (m_phongLighinting->lighingDebug.RenderEmissive)
		{
			m_phongLighinting->lighingDebug.RenderEmissive = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.RenderEmissive = true;
		}
	}	
	
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM5))
	{
		if (m_phongLighinting->lighingDebug.UseDiffuseMap)
		{
			m_phongLighinting->lighingDebug.UseDiffuseMap = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.UseDiffuseMap = true;
		}
	}	

	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM6))
	{
		if (m_phongLighinting->lighingDebug.UseNormalMap)
		{
			m_phongLighinting->lighingDebug.UseNormalMap = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.UseNormalMap = true;
		}	
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM7))
	{
		if (m_phongLighinting->lighingDebug.UseSpecularMap)
		{
			m_phongLighinting->lighingDebug.UseSpecularMap = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.UseSpecularMap = true;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM8))
	{
		if (m_phongLighinting->lighingDebug.UseGlossinessMap)
		{
			m_phongLighinting->lighingDebug.UseGlossinessMap = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.UseGlossinessMap = true;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM9))
	{
		if (m_phongLighinting->lighingDebug.UseEmissiveMap)
		{
			m_phongLighinting->lighingDebug.UseEmissiveMap = false;
		}
		else
		{
			m_phongLighinting->lighingDebug.UseEmissiveMap = true;
		}
	}
}

void Game::ControlLightingSettings()
{
	auto iter = m_entities.find("Sun");
	if (iter != m_entities.end())
	{
		Entity* sun = iter->second;

		if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW))
		{
			sun->m_orientation.m_yawDegrees -= 0.5f;
		}		
		if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW))
		{
			sun->m_orientation.m_yawDegrees += 0.5f;
		}		
		if (g_theInput->IsKeyDown(KEYCODE_UPARROW))
		{
			sun->m_orientation.m_pitchDegrees += 0.5f;
		}
		if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW))
		{
			sun->m_orientation.m_pitchDegrees -= 0.5f;
		}		
		
		//----------------------------------------------------------------------------------------------------------------------------------------------------
		if (g_theInput->WasKeyJustPressed(KEYCODE_LESSTHAN))
		{
			if (m_phongLighinting->SunIntensity > 0.f)
			{
				m_phongLighinting->SunIntensity -= 0.1f;
			}
		}		
		if (g_theInput->WasKeyJustPressed(KEYCODE_GREATERTHAN))
		{
			if (m_phongLighinting->SunIntensity < 1.f)
			{
				m_phongLighinting->SunIntensity += 0.1f;
			}
		}

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		m_phongLighinting->SunDirection = sun->m_orientation.GetForwardIBasis().GetNormalized();
		m_phongLighinting->WorldEyePosition = dynamic_cast<Player*>(m_player)->m_playerCamera.m_position;
		m_phongLighinting->AmbientIntensity = 1.f - m_phongLighinting->SunIntensity;
	}
}

void Game::GetWindowsLastErrorAndDisplayOnScreen()
{
	// Retrieves the calling thread's last-error code value
	DWORD errorCode = GetLastError();

	// translate the error code to a readable message
	LPSTR messageBuffer = nullptr;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	FormatMessageA(
		flags,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&messageBuffer,
		0, NULL);

	// show the fail message on screen
	std::string errorMessage(messageBuffer);
	DebugAddMessage(errorMessage, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

	LocalFree(messageBuffer);  // Free the buffer allocated by FormatMessage
}

void Game::LoadNewModel()
{
	// check if there is same name XML, is yes, load the transform info
	// Buffers for split the file path
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	// Split the file path into components
	_splitpath_s(m_modelFilePath.c_str(), drive, dir, fname, ext);

	Model* model = new Model(this);
	// use xml to load a model
	if (_stricmp(ext, ".xml") == 0)
	{
		std::string loadingInfo = "Identify xml file, loading... Please wait...";
		DebugAddMessage(loadingInfo, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

		if (model->LoadXml(m_modelFilePath))
		{
			std::string errorMsg = "File loading succeeded";
			DebugAddMessage(errorMsg, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

			model->m_objectFileName = m_modelFilePath;
			m_loadedModels.push_back(model);
		}
		else
		{
			std::string errorMsg = "File loading failed";
			DebugAddMessage(errorMsg, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
		}
	}
	// use obj to load a model
	else if (_stricmp(ext, ".obj") == 0)
	{
		std::string loadingInfo = "Identify obj file, loading... Please wait...";
		DebugAddMessage(loadingInfo, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

		if (model->LoadObj(m_modelFilePath))
		{
			std::string errorMsg = "File loading succeeded";
			DebugAddMessage(errorMsg, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

			model->m_objectFileName = m_modelFilePath;
			m_loadedModels.push_back(model);
		}
		else
		{
			std::string errorMsg = "File loading failed";
			DebugAddMessage(errorMsg, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
		}

	}
	else
	{
		std::string errorMsg = "File type do not support. Sorry.";
		DebugAddMessage(errorMsg, 5.f, m_debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}
}

void Game::Shutdown()
{
	delete m_player;
	m_player = nullptr;

	for (int i = 0; i < (int)m_loadedModels.size(); ++i)
	{
		if (m_loadedModels[i])
		{
			delete m_loadedModels[i];
		}
	}

	for (auto iter = m_entities.begin(); iter != m_entities.end(); ++iter)
	{
		if (iter->second != nullptr) 
		{  
			delete iter->second;
		}
	}
	m_entities.clear(); 

	for (int i = 0; i < (int)m_testSceneProps.size(); ++i)
	{
		if (m_testSceneProps[i])
		{
			delete m_testSceneProps[i];
		}
	}
	m_testSceneProps.clear();

	delete g_theGameClock;
}

Vec2 Game::GetRandomPosInWorld(Vec2 worldSize)
{
	float posX = g_rng->RollRandomFloatInRange(0, worldSize.x);
	float posY = g_rng->RollRandomFloatInRange(0, worldSize.y);
	return Vec2(posX, posY);
}

// add camera shake when player is dead
void Game::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

void Game::RenderHUD() const
{
	// For the HUD
	// get HUD ratio
	Texture* reticleTexture = g_textures[RETICLE];

	// get current screen camera dimension
	AABB2 screenCameraBounds = m_screenCamera.GetOrthoViewport();
	float viewportWidth = screenCameraBounds.GetDimensions().x;
	float viewportHeight = screenCameraBounds.GetDimensions().y;
	AABB2 HUDBounds;

	// for the reticle
	vector<Vertex_PCU> reticleVerts;

	AABB2 screenViewport = g_theRenderer->GetCameraViewportForD3D11(m_screenCamera);
	IntVec2 reticleSize = IntVec2(50, 50);
	float reticleXFraction = (float)reticleSize.x / screenViewport.GetDimensions().x;
	float reticleYFraction = (float)reticleSize.y / screenViewport.GetDimensions().y;
	AABB2 reticleBounds;

	Vec2 screenCameraCenter(viewportWidth * 0.5f, viewportHeight * 0.45f);
	Vec2 BL = screenCameraCenter - Vec2(reticleXFraction * viewportWidth * 0.5f, reticleYFraction * viewportHeight * 0.5f);
	Vec2 TR = screenCameraCenter + Vec2(reticleXFraction * viewportWidth * 0.5f, reticleYFraction * viewportHeight * 0.5f);
	reticleBounds = AABB2(BL, TR);
	AddVertsUVForAABB2D(reticleVerts, reticleBounds, Rgba8::WHITE);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(reticleTexture);
	g_theRenderer->DrawVertexArray((int)reticleVerts.size(), reticleVerts.data());
}

void Game::RespawnPlayer()
{
	// only allow to respawn playerShip when player is dead
	//if (m_playerShip->m_isDead && m_playerLivesNum > 0)
	//{
	//	// update the playerShip UI after use a life
	//	m_UI_lives[m_playerLivesNum - 1]->m_isEnabled = false;
	//	m_playerLivesNum -= 1;
	//
	//	// reinitialize the player
	//	Vec2 PlayerStart(WORLD_SIZE_X * .5f, WORLD_SIZE_Y * .5f);// Declare & define the pos of player start
	//	m_playerShip = new PlayerShip(this, PlayerStart);// Spawn the player ship
	//}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Game::ClearTestScene()
{
	for (int i = 0; i < (int)m_testSceneProps.size(); ++i)
	{
		if (m_testSceneProps[i])
		{
			delete m_testSceneProps[i];
		}
	}	
	m_testSceneProps.clear();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	for (int i = 0; i < (int)m_loadedModels.size(); ++i)
	{
		if (m_loadedModels[i])
		{
			delete m_loadedModels[i];
		}
	}
	m_loadedModels.clear();
}

void Game::DeleteCurrentTestMode_And_CreateNewTestMode(TestingScene type)
{
	ClearTestScene();

	switch (type)
	{
	case EMISSIVE_CUBES:
	{
		Model* model_textured = new Model(this);
		model_textured->LoadXml("Data/Models/Cube_Textured.xml");
		model_textured->m_position = Vec3(0.f, 0.f, 0.5f);
		m_loadedModels.push_back(model_textured);
		
		Model* model_W = new Model(this);
		model_W->LoadXml("Data/Models/Cube_Emissive.xml");
		model_W->m_position = Vec3(-1.f, -1.f, 0.5f);
		m_loadedModels.push_back(model_W);

		Model* model_R = new Model(this);
		model_R->LoadXml("Data/Models/Cube_Emissive.xml");
		model_R->m_position = Vec3(1.f, -1.f, 0.5f);
		model_R->m_color = Rgba8::RED;
		m_loadedModels.push_back(model_R);

		Model* model_G = new Model(this);
		model_G->LoadXml("Data/Models/Cube_Emissive.xml");
		model_G->m_position = Vec3(1.f, 1.f, 0.5f);
		model_G->m_color = Rgba8::GREEN;
		m_loadedModels.push_back(model_G);

		Model* model_B = new Model(this);
		model_B->LoadXml("Data/Models/Cube_Emissive.xml");
		model_B->m_position = Vec3(-1.f, 1.f, 0.5f);
		model_B->m_color = Rgba8::BLUE;
		m_loadedModels.push_back(model_B);
	}
		break;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	case TUTORIAL_BOX:
	{
		Model* tutorialBox = new Model(this);
		tutorialBox->LoadXml("Data/Models/Tutorial_Box.xml");
		m_loadedModels.push_back(tutorialBox);
	}
		break;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	case HADRIAN_TANK:
	{
		Model* tutorialBox = new Model(this);
		tutorialBox->LoadXml("Data/Models/Hadrian.xml");
		m_loadedModels.push_back(tutorialBox);
	}
		break;
	
	default:
		return;
		break;
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------




