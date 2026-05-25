#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Game/Actor.hpp"
#include "Game/Player.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"

extern RandomNumberGenerator* g_rng;
extern Game* g_theGame;
extern App* g_theApp;
extern Renderer* g_theRenderer;
extern Clock* g_theGameClock;
extern AudioSystem* g_theAudio;

std::vector<ActorDefinition*> ActorDefinition::s_actorDefs;

//----------------------------------------------------------------------------------------------------------------------------------------------------
void ActorDefinition::InitializeActorDefs(char const* filePath)
{
	XmlDocument actorDefXml;
	XmlResult result = actorDefXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("failed to load actor definitions xml file"));

	XmlElement* rootElement = actorDefXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "actor definition root Element is nullPtr");

	XmlElement* actorDefElement = rootElement->FirstChildElement();
	// XmlElement* spawnInfoElement = mapDefElement->FirstChildElement();

	while (actorDefElement)
	{
		// read map info
		std::string elementName = actorDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "ActorDefinition", Stringf("root cant matchup with the name of \"ActorDefinition\""));
		ActorDefinition* newActorDef = new ActorDefinition(actorDefElement);// calls the constructor function of TileTypeDefinition
		s_actorDefs.push_back(newActorDef);

		actorDefElement = actorDefElement->NextSiblingElement();
	}
}

