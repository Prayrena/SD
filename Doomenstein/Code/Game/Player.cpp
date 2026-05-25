#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/Easing.hpp"
#include "Game/Controller.hpp"
#include "Game/Player.hpp"
#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"

extern App* g_theApp;
extern Game* g_theGame;
extern InputSystem* g_theInput;
extern Window* g_theWindow;
extern Clock* g_theGameClock;
extern AudioSystem* g_theAudio;
extern BitmapFont* g_consoleFont;

Player::Player(Vec3 position /*= Vec3()*/, Map* owner /*= nullptr*/)
	: Controller(owner)
	, m_position(position)
	, m_mapOwner(owner)
{
	m_onePerSprintModifier = 1.f / m_sprintModifier;
}

Player::Player(SpawnInfo spawnInfo, Map* owner /*= nullptr*/, ActorUID uid /*= ActorUID::INVALID*/)
	:Controller(owner, uid)
{
	m_position = spawnInfo.m_position;
	m_orientation = spawnInfo.m_orientation;
}

Player::~Player()
{

}

void Player::Startup()
{
	m_position = Vec3(3.f, 3.f, 10.f);
	m_orientation = EulerAngles(45.f, 30.f, 0.f);

	m_hitIndicator = new Timer(m_hitIndicateDuration, g_theGameClock);

	// camera set up
	// world camera
	m_worldCamera.SetTransform(m_position, m_orientation);
	m_worldCamera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	m_worldCamera.SetPerspectiveView(g_theGame->m_playerCameraAspectRatio, 60.f, 0.1f, 100.f, this->GetPlayerNormalizedViewport());

	// screen camera
	if (g_theGame->m_playerCameraAspectRatio == 2.f)
	{
		m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y), 0.f, 1.f, this->GetPlayerNormalizedViewport());
	}
	else if (g_theGame->m_playerCameraAspectRatio == 4.f)
	{
		m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y * 0.5f), 0.f, 1.f, this->GetPlayerNormalizedViewport());
	}

	m_listeningTimer = new Timer();
}

void Player::Update()
{
	UpdateLockingWeaponIndicator();
	UpdateInput();
	UpdateCamera();
	Update3DListening();
	UpdateHitIndicator();
	SpawnDebugRenderGeometry();
	UpdateAnimClockForCurrentWeapon();

	if (m_isListening)
	{
		DetectNearbyEnemiesInListeningMode();
	}
	else
	{
		ResetAllActorsBeingListenedStatus();
	}
}

void Player::UpdateCamera()
{
	m_worldCamera.SetTransform(m_position, m_orientation);
}

void Player::UpdateAnimClockForCurrentWeapon()
{
	Actor* controlledActor = m_map->GetActorByUID(m_actorUID);
	Weapon* currentWeapon = controlledActor->GetEquippedWeapon();
	currentWeapon->UpdateAnimClock();
}

void Player::Update3DListening()
{
	g_theAudio->UpdateListener(m_listenerIndex, m_position, GetCameraOrientation(), GetCameraUp());
}

void Player::UpdateHitIndicator()
{
	if (m_hitIndicator->HasPeroidElapsed())
	{
		m_wasHit = false;
		m_hitIndicator->Stop();
	}
}


void Player::Render() const
{

}

void Player::RenderHitIndicator() const
{
	if (m_wasHit)
	{
		std::vector<Vertex_PCU> backgroundVerts;
		float lifeTimeFraction;
		if (m_hitIndicator->m_period <= 0.f)
		{
			lifeTimeFraction = 0.f;
		}
		else
		{
			lifeTimeFraction = m_hitIndicator->GetElapsedFraction();
		}
		Rgba8 color = InterpolateRGBA(m_hitColor, m_healColor, lifeTimeFraction);
		AddVertsForAABB2D(backgroundVerts, g_theGame->m_sharedScreenCamera.GetCameraBounds(), color);

		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->DrawVertexArray((int)backgroundVerts.size(), backgroundVerts.data());
	}
}

void Player::RenderListeningMode() const
{
	if (!m_isListening)
	{
		return;
	}

	// black background
	std::vector<Vertex_PCU> backgroundVerts;
	AddVertsForAABB2D(backgroundVerts, m_screenCamera.GetCameraBounds(), Rgba8::BLACK_TRANSPARENT);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->DrawVertexArray((int)backgroundVerts.size(), backgroundVerts.data());

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// for the listening mode rings
	std::vector<Vertex_PCU> listeningVerts;
	listeningVerts.reserve(324); // assume 3 ring

	AABB2 screenCameraBounds = m_screenCamera.GetCameraBounds();

	// ring thickness will change according to time
	float ringThickness = m_ringThickness;
	ringThickness = ringThickness * 0.5f + ringThickness * fabsf(sinf(2.f * (float)m_listeningTimer->GetElapsedTime()));


	// add rings of which ring radius is based on the dist
	for (int i = 0; i < (int)m_ringPosList.size(); ++i)
	{
		Vec2 ringCenter = m_ringPosList[i];
		ringCenter.x *= screenCameraBounds.GetDimensions().x;
		ringCenter.y *= screenCameraBounds.GetDimensions().y;

		float radius = m_ringDistList[i];

		AddVertesForRing2D(listeningVerts, ringCenter, radius, ringThickness, Rgba8::WHITE, 36);
	}

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->DrawVertexArray((int)listeningVerts.size(), listeningVerts.data());
}

