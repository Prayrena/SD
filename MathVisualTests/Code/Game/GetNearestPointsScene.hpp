#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/GameMode.hpp"
#include <vector>

class App;
class ReferenceInput;
class Entity;
class UI;
class RandomNumberGenerator;
class Boid;
class EnergyBar;
class EnergySelectionRing;
struct OBB2;
struct LineSegment2;
struct Capsule2;

class GetNearestPointsScene : GameMode
{
public:
	GetNearestPointsScene();
	~GetNearestPointsScene();
	void Startup();
	void Update(float deltaSeconds);
	void Render() const;//mark for that the render is not going to change the variables
	void Shutdown();
	void ShutDownUIElementList(int arraySize, UI** m_entityArrayPointer);

	// UI functions
	void InitializeEnergyBar();
	void RenderUIElements() const;
	void RenderUIElementList(int arraySize, UI* const* m_UIElementArrayPointer) const;
	void UpdateUIElementList(int arraySize, UI* const* m_UIElementArrayPointer, float deltaSeconds) const;
	void UpdateUI(float deltaSeconds);
	EnergyBar* m_energyBar = nullptr;
	EnergySelectionRing* m_energySelectionRing = nullptr;
	bool m_speedBoost		= true;
	bool m_fireRateBoost	= false;
	bool m_shieldGenerate	= false;

	// placing random shape in the world
	Vec2 GetRandomPosInWorld(Vec2 worldSize);

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

	OBB2* m_OBB2;
	LineSegment2* m_lineSegment;
	LineSegment2* m_infiniteLine;
	Capsule2* m_capsule;
	AABB2* m_AABB2;
	Vec2 m_diskCenter;
	float m_diskRadius;

	Vec2 m_sectorTip;
	float m_sectorRadius;
	Vec2 m_sectorForwardNormal;
	float m_sectorApertureDegrees;

	float m_thicknessLinkLine = 0.25f;

	std::vector<Vertex_PCU> m_testingShapesVerts;// contain all the shapes verts
	std::vector<Vertex_PCU> m_testingPointsVerts;// contain all the reference point and nearest points

	// rendering shapes
	void RenderAllShapes() const;

	// camera functions
	void UpdateCameras(float deltaSeconds);
	float m_screenShakeAmount = 0.f;
	bool  m_cameraShakeIsDisplayed = true;
	Camera m_worldCamera;
	Camera m_screenCamera;

	bool  m_gameIsOver					 = false;
	float m_returnToStartTimer			 = 0.f;
	bool  m_UIisDisplayed				 = true;
	int	  m_playerLivesNum				 = PLAYER_LIVES_NUM;
	float m_introTimer = 0.f;

	// Entity children special power
	ReferenceInput* m_testingPoint = nullptr;// in order to let game's owned class use player ship's info

private:
	bool CheckUIEnabled(UI* const UI) const;
	bool m_OpenScene = true;

	// Collision Detecting
	bool DoEntitiesOverlap(Entity const& a, Entity const& b);
};