ActorDefinition::ActorDefinition(XmlElement* actorDefElement)
{
	// action definition
	m_actorName = ParseXmlAttribute(*actorDefElement, "name", "Named actor not found");
	if (m_actorName == "SpawnPoint") // spawnPoint does not have following settings in the XML
	{
		return;
	}

	std::string name = ParseXmlAttribute(*actorDefElement, "faction", "Named faction not found");
	if (name != "Named faction not found") // bullet hit and blood hit has no faction
	{
		m_faction = ActorDefinition::GetActorFactionByString(name);
	}

	m_health = ParseXmlAttribute(*actorDefElement, "health", 0.f);
	m_canBePossessed = ParseXmlAttribute(*actorDefElement, "canBePossessed", false);
	m_corpseLifetime = ParseXmlAttribute(*actorDefElement, "corpseLifetime", 0.f);
	m_visible = ParseXmlAttribute(*actorDefElement, "visible", false);
	m_invisibleDuration = ParseXmlAttribute(*actorDefElement, "invisibleDuration", 0.f);
	m_teleportDist = ParseXmlAttribute(*actorDefElement, "teleportDistance", 0.f);
	m_dieOnSpawn = ParseXmlAttribute(*actorDefElement, "dieOnSpawn", false);

	// Gold project
	m_isShielded = ParseXmlAttribute(*actorDefElement, "isShielded", false);
	m_bulletproof = ParseXmlAttribute(*actorDefElement, "bulletproof", false);
	m_isProjectile = ParseXmlAttribute(*actorDefElement, "isProjectile", false);

	// collision
	XmlElement* collisionElement = actorDefElement->FirstChildElement("Collision");
	if (collisionElement)
	{
		m_physicsRadius = ParseXmlAttribute(*collisionElement, "radius", 0.f);
		m_physicsFootHeight = ParseXmlAttribute(*collisionElement, "footHeight", 0.f);
		m_physicsHeight = ParseXmlAttribute(*collisionElement, "height", 0.f);
		m_collidesWithWorld = ParseXmlAttribute(*collisionElement, "collidesWithWorld", false);
		m_collidesWithActors = ParseXmlAttribute(*collisionElement, "collidesWithActors", false);

		// for projectile actor
		m_damageOnCollide = ParseXmlAttribute(*collisionElement, "damageOnCollide", FloatRange());
		m_impulseOnCollide = ParseXmlAttribute(*collisionElement, "impulseOnCollide", 0.f);
		m_dieOnCollide = ParseXmlAttribute(*collisionElement, "dieOnCollide", false);
	}

	// physics
	XmlElement* physicsElement = actorDefElement->FirstChildElement("Physics");
	if (physicsElement)
	{
		m_simulated = ParseXmlAttribute(*physicsElement, "simulated", false);
		m_isHomingMissile = ParseXmlAttribute(*physicsElement, "isHomingMissile", false);
		m_walkSpeed = ParseXmlAttribute(*physicsElement, "walkSpeed", 0.f);
		m_runSpeed = ParseXmlAttribute(*physicsElement, "runSpeed", 0.f);
		m_turnSpeed = ParseXmlAttribute(*physicsElement, "turnSpeed", 0.f);
		m_flying = ParseXmlAttribute(*physicsElement, "flying", false);
		m_drag = ParseXmlAttribute(*physicsElement, "drag", 0.f);
	}

	// if it could not be possessed, it is a blood hit or bullet hit or a projectile
	// following settings will be existed in XML only if this action is able to be possessed, which means it is an actor like demon or marine
	if (m_canBePossessed)
	{
		// camera
		XmlElement* cameraElement = actorDefElement->FirstChildElement("Camera");
		name = cameraElement->Name();
		if (cameraElement)
		{
			m_eyeHeight = ParseXmlAttribute(*cameraElement, "eyeHeight", 0.f);
			m_cameraFOVDegrees = ParseXmlAttribute(*cameraElement, "cameraFOV", 0.f);
		}
	}
	// visuals: empty for this assignment
	XmlElement* aiElement = actorDefElement->FirstChildElement("AI");
	if (aiElement) // Enemy actor has AI settings
	{
		m_aiEnabled = ParseXmlAttribute(*aiElement, "aiEnabled", false);
		m_sightRadius = ParseXmlAttribute(*aiElement, "sightRadius", 0.f);
		m_sightAngle = ParseXmlAttribute(*aiElement, "sightAngle", 0.f);
	}

	XmlElement* visualsElement = actorDefElement->FirstChildElement("Visuals");
	if (visualsElement)
	{
		m_size = ParseXmlAttribute(*visualsElement, "size", Vec2(1.f, 1.f));
		m_pivot = ParseXmlAttribute(*visualsElement, "pivot", Vec2(0.5f, 0.5f));

		std::string billboardType = ParseXmlAttribute(*visualsElement, "billboardType", "Not found in Xml");
		m_billboardType = GetBillboardTypeFromString(billboardType);

		m_renderLit = ParseXmlAttribute(*visualsElement, "renderLit", false);
		m_renderRounded = ParseXmlAttribute(*visualsElement, "renderRounded", false);

		std::string shaderPath = ParseXmlAttribute(*visualsElement, "shader", "Not found in Xml");
		m_shader = g_theRenderer->CreateOrGetShader(shaderPath.c_str());

		// get sprite sheet texture, cell count and sprite sheet
		std::string spriteSheetPath = ParseXmlAttribute(*visualsElement, "spriteSheet", "Not found in Xml");
		Texture* spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile(spriteSheetPath.c_str());

		m_cellCount = ParseXmlAttribute(*visualsElement, "cellCount", IntVec2(0, 0));
		m_spriteSheet = new SpriteSheet(*spriteSheetTexture, m_cellCount);
	}
	// AnimationGroup
	// actorDefElement = actorDefElement->NextSiblingElement();
	XmlElement* animationGroupElement = visualsElement->FirstChildElement("AnimationGroup");
	// name = animationGroupElement->Name();

	while (animationGroupElement)
	{
		SpriteAnimationGroupDefinition* newAnimGroupDef = new SpriteAnimationGroupDefinition();
		newAnimGroupDef->m_name = ParseXmlAttribute(*animationGroupElement, "name", "Not found in Xml");
		newAnimGroupDef->m_scaleBySpeed = ParseXmlAttribute(*animationGroupElement, "scaleBySpeed", false);
		newAnimGroupDef->m_secondsPerFrame = ParseXmlAttribute(*animationGroupElement, "secondsPerFrame", 0.f);
		std::string animPlayMode = ParseXmlAttribute(*animationGroupElement, "playbackMode", "Not found in Xml");
		newAnimGroupDef->m_playbackMode = SpriteAnimDefinition::GetAnimPlaybackModeByString(animPlayMode);

		// Direction
		XmlElement* directionElement = animationGroupElement->FirstChildElement();
		// name = directionElement->Name();

		while (directionElement)
		{
			Vec3 direction = ParseXmlAttribute(*directionElement, "vector", Vec3());
			direction = direction.GetNormalized();
			newAnimGroupDef->m_directions.push_back(direction);

			// Animation
			XmlElement* animationElement = directionElement->FirstChildElement();
			name = animationElement->Name();

			int startFrame = ParseXmlAttribute(*animationElement, "startFrame", 0);
			int endFrame = ParseXmlAttribute(*animationElement, "endFrame", 0);
			float animDuration = (endFrame - startFrame + 1) * newAnimGroupDef->m_secondsPerFrame;

			SpriteAnimDefinition* newAnim = new SpriteAnimDefinition(*m_spriteSheet, startFrame, endFrame, animDuration, newAnimGroupDef->m_playbackMode);
			newAnimGroupDef->m_spriteAnimDefs.push_back(newAnim);
			// newAnimGroupDef.m_animations[direction] = newAnim; 

			// next direction
			directionElement = directionElement->NextSiblingElement();
		}

		m_spriteAnimGroupDefs.push_back(newAnimGroupDef);
		// if there is no sibling of the direction, we continue to next animation group
		animationGroupElement = animationGroupElement->NextSiblingElement();
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// if there is no sibling of the animation group, we continue to sounds
	XmlElement* soundsElement = actorDefElement->FirstChildElement("Sounds");
	if (soundsElement)
	{
		// if we got sounds, we are going to read all its child sound info		
		// sound
		XmlElement* soundElement = soundsElement->FirstChildElement();
		name = soundElement->Name();
		while (soundElement)
		{
			std::string soundName = ParseXmlAttribute(*soundElement, "sound", "Not found in Xml");
			std::string soundLoadPath = ParseXmlAttribute(*soundElement, "name", "Not found in Xml");

			if (soundName == "Hurt")
			{
				m_hurtSoundID = g_theAudio->CreateOrGetSound(soundLoadPath, true);
			}
			if (soundName == "Death")
			{
				m_deathSoundID = g_theAudio->CreateOrGetSound(soundLoadPath, true);
			}
			if (soundName == "Active")
			{
				m_activeSoundID = g_theAudio->CreateOrGetSound(soundLoadPath, true);
			}

			soundElement = soundElement->NextSiblingElement();
		}
	}

	// Inventory
	XmlElement* inventoryElement = actorDefElement->FirstChildElement("Inventory");
	if (inventoryElement) // if we got sounds, we are going to read all its child sound info
	{
		// weapon
		XmlElement* weaponElement = inventoryElement->FirstChildElement();
		name = weaponElement->Name();
		while (weaponElement)
		{
			name = ParseXmlAttribute(*weaponElement, "name", "Named actor not found");
			WeaponDefinition* weaponDef = WeaponDefinition::GetWeaponDefByString(name);
			m_weapons.push_back(weaponDef);
			weaponElement = weaponElement->NextSiblingElement();
		}
	}

	// all info is read through, get the next actor definition
	return;
}

ActorDefinition* ActorDefinition::GetActorDefByString(std::string str)
{
	for (int i = 0; i < (int)s_actorDefs.size(); i++)
	{
		if (s_actorDefs[i]->m_actorName == str)
		{
			return s_actorDefs[i];
		}
	}

	ERROR_AND_DIE("Enum type is not defined in Actor type enum class");
}

ActorFaction ActorDefinition::GetActorFactionByString(std::string str)
{
	if (str == "Marine") { return ActorFaction::MARINE; }
	if (str == "Demon") { return ActorFaction::DEMON; }

	ERROR_AND_DIE("Enum type is not defined in Actor faction enum class");
}


Actor::Actor(Map* owner, Vec3 startPos, EulerAngles orientation, Rgba8 color, ZCylinder collision, bool isStatic /*= true*/)
	: m_map(owner)
	, m_position(startPos)
	, m_orientation(orientation)
	, m_color(color)
	, m_collision(collision)
	, m_isStatic(isStatic)
{
	m_game = g_theGame;

	// InitializeLocalVerts();
}

Actor::Actor(SpawnInfo spawnInfo, Map* map, ActorUID uid)
	: m_map(map)
	, m_actorUID(uid)
{
	m_actorUID = uid;

	// actor def will match with the actor def in the spawn info
	m_actorDef = spawnInfo.m_actorDef;
	m_health = m_actorDef->m_health;

	// animation set up
	m_animationClock = new Clock(*g_theGameClock);
	m_spriteAnimGroupDefs = spawnInfo.m_actorDef->m_spriteAnimGroupDefs;

	// based on the actor def, create new weapon for the actor, 
	// because each weapon has its own reloading and status, could not be reuse for different actors
	// Create all weapons in our actor definition, add them to our inventory
	// set our equipped weapon to be the first one in the list
	if (!m_actorDef->m_weapons.empty())
	{
		for (int i = 0; i < (int)m_actorDef->m_weapons.size(); i++)
		{
			Weapon* weapon = new Weapon(m_actorDef->m_weapons[i]);
			m_weapons.push_back(weapon);
		}
	}

	m_position = spawnInfo.m_position;
	m_orientation = spawnInfo.m_orientation;
	m_velocity = spawnInfo.m_velocity;

	m_lifetimeTimer = new Timer(m_actorDef->m_corpseLifetime, g_theGameClock);
}

Actor::~Actor()
{

}

void Actor::Startup()
{
	InitializeCollision();

	if (m_actorDef->m_dieOnSpawn)
	{
		m_isDead = true;
		m_lasFrameState = ActorAnimState::DEATH;
		PlayAnimation(ActorAnimState::DEATH);
	}

	// for gold assignment
	if (m_actorDef->m_invisibleDuration != 0.f)
	{
		m_invisibleTimer = new Timer(m_actorDef->m_invisibleDuration, g_theGameClock);
	}
}

void Actor::Update()
{
	// update collision's position to origin center
	// m_collision.CenterXY.x = m_position.x;
	// m_collision.CenterXY.y = m_position.y;

	// // update the actor position according to its collision
	// Vec3 collisionPos(m_collision.CenterXY, m_collision.MinMaxZ.m_min); // world space movement
	// m_position += (GetModelMatrix().TransformVectorQuantity3D(collisionPos) - m_collisionOffset); // collision movement in local space
	// 
	// // after move actor position according to collision, reset collision back to center
	// m_collision.CenterXY.x = m_collisionOffset.x;
	// m_collision.CenterXY.y = m_collisionOffset.y;
	// float cylinderHeight = m_collision.MinMaxZ.GetRangeLength();
	// m_collision.MinMaxZ.m_min = m_collisionOffset.z;
	// m_collision.MinMaxZ.m_max = m_collisionOffset.z + cylinderHeight;

	// update its AIController when it is currently controlling the actor
	// if it is player controller, do not update, we update player controller separately
	if (m_controller)
	{
		if (m_controller == m_AIController)
		{
			m_controller->Update(); // AI controller
		}
	}
	UpdatePhysics();
	UpdateDeathDestroyedStatus();
	UpdateAnimClock();
	UpdateAudio();
}

void Actor::UpdatePhysics()
{
	float deltaSeconds = g_theGameClock->GetDeltaSeconds();
	if (m_isDead)
	{
		return;
	}

	// teleport whenever was hit
	if (m_invisibleTimer)
	{
		float animDuration = GetAnimDurationForActorState(ActorAnimState::HURT);
		// if the timer ends, continue playing
		if (!m_invisibleTimer->IsStopped())
		{
			// get invisible after playing the hurt animation, if it is not dead
			if (m_invisibleTimer->GetElapsedTime() > animDuration)
			{
				if (!m_hasTeleported)
				{
					TeleportToRandomDirection();
				}
			}

			if (m_invisibleTimer->HasPeroidElapsed())
			{
				m_invisibleTimer->Stop();
			}
		}
	}

	// if (m_actorDef->m_actorName == "Demon")
	// {
	// 	std::string lockingTimer = Stringf("x= %.2f, y= %.2f", m_position.x, m_position.y);
	// 	DebugAddScreenText(lockingTimer, Vec2(0.f, 0.f), 24.f, Vec2(0.f, 0.4f), -1.f);
	// }

	// Perform physics processing if the actor is simulated
	if (m_actorDef->m_simulated)
	{
		// Add a drag force equal to our drag times our negative current velocity
		m_acceleration += m_actorDef->m_drag * (m_velocity * (-1.f));

		m_velocity += m_acceleration * deltaSeconds;

		// Set velocity Z-components to zero for non-flying actors
		if (!m_actorDef->m_flying)
		{
			// m_position.z = 0.f; // keep the actor on the ground
			m_velocity.z = 0.f;
		}
		m_position += m_velocity * deltaSeconds;

		// clear out acceleration for next frame
		m_acceleration = Vec3();
	}
	else 
	{
		// if this is a shield
		if (m_actorDef->m_bulletproof)
		{
			// update the shield info according to its owner
			Actor* shieldedActor = m_map->GetActorByUID(m_shieldOwnerID);
			if (shieldedActor)
			{
				if (shieldedActor->m_isDead)
				{
					m_isDead = true;
					m_lifetimeTimer->Start();
				
					PlayAnimation(ActorAnimState::DEATH);
					PlaySound(m_actorDef->m_deathSoundID);
				}
				else
				{
					m_orientation = shieldedActor->m_orientation;
					m_position = shieldedActor->m_position + m_orientation.GetForwardIBasis() * shieldedActor->m_collision.Radius;
				}
			}
		}

		// this is a homing missile
		if (m_actorDef->m_isHomingMissile)
		{
			// if the locked target exist, then fix the trajectory and move again
			if (m_lockedTarget)
			{
				// update trajectory
				float dist = GetDistance3D(m_position, m_lockedTarget->m_position);
				float flyingDuration = dist / m_flyingSpeed;
				ConstructProjectileTrajectory(m_position, m_lockedTarget, flyingDuration, m_flyingSpeed * 1.f);

				UpdateMissilePositionOnTrajectory();
			}
		}
	}
	
	// Debug velocity
	//if (m_actorDef->m_actorName == "Marine")
	//{
	//	std::string velocity = Stringf("velocity: %.2f", m_velocity.GetLength());
	//	DebugAddScreenText(velocity, Vec2(0.f, 0.5f), 24.f, Vec2(), -1.f);
	//}
}

void Actor::UpdateDeathDestroyedStatus()
{
	if (m_lifetimeTimer->IsStopped() && m_isDead)
	{
		// PlayAnimation(ActorAnimState::DEATH);
		m_lifetimeTimer->Start();
	}
	if (m_lifetimeTimer->HasPeroidElapsed())
	{
		m_isDestroyed = true;
		m_lifetimeTimer->Stop();
	}
}

void Actor::ShutDown()
{
	delete m_controller;
	delete m_AIController;
	delete m_lifetimeTimer;
}

ZCylinder Actor::GetCylinderCollisionInWorldSpace() const
{
	ZCylinder cylinderInWorld;
	cylinderInWorld.CenterXY = Vec2(m_position);
	cylinderInWorld.Radius = m_collision.Radius;
	Vec3 baseHeight = m_position + GetModelMatrix().TransformVectorQuantity3D(Vec3(0.f, 0.f, m_collision.MinMaxZ.m_min));
	Vec3 topHeight = m_position + GetModelMatrix().TransformVectorQuantity3D(Vec3(0.f, 0.f, m_collision.MinMaxZ.m_max));
	cylinderInWorld.MinMaxZ.m_min = baseHeight.z;
	cylinderInWorld.MinMaxZ.m_max = topHeight.z;
	return cylinderInWorld;
}

void Actor::Render() const
{
	// do not render the character when the player controller is controlling, but the render of current weapon
	Player* playerController = dynamic_cast<Player*>(m_controller);

	if (playerController)
	{
		// specifically in FPS camera mode, not in free fly mode, we want to see and check the character
		if (!playerController->m_freeFlyCamera && playerController == g_theGame->m_currentRenderPlayerController) // but if the actor's player controller is the current rendering player controller's camera, skip rendering
		{
			return;
		}
	}

	// do no render an actor when it is not visible
	if (!m_actorDef->m_visible)
	{
		return;
	}

	InitializeQuadVertsAndRender();
}

void Actor::InitializeCollision()
{
	m_collision.Radius = m_actorDef->m_physicsRadius;
	m_collision.MinMaxZ = FloatRange(m_actorDef->m_physicsFootHeight, m_actorDef->m_physicsHeight);
}

void Actor::InitializeQuadVertsAndRender() const
{
	std::vector<Vertex_PCU>		vertex_PCUs;
	std::vector<Vertex_PCUTBN>  vertex_PCUTBNs;

	// Debug mode will draw the collision
	if (g_theApp->m_debugMode)
	{
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetModelConstants(GetRenderModelMatrix());
		g_theRenderer->BindShader(m_actorDef->m_shader);

		// render cylinder
		Vec3 cylinderStart = Vec3(m_collision.CenterXY, m_collision.MinMaxZ.m_min);
		Vec3 cylinderEnd = Vec3(m_collision.CenterXY, m_collision.MinMaxZ.m_max);
		AddVertsForCylinder3D(vertex_PCUs, cylinderStart, cylinderEnd, m_collision.Radius);
		Vec3 noseStart = cylinderStart + Vec3(m_actorDef->m_physicsRadius, 0.f, m_actorDef->m_eyeHeight);
		Vec3 noseEnd = noseStart + Vec3(m_actorDef->m_physicsRadius * 0.25f, 0.f, 0.f);
		AddVertsForCone3D(vertex_PCUs, noseStart, noseEnd, m_collision.Radius * 0.25f);

		if (!m_isDead)
		{
			// hard coded color for game mechanics 
			Rgba8 cylinderColor;
			Rgba8 wireframeColor = Rgba8::WHITE;
			if (m_actorDef->m_actorName == "Marine")
			{
				cylinderColor = Rgba8::GREEN;
			}
			if (m_actorDef->m_actorName == "Demon")
			{
				cylinderColor = Rgba8::RED;
			}
			if (m_actorDef->m_actorName == "PlasmaProjectile")
			{
				cylinderColor = Rgba8::BLUE;
			}

			// cylinder
			g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
			g_theRenderer->SetModelConstants(GetRenderModelMatrix(), cylinderColor);
			g_theRenderer->DrawVertexArray((int)vertex_PCUs.size(), vertex_PCUs.data());

			// white Wireframe
			g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
			g_theRenderer->SetModelConstants(GetRenderModelMatrix(), wireframeColor);
			g_theRenderer->DrawVertexArray((int)vertex_PCUs.size(), vertex_PCUs.data());
		}
		else
		{
			// cylinder
			g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
			g_theRenderer->SetModelConstants(GetRenderModelMatrix(), m_actorDef->m_solidColor);
			g_theRenderer->DrawVertexArray((int)vertex_PCUs.size(), vertex_PCUs.data());

			// white Wireframe
			g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
			g_theRenderer->SetModelConstants(GetRenderModelMatrix(), m_actorDef->m_wireframeColor);
			g_theRenderer->DrawVertexArray((int)vertex_PCUs.size(), vertex_PCUs.data());
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// sprite render
	// if the enemy is currently in invisible status, then we do not need to render
	if (m_invisibleTimer)
	{

		// if the timer ends, continue playing
		if (!m_invisibleTimer->IsStopped())
		{
			float animDuration = GetAnimDurationForActorState(ActorAnimState::HURT);

			// get invisible after playing the hurt animation, if it is not dead
			if (m_invisibleTimer->GetElapsedTime() > animDuration && !m_isDead)
			{
				return;
			}
		}
	}
	
	// create quad
	// get four corner points for the quad
	float width = m_actorDef->m_size.x;
	float height = m_actorDef->m_size.y;
	Vec3 BL = Vec3();
	Vec3 BR = Vec3(0.f, width, 0.f);
	Vec3 TL = Vec3(0.f, 0.f, height);
	Vec3 TR = Vec3(0.f, width, height);

	// move all those points according to the pivot point and
	float shiftOnY = m_actorDef->m_pivot.x;
	float shiftOnZ = m_actorDef->m_pivot.y;

	Vec3 pivotShift = Vec3(0.f, shiftOnY * width, shiftOnZ * height) * (-1.f);
	BL += pivotShift;
	BR += pivotShift;
	TL += pivotShift;
	TR += pivotShift;

	SpriteDefinition spriteDef = GetSpriteDefForCurrentState();
	AABB2 uvBounds = spriteDef.GetUVs();

	// debug sprite index
	//if (m_actorDef->m_actorName == "EnergyShield" && g_theApp->m_debugMode)
	//{
	//	std::string spriteIndex = Stringf("1st Player SpriteIndex: %i", spriteDef.GetSpriteIndex());
	//	DebugAddScreenText(spriteIndex, Vec2(0.f, 0.f), 24.f, Vec2(0.f, 0.8f), -1.f);
	//}

	if (m_actorDef->m_renderRounded)
	{
		// create rounded double quad 
		AddVertsForRoundedQuad3D(vertex_PCUTBNs, TL, BL, BR, TR, Rgba8::WHITE, uvBounds);
	}
	else
	{
		// create a regular flat quad
		AddVertsForQuad3D(vertex_PCUs, BL, BR, TR, TL, Rgba8::WHITE, uvBounds);
	}

	if (m_actorDef->m_bulletproof)
	{
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	}
	else
	{
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	}

	// when being listened, disable the depth mode
	if (m_isListenedByPlayer)
	{
		g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	}
	else
	{
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	}

	// get the sprite texture for the actor to render
	Texture& texture = spriteDef.GetTexture();
	g_theRenderer->BindTexture(&texture);

	// if the quad is a billboard, get transform info
	if (m_actorDef->m_billboardType != BillboardType::NONE)
	{
		Mat44 cameraMatrix = g_theGame->m_currentRenderPlayerController->m_worldCamera.GetModelMatrix();
		Mat44 modelMatrix = GetBillboardMatrix(m_actorDef->m_billboardType, cameraMatrix, m_position);

		// listen mode will give it a red tint
		if (m_isListenedByPlayer)
		{
			g_theRenderer->SetModelConstants(modelMatrix, Rgba8::RED);
		}
		else
		{
			g_theRenderer->SetModelConstants(modelMatrix);
		}
	}
	else
	{
		if (m_isListenedByPlayer)
		{
			g_theRenderer->SetModelConstants(GetRenderModelMatrix(), Rgba8::RED);
		}
		else
		{
			g_theRenderer->SetModelConstants(GetRenderModelMatrix());
		}
	}

	// set lighting constant and shader
	if (m_actorDef->m_renderLit)
	{
		g_theRenderer->SetLightingConstants(*m_map->m_mapLightingSettings);
	}
	g_theRenderer->BindShader(m_actorDef->m_shader);

	// render different kind of vertex array
	if (m_actorDef->m_renderRounded)
	{
		g_theRenderer->DrawVertexArray((int)vertex_PCUTBNs.size(), vertex_PCUTBNs.data());
	}
	else
	{
		g_theRenderer->DrawVertexArray((int)vertex_PCUs.size(), vertex_PCUs.data());
	}

}

Mat44 Actor::GetModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);
	Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	// orientationMat.Append(transformMat);
	// return orientationMat;
	transformMat.Append(orientationMat);
	return transformMat;
}

Mat44 Actor::GetRenderModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);

	EulerAngles renderOrientation = m_orientation;
	renderOrientation.m_pitchDegrees = 0.f;
	Mat44 orientationMat = renderOrientation.GetAsMatrix_XFwd_YLeft_ZUp();
	// orientationMat.Append(transformMat);
	// return orientationMat;
	transformMat.Append(orientationMat);
	return transformMat;
}

