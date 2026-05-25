#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Engine/Math/OpenXRMathUtils.hpp"
#include "Engine/Physics/ThePhysX.hpp"
#include "Game/WinPlayer.hpp"
#include "Game/Game.hpp"
#include "Game/openxr_program.h"
#include "Game/VRPlayer.hpp"
#include "Game/Crawler.hpp"
#include <vector>

extern OpenXrProgram* g_theApp;
extern Game* g_theGame;
extern InputSystem* g_theInput;
extern Window* g_theWindow;
extern ThePhysX* g_thePhysX;
extern VRPlayer* g_theVRPlayer;

using namespace std;

WinPlayer::WinPlayer(Vec3 const& pos /*= Vec3()*/)
		: Actor(pos)
{

}

WinPlayer::~WinPlayer()
{
	if (m_movingTargetVertexBuffer)
	{
		delete m_movingTargetVertexBuffer;
		m_movingTargetVertexBuffer = nullptr;
	}	
	
	if (m_movingTarget2VertexBuffer)
	{
		delete m_movingTarget2VertexBuffer;
		m_movingTarget2VertexBuffer = nullptr;
	}
}

void WinPlayer::Startup()
{
	// set window camera
	Vec3 cameraPos = Vec3(-3.f, 0.f, 6.f);
	EulerAngles playerStartRotation = EulerAngles(0.f, 50.f, 0.f);

	m_position = cameraPos;
	m_orientation = playerStartRotation;

	m_windowsCamera = new Camera();
	m_screenCamera = new Camera();

	m_windowsCamera->SetTransform(cameraPos, playerStartRotation);
	m_windowsCamera->SetPerspectiveView(g_theApp->m_windowAspectRatio, 60.f, 0.1f, 100.f);

	m_windowsCamera->SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f)); // game to directX

	IntVec2 appResolution = g_theWindow->GetWindowDimensions();
	m_screenCamera->SetOrthoView(Vec2(0.f, 0.f), Vec2((float)appResolution.x, (float)appResolution.y));

	m_FPSTimer = new Timer(0.15f);
	m_FPSTimer->Start();

	CreateBufferForDebugTarget();
}

void WinPlayer::CreateBufferForDebugTarget()
{
	AddVertsForSphere3D(m_debugVerts_endEffectorTarget, Vec3(), m_debugRadius, m_movingTargetColor);

	m_movingTargetVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_debugVerts_endEffectorTarget.size()), sizeof(Vertex_PCU));

	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArrayDataSize = (m_debugVerts_endEffectorTarget.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_debugVerts_endEffectorTarget.data(), vertexArrayDataSize, m_movingTargetVertexBuffer);	
	
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	AddVertsForSphere3D(m_debugVerts_PoleVector, Vec3(), m_debugRadius, m_movingPolveVectorColor);

	m_movingTarget2VertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_debugVerts_PoleVector.size()), sizeof(Vertex_PCU));

	vertexSize = sizeof(Vertex_PCU);
	vertexArrayDataSize = (m_debugVerts_PoleVector.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_debugVerts_PoleVector.data(), vertexArrayDataSize, m_movingTarget2VertexBuffer);
}

void WinPlayer::RenderDebugTargets() const
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetModelConstants(GetModelMatrix_EndEffectorTarget());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexBuffer(m_movingTargetVertexBuffer, (int)m_debugVerts_endEffectorTarget.size());

	g_theRenderer->SetModelConstants(GetModelMatrix_PoleVector());
	g_theRenderer->DrawVertexBuffer(m_movingTarget2VertexBuffer, (int)m_debugVerts_endEffectorTarget.size());
}

Mat44 WinPlayer::GetModelMatrix_EndEffectorTarget() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_movingTarget_endEffector);
	return transformMat;
}

Mat44 WinPlayer::GetModelMatrix_PoleVector() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_movingTarget_poleVector);
	return transformMat;
}

