#pragma once
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------------------------------------------------------
// VR stuff

struct XrVector3f;
struct XrQuaternionf;
struct XrPosef;

struct XRPose
{
	XRPose() = default;
	XRPose(Vec3 p, Quat q);
	~XRPose() = default;

	Vec3 position;
	Quat orientation;

	void operator = (XRPose const& copy);
};

// VR matrix calculation
Mat44 GetGameToDirectXMat();
Mat44 GetDirectXToGameMat();

Mat44 GetGameToOpenXRMat();
Mat44 GetOpenXRToGameMat();

Mat44 GetOpenXRToDirectXMat();
Mat44 GetDirectXToOpenXRMat();

XrVector3f GetXRVec3f(Vec3 const& v);
XrQuaternionf GetXRQuat(Quat const& q);
XrPosef	MakeXrPosef(Vec3 const& v, Quat const& q);

const char* XrResultToString(XrResult result);