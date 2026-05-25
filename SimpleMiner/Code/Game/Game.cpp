#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/core/FileUtils.hpp"
#include "Game/Player.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Game/Prop.hpp"
#include "Game/UI.hpp"
#include "Game/EnergyBar.hpp"
#include "Game/EnergySelectionRing.hpp"
#include "Game/App.hpp"
#include "Game/World.hpp"

Clock* g_theGameClock = nullptr;
Clock* g_theColorChangingClock = nullptr;
World* g_theWorld = nullptr;

extern App* g_theApp;
extern InputSystem* g_theInput; 
extern AudioSystem* g_theAudio;
extern Renderer* g_theRenderer;
extern Window* g_theWindow;
extern RandomNumberGenerator* g_rng;

Game::Game()
{
}

Game::~Game()
{

}

void Game::Startup()
{
	// create a player and set it up
	m_player = new Player();
	m_player->Startup();
	g_theDebugRenderConfig.m_camera = &dynamic_cast<Player*>(m_player)->m_playerCamera;

	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	// set up the clock for the game 
	g_theGameClock = new Clock();

	// create the world
	g_theWorld = new World();
	g_theWorld->Startup();

	// SpawnProps();

	// create saving data folder
	unsigned char worldSeed = (unsigned char)g_gameConfigBlackboard.GetValue("WorldSeed", 0);
	std::string saveFolderPathName = Stringf("Saves/World_%u", worldSeed);
	CreateFolder(saveFolderPathName);
}

void Game::Update()
{
	// g_debugPosition = m_player->m_position; // send the info to the debug render system to show on screen

	g_theWorld->Update();

	// update testing entities
	if (!m_entities.empty())
	{
		for (int i = 0; i < m_entities.size(); ++i)
		{
			m_entities[i]->Update();
		}
	}
}

void Game::Render()
{	
	RenderWorldInPlayerCamera();
//----------------------------------------------------------------------------------------------------------------------------------------------------
	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);

	if (GetDebugRenderVisibility())
	{
		// render the messages on the screen
		DebugRenderScreen(m_screenCamera);
	}
	if (g_theGameClock->IsPaused())
	{
		std::vector<Vertex_PCU> backgroundVerts;
		AddVertsForAABB2D(backgroundVerts, m_screenCamera.GetCameraBounds(), Rgba8::BLACK_TRANSPARENT);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->DrawVertexArray((int)backgroundVerts.size(), backgroundVerts.data());
	}
	g_theRenderer->EndCamera(m_screenCamera);
}

// use player camera to render entities in the world
void Game::RenderWorldInPlayerCamera()
{
	Camera& playerCamera = dynamic_cast<Player*>(m_player)->m_playerCamera;
	g_theRenderer->BeginCamera(playerCamera);
	Rgba8 fogDayColor = GetColorFromVec4(g_theWorld->m_gpuShaderData->m_fogColor);
	g_theRenderer->ClearScreen(fogDayColor);//the background color setting of the window
	// render game world
	if (!m_entities.empty())
	{
		for (int i = 0; i < m_entities.size(); ++i)
		{
			m_entities[i]->Render();
		}
	}
	g_theWorld->Render();
	// debug render
	DebugRenderWorld(playerCamera);
	g_theRenderer->EndCamera(playerCamera);
}

