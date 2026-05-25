#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Game/Bullet.hpp"
#include "Game/Asteroid.hpp"
#include "Game/PlayerShip.hpp"
#include "Game/PlayerShipIcon.hpp"
#include "Game/StarField.hpp"
#include "Game/Beetle.hpp"
#include "Game/Wasp.hpp"
#include "Game/Boid.hpp"
#include "Game/Entity.hpp"
#include "Game/UIElement.hpp"
#include "Game/EnergyBar.hpp"
#include "Game/EnergySelectionRing.hpp"
#include "Game/Debris.hpp"
#include "Game/App.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"
#include "Engine/Audio/AudioSystem.hpp"

RandomNumberGenerator*	g_rng = nullptr; // always initialize the global variable in the cpp file
PlayerShip*				g_playerShip = nullptr;
Clock*					g_theGameClock = nullptr;

extern App* g_theApp;
extern InputSystem* g_theInput; 
extern AudioSystem* g_theAudio;
extern Renderer* g_theRenderer;

Game::Game()
{
}

Game::~Game()
{

}

// TASK:
// spawn PlayerShip in the dead center of the world
// randomly spawn asteroids in the world
void Game::Startup()
{
	// initialize the lives bar
	InitializePlayerLives();
	InitializeEnergyBar();
	InitializeStarField();

	// initialize the player
	Vec2 PlayerStart(WORLD_SIZE_X * .5f, WORLD_SIZE_Y * .5f);// Declare & define the pos of player start
	m_playerShip = new PlayerShip(this, PlayerStart);// Spawn the player ship
	g_playerShip = m_playerShip;

	// initialize a randomNumberGenerator
	g_rng = new RandomNumberGenerator;

	// set the cameras
	AABB2 cameraStart(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	cameraStart.SetDimensions(Vec2(100.f, 50.f));
	cameraStart.SetCenter(PlayerStart);
	m_worldCamera.SetOrthoView(Vec2(cameraStart.m_mins.x, cameraStart.m_mins.y), Vec2(cameraStart.m_maxs.x, cameraStart.m_maxs.y));
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	// set up the clock for the game 
	g_theGameClock = new Clock();
}

void Game::Update()
{
	float deltaSeconds = g_theGameClock->GetDeltaSeconds();

	m_boidLaunchingTimer += deltaSeconds;

	// initialize the first wave of the entities
	if (m_OpenScene)
	{
		spawnNewLevelEntities(deltaSeconds);
	}
	m_OpenScene = false;

	// use I to spawn new asteroids
	if (g_theInput->WasKeyJustPressed('X'))
	{
		SpawnRandomAsteroid();
	}	

	// cheat code for level testing
	if (g_theInput->WasKeyJustPressed('C'))
	{
		ClearLevelEntityForTesting();
	}

	// check if current level is clear
	CheckIfLevelIsClear(deltaSeconds);

	// check collision detection before update the entities
	CheckEntityCollision();

	// update the boid
	// UpdateBoidController(deltaSeconds);

	// so the entities could know if they should die
	UpdateEntities(deltaSeconds);

	// world camera shake when player is got hit or there is large explosion nearby
	// screen camera shake when shield is got hit
	UpdateCameras(deltaSeconds);

	// clear up garbage and dead entities
	DeleteGarbageEntities();

	// update the background star field's alpha color
	UpdateStarField(deltaSeconds);

	if (m_UIisDisplayed)
	{
		UpdateUI(deltaSeconds);
	}
}

void Game::Render() const
{
	// use world camera to render entities in the world
	g_theRenderer->BeginCamera(m_worldCamera);
	RenderStarField();
	RenderEntities();
	g_theRenderer->EndCamera(m_worldCamera);

	// use screen camera to render all UI elements
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderUIElements();
	if (g_theApp->m_debugMode)
	{
		DebugRenderEntities();
	}
	if (g_theGameClock->IsPaused())
	{
		std::vector<Vertex_PCU> backgroundVerts;
		AddVertsForAABB2D(backgroundVerts, m_screenCamera.GetCameraBounds(), Rgba8::BLACK_TRANSPARENT);
		g_theRenderer->DrawVertexArray((int)backgroundVerts.size(), backgroundVerts.data());
	}
	g_theRenderer->EndCamera(m_screenCamera);

}

/// <shut down>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// </summary>
void Game::ClearLevelEntityForTesting()
{
	ShutDownEntityList(MAX_ASTEROIDS, &m_asteroids[0]);
	ShutDownEntityList(MAX_BEETLES, &m_beetles[0]);
	ShutDownEntityList(MAX_WASPS, &m_wasps[0]);
}

void Game::ShutDownEntityList(int arraySize, Entity** m_entityArrayPointer)
{
	for (int i = 0; i < arraySize; ++i)
	{
		Entity*& entityPtr = m_entityArrayPointer[i];// the first const says that the entity instance is const, the second const say that the entity pointer is const

		// tell every existing asteroid to draw analysis
		if (CheckEntityIsAlive(entityPtr)) // the function that inside a const function calls must be const
		{
			delete entityPtr;
			entityPtr = nullptr;
		}
	}

}

void Game::ShutDownUIElementList(int arraySize, UIElement** m_UIElementArrayPointer)
{
	for (int i = 0; i < arraySize; ++i)
	{
		UIElement*& UIPtr = m_UIElementArrayPointer[i];// the first const says that the entity instance is const, the second const say that the entity pointer is const

		// tell every existing asteroid to draw analysis
		if (CheckUIElementEnabled(UIPtr)) // the function that inside a const function calls must be const
		{
			delete UIPtr;
			UIPtr = nullptr;
		}
	}

}

void Game::Shutdown()
{
	ShutDownEntityList(MAX_ASTEROIDS, &m_asteroids[0]);
	ShutDownEntityList(MAX_BULLETS, &m_bullets[0]);
	ShutDownEntityList(MAX_BEETLES, &m_beetles[0]);
	ShutDownEntityList(MAX_WASPS, &m_wasps[0]);
	ShutDownEntityList(MAX_DEBRIS_NUMBERS, &m_debris[0]);
	ShutDownUIElementList(PLAYER_LIVES_NUM, &m_UI_lives[0]);

	delete m_playerShip;
	m_playerShip = nullptr;
}
/// <UI Settings>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// <health bar>
void Game::InitializePlayerLives()
{
	for ( int i = 0; i < m_playerLivesNum; ++i )
	{
		Vec2 iconPos (UI_POSITION_PLAYER_LIVES_X + (i * UI_POSITION_PLAYER_LIVES_SPACING), UI_POSITION_PLAYER_LIVES_Y);
		m_UI_lives[i] = new PlayerShipIcon(this, iconPos);
		m_UI_lives[i]->m_orientationDegrees = 90.f;
	}
}

void Game::InitializeEnergyBar()
{
	m_energyBar = new EnergyBar(this, Vec2(0.f, 0.f));
}

void Game::DrawIconShield(Vec2 centerPos) const
{
	Vec3 worldPos = Vec3(centerPos.x, centerPos.y, 0.f);
	Vertex_PCU verts[9] = {};
	Vec3 pointA = Vec3(-1.2f, 1.f, 0.f);
	Vec3 pointB = Vec3(-1.f, -1.f, 0.f);
	Vec3 pointC = Vec3(0.f, -2.f, 0.f);
	Vec3 pointD = Vec3(1.f, -1.f, 0.f);
	Vec3 pointE = Vec3(1.2f, 1.f, 0.f);

	verts[0].m_position = pointA + worldPos;
	verts[1].m_position = pointB + worldPos;
	verts[2].m_position = pointD + worldPos;

	verts[3].m_position = pointA + worldPos;
	verts[4].m_position = pointD + worldPos;
	verts[5].m_position = pointE + worldPos;

	verts[6].m_position = pointB + worldPos;
	verts[7].m_position = pointC + worldPos;
	verts[8].m_position = pointD + worldPos;

	g_theRenderer->DrawVertexArray(9, &verts[0]);
}

void Game::DrawIconVelocity(Vec2 centerPos) const
{
	Vec3 worldPos = Vec3(centerPos.x, centerPos.y, 0.f);
	Vertex_PCU verts[9] = {};
	Vec3 pointA = Vec3(0.f, 1.8f, 0.f);
	Vec3 pointB = Vec3(0.f, -1.8f, 0.f);
	Vec3 pointC = Vec3(2.f, -1.8f, 0.f);
	Vec3 pointD = Vec3(2.f, 1.8f, 0.f);

	verts[0].m_position = pointA + worldPos;
	verts[1].m_position = pointB + worldPos;
	verts[2].m_position = pointC + worldPos;

	verts[3].m_position = pointA + worldPos;
	verts[4].m_position = pointC + worldPos;
	verts[5].m_position = pointD + worldPos;

	verts[6].m_position = Vec3(-2.f, 0.f, 0.f) + worldPos;
	verts[7].m_position = Vec3(0.f, .3f, 0.f) + worldPos;
	verts[8].m_position = Vec3(0.f, -.3f, 0.f) + worldPos;

	g_theRenderer->DrawVertexArray(9, &verts[0]);
}

void Game::DrawIconWeapon(Vec2 centerPos) const
{
	Vec3 worldPos = Vec3(centerPos.x, centerPos.y, 0.f);
	Vertex_PCU verts[18] = {};
	Vec3 pointA = Vec3(-2.f, .6f, 0.f);
	Vec3 pointB = Vec3(-2.f, -.6f, 0.f);
	Vec3 pointC = Vec3(-.5f, -.6f, 0.f);
	Vec3 pointD = Vec3(-.5f, .6f, 0.f);
	Vec3 pointE = Vec3(-.5f, .3f, 0.f);
	Vec3 pointF = Vec3(-.5f, -.3f, 0.f);
	Vec3 pointG = Vec3(1.f, -.3f, 0.f);
	Vec3 pointH = Vec3(1.f, .3f, 0.f);
	Vec3 pointI = Vec3(1.f, .4f, 0.f);
	Vec3 pointJ = Vec3(1.f, -.4f, 0.f);
	Vec3 pointK = Vec3(1.8f, -0.4f, 0.f);
	Vec3 pointL = Vec3(1.8f, 0.4f, 0.f);

	verts[0].m_position = pointA + worldPos;
	verts[1].m_position = pointB + worldPos;
	verts[2].m_position = pointC + worldPos;

	verts[3].m_position = pointA + worldPos;
	verts[4].m_position = pointC + worldPos;
	verts[5].m_position = pointD + worldPos;

	verts[6].m_position = pointE + worldPos;
	verts[7].m_position = pointF + worldPos;
	verts[8].m_position = pointG + worldPos;

	verts[9].m_position = pointE + worldPos;
	verts[10].m_position = pointG + worldPos;
	verts[11].m_position = pointH + worldPos;

	verts[12].m_position = pointI + worldPos;
	verts[13].m_position = pointJ + worldPos;
	verts[14].m_position = pointK + worldPos;

	verts[15].m_position = pointI + worldPos;
	verts[16].m_position = pointK + worldPos;
	verts[17].m_position = pointL + worldPos;

	g_theRenderer->DrawVertexArray(18, &verts[0]);
}


void Game::InitializeStarField()
{
	// generate the farther star field and fill the pointer in a array
	for (int starIndex = 0; starIndex < UI_STARFIELD_FAR_NUM; ++starIndex)
	{
		UIElement*& starPtr = m_starField_Far[starIndex];

		//initialize an asteroid
		starPtr = new StarField(  this, GetRandomPosInWorld(Vec2(WORLD_SIZE_X, WORLD_SIZE_Y))  );

		// define random orientation and random angular speed
		starPtr->m_orientationDegrees = 90.f * (g_rng->RollRandomIntInRange(0, 1));
		starPtr->m_uniformScale = UI_STARFIELD_FAR_SCALE * (g_rng->RollRandomFloatInRange(0.6f, 1.2f));
		starPtr->m_shinningFrequency = g_rng->RollRandomFloatInRange(0.5f, 5.0f);

		StarField* tempStarFieldPtr = static_cast<StarField*>(starPtr);
		tempStarFieldPtr->m_counterPlayerVelocityScale = UI_STARFIELD_FAR_MOVING_SCALE;
	}

	// generate the near star field and fill the pointer in a array
	for (int starIndex = 0; starIndex < UI_STARFIELD_NEAR_NUM; ++starIndex)
	{
		UIElement*& starPtr = m_starField_Near[starIndex];

		//initialize an asteroid
		starPtr = new StarField(this, GetRandomPosInWorld(Vec2(WORLD_SIZE_X, WORLD_SIZE_Y)));

		// define random orientation and random angular speed
		starPtr->m_orientationDegrees = 90.f * (g_rng->RollRandomIntInRange(0, 1));
		starPtr->m_uniformScale = UI_STARFIELD_NEAR_SCALE * (g_rng->RollRandomFloatInRange(0.8f, 1.5f));
		starPtr->m_shinningFrequency = g_rng->RollRandomFloatInRange(0.5f, 5.0f);

		StarField* tempStarFieldPtr = static_cast<StarField*>(starPtr);
		tempStarFieldPtr->m_counterPlayerVelocityScale = UI_STARFIELD_NEAR_MOVING_SCALE;
	}
}

void Game::UpdateStarField(float deltaSeconds)
{
	UpdateUIElementList(UI_STARFIELD_FAR_NUM, &m_starField_Far[0], deltaSeconds);
	UpdateUIElementList(UI_STARFIELD_NEAR_NUM, &m_starField_Near[0], deltaSeconds);
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
	// normally the camera follows the player
	//if (!m_playerShip->m_isDead && !m_introTimer > 0.f)
	//{
	//	AABB2 followingCamera(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	//	followingCamera.SetDimensions(Vec2(100.f, 50.f));
	//	followingCamera.SetCenter(m_playerShip->m_position);
	//	m_worldCamera.SetOrthoView(Vec2(followingCamera.m_mins.x, followingCamera.m_mins.y), Vec2(followingCamera.m_maxs.x, followingCamera.m_maxs.y));

	//	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));
	//}

	// the intro start with a zoom out
	m_introTimer -= deltaSeconds;
	if (m_introTimer > 0.f)
	{
		m_worldCamera.ZoomInOutCamera(deltaSeconds, AABB2(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y)), TIME_INTRO_ZOOMOUT);
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
		AABB2 followingCamera(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
		followingCamera.SetDimensions(Vec2(200.f, 100.f));
		followingCamera.SetCenter(Vec2(WORLD_SIZE_X * 0.5f, WORLD_SIZE_Y * 0.5f));
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

void Game::UpdateUI(float deltaSeconds)
{
	CheckPlayerLives(deltaSeconds);
	UpdateEnergyBar(deltaSeconds);
	UpdateEnergySelectionRing(deltaSeconds);
}

void Game::UpdateEnergyBar(float deltaSeconds)
{
	m_energyBar->Update(deltaSeconds);
}

void Game::SpawnEnergySelectionRing()
{
	m_energySelectionRing = new EnergySelectionRing(this, Vec2(0.f, 0.f));
}

void Game::DeleteEnergySelectionRing()
{
	if (m_energySelectionRing != nullptr)
	{
		delete m_energySelectionRing;
		m_energySelectionRing = nullptr;
	}
}

void Game::UpdateEnergySelectionRing(float deltaSeconds)
{
	if (m_energySelectionRing != nullptr)
	{
		m_energySelectionRing->Update(deltaSeconds);
	}
}

void Game::CheckPlayerLives(float deltaSeconds)
{
	if (m_playerShip->m_isDead && m_playerLivesNum < 1)
	{
		g_theGameClock->SetTimeScale(0.1f);
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[GAMEOVER_DEFEATE]);
		m_returnToStartTimer += deltaSeconds * 10.f;// make the delta seconds value without the effect of the slow mode
		if (m_returnToStartTimer >= TIME_RETURN_TO_ATTRACTMODE)
		{
			g_theGameClock->SetTimeScale(1.f);
			m_gameIsOver = true;
		}
	}
}

void Game::RespawnPlayer()
{
	// only allow to respawn playerShip when player is dead
	if (m_playerShip->m_isDead && m_playerLivesNum > 0)
	{
		// update the playerShip UI after use a life
		m_UI_lives[m_playerLivesNum - 1]->m_isEnabled = false;
		m_playerLivesNum -= 1;

		// reinitialize the player
		Vec2 PlayerStart(WORLD_SIZE_X * .5f, WORLD_SIZE_Y * .5f);// Declare & define the pos of player start
		m_playerShip = new PlayerShip(this, PlayerStart);// Spawn the player ship

		g_theAudio->StartSound(g_theApp->g_soundEffectsID[PLAYER_RESPAWN], false, 1.5f);
	}
}

/// <spawn entities>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// </summary>
void Game::SpawnRandomAsteroid()
{
	// go through the array of Asteroids and spawn one when there is an opening
	for (int astrdIndex = 0; astrdIndex < MAX_ASTEROIDS; ++astrdIndex)
	{
		Entity*& a = m_asteroids[astrdIndex];
		if (!a)
		{
			//initialize an asteroid
			a = new Asteroid(this, EntitySpawnOffScreenPos(ASTEROID_COSMETIC_RADIUS));

			// define random orientation and random angular speed
			a->m_orientationDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
			a->m_angularVelocity = g_rng->RollRandomFloatInRange(-200.f, 200.f);

			// asteroid being drifted in the space
			Vec2 velocity;
			a->m_linearVelocity = ASTEROID_SPEED; // speed's unit is pixel per second
			a->m_velocity = velocity.MakeFromPolarDegrees(a->m_orientationDegrees, a->m_linearVelocity);

			return;
		}
	}
}

void Game::SpawnRandomWasp()
{
	for (int waspIndex = 0; waspIndex < MAX_WASPS; ++waspIndex)
	{
		Entity*& a = m_wasps[waspIndex];
		if (!a)
		{
			a = new Wasp(this, EntitySpawnOffScreenPos(WASP_COSMETIC_RADIUS));
			return;
		}
	}
}

void Game::SpawnRandomBoid()
{
	for (int i = 0; i < MAX_BOIDS; ++i)
	{
		Entity*& a = m_boids[i];
		if (!a)
		{
			a = new Boid(this, EntitySpawnOffScreenPos(WASP_COSMETIC_RADIUS));
			return;
		}
	}
}

void Game::SpawnRandomBeetle()
{
	for (int beetleIndex = 0; beetleIndex < MAX_BEETLES; ++beetleIndex)
	{
		Entity*& a = m_beetles[beetleIndex];
		if (!a)
		{
			a = new Beetle(this, EntitySpawnOffScreenPos(BEETLE_COSMETIC_RADIUS));
			return;
		}
	}
}

void Game::SpawnBullet(Vec2 const& pos, float& forwardDegrees)
{
	for ( int bI = 0; bI < MAX_BULLETS; ++bI) // for need two ;
	{
		Entity*& bullet = m_bullets[bI];
		if (!bullet) // if the slot in the array if is nullptr
		{
			// new instance will return a pointer to the instance
			bullet = new Bullet(this, pos);// then create a new bullet instance

			// set orientation
			bullet->m_orientationDegrees = forwardDegrees;

			// set velocity
			Vec2 V;
			bullet->m_linearVelocity = BULLET_SPEED;
			bullet->m_velocity = V.MakeFromPolarDegrees(forwardDegrees, bullet->m_linearVelocity);

			break;
		}
	}
	if (m_bullets[MAX_BULLETS-1])
	{
 		ERROR_RECOVERABLE("Reloading bullets!!!");
	}
}

/// <collision detection>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// </summary>
bool Game::DoEntitiesOverlap(Entity const& a, Entity const& b)
{
	Vec2 posA = a.m_position;
	float collisionRadius_A = a.m_physicsRadius;
	Vec2 posB = b.m_position;
	float collisionRadius_B = b.m_physicsRadius;
	float distAB = (posA - posB).GetLength();

	return(distAB < (collisionRadius_A + collisionRadius_B));
}

Vec2 Game::GetCenterPositionofFriendsInRange(Vec2 referencePosition)
{
	Vec2 centerPosition = Vec2(0.f, 0.f);
	int  friendsNum = 0;

	// see if the friend is in the range
	for (int i = 0; i < MAX_BOIDS; ++i)
	{
		Boid* boid = static_cast<Boid*>(m_boids[i]);
		if (boid != nullptr)
		{
			float distToBoid = GetDistance2D(referencePosition, m_boids[i]->m_position);
			if (distToBoid < BOID_TOWARDSCENTER_RANGE)
			{
				// calculate if the friend is in front of you
				centerPosition += m_boids[i]->m_position;
				friendsNum += 1;
			}
		}
	}
	if (friendsNum == 0)
	{
		return referencePosition;
	}
	centerPosition /= static_cast<float>( friendsNum );
	return centerPosition;
}

Vec2 Game::GetAverageFriendVelocityInRange(Vec2 referencePosition, Vec2 normal)
{
	// calculate the average velocity of friends within the foward 180 degrees
	Vec2 averageVelocity = Vec2(0.f, 0.f);
	int  friendsNum = 0;

	// see if the friend is in the range
	for (int i = 0; i < MAX_BOIDS; ++i)
	{
		Boid* boid = static_cast<Boid*>(m_boids[i]);
		if (boid != nullptr)
		{
			float distToBoid = GetDistance2D(referencePosition, m_boids[i]->m_position);
			if (distToBoid < BOID_SAMEDIRECTION_RANGE && distToBoid != 0.f)
			{
				// calculate if the friend is in front of you
				Vec2 disp = boid->m_position - referencePosition;

				if (DotProduct2D(normal, disp) > 0.f)
				{
					averageVelocity += m_boids[i]->m_velocity;
					friendsNum += 1;
				}
			}
		}
	}
	if (friendsNum == 0)
	{
		return Vec2(0.f, 0.f);
	}

	averageVelocity /= static_cast<float>( friendsNum );
	return averageVelocity;
}

Boid* Game::GetTheNearestBoid(Vec2 referencePosition)
{
	// get the nearest friend and keep the social distance
	Boid* nearestBoid = nullptr;
	float nearestBulletDistance = 9999999.f;

	for (int i = 0; i < MAX_BOIDS; ++i)
	{
		Boid* boid = static_cast<Boid*>(m_boids[i]);
		if (boid != nullptr)
		{
			float distToBoid = GetDistance2D(referencePosition, m_boids[i]->m_position);
			if ( distToBoid < nearestBulletDistance && distToBoid != 0.f)
			{
				nearestBoid = boid;
				nearestBulletDistance = distToBoid;
			}
		}
	}

	return nearestBoid;
}

void Game::CheckEntityCollision()
{
	CheckBulletsVSAsteroids();
	CheckBulletsVSBeetles();
	CheckBulletsVSWasps();
	CheckBulletsVSBoids();

	CheckAsteroidsVSShips();
	CheckBeetlesVSShips();
	CheckWaspsVSShips();
	CheckBoidsVSShips();
}

void Game::CheckEntityListCollision(int A_size, Entity** A, int B_size, Entity** B)
{
	for (int aI = 0; aI < A_size; ++aI)
	{
		Entity*& a = A[aI];
		if (CheckEntityIsAlive(a))
		{
			for (int bI = 0; bI < B_size; ++bI) 
			{
				Entity*& b = B[bI];
				if (CheckEntityIsAlive(b))
				{
					if (DoEntitiesOverlap(*a, *b))
					{
						a->TakeDamage();
						b->TakeDamage();
						g_theAudio->StartSound(g_theApp->g_soundEffectsID[ENEMY_WASHIT]);
					}
				}
			}
		}
	}
}

void Game::CheckEntityListWithPlayerShipCollision(int A_size, Entity** A)
{
	for (int aI = 0; aI < A_size; ++aI)
	{
		if (!m_playerShip->m_isDead)
		{
			Entity*& a = A[aI];
			if (CheckEntityIsAlive(a))
			{
				if (DoEntitiesOverlap(*a, *m_playerShip))
				{
					a->TakeDamage();
					m_playerShip->TakeDamage();
					if (!m_playerShip->m_shieldGenerated && m_playerShip->m_isDead)
					{
						g_theAudio->StartSound(g_theApp->g_soundEffectsID[PLAYER_DEAD]);
					}
					TurnCameraShakeOnForPlayerDeath();
				}

			}
		}
		else
		{
			return;
		}
	}	
}

void Game::CheckBulletsVSAsteroids()
{
	CheckEntityListCollision(MAX_BULLETS, &m_bullets[0], MAX_ASTEROIDS, &m_asteroids[0]);
}

void Game::CheckBulletsVSBoids()
{
	CheckEntityListCollision(MAX_BULLETS, &m_bullets[0], MAX_BOIDS, &m_boids[0]);
}

void Game::CheckBulletsVSBeetles()
{

	CheckEntityListCollision(MAX_BULLETS, &m_bullets[0], MAX_BEETLES, &m_beetles[0]);
}

void Game::CheckBulletsVSWasps()
{
	CheckEntityListCollision(MAX_BULLETS, &m_bullets[0], MAX_WASPS, &m_wasps[0]);
}

void Game::CheckAsteroidsVSShips()
{

	CheckEntityListWithPlayerShipCollision(MAX_ASTEROIDS, &m_asteroids[0]);
}

void Game::CheckBeetlesVSShips()
{
	CheckEntityListWithPlayerShipCollision(MAX_BEETLES, &m_beetles[0]);
}

void Game::CheckWaspsVSShips()
{
	CheckEntityListWithPlayerShipCollision(MAX_WASPS, &m_wasps[0]);
}

void Game::CheckBoidsVSShips()
{
	CheckEntityListWithPlayerShipCollision(MAX_BOIDS, &m_boids[0]);
}

/// <Entity update functions>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// check if the entity exist && alive
bool Game::CheckEntityIsAlive(Entity* const entity) const// input a pointer that points to a entity
{
	if (entity)
	{
		if (!entity->m_isDead && !entity->m_isGarbage)
		{
			return true;
		}
		return false;
	}
	else
	{
		return false;
	}
}

bool Game::CheckUIElementEnabled(UIElement* const UIElement) const
{
	if (UIElement != nullptr)
	{
		if (UIElement->m_isEnabled)
		{
			return true;
		}
	}
	return false;
}
/// <summary>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// </summary>

//void Game::UpdateBoidController(float deltaSeconds)
//{
//	//AlignBoids(deltaSeconds);
//	//CohereBoids(deltaSeconds);
//	//SeperateBoids(deltaSeconds);
//}

void Game::SpawnDebris(Entity* entityPtr, float deflectRotationDegrees, int debris_min_num, int debris_max_num, Rgba8 debrisColor)
{
	// bullet's have random number of debris
	int debrisPiecesNum = g_rng->RollRandomIntInRange(debris_min_num, debris_max_num);

	for (int i = 0; i < MAX_DEBRIS_NUMBERS; ++i)
	{

		if (!m_debris[i])
		{
			Entity*& d = m_debris[i];
			d = new Debris(this, entityPtr->m_position);

			// debris' color assign by the entity which is encountered with collision
			//d->m_debrisColor = entityPtr->m_debrisColor;
			d->m_debrisColor = debrisColor;

			// each debris deflect at a different angle
			float randomRotation = 0.f;

			if (entityPtr->m_velocity.GetLength() < 0.2)
			{
				randomRotation = g_rng->RollRandomFloatInRange(0.f, 359.9f);
			}

			else
			{
				randomRotation = g_rng->RollRandomFloatInRange(-DEBRIS_DEFLECT_RANGE_DEGREES, DEBRIS_DEFLECT_RANGE_DEGREES);
			}

			// the debris 's linear speed might increase of decrease randomly
			float debrisLnrVelScale = g_rng->RollRandomFloatInRange(DEBRIS_LINEAR_SPEED_SCALE_MIN, DEBRIS_LINEAR_SPEED_SCALE_MAX);

			// bullet's debris fly to the opposite direction
			d->m_velocity = (entityPtr->m_velocity.GetRotatedDegrees(randomRotation + deflectRotationDegrees)) * debrisLnrVelScale;
			// if the entity sit still, the debris pieces also will fly around 
			Vec2 explosionVelocity;
			explosionVelocity = explosionVelocity.MakeFromPolarDegrees(randomRotation, DEBRIS_EXPLOSION_ACCELERATION) * debrisLnrVelScale;

			// the final velocity for the debris pieces
			d->m_velocity += explosionVelocity;

			// each bullet's debris is rotation at a speed range
			d->m_angularVelocity = g_rng->RollRandomFloatInRange(-DEBRIS_ANGULAR_SPEED_DEGREES, DEBRIS_ANGULAR_SPEED_DEGREES);

			// shrink the debris size from the bullet
			float debrisScale = g_rng->RollRandomFloatInRange(DEBRIS_MIN_SCALE, DEBRIS_MAX_SCALE);
			d->debris_inner_radius = entityPtr->m_physicsRadius * debrisScale * debrisScale;
			d->debris_outer_radius = entityPtr->m_cosmeticRadius * debrisScale;
			// initialize the vertexes of the debris only after the radius is set, otherwise it
			d->InitializeLocalVerts();

			// set the debrisPiecesNum -1 when spawn a new pieces
			debrisPiecesNum -= 1;
		}

		// when all debris pieces are spawn, end the function
		if ( debrisPiecesNum == 0 )
		{
			return;
		}		
	}
}

// spawn the enemy at the edge off screen 
Vec2 Game::EnemySpawnOffScreenPos(float entityCosmeticRadius)
{
	float newX = 0.f;
	float newY = 0.f;

	// generate a random number that decide which edge should they spawn at
	int edgeSelect = g_rng->RollRandomIntInRange(1, 4);

	// top edge
	if (edgeSelect == 1)
	{
		newX = (g_rng->RollRandomFloatZeroToOne()) * (200.f + 2 * entityCosmeticRadius);
		newY = 100.f + entityCosmeticRadius;
	}

	// right edge
	if (edgeSelect == 2)
	{
		newX = 200.f + entityCosmeticRadius;
		newY = (g_rng->RollRandomFloatZeroToOne()) * (100.f + 2 * entityCosmeticRadius);
	}

	// bottom edge
	if (edgeSelect == 3)
	{
		newX = (g_rng->RollRandomFloatZeroToOne()) * (200.f + 2 * entityCosmeticRadius);
		newY = -entityCosmeticRadius;
	}

	// left edge
	if (edgeSelect == 3)
	{
		newX = -entityCosmeticRadius;
		newY = (g_rng->RollRandomFloatZeroToOne()) * (100.f + 2 * entityCosmeticRadius);
	}

	Vec2 spawnPos(newX, newY);
	return spawnPos;
}

// spawn the asteroid at the two edge of screen
// because its velocity orientation will be random and it will be teleported to the other side of the screen when touch the edge
Vec2 Game::EntitySpawnOffScreenPos(float entityCosmeticRadius)
{
	float newX = 0;
	float newY = 0;

	// generate a random number that decide which edge should they spawn at
	int edgeSelect = g_rng->RollRandomIntInRange(0, 1);

	// left edge
	if (edgeSelect == 0)
	{
		newX = -entityCosmeticRadius;
		newY = (g_rng->RollRandomFloatZeroToOne()) * (WORLD_SIZE_Y + 2 * entityCosmeticRadius);
	}

	// top edge
	if (edgeSelect == 1)
	{
		newX = (g_rng->RollRandomFloatZeroToOne()) * (WORLD_SIZE_X + 2 * entityCosmeticRadius);
		newY = WORLD_SIZE_Y + entityCosmeticRadius;
	}

	Vec2 spawnPos(newX, newY);

	return spawnPos;
}

// the Entity** means the pointer to the first one of the pointer array
// or when it dereferenced, it is the first one in the pointer array 
void Game::UpdateEntityList(int arraySize, Entity** m_entityArrayPointer, float deltaSeconds)
{
	for ( int i = 0; i < arraySize; ++i )
	{
		Entity* entityPtr = m_entityArrayPointer[i];
		if (entityPtr && !entityPtr->m_isDead )
		{
			entityPtr->Update(deltaSeconds);
		}
	}
}

void Game::UpdateEntities(float deltaSeconds)
{
	UpdateEntityList(MAX_ASTEROIDS, &m_asteroids[0], deltaSeconds);
	UpdateEntityList(MAX_BULLETS,	&m_bullets[0], deltaSeconds);
	UpdateEntityList(MAX_BEETLES,	&m_beetles[0], deltaSeconds);
	UpdateEntityList(MAX_WASPS,		&m_wasps[0], deltaSeconds);
	UpdateEntityList(MAX_BOIDS, &m_boids[0], deltaSeconds);
	UpdateEntityList(MAX_DEBRIS_NUMBERS, &m_debris[0], deltaSeconds);


	// update playerShip
	if (m_playerShip)
	{
		m_playerShip->Update(deltaSeconds);
	}
}

// clear the entity that is dead or it is garbage, so the dead ones do not affect next runframe collision detection
void Game::DeleteGarbageEntityList(int arraySize, Entity** m_entityArrayPointer)
{
	for (int i = 0; i < arraySize; ++i)
	{
		Entity*& entityPtr = m_entityArrayPointer[i];
		if (entityPtr)
		{
			if (entityPtr->m_isDead || entityPtr->m_isGarbage)
			{
				if (!entityPtr->m_isVFX && !entityPtr->m_isProjectile)
				{
					g_theAudio->StartSound(g_theApp->g_soundEffectsID[ENEMY_DESTROYED], false, 0.3f);
				}
				delete entityPtr;// delete the pointer first
				entityPtr = nullptr;// then delete the pointer 
			}
		}
	}
}

void Game::DeleteGarbageEntities()
{ 
	DeleteGarbageEntityList(MAX_ASTEROIDS, &m_asteroids[0]);
	DeleteGarbageEntityList(MAX_BULLETS, &m_bullets[0]);
	DeleteGarbageEntityList(MAX_BEETLES, &m_beetles[0]);
	DeleteGarbageEntityList(MAX_WASPS, &m_wasps[0]);
	DeleteGarbageEntityList(MAX_DEBRIS_NUMBERS, &m_debris[0]);
}

void Game::RenderUIElements() const
{
	RenderUIElementList(PLAYER_LIVES_NUM, &m_UI_lives[0]);
	if (m_energyBar)
	{
		m_energyBar->Render();
	}
	if (m_energySelectionRing)
	{
		m_energySelectionRing->Render();
	}
	if (m_playerShip->m_shieldGenerated || m_playerShip->m_speedBooster || m_playerShip->m_fireRateBooster)
	{
		RenderEnergyBoosterStatus();
	}
}

void Game::RenderEnergyBoosterStatus() const
{
	Vec2 iconPos = Vec2(UI_POSITION_ENERGY_BOOSTER_X, UI_POSITION_ENERGY_BOOSTER_Y);
	Rgba8 iconColor = Rgba8(ENERGY_REMAIN_COLOR_R, ENERGY_REMAIN_COLOR_G, ENERGY_REMAIN_COLOR_B, ENERGY_REMAIN_COLOR_A);
	DrawDisk(iconPos, 4.f, iconColor, Rgba8 (0,0,0,0));

	if (m_playerShip->m_speedBooster)
	{
		DrawIconVelocity(iconPos);
		return;
	}
	if (m_playerShip->m_shieldGenerated)
	{
		DrawIconShield(iconPos);
		return;
	}
	if (m_playerShip->m_fireRateBooster)
	{
		DrawIconWeapon(iconPos);
		return;
	}
}



void Game::RenderUIElementList(int arraySize, UIElement* const* m_UIElementArrayPointer) const
{
	for (int i = 0; i < arraySize; ++i)
	{
		UIElement* const UIptr = m_UIElementArrayPointer[i];// we only need to promise that the entity ptr will not be changed because the instance that the pointer point to do not own by the class
		if (CheckUIElementEnabled(UIptr))
		{
			UIptr->Render();
		}
	}
}

void Game::UpdateUIElementList(int arraySize, UIElement* const* m_UIElementArrayPointer, float deltaSeconds) const
{
	for (int i = 0; i < arraySize; ++i)
	{
		UIElement* const UIptr = m_UIElementArrayPointer[i];// we only need to promise that the entity ptr will not be changed because the instance that the pointer point to do not own by the class
		if (CheckUIElementEnabled(UIptr))
		{
			UIptr->Update(deltaSeconds);
		}
	}
}

void Game::RenderStarField() const
{
	RenderUIElementList(UI_STARFIELD_NEAR_NUM, &m_starField_Near[0]);
	RenderUIElementList(UI_STARFIELD_FAR_NUM, &m_starField_Far[0]);
}

// the game owns the pointer array not the entity
void Game::RenderEntityList(int arraySize, Entity* const* m_entityArrayPointer) const// this promise that the member of this class will not be changed
{
	for (int i = 0; i < arraySize; ++i)
	{
		Entity* const entityPtr = m_entityArrayPointer[i];// we only need to promise that the entity ptr will not be changed because the instance that the pointer point to do not own by the class
		if (CheckEntityIsAlive(entityPtr))
		{
			entityPtr->Render();
		}
	}
}

void Game::RenderEntities() const
{
	RenderEntityList(MAX_ASTEROIDS,		 &m_asteroids[0]);
	RenderEntityList(MAX_BULLETS,		 &m_bullets[0]);
	RenderEntityList(MAX_BEETLES,		 &m_beetles[0]);
	RenderEntityList(MAX_WASPS,			 &m_wasps[0]);
	RenderEntityList(MAX_DEBRIS_NUMBERS, &m_debris[0]);
	RenderEntityList(MAX_BOIDS, &m_boids[0]);

	if (m_playerShip && !m_playerShip->m_isDead)
	{
		m_playerShip->Render();
	}
}

void Game::DebugRenderEntityList(int arraySize, Entity* const* m_entityArrayPointer) const
{
	for (int i = 0; i < arraySize; ++i)
	{
		Entity* const entityPtr = m_entityArrayPointer[i];// the first const says that the entity instance is const, the second const say that the entity pointer is const

		// tell every existing asteroid to draw analysis
		if (CheckEntityIsAlive(entityPtr)) // the function that inside a const function calls must be const
		{
			if (m_playerShip)
			{
				// draw debug line that connect existing entities to the player ship
				DebugDrawLine(m_playerShip->m_position, entityPtr->m_position, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_TARGET_COLOR_R, DEBUGLINE_TARGET_COLOR_G, DEBUGLINE_TARGET_COLOR_B, DEBUGLINE_TARGET_COLOR_A));
			}
			entityPtr->DebugRender();
		}
	}
}

void Game::DebugRenderEntities() const
{
	//tell every existing entity to draw analysis
	DebugRenderEntityList(MAX_ASTEROIDS, &m_asteroids[0]);
	DebugRenderEntityList(MAX_BULLETS, &m_bullets[0]);
	DebugRenderEntityList(MAX_BEETLES, &m_beetles[0]);
	DebugRenderEntityList(MAX_WASPS, &m_wasps[0]);
	DebugRenderEntityList(MAX_BOIDS, &m_boids[0]);
	DebugRenderEntityList(MAX_DEBRIS_NUMBERS, &m_debris[0]);

	// if PlayerShip is still alive, draw analysis
	if (m_playerShip && !m_playerShip->m_isDead)
	{
		m_playerShip->DebugRender();
	}
}

/// <Level functions>
/// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// </summary>
bool Game::CheckEntityListIsClear(int arraySize, Entity** entityList)
{
	for (int i = 0; i < arraySize; ++i)
	{
		Entity* const entityPtr = entityList[i];// we only need to promise that the entity ptr will not be changed because the instance that the pointer point to do not own by the class
		if (CheckEntityIsAlive(entityPtr))
		{
			return false;
		}
	}

	return true;
}

void Game::CheckIfLevelIsClear(float deltaSeconds)
{
	if (CheckEntityListIsClear(MAX_ASTEROIDS, &m_asteroids[0]) &&
		CheckEntityListIsClear(MAX_BEETLES, &m_beetles[0]) &&
		CheckEntityListIsClear(MAX_WASPS, &m_wasps[0]) &&
		CheckEntityListIsClear(MAX_BOIDS, &m_boids[0]))
	{
		m_currentLevelIsClear = true;
		++m_currentLevelIndex;
		spawnNewLevelEntities(deltaSeconds);
	}
	else
	{
		m_currentLevelIsClear = false;
	}
}

void Game::spawnNewLevelEntities(float deltaSeconds)
{
	// when the final level is clear
	if (m_currentLevelIndex > 5)
	{
		g_theGameClock->SetTimeScale(0.1f);

		m_returnToStartTimer += deltaSeconds * 10.f;// make the delta second's value without the effect of the slow mode

		g_theAudio->SetSoundPlaybackSpeed(g_theApp->m_gameModeBgm, 0.f);
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[GAMEOVER_VICTORY]);

		if (m_returnToStartTimer >= TIME_RETURN_TO_ATTRACTMODE)
		{
			g_theGameClock->SetTimeScale(1.f);			
			m_gameIsOver = true;
		}
	}
	else // for level 12345
	{
		SpawnAsteroidForLevel(m_currentLevelIndex - 1);
		SpawnBeetleForLevel(m_currentLevelIndex - 1);
		SpawnWaspForLevel(m_currentLevelIndex - 1);
		SpawnBoidsForLevel(m_currentLevelIndex - 1);

		g_theAudio->StartSound(g_theApp->g_soundEffectsID[NEW_WAVE]);
		if (m_currentLevelIndex == 5)
		{
		}
	}
}

void Game::SpawnAsteroidForLevel(int levelIndex)
{
	for (int i = 0; i < m_asteroidWave[levelIndex]; ++i)
	{
		SpawnRandomAsteroid();
	}
}

void Game::SpawnWaspForLevel(int levelIndex)
{
	for (int i = 0; i < m_waspWave[levelIndex]; ++i)
	{
		SpawnRandomWasp();
	}
}

void Game::SpawnBoidsForLevel(int levelIndex)
{
	for (int i = 0; i < m_boidWave[levelIndex]; ++i)
	{
		SpawnRandomBoid();
	}
}

void Game::SpawnBeetleForLevel(int levelIndex)
{
	for (int i = 0; i < m_beetleWave[levelIndex]; ++i)
	{
		SpawnRandomBeetle();
	}
}