void Player::UpdateLockingWeaponIndicator()
{
	AABB2 screenCameraBounds = m_screenCamera.GetOrthoViewport();
	Actor* controlledActor = m_map->GetActorByUID(m_actorUID);
	Weapon* currentWeapon = controlledActor->GetEquippedWeapon();
	Vec2 screenCenter = Vec2(SCREEN_CAMERA_ORTHO_X * 0.5f, SCREEN_CAMERA_ORTHO_Y * 0.5f);

	// locking indicator
	if (currentWeapon->m_weaponDef->m_weaponType == WeaponType::LOCKING)
	{
		m_lockTargetPos.x *= screenCameraBounds.GetDimensions().x;
		m_lockTargetPos.y *= screenCameraBounds.GetDimensions().y;
		Vec2 disp = m_lockTargetPos - m_lockCursorPos;

		Vec2 directionToTarget = disp.GetNormalized();
		float dist = disp.GetLength();
		float t = RangeMapClamped(dist, m_lockingCircleRadius * 2.f, 0.f, 0.3f, 1.f);
		t = EaseInCubic(t);
		float speed = m_lockingSpeed * t; // speed is related to the dist of disp, move faster when get closer
		GetClamped(speed, 0.f, m_lockingSpeed);

		// the cursor pos could only be in the radius of the center of the screen
		Vec2 newCursorPos = m_lockCursorPos + speed * directionToTarget * g_theGameClock->GetDeltaSeconds();
		if (GetDistance2D(newCursorPos, screenCenter) < m_lockingCircleRadius)
		{
			m_lockCursorPos = newCursorPos;
		}
		else
		{
			Vec2 direction = m_lockTargetPos - screenCenter;
			direction = direction.GetNormalized();
			m_lockCursorPos = screenCenter + direction * m_lockingCircleRadius;
		}

		// std::string lockingTimer = Stringf("lockingTimer= %.2f", currentWeapon->m_lockingTimer->GetElapsedTime());
		// DebugAddScreenText(lockingTimer, Vec2(0.f, 0.f), 24.f, Vec2(0.f, 0.5f), -1.f);
		// 
		// std::string missingTimer = Stringf("missingTimer= %.2f", currentWeapon->m_missingTimer->GetElapsedTime());
		// DebugAddScreenText(missingTimer, Vec2(0.f, 0.f), 24.f, Vec2(0.f, 0.6f), -1.f);

		// if the cursor is too close to target, snapped to the target
		if (GetDistance2D(m_lockTargetPos, m_lockCursorPos) < m_lockingLengthOnScreen)
		{
			m_lockCursorPos = m_lockTargetPos;
		}

		// if the lock target pos is within the aiming circle
		if (GetDistance2D(m_lockTargetPos, screenCenter) < m_lockingCircleRadius)
		{
			currentWeapon->m_aimingAligned = true;
		}
		else
		{
			currentWeapon->m_aimingAligned = false;
		}

	}
}

void Player::RenderScreenCamera() const
{
	if (!m_freeFlyCamera)
	{
		g_theRenderer->BeginCamera(m_screenCamera);

		RenderHUDAndWeapon();

		RenderHitIndicator();

		RenderListeningMode();

		g_theRenderer->EndCamera(m_screenCamera);
	}
}

