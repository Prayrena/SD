#include "Engine/core/Clock.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Actor.hpp"
#include "Game/Weapon.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include <vector>

std::vector<WeaponDefinition> WeaponDefinition::s_weaponDefs;

extern Clock* g_theGameClock;
extern RandomNumberGenerator* g_rng;
extern Game* g_theGame;
extern AudioSystem* g_theAudio;

void WeaponDefinition::InitializeWeaponDefs()
{
	XmlDocument weaponDefXml;
	char const* filePath = "Data/Definitions/WeaponDefinitions.xml";
	XmlResult result = weaponDefXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("failed to load weapon definitions xml file"));

	XmlElement* rootElement = weaponDefXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "actor definition root Element is nullPtr");

	XmlElement* WeaponDefElement = rootElement->FirstChildElement();

	while (WeaponDefElement)
	{
		// read map info
		std::string elementName = WeaponDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "WeaponDefinition", Stringf("root cant matchup with the name of \"WeaponDefinition\""));
		WeaponDefinition* newWeaponDef = new WeaponDefinition(WeaponDefElement);// calls the constructor function of TileTypeDefinition
		s_weaponDefs.push_back(*newWeaponDef);

		WeaponDefElement = WeaponDefElement->NextSiblingElement();
	}
}

WeaponDefinition* WeaponDefinition::GetWeaponDefByString(std::string str)
{
	for (int i = 0; i < (int)s_weaponDefs.size(); i++)
	{
		if (s_weaponDefs[i].m_weaponName == str)
		{
			return &s_weaponDefs[i];
		}
	}

	ERROR_AND_DIE(Stringf("Could not find %s weapon definition", str.c_str()));
}

