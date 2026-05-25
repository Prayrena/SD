#pragma once
#include "Engine/Animation/Actor.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Physics/ThePhysX.hpp"
#include <unordered_set>

class	VertexBuffer;
class	IndexBuffer;
class   Chain;
class   Joint;

namespace physx 
{
	class PxRigidDynamic;
	template <typename T> class PxTransformT;
}

enum class CrawlerAnimMode
{
	IDLE,
	ADJUSTING,
	WALKING,
	ROTATING,
	HIT,
	DEAD,
	NUM_MODE
};

struct ForceRequestByEvent
{
	physx::PxRigidDynamic* rigidActor = nullptr;
	physx::PxVec3 force;
	physx::PxVec3 forceApplyPosition;
	Vec3 hitActorVelocityDir;		
	physx::PxForceMode::Enum mode;
};

struct CrawlerLeg
{
	CrawlerLeg(Chain* c, Vec3 initializePos);
	Chain*	m_chain = nullptr;

	Vec3	m_footDefaultOffset = Vec3::ZERO;

	Vec3	m_animCycleStartPos = Vec3::ZERO;
	Quat	m_animCycleStartRootOrientation = Quat();
	Vec3	m_animStayPos = Vec3::ZERO;
	Quat	m_animStayRootOrientation = Quat();

	Quat	m_legRootDefaultLocalOrientation = Quat();
	Quat	m_legKneeDefaultLocalOrientation = Quat();
	Quat	m_legRootRecordedWorldOrientation = Quat();	// the body forward orientation that the leg remembered
	Quat	m_rotationCompensation = Quat();

	EulerAngles m_legRecordedForwordOrientation = EulerAngles();
	Vec3 m_footRotationExpectedPos = Vec3();

	bool m_haveRotated = false;

	physx::PxRigidDynamic* m_upperLegActor = nullptr;
	physx::PxRigidDynamic* m_lowerLegActor = nullptr;

	float	m_chainLength = 0.f;
	bool	m_isGrounded = false;
	bool	m_shouldMove = false;
	bool	m_shouldAdjust = false;
	bool	m_shouldRotate = false;

	bool	m_wasHit = false;

	CrawlerAnimMode m_legLastFrameAnimState = CrawlerAnimMode::IDLE;
	CrawlerAnimMode m_legCurrentAnimState = CrawlerAnimMode::IDLE;
};

class Crawler : public Actor
{
public:
	Crawler(Vec3 const& pos = Vec3(), EulerAngles const& angle = EulerAngles());
	// Crawler(Crawler* templateCrawler);
	~Crawler(); // Actor's constructor is called first, then this

	virtual void Startup() override;
	virtual void Update() override;
	virtual void Render() const override;

	Mat44 GetMovementTransformMatrix();

	void		AdjustInitialPose();
	EulerAngles m_initialAngle = EulerAngles();
	bool		m_poseAdjusted = false;

	void	UpdateInput();
	void	UpdateActorInfoByRigidBody();
	void	UpdateActorDamping();

	physx::PxTransform m_desiredRootD6JointTransform;

	float	m_speedToRecoverFromHit = 0.3f;
	float	m_maxSpeedAfterHit = 0.f;

	void	ControlActorByInput();
	void	UpdateTargetPositionByMouseRaycast();
	void	UpdateDebugRenderMessages();

	void	CreateCrawlerSkeleton();
	void	CreateLegDynamicActors(CrawlerLeg* leg);
	physx::PxRigidDynamic* CreateCapsuleShapeDynamicActorForTwoJoint(Joint* A, Joint* B, float radius, float mass);
	void	UpdateDynamicActorForTwoJoints(Joint* A, Joint* B, physx::PxRigidDynamic* actor);

	void	AlignLegD6JointsAndUpdateJointsRotation(CrawlerLeg* leg);
	void	AlignLegD6JointsWithSkeleton(CrawlerLeg* leg);
	void	AlignLegRootJointWithTarget(CrawlerLeg* leg);
	Quat	SetD6JointDriveAlignWithJointOrientation(Joint* skeletonJoint, Quat const& rotationAdjustment = Quat(), bool swing2RotationOnly = false);
	void	InterpolateJointWithItsD6Joint(Joint* j, float weight);

