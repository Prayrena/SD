#include "Game/PlayerTank.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Game/Map.hpp"
#include "Game/Bullet.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern RandomNumberGenerator* g_rng;
extern Game* g_theGame;

// the children's constructor function need to initialize its parent class constructor first
PlayerTank::PlayerTank(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees)
	:Entity(owner, type, faction, startPos, angleDegrees)
{
	m_physicsRadius =  0.3f;
	m_isFixedOnGround = false;
	m_isPushedByEntities = true;
	m_doesPushEntities = true;
	m_isPushedByWalls = true;
	m_isHitByBullets = true;
	m_isProjectile = false;
	m_canSwim = g_gameConfigBlackboard.GetValue("playerCanSwim", false);
	m_entityFaction = FACTION_GOOD;

	m_bodyCurrent_OrientDegrees = angleDegrees;
	m_turretCurrent_OrientDegrees = angleDegrees;

	m_health = PLAYERSHIP_HEALTH;
	m_maxHealth = PLAYERSHIP_HEALTH;
	m_fireRateInterval = PLAYERTANK_COOLDOWN;

	m_speed = g_gameConfigBlackboard.GetValue("playerTankLinearSpeed", 1.f);
	AddVertsForRender();
}

PlayerTank::~PlayerTank()
{
}

void PlayerTank::Update(float deltaSeconds)
{
	if (m_isDead || m_isGarbage)// is player is mark as dead, no need to update
	{
		CheckIfPlayerContinueToPlayAfterDeath();
		return;
	}

	m_fractionOfSpeed = 0.f;
	m_normal = m_iBasis_Body;
	m_timer += deltaSeconds;

	PlayerControl();

	// if there is no input for new turret orientation, the turret rotates with the tank
	m_turretNewGoal = false;
	m_bodyNewGoal = false;
	GetInputOrientationDegreesForBody();
	GetInputOrientationDegreesForTurret();

	AlignTankMovementWithInput( deltaSeconds );
}

void PlayerTank::PlayerControl()
{
	XboxController const& playerController = g_theInput->GetController(0);

	// fire bullet
	if (g_theInput->IsKeyDown(' ') || playerController.IsButtonDown(XboxButtonID::XBOX_BUTTON_A))
	{
		if (m_timer >= m_fireRateInterval)
		{
			Vec2 bulletPos = Vec2(m_physicsRadius * 0.5f, 0.f);
			Vec2 jBasis = m_iBasis_Turret.GetRotated90Degrees();
			Vec2 explosionPos = Vec2(m_physicsRadius * 1.2f, 0.f);
			TransformPosition2D(bulletPos, m_iBasis_Turret, jBasis, m_position);
			TransformPosition2D(explosionPos, m_iBasis_Turret, jBasis, m_position);

			g_theGame->m_currentMap->SpawnNewEntity(ENTITY_TYPE_GOOD_BULLET, bulletPos, m_turretCurrent_OrientDegrees);
			m_timer = 0.f;
			g_theAudio->StartSound(g_theApp->g_soundEffectsID[PLAYER_SHOOT], false, 0.6f, 0.f, 1.f, false);
		}
	}

	// flamethrower
	if (g_theInput->IsKeyDown('G') || playerController.IsButtonDown(XboxButtonID::XBOX_BUTTON_B))
	{
		if ( m_timer >= (m_fireRateInterval * 0.8f) )
		{
			Vec2 jBasis = m_iBasis_Turret.GetRotated90Degrees();
			Vec2 flamePos = Vec2(m_physicsRadius * 1.5f, 0.f);
			TransformPosition2D(flamePos, m_iBasis_Turret, jBasis, m_position);

			float randomSpread = g_rng->RollRandomFloatInRange(-15.f, 15.f);
			g_theGame->m_currentMap->SpawnNewEntity(ENTITY_TYPE_GOOD_FLAME, flamePos, (m_turretCurrent_OrientDegrees + randomSpread) );
			m_timer = 0.f;
			g_theAudio->StartSound(g_theApp->g_soundEffectsID[PLAYER_SHOOT], false, 0.6f, 0.f, 1.f, false);
		}
	}
}

void PlayerTank::CheckIfPlayerContinueToPlayAfterDeath()
{
	if (m_isDead || m_isGarbage)
	{
		XboxController const& playerController = g_theInput->GetController(0);

		// respawn after defeat
		if (g_theInput->WasKeyJustReleased('N') || playerController.WasButtonJustReleased(XboxButtonID::XBOX_BUTTON_START))
		{
			g_theGame->RespawnPlayer();
		}
	}
}

