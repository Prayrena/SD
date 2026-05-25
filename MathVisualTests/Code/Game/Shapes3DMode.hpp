#pragma once
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Plane3.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameMode.hpp"

class Player;
class Prop;

enum class GeometryType
{
	AABB3D,
	OBB3,
	ZCYLINDER,
	SPHERE,
	PLANE,
	COUNT
};

class TestShape
{
public:
	TestShape(GeometryType type, Vec3 pos)
	: m_type(type)
	, m_position(pos)
	{}
	virtual ~TestShape() {}

	virtual void Startup();
	virtual void Update();
	virtual void AddVertsForShape() = 0;
	virtual void UpdateShapeInfoInWorldSpace() = 0;
	virtual void Render() const;
	virtual void RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) = 0;
	
	void ChangeColorAccordingToStatus();

	Mat44 GetModelMatrix() const;
	Mat44 m_modelMatrix;

	RaycastResult3D m_raycastResult;
	GeometryType	m_type = GeometryType::COUNT;
	Vec3			m_nearestPt;
	Vec3			m_position; // we the player grab the shape, it just move the center
	EulerAngles		m_orientation;
	std::vector<Vertex_PCU> m_vertices;
	Rgba8			m_color;
	
	int  m_numOverlap = 0;
	bool m_isTheCloestImpactedShape = false;
	bool m_isOverlaped = false;
	bool m_renderInWireframe = false;
	bool m_grabbedByPlayer = false;
};

// class Plane3D_RaycastResult : public TestShape
// {
// 	Plane3 m_plane;
// 
// 	virtual void AddVertsForShape() override;
// 	Vec3 GetNearestPoint() const;
// };

class AABB3_RaycastResult: public TestShape
{
public:
	AABB3_RaycastResult(GeometryType type, Vec3 pos, AABB3 aabb3)
		: TestShape(type, pos)
		, m_AABB3Info(aabb3)
	{};
	~AABB3_RaycastResult() {};

	// virtual void Startup() override;
	virtual void Update() override; // the shape info will update according to the center
	virtual void AddVertsForShape() override;
	virtual void UpdateShapeInfoInWorldSpace() override;
	// virtual void Render() const override;
	virtual void RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) override;

	AABB3			m_AABB3Info;
	AABB3			m_AABB3InWorld;
};

class OBB3_RaycastResult : public TestShape
{
public:
	OBB3_RaycastResult(GeometryType type, Vec3 pos, OBB3 obb3)
		: TestShape(type, pos)
		, m_OBB3Info(obb3)
	{};
	~OBB3_RaycastResult() {};

	// virtual void Startup() override;
	virtual void Update() override; // the shape info will update according to the center
	virtual void AddVertsForShape() override;
	virtual void UpdateShapeInfoInWorldSpace() override;
	// virtual void Render() const override;
	virtual void RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) override;

	OBB3			m_OBB3Info;
	OBB3			m_OBB3InWorld;
};

class Plane_RaycastResult : public TestShape
{
public:
	Plane_RaycastResult(GeometryType type, Vec3 pos, Plane3 plane)
		: TestShape(type, pos)
		, m_planeInfo(plane)
	{};
	~Plane_RaycastResult() {};

	// virtual void Startup() override;
	virtual void Update() override; // the shape info will update according to the center
	virtual void AddVertsForShape() override;
	virtual void UpdateShapeInfoInWorldSpace() override;
	// virtual void Render() const override;
	virtual void RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) override;

	Plane3 m_planeInfo;
	Plane3 m_planeInWorld;
};

class Sphere_RaycastResult : public TestShape
{
public:
	Sphere_RaycastResult(GeometryType type, Vec3 pos, Sphere sphere)
		: TestShape(type, pos)
		, m_sphereInfo(sphere)
	{};
	~Sphere_RaycastResult() {};

	// virtual void Startup() override;
	virtual void Update() override;
	virtual void AddVertsForShape() override;
	virtual void UpdateShapeInfoInWorldSpace() override;
	// virtual void Render() const override;
	virtual void RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) override;

	Sphere	m_sphereInfo;
	Sphere  m_sphereInWorld;
};

class ZCylinder_RaycastResult : public TestShape
{
public:
	ZCylinder_RaycastResult(GeometryType type, Vec3 pos, ZCylinder zcylinder)
		: TestShape(type, pos)
		, m_ZCylinderInfo(zcylinder)
	{};
	~ZCylinder_RaycastResult() {};

	// virtual void Startup() override;
	virtual void Update() override;
	virtual void AddVertsForShape() override;
	virtual void UpdateShapeInfoInWorldSpace() override;
	// virtual void Render() const override;
	virtual void RaycastVSShape(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) override;

	ZCylinder			m_ZCylinderInfo;
	ZCylinder			m_cylinderInWorld;
};

class Shapes3DMode : public GameMode
{
public:
	Shapes3DMode();
	~Shapes3DMode();
	void Startup() override;
	void Update(float deltaSeconds) override;
	void Render() const override;//mark for that the render is not going to change the variables
	void Shutdown() override;

	void GenerateWorldGridsAndAxes();
	virtual void CreateRandomShapes() override;

	virtual void UpdateModeInfo() override;

	// raycast and collision
	TestShape* m_impactShape = nullptr;
	TestShape* m_grabbedShape = nullptr;
	bool  m_hasGrabbedAShape = false;
	bool  m_raycastIsLock = false;

	float m_rayDist = 20.f;
	Vec3  m_rayFwdNormal;
	Vec3  m_rayStart;
	Vec3  m_rayEnd;

	std::vector<Vertex_PCU> m_raycastVerts;
	RaycastResult3D m_closetRaycastResult;

	void CheckIfShapesAreOverlapped();
	void ShootRaycastForCollisionTest();
	RaycastResult3D RaycastAllShapes(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist);
	void GenerateWorldAxesInfrontOfCamera();
	void ClickToGrabAndReleaseShape();

	void RenderAll3DShapes() const;
	void RenderLockRaycast() const;
	void RenderDebugRenderSystem() const;

	Player* m_player = nullptr;
	Texture* m_texture = nullptr;
	void RenderWorldInPlayerCamera() const;

	std::vector<Prop*> m_gridsAndAxes;
	std::vector<TestShape*> m_shapes;
}; 