	float   m_upperLegCollisionRadius = 0.15f;
	float   m_lowerLegCollisionRadius = 0.15f;
	float   m_headCollisionRadius = 0.42f;

	float	m_upperLegMass = 0.2f;
	float	m_lowerLegMass = 0.1f;
	float	m_headMass = 0.3f;

	// control debug - use arrow keys to move the sphere
	std::vector<CrawlerLeg*> m_legs;


	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// health and billboard UI
	float m_health = 100.f;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// AI settings

	void  RollRandomDesiredPositionInFrontOfPlayer();
	float m_distToPlayerMax = 5.7f;
	float m_distToPlayerMin = 2.1f;
	float m_closestDistToPlayer = 3.0f;

	float m_directionChangingPeroidMax = 14.f;
	float m_directionChangingPeroidMin = 10.0f;
	float m_directionChangingPeroid = 6.f;
	Timer* m_directionChangingTimer = nullptr;	// after every period, the crawler change its moving direction

	Vec3  m_rememberedPlayerPos = Vec3::ZERO;
	EulerAngles m_rememberedPlayerDir = EulerAngles();

	float m_playerAngleRange = 75.f;			// the crawler will get to player's forward angle -60 to +60, will random choose it after the timer
	float m_playerAngleOffset = 0.f;

	float m_brakeTriggerSpeed = 3.f;
	float m_mechBrakeLinearDampingValue = 30.f; // 15.f;
	float m_mechHitLinearDampingValue = 21.f;

	float m_mechMaxRotationVelocity = 3.f;
	float m_mechRotatingAdjustingDampingValue = 60.f;
	float m_mechHitRotationDampingValue = 40.f;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// procedural animation
	void	UpdateAnimState();
	void	UpdateProceduralAnimationByCurrentState();
	void	UpdateToeGroundedStatus();
	bool	AreAllLegsGrounded();

	void UpdateIfLegShouldMove();
	bool CheckIfCrawlerNeedsToAdjustPose();

	bool CheckIfLegShouldRotate();

	Vec3 CalculateLegWalkStepByCrawlerVelocity(CrawlerLeg* leg, bool IsPredicting = false);
	int	 GetNumLegsShouldMove();

	Timer*	m_idleTimer = nullptr;
	float	m_idlePeroid = 999.f;
	float	m_idleHeightOffset = 0.05f;
	float	m_pelvisHeight = 0.6f;
	float	m_legBoneLength = 0.635f;
	float	m_footBoneLength = 0.35f;

	float	m_panicDurationForHit_max = 0.6f;
	float	m_panicDurationForHit_min = 0.15f;
	// Timer*	m_squatTimer = nullptr;
	Clock*	m_panicClock = nullptr;

	float	m_speedLastFrame = 0.f;

	float	m_walkingTimer_hit = 2.1f;
	float	m_walkingTimerPeroid_max = 0.8f;
	float	m_walkingTimerPeroid_min = 0.3f;
	float	m_walkingStep = 0.5f;

	bool	m_legsShouldStartToWalk = false;

	float	m_adjustingTimerPeroid = 0.2f;

	float m_movementSpeed = 0.15f;

	float m_legRotationAngleBoundary = 6.f;	// the max rotation a leg going to make
	bool  m_rotatingCounterClockwise = false;
	float m_rotatingPeroid = 0.8f;

	EulerAngles m_desiredOrientation = EulerAngles();

	CrawlerAnimMode m_crawlerCurrentFrameState = CrawlerAnimMode::IDLE;
	CrawlerAnimMode m_crawlerLastFrameState = CrawlerAnimMode::NUM_MODE;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// physX
	void CreateDynamicRigidBodyForCrawler();
	void ApplyPendingForces();

	void WasHitByPlayer(physx::PxRigidDynamic* rigidActor, physx::PxVec3 const& pos, Vec3 const& normal, float hittingSpeed);
	void WasHitByPlayer(physx::PxRigidDynamic* attackActor, physx::PxRigidDynamic* hitActor, physx::PxVec3 const& pos, Vec3 const& normal, float hittingSpeed);
	void MarkHitCrawlerLeg(physx::PxRigidDynamic* hitActor);
	void UnmarkHitCrawlerLeg();