void PlayerTank::GetInputOrientationDegreesForBody()
{
	XboxController const& controller = g_theInput->GetController(0);
	float leftAnalgoMagnitude = controller.GetLeftstick().GetMagnitude();
	float leftAnalogOrientation = controller.GetLeftstick().GetOrientationDegrees();

	if (leftAnalogOrientation < 0.f)
	{
		leftAnalogOrientation += 360.f;
	}

	// changing the bool needs both the magnitude as well as orientation
	if (leftAnalgoMagnitude > 0.1f)
	{
		m_fractionOfSpeed = leftAnalgoMagnitude;
		m_bodyGoal_OrientDegrees = leftAnalogOrientation;
		m_bodyNewGoal = true;
		return;
	}

	if (g_theInput->IsKeyDown('F') && g_theInput->IsKeyDown('E'))
	{
		m_fractionOfSpeed = 1.0f;
		m_bodyGoal_OrientDegrees = 45.f;
		m_bodyNewGoal = true;
		return;
	}

	if (g_theInput->IsKeyDown('E') && g_theInput->IsKeyDown('S'))
	{
		m_fractionOfSpeed = 1.0f;
		m_bodyGoal_OrientDegrees = 135.f;
		m_bodyNewGoal = true;
		return;
	}

	if (g_theInput->IsKeyDown('S') && g_theInput->IsKeyDown('D'))
	{
		m_fractionOfSpeed = 1.0f;
		m_bodyGoal_OrientDegrees = 225.f;
		m_bodyNewGoal = true;
		return;
	}

	if ( g_theInput->IsKeyDown('D') && g_theInput->IsKeyDown('F') )
	{
		m_fractionOfSpeed = 1.0f;
		m_bodyGoal_OrientDegrees = 315.f;
		m_bodyNewGoal = true;
		return;
	}

	// if the player holds opposite direction buttons, the ship will not turn
	if (g_theInput->IsKeyDown('S') && g_theInput->IsKeyDown('F'))
	{
		m_bodyGoal_OrientDegrees = m_bodyCurrent_OrientDegrees;
		return;

	}

	if (g_theInput->IsKeyDown('E') && g_theInput->IsKeyDown('D'))
	{
		m_bodyGoal_OrientDegrees = m_bodyCurrent_OrientDegrees;
		return;

	}

	if (g_theInput->IsKeyDown('D'))
	{
		m_fractionOfSpeed = 1.0f;
		m_bodyGoal_OrientDegrees = 270.f;
		m_bodyNewGoal = true;
		return;
	}


	if (g_theInput->IsKeyDown('F'))
	{
		m_fractionOfSpeed = 1.0f;
		m_bodyGoal_OrientDegrees = 0.f;
		m_bodyNewGoal = true;
		return;
	}

	if (g_theInput->IsKeyDown('E'))
	{
		m_fractionOfSpeed = 1.0f;
		m_bodyGoal_OrientDegrees = 90.f;
		m_bodyNewGoal = true;
		return;
	}

	if (g_theInput->IsKeyDown('S'))
	{
		m_fractionOfSpeed = 1.0f;
		m_bodyGoal_OrientDegrees = 180.f;
		m_bodyNewGoal = true;
		return;
	}
}

void PlayerTank::GetInputOrientationDegreesForTurret()
{
	XboxController const& controller = g_theInput->GetController(0);
	float RightAnalgoMagnitude = controller.GetRightstick().GetMagnitude();
	float RightAnalogOrientation = controller.GetRightstick().GetOrientationDegrees();

	if (RightAnalgoMagnitude < 0.f)
	{
		RightAnalogOrientation += 360.f;
	}

	// changing the bool needs both the magnitude as well as orientation
	if (RightAnalgoMagnitude > 0.1f)
	{
		m_turretGoal_OrientDegrees = RightAnalogOrientation;
		m_turretNewGoal = true;
		return;
	}


	if (g_theInput->IsKeyDown('L') && g_theInput->IsKeyDown('I'))
	{
		m_turretNewGoal = true;
		m_turretGoal_OrientDegrees = 45.f;
		return;
	}

	if (g_theInput->IsKeyDown('I') && g_theInput->IsKeyDown('J'))
	{
		m_turretNewGoal = true;
		m_turretGoal_OrientDegrees = 135.f;
		return;
	}

	if (g_theInput->IsKeyDown('K') && g_theInput->IsKeyDown('L'))
	{
		m_turretNewGoal = true;
		m_turretGoal_OrientDegrees = 315.f;
		return;
	}

	if (g_theInput->IsKeyDown('J') && g_theInput->IsKeyDown('K'))
	{
		m_turretNewGoal = true;
		m_turretGoal_OrientDegrees = 225.f;
		return;
	}
	// if the player holds opposite direction buttons, the turret will not turn
	if (g_theInput->IsKeyDown('L') && g_theInput->IsKeyDown('J'))
	{
		m_turretGoal_OrientDegrees = m_turretCurrent_OrientDegrees;
		return;
	}

	if (g_theInput->IsKeyDown('I') && g_theInput->IsKeyDown('K'))
	{
		m_turretGoal_OrientDegrees = m_turretCurrent_OrientDegrees;
		return;
	}

	if (g_theInput->IsKeyDown('L'))
	{
		m_turretNewGoal = true;
		m_turretGoal_OrientDegrees = 0.f;
		return;
	}


	if (g_theInput->IsKeyDown('I'))
	{
		m_turretNewGoal = true;
		m_turretGoal_OrientDegrees = 90.f;
		return;
	}

	if (g_theInput->IsKeyDown('J'))
	{
		m_turretNewGoal = true;
		m_turretGoal_OrientDegrees = 180.f;
		return;
	}


	if (g_theInput->IsKeyDown('K'))
	{
		m_turretNewGoal = true;
		m_turretGoal_OrientDegrees = 270.f;
		return;
	}

}