WeaponDefinition::WeaponDefinition(XmlElement* weaponDefElement)
{
	// general weapon definition
	m_weaponName = ParseXmlAttribute(*weaponDefElement, "name", "weapon name not found");
	m_refireTime = ParseXmlAttribute(*weaponDefElement, "refireTime", 0.f);
	m_lockOnDuration = ParseXmlAttribute(*weaponDefElement, "lockingTime", 0.f);
	m_missDuration = ParseXmlAttribute(*weaponDefElement, "missDuration", 0.f);

	// raycast weapon
	m_rayCount = ParseXmlAttribute(*weaponDefElement, "rayCount", 0);
	m_rayCone = ParseXmlAttribute(*weaponDefElement, "rayCone", 0.f);
	m_rayRange = ParseXmlAttribute(*weaponDefElement, "rayRange", 0.f);
	m_rayDamage = ParseXmlAttribute(*weaponDefElement, "rayDamage", FloatRange());
	m_rayImpulse = ParseXmlAttribute(*weaponDefElement, "rayImpulse", 0.f);

	// projectile weapon
	m_projectileCount = ParseXmlAttribute(*weaponDefElement, "projectileCount", 0);
	m_refireTime = ParseXmlAttribute(*weaponDefElement, "refireTime", 0.f);
	std::string projectileActorName = ParseXmlAttribute(*weaponDefElement, "projectileActor", "projectile actor is not defined");
	if (projectileActorName != "projectile actor is not defined")
	{
		m_projectileActorDef = ActorDefinition::GetActorDefByString(projectileActorName);
	}
	m_projectileCone = ParseXmlAttribute(*weaponDefElement, "projectileCone", 0.f);
	m_projectileSpeed = ParseXmlAttribute(*weaponDefElement, "projectileSpeed", 0.f);

	// Melee attack
	m_meleeCount = ParseXmlAttribute(*weaponDefElement, "meleeCount", 0);
	m_meleeArc = ParseXmlAttribute(*weaponDefElement, "meleeArc", 0.f);
	m_meleeRange = ParseXmlAttribute(*weaponDefElement, "meleeRange", 0.f);
	m_meleeDamage = ParseXmlAttribute(*weaponDefElement, "meleeDamage", FloatRange());
	m_meleeImpulse = ParseXmlAttribute(*weaponDefElement, "meleeImpulse", 0.f);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// HUD and animations
	XmlElement* HUDElement = weaponDefElement->FirstChildElement("HUD");
	if (HUDElement)
	{
		std::string name = HUDElement->Name();

		std::string shaderPath = ParseXmlAttribute(*HUDElement, "shader", "Default");
		m_shader = g_theRenderer->CreateOrGetShader(shaderPath.c_str());

		std::string texturePath = ParseXmlAttribute(*HUDElement, "baseTexture", "Not found in Xml");
		m_baseTexture = g_theRenderer->CreateOrGetTextureFromFile(texturePath.c_str());

		texturePath = ParseXmlAttribute(*HUDElement, "reticleTexture", "Not found in Xml");
		m_reticleTexture = g_theRenderer->CreateOrGetTextureFromFile(texturePath.c_str());

		m_reticleSize = ParseXmlAttribute(*HUDElement, "reticleSize", IntVec2(0, 0));
		m_spriteSize = ParseXmlAttribute(*HUDElement, "spriteSize", IntVec2(0, 0));

		m_spritePivot = ParseXmlAttribute(*HUDElement, "spritePivot", Vec2());

		// weapon animation
		XmlElement* animElement = HUDElement->FirstChildElement();

		while (animElement)
		{
			name = animElement->Name();

			HUDAnimationDefinition* newHUDAnimDef = new HUDAnimationDefinition();

			newHUDAnimDef->m_name = ParseXmlAttribute(*animElement, "name", "Not found in Xml");

			shaderPath = ParseXmlAttribute(*animElement, "shader", "Default");
			newHUDAnimDef->m_shader = g_theRenderer->CreateOrGetShader(shaderPath.c_str());

			// get sprite sheet texture, cell count and sprite sheet
			std::string spriteSheetPath = ParseXmlAttribute(*animElement, "spriteSheet", "Not found in Xml");
			Texture* spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile(spriteSheetPath.c_str());
			newHUDAnimDef->m_cellCount = ParseXmlAttribute(*animElement, "cellCount", IntVec2(0, 0));
			newHUDAnimDef->m_spriteSheet = new SpriteSheet(*spriteSheetTexture, newHUDAnimDef->m_cellCount);

			// get animation
			newHUDAnimDef->m_startFrame = ParseXmlAttribute(*animElement, "startFrame", 0);
			newHUDAnimDef->m_endFrame = ParseXmlAttribute(*animElement, "endFrame", 0);
			newHUDAnimDef->m_secondsPerFrame = ParseXmlAttribute(*animElement, "secondsPerFrame", 0.f);
			float animDuration = (newHUDAnimDef->m_endFrame - newHUDAnimDef->m_startFrame + 1) * newHUDAnimDef->m_secondsPerFrame;

			std::string animPlayMode = ParseXmlAttribute(*animElement, "playbackMode", "Once");
			newHUDAnimDef->m_playbackMode = SpriteAnimDefinition::GetAnimPlaybackModeByString(animPlayMode);

			newHUDAnimDef->m_spriteAnimDef = new SpriteAnimDefinition(*newHUDAnimDef->m_spriteSheet, newHUDAnimDef->m_startFrame, newHUDAnimDef->m_endFrame, animDuration, newHUDAnimDef->m_playbackMode);

			// store in weapon definition
			m_HUDAnims.push_back(newHUDAnimDef);

			// next anim
			animElement = animElement->NextSiblingElement();
		}

		// next HUD
		HUDElement = HUDElement->NextSiblingElement();
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// sounds
	XmlElement* soundsElement = weaponDefElement->FirstChildElement("Sounds");
	if (soundsElement)
	{
		// if we got sounds, we are going to read all its child sound info		
		// sound
		XmlElement* soundElement = soundsElement->FirstChildElement();

		while (soundElement)
		{
			std::string soundName = ParseXmlAttribute(*soundElement, "sound", "Not found in Xml");
			std::string soundLoadPath = ParseXmlAttribute(*soundElement, "name", "Not found in Xml");

			if (soundName == "Fire")
			{
				m_fireSoundID = g_theAudio->CreateOrGetSound(soundLoadPath, true);
			}

			if (soundName == "LockOn")
			{
				m_lockingID = g_theAudio->CreateOrGetSound(soundLoadPath, true);
			}

			if (soundName == "Locked")
			{
				m_lockedID = g_theAudio->CreateOrGetSound(soundLoadPath, true);
			}

			soundElement = soundElement->NextSiblingElement();
		}
	}

	// see which type of weapon it belongs to
	if (m_rayCount != 0)
	{
		m_weaponType = WeaponType::RAYCAST;
	}
	if (m_projectileCount != 0)
	{
		m_weaponType = WeaponType::PROJECTILE;
	}
	if (m_meleeCount != 0)
	{
		m_weaponType = WeaponType::MELEE;
	}
	if (m_lockOnDuration != 0.f)
	{
		m_weaponType = WeaponType::LOCKING;
	}
}

Weapon::Weapon(WeaponDefinition* weaponDef)
	:m_weaponDef(weaponDef)
{
	m_weaponTimer = new Timer(m_weaponDef->m_refireTime, g_theGameClock);
	m_weaponTimer->Start();

	m_animationClock = new Clock(*g_theGameClock);
	if (m_weaponDef->m_weaponType == WeaponType::LOCKING)
	{
		m_lockingTimer = new Timer(m_weaponDef->m_lockOnDuration, g_theGameClock);
		m_missingTimer = new Timer(m_weaponDef->m_missDuration, g_theGameClock);
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// Animation
void Weapon::UpdateAnimClock()
{
	// if the state changes, reset the anim clock
	if (m_currentState != m_lasFrameState)
	{
		m_animationClock->Reset();
		m_lasFrameState = m_currentState;
	}

	// if the current anim does not loop
	if (GetHUDAnimDefForState(m_currentState)->m_playbackMode != SpriteAnimPlaybackType::LOOP)
	{
		float period = GetWeaponAnimDurationForWeaponState(m_currentState);

		// and the play time has past, change back to default anim
		if (m_animationClock->HasTotalTimeElapsedPeriod(period))
		{
			std::string defaultAnimState = m_weaponDef->m_HUDAnims[0]->m_name;
			WeaponAnimState defaultState = GetWeaponAnimStateByString(defaultAnimState);

			m_currentState = defaultState;
		}
		else
		{
			return;
		}
	}
}

float Weapon::GetWeaponAnimDurationForWeaponState(WeaponAnimState state)
{
	HUDAnimationDefinition* weaponAnimDef = GetHUDAnimDefForState(state);
	return weaponAnimDef->m_spriteAnimDef->GetDurationSeconds();
}

SpriteDefinition const& Weapon::GetSpriteDefForCurrentState() const
{
	HUDAnimationDefinition* weaponAnimDef = GetHUDAnimDefForState(m_currentState);
	return weaponAnimDef->m_spriteAnimDef->GetSpriteDefAtTime(m_animationClock->GetTotalSeconds());
}

HUDAnimationDefinition* Weapon::GetHUDAnimDefForState(WeaponAnimState state) const
{
	// get string for current state
	std::string stateString = GetStringForWeaponState(state);

	// find the animation group according to current state string
	int numAnim = (int)m_weaponDef->m_HUDAnims.size();
	for (int i = 0; i < numAnim; ++i)
	{
		if (m_weaponDef->m_HUDAnims[i]->m_name == stateString)
		{
			HUDAnimationDefinition* HUDAnimDef = m_weaponDef->m_HUDAnims[i];
			return HUDAnimDef;
		}
	}

	ERROR_AND_DIE(Stringf("Has not found the matching animation group definition for the %s state", stateString.c_str()));
}

void Weapon::PlayAnimation(WeaponAnimState state)
{
	// change the current state, reset the clock time scale
	// don't change the state if it is already in the state
	if (m_currentState != state)
	{
		m_currentState = state;
	}
}

std::string Weapon::GetStringForWeaponState(WeaponAnimState state) const
{
	// get string for current weapon state
	std::string stateString;
	switch (state)
	{
	case WeaponAnimState::IDLE: {return stateString = "Idle"; } break;
	case WeaponAnimState::ATTACK: {return stateString = "Attack"; } break;
	}
	ERROR_AND_DIE("Missing case");
}


WeaponAnimState Weapon::GetWeaponAnimStateByString(std::string name) const
{
	if (name == "Idle")
	{
		return WeaponAnimState::IDLE;
	}
	if (name == "Attack")
	{
		return WeaponAnimState::ATTACK;
	}

	ERROR_AND_DIE("Did not find actor state for the input name");
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// If so, fires each of the ray casts, projectiles, and melee attacks defined in the definition
// Needs to pass along its owning actor to be ignored in all raycast and collision checks.
ActorPtrList Weapon::Fire(Actor* owner)
{
	ActorPtrList enemies;

	// Checks if the weapon is ready to fire
	if (m_weaponTimer->HasPeroidElapsed())
	{
		PlayAnimation(WeaponAnimState::ATTACK);

		switch (m_weaponDef->m_weaponType)
		{
		case WeaponType::RAYCAST:
		{
			// get the actors of different faction in the map
			Map* ownerMap = owner->m_map;
			enemies = ownerMap->GetActorsExceptSelf(owner);

			// get deflect shooting normal
			Vec3 rayStart = owner->GetRaycastShootingPosition();
			Vec3 fwdNormal = owner->m_controller->GetAimingDirection();
			fwdNormal = GetRandomDirectionInCone(fwdNormal, m_weaponDef->m_rayCone);

			// get hit enemy
			float shootRange = m_weaponDef->m_rayRange;
			Actor* hitEnemy = owner->m_map->CollisionTestForRaycastWeaponFiring(rayStart, fwdNormal, shootRange, owner, enemies);

			// add damage to the hit enemy
			if (hitEnemy)
			{
				float damage = g_rng->RollRandomFloatInFloatRange(m_weaponDef->m_rayDamage);
				hitEnemy->TakeDamage(damage, owner);
				// g_theGame->m_currentMap->SpawnBloodSplatterOnHitEnemy(); // todo: 4.18 should this be a function for map to manage or weapon to manage?
				if (m_weaponDef->m_rayImpulse > 0.f)
				{
					CalculateAndApplyImpulse(owner,hitEnemy, m_weaponDef->m_rayImpulse);
				}
			}
		} break;
		case WeaponType::LOCKING:
		{
			if (m_lockingTarget)
			{
				// Projectiles spawn a projectile actor that is imparted an initial velocity then does damage when it collides
				ActorDefinition* def = m_weaponDef->m_projectileActorDef;

				Vec3 pos = owner->GetPlasmaShootingPosition();
				Vec3 direction = owner->m_controller->GetAimingDirection();
				Vec3 vel = m_weaponDef->m_projectileSpeed * direction;
				EulerAngles orientation = direction.GetOrientation();

				SpawnInfo* projectileSpawnInfo = new SpawnInfo(def, pos, vel, orientation);
				Actor* projectileActor = owner->m_map->SpawnActorAndAddToMapActorList(*projectileSpawnInfo);
				projectileActor->m_ownerUID = owner->m_actorUID;
				projectileActor->m_ownerWeapon = this;
				projectileActor->Startup();

				float dist = GetDistance3D(owner->m_position, m_lockingTarget->m_position);
				float flyingDuration = dist / m_weaponDef->m_projectileSpeed;
				Vec3 hitPosition = m_lockingTarget->m_position + Vec3(0.f, 0.f, m_lockingTarget->m_actorDef->m_eyeHeight);
				projectileActor->ConstructProjectileTrajectory(pos, m_lockingTarget, flyingDuration, m_weaponDef->m_projectileSpeed);
			}
		} break;
		case WeaponType::PROJECTILE:
		{
			// Projectiles spawn a projectile actor that is imparted an initial velocity then does damage when it collides
			ActorDefinition* def = m_weaponDef->m_projectileActorDef;

			Vec3 pos = owner->GetPlasmaShootingPosition();
			Vec3 direction = owner->m_controller->GetAimingDirection();
			direction = GetRandomDirectionInCone(direction, m_weaponDef->m_projectileCone);
			Vec3 vel = m_weaponDef->m_projectileSpeed * direction;
			EulerAngles orientation = direction.GetOrientation();

			SpawnInfo* projectileSpawnInfo = new SpawnInfo(def, pos, vel, orientation);
			Actor* projectileActor = owner->m_map->SpawnActorAndAddToMapActorList(*projectileSpawnInfo);
			projectileActor->m_ownerUID = owner->m_actorUID;
			projectileActor->m_ownerWeapon = this;
			projectileActor->Startup();
		} break;
		case WeaponType::MELEE:
		{
			// get the actors of different faction in the map
			Map* ownerMap = owner->m_map;
			enemies = ownerMap->GetActorsOfDifferentFaction(owner->m_actorDef->m_faction);

			// check if the enemy is in the attack sector
			for (int i = 0; i < (int)enemies.size(); ++i)
			{
				bool hit = IsPointInsideOrientedSector2D(Vec2(enemies[i]->m_position), Vec2(owner->m_position), Vec2(owner->GetForwardNormal()),
					m_weaponDef->m_meleeArc, m_weaponDef->m_meleeRange);
				if (hit)
				{
					for (int j = 0; j < m_weaponDef->m_meleeCount; ++j)
					{
						float damage = g_rng->RollRandomFloatInFloatRange(m_weaponDef->m_meleeDamage);
						enemies[i]->TakeDamage(damage, owner);
						if (m_weaponDef->m_meleeImpulse > 0.f)
						{
							CalculateAndApplyImpulse(owner, enemies[j], m_weaponDef->m_meleeImpulse);
						}
					}
				}
			}
		} break;
		}
		m_weaponTimer->Restart();
		return enemies; // could tell which actor the weapon has hit for later VFX
	}
	else
	{
		return enemies;
	}
}

void Weapon::DetectLockingTarget(Player* player, Actor* playerActor)
{
	if (playerActor)
	{
		Map* ownerMap = playerActor->m_map;
		ActorPtrList enemies = ownerMap->GetActorsExceptSelf(playerActor);

		// get deflect shooting normal
		Vec3 rayStart = playerActor->GetRaycastShootingPosition();
		Vec3 fwdNormal = playerActor->m_controller->GetAimingDirection();

		// update the target that current weapon is locking
		float shootRange = m_weaponDef->m_rayRange;
		m_lockingTarget = playerActor->m_map->LockingTestForLockOnWeaponFiring(rayStart, fwdNormal, shootRange, playerActor, enemies, m_lockingTimer, player);

		// change the weapon according to whether the locking target is acquired
		if (m_lockingTarget)
		{
			m_lockingStatus = WeaponLockingStatus::LOCKING;

			m_missingTimer->Stop();

			// when reloaded and aiming aligned, start the locking timer
			if (m_lockingTimer->IsStopped() && m_aimingAligned && m_weaponTimer->HasPeroidElapsed())
			{
				m_lockingTimer->Start();
			}

			// save the locking target when timer is past
			if (m_lockingTimer->HasPeroidElapsed() && m_aimingAligned)
			{
				m_lockingStatus = WeaponLockingStatus::LOCKED;
				m_lockingTarget = m_lockingTarget;
				// when locked, the weapon remember the target
				m_savedTarget = m_lockingTarget;
			}

			// if the aiming is off, do not let player shoot
			if (m_lockingStatus == WeaponLockingStatus::LOCKED && !m_aimingAligned)
			{
				m_lockingStatus = WeaponLockingStatus::LOCKING;
			}

			// if there is a saved target and the aiming is aligned again, allowing shooting again
			if (m_lockingStatus == WeaponLockingStatus::LOCKING && m_aimingAligned && m_savedTarget == m_lockingTarget)
			{
				m_lockingStatus = WeaponLockingStatus::LOCKED;
				m_savedTarget = m_lockingTarget;
			}

			// if the aiming is not aligned, do not let it shoot
			if (!m_aimingAligned)
			{
				m_lockingStatus = WeaponLockingStatus::LOCKING;
			}

			// if the refire time is not ready, change the status to invalid
			if (!m_weaponTimer->HasPeroidElapsed())
			{
				m_lockingStatus = WeaponLockingStatus::INVALID;
			}
		}
		else 
		{
			m_lockingTimer->Stop();

			// if locking target is valid, use this saved locking target
			if (m_savedTarget)
			{
				if (!m_savedTarget->m_isDead)
				{
					for (int i = 0; i < (int)g_theGame->m_playersList.size(); ++i)
					{
						Player*& playerController = g_theGame->m_playersList[i];
						if (playerController == player)
						{
							playerController->m_inLockingAimingMode = true;
							Vec3 hittingTarget = m_savedTarget->m_position + Vec3(0.f, 0.f, m_savedTarget->m_actorDef->m_eyeHeight * 0.5f);
							playerController->m_lockTargetPos = playerController->m_worldCamera.GetViewportNormolizedPositionForWorldPosition(hittingTarget);
						}
					}
				}
			}
			else // if there is saved target, move back to center
			{
				for (int i = 0; i < (int)g_theGame->m_playersList.size(); ++i)
				{
					Player*& playerController = g_theGame->m_playersList[i];
					if (playerController == player)
					{
						playerController->m_lockTargetPos = Vec2(0.5f, 0.5f);
					}
				}
			}

			// timer starts, target lost when missing timers elapsed
			if (m_missingTimer->IsStopped())
			{
				m_missingTimer->Start();
			}
			if (m_missingTimer->HasPeroidElapsed())
			{
				m_savedTarget = nullptr;
				m_missingTimer->Stop();
				m_lockingStatus = WeaponLockingStatus::INVALID;
			}
		}
	}

	UpdateWeaponLockingSound(playerActor);
}

void Weapon::UpdateWeaponLockingSound(Actor* actor)
{
	if (m_lockingStatus == WeaponLockingStatus::INVALID || g_theGame->m_currentState != GameState::PLAYING || actor->GetEquippedWeapon()->m_weaponDef->m_weaponName != m_weaponDef->m_weaponName || g_theGameClock->IsPaused())
	{
		if (g_theAudio->IsPlaying(m_lockedSoundPlaybackID))
		{
			g_theAudio->StopSound(m_lockedSoundPlaybackID);
		}
		if (g_theAudio->IsPlaying(m_lockingSoundPlaybackID))
		{
			g_theAudio->StopSound(m_lockingSoundPlaybackID);
		}
	}

	else if (m_lockingStatus == WeaponLockingStatus::LOCKING)
	{
		if (g_theAudio->IsPlaying(m_lockedSoundPlaybackID))
		{
			g_theAudio->StopSound(m_lockedSoundPlaybackID);
		}		
		if (g_theAudio->IsPlaying(m_lockingSoundPlaybackID))
		{
			// if is playing already, do not start a new one, but update sound position
			g_theAudio->SetSoundPosition(m_lockingSoundPlaybackID, actor->m_position);
		}
		else
		{
			m_lockingSoundPlaybackID = g_theAudio->StartSoundAt(m_weaponDef->m_lockingID, actor->m_position);
		}
	}

	else if (m_lockingStatus == WeaponLockingStatus::LOCKED)
	{
		if (g_theAudio->IsPlaying(m_lockingSoundPlaybackID))
		{
			g_theAudio->StopSound(m_lockingSoundPlaybackID);
		}
		if (g_theAudio->IsPlaying(m_lockedSoundPlaybackID))
		{
			// if is playing already, do not start a new one, but update sound position
			g_theAudio->SetSoundPosition(m_lockedSoundPlaybackID, actor->m_position);
		}
		else
		{
			m_lockedSoundPlaybackID = g_theAudio->StartSoundAt(m_weaponDef->m_lockedID, actor->m_position);
		}
	}
}

void Weapon::CalculateAndApplyImpulse(Actor* attacker, Actor* defender, float impulseValue)
{
	Vec3 disp = defender->m_position - attacker->m_position;
	Vec3 impulseDirection = disp.GetNormalized();
	Mat44 worldToLocal = defender->GetModelMatrix().GetOrthonormalInverse();
	Vec3 impulse = worldToLocal.TransformVectorQuantity3D(impulseDirection * impulseValue);
	defender->AddImpulse(impulse);
}

Vec3 Weapon::GetRandomDirectionInCone(Vec3 direction, float range)
{
	FloatRange yawRange(-range, range);
	FloatRange pitchRange(-range, range);

	float yawDeflection = g_rng->RollRandomFloatInFloatRange(yawRange);
	float pitchDeflection = g_rng->RollRandomFloatInFloatRange(pitchRange);

	EulerAngles orientation = direction.GetOrientation();
	orientation.m_yawDegrees += yawDeflection;
	orientation.m_pitchDegrees += pitchDeflection;

	return Vec3::GetDirectionForYawPitch(orientation.m_yawDegrees, orientation.m_pitchDegrees);
}