	// float		m_squatDist = 0.f;
	std::mutex	m_attackSetMutex;
	std::unordered_set<physx::PxRigidDynamic*> m_attackRigidActorSet;

	Timer* m_setClearTimer = nullptr;
	float m_setClearDuration = 0.3f;

	float m_lastSpeed = 0.f;

	void PlayDeathPhysicalAnimation();
	void DisableCollisionWithPlayerMech();
	void ApplyLinearVelocityToAllRigidBodies();
	Vec3 m_lastBlowDir = Vec3::ZERO;
	float m_deflectionAngle = 30.f;
	float m_blowAcceration_min = 1.8f;
	float m_blowAcceration_max = 2.1f;
	bool  m_velocityApplied = false;
	Timer* m_deathClearTimer = nullptr;
	float m_deathClearDuration = 6.f;

	float m_minHitSpeedToDamage = 3.f;
	float m_maxHitSpeed = 7.5f;
	float m_minDamage = 10.f;
	float m_maxDamage = 20.f;

	bool  m_isDead = false;

	float m_capsuleHalfHeight = 0.27f;
	float m_capsuleRadius = 0.6f;

	float m_staticFriction = 0.f;
	float m_dynamicFriction = 0.f;
	float m_restitution = 0.f;

	float m_totalMass = 1.8f;	// kg

	bool m_isDriving = false;

	float m_forceAcceleration = 0.9f; // (m / s) / s
	float m_maxMovingSpeed = 1.5f;
	
	float m_maxPredictionDist = 0.2f;

	std::mutex	m_pendingForceMutex;
	std::vector<ForceRequestByEvent> m_pendingForces;
};

