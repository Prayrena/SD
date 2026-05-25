#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Game/Player.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/World.hpp"
#include "Game/Block.hpp"

extern App* g_theApp;
extern Game* g_theGame;
extern InputSystem* g_theInput;
extern Window* g_theWindow;
extern World* g_theWorld;

Player::Player()
	:Entity(g_theGame, Vec3(0.f, 0.f, 0.f))
{

}

Player::~Player()
{

}

void Player::Startup()
{
	m_position = Vec3(-16.f, -16.f, 96.f);
	m_orientation = EulerAngles(45.f, 45.f, 0.f);

	m_playerCamera.SetTransform(m_position, m_orientation);
	// m_playerCamera.SetOrthoView(Vec2(-1.f, -1.f), Vec2(1.f, 1.f));
	m_playerCamera.SetPerspectiveView(g_theApp->m_windowAspectRatio, 60.f, 0.1f, 1000.f);
	m_playerCamera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
}

void Player::Update()
{
	PlayerInputControls();
	UpdateCameraTransformation();
	// SpawnDebugRenderGeometry();

	GenerateWorldAxesInfrontOfCamera();
}

void Player::Render() const
{

}

Mat44 Player::GetMovementTransformMatrix()
{
	// transform coordinate from local to world matrix
	// [translate][rotate]
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);

	EulerAngles movingAngle = m_orientation;
	movingAngle.m_pitchDegrees = 0.f;
	movingAngle.m_rollDegrees = 0.f;
	Mat44 orientationMat = movingAngle.GetAsMatrix_XFwd_YLeft_ZUp();
	// orientationMat.Append(transformMat);
	// return orientationMat;
	transformMat.Append(orientationMat);
	return transformMat;
}

void Player::PlayerInputControls()
{
	if (g_theInput->WasKeyJustPressed('1'))
	{
		g_theGame->m_player->m_buildingBlockDef = BlockDef::GetBlockDefByName("cobbleStone");
	}

	if (g_theInput->WasKeyJustPressed('2'))
	{
		g_theGame->m_player->m_buildingBlockDef = BlockDef::GetBlockDefByName("glowStone");
	}

	// player could fly even when the game is paused
	float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();
	float movementSpeed = m_moveSpeed;
	float floatingSpeed = m_floatingSpeed;
	float controllerMovementMultiplier = m_controllerMovementMultiplier;

	// we use delta velocity and delta orientation to change m_position and m_orientation
	Vec3 deltaVelocity = Vec3();
	EulerAngles deltaOrientation;

	XboxController const& controller = g_theInput->GetController(0);

	// press shift or A button to increase speed by a factor of 10 while held
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT) || controller.IsButtonDown(XBOX_BUTTON_A))
	{
		movementSpeed *= 20.f;
		floatingSpeed *= 5.f;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_SHIFT) || controller.WasButtonJustPressed(XBOX_BUTTON_A))
	{
		movementSpeed *= 0.1f;
		floatingSpeed *= 0.2f;
	}

	// controller control
	Vec2 leftAnalogPos = controller.GetLeftstick().m_correctedPosition;
	Vec2 rightAnalogPos = controller.GetRightstick().m_correctedPosition;
	float LeftTrigger = controller.GetLeftTrigger();
	float rightTrigger = controller.GetRightTrigger();

	// orientation
	float constollerDeltaPitch = rightAnalogPos.y * m_controllerPitchSensitiveMultiplier * (-1.f);
	float constollerDeltaYaw = rightAnalogPos.x * m_controllerYawSensitiveMultiplier * (-1.f);
	deltaOrientation += EulerAngles(constollerDeltaYaw, constollerDeltaPitch, 0.f);
	deltaOrientation += EulerAngles(0.f, 0.f, (rightTrigger - LeftTrigger) * m_controllerRollMultiplier * deltaSeconds);

	// movement
	deltaVelocity += movementSpeed * Vec3(leftAnalogPos.y, leftAnalogPos.x * (-1.f), 0.f) * controllerMovementMultiplier;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// KeyBoard control

	// the movement is based on player orientation, in local space, not in world space
	if (g_theInput->IsKeyDown('W'))
	{
		deltaVelocity += Vec3(movementSpeed * deltaSeconds, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown('S'))
	{
		deltaVelocity += Vec3(-(movementSpeed * deltaSeconds), 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown('A'))
	{
		deltaVelocity += Vec3( 0.f, movementSpeed * deltaSeconds, 0.f);
	}
	if (g_theInput->IsKeyDown('D'))
	{
		deltaVelocity += Vec3(0.f, -(movementSpeed * deltaSeconds), 0.f);
	}
	// turn the movement in local space and transform into the world space 
	// Holding WASD (or, if you are cool, ESDF) “drives?(or “strafes? forward, left, backward, or right, respectively, 
	// but only in XY (never changing altitude along +/- Z).  
	m_position += (GetMovementTransformMatrix().TransformVectorQuantity3D(deltaVelocity));
	
	// the going from up and down is the changing the coordinates in world space
	if (g_theInput->IsKeyDown('E') || controller.IsButtonDown(XBOX_BUTTON_RSHOULDER))
	{
		m_position += Vec3(0.f, 0.f, floatingSpeed * deltaSeconds);
	}
	if (g_theInput->IsKeyDown('Q') || controller.IsButtonDown(XBOX_BUTTON_LSHOULDER))
	{
		m_position += Vec3(0.f, 0.f, -(floatingSpeed * deltaSeconds));
	}

	// the arrow key controls the rotation in local space
	if (g_theInput->IsKeyDown(KEYCODE_UPARROW))
	{
		deltaOrientation += EulerAngles(0.f, -(m_turnRate * deltaSeconds), 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW))
	{
		deltaOrientation += EulerAngles( 0.f, (m_turnRate * deltaSeconds), 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW))
	{
		deltaOrientation += EulerAngles((m_turnRate * deltaSeconds), 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW))
	{
		deltaOrientation += EulerAngles(-(m_turnRate * deltaSeconds), 0.f, 0.f);
	}
	//if (g_theInput->IsKeyDown('Q'))
	//{
	//	deltaOrientation += EulerAngles(0.f, 0.f, -(m_turnRate * deltaSeconds));
	//}
	//if (g_theInput->IsKeyDown('E'))
	//{
	//	deltaOrientation += EulerAngles(0.f, 0.f, (m_turnRate * deltaSeconds));
	//}

	// mouse control
	float deltaYaw = (g_theInput->m_cursorState.m_cusorClientDelta.x) * m_mouseYawSensitiveMultiplier * (-1.f);
	float deltaPitch = (g_theInput->m_cursorState.m_cusorClientDelta.y) * m_mousePitchSensitiveMultiplier;
	deltaOrientation += EulerAngles(deltaYaw, deltaPitch, 0.f);

	m_orientation += deltaOrientation;

	// clamp the orientation's max pitch and roll
	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.f, 45.f);
	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);

	// press 'H' to reset the camera position and orientation
	if (g_theInput->WasKeyJustPressed('H') || controller.IsButtonDown(XBOX_BUTTON_START))
	{
		m_position = Vec3(0.f, 0.f, 90.f);
		m_orientation = EulerAngles(0.f, 0.f, 0.f);
	}

	// dig and placing blocks
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		g_theWorld->DigBlock();
	}	
	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		g_theWorld->BuildPlayerBlock();
	}
}

