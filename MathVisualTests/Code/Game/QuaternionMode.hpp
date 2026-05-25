#pragma once
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Plane3.hpp"
#include "Engine/Math/Splines.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameMode.hpp"

class Player;
class Prop;
class VertexBuffer;

constexpr int NUM_QUAT = 4;

enum class QUAT
{
	ONE,
	TWO,
	NLERP,
	SLERP,
	NUM_QUAT
};

class QuaternionMode : public GameMode
{
public:
	QuaternionMode();
	~QuaternionMode();
	void Startup() override;
	void Update(float deltaSeconds) override;
	void Render() const override;//mark for that the render is not going to change the variables
	void Shutdown() override;

	void GenerateWorldGridsAndAxes();

	// sphere settings
	virtual void CreateRandomShapes() override;	// we only create one sphere for showcase quaternion
	void	CreateVerticeAndVertexBufferForSphere();
	Mat44	GetSphereModelMatrix() const;
	void	RenderSphere() const;
	Vec3 m_sphereCenter = Vec3(2.f, 2.f, 2.f);
	EulerAngles m_sphereOrientation = EulerAngles();
	float m_sphereRadius = 1.f;
	Rgba8 m_sphereDefaultColor = Rgba8::BLUE_MVT;

	std::vector<Vertex_PCU> m_sphereVerts;
	float					m_cameraDist = 3.f;
	VertexBuffer*			m_sphereVertexBuffer = nullptr;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// debug arrow
	Quat				m_quatArray[NUM_QUAT] = {};

	void		ControlQuaternion();
	void		InputNumKeyToSetQuat();
	void		AddVertsAndCreateVertexBufferForAllDebugArrows();
	Mat44		GetQuat1ModelMatrix() const;
	Mat44		GetQuat2ModelMatrix() const;
	Mat44		GetQuat3ModelMatrix() const;
	Mat44		GetQuat4ModelMatrix() const;
	void		RenderAllArrows() const;
	// Quat		m_quat_1 = Quat();
	bool		m_showQuat_1 = true;
	bool		m_showQuat_2 = true;

	float		m_arrowLength = 1.5f;
	float		m_arrowRadius = 0.05f;

	std::vector<Vertex_PCU> m_quat1_Verts;
	VertexBuffer*			m_quat1_VertexBuffer = nullptr;

	std::vector<Vertex_PCU> m_quat2_Verts;
	VertexBuffer* m_quat2_VertexBuffer = nullptr;

	std::vector<Vertex_PCU> m_quat3_Verts;
	VertexBuffer* m_quat3_VertexBuffer = nullptr;	
	
	std::vector<Vertex_PCU> m_quat4_Verts;
	VertexBuffer* m_quat4_VertexBuffer = nullptr;

	Rgba8 m_quatColorArray[5] = {Rgba8::BURNT_RED, Rgba8::CYSTAL_BLUE, Rgba8::CANDLE_YELLOW_TRANSPARENT, Rgba8::PINK_TRANSPARENT};

	void		UpdateQuatBlending();

	float		m_blendingWeight = 0.f;
	float		m_blendingWeightChangeSpeed = 0.001f;
	float		m_blendingTimerChangeSpeed = 0.01f;

	bool		m_blendingIsPaused = true;
	Timer*		m_blendingTimer = nullptr;

	// testing euler angle conversion
	EulerAngles m_eulerAngle = EulerAngles();
	Rgba8 m_eulerAngleColor = Rgba8::GREEN_TRANSPARENT;
	Mat44 GetEulerAngleModelMatrix() const;

	bool m_showEulerAngle = false;

	std::vector<Vertex_PCU> m_eulerAngle_Verts;
	VertexBuffer* m_eulerAngle_VertexBuffer = nullptr;

//----------------------------------------------------------------------------------------------------------------------------------------------------
	void	UpdateQuatShortestPathSpline();
	void	AddingVertsForMovingPathSpline();
	void	RenderMovingPath() const;
	void	ControlTrajectory();

	bool						m_showTrajectory = true;
	SamplePointsCurve3D			m_quatTrajectory[NUM_QUAT] = {};
	int							m_quatIndexInControl = 0;
	std::vector<Vertex_PCU>		m_pathSplineVertexs;
	Timer*						m_splineFlowingTimer = nullptr;
	float						m_splineFlowingInterval = 0.03f;
	float						m_drawingSplineRadius = 0.015f;

	float						m_quatAxisRadius = 0.01f;

	int							m_numTrajectorySegments = 50;
	int							m_drawingSectionNum = 5;
	int							m_drawingSectionIndex = 0;

	// speed control
	float m_quaternionChangeRate = 0.05f;

	// debug info
	virtual void UpdateModeInfo() override;
	void	UpdateDebugMessages();
	void	RenderDebugMessages();

	// raycast and collision
	bool  m_hasGrabbedAShape = false;
	bool  m_raycastIsLock = false;

	float m_rayDist = 20.f;
	Vec3  m_rayFwdNormal;
	Vec3  m_rayStart;
	Vec3  m_rayEnd;

	std::vector<Vertex_PCU> m_raycastVerts;
	RaycastResult3D m_closetRaycastResult;

	void ClickToGrabAndReleaseShape();

	void RenderDebugRenderSystem() const;

	Player* m_player = nullptr;
	void RenderWorldInPlayerCamera() const;

	std::vector<Prop*> m_gridsAndAxes;

	// event 
	void		ScribeQuatModeCommands();
	static bool Command_SetQuatIndex(EventArgs& args);
	static bool Command_SetQuatValue(EventArgs& args);
}; 