// the HUD includes the base and reticle
void Player::RenderHUDAndWeapon() const
{
	// first get the aspect ratio of the HUD sprite
	Actor* controlledActor = m_map->GetActorByUID(m_actorUID);
	if (controlledActor)
	{
		Weapon* currentWeapon = controlledActor->GetEquippedWeapon();
		if (!controlledActor->m_isDead) // if the controlled actor is dead, do not display HUD
		{
			if (currentWeapon)
			{
				// verts for HUD
				std::vector<Vertex_PCU> HUDVerts;
				HUDVerts.reserve(6);

				// For the HUD
				// get HUD ratio
				Texture* HUDBaseTexture = currentWeapon->m_weaponDef->m_baseTexture;
				IntVec2 HUDDimensions = HUDBaseTexture->GetDimensions();
				float ratio_H_W = (float)HUDDimensions.y / (float)HUDDimensions.x;
				
				// get current screen camera dimension
				AABB2 screenCameraBounds = m_screenCamera.GetOrthoViewport();
				float viewportWidth = screenCameraBounds.GetDimensions().x;
				float viewportHeight = screenCameraBounds.GetDimensions().y;
				float HUDHeight = viewportWidth * ratio_H_W;
				AABB2 HUDBounds;

				//----------------------------------------------------------------------------------------------------------------------------------------------------
				// for the reticle
				std::vector<Vertex_PCU> reticleVerts;
				HUDVerts.reserve(6);

				AABB2 screenViewport = g_theRenderer->GetCameraViewportForD3D11(m_screenCamera);
				IntVec2 reticleSize = currentWeapon->m_weaponDef->m_reticleSize;
				float reticleXFraction = (float)reticleSize.x / screenViewport.GetDimensions().x;
				float reticleYFraction = (float)reticleSize.y / screenViewport.GetDimensions().y;
				AABB2 reticleBounds;

				//----------------------------------------------------------------------------------------------------------------------------------------------------
				std::vector<Vertex_PCU> weaponVerts;
				weaponVerts.reserve(6);

				// weapon sprite render
				// get four corner points for the quad
				float weaponSpriteWidth = (float)currentWeapon->m_weaponDef->m_spriteSize.x;
				float weaponSpriteHeight = (float)currentWeapon->m_weaponDef->m_spriteSize.y;
				float weaponSpriteXFraction = weaponSpriteWidth / viewportWidth;
				float weaponSpriteYFraction = weaponSpriteHeight / viewportHeight;
				AABB2 weaponSpriteBounds;

				// calculate the pivot shift for the weapon sprite
				float shiftOnX = currentWeapon->m_weaponDef->m_spritePivot.x;
				float shiftOnY = currentWeapon->m_weaponDef->m_spritePivot.y;
				Vec2 pivotShift = Vec2(shiftOnX * weaponSpriteWidth, shiftOnY * weaponSpriteHeight) * (-1.f);

				//----------------------------------------------------------------------------------------------------------------------------------------------------
				// death and kill numbers display
				std::vector<Vertex_PCU> numVerts;
				AABB2 screenBounds = m_screenCamera.GetCameraBounds();
				numVerts.reserve(24);

				//----------------------------------------------------------------------------------------------------------------------------------------------------
				// for the locking weapon
				std::vector<Vertex_PCU> lockingVerts;
				lockingVerts.reserve(108);

				//----------------------------------------------------------------------------------------------------------------------------------------------------
				// add verts
				// base on 1P or 2P, create HUD and reticle at different place
				if (g_theGame->GetNumberOfJoiningPlayer() == 1)
				{
					// HUD
					HUDBounds = AABB2(Vec2(), Vec2(viewportWidth, HUDHeight));
					AddVertsUVForAABB2D(HUDVerts, HUDBounds, Rgba8::WHITE);

					// HUD numbers
					float numHeight = 0.05f;
					std::string killNumbers = Stringf("%i", m_kills);

					std::string healthNumbers = Stringf("%.0f", controlledActor->m_health);
					 
					std::string deathNumbers = Stringf("%i", m_deaths);

					g_consoleFont->AddVertsForTextInBox2D(numVerts, killNumbers, screenBounds, m_HUDnumFontSize, Vec2(0.06f, numHeight), Rgba8::CYAN);
					g_consoleFont->AddVertsForTextInBox2D(numVerts, healthNumbers, screenBounds, m_HUDnumFontSize, Vec2(0.293f, numHeight), Rgba8::CYAN);
					g_consoleFont->AddVertsForTextInBox2D(numVerts, deathNumbers, screenBounds, m_HUDnumFontSize, Vec2(0.945f, numHeight), Rgba8::CYAN);


					// reticle
					Vec2 screenCameraCenter(viewportWidth * 0.5f, viewportHeight * 0.45f);
					Vec2 BL = screenCameraCenter - Vec2(reticleXFraction * viewportWidth * 0.5f, reticleYFraction * viewportHeight * 0.5f);
					Vec2 TR = screenCameraCenter + Vec2(reticleXFraction * viewportWidth * 0.5f, reticleYFraction * viewportHeight * 0.5f);
					reticleBounds = AABB2(BL, TR);
					AddVertsUVForAABB2D(reticleVerts, reticleBounds, Rgba8::WHITE);

					// weapon
					Vec2 weaponPivot = Vec2(viewportWidth * 0.5f, HUDHeight);
					Vec2 weaponBL = Vec2() + weaponPivot;
					Vec2 weaponTR = Vec2(weaponSpriteXFraction * viewportWidth, weaponSpriteYFraction * viewportHeight) + weaponPivot;
					weaponBL += pivotShift;
					weaponTR += pivotShift;
					weaponSpriteBounds = AABB2(weaponBL, weaponTR);

					// locking indicator
					if (currentWeapon->m_weaponDef->m_weaponType == WeaponType::LOCKING)
					{
						Rgba8 lockingColor;
						if (currentWeapon->m_lockingStatus == WeaponLockingStatus::INVALID)
						{
							lockingColor = Rgba8::WHITE;
						}
						else if (currentWeapon->m_lockingStatus == WeaponLockingStatus::LOCKING)
						{
							lockingColor = Rgba8::YELLOW;
						}
						else if (currentWeapon->m_lockingStatus == WeaponLockingStatus::LOCKED)
						{
							lockingColor = Rgba8::GREEN;
						}

						AddVertesForDisc2D(lockingVerts, m_lockCursorPos, 10.f, lockingColor, 36);
						AddVertesForRing2D(lockingVerts, Vec2(SCREEN_CAMERA_ORTHO_X * 0.5f, SCREEN_CAMERA_ORTHO_Y * 0.5f), m_lockingCircleRadius, m_lockingRadiusThickness, lockingColor, 18);
					}

					SpriteDefinition spriteDef = currentWeapon->GetSpriteDefForCurrentState();
					AABB2 uvBounds = spriteDef.GetUVs();
					AddVertsUVForAABB2D(weaponVerts, weaponSpriteBounds, Rgba8::WHITE, uvBounds);
				}
				else if (g_theGame->GetNumberOfJoiningPlayer() == 2)
				{
					// HUD - multilayer will shrink half height
					HUDBounds = AABB2(Vec2(), Vec2(viewportWidth, HUDHeight * 0.5f));
					AddVertsUVForAABB2D(HUDVerts, HUDBounds, Rgba8::WHITE);

					// HUD numbers
					float numHeight = 0.05f;
					std::string killNumbers = Stringf("%i", m_kills);
					std::string healthNumbers = Stringf("%.0f", controlledActor->m_health);
					std::string deathNumbers = Stringf("%i", m_deaths);

					g_consoleFont->AddVertsForTextInBox2D(numVerts, killNumbers, screenBounds, m_HUDnumFontSize * 0.65f, Vec2(0.06f, numHeight * 0.75f), Rgba8::CYAN);
					g_consoleFont->AddVertsForTextInBox2D(numVerts, healthNumbers, screenBounds, m_HUDnumFontSize * 0.65f, Vec2(0.293f, numHeight * 0.75f), Rgba8::CYAN);
					g_consoleFont->AddVertsForTextInBox2D(numVerts, deathNumbers, screenBounds, m_HUDnumFontSize * 0.65f, Vec2(0.945f, numHeight * 0.75f), Rgba8::CYAN);

					// reticle
					Vec2 screenCameraCenter(viewportWidth * 0.5f, viewportHeight * 0.45f);
					Vec2 BL = screenCameraCenter - Vec2(reticleXFraction * viewportWidth * 0.5f, reticleYFraction * viewportHeight * 0.5f);
					Vec2 TR = screenCameraCenter + Vec2(reticleXFraction * viewportWidth * 0.5f, reticleYFraction * viewportHeight * 0.5f);
					reticleBounds = AABB2(BL, TR);
					AddVertsUVForAABB2D(reticleVerts, reticleBounds, Rgba8::WHITE);

					// weapon - multilayer will shrink half height and width
					Vec2 weaponPivot = (Vec2(HUDBounds.m_mins.x, HUDBounds.m_maxs.y) + HUDBounds.m_maxs) * 0.5f;
					Vec2 weaponBL = Vec2() + weaponPivot;
					Vec2 weaponTR = Vec2(weaponSpriteXFraction * viewportWidth * 0.5f, weaponSpriteYFraction * viewportHeight * 0.5f) + weaponPivot;
					weaponBL += pivotShift * 0.5f;
					weaponTR += pivotShift * 0.5f;
					weaponSpriteBounds = AABB2(weaponBL, weaponTR);

					SpriteDefinition spriteDef = currentWeapon->GetSpriteDefForCurrentState();
					AABB2 uvBounds = spriteDef.GetUVs();
					AddVertsUVForAABB2D(weaponVerts, weaponSpriteBounds, Rgba8::WHITE, uvBounds);
				}

				g_theRenderer->SetBlendMode(BlendMode::ALPHA);
				g_theRenderer->SetDepthMode(DepthMode::DISABLED);
				g_theRenderer->SetModelConstants();
				g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

				// render HUD
				if (currentWeapon)
				{
					g_theRenderer->BindTexture(currentWeapon->m_weaponDef->m_baseTexture);
					g_theRenderer->BindShader(currentWeapon->m_weaponDef->m_shader);
				}
				else 
				{
					g_theRenderer->BindTexture(nullptr);
					g_theRenderer->BindShader(nullptr);
				}
				
				g_theRenderer->DrawVertexArray((int)HUDVerts.size(), HUDVerts.data());

				// render HUD numbers
				g_theRenderer->BindTexture(&g_consoleFont->GetTexture());
				g_theRenderer->DrawVertexArray((int)numVerts.size(), numVerts.data());

				// locking indicator
				if (currentWeapon->m_weaponDef->m_weaponType == WeaponType::LOCKING)
				{
					g_theRenderer->BindTexture(nullptr);
					g_theRenderer->BindShader(nullptr);
				}

				g_theRenderer->DrawVertexArray((int)lockingVerts.size(), lockingVerts.data());

				// render reticle
				if (currentWeapon)
				{
					Texture* reticleTexture = currentWeapon->m_weaponDef->m_reticleTexture;
					g_theRenderer->BindTexture(reticleTexture);
				}
				else
				{
					g_theRenderer->BindTexture(nullptr);
				}
				g_theRenderer->DrawVertexArray((int)reticleVerts.size(), reticleVerts.data());

				// render weapon
				if (currentWeapon)
				{
					SpriteDefinition spriteDef = currentWeapon->GetSpriteDefForCurrentState();
					g_theRenderer->BindTexture(&spriteDef.GetTexture());
					g_theRenderer->BindShader(currentWeapon->GetHUDAnimDefForState(currentWeapon->m_currentState)->m_shader);
				}
				else
				{
					g_theRenderer->BindTexture(nullptr);
				}
				g_theRenderer->DrawVertexArray((int)weaponVerts.size(), weaponVerts.data());
			}
		}
	}
}