void Player::SpawnDebugRenderGeometry()
{
	// press 1
	// Spawn a line from the player along their forward direction 20 units in length
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM1))
	{
		// spawned geometry is spawned in a distance in front of the player in 5.f
		Vec3 spawnDist = Vec3(1.f, 0.f, 0.f);
		Mat44 id;
		Vec3 spawnPos = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnDist);

		Vec3 dist = Vec3(21.f, 0.f, 0.f);
		Vec3 end = spawnPos + GetModelMatrix().TransformVectorQuantity3D(dist);
		DebugAddWorldLine(spawnPos, end, 0.2f, 10.f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::X_RAY);
		// g_debugRenderGeometries.back()->m_modelMatrix = GetModelMatrix();
	}

	// press 2
	// Spawn a sphere directly below the player position on the XY-plane
	if (g_theInput->IsKeyDown(KEYCODE_NUM2))
	{
		Vec3 projectionOnXY = Vec3(m_position.x, m_position.y, 0.f);
		Rgba8 sphereColor = Rgba8(150, 75, 0, 255);
		DebugAddWorldPoint(projectionOnXY, .25f, 60.f, sphereColor, sphereColor);
	}

	// press 3
	// Spawn a wire frame sphere 2 units in front of player camera
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUM3))
	{
		// 2 units ahead
		Vec3 spawnDist = Vec3(2.f, 0.f, 0.f);
		Vec3 spawnPos = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnDist);

		DebugAddWorldWireSphere(spawnPos, 1.f, 5.f, Rgba8::GREEN, Rgba8::RED);
	}

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

		// DebugAddMessage(orientation, 5.f, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}
}

void Player::GenerateWorldAxesInfrontOfCamera()
{
	Vec3 rayStart = m_position;
	Vec3 rayDisp = Vec3(0.2f, 0.f, 0.f);
	Vec3 rayFwdNormal = GetModelMatrix().TransformVectorQuantity3D(rayDisp).GetNormalized();
	Vec3 rayEnd = rayStart + rayFwdNormal * m_rayDist;

	Mat44 identity;
	Mat44 worldBasis = Mat44::CreateTranslation3D(rayEnd);

	// generate the reference at the pos of the ray start
	DebugAddWorldBasis(worldBasis, 0.f, 0.09f, DebugRenderMode::X_RAY, 1.f);
}

void Player::UpdateCameraTransformation()
{
	m_playerCamera.SetTransform(m_position, m_orientation);
}
