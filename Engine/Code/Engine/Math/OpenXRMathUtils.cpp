#include "Engine/Math/OpenXRMathUtils.hpp"
#include "ThirdParty/OpenXR/include/openxr/openxr.h"

Mat44 GetGameToDirectXMat()
{
	return Mat44(Mat44(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3()));
}

Mat44 GetDirectXToGameMat()
{
	// 	return Mat44(Vec3(0.f, -1.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(1.f, 0.f, 0.f), Vec3());

	Mat44 result = GetGameToDirectXMat();
	result.Transpose();
	return result;
}

// OpenXR is right-handed system and X-right, Y-up, Z-backward
Mat44 GetGameToOpenXRMat()
{
	return Mat44(Vec3(0.f, 0.f, -1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3()); // post multiplication this
}

Mat44 GetOpenXRToGameMat()
{
	// return Mat44(Vec3(0.f, -1.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3());

	Mat44 result = GetGameToOpenXRMat();
	result.Transpose();
	return result;
}

Mat44 GetOpenXRToDirectXMat()
{
	return Mat44(Vec3(1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, -1.f), Vec3()); // post multiplication this
}

Mat44 GetDirectXToOpenXRMat()
{
	return Mat44(Vec3(1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, -1.f), Vec3()); // post multiplication this
}

Quat TransformQuatFromGameToOpenXR(Quat const& q)
{
	return Quat(-q.y, q.z, -q.x, q.w);
}

Quat TransformQuatFromOpenXRToGame(Quat const& q)
{
	return Quat(-q.z, -q.x, q.y, q.w);
}

const char* XrResultToString(XrResult result)
{
	switch (result) {
	case XR_SUCCESS: return "XR_SUCCESS";
	case XR_TIMEOUT_EXPIRED: return "XR_TIMEOUT_EXPIRED";
	case XR_SESSION_LOSS_PENDING: return "XR_SESSION_LOSS_PENDING";
	case XR_EVENT_UNAVAILABLE: return "XR_EVENT_UNAVAILABLE";
	case XR_SPACE_BOUNDS_UNAVAILABLE: return "XR_SPACE_BOUNDS_UNAVAILABLE";
	case XR_SESSION_NOT_FOCUSED: return "XR_SESSION_NOT_FOCUSED";
	case XR_FRAME_DISCARDED: return "XR_FRAME_DISCARDED";
	case XR_ERROR_VALIDATION_FAILURE: return "XR_ERROR_VALIDATION_FAILURE";
	case XR_ERROR_RUNTIME_FAILURE: return "XR_ERROR_RUNTIME_FAILURE";
	case XR_ERROR_OUT_OF_MEMORY: return "XR_ERROR_OUT_OF_MEMORY";
	case XR_ERROR_API_VERSION_UNSUPPORTED: return "XR_ERROR_API_VERSION_UNSUPPORTED";
	case XR_ERROR_INITIALIZATION_FAILED: return "XR_ERROR_INITIALIZATION_FAILED";
	case XR_ERROR_FUNCTION_UNSUPPORTED: return "XR_ERROR_FUNCTION_UNSUPPORTED";
	case XR_ERROR_FEATURE_UNSUPPORTED: return "XR_ERROR_FEATURE_UNSUPPORTED";
	case XR_ERROR_SIZE_INSUFFICIENT: return "XR_ERROR_SIZE_INSUFFICIENT";
	case XR_ERROR_HANDLE_INVALID: return "XR_ERROR_HANDLE_INVALID";
	case XR_ERROR_INSTANCE_LOST: return "XR_ERROR_INSTANCE_LOST";
	case XR_ERROR_SESSION_RUNNING: return "XR_ERROR_SESSION_RUNNING";
	case XR_ERROR_SESSION_NOT_RUNNING: return "XR_ERROR_SESSION_NOT_RUNNING";
	case XR_ERROR_TIME_INVALID: return "XR_ERROR_TIME_INVALID";
	case XR_ERROR_PATH_INVALID: return "XR_ERROR_PATH_INVALID";
	case XR_ERROR_PATH_COUNT_EXCEEDED: return "XR_ERROR_PATH_COUNT_EXCEEDED";
	case XR_ERROR_PATH_FORMAT_INVALID: return "XR_ERROR_PATH_FORMAT_INVALID";
	case XR_ERROR_PATH_UNSUPPORTED: return "XR_ERROR_PATH_UNSUPPORTED";
	case XR_ERROR_LAYER_INVALID: return "XR_ERROR_LAYER_INVALID";
	case XR_ERROR_LAYER_LIMIT_EXCEEDED: return "XR_ERROR_LAYER_LIMIT_EXCEEDED";
	case XR_ERROR_SWAPCHAIN_RECT_INVALID: return "XR_ERROR_SWAPCHAIN_RECT_INVALID";
	case XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED: return "XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED";
	case XR_ERROR_ACTION_TYPE_MISMATCH: return "XR_ERROR_ACTION_TYPE_MISMATCH";
	case XR_ERROR_SESSION_NOT_READY: return "XR_ERROR_SESSION_NOT_READY";
	case XR_ERROR_SESSION_NOT_STOPPING: return "XR_ERROR_SESSION_NOT_STOPPING";
	default: return "XR_UNKNOWN_ERROR";
	}
}

//bool DoPlaneIntersectZCylinder(Plane3 plane, ZCylinder box)
//{
//	// we are going to check the intersection from XY, XZ and YZ view
//
//}

void ZCylinder::SetUniformScale(float uniformScale)
{
	Radius *= uniformScale;
	float rangeCenter = (MinMaxZ.m_min + MinMaxZ.m_max) * 0.5f;
	float rangeHalfLength = MinMaxZ.GetRangeLength() * 0.5f * uniformScale;
	MinMaxZ = FloatRange(rangeCenter - rangeHalfLength, rangeCenter + rangeHalfLength);
}

void XRPose::operator=(XRPose const& copy)
{
	this->orientation = copy.orientation;
	this->position = copy.position;
}

XRPose::XRPose(Vec3 p, Quat q)
	: position(p)
	, orientation(q)
{

}

XrVector3f GetXRVec3f(Vec3 const& v)
{
	XrVector3f xrV;
	xrV.x = v.x;
	xrV.y = v.y;
	xrV.z = v.z;
	return xrV;
}

XrQuaternionf GetXRQuat(Quat const& q)
{
	XrQuaternionf xrQ;
	xrQ.x = q.x;
	xrQ.y = q.y;
	xrQ.z = q.z;
	xrQ.w = q.w;
	return xrQ;
}

XrPosef MakeXrPosef(Vec3 const& v, Quat const& q)
{
	XrQuaternionf XrQuat = GetXRQuat(q);
	XrVector3f XrPos = GetXRVec3f(v);
	XrPosef pose;
	pose.orientation = XrQuat;
	pose.position = XrPos;
	return pose;
}