void PlayerTank::AlignTankMovementWithInput( float deltaSeconds )
{
	// get new rotated local space i vectors and new orientations
	float body_MaxDeltaDegrees   = PLAYERTANK_BODY_TURNRATE * deltaSeconds;
	float turret_MaxDeltaDegrees = PLAYERTANK_TURRET_TURNRATE * deltaSeconds;
	float body_turnnedDegrees;
	float turret_turnnedDegrees;
	// if there is no one operating the tank body, the turret moves as much as the body
	if (!m_bodyNewGoal)
	{
		body_turnnedDegrees = m_bodyCurrent_OrientDegrees ;
	}
	// if someone is operating the turret, the turret move speed is independent to the tank body
	else
	{
		body_turnnedDegrees = GetTurnedTowardDegrees(m_bodyCurrent_OrientDegrees, m_bodyGoal_OrientDegrees, body_MaxDeltaDegrees);
	}

	// if there is no one operating the turret, the turret moves as much as the body
	if (!m_turretNewGoal)
	{
		turret_turnnedDegrees = m_turretCurrent_OrientDegrees + (body_turnnedDegrees - m_bodyCurrent_OrientDegrees);
	}
	// if someone is operating the turret, the turret move speed is independent to the tank body
	else
	{
		turret_turnnedDegrees = GetTurnedTowardDegrees(m_turretCurrent_OrientDegrees, m_turretGoal_OrientDegrees, turret_MaxDeltaDegrees);
	}

	// copy the turned result value to the current value
	m_bodyCurrent_OrientDegrees = body_turnnedDegrees;
	m_turretCurrent_OrientDegrees = turret_turnnedDegrees;

	// get the new iBasis for body and turrent
	m_iBasis_Body = m_iBasis_Body.MakeFromPolarDegrees(body_turnnedDegrees, 1.f);
	m_iBasis_Turret = m_iBasis_Turret.MakeFromPolarDegrees(turret_turnnedDegrees, 1.f);

	// Driving
	// Get new translation and position
	XboxController const& playerController = g_theInput->GetController(0);
	float leftStickMagnitude = playerController.GetLeftstick().GetMagnitude();
	float speed = 0.f;
	if (leftStickMagnitude > 0.1f)
	{
		m_fractionOfSpeed = leftStickMagnitude;
		speed = m_fractionOfSpeed * m_speed;
		m_velocity = m_velocity.MakeFromPolarDegrees(m_bodyCurrent_OrientDegrees, speed);
		m_position += m_velocity * deltaSeconds;
		return;
	}
	speed = m_fractionOfSpeed * m_speed;
	m_velocity = m_velocity.MakeFromPolarDegrees(m_bodyCurrent_OrientDegrees, speed);
	m_position += m_velocity * deltaSeconds;
 }

void PlayerTank::ResetToRespawn()
{
	m_health = PLAYERSHIP_HEALTH;
	m_isDead = false;
	m_isGarbage = false;
	m_timer = 0.f;
	m_explosionAnimationPlayed = false;
}

/// <Player control>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PlayerTank::Render() const //all called functions must be const
{
	RenderTank();
}