//struct NavMeshNode 
//{
//	Vec2 position;
//	std::vector<NavMeshNode*> neighbors; // indexes for adjacent NavMeshNode
//};
//
//struct Corridor 
//{
//	std::vector<Vec2> waypoints; // key position defining the corridor
//	std::vector<float> widths;   // the widths at each way point in the corridor
//};
//
//typedef std::vector<NavMeshNode*> Path;
//
//class Map
//{
//public:
//	std::vector<NavMeshNode> m_navMesh;
//
//	Path AStarPathfinding(NavMeshNode* start, NavMeshNode* goal)
//	{
//		if (!start || !goal || start == goal) return {};
//
//		std::priority_queue<std::pair<float, NavMeshNode*>,
//			std::vector<std::pair<float, NavMeshNode*>>,
//			std::greater<>> openSet;
//
//		std::unordered_map<NavMeshNode*, NavMeshNode*> cameFrom;
//		std::unordered_map<NavMeshNode*, float> gScore, fScore;
//
//		gScore[start] = 0;
//		fScore[start] = GetDistance2D(start->position, goal->position);
//		openSet.push({ fScore[start], start });
//
//		while (!openSet.empty())
//		{
//			NavMeshNode* current = openSet.top().second;
//			openSet.pop();
//
//			if (current == goal)
//			{
//				std::vector<NavMeshNode*> path;
//				while (cameFrom.count(current))
//				{
//					path.push_back(current);
//					current = cameFrom[current];
//				}
//				path.push_back(start);
//				std::reverse(path.begin(), path.end());
//				return path;
//			}
//
//			for (NavMeshNode* neighbor : current->neighbors)
//			{
//				float tentative_gScore = gScore[current] + GetDistance2D(current->position, neighbor->position);
//				if (!gScore.count(neighbor) || tentative_gScore < gScore[neighbor])
//				{
//					cameFrom[neighbor] = current;
//					gScore[neighbor] = tentative_gScore;
//					fScore[neighbor] = tentative_gScore + GetDistance2D(neighbor->position, goal->position);
//					openSet.push({ fScore[neighbor], neighbor });
//				}
//			}
//		}
//		return {}; // No path found
//	}
//
//	// create corridor from the A* path
//	Corridor* CreateCorridorMapByPath(Path const& path)
//	{
//		Corridor* corridor = new Corridor();
//		for (size_t i = 0; i < path.size(); ++i)
//		{
//			corridor->waypoints.push_back(path[i]->position);
//
//			// Get the width for each waypoint in the corridor
//			float minWidth = FLT_MAX;
//			for (NavMeshNode* neighbor : path[i]->neighbors)
//			{
//				float distance = GetDistance2D(path[i]->position, neighbor->position);
//				minWidth = std::min(minWidth, distance);
//			}
//			corridor->widths.push_back(minWidth * 0.8f); // Slightly narrow the width to prevent unit collision
//		}
//		return corridor;
//	}
//};
//
//struct Unit
//{
//	Vec2	m_position = Vec2();
//	float	m_movingSpeed = 0.f;
//
//	Squad*	m_squad = nullptr;
//
//	void SteerTowardCorridor(Corridor* corridor, int currentIndex) 
//	{
//		if (currentIndex >= corridor->waypoints.size()) return; // do nothing when already at the corridor way point
//
//		Vec2 target = corridor->waypoints[currentIndex];
//		Vec2 dir = (target - m_squad->m_squadCenter).GetNormalized();
//
//		// determine corridor width at this way point
//		float corridorWidth = corridor->widths[currentIndex];
//
//		// squad positioning within the corridor
//		if (m_squad && !m_squad->m_units.empty())
//		{
//			int squadSize = (int)(m_squad->m_units.size());
//			int unitIndex = m_squad->GetUnitIndex(this);
//
//			// calculate unit pos offset within the corridor
//			// say the squad is evenly distributed within the corridor
//			float unitSpacing = corridorWidth / std::max(1, squadSize - 1); 
//			float offset = (float)(unitIndex - (squadSize - 1)) * 0.5f * unitSpacing; // to the center of the formation
//
//			// get perpendicular vector to corridor direction
//			Vec2 perpDir = dir;
//			perpDir.Rotate90Degrees();
//			Vec2 formationPosition = target + perpDir * offset; // Adjust formation based on spacing
//
//			// Move toward adjusted formation position
//			m_position += (formationPosition - m_position).GetNormalized() * m_movingSpeed;
//		}
//
//		// get to the center of corridor way point if last man standing
//		m_position += dir * m_movingSpeed;
//	}
//};
//
//class Squad
//{
//public:
//	std::vector<Unit*> m_units;
//	Vec2 m_squadCenter; // average position of all units
//	Map* m_map = nullptr;
//
//	int GetUnitIndex(Unit* unit)
//	{
//		if (unit)
//		{
//			for (int i = 0; i < (int)m_units.size(); ++i)
//			{
//				if (m_units[i] == unit)
//				{
//					return i;
//				}
//			}
//		}
//		else return -1;	// return -1 if the unit point is invalid
//	}
//
//	int GetCorridorIndexForGoal(Corridor* corridor, NavMeshNode* goal)
//	{
//		// Find the corresponding index for the goal in the corridor waypoints
//		for (int i = 0; i < corridor->waypoints.size(); ++i)
//		{
//			if (corridor->waypoints[i] == goal->position) 
//			{
//				return i; 
//			}
//		}
//	}
//
//	// Move squad to a position
//	void MoveSquadToPosition(NavMeshNode* start, NavMeshNode* goal)
//	{
//		// find the path using A* algorithm
//		Path path = m_map->AStarPathfinding(start, goal);
//		if (path.empty()) return; // No path found
//
//		// create a corridor from the path result
//		Corridor* corridor = m_map->CreateCorridorMapByPath(path);
//
//		// Update squad center based on average position of all units
//		m_squadCenter = Vec2();
//		for (Unit* unit : m_units)
//		{
//			m_squadCenter += unit->m_position;
//		}
//		m_squadCenter /= (float)m_units.size();
//
//		// Get the index in the corridor that corresponds to the goal's position
//		int goalCorridorIndex = GetCorridorIndexForGoal(corridor, goal);
//
//		// Move each unit toward the corresponding waypoint in the corridor
//		for (int i = 0; i < m_units.size(); ++i)
//		{
//			// Each unit should move toward the waypoint in the corridor
//			// adjust for their index in the squad
//			m_units[i]->SteerTowardCorridor(corridor, goalCorridorIndex);
//		}
//	}
//};