void Player::DetectNearbyEnemiesInListeningMode()
{
	m_ringPosList.clear();
	m_ringDistList.clear();
	float dotProductRequirement = cosf(ConvertDegreesToRadians(60.f));

	for (int i = 0; i < (int)g_theGame->m_currentMap->m_actorList.size(); ++i)
	{
		Actor* A = g_theGame->m_currentMap->m_actorList[i];
		if (A) // exist
		{
			if (!A->m_isDead && A->m_actorDef->m_faction == ActorFaction::DEMON && A->m_actorDef->m_actorName != "EnergyShield") // not dead and is in enemy faction
			{
				float distToPlayer = GetDistance3D(A->m_position, m_position);
				if (distToPlayer < m_listeningDist)
				{
					// we also need to check if the enemy is in front of us
					if (DotProduct3D((A->m_position - m_position), m_orientation.GetForwardIBasis()) > dotProductRequirement)
					{
						Vec3 targetWorldPos = A->m_position + Vec3(0.f, 0.f, (A->m_actorDef->m_physicsHeight + A->m_actorDef->m_physicsFootHeight) * 0.5f);
						Vec2 screenPos = m_worldCamera.GetViewportNormolizedPositionForWorldPosition(targetWorldPos);
						m_ringPosList.push_back(screenPos);

						float fraction = distToPlayer / m_listeningDist;
						fraction = GetClamped(fraction, 0.f, 1.f);
						fraction = RangeMapClamped(fraction, 0.f, 1.f, m_maxRingRadius, m_minRingRadius);

						m_ringDistList.push_back(fraction);

						// tell the actor is being listening
						A->m_isListenedByPlayer = true;
					}
				}
			}
		}
	}
}

