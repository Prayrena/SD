#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include <cmath>

Camera::Camera(AABB2 cameraSize)
{
	m_viewport.m_mins = cameraSize.m_mins;
	m_viewport.m_maxs = cameraSize.m_maxs;
}

Camera::Camera()
{
	m_position = Vec3(0.f, 0.f, 0.f);;
	m_orientation = EulerAngles(0.f, 0.f, 0.f);
}

void Camera::update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

Vec2 Camera::GetOrthoBottomLeft()const
{
	return m_viewport.m_mins;
}

Vec2 Camera::GetOrthoTopRight()const
{
	return m_viewport.m_maxs;
}

AABB2 Camera::GetCameraBounds() const
{
	return m_viewport;
}

Vec2 Camera::GetVeiwportDimensions() const
{
	Vec2 dimensions;
	dimensions.x = m_viewport.m_maxs.x - m_viewport.m_mins.x;
	dimensions.y = m_viewport.m_maxs.y - m_viewport.m_mins.y;
	return dimensions;
}

void Camera::Translate2D(const Vec2& translation2D)
{
	m_viewport.m_mins += translation2D;
	m_viewport.m_maxs += translation2D;
}

// if the camera no need to reposition, return false
// use duration time(seconds) to reposition the camera
bool Camera::ZoomInOutCamera(float deltaSeconds, AABB2 newSize, float duration)
{
	if (m_viewport.m_mins == newSize.m_mins && m_viewport.m_maxs == newSize.m_maxs)
	{
		return false;
	}
	float lerpRate = 1.0f / duration;
	Vec2 newBottomLeft = Vec2(Interpolate(m_viewport.m_mins.x, newSize.m_mins.x, m_timer += (lerpRate * deltaSeconds)),
							  Interpolate(m_viewport.m_mins.y, newSize.m_mins.y, m_timer += (lerpRate * deltaSeconds)));
	Vec2 newTopRight = Vec2(Interpolate(m_viewport.m_maxs.x, newSize.m_maxs.x, m_timer += (lerpRate * deltaSeconds)),
							Interpolate(m_viewport.m_maxs.y, newSize.m_maxs.y, m_timer += (lerpRate * deltaSeconds)));

	SetOrthoView(newBottomLeft, newTopRight);
	if (m_timer >= 1.f)
	{
		m_timer = 0.f;
	}
	return true;
}

Mat44 Camera::GetOrthographicMatrix() const
{
	Vec3 LBN(m_viewport.m_mins.x, m_viewport.m_mins.y, m_orthographicNear);
	Vec3 RTF(m_viewport.m_maxs.x, m_viewport.m_maxs.y, m_orthographicFar);
	return Mat44::CreateOrthoProjection(LBN, RTF);
}

Mat44 Camera::GetPerspectiveMatrix() const
{
	float perspectiveActive = Window :: s_theWindow->GetCurrentAspectRatio();
	return Mat44::CreatePerspectiveProjection(m_perspectiveFOV, perspectiveActive, m_perspectiveNear, m_perspectiveFar);
}

// render matrix is appended to the projection matrix, so the transform of the world to render space come first
// then the projection matrix clip the points in render space
// [projection][render][worldToView][localToWorld]
Mat44 Camera::GetProjectionMatrix() const
{
	Mat44 renderMat = GetRenderMatrix();

	switch (m_mode)
	{
	case Camera::eMode_Orthographic: {
		Mat44 projectMat = GetOrthographicMatrix();
		projectMat.Append(renderMat);
		return projectMat; } break;

	case Camera::eMode_Perspective: {
		Mat44 projectMat = GetPerspectiveMatrix();
		projectMat.Append(renderMat);
		return projectMat; } break;

	default: {
		Mat44 projectMat = GetPerspectiveMatrix();
		projectMat.Append(renderMat);
		return projectMat; } break;
	}
}

// should be the normalized viewport be calculated or be memorized by the class
AABB2 Camera::GetNormalizedViewport() const
{
	return m_normalizedViewport;
}

void Camera::SetNormalizedViewport(AABB2 normalizedViewport)
{
	m_normalizedViewport = normalizedViewport;
}

void Camera::SetAspectRatio(float widthHeight)
{
	switch (m_mode)
	{
	case Camera::eMode_Orthographic: {m_orthoAspectRatio = widthHeight; }break;
	case Camera::eMode_Perspective: {m_perspectiveAspect = widthHeight; }break;
	}
}