Vec3 Actor::GetForwardNormal() const
{
	return GetModelMatrix().TransformVectorQuantity3D(Vec3(1.f, 0.f, 0.f));
}

void Actor::TakeDamage(float damage, Actor* source)
{
	if (m_actorDef->m_actorName == "Marine") // tell the player controller to show hit indicator on screen
	{
		for (int i = 0; i < (int)g_theGame->m_playersList.size(); ++i)
		{
			Actor* hitPlayer = m_map->GetActorByUID(g_theGame->m_playersList[i]->m_actorUID);
			if (hitPlayer == this)
			{
				g_theGame->m_playersList[i]->PlayerGotHit();
			}
		}
	}

	if (m_controller)
	{
		if (m_controller == m_AIController) // tell the hit enemy which actor hits it
		{
			m_AIController->m_targetUID = source->m_actorUID;
		}
	}
	PlayAnimation(ActorAnimState::HURT);

	if (!m_actorDef->m_dieOnCollide)
	{
		m_health -= damage;

		if (m_health < 0.f && m_lifetimeTimer->IsStopped()) // we are not going to start the timer multiple times
		{
			m_isDead = true;
			m_lifetimeTimer->Start();

			PlayAnimation(ActorAnimState::DEATH);

			// for the source to increase its kill numbers
			if (m_actorDef->m_actorName == "Marine")
			{
				Player* sourcePlayerController = dynamic_cast<Player*>(source->m_controller);
				if (sourcePlayerController)
				{
					++sourcePlayerController->m_kills;
				}

				// for itself, increase its death
				Player* killedController = dynamic_cast<Player*>(m_controller);
				if (killedController)
				{
					++killedController->m_deaths;
				}
			}
		}
		else // if the actor is not killed
		{
			if (m_invisibleTimer)
			{
				if (m_invisibleTimer->IsStopped())
				{
					m_hasTeleported = false; // allow the actor to teleport
					m_invisibleTimer->Start();
				}
			}
		}
	}

	// sounds
	if (!m_isDead) // not dead, is hurt
	{
		PlaySound(m_actorDef->m_hurtSoundID);
	}
	else // dead
	{
		PlaySound(m_actorDef->m_deathSoundID);
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// physics
// Add a force to our acceleration to be used next frame. Must be called every frame to apply continual force over time.
void Actor::AddForce(const Vec3& force)
{
	m_acceleration += force;
}

void Actor::AddImpulse(const Vec3& impulse)
{
	m_velocity += impulse;
}

void Actor::OnCollide(Actor* other)
{
	//if (other)
	//{
	//	if (other->m_actorDef->m_bulletproof) // if the other is a shield
	//	{
	//		Vec2 impactNormal = GetCylinderCollisionInWorldSpace().CenterXY - other->GetCylinderCollisionInWorldSpace().CenterXY;
	//		Vec2 reflectVel = Vec2(m_velocity.x, m_velocity.y).Reflect(impactNormal);
	//		m_velocity.x = reflectVel.x;
	//		m_velocity.y = reflectVel.y;
	//	}
	//	else
	//	{
	//	}
	//}

	// I am a plasma, the other is a Demon
	if (m_actorDef->m_dieOnCollide && m_lifetimeTimer->IsStopped())
	{
		m_isDead = true;
		m_lifetimeTimer->Start();
		PlayAnimation(ActorAnimState::DEATH);
		return;
	}

	// I am a Demon, other is a plasma
	// Demon got impulse from plasma
	if (other) // because this might be the actor collide with the map
	{
		Actor* theOtherActorOwner = m_map->GetActorByUID(other->m_ownerUID);
		if (other->m_actorDef->m_impulseOnCollide)
		{
			if (other->m_ownerWeapon)
			{
				other->m_ownerWeapon->CalculateAndApplyImpulse(other, this, m_actorDef->m_impulseOnCollide);
			}		
		}
		// Demon got damage from plasma
		if (other->m_actorDef->m_damageOnCollide != FloatRange())
		{
			float damage = g_rng->RollRandomFloatInFloatRange(other->m_actorDef->m_damageOnCollide);
			TakeDamage(damage, theOtherActorOwner);
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// If the player possesses an actor with an AI controller, the AI controller is saved and then restored if the player Unpossess the actor.
void Actor::OnPossessed(Controller* controller)
{
	// if the coming possess controller is not a player controller, we are going to store this AI controller
	Player* playerController = dynamic_cast<Player*>(controller);
	m_controller = controller;
	if (!playerController) // if it is not player controller, we are going to store the AI controller
	{
		m_AIController = dynamic_cast<AIController*>(m_controller);
	}
}

void Actor::OnUnpossessed(Controller* controller)
{
	if (!controller)
	{
		return;
	}

	// if the actor has AI controller stored, UnPossessed process will let the AI controller take control again
	if (m_AIController)
	{
		m_controller = dynamic_cast<Controller*>(m_AIController);
		m_controller->m_actorUID = m_actorUID;
	}
	else
	{
		m_controller = nullptr;
	}

	controller->m_actorUID = ActorUID::INVALID;
}

void Actor::MoveInDirection(Vec3 direction, float desiredSpeed)
{
	// addforce = speed * m_definition->m_drag
	direction = direction.GetNormalized();
	AddForce(desiredSpeed * direction * (m_actorDef->m_drag));
}

void Actor::TeleportToRandomDirection()
{
	EulerAngles orientation = m_orientation;
	float yawDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
	orientation.m_yawDegrees = yawDegrees;
	Vec3 teleportDisp = orientation.GetForwardIBasis() * m_actorDef->m_teleportDist;
	m_position += teleportDisp;
	m_hasTeleported = true;
}

void Actor::TurnInDirection(float goalDegrees)
{
	float currentAngle = m_orientation.m_yawDegrees;
	float deltaSeconds = g_theGameClock->GetDeltaSeconds();
	float maxAngle = m_actorDef->m_turnSpeed * deltaSeconds;
	m_orientation.m_yawDegrees = GetTurnedTowardDegrees(currentAngle, goalDegrees, maxAngle);
}

ActorFaction Actor::GetActorFaction()
{
	return m_actorDef->m_faction;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
Weapon* Actor::GetEquippedWeapon()
{
	return m_weapons[m_equippedWeaponIndex];
}

void Actor::EquipNextWeapon()
{
	int numWeapon = (int)m_weapons.size();
	++m_equippedWeaponIndex;
	if (m_equippedWeaponIndex >= numWeapon)
	{
		m_equippedWeaponIndex = 0;
	}
}

void Actor::EquipPreviousWeapon()
{
	int numWeapon = (int)m_weapons.size();
	--m_equippedWeaponIndex;
	if (m_equippedWeaponIndex < 0)
	{
		m_equippedWeaponIndex = numWeapon - 1;
	}
}

void Actor::EquipWeapon(int weaponIndex)
{
	int numWeapon = (int)m_weapons.size();

	// if the input value matches with how many weapons we have, switch
	if (weaponIndex < numWeapon && weaponIndex >= 0)
	{
		m_equippedWeaponIndex = weaponIndex;
	}
}

void Actor::Attack()
{
	Weapon* currentUsingWeapon = GetEquippedWeapon();

	// fire current weapon
	if (currentUsingWeapon)
	{
		if (currentUsingWeapon->m_weaponDef->m_weaponType != WeaponType::LOCKING)
		{
			// see if the weapon is ready to fire, change the anim state to 
			if (currentUsingWeapon->m_weaponTimer->HasPeroidElapsed())
			{
				PlayAnimation(ActorAnimState::ATTACK);
				PlayWeaponSound(currentUsingWeapon->m_weaponDef->m_fireSoundID);
			}
		}

		// be to be after the check of period elapsed, otherwise the clock will be reset
		currentUsingWeapon->Fire(this);
	}

}

Vec3 Actor::GetRaycastShootingPosition() const
{
	Vec3 localPos = Vec3(m_actorDef->m_physicsRadius, m_rightHandOffset, (m_actorDef->m_eyeHeight * m_launchHeightMultiplier));
	Vec3 worldPos = GetModelMatrix().TransformPosition3D(localPos);
	return worldPos;
}

Vec3 Actor::GetPlasmaShootingPosition() const
{
	Vec3 localPos = Vec3(m_actorDef->m_physicsRadius * 1.5f, m_rightHandOffset, (m_actorDef->m_eyeHeight * m_launchHeightMultiplier));
	Vec3 worldPos = GetModelMatrix().TransformPosition3D(localPos);
	return worldPos;
}

std::string Actor::GetStringForActorState(ActorAnimState state) const
{
	// get string for current state
	std::string stateString;
	switch (state)
	{
	case ActorAnimState::WALK: {return stateString = "Walk"; } break;
	case ActorAnimState::ATTACK: {return stateString = "Attack"; } break;
	case ActorAnimState::HURT: {return stateString = "Hurt"; } break;
	case ActorAnimState::DEATH: {return stateString = "Death"; } break;
	}
	ERROR_AND_DIE("Missing case");
}

void Actor::UpdateAnimClock()
{
	if (!m_actorDef->m_visible)
	{
		return;
	}

	// however, if last state is death, no need update to any other animation
	if (m_lasFrameState == ActorAnimState::DEATH)
	{
		return;
	}

	// if the state changes, reset the anim clock
	if (m_currentState != m_lasFrameState)
	{
		// if the last attack animation is unfinished and new state is walking, do not switch to walking, continue to play attack animation
		// float period = GetAnimDurationForActorState(m_lasFrameState);
		// if (m_lasFrameState == ActorAnimState::ATTACK && m_currentState == ActorAnimState::WALK && !m_animationClock->HasTotalTimeElapsedPeriod(period))
		// {
		// 	m_currentState = ActorAnimState::ATTACK;
		// }
		// else
		// {
		// }
		m_animationClock->SetTimeScale(1.f);
		m_animationClock->Reset();
		m_lasFrameState = m_currentState;
		// if (m_actorDef->m_actorName == "Marine")
		// {
		// 	DebugAddMessage(GetStringForActorState(m_currentState), GetAnimDurationForActorState(m_currentState), Rgba8::WHITE, Rgba8(255, 255, 255, 100));
		// }
	}

	// if the animation has scale by speed, calculate the speed fraction
	// then set time scale of the animation clock to decelerate or accelerate the animation playing speed
	if (GetAnimGroupDefByActorState(m_currentState)->m_scaleBySpeed)
	{
		if (m_actorDef->m_runSpeed != 0.f)
		{
			float speedFaction = m_velocity.GetLength() / m_actorDef->m_runSpeed;
			m_animationClock->SetTimeScale(speedFaction);
		}
	}

	// if the current anim does not loop
	if (GetAnimGroupDefByActorState(m_currentState)->m_playbackMode == SpriteAnimPlaybackType::ONCE)
	{
		float period = GetAnimDurationForActorState(m_currentState);

		// and the play time has past, change back to default anim
		if (m_animationClock->HasTotalTimeElapsedPeriod(period))
		{
			std::string defaultAnimState = m_spriteAnimGroupDefs[0]->m_name;
			ActorAnimState defaultState = GetActorStateByString(defaultAnimState);

			if (m_currentState != ActorAnimState::DEATH) // change the anim state back to default only if current state is not death, because the actor there is corpose time
			{
				m_currentState = defaultState;
			}
		}
		else
		{
			return;
		}
	}
	// animation clock debug
	// if (m_actorDef->m_actorName == "EnergyShield" && g_theApp->m_debugMode)
	// {
	// 	std::string velocity = Stringf("1st Player AnimationClock: %.2f", m_animationClock->GetTotalSeconds());
	// 	DebugAddScreenText(velocity, Vec2(0.f, 0.f), 24.f, Vec2(0.f, 0.9f), -1.f);
	// }
	// if this anim do loop, just keep playing
	else return;
}

ActorAnimState Actor::GetActorStateByString(std::string name) const
{
	if (name == "Walk")
	{
		return ActorAnimState::WALK;
	}
	if (name == "Attack")
	{
		return ActorAnimState::ATTACK;
	}
	if (name == "Hurt")
	{
		return ActorAnimState::HURT;
	}
	if (name == "Death")
	{
		return ActorAnimState::DEATH;
	}

	ERROR_AND_DIE("Did not find actor state for the input name");
}

void Actor::PlayAnimation(ActorAnimState state)
{
	// change the current state, reset the clock time scale
	// don't change the state if it is already in the state
	if (m_currentState != state)
	{
		m_currentState = state;
		m_animationClock->SetTimeScale(1.f);
	}
}

// for one anim group def, all the sprite anim has the same duration
float Actor::GetAnimDurationForActorState(ActorAnimState state) const
{
	SpriteAnimationGroupDefinition* animGroup = GetAnimGroupDefByActorState(state);
	return animGroup->m_spriteAnimDefs[0]->GetDurationSeconds();
}

SpriteAnimationGroupDefinition* Actor::GetAnimGroupDefByActorState(ActorAnimState state) const
{
	// get string for current state
	std::string stateString = GetStringForActorState(state);

	// find the animation group according to current state string
	int numAnim = (int)m_spriteAnimGroupDefs.size();
	for (int i = 0; i < numAnim; ++i)
	{
		if (m_spriteAnimGroupDefs[i]->m_name == stateString)
		{
			SpriteAnimationGroupDefinition* animGroup = m_spriteAnimGroupDefs[i];
			return animGroup;
		}
	}

	ERROR_AND_DIE(Stringf("Has not found the matching animation group definition for the %s state", stateString.c_str()));
}

SpriteDefinition const& Actor::GetSpriteDefForCurrentState() const
{
	// get string for current state
	std::string stateString = GetStringForActorState(m_currentState);

	// find the animation group according to current state string
	int numAnim = (int)m_spriteAnimGroupDefs.size();
	for (int i = 0; i < numAnim; ++i)
	{
		if (m_spriteAnimGroupDefs[i]->m_name == stateString)
		{
			SpriteAnimationGroupDefinition* animGroup = m_spriteAnimGroupDefs[i];
			std::vector<SpriteAnimDefinition*>& spriteAnimDefs = animGroup->m_spriteAnimDefs;
			std::vector<Vec3> animDirections = animGroup->m_directions;

			// get the direction from camera to this actor and normalize it
			Vec3 direction = m_position - g_theGame->m_currentRenderPlayerController->m_position;
			direction = direction.GetNormalized();

			// transfer
			direction = GetModelMatrix().GetOrthonormalInverse().TransformVectorQuantity3D(direction);

			// calculate through all the anim direction and find the one with max value
			float maxValue = 0.f;
			int   animIndex = 0;
			for (int j = 0; j < (int)spriteAnimDefs.size(); ++j)
			{
				// for all the anim defined direction, first transform them from world space to local space

				float dotProductValue = DotProduct3D(direction, animDirections[j]);
				if (dotProductValue > maxValue)
				{
					maxValue = dotProductValue;
					animIndex = j;
				}
			}

			return spriteAnimDefs[animIndex]->GetSpriteDefAtTime(m_animationClock->GetTotalSeconds());
		}
	}

	ERROR_AND_DIE("Has no defined animation match up current state");
}

void Actor::UpdateAudio()
{
	// when the actor is being listened, play active sound
	if (m_isListenedByPlayer)
	{
		if (!g_theAudio->IsPlaying(m_soundPlaybackID))
		{
			PlaySound(m_actorDef->m_activeSoundID);
		}
	}

	// it the sound is still playing, then adjusted position every frame
	if (g_theAudio->IsPlaying(m_soundPlaybackID))
	{
		g_theAudio->SetSoundPosition(m_soundPlaybackID, m_position);
	}

	// shut off locking weapon sound
	for (int i = 0; i < (int)m_weapons.size(); i++)
	{
		m_weapons[i]->UpdateWeaponLockingSound(this);
	}
}

void Actor::PlaySound(SoundID soundID)
{
	// if the sound is currently playing, stop it then play the new one
	if (g_theAudio->IsPlaying(m_soundPlaybackID))
	{
		g_theAudio->SetSoundPlaybackSpeed(m_soundPlaybackID, 0.f);
	}

	m_soundPlaybackID = g_theAudio->StartSoundAt(soundID, m_position);
}

void Actor::PlayWeaponSound(SoundID soundID)
{
	// if the sound is currently playing, stop it then play the new one
	if (g_theAudio->IsPlaying(m_weaponSoundPlaybackID))
	{
		g_theAudio->StopSound(m_weaponSoundPlaybackID); // todo: 4.21 this cause the sound to decrease lower and lower
	}

	m_weaponSoundPlaybackID = g_theAudio->StartSoundAt(soundID, m_position);
}

void Actor::StopWeaponSound()
{
	if (g_theAudio->IsPlaying(m_weaponSoundPlaybackID))
	{
		g_theAudio->StopSound(m_weaponSoundPlaybackID);
	}
}

void Actor::ConstructProjectileTrajectory(Vec3 start, Actor* actor, float flyingDuration, float flyingSpeed)
{
	if (actor)
	{
		m_lockedTarget = actor;
		m_flyingSpeed = flyingSpeed;

		Vec3 end = m_lockedTarget->m_position + Vec3(0.f, 0.f, m_lockedTarget->m_actorDef->m_eyeHeight);
		m_flyingDuration = flyingDuration;
		Vec3 disp = end - start;
		Vec3 midPt1 = start + disp * 0.5f + Vec3(0.f, 0.f, 0.6f);
		Vec3 midPt2 = start + disp * 0.85f + Vec3(0.f, 0.f, 1.f);

		// do not create new clock when fixing the trajectory
		if (!m_flyingClock)
		{
			m_flyingClock = new Clock();
		}

		// create new trajectory
		if (m_trajectory)
		{
			delete m_trajectory;
		}
		m_trajectory = new CubicBezierCurve3D(start, midPt1, midPt2, end);
	}
}

void Actor::UpdateMissilePositionOnTrajectory()
{
	// first get the fraction of flying time
	float fraction = m_flyingClock->GetTotalSeconds() / m_flyingDuration;

	float division = 1.f / 128.f;
	float tailFraction = fraction - division;
	float pilotFraction = fraction + division;

	fraction = GetClamped(fraction, 0.f, 1.f);

	// reference pilot position
	pilotFraction = GetClamped(fraction, 0.f, 1.f);
	tailFraction = GetClamped(fraction, 0.f, 1.f);

	m_map->SpawnBulletHitOnEnvirnment(m_trajectory->EvaluateAtParametric(tailFraction));

	Vec3 pilotPosition;
	if (pilotFraction >= 1.f)
	{
		pilotPosition = m_trajectory->m_d + Vec3(0.f, 0.f, -1.f);
	}
	else
	{
		pilotPosition = m_trajectory->EvaluateAtParametric(pilotFraction);
	}
	m_position = m_trajectory->EvaluateAtParametric(fraction);

	// todo: adjust the collision cylinder
	// // based on the fraction of the two points, update the homing missile matrix
	// Vec3 Ib;
	// Vec3 Jb;
	// Vec3 Kb;
	// Vec3 Tb = m_position;
	// 
	// Vec3 Tt = pilotPosition;
	// 
	// // world space info
	// Vec3 worldY(0.f, 1.f, 0.f);
	// Vec3 worldZ(0.f, 0.f, 1.f);
	// 
	// Ib = (Tt - m_position).GetNormalized();
	// 
	// // check if the billboard it facing towards the world Z direction
	// if (abs(DotProduct3D(Ib, worldZ) != 1.f))
	// {
	// 	Jb = CrossProduct3D(worldZ, Ib).GetNormalized();
	// 	Kb = CrossProduct3D(Ib, Jb);
	// }
	// else// if the billboard is facing the sky
	// {
	// 	Kb = CrossProduct3D(Ib, worldY);
	// 	Jb = CrossProduct3D(Kb, Ib);
	// }
	// 
	// Mat44 result(Ib, Jb, Kb, m_position);
	// m_orientation = result.GetIBasis3D().GetOrientation();
}
