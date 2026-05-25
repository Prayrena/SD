#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/Renderer/Window.hpp"

class Camera 
{
public:

	enum Mode
	{
		eMode_Orthographic,
		eMode_Perspective,

		eMode_Count
	};

	Camera();
	~Camera() {};
	Camera(AABB2 cameraSize);

	void update(float deltaSeconds);

	void SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight, float near = 0.f, float far = 1.f, AABB2 normalizedViewport = AABB2::ZERO_TO_ONE);
	void SetOrthoView(AABB2 cameraFrame, float near = 0.f, float far = 1.f, AABB2 normalizedViewport = AABB2::ZERO_TO_ONE);
	void SetPerspectiveView(float aspect, float fov, float near = 0.1f, float far = 100.f, AABB2 normalizedViewport = AABB2::ZERO_TO_ONE);

	Vec2 GetOrthoBottomLeft() const;
	Vec2 GetOrthoTopRight() const;
	AABB2 GetCameraBounds() const;
	Vec2 GetVeiwportDimensions() const;

	AABB2 GetNormalizedViewport() const;
	void SetNormalizedViewport(AABB2 normalizedViewport);
	AABB2 GetOrthoViewport() const;
	void  SetOrthoViewport(AABB2 viewport);

	Vec2 GetViewportNormolizedPositionForWorldPosition(Vec3 worldPos);
	Vec3 GetCursorPosOnCameraNearClipPlane(Window const& windows);

	float GetHorizontalFOV_InDegrees(Window const& windows);

	void  SetAspectRatio(float widthHeight);

	void Translate2D(const Vec2& translation2D);

	Mat44 GetOrthographicMatrix() const;
	Mat44 GetPerspectiveMatrix() const;
	Mat44 GetProjectionMatrix() const;
	
	Vec2  GetOrthoNearAndFar() const;
	Vec2  GetPerspectiveNearAndFar() const;

	void SetRenderBasis(Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis);
	Mat44 GetRenderMatrix() const;

	bool	m_useRotationMatrix = false;
	Mat44	m_rotationMat = Mat44();
	Vec3	m_renderIBasis = Vec3(1.f, 0.f, 0.f);
	Vec3	m_renderJBasis = Vec3(0.f, 1.f, 0.f);
	Vec3	m_renderKBasis = Vec3(0.f, 0.f, 1.f);

	void SetTransform(const Vec3& position, const EulerAngles& orientation);
	Mat44 GetViewMatrix() const; // the inverse of world to view transform matrix, which uses the camera position and orientation
	Mat44 GetModelMatrix() const; // camera transform info, including position and orientation

	Vec3 m_position;
	EulerAngles m_orientation;

	bool ZoomInOutCamera(float deltaSeconds, AABB2 newSize, float duration);

	float m_zoomingSpeed = 0.1f;
	float m_timer = 0.f;

	Mode m_mode = eMode_Orthographic;

private:
	AABB2 m_viewport; // BL is (0.f, 0.f), TR is (2000.f, 1000.f) this is screen unit used for ortho camera to display 2D content on screen
	AABB2 m_normalizedViewport = AABB2();

	float m_orthoAspectRatio = 0.f; // width / height

	float m_orthographicNear;
	float m_orthographicFar;

	float m_perspectiveAspect;
	float m_perspectiveFOV = 60.f;
	float m_perspectiveNear = 0.01f;
	float m_perspectiveFar = 100.f;
};