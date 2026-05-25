#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/GameMode.hpp"
#include "Game/ReferencePoint.hpp"
#include <vector>

class App;
class ReferencePoint;
class Entity;
class UI;
class RandomNumberGenerator;
struct OBB2;
struct LineSegment2;
struct Capsule2;

class GetNearestPointsMode : public GameMode
{
public:
	GetNearestPointsMode();
	~GetNearestPointsMode();
	virtual void Startup() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;//mark for that the render is not going to change the variables
	virtual void Shutdown() override;
	void ShutDownUIElementList(int arraySize, UI** m_entityArrayPointer);

	// UI functions
	void RenderUIElements() const;
	void RenderUIElementList(int arraySize, UI* const* m_UIElementArrayPointer) const;
	void UpdateUIElementList(int arraySize, UI* const* m_UIElementArrayPointer, float deltaSeconds) const;

	// placing random shape in the world
	Vec2 GetRandomPosInWorld(Vec2 worldSize);
	virtual void CreateRandomShapes() override;

	// adding shapes
	void CreateRandomOBB2();
	void CreateRandomLineSegment2();
	void CreateRandomInfiniteLine2();
	void CreateRandomCapsule2();
	void CreateRandomDisk2();
	void CreateRandomAABB2();
	void CreateRandomSector2();

	void CreateTestingPoint();
	void CreateNearestPointOnOBB2();
	void CreateNearestPointOnLineSegment2();
	void CreateNearestPointOnInifiniteLine2();
	void CreateNearestPointOnCapsule2();
	void CreateNearestPointOnAABB2();
	void CreateNearestPointOnDisk2();
	void CreateNearestPointOnSector2();

	virtual void UpdateModeInfo() override;

	OBB2* m_OBB2 = nullptr;
	LineSegment2* m_lineSegment = nullptr;
	LineSegment2* m_infiniteLine = nullptr;
	Capsule2* m_capsule = nullptr;
	AABB2* m_AABB2 = nullptr;
	Vec2 m_diskCenter;
	float m_diskRadius;

	Vec2 m_sectorTip;
	float m_sectorRadius = 5.f;
	Vec2 m_sectorForwardNormal;
	float m_sectorApertureDegrees = 90.f;

	float m_thicknessLinkLine = 0.25f;

	std::vector<Vertex_PCU> m_testingShapesVerts;// contain all the shapes verts
	std::vector<Vertex_PCU> m_testingPointsVerts;// contain all the reference point and nearest points

	ReferencePoint* m_testingPoint;
	// rendering shapes
	void RenderAllShapes() const;

	// camera functions
	void UpdateCameras(float deltaSeconds);
	float m_screenShakeAmount = 0.f;
	bool  m_cameraShakeIsDisplayed = true;

	bool  m_gameIsOver					 = false;
	float m_returnToStartTimer			 = 0.f;
	bool  m_UIisDisplayed				 = true;
	int	  m_playerLivesNum				 = PLAYER_LIVES_NUM;
	float m_introTimer = 0.f;

	// Entity children special power
	Vec2 m_referencePoint;
private:
	bool CheckUIEnabled(UI* const UI) const;
	bool m_OpenScene = true;

	// Collision Detecting
	bool DoEntitiesOverlap(Entity const& a, Entity const& b);
};
