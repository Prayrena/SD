#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameMode.hpp"
#include <vector>

class App;
class Entity;
class UI;
class RandomNumberGenerator;
struct LineSegment2;

// record every disc center and radius information into an array
struct Disc
{
	Vec2 discCenter = Vec2::ZERO;
	float discRadius = 0.f;
	RaycastResult2D result;
};

class RaycastVSDiscsMode : public GameMode
{
public:
	RaycastVSDiscsMode();
	~RaycastVSDiscsMode();
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
	virtual void CreateRandomShapes() override;
	void AddvertsForDiscs();
	void AddvertsForRay();

	// reference ray
	void UpdateRaycastResult_andHitDisc();

	virtual void UpdateModeInfo() override;

	bool m_hasHitDisc = false;
	float m_shortestImpactDist = 0.f;
	float m_arrowSize = WORLD_SIZE_Y * 0.02f;
	float m_arrowLineThickness = 0.15f;

	int m_numDiscSides = 36;
	Vec2 m_diskCenter;
	float m_diskRadius;

	Vec2 m_sectorTip;
	float m_sectorRadius;
	Vec2 m_sectorForwardNormal;
	float m_sectorApertureDegrees;

	float m_thicknessLinkLine = 0.25f;

	Disc* m_hitDisc = nullptr;
	std::vector<Disc*> m_discsPtrList; // store every testing discs' information
	std::vector<Vertex_PCU> m_discsVerts;// contain all the shapes verts

	// DiscRaycastObject m_highlightDisk
	// rendering shapes
	void RenderAllDiscs() const;
	void RenderRaycastResults() const;

	// camera functions
	void UpdateCameras(float deltaSeconds);
	float m_screenShakeAmount = 0.f;
	bool  m_cameraShakeIsDisplayed = true;

	bool  m_gameIsOver					 = false;
	float m_returnToStartTimer			 = 0.f;
	bool  m_UIisDisplayed				 = true;
	int	  m_playerLivesNum				 = PLAYER_LIVES_NUM;
	float m_introTimer = 0.f;

private:
	bool CheckUIEnabled(UI* const UI) const;
	bool m_OpenScene = true;

	// Collision Detecting
	bool DoEntitiesOverlap(Entity const& a, Entity const& b);
};