void WinPlayer::Update()
{
#ifndef SHIPPING
	UpdateInput();
	UpdateDebugRenderMessages();

	// update the camera by the actor position and orientation
	if (m_WinCameraIsInControl)
	{
		m_windowsCamera->SetTransform(m_position, m_orientation);
		// RaycastFromCameraToMouseToMap();
	}
#endif // SHIPPING
}

void WinPlayer::UpdateInput()
{
	if (m_WinCameraIsInControl)
	{
		ControlActorByInput();
		g_theInput->SetCursorMode(true, true);
	}
	else
	{
		g_theInput->SetCursorMode(false, false);
	}

	// control synchronize camera
	if (g_theInput->WasKeyJustPressed('F') && g_theApp->m_openXRAvaible)
	{
		if (!m_WinCameraIsInControl)
		{
			m_WinCameraIsInControl = true;
		}
		else
		{
			m_WinCameraIsInControl = false;

			// reset the actor to view
			m_position = Vec3(g_theRenderer->m_views[0].pose.position) + g_theVRPlayer->m_position + Vec3(0.f, 0.f, 1.8f);
			m_orientation = ConvertQuatToEulerAngles(Quat(g_theRenderer->m_views[0].pose.orientation));
			m_orientation.m_rollDegrees = 0.f;
		}
	}

	// turn on and off the debug interface
	if (g_theInput->WasKeyJustPressed('V'))
	{
		m_showDebugInterface = !m_showDebugInterface;

		SetDebugRenderVisibility(m_showDebugInterface);
	}

	// control lock mouse control target
	if (g_theInput->WasKeyJustPressed('G'))
	{
		if (m_target1_inControl)
		{
			m_target1_inControl = false;
		}
		else
		{
			m_target1_inControl = true;
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	Actor* sun = g_theGame->m_sun;
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
		if (g_theGame->m_phongLighinting->SunIntensity > 0.f)
		{
			g_theGame->m_phongLighinting->SunIntensity -= 0.1f;
		}
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_GREATERTHAN))
	{
		if (g_theGame->m_phongLighinting->SunIntensity < 1.f)
		{
			g_theGame->m_phongLighinting->SunIntensity += 0.1f;
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	g_theGame->m_phongLighinting->SunDirection = sun->m_orientation.GetForwardIBasis().GetNormalized();
}

void WinPlayer::Render() const
{
	// g_theRenderer->ClearScreen(Rgba8::GRAY_Dark);//the background color setting of the window

	if (g_theGame->m_currentState == GameState::ATTRACT)
	{
		g_theRenderer->ClearScreen(Rgba8::BLACK);
	}
	else if (g_theGame->m_currentState == GameState::LOBBY)
	{
		g_theRenderer->ClearScreen(Rgba8::BLUE_LIGHT);
	}

	// synchronize the windows camera to VR camera left eye
	if (!m_WinCameraIsInControl && g_theApp->m_openXRAvaible)
	{
		// translation of the windows camera - the is correct after testing
		Mat44 OpenXRToGameMat = GetOpenXRToGameMat(); // openXR to game
		Mat44 translationMat(g_theRenderer->m_views[0].pose.position);
		// translationMat = translationMat.MatMultiply(OpenXRToGameMat);
		translationMat = OpenXRToGameMat.MatMultiply(translationMat);
		m_windowsCamera->m_position = translationMat.GetTranslation3D() + g_theVRPlayer->m_position;

		// rotation of the windows camera
		Quat quat(g_theRenderer->m_views[0].pose.orientation);
		Mat44 transformMat = GetOpenXRToGameMat();

		quat = (transformMat.TransformQuaternion(quat) * ConvertEulerAnglesToQuat(g_theVRPlayer->m_orientation)).GetNormalized();
		Mat44 rotationMat = Mat44(quat);

		m_windowsCamera->m_useRotationMatrix = true;
		m_windowsCamera->m_rotationMat = rotationMat;
	}
	else
	{
		m_windowsCamera->m_useRotationMatrix = false;
		// m_windowsCamera->m_rotationMat = Mat44(Quat(g_theRenderer->m_views[0].pose.orientation));
	}

	g_theRenderer->BeginCamera(*m_windowsCamera);
	g_theGame->m_phongLighinting->WorldEyePosition = m_windowsCamera->m_position;
	// g_theGame->m_phongLighinting->WorldEyePosition = GetOpenXRToGameMat().TransformPosition3D(Vec3(g_theRenderer->m_views[0].pose.position));
	g_theGame->UpdateFogShaderDataWithNewCameraPos(m_windowsCamera->m_position);
	g_theGame->Render();
	RenderDebugTargets();
	DebugRenderWorld(*m_windowsCamera);
	g_thePhysX->Render();
	g_theRenderer->EndCamera(*m_windowsCamera);

	// debug messages on screen
	g_theRenderer->BeginCamera(*m_screenCamera);
	if (GetDebugRenderVisibility())
	{
		// render the messages on the screen
		DebugRenderScreen(*m_screenCamera);
	}
	g_theRenderer->EndCamera(*m_screenCamera);
}


Mat44 WinPlayer::GetMovementTransformMatrix()
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

void WinPlayer::ControlActorByInput()
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
	// Holding WASD (or, if you are cool, ESDF) “drives” (or “strafes”) forward, left, backward, or right, respectively, 
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

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// use arrow keys to control the movement of the moving target
	if (m_target1_inControl)
	{
		if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW))
		{
			m_movingTarget_endEffector.y += movementSpeed * deltaSeconds * 0.5f;
		}
		if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW))
		{
			m_movingTarget_endEffector.y -= movementSpeed * deltaSeconds * 0.5f;
		}
		if (g_theInput->IsKeyDown(KEYCODE_UPARROW))
		{
			m_movingTarget_endEffector.x += movementSpeed * deltaSeconds * 0.5f;
		}
		if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW))
		{
			m_movingTarget_endEffector.x -= movementSpeed * deltaSeconds * 0.5f;
		}

		if (g_theInput->IsKeyDown('Z'))
		{
			m_movingTarget_endEffector.z -= floatingSpeed * deltaSeconds * 0.5f;
		}
		if (g_theInput->IsKeyDown('X'))
		{
			m_movingTarget_endEffector.z += floatingSpeed * deltaSeconds * 0.5f;
		}
	}
	else
	{
		if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW))
		{
			m_movingTarget_poleVector.y += movementSpeed * deltaSeconds * 0.5f;
		}
		if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW))
		{
			m_movingTarget_poleVector.y -= movementSpeed * deltaSeconds * 0.5f;
		}
		if (g_theInput->IsKeyDown(KEYCODE_UPARROW))
		{
			m_movingTarget_poleVector.x += movementSpeed * deltaSeconds * 0.5f;
		}
		if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW))
		{
			m_movingTarget_poleVector.x -= movementSpeed * deltaSeconds * 0.5f;
		}

		if (g_theInput->IsKeyDown('Z'))
		{
			m_movingTarget_poleVector.z -= floatingSpeed * deltaSeconds * 0.5f;
		}
		if (g_theInput->IsKeyDown('X'))
		{
			m_movingTarget_poleVector.z += floatingSpeed * deltaSeconds * 0.5f;
		}
	}

	if (g_theInput->WasKeyJustPressed('C'))
	{
		m_solveTarget = !m_solveTarget;
	}	
	
	if (g_theInput->WasKeyJustPressed('I'))
	{
		if (m_debugChain)
		{
			m_debugChain->m_solver->m_debugMode = !m_debugChain->m_solver->m_debugMode;
		}
	}	
	
	if (g_theInput->WasKeyJustPressed('K'))
	{
		if (m_debugChain->m_solver->m_debugMode)
		{
			m_debugChain->m_solver->m_debugCounter += m_addingCounter;
		}
	}
}

