#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Game/Player.hpp"
#include "Game/App.hpp"
#include "Game/QuaternionMode.hpp"

extern App* g_theApp;
extern InputSystem* g_theInput;
extern Window* g_theWindow;

Player::Player(Vec3 position /*= Vec3()*/)
	: m_position(position)
{
	m_onePerSprintModifier = 1.f / m_sprintModifier;
}

Player::~Player()
{

}

void Player::Startup()
{
	// m_position = Vec3(3.f, 3.f, 10.f);
	m_orientation = EulerAngles(45.f, 30.f, 0.f);
	// m_quatOrientation = ConvertEulerAnglesToQuat(m_orientation);

	m_playerCamera.SetTransform(m_position, m_orientation);
	// m_playerCamera.SetOrthoView(Vec2(-1.f, -1.f), Vec2(1.f, 1.f));
	m_playerCamera.SetPerspectiveView(g_theApp->m_windowAspectRatio, 60.f, 0.1f, 100.f);
	m_playerCamera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
}

void Player::Update()
{
	CalculateTransformAndRoationBasedOnInput();
	UpdateCameraTransformation();

	SpawnDebugRenderGeometry();
}

void Player::Render() const
{

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

void Player::CalculateTransformAndRoationBasedOnInput()
{
	QuaternionMode* mode = dynamic_cast<QuaternionMode*>(g_theApp->m_currentGameMode);

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		if (m_controllingCamera)
		{
			m_controllingCamera = false;
		}
		else
		{
			m_controllingCamera = true;
		}
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
		movementSpeed *= m_sprintModifier;
		floatingSpeed *= m_sprintModifier;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_SHIFT) || controller.WasButtonJustPressed(XBOX_BUTTON_A))
	{
		movementSpeed *= m_onePerSprintModifier;
		floatingSpeed *= m_onePerSprintModifier;
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

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// mouse control of orientation

	// press 'H' to reset the camera position and orientation
	// if (g_theInput->WasKeyJustPressed('H') || controller.IsButtonDown(XBOX_BUTTON_START))
	// {
	// 	m_position = Vec3(0.f, 0.f, 0.f);
	// 	m_orientation = EulerAngles(0.f, 0.f, 0.f);
	// }

	if (mode)
	{
		// first set quat orientation and position of the actor
		// Quat(currentFrame) = Quat(last) * Quat(additional)
		// float yawRadians = (float)(g_theInput->m_cursorState.m_cusorClientDelta.x) * m_mouseYawSensitiveMultiplier * (-1.f);
		// float pitchRadians = (float)(g_theInput->m_cursorState.m_cusorClientDelta.y) * m_mousePitchSensitiveMultiplier;
		// 
		// // construct the quat based on the rotation axis and rotation radian
		// Quat yawQuat(Vec3(0.f, 0.f, 1.f), yawRadians);
		// Quat pitchQuat(Vec3(0.f, 1.f, 0.f), pitchRadians);
		// m_quatOrientation *= yawQuat;		// this is doing relative rotation
		// m_quatOrientation *= pitchQuat;

		// Mat44 rotationMat(m_quatOrientation);

		float deltaYaw = (g_theInput->m_cursorState.m_cusorClientDelta.x) * m_mouseYawSensitiveMultiplier * 2.f * (-1.f);
		float deltaPitch = (g_theInput->m_cursorState.m_cusorClientDelta.y) * m_mousePitchSensitiveMultiplier * 2.f;
		deltaOrientation += EulerAngles(deltaYaw, deltaPitch, 0.f);

		m_orientation += deltaOrientation;

		// clamp the orientation's max pitch and roll
		// m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.f, 45.f);
		m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);

		
		// then get the position of the actor
		// Vec3 localForwadNormal = Vec3(1.f, 0.f, 0.f);
		Mat44 rotationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
		Vec3 cameraForward = rotationMat.GetIBasis3D();
		m_position = mode->m_sphereCenter + cameraForward * mode->m_cameraDist * -1.f;

		// m_playerCamera.m_rotationMat = Mat44(m_position).MatMultiply(rotationMat);
		m_playerCamera.m_rotationMat = rotationMat;
	}
	else
	{
		float deltaYaw = (g_theInput->m_cursorState.m_cusorClientDelta.x) * m_mouseYawSensitiveMultiplier * (-1.f);
		float deltaPitch = (g_theInput->m_cursorState.m_cusorClientDelta.y) * m_mousePitchSensitiveMultiplier;
		deltaOrientation += EulerAngles(deltaYaw, deltaPitch, 0.f);

		m_orientation += deltaOrientation;

		// clamp the orientation's max pitch and roll
		// m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.f, 45.f);
		m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);

	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// KeyBoard control
	// movement
	deltaVelocity += movementSpeed * Vec3(leftAnalogPos.y, leftAnalogPos.x * (-1.f), 0.f) * controllerMovementMultiplier;

	Mat44 rotationMat = Mat44();
	if (mode)
	{
		rotationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	}

	if (g_theInput->IsKeyDown('W'))
	{
		deltaVelocity += rotationMat.TransformVectorQuantity3D(Vec3(movementSpeed * deltaSeconds, 0.f, 0.f));
	}
	if (g_theInput->IsKeyDown('S'))
	{
		deltaVelocity += rotationMat.TransformVectorQuantity3D(Vec3(-(movementSpeed * deltaSeconds), 0.f, 0.f));
	}
	if (g_theInput->IsKeyDown('A'))
	{
		deltaVelocity += rotationMat.TransformVectorQuantity3D(Vec3(0.f, movementSpeed * deltaSeconds, 0.f));
	}
	if (g_theInput->IsKeyDown('D'))
	{
		deltaVelocity += rotationMat.TransformVectorQuantity3D(Vec3(0.f, -(movementSpeed * deltaSeconds), 0.f));
	}

	// the going from up and down is the changing the coordinates in world space
	if (g_theInput->IsKeyDown('E') || controller.IsButtonDown(XBOX_BUTTON_RSHOULDER))
	{
		deltaVelocity += Vec3(0.f, 0.f, floatingSpeed * deltaSeconds);
	}
	if (g_theInput->IsKeyDown('Q') || controller.IsButtonDown(XBOX_BUTTON_LSHOULDER))
	{
		deltaVelocity += Vec3(0.f, 0.f, -(floatingSpeed * deltaSeconds));
	}

	// the movement is based on player orientation, in local space, not in world space
	if (mode)
	{
		// in quaternion mode, we are controlling the movement of the sphere
		m_deltaMovement = Mat44(m_quatOrientation).TransformVectorQuantity3D(deltaVelocity);
		mode->m_sphereCenter += m_deltaMovement;
	}
	else
	{
		m_deltaMovement = (GetModelMatrix().TransformVectorQuantity3D(deltaVelocity));
		m_position += m_deltaMovement;
	}


	// the arrow key controls the player rotation in local space
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
	// if (g_theInput->IsKeyDown('Q'))
	// {
	// 	deltaOrientation += EulerAngles(0.f, 0.f, -(m_turnRate * deltaSeconds));
	// }
	// if (g_theInput->IsKeyDown('E'))
	// {
	// 	deltaOrientation += EulerAngles(0.f, 0.f, (m_turnRate * deltaSeconds));
	// }
}