void PlayerTank::RenderTank() const
{
	if (m_isDead || m_isGarbage)
	{
		return;
	}

	std::vector<Vertex_PCU> tankBodyVerts;
	tankBodyVerts.resize(6);
	for (int bodyIndex = 0; bodyIndex < (int)tankBodyVerts.size(); ++bodyIndex)
	{
		tankBodyVerts[bodyIndex] = m_tankBody_LocalVerts[bodyIndex];
	}
	TransformVertexArrayXY3D(tankBodyVerts, m_iBasis_Body, m_position);

	std::vector<Vertex_PCU> tankTurretVerts;
	tankTurretVerts.resize(6);
	for (int turretIndex = 0; turretIndex < (int)tankTurretVerts.size(); ++turretIndex)
	{
		tankTurretVerts[turretIndex] = m_tankTurret_LocalVerts[turretIndex];
	}
	TransformVertexArrayXY3D(tankTurretVerts, m_iBasis_Turret, m_position);

	g_theRenderer->BindTexture(g_theApp->g_textureID[PLAYERTANK_BASE]);
	g_theRenderer->DrawVertexArray((int)tankBodyVerts.size(), tankBodyVerts.data());
	g_theRenderer->BindTexture(g_theApp->g_textureID[PLAYERTANK_TURRET]);
	g_theRenderer->DrawVertexArray((int)tankTurretVerts.size(), tankTurretVerts.data());
	return;
}

void PlayerTank::ReactToBulletHit(Bullet& bullet)
{
	if (isAlive())
	{
		bullet.m_isGarbage = true;
		TakeDamage(1);
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[PLAYER_HIT], false, 1.0f, 0.f, 1.f, false);
	}
}


// draw debug mode
void PlayerTank::DebugRender() const
{
	DrawXYAxis_ForDebug(m_iBasis_Body, m_cosmeticRadius, DEBUGLINE_THICKNESS);
	DrawEntityVelocity_ForDebug(m_velocity, DEBUGLINE_THICKNESS);
	DrawPhysicsCollisionRing_ForDebug(DEBUGRING_THICKNESS);
	DrawCosmeticCollisionRing_ForDebug(DEBUGRING_THICKNESS);

	// body orientation debug line
	Vec2 debugBodyOrientGoal_Start = Vec2(PLAYERTANK_COSMETIC_RADIUS, 0.f);
	Vec2 debugBodyOrientGoal_End = debugBodyOrientGoal_Start + Vec2(0.5f, 0.f);
	debugBodyOrientGoal_Start.SetOrientationDegrees(m_bodyGoal_OrientDegrees);
	debugBodyOrientGoal_End.SetOrientationDegrees(m_bodyGoal_OrientDegrees);
	debugBodyOrientGoal_Start += m_position;
	debugBodyOrientGoal_End += m_position;
	g_theRenderer->BindTexture(nullptr);
	DebugDrawLine(debugBodyOrientGoal_Start, debugBodyOrientGoal_End, DEBUGLINE_THICKNESS * 3.f, Rgba8::RED);

	// turret X axis// barrel direction
	Vec2 debugTurretLineEnd_X = Vec2(PLAYERTANK_BODY_SIZE_Y * 0.5f, 0.f);
	debugTurretLineEnd_X.SetOrientationDegrees(m_turretCurrent_OrientDegrees);
	debugTurretLineEnd_X += m_position;
	g_theRenderer->BindTexture(nullptr);
	DebugDrawLine(m_position, debugTurretLineEnd_X, DEBUGLINE_THICKNESS * 3.f, Rgba8::CYAN);
	// turret orientation debug line
	Vec2 debugTurretOrientGoal_Start = Vec2(PLAYERTANK_COSMETIC_RADIUS, 0.f);
	Vec2 debugTurretOrientGoal_End = debugTurretOrientGoal_Start + Vec2(0.5f, 0.f);
	debugTurretOrientGoal_Start.SetOrientationDegrees(m_turretGoal_OrientDegrees);
	debugTurretOrientGoal_End.SetOrientationDegrees(m_turretGoal_OrientDegrees);
	debugTurretOrientGoal_Start += m_position;
	debugTurretOrientGoal_End += m_position;
	g_theRenderer->BindTexture(nullptr);
	DebugDrawLine(debugTurretOrientGoal_Start, debugTurretOrientGoal_End, DEBUGLINE_THICKNESS * 3.f, Rgba8::CYAN);
}

bool PlayerTank::IsOffScreen() const
{
	return false;
}

// the shape of the ship
// make the localverts related to iBasis
void PlayerTank::AddVertsForRender()
{
	m_tankBody_LocalVerts.reserve(6);
	m_tankTurret_LocalVerts.reserve(6);
	
	AABB2 body = AABB2(-.5f, -.5f, .5f, .5f);
	AddVertsForAABB2D(m_tankBody_LocalVerts, body, Rgba8(255, 255, 255, 255));

	AABB2 turret = AABB2(-.5f, -.5f, .5f, .5f);
	AddVertsForAABB2D(m_tankTurret_LocalVerts, turret, Rgba8(255, 255, 255, 255));
}