void WinPlayer::UpdateDebugRenderMessages()
{
	if (!m_showDebugInterface)
	{
		return;
	}

	Vec2  textPos = Vec2(1.f, 0.f);
	Vec2  spacing = Vec2(0.f, 0.04f);

	Vec2  fpsAlignment = Vec2(1.f, 0.995f);
	Vec2  slot2 = fpsAlignment - spacing;
	Vec2  slot3 = slot2 - spacing;
	Vec2  slot4 = slot3 - spacing;
	Vec2  slot5 = slot4 - spacing;
	Vec2  slot6 = slot5 - spacing;

	Vec2  slot7 = slot6 - spacing;
	Vec2  slot8 = slot7 - spacing;
	Vec2  slot9 = slot8 - spacing;

	Vec2  keyPressAlignment_1 = Vec2(0.f, 0.995f);
	Vec2  keyPressAlignment_2 = keyPressAlignment_1 - spacing;
	Vec2  keyPressAlignment_3 = keyPressAlignment_2 - spacing;

	float fontSize = 24.f;

	string keyPressStr = "Press R to toggle PhysX debug, L to toggle skeleton debug, V to toggle this interface";
	DebugAddScreenText(keyPressStr, Vec2(), fontSize, keyPressAlignment_1, -1.f, Rgba8::BLACK, Rgba8::BLACK);	
	
	keyPressStr = "Press K to kill current crawlers";
	DebugAddScreenText(keyPressStr, Vec2(), fontSize, keyPressAlignment_2, -1.f, Rgba8::BLACK, Rgba8::BLACK);	
	
	keyPressStr = "Press N/M to decrease/increase the number of crawlers";
	DebugAddScreenText(keyPressStr, Vec2(), fontSize, keyPressAlignment_3, -1.f, Rgba8::BLACK, Rgba8::BLACK);

	if (m_FPSTimer->DecrementPeroidIfElapsed())
	{
		m_FPSString = Stringf("FPS: %s", std::to_string(Clock::GetSystemClock().GetFrameRatePerSecond()).c_str());
	}
	DebugAddScreenText(m_FPSString, Vec2(), fontSize, fpsAlignment, -1.f, Rgba8::BLACK, Rgba8::BLACK);

	std::string takeCameraControl;
	if (m_WinCameraIsInControl)
	{
		takeCameraControl = "Synchronize Camera(F): false";
	}
	else
	{
		takeCameraControl = "Synchronize Camera(F): true";
	}
	DebugAddScreenText(takeCameraControl, Vec2(), fontSize, slot2, -1.f, Rgba8::BLACK, Rgba8::BLACK);

	// std::string lockTarget;
	// if (m_targetLocked)
	// {
	// 	lockTarget = "Target Locked(G): true";
	// }
	// else
	// {
	// 	lockTarget = "Target Locked(G): false";
	// }
	// DebugAddScreenText(lockTarget, Vec2(), fontSize, slot3, -1.f);

	std::string targetInControl;
	if (m_target1_inControl)
	{
		targetInControl = "Controlling solving target(G)";
		DebugAddScreenText(targetInControl, Vec2(), fontSize, slot3, -1.f, m_movingTargetColor);
	}
	else
	{
		targetInControl = "Controlling pole vector(G)";
		DebugAddScreenText(targetInControl, Vec2(), fontSize, slot3, -1.f, m_movingPolveVectorColor);
	}

	// std::string debugTargetPosition = Stringf("Target Position (Arrows/Z/X): ( %.2f, %.2f, %.2f )", m_movingTarget_endEffector.x, m_movingTarget_endEffector.y, m_movingTarget_endEffector.z);
	// DebugAddScreenText(debugTargetPosition, Vec2(), fontSize, slot4, -1.f);

	std::string debugTargetPosition = Stringf("Hand Orientation: ( %.2f, %.2f, %.2f, %.2f )", g_theVRPlayer->m_hands[1].m_orientation.x, g_theVRPlayer->m_hands[1].m_orientation.y, g_theVRPlayer->m_hands[1].m_orientation.z, g_theVRPlayer->m_hands[1].m_orientation.w);
	DebugAddScreenText(debugTargetPosition, Vec2(), fontSize, slot4, -1.f, Rgba8::BLACK, Rgba8::BLACK);

	// std::string debugJointPosition = Stringf("JointC Position : ( %.1f, %.1f, %.1f )", m_debugJoint2Pos.x, m_debugJoint2Pos.y, m_debugJoint2Pos.z);
	// DebugAddScreenText(debugJointPosition, Vec2(), fontSize, Vec2(0.98f, 0.02f), -1.f);	
	// 
	// std::string debugAngleDegrees = Stringf("Rotation Angle: %.1f", ConvertRadiansToDegrees(m_debugAngle));
	// DebugAddScreenText(debugAngleDegrees, Vec2(), fontSize, Vec2(0.98f, 0.02f) + spacing, -1.f);

	std::string solvingStatus;
	if (m_solveTarget)
	{
		solvingStatus = "Solving (C) : O";
		DebugAddScreenText(solvingStatus, Vec2(), fontSize, slot5, -1.f, Rgba8::GREEN);
	}
	else
	{
		solvingStatus = "Solving (C) : X";
		DebugAddScreenText(solvingStatus, Vec2(), fontSize, slot5, -1.f, Rgba8::RED);
	}

	std::string debugChain;
	if (m_debugChain)
	{
		if (m_debugChain->m_solver->m_debugMode)
		{
			debugChain = "Debug chain (I): O";
			DebugAddScreenText(debugChain, Vec2(), fontSize, slot6, -1.f, Rgba8::GREEN);
		}
		else
		{
			debugChain = "Debug chain (I): X";
			DebugAddScreenText(debugChain, Vec2(), fontSize, slot6, -1.f, Rgba8::RED);
		}
	}
	else
	{
		debugChain = "Debug chain (I): X";
		DebugAddScreenText(debugChain, Vec2(), fontSize, slot6, -1.f, Rgba8::RED);
	}

	std::string debugSolverStep = "Press K to step solver forward";
	DebugAddScreenText(debugSolverStep, Vec2(), fontSize, slot7, -1.f, Rgba8::BLACK, Rgba8::BLACK);

	string rightJoystickValue = Stringf("right joystick:(%.2f, %.2f)", g_theVRPlayer->m_hands[Side::RIGHT].m_joyStickPos.x,  g_theVRPlayer->m_hands[Side::RIGHT].m_joyStickPos.y);
	DebugAddScreenText(rightJoystickValue, Vec2(), fontSize, slot8, -1.f, Rgba8::GREEN);

	string crawlerState;
	if (!g_theGame->m_crawlers.empty())
	{
		switch (g_theGame->m_crawlers[0]->m_crawlerCurrentFrameState)
		{
		case CrawlerAnimMode::ADJUSTING:
		{
			crawlerState = Stringf("Anim State: %s", "Adjust");
		}break;
		case CrawlerAnimMode::WALKING:
		{
			crawlerState = Stringf("Anim State: %s", "Walk");
		}break;
		case CrawlerAnimMode::IDLE:
		{
			crawlerState = Stringf("Anim State: %s", "Idle");
		}break;
		case CrawlerAnimMode::ROTATING:
		{
			crawlerState = Stringf("Anim State: %s", "Rotate");
		}break;
		}
	}
	DebugAddScreenText(crawlerState, Vec2(), fontSize, slot9, -1.f, Rgba8::CANDLE_YELLOW);

	DebugAddWorldArrow(m_debugJointPos, m_debugJointPos + m_debugAxis, 0.01f, 0.f, Rgba8::BRIGHT_ORANGE);
}

void WinPlayer::ControlMovingTargetByArrowKeys()
{


}