void Camera::SetOrthoViewport(AABB2 viewport)
{
	m_viewport = viewport;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// this function is used by the world camera, not screen camera!!!
// this is first time used for getting a enemy in world position and get the normalized position on the view port
Vec2 Camera::GetViewportNormolizedPositionForWorldPosition(Vec3 worldPos)
{
	Mat44 projectionMatrix = GetProjectionMatrix();
	Mat44 viewMatrix = GetViewMatrix();

	Mat44 transformMat = projectionMatrix;
	transformMat.Append(viewMatrix);

	Vec3 clipPos = transformMat.TransformPosition3D(worldPos);
	Vec3 NDCPos = clipPos / clipPos.z;
	Vec2 viewportPos;
	viewportPos.x = RangeMap(NDCPos.x, -1.f, 1.f, 0.f, 1.f);
	viewportPos.y = RangeMap(NDCPos.y, -1.f, 1.f, 0.f, 1.f);
	return viewportPos;
}

// Used in Vaporum project to get player's cursor position on the camera near plane to get mouse raycast direction
Vec3 Camera::GetCursorPosOnCameraNearClipPlane(Window const& windows)
{
	float NP_dist = m_perspectiveNear;
	// float theta = GetHorizontalFOV_InDegrees(windows);
	float theta = m_perspectiveFOV;
	float aspectRatio = windows.GetCurrentAspectRatio();
	Vec3 camPosOnNearPlaneInGameWorld;

	// get camera's IJK in world space
	Mat44	rotationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	Vec3	I_basis = rotationMat.GetIBasis3D();
	Vec3	J_basis = rotationMat.GetJBasis3D();
	Vec3	K_basis = rotationMat.GetKBasis3D();

	// we are going to get a, b, c dist on IJK to locate the pos on the near plane
	Vec3 A = NP_dist * I_basis;
	Vec3 B;
	Vec3 C;

	// get near plane size
	float nearPlaneHeight = (NP_dist * tanf( ConvertDegreesToRadians(theta * 0.5f) )) * 2.f;
	float nearPlaneWidth = aspectRatio * nearPlaneHeight;

	Vec2 cursorNormalizedPosOnNearPlane = windows.m_config.m_inputSystem->GetNormalizedCursorPos();		// the result is relative to left bottom corner
	Vec2 cursorRelativePosToNearPlaneCenter = cursorNormalizedPosOnNearPlane - Vec2(0.5f, 0.5f);
	J_basis.x *= -1.f;	// flip J's x direction
	B = RangeMap(cursorRelativePosToNearPlaneCenter.x, 0.f, 1.f, 0.f, nearPlaneWidth) * J_basis;
	C = RangeMap(cursorRelativePosToNearPlaneCenter.y, 0.f, 1.f, 0.f, nearPlaneHeight)* K_basis;

	camPosOnNearPlaneInGameWorld = A + B + C + m_position; // the position is in camera's position world, which should be game world
	return camPosOnNearPlaneInGameWorld;
}

float Camera::GetHorizontalFOV_InDegrees(Window const& windows)
{
	float aspectRatio = windows.GetCurrentAspectRatio();
	float horizontalFOV_radians = ConvertDegreesToRadians(m_perspectiveFOV);
	float verticalFOV_radians = 2.0f * atanf(tanf(horizontalFOV_radians * 0.5f) / aspectRatio);
	float verticalFOV_degrees = ConvertRadiansToDegrees(verticalFOV_radians);
	return verticalFOV_degrees;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
AABB2 Camera::GetOrthoViewport() const
{
	return m_viewport;
}

Vec2 Camera::GetOrthoNearAndFar() const
{
	return Vec2(m_orthographicNear, m_orthographicFar);
}

Vec2 Camera::GetPerspectiveNearAndFar() const
{
	return Vec2(m_perspectiveNear, m_perspectiveFar);
}

void Camera::SetRenderBasis(Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis)
{
	m_renderIBasis = iBasis;
	m_renderJBasis = jBasis;
	m_renderKBasis = kBasis;
}

// view to render matrix
Mat44 Camera::GetRenderMatrix() const
{
	return Mat44(m_renderIBasis, m_renderJBasis, m_renderKBasis, Vec3(0.f, 0.f, 0.f));
}

void Camera::SetTransform(const Vec3& position, const EulerAngles& orientation)
{
	m_position = position;
	m_orientation = orientation;
}

// world to view matrix
// take world coordinate into local camera space
// use this inverse to take object in world space to view space
// [translate][rotate][P]
Mat44 Camera::GetViewMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);
	if (m_useRotationMatrix)
	{
		transformMat.Append(m_rotationMat);
	}
	else
	{
		Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
		transformMat.Append(orientationMat);
	}
	return transformMat.GetOrthonormalInverse();
}

Mat44 Camera::GetModelMatrix() const
{
	// Mat44 transformMat;
	// Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	// transformMat.SetTranslation3D(m_position);
	// transformMat.Append(orientationMat);
	// return transformMat;

	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);
	Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();

	transformMat.Append(orientationMat);
	return transformMat;
}

void Camera::SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight, float nearDist /*= 0.f*/, float farDist /*= 1.f*/, AABB2 normalizedViewport /*= AABB2::ZERO_TO_ONE*/)
{
	m_mode = eMode_Orthographic;

	m_viewport.m_mins = bottomLeft;
	m_viewport.m_maxs = topRight;

	m_orthographicNear = nearDist;
	m_orthographicFar = farDist;

	// get window aspect ratio
	float width = topRight.x - bottomLeft.x;
	float height = topRight.y - bottomLeft.y;
	if (height != 0.f)
	{
		m_orthoAspectRatio = width / height;
	}

	SetNormalizedViewport(normalizedViewport);
}

void Camera::SetOrthoView(AABB2 cameraFrame, float nearDist /*= 0.f*/, float farDist /*= 1.f*/, AABB2 normalizedViewport /*= AABB2::ZERO_TO_ONE*/)
{
	m_mode = eMode_Orthographic;

	m_viewport = cameraFrame;

	m_orthographicNear = nearDist;
	m_orthographicFar = farDist;

	// get window aspect ratio
	float width = cameraFrame.m_maxs.x - cameraFrame.m_mins.x;
	float height = cameraFrame.m_maxs.y - cameraFrame.m_mins.y;
	if (height != 0.f)
	{
		m_orthoAspectRatio = width / height;
	}

	SetNormalizedViewport(normalizedViewport);
}

void Camera::SetPerspectiveView(float aspect, float fov, float nearDist /*= 0.f*/, float farDist /*= 1.f*/, AABB2 normalizedViewport /*= AABB2::ZERO_TO_ONE*/)
{
	m_mode = eMode_Perspective;

	m_perspectiveAspect = aspect;
	m_perspectiveFOV = fov;
	m_perspectiveNear = nearDist;
	m_perspectiveFar = farDist;

	SetNormalizedViewport(normalizedViewport);
}
