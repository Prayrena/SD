#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Game/Unit.hpp"
#include "Game/Player.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"

extern App* g_theApp;
extern Game* g_theGame;
extern InputSystem* g_theInput;
extern Window* g_theWindow;

Player::Player()
	:Entity(g_theGame, Vec3(0.f, 0.f, 0.f))
{

}

Player::~Player()
{

}

void Player::Startup()
{
	m_playerCamera.m_position = Vec3(0.f, 0.f, 5.f);
	EulerAngles cameraStartRotation = EulerAngles(90.f, 60.f, 0.f);
	m_playerCamera.m_orientation = cameraStartRotation;

	UpdatePlayerTransformation();

	// m_playerCamera.SetOrthoView(Vec2(-1.f, -1.f), Vec2(1.f, 1.f));
	m_playerCamera.SetPerspectiveView(g_theApp->m_windowAspectRatio, 60.f, 0.1f, 100.f);
	m_playerCamera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
}

void Player::Update()
{
	CalculateTransformAndRoationBasedOnInput();
	UpdatePlayerTransformation();
}

void Player::Render() const
{

}

void Player::CalculateTransformAndRoationBasedOnInput()
{
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
		deltaVelocity += Vec3(0.f, movementSpeed * deltaSeconds, 0.f);
	}
	if (g_theInput->IsKeyDown('D'))
	{
		deltaVelocity += Vec3(0.f, -(movementSpeed * deltaSeconds), 0.f);
	}

	// turn the movement in local space and transform into the world space 
	// Holding WASD (or, if you are cool, ESDF) "drives" (or "strafes") forward, left, backward, or right, respectively,
	// but only in XY (never changing altitude along +/- Z).  
	m_playerCamera.m_position += (GetMovementTransformMatrix().TransformVectorQuantity3D(deltaVelocity));

	// the going from up and down is the changing the coordinates in world space
	if (g_theInput->IsKeyDown('E') || controller.IsButtonDown(XBOX_BUTTON_RSHOULDER))
	{
		m_playerCamera.m_position += Vec3(0.f, 0.f, floatingSpeed * deltaSeconds);
	}
	if (g_theInput->IsKeyDown('Q') || controller.IsButtonDown(XBOX_BUTTON_LSHOULDER))
	{
		m_playerCamera.m_position += Vec3(0.f, 0.f, -(floatingSpeed * deltaSeconds));
	}

	ClampCameraBasedOnHeightAndProjectedScreenCenter();

	// mouse control
	// float deltaYaw = (g_theInput->m_cursorState.m_cusorClientDelta.x) * m_mouseYawSensitiveMultiplier * (-1.f);
	// float deltaPitch = (g_theInput->m_cursorState.m_cusorClientDelta.y) * m_mousePitchSensitiveMultiplier;
	// deltaOrientation += EulerAngles(deltaYaw, deltaPitch, 0.f);
	// 
	// m_orientation += deltaOrientation;

	// clamp the orientation's max pitch and roll
	// m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.f, 45.f);
	// m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);

	// press 'H' to reset the camera position and orientation
	if (g_theInput->WasKeyJustPressed('H') || controller.IsButtonDown(XBOX_BUTTON_START))
	{
		m_position = Vec3(0.f, 0.f, 90.f);
		m_orientation = EulerAngles(0.f, 0.f, 0.f);
	}
}

// restrict camera position according to the map world bounds and minimum height
void Player::ClampCameraBasedOnHeightAndProjectedScreenCenter()
{
	// get projected screen center on map ground plane
	Vec3& cameraPos = m_playerCamera.m_position;
	float mapFloorHeight = g_theGame->m_currentMap->m_mapOrigin.z;

	float cameraHeight = GetClamped(cameraPos.z - mapFloorHeight, m_cameraHeightMin, m_mapBoundsMax.z);
	float cosPitchValue = cosf(ConvertDegreesToRadians(m_playerCamera.m_orientation.m_pitchDegrees));
	float deltaY = cameraHeight * cosPitchValue; // we are facing +Y direction

	// what is unchanged is the vector from the projected pos to camera pos
	Vec3 projectedCenter = Vec3(cameraPos.x, (cameraPos.y + deltaY), g_theGame->m_currentMap->m_mapOrigin.z);
	Vec3 fromProjectedPosToCamera = cameraPos - projectedCenter;
	fromProjectedPosToCamera.z = cameraHeight;

	// clamp the camera projected screen center XY -coordinates to the world bounds
	projectedCenter = GetClamped(projectedCenter, m_mapBoundsMin, m_mapBoundsMax);

	// from the clamped projected center back to get the position of the camera
	Vec3 desiredCameraPos = projectedCenter + fromProjectedPosToCamera;
	// desiredCameraPos.z = GetClamped(desiredCameraPos.z, m_cameraHeightMin, m_mapBoundsMax.z);
	m_playerCamera.m_position = desiredCameraPos;
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

// the logic is the player position follows the camera position, because we are clamping the camera
void Player::UpdatePlayerTransformation()
{
	m_position = m_playerCamera.m_position;
	m_orientation = m_playerCamera.m_orientation;
}
