#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/input/XRInputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Animation/Skeleton.hpp"
#include "Engine/Animation/ProceduralAnimation/IKTwoBonesSolver.hpp"
#include "Engine/Physics/ThePhysX.hpp"
#include "ThirdParty/Physx/include/PxSimulationEventCallback.h"  // for PxSimulationEventCallback
#include "Game/PlayerHand.hpp"
#include "Game/GameCommon.hpp"
#include <vector>
#include <set>

struct XrPosef;
class Timer;
class HUD;
class Crawler;
class ConstantBuffer;
struct PhongLightingConstants;

struct FogGPUData
{
	Vec4  m_cameraPos;
	Vec4  m_fogColor;  //= Rgba8::WHITE;
	float m_fogStartDist = 12.f;
	float m_fogEndDist = 30.f;
	float m_fogMaxAlpha = 0.9f;
	float m_dummyPadding = 0.f; // CBO struct and members must be 16B-aligned and 16B-sized!!
};

enum class SoundEffectID
{
	GAMEMUSIC,
	SCORESMUSIC,

	NUM_SOUNDEFFECTS
};

enum class GameState
{
	NONE,
	ATTRACT,
	LOBBY,
	PLAYING,
	SHOWSCORES,
	VICTORY,
	FAILURE,
	COUNT
};

class Game : public physx::PxSimulationEventCallback 
{
#pragma region NoneOverriddenPart
public:
	Game();
	~Game();

	void Startup();
	void Update();
	void Render() const;
	void Shutdown();

	void SetModelMatrix() const;

	void AddVertsForWorldAxis();
	void AddVertsForGround();

	std::vector<Vertex_PCU> m_axisVerts;
	VertexBuffer* m_axisVertexBuffer = nullptr; 
	
	std::vector<Vertex_PCU> m_groundVerts;
	VertexBuffer* m_groundVertexBuffer = nullptr;

	std::vector<Vertex_PCU> m_pillarVerts;
	std::vector<Vertex_PCU> m_scoreVerts;

	// Assets
	void LoadAudioAssets();
	SoundID m_soundEffectsID[int(SoundEffectID::NUM_SOUNDEFFECTS)];

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	#pragma region Game state machine
	void EnterState(GameState state);
	void ExitState(GameState state);

	void EnterAttract();
	void EnterLobby();
	void EnterPlaying();
	void EnterVictory();
	void EnterShowScores();

	void ExitAttract();
	void ExitLobby();
	void ExitPlaying();
	void ExitVictory();
	void ExitShowScores();

	void UpdateAttract();
	void UpdateLobby();
	void UpdatePlaying();
	void UpdateShowScores();
	void UpdateVictory();

	void RenderAttract() const;
	void RenderLobby() const;
	void RenderPlaying() const;
	void RenderShowScores() const;
	void RenderVictory() const;

	GameState m_currentState = GameState::ATTRACT;
	#pragma endregion

	int GetIndexForCoords(IntVec2 coords);

	Actor*					m_sun = nullptr;
	PhongLightingConstants* m_phongLighinting = nullptr;

	void UpdateFogShaderDataWithNewCameraPos(Vec3 const& pos);
	void BindFogShaderData() const;
	FogGPUData* m_gpuShaderData = nullptr;
	ConstantBuffer* m_fog_CBO = nullptr;

	Rgba8 m_noonSkyFogColor = Rgba8::BLUE_LIGHT; // light blue (200,230,255) at high noon

	bool m_debugSkeleton = true;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	SoundPlaybackID m_gameMusic;
	SoundPlaybackID m_scoresMusic;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// instruction and count down start UI is drawn in the HUD class
	// scores will be drawn and render in the scene for not interfere with game play

	int m_currentScores = 0;
	int m_activatedCubesCounter = 0;
	
	Timer* m_grabToStartTimer = nullptr;

	Timer* m_recenterTimer = nullptr;


	void	ResetGameWorldByHeadPose(Mat44 const& newMat);
	Mat44   m_toNewResetGameWold;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// crawlers
	ObjModel* m_footModel = nullptr;
	ObjModel* m_legModel = nullptr;
	ObjModel* m_baseModel = nullptr;
	ObjModel* m_headModel = nullptr;
	ObjModel* m_gunModel = nullptr;
	void LoadingCrawlerModelParts();

	void UpdateAllCrawlersHealthBarTrackingMat(Mat44 const& trackingMat);

	// Crawler* m_crawlerTemplate = nullptr;
	std::vector<Crawler*> m_crawlers;

	int m_numCrawlers = 1;

#pragma endregion
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Called when two actors collide, override physx function
	void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
	std::set<std::pair<const void*, const void*>> m_activeContacts;

	// Implement all PhysX simulation event callbacks because it reuqires to do so, otherwise it will have error for g_theGame = new Game();
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 nbPairs) override;
	void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 nbConstraints) override;
	void onWake(physx::PxActor** actors, physx::PxU32 count) override;
	void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
	void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;

	void ApplyForceOnActorCollision(physx::PxRigidDynamic* actorA, physx::PxRigidDynamic* actorB, const physx::PxContactPair& contactPair);
};