void Player::ResetAllActorsBeingListenedStatus()
{
	for (int i = 0; i < (int)g_theGame->m_currentMap->m_actorList.size(); ++i)
	{
		Actor* A = g_theGame->m_currentMap->m_actorList[i];
		if (A) // exist
		{
			if (A->m_isListenedByPlayer) // was being listened
			{
				A->m_isListenedByPlayer = false;
			}
		}
	}
}

bool Player::IsPlayer()
{
	return true;
}

Mat44 Player::GetModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);
	Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	// orientationMat.Append(transformMat);
	// return orientationMat;
	transformMat.Append(orientationMat);
	return transformMat;
}

Vec3 Player::GetCameraOrientation()
{
	// Vec3 i;
	// Vec3 j;
	// Vec3 k;
	// m_orientation.GetAsVectors_IFwd_JLeft_KUp(i, j, k);
	// return i;

	return GetModelMatrix().TransformVectorQuantity3D(Vec3(1.f, 0.f, 0.f));
}

Vec3 Player::GetCameraUp()
{
	return GetModelMatrix().TransformVectorQuantity3D(Vec3(0.f, 0.f, 1.f));
}

void Player::SetPlayerNormalizedViewport(const AABB2& viewport)
{
	m_normalizedViewport = viewport;
}

AABB2 Player::GetPlayerNormalizedViewport() const
{
	return m_normalizedViewport;
}

void Player::PlayerGotHit()
{
	m_hitIndicator->Restart();
	m_wasHit = true;
}