void Player::SpawnDebugRenderGeometry()
{
	//// press 1
	//// Spawn a line from the player along their forward direction 20 units in length
	//if (g_theInput->WasKeyJustPressed(KEYCODE_NUM1))
	//{
	//	// spawned geometry is spawned in a distance in front of the player in 5.f
	//	Vec3 spawnDist = Vec3(1.f, 0.f, 0.f);
	//	Mat44 id;
	//	Vec3 spawnPos = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnDist);

	//	Vec3 dist = Vec3(21.f, 0.f, 0.f);
	//	Vec3 end = spawnPos + GetModelMatrix().TransformVectorQuantity3D(dist);
	//	DebugAddWorldLine(spawnPos, end, 0.2f, 10.f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::X_RAY);
	//	// g_debugRenderGeometries.back()->m_modelMatrix = GetModelMatrix();
	//}

	//// press 2
	//// Spawn a sphere directly below the player position on the XY-plane
	//if (g_theInput->IsKeyDown(KEYCODE_NUM2))
	//{
	//	Vec3 projectionOnXY = Vec3(m_position.x, m_position.y, 0.f);
	//	Rgba8 sphereColor = Rgba8(150, 75, 0, 255);
	//	DebugAddWorldPoint(projectionOnXY, .25f, 60.f, sphereColor, sphereColor);
	//}

	//// press 3
	//// Spawn a wire frame sphere 2 units in front of player camera
	//if (g_theInput->WasKeyJustPressed(KEYCODE_NUM3))
	//{
	//	// 2 units ahead
	//	Vec3 spawnDist = Vec3(2.f, 0.f, 0.f);
	//	Vec3 spawnPos = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnDist);

	//	DebugAddWorldWireSphere(spawnPos, 1.f, 5.f, Rgba8::GREEN, Rgba8::RED);
	//}

	//// press 4
	//// Spawn a basis using the player current model matrix
	//if (g_theInput->WasKeyJustPressed(KEYCODE_NUM4))
	//{
	//	Mat44 modelmatrix = GetModelMatrix();
	//	DebugAddWorldBasis(modelmatrix, 20.f);
	//}

	//// press 5
	//// Spawn billboarded text showing the player position and orientation
	//if (g_theInput->WasKeyJustPressed(KEYCODE_NUM5))
	//{
	//	// 0.5 units ahead
	//	// otherwise player has to move back to see it and when player slide left or right, the billboard immediately turn sideway - which make sense
	//	Vec3 spawnDist = Vec3(0.5f, 0.f, 0.f);
	//	Vec3 spawnPos = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnDist);

	//	// keep only one decimal for the position and orientation
	//	std::string line_position_orientation = Stringf("Position = %.1f, %.1f, %.1f   Orientation = %.1f, %.1f, %.1f", m_position.x, m_position.y, m_position.z, 
	//		m_orientation.m_yawDegrees, m_orientation.m_pitchDegrees, m_orientation.m_rollDegrees);

	//	DebugAddWorldBillboardText(line_position_orientation, spawnPos, 0.3f, Vec2(0.5f, 0.5f), 10.f, BillboardType::FULL_CAMERA_FACING, Rgba8::WHITE, Rgba8::RED);
	//	// DebugAddWorldText(line_position_orientation, GetModelMatrix(), 0.5f, 10.f, Vec2(0.5f, 0.5f), Rgba8::WHITE, Rgba8::RED);
	//}

	//// press 6
	//// Spawn a wire frame cylinder at player position and orientation
	//if (g_theInput->WasKeyJustPressed(KEYCODE_NUM6))
	//{
	//	// 1.6 units below(1.6m tall
	//	Vec3 spawnBase = Vec3(0.f, 0.f, -1.6f);
	//	Vec3 cylinderBottom = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnBase);
	//	
	//	// overall is 1.8 tall
	//	Vec3 spawnTop = Vec3(0.f, 0.f, .2f);
	//	Vec3 cylinderTop = m_position + GetModelMatrix().TransformVectorQuantity3D(spawnTop);

	//	DebugAddWorldWireCylinder(cylinderBottom, cylinderTop, 0.9f, 10.f, Rgba8::WHITE, Rgba8::RED);
	//}

	//// press 7
	//// Add a screen message with the current camera orientation
	//if (g_theInput->WasKeyJustPressed(KEYCODE_NUM7))
	//{
	//	// keep only one decimal for the position and orientation
	//	std::string orientation = Stringf("Camera Orientation = %.2f, %.2f, %.2f", m_orientation.m_yawDegrees, m_orientation.m_pitchDegrees, m_orientation.m_rollDegrees);

	//	DebugAddMessage(orientation, 5.f, 10.f, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	//}
}

void Player::UpdateCameraTransformation()
{
	QuaternionMode* mode = dynamic_cast<QuaternionMode*>(g_theApp->m_currentGameMode);
	if (mode)
	{
		m_playerCamera.m_useRotationMatrix = true;
		m_playerCamera.m_position = m_position;
	}
	else
	{
		m_playerCamera.SetTransform(m_position, m_orientation);
		m_playerCamera.m_useRotationMatrix = false;
	}
}
