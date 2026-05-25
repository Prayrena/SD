#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameMode.hpp"
#include <vector>

class App;
class UI;
class RandomNumberGenerator;
struct LineSegment2;

// record every disc center and radius information into an array
class PachinkoBall
{
public:
	PachinkoBall(Vec2 pos, float radius, Vec2 vel, Rgba8 color, float elasticity);
	~PachinkoBall() {}

	Vec2 m_discCenter = Vec2::ZERO;
	float m_discRadius = 0.f;
	Vec2 m_velocity = Vec2();
	float m_elasticity = 0.9f;

	Rgba8 m_color = Rgba8::BLUE_MVT;

	bool VelocityIsConvergentToFixedPoint(Vec2 fixedPos);
	void AddVertsForBalls(std::vector<Vertex_PCU>& verts);
	void Update();
};

class PachinkoBumper
{
public:
	PachinkoBumper(float elasticity);
	~PachinkoBumper() {}

	virtual void AddVerts(std::vector<Vertex_PCU>& verts) = 0;
	virtual void BounceBallOffOfBumper(PachinkoBall& ball) = 0;

	Rgba8 GetBumperColorForElasticity();

	float m_elasticity = 0.f;
};

class OBBPachinkoBumper: public PachinkoBumper
{
public:
	OBBPachinkoBumper(OBB2* shape, float elasticity);
	~OBBPachinkoBumper() {}

	virtual void AddVerts(std::vector<Vertex_PCU>& verts) override;
	virtual void BounceBallOffOfBumper(PachinkoBall& ball) override;

	OBB2 m_OBB2;
};

class DiscPachinkoBumper : public PachinkoBumper
{
public:
	DiscPachinkoBumper(Vec2 center, float radius, float elasticity);
	~DiscPachinkoBumper() {}

	virtual void AddVerts(std::vector<Vertex_PCU>& verts) override;
	virtual void BounceBallOffOfBumper(PachinkoBall& ball) override;

	Vec2 m_center;
	float m_radius = 0.f;
};

class CapsulePachinkoBumper : public PachinkoBumper
{
public:
	CapsulePachinkoBumper(Capsule2* capsule, float elasticity);
	~CapsulePachinkoBumper() {}

	virtual void AddVerts(std::vector<Vertex_PCU>& verts) override;
	virtual void BounceBallOffOfBumper(PachinkoBall& ball) override;

	Capsule2 m_capsule;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
class Pachinko2D : public GameMode
{
public:
	Pachinko2D();
	~Pachinko2D();
	void Startup() override;
	void Update(float deltaSeconds) override;
	void Render() const override;//mark for that the render is not going to change the variables
	void Shutdown() override;
	void ShutDownUIElementList(int arraySize, UI** entityArrayPointer);

	// UI functions
	void RenderUIElements() const;

	// placing random shape in the world
	Vec2 GetRandomPosInWorld(Vec2 worldSize);

	// adding shapes
	int m_numEachTypeBumper = 10;

	virtual void CreateRandomShapes() override;
	void AddVertsForBalls();
	void AddVertsForGeneratingRay();
	void AddVertsForBumpers();

	// reference ray
	void SpawnNewBallAtRayByInput();

	virtual void UpdateModeInfo() override;

	float m_arrowSize = WORLD_SIZE_Y * 0.03f;
	float m_arrowLineThickness = 0.15f;

	int m_numDiscSides = 36;
	Vec2 m_diskCenter;
	float m_diskRadius;

	Vec2 m_sectorTip;
	float m_sectorRadius;
	Vec2 m_sectorForwardNormal;
	float m_sectorApertureDegrees;

	// ray related
	float m_spawnRingThickness = 0.24f;
	float m_maxBallRadius = 4.2f;
	float m_minBallRadius = 1.8f;
	float m_initialSpeedMultiplier = 3.f;

	// Bumper settings
	float m_maxCircleBumperRadius = WORLD_SIZE_Y * 0.06f;
	float m_minCircleBumperRadius = WORLD_SIZE_Y * 0.006f;
	float m_maxCapsuleBumperLength = WORLD_SIZE_Y * 0.12f;
	float m_minCapsuleBumperLength = WORLD_SIZE_Y * 0.02f;

	// ptr lists
	std::vector<PachinkoBall*> m_balls; 
	std::vector<PachinkoBumper*> m_bumpers;

	// verts list
	std::vector<Vertex_PCU> m_ballVerts;
	std::vector<Vertex_PCU> m_bumperVerts;

	// DiscRaycastObject m_highlightDisk
	// rendering shapes
	void RenderAllBumpers() const;
	void RenderBallsAndRay() const;

	// input control
	void UpdateFixedTimeStepControl();

	// camera functions
	void UpdateCameras(float deltaSeconds);
	float m_screenShakeAmount = 0.f;
	bool  m_cameraShakeIsDisplayed = true;

	bool  m_gameIsOver					 = false;
	float m_returnToStartTimer			 = 0.f;
	bool  m_UIisDisplayed				 = true;
	int	  m_playerLivesNum				 = PLAYER_LIVES_NUM;
	float m_introTimer = 0.f;

	// time step
	float m_isUsingFixedPhysicsTimestep = true;
	float m_physicsTimeOwed = 0.f;
	float m_physicsFixedTimestep = 0.01f;
	float m_wallElasticity = 0.95f;

	// physics settings
	float m_gravityScale = 100.f;
	bool m_buttomWrap = true;

	// color
	static Rgba8 const s_hardElasticityColor;
	static Rgba8 const s_softElasticityColor;
	static Rgba8 const s_deepBallColor;
	static Rgba8 const s_lightBallColor;
	static Rgba8 const s_backgroundColor;

private:
	bool CheckUIEnabled(UI* const UI) const;
	bool m_OpenScene = true;

	// Collision Detecting
	void UpdatePhysics(float timeStep);
	void MoveBallByGravityAndVelocity(PachinkoBall& ball, float deltaSeconds);
	void CheckBallVsBall(PachinkoBall& a, PachinkoBall& b);
	void CheckBallVsWalls(PachinkoBall& ball);
	void CheckBallVsBumper(PachinkoBall& ball, PachinkoBumper& bumper);
};