void Player::UpdateInput()
{
	// g_debugPosition = m_position;
	// g_debugPosition.z = m_orientation.m_yawDegrees;

	// mode switching functions
	Actor* currentActor = m_map->GetActorByUID(m_actorUID);

	// those functions only allow the there is one player in the game
	if (g_theGame->GetNumberOfJoiningPlayer() == 1)
	{
		// switch between free-fly mode and first person mode
		if (g_theInput->WasKeyJustPressed('F'))
		{
			if (m_freeFlyCamera)
			{
				m_freeFlyCamera = false;
				// when come back to first person mode, the player's camera orientation need to be reset as actor's

				m_orientation = currentActor->m_orientation;
			}
			else
			{
				m_freeFlyCamera = true;
			}
		}

		// possess next actor in the map
		if (g_theInput->WasKeyJustPressed('N'))
		{
			Actor* newActor = m_map->DebugPossessNext(m_map->GetActorByUID(m_actorUID));
			currentActor = m_map->GetActorByUID(m_actorUID);
			currentActor->OnUnpossessed(this);
			newActor->OnUnpossessed(newActor->m_controller);
			newActor->OnPossessed(this);

			Possess(newActor);
		}
	}

	// if is in paused mode and it is not in free fly camera mode, all character control input is stopped
	if (g_theGameClock->IsPaused() && !m_freeFlyCamera)
	{
		return;
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// player could fly even when the game is paused
	float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();
	float floatingSpeed = m_floatingSpeed;
	float walkSpeed = 0.f;
	float runSpeed = 0.f;
	if (currentActor)
	{
		walkSpeed = m_map->GetActorByUID(m_actorUID)->m_actorDef->m_walkSpeed;
		runSpeed = m_map->GetActorByUID(m_actorUID)->m_actorDef->m_runSpeed;
	}
	float controllerMovementMultiplier = m_controllerMovementMultiplier;

	// listening mode will slow down the movement
	if (m_isListening)
	{
		runSpeed *= 0.3f;
		walkSpeed *= 0.3f;
	}

	// we use drag and physics simulation to change actor acceleration in actor control
	// float drag = m_map->GetActorByUID(m_actorUID)->m_actorDef->m_drag;
	float desiredSpeed = walkSpeed;
	float MovementSpeed = walkSpeed;
	Vec3  moveDirection;

	// we use delta velocity and delta orientation to change m_position and m_orientation in free fly mode
	Vec3 deltaVelocity = Vec3();
	EulerAngles deltaOrientation;

	// controller controlling
	if (m_controllerIndex != -1)
	{
		XboxController const& controller = g_theInput->GetController(m_controllerIndex);

		if (currentActor)
		{
			if (controller.WasButtonJustPressed(XBOX_BUTTON_X))
			{
				currentActor->EquipWeapon(0);
			}

			if (controller.WasButtonJustPressed(XBOX_BUTTON_Y))
			{
				currentActor->EquipWeapon(1);
			}

			if (controller.WasButtonJustPressed(XBOX_BUTTON_DPAD_UP))
			{
				currentActor->EquipPreviousWeapon();
			}

			if (controller.WasButtonJustPressed(XBOX_BUTTON_DPAD_DOWN))
			{
				currentActor->EquipNextWeapon();
			}

			float RTrigger = controller.GetRightTrigger();
			if (RTrigger > RIGHTTRIGGER_TRIGGERVALUE && !currentActor->m_isDead && !m_freeFlyCamera)
			{
				currentActor->Attack();
			}
		}

		// press A button to increase speed by a factor of 10 while held
		if (controller.IsButtonDown(XBOX_BUTTON_A))
		{
			MovementSpeed = runSpeed;
			desiredSpeed = runSpeed;
			floatingSpeed *= m_sprintModifier;
		}
		if (controller.WasButtonJustPressed(XBOX_BUTTON_A))
		{
			MovementSpeed = walkSpeed;
			desiredSpeed = walkSpeed;
			floatingSpeed *= m_onePerSprintModifier;
		}

		// orientation
		Vec2 leftAnalogPos = controller.GetLeftstick().m_correctedPosition;
		Vec2 rightAnalogPos = controller.GetRightstick().m_correctedPosition;
		// float constollerDeltaPitch = rightAnalogPos.y * m_controllerPitchSensitiveMultiplier * (-1.f);
		// float constollerDeltaYaw = rightAnalogPos.x * m_controllerYawSensitiveMultiplier * (-1.f);
		float controllerViewSpeedPerFrame = m_controllerViewSpeed * g_theGameClock->GetDeltaSeconds();
		float constollerDeltaPitch = rightAnalogPos.y * controllerViewSpeedPerFrame * (-1.f);
		float constollerDeltaYaw = rightAnalogPos.x * controllerViewSpeedPerFrame * (-1.f);
		deltaOrientation += EulerAngles(constollerDeltaYaw, constollerDeltaPitch, 0.f);

		// movement
		deltaVelocity += MovementSpeed * Vec3(leftAnalogPos.y, leftAnalogPos.x * (-1.f), 0.f) * controllerMovementMultiplier;
		moveDirection += Vec3(leftAnalogPos.y, leftAnalogPos.x * (-1.f), 0.f);

		// the going from up and down is the changing the coordinates in world space
		if (m_freeFlyCamera)
		{
			if (controller.IsButtonDown(XBOX_BUTTON_RSHOULDER))
			{
				deltaVelocity += Vec3(0.f, 0.f, floatingSpeed * deltaSeconds);
			}
			if (controller.IsButtonDown(XBOX_BUTTON_LSHOULDER))
			{
				deltaVelocity += Vec3(0.f, 0.f, -(floatingSpeed * deltaSeconds));
			}
		}
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// keyboard controlling
	if (m_controllerIndex == -1)
	{
		if (currentActor)
		{
			if (g_theInput->WasKeyJustPressed(KEYCODE_NUM1))
			{
				currentActor->EquipWeapon(0);
			}

			if (g_theInput->WasKeyJustPressed(KEYCODE_NUM2))
			{
				currentActor->EquipWeapon(1);
			}

			if (g_theInput->WasKeyJustPressed(KEYCODE_NUM3))
			{
				currentActor->EquipWeapon(2);
			}

			if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTARROW))
			{
				currentActor->EquipPreviousWeapon();
			}

			if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTARROW))
			{
				currentActor->EquipNextWeapon();
			}

			if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE) && !currentActor->m_isDead && !m_freeFlyCamera)
			{
				if (currentActor->GetEquippedWeapon()->m_weaponDef->m_weaponType != WeaponType::LOCKING)
				{
					currentActor->Attack();
				}
				else
				{
					if (currentActor->GetEquippedWeapon()->m_lockingStatus == WeaponLockingStatus::LOCKED)
					{
						currentActor->Attack();
					}
				}
			}
		}

		// press shift or A button to increase speed by a factor of 10 while held
		if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
		{
			MovementSpeed = runSpeed;
			desiredSpeed = runSpeed;
			floatingSpeed *= m_sprintModifier;
		}
		if (g_theInput->WasKeyJustPressed(KEYCODE_SHIFT))
		{
			MovementSpeed = walkSpeed;
			desiredSpeed = walkSpeed;
			floatingSpeed *= m_onePerSprintModifier;
		}

		// press R to enter listening mode
		if (g_theInput->IsKeyDown('R'))
		{
			m_isListening = true;
			if (m_listeningTimer->IsStopped())
			{
				m_listeningTimer->Start();
			}
		}		
		if (!g_theInput->IsKeyDown('R'))
		{
			m_isListening = false;
			m_listeningTimer->Stop();
		}

		// the movement is based on player orientation, in local space, not in world space
		if (g_theInput->IsKeyDown('W'))
		{
			deltaVelocity += Vec3(MovementSpeed * deltaSeconds, 0.f, 0.f);
			moveDirection += Vec3(1.f, 0.f, 0.f);
		}
		if (g_theInput->IsKeyDown('S'))
		{
			deltaVelocity += Vec3(-(MovementSpeed * deltaSeconds), 0.f, 0.f);
			moveDirection += Vec3(-1.f, 0.f, 0.f);
		}
		if (g_theInput->IsKeyDown('A'))
		{
			deltaVelocity += Vec3(0.f, MovementSpeed * deltaSeconds, 0.f);
			moveDirection += Vec3(0.f, 1.f, 0.f);
		}
		if (g_theInput->IsKeyDown('D'))
		{
			deltaVelocity += Vec3(0.f, -(MovementSpeed * deltaSeconds), 0.f);
			moveDirection += Vec3(0.f, -1.f, 0.f);
		}		
		if (g_theInput->IsKeyDown('Q'))
		{
			deltaVelocity += Vec3(0.f, 0.f, MovementSpeed * deltaSeconds);
			moveDirection += Vec3(0.f, 1.f, 0.f);
		}
		if (g_theInput->IsKeyDown('E'))
		{
			deltaVelocity += Vec3(0.f, 0.f, -(MovementSpeed * deltaSeconds));
			moveDirection += Vec3(0.f, -1.f, 0.f);
		}

		// the arrow key controls the player rotation in local space
		if (g_theInput->IsKeyDown(KEYCODE_UPARROW))
		{
			deltaOrientation += EulerAngles(0.f, -(m_turnRate * deltaSeconds), 0.f);
		}
		if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW))
		{
			deltaOrientation += EulerAngles(0.f, (m_turnRate * deltaSeconds), 0.f);
		}

		// the going from up and down is the changing the coordinates in world space
		if (m_freeFlyCamera)
		{
			if (g_theInput->IsKeyDown('C'))
			{
				deltaVelocity += Vec3(0.f, 0.f, floatingSpeed * deltaSeconds);
			}
			if (g_theInput->IsKeyDown('Z'))
			{
				deltaVelocity += Vec3(0.f, 0.f, -(floatingSpeed * deltaSeconds));
			}
		}

		// mouse control
		float deltaYaw = (g_theInput->m_cursorState.m_cusorClientDelta.x) * m_mouseYawSensitiveMultiplier * (-1.f);
		float deltaPitch = (g_theInput->m_cursorState.m_cusorClientDelta.y) * m_mousePitchSensitiveMultiplier;
		deltaOrientation += EulerAngles(deltaYaw, deltaPitch, 0.f);

		// clamp the orientation's max pitch and roll
		m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.f, 45.f);
		m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);
	}

	// rolling is banned in this version
	// controller control
	// roll
	// float LeftTrigger = controller.GetLeftTrigger();
	// float rightTrigger = controller.GetRightTrigger();
	// deltaOrientation += EulerAngles(0.f, 0.f, (rightTrigger - LeftTrigger) * m_controllerRollMultiplier * deltaSeconds);

	// if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW))
	// {
	// 	deltaOrientation += EulerAngles((m_turnRate * deltaSeconds), 0.f, 0.f);
	// }
	// if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW))
	// {
	// 	deltaOrientation += EulerAngles(-(m_turnRate * deltaSeconds), 0.f, 0.f);
	// }
	// if (g_theInput->IsKeyDown('Q'))
	// {
	// 	deltaOrientation += EulerAngles(0.f, 0.f, -(m_turnRate * deltaSeconds));
	// }
	// if (g_theInput->IsKeyDown('E'))
	// {
	// 	deltaOrientation += EulerAngles(0.f, 0.f, (m_turnRate * deltaSeconds));
	// }

	// for lockOn target weapon, if the controlled actor is currently using this weapon
	// we will trigger its detecting target function every frame
	Weapon* usingWeapon = currentActor->GetEquippedWeapon();
	if (usingWeapon->m_weaponDef->m_weaponType == WeaponType::LOCKING)
	{
		usingWeapon->DetectLockingTarget(this, currentActor);
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------

	// if player is controlling an actor and in first person mode
	if (currentActor && !m_freeFlyCamera)
	{
		// FPS camera mode
		if (!currentActor->m_isDead)
		{
			m_position = currentActor->m_position + Vec3(0.f, 0.f, currentActor->m_actorDef->m_eyeHeight);

			if (moveDirection.GetLength() > 0.f)
			{
				// transform the move direction from local space to world space and tell the actor about the desired speed
				moveDirection = GetModelMatrix().TransformVectorQuantity3D(moveDirection);
				moveDirection.z = 0.f;
				currentActor->MoveInDirection(moveDirection, desiredSpeed);
			}

			m_orientation += deltaOrientation;
			currentActor->m_orientation = m_orientation;
		}
		else
		{
			if (currentActor)
			{
				// While the player actor is dead
				// animate the camera dropping to the ground such that it hits the ground halfway through the corpse lifetime
				float corpseDuration = currentActor->m_actorDef->m_corpseLifetime;
				float headFallingDuration = corpseDuration * 0.5f;
				float fraction = (currentActor->m_lifetimeTimer->GetElapsedTime()) / headFallingDuration;
				fraction = GetClamped(fraction, 0.f, 1.f);
				m_position.z = Interpolate(currentActor->m_actorDef->m_eyeHeight, 0.f, fraction);

				std::string height = Stringf("height: %.2f", fraction);
				DebugAddScreenText(height, Vec2(0.f, 0.f), 24.f, Vec2(), -1.f);
			}
		}
	}
	else if (m_freeFlyCamera) // in camera free fly mode we are not changing the actor's info
	{
		m_orientation += deltaOrientation;
		m_position += (GetModelMatrix().TransformVectorQuantity3D(deltaVelocity));
	}

	// press 'H' to reset the camera position and orientation
	// if (g_theInput->WasKeyJustPressed('H') || controller.IsButtonDown(XBOX_BUTTON_START))
	// {
	// 	m_position = Vec3(0.f, 0.f, 0.f);
	// 	m_orientation = EulerAngles(0.f, 0.f, 0.f);
	// }
}


