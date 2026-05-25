#include "Game/PlayerHand.hpp"
#include "ThirdParty/OpenXR/include/openxr/openxr.h"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/OpenXRMathUtils.hpp"
#include "Game/Game.hpp"
#include "Game/VRPlayer.hpp"
#include "openxr_program.h"

extern Game* g_theGame;
extern Renderer* g_theRenderer;
extern VRPlayer* g_theVRPlayer;
extern ThePhysX* g_thePhysX;
extern Clock* g_theGameClock;
extern OpenXrProgram* g_theApp;

using namespace std;
using namespace physx;

PlayerHand::PlayerHand()
{

}

PlayerHand::~PlayerHand()
{
	if (m_model)
	{
		delete m_model;
	}
}

void PlayerHand::Update()
{
	UpdateHandPoseInGameWorld();
	AddVertsForHand();

	if (m_vibrationTimer)
	{
		if (!m_vibrationTimer->IsStopped() && !m_vibrationTimer->HasPeroidElapsed())
		{
			float timeFraction = m_vibrationTimer->GetElapsedFraction();
			float currentAmplitude = RangeMapClamped(timeFraction, 0.f, 1.f, m_amplitude, 0.f);

			XrHapticVibration vibration{ XR_TYPE_HAPTIC_VIBRATION };
			vibration.amplitude = currentAmplitude;
			vibration.duration = XR_MIN_HAPTIC_DURATION;
			vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

			XrHapticActionInfo hapticActionInfo{ XR_TYPE_HAPTIC_ACTION_INFO };
			hapticActionInfo.action = g_theApp->m_input.vibrateAction;
			hapticActionInfo.subactionPath = g_theApp->m_input.handSubactionPath[m_handIndex];
			xrApplyHapticFeedback(g_theApp->m_session, &hapticActionInfo, (XrHapticBaseHeader*)&vibration);
		}
		else if (m_vibrationTimer)
		{
			m_vibrationTimer->Stop();
		}
	}
}

void PlayerHand::Render() const
{
	g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_NONE);
	g_theRenderer->BindTexture(nullptr);
	if (!m_debugVerts.empty() && g_theVRPlayer->m_skeleton->m_drawSkeletonDebug)
	{
		g_theRenderer->SetModelConstants(GetModelMatrix());
		g_theRenderer->DrawVertexArray((int)(m_debugVerts.size()), m_debugVerts.data());
	}

	if (m_model)
	{
		g_theRenderer->SetModelConstants(GetModelMatrix());
		m_model->Render();
	}
}

void PlayerHand::UpdateHandPoseInGameWorld()
{
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Mat44 renderMat(Vec3(0.f, 0.f, -1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3());
	// renderMat.Transpose();
	// Mat44 actionSpaceMat(m_actionSpacePose);
	// m_pos = (renderMat.MatMultiply(actionSpaceMat)).GetTranslation3D();

	// the position is correct
	// Mat44 ActionSpaceToGame = GetGameToActionSpaceMat();
	// Mat44 translationMat(m_actionSpacePose.position);
	// translationMat = ActionSpaceToGame.MatMultiply(translationMat);

	Quat rotationInOpenXR(m_actionSpacePose.orientation);

	Quat trackingRotationInGameWorld = GetOpenXRToGameMat().TransformQuaternion(rotationInOpenXR);

	// adjust rotation based on player rotation
	// Quat(final) = Q(current) * Q(new)
	Quat playerWorldRotation = ConvertEulerAnglesToQuat(g_theVRPlayer->m_orientation);
	m_orientation = playerWorldRotation * trackingRotationInGameWorld;

	Vec3 posRelativeToGameOrigion = GetOpenXRToGameMat().TransformPosition3D(Vec3(m_actionSpacePose.position));
	Vec3 localPos = g_theVRPlayer->m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformPosition3D(posRelativeToGameOrigion);
	// Vec3 localPos = m_orientation.RotateVector(posRelativeToGameOrigion);
	m_posLastFrame = m_pos;
	m_pos = g_theVRPlayer->m_position + localPos;

	Quat adjustment = Quat::CreateRotationAroundYAxis(80.f); // originally when there is no rotation, there is extra pitch, we want to cancel that
	m_orientation *= adjustment;

	// if (m_handIndex == Side::RIGHT)
	// {
	// 	// Log::Write(Log::Level::Info, Fmt("hand %i", m_handIndex));
	// 	// Log::Write(Log::Level::Info, Fmt("Right Hand Orientation: X = %.02f,Y = %.02f, Z = %.02f", rotation.x, rotation.y, rotation.z));
	// }
}

void PlayerHand::AddVertsForHand()
{	
	if (m_isActive)
	{
		std::vector<Vertex_PCU>& verts = m_debugVerts;
		verts.clear();
		if (m_overlapWithCube)
		{
			AddVertsForSphere3D(verts, Vec3(), HAND_SPHERE_RADIUS, Rgba8::YELLOW);
		}
		else
		{
			AddVertsForSphere3D(verts, Vec3(), HAND_SPHERE_RADIUS, Rgba8::PASTEL_BLUE);
		}

		AddVertsForArrow3D(verts, Vec3(), Vec3(m_debugArrowLength, 0.f, 0.f), m_debugArrowRadius, Rgba8::RED, Rgba8::RED);
		AddVertsForArrow3D(verts, Vec3(), Vec3(0.f, m_debugArrowLength, 0.f), m_debugArrowRadius, Rgba8::GREEN, Rgba8::GREEN);
		AddVertsForArrow3D(verts, Vec3(), Vec3(0.f, 0.f, m_debugArrowLength), m_debugArrowRadius, Rgba8::BLUE, Rgba8::BLUE);
	}
}

Mat44 PlayerHand::GetModelMatrix() const
{
	// this is how we set the model matrix in Doomenstein, this is 100% correct
	// Mat44 transformMat;
	// transformMat.SetTranslation3D(m_position);
	// Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	// transformMat.Append(orientationMat);
	// return transformMat;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	Mat44 transformMat(m_pos);
	Mat44 orientationMat(m_orientation);
	Mat44 modelMat = transformMat.MatMultiply(orientationMat);

// #ifdef VR_MODE
// // for VR games, we might need to reset game world
// 	modelMat = g_theGame->m_toNewResetGameWold.SimilarityTransformation(modelMat);
// #endif

	return modelMat;
}

Vec3 PlayerHand::GetWorldPosInGame() const
{
	return GetModelMatrix().GetTranslation3D();
}

void PlayerHand::StartControllerVibration(float duration, float amplitude)
{
	m_amplitude = amplitude;

	if (!m_vibrationTimer)
	{
		m_vibrationTimer = new Timer(duration, g_theGameClock);
	}
	else
	{
		m_vibrationTimer->m_period = duration;
		m_vibrationTimer->Restart();
	}
}