void Game::SpawnProps()
{
	g_theColorChangingClock = new Clock(*g_theGameClock);
	g_theColorChangingClock->SetTimeScale(0.5f);

	//// add a cube prop
	//Prop* bigCubeProp = new Prop();
	//bigCubeProp->m_name = "First Big Cube";
	//AABB3 bigCube = AABB3(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f));
	//AABB3 smallCube = bigCube * 0.25f;
	//bigCubeProp->m_position = Vec3(2.f, 2.f, 0.f);
	//AddVertsForAABB3D(bigCubeProp->m_vertexes, bigCube,
	//	Rgba8::RED, Rgba8::CYAN, Rgba8::GREEN, Rgba8::MAGENTA, Rgba8::BLUE, Rgba8::YELLOW);
	//m_entities.push_back(bigCubeProp);

	//// a copy of the cube
	//Prop* bigCubeCopy = new Prop();
	//bigCubeCopy->m_name = "Second Big Cube";
	//bigCubeCopy->m_position = Vec3(-2.f, -2.f, 0.f);
	//AddVertsForAABB3D(bigCubeCopy->m_vertexes, bigCube,
	//	Rgba8::RED, Rgba8::CYAN, Rgba8::GREEN, Rgba8::MAGENTA, Rgba8::BLUE, Rgba8::YELLOW);
	//m_entities.push_back(bigCubeCopy);

	//// add a sphere of rotating about its world axis
	//Prop* sphere = new Prop();
	//sphere->m_name = "Sphere";
	//sphere->m_position = Vec3(10.f, -5.f, 1.f);
	//sphere->m_texture = g_theApp->g_textures[TESTUV];
	//AddVertsForSphere3D(sphere->m_vertexes, Vec3(), 1.f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 16, 8);
	//m_entities.push_back(sphere);

	//// add a cylinder of rotating about world z axis
	//Prop* cylinder = new Prop();
	//cylinder->m_name = "Cylinder";
	//cylinder->m_position = Vec3(10.f, 5.f, 1.f);
	//cylinder->m_texture = g_theApp->g_textures[TESTUV];
	//AddVertsForCylinder3D(cylinder->m_vertexes, Vec3(0.f, 0.f, -1.5f), Vec3(0.f, 0.f, 1.5f), 1.5f);
	//m_entities.push_back(cylinder);

	//// add a cone of rotating about world z axis
	//Prop* cone = new Prop();
	//cone->m_name = "Cone";
	//cone->m_position = Vec3(10.f, 0.f, 1.f);
	//cone->m_texture = g_theApp->g_textures[TESTUV];
	//AddVertsForCone3D(cone->m_vertexes, Vec3(-1.5f, 0.f, 0.f), Vec3(1.5f, 0.f, 0.f), 1.5f);
	//m_entities.push_back(cone);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add a grid for the world
	Prop* grid = new Prop();
	grid->m_name = "Grid";
	float spacingXY = 5.f;
	int numOfXY = (int)(100.f / spacingXY) + 1;
	int numOfGrid = 100 + 1;
	float dimensionRed = 0.05f;
	float dimensionGreen = 0.04f;
	float dimensionGray = 0.02f;
	// gray grid
	for (int i = 0; i < numOfGrid; ++i)
	{
		AABB3 pipe(Vec3(-50.f + (float)i - (dimensionGray * 0.5f), -50.f, -(dimensionGray * 0.5f)),
			Vec3( -50.f + (float)i + (dimensionGray * 0.5f), 50.f, (dimensionGray * 0.5f)));
		AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	}
	for (int i = 0; i < numOfGrid; ++i)
	{
		AABB3 pipe(Vec3(-50.f, -50.f + (float)i - (dimensionGray * 0.5f), -(dimensionGray * 0.5f)),
			Vec3(50.f, -50.f + (float)i + (dimensionGray * 0.5f), (dimensionGray * 0.5f)));
		AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	}
	// GREEN lane
	for (int i = 0; i < numOfXY; ++i)
	{
		if ( i == (numOfXY / 2))
		{
			AABB3 pipe(Vec3(-50.f + (float)i * spacingXY - (dimensionGreen * 1.2f), -50.f, -(dimensionGreen * 2.f)),
				Vec3(-50.f + (float)i * spacingXY + (dimensionGreen * 2.f), 50.f, (dimensionGreen * 2.f)));
			AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
		}
		else 
		{
			AABB3 pipe(Vec3(-50.f + (float)i * spacingXY - (dimensionGreen * 0.5f), -50.f, -(dimensionGreen * 0.5f)),
				Vec3(-50.f + (float)i * spacingXY + (dimensionGreen * 0.5f), 50.f, (dimensionGreen * 0.5f)));
			AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
		}
	}
	// RED lane
	for (int i = 0; i < numOfXY; ++i)
	{
		if (i == (numOfXY / 2))
		{
			AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXY - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
				Vec3(50.f, -50.f + (float)i * spacingXY + (dimensionRed * 2.f), (dimensionRed * 2.f)));
			AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
		}
		else
		{
			AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXY - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
				Vec3(50.f, -50.f + (float)i * spacingXY + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
			AddVertsForAABB3D(grid->m_vertexes, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
		}
	}
	m_entities.push_back(grid);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add world basis
	Mat44 worldBasis;
	DebugAddWorldBasis(worldBasis, -1.f);
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

}

void Game::ShutDownUIElementList(int arraySize, UI** m_UIElementArrayPointer)
{
	for (int i = 0; i < arraySize; ++i)
	{
		UI*& UIPtr = m_UIElementArrayPointer[i];// the first const says that the entity instance is const, the second const say that the entity pointer is const

		// tell every existing asteroid to draw analysis
		if (CheckUIEnabled(UIPtr)) // the function that inside a const function calls must be const
		{
			delete UIPtr;
			UIPtr = nullptr;
		}
	}

}

void Game::Shutdown()
{
	delete g_theWorld;
	g_theWorld = nullptr;

	delete m_player;
	m_player = nullptr;

	delete g_theGameClock;
}
/// <UI Settings>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// <health bar>


Vec2 Game::GetRandomPosInWorld(Vec2 worldSize)
{
	float posX = g_rng->RollRandomFloatInRange(0, worldSize.x);
	float posY = g_rng->RollRandomFloatInRange(0, worldSize.y);
	return Vec2(posX, posY);
}

// add camera shake when player is dead
void Game::UpdateCameras(float deltaSeconds)
{
	// the intro start with a zoom out
	m_introTimer -= deltaSeconds;
	if (m_introTimer > 0.f)
	{
		m_worldCamera.ZoomInOutCamera(deltaSeconds, AABB2(Vec2(0.f, 0.f), Vec2(HUD_SIZE_X, HUD_SIZE_Y)), TIME_INTRO_ZOOMOUT);
	}

	// if the camera is going to shake
	if (!m_cameraShakeIsDisplayed)
	{
		// if it is the final blow to the player, because the delta second is slow down, this is the compensation
		if (m_playerLivesNum < 1)
		{
			deltaSeconds *= 8.f;
		}

		// reset camera back to player before shake
		AABB2 followingCamera(Vec2(0.f, 0.f), Vec2(HUD_SIZE_X, HUD_SIZE_Y));
		followingCamera.SetDimensions(Vec2(200.f, 100.f));
		followingCamera.SetCenter(Vec2(HUD_SIZE_X * 0.5f, HUD_SIZE_Y * 0.5f));
		m_worldCamera.SetOrthoView(Vec2(followingCamera.m_mins.x, followingCamera.m_mins.y), Vec2(followingCamera.m_maxs.x, followingCamera.m_maxs.y));
		//m_worldCamera.SetOrthoView(Vec2(0.f, 0.f) + m_playerShip->m_position, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));

		//m_worldCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
		m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

		// calculate the shake amount for the world camera
		m_screenShakeAmount -= CAMERA_SHAKE_REDUCTION * CAMERA_SHAKE_REDUCTION * deltaSeconds;
		m_screenShakeAmount = GetClamped(m_screenShakeAmount, 0.f, CAMERA_SHAKE_AMOUNT_PLAYERDEAD);

		float shakeX = g_rng->RollRandomFloatInRange(-m_screenShakeAmount, m_screenShakeAmount);
		float shakeY = g_rng->RollRandomFloatInRange(-m_screenShakeAmount, m_screenShakeAmount);
		m_worldCamera.Translate2D(Vec2(shakeX, shakeY));

		// if the camera's shaking ends, camera shake has been displayed
		if (m_screenShakeAmount <= 0.f)
		{
			m_cameraShakeIsDisplayed = true;
		}
		return;
	}

}

void Game::TurnCameraShakeOnForPlayerDeath()
{
	// tell that camera shake has not been displayed yet(should start)
	m_cameraShakeIsDisplayed = false;
	m_screenShakeAmount = CAMERA_SHAKE_AMOUNT_PLAYERDEAD;
}

void Game::TurnCameraShakeOnForExplosion()
{
	// tell that camera shake has not been displayed yet(should start)
	m_cameraShakeIsDisplayed = false;
	m_screenShakeAmount = CAMERA_SHAKE_AMOUNT_EXPLOSION;
}

void Game::UpdateUI()
{
	// CheckPlayerLives(deltaSeconds);
	// UpdateEnergyBar(deltaSeconds);
	// UpdateEnergySelectionRing(deltaSeconds);
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

void Game::RenderUIElements() const
{ 
	// dx11 testing
	// Vertex_PCU vertices[] = {
	// Vertex_PCU(Vec3(-0.5f, -0.5f, 0.f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
	// Vertex_PCU(Vec3(0.f, 0.5f, 0.f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
	// Vertex_PCU(Vec3(0.5f, -0.5f, 0.f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f))
	// };
	// g_theRenderer->DrawVertexArray(3, vertices);
}

bool Game::CheckUIEnabled(UI* const UI) const
{
	UNUSED(UI);
	return false;
}

bool Game::DoEntitiesOverlap(Entity const& a, Entity const& b)
{
	UNUSED(a);
	UNUSED(b);
	return false;
}

void Game::RenderUIElementList(int arraySize, UI* const* m_UIElementArrayPointer) const
{
	for (int i = 0; i < arraySize; ++i)
	{
		UI* const UIptr = m_UIElementArrayPointer[i];// we only need to promise that the entity ptr will not be changed because the instance that the pointer point to do not own by the class
		if (CheckUIEnabled(UIptr))
		{
			UIptr->Render();
		}
	}
}

void Game::UpdateUIElementList(int arraySize, UI* const* m_UIElementArrayPointer, float deltaSeconds) const
{
	for (int i = 0; i < arraySize; ++i)
	{
		UI* const UIptr = m_UIElementArrayPointer[i];// we only need to promise that the entity ptr will not be changed because the instance that the pointer point to do not own by the class
		if (CheckUIEnabled(UIptr))
		{
			UIptr->Update(deltaSeconds);
		}
	}
}