void Player::SpawnDebugRenderGeometry()
{
	// press 1
	// Spawn a line from the player along their forward direction 20 units in length
	// if (g_theInput->WasKeyJustPressed(KEYCODE_NUM1))
	// {
	// 	// spawned geometry is spawned in a distance in front of the player in 5.f
	// 	Vec3 spawnDist = Vec3(1.f, 0.f, 0.f);
	// 	Mat44 id;
	// 	Vec3 spawnPos = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnDist);
	// 
	// 	Vec3 dist = Vec3(21.f, 0.f, 0.f);
	// 	Vec3 end = spawnPos + GetModelMatrix().TransformVectorQuantity3D(dist);
	// 	DebugAddWorldLine(spawnPos, end, 0.2f, 10.f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::X_RAY);
	// 	// g_debugRenderGeometries.back()->m_modelMatrix = GetModelMatrix();
	// }

	// press 2
	// Spawn a sphere directly below the player position on the XY-plane
	// if (g_theInput->IsKeyDown(KEYCODE_NUM2))
	// {
	// 	Vec3 projectionOnXY = Vec3(m_position.x, m_position.y, 0.f);
	// 	Rgba8 sphereColor = Rgba8(150, 75, 0, 255);
	// 	DebugAddWorldPoint(projectionOnXY, .25f, 60.f, sphereColor, sphereColor);
	// }

	// press 3
	// Spawn a wire frame sphere 2 units in front of player camera
	// if (g_theInput->WasKeyJustPressed(KEYCODE_NUM3))
	// {
	// 	// 2 units ahead
	// 	Vec3 spawnDist = Vec3(2.f, 0.f, 0.f);
	// 	Vec3 spawnPos = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnDist);
	// 
	// 	DebugAddWorldWireSphere(spawnPos, 1.f, 5.f, Rgba8::GREEN, Rgba8::RED);
	// }

	// press 4
	// Spawn a basis using the player current model matrix
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM4))
	{
		Mat44 modelmatrix = GetModelMatrix();
		DebugAddWorldBasis(modelmatrix, 20.f);
	}

	// press 5
	// Spawn billboarded text showing the player position and orientation
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM5))
	{
		// 0.5 units ahead
		// otherwise player has to move back to see it and when player slide left or right, the billboard immediately turn sideway - which make sense
		Vec3 spawnDist = Vec3(0.5f, 0.f, 0.f);
		Vec3 spawnPos = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnDist);

		// keep only one decimal for the position and orientation
		std::string line_position_orientation = Stringf("Position = %.1f, %.1f, %.1f   Orientation = %.1f, %.1f, %.1f", m_position.x, m_position.y, m_position.z, 
			m_orientation.m_yawDegrees, m_orientation.m_pitchDegrees, m_orientation.m_rollDegrees);

		DebugAddWorldBillboardText(line_position_orientation, spawnPos, 0.3f, Vec2(0.5f, 0.5f), 10.f, BillboardType::FULL_CAMERA_FACING, Rgba8::WHITE, Rgba8::RED);
		// DebugAddWorldText(line_position_orientation, GetModelMatrix(), 0.5f, 10.f, Vec2(0.5f, 0.5f), Rgba8::WHITE, Rgba8::RED);
	}

	// press 6
	// Spawn a wire frame cylinder at player position and orientation
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM6))
	{
		// 1.6 units below(1.6m tall
		Vec3 spawnBase = Vec3(0.f, 0.f, -1.6f);
		Vec3 cylinderBottom = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnBase);
		
		// overall is 1.8 tall
		Vec3 spawnTop = Vec3(0.f, 0.f, .2f);
		Vec3 cylinderTop = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnTop);

		DebugAddWorldWireCylinder(cylinderBottom, cylinderTop, 0.9f, 10.f, Rgba8::WHITE, Rgba8::RED);
	}

	// press 7
	// Add a screen message with the current camera orientation
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM7))
	{
		// keep only one decimal for the position and orientation
		std::string orientation = Stringf("Camera Orientation = %.2f, %.2f, %.2f", m_orientation.m_yawDegrees, m_orientation.m_pitchDegrees, m_orientation.m_rollDegrees);

		DebugAddMessage(orientation, 5.f, 10.f, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}
}

