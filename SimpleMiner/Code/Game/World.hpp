#pragma once
#include "Game/Chunk.hpp"
#include "Game/BlockIterator.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Math/Capsule3.hpp"
#include <deque>
#include <vector>
#include <set>

class Timer;

bool operator<(IntVec2 const& a, IntVec2 const& b); // stand alone function

struct SimpleMinerRaycastResult : public RaycastResult3D
{
	BlockIter m_aimedBlockIter;
};

struct SimpleMinerGPUData
{
	Vec4 m_indoorColor; //= Rgba8(255, 230, 204, 255);
	Vec4 m_outdoorColor; // = Rgba8::WHITE;
	Vec4  m_cameraPos;
	Vec4 m_fogColor;  //= Rgba8::WHITE;
	float m_fogStartDist = (float)(CHUNK_ACTIVATE_RANGE - 16) * 0.5f;
	float m_fogEndDist = (float)(CHUNK_ACTIVATE_RANGE - 16);
	float m_fogMaxAlpha = 0.9f;
	float m_dummyPadding = 0.f; // CBO struct and members must be 16B-aligned and 16B-sized!!
	// REMEMBER: CBO struct must have size multiple of 16B
	// and <16B members cannot straddle 16B boundaries!!
};

class World
{
public:
	World();
	~World();

	void Startup();
	void Update();
	void Render() const;
	void RenderDebug() const;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// player info
	void UpdatePlayerLocatedCoords();
	void UpdatePlayerAimingBlock();

	void RenderPlayerAimingSurface() const;
	IntVec2 GetChunkCoordsForWorldPos(Vec3 worldPos);

	SimpleMinerRaycastResult m_playerAimingRaycastResult;
	IntVec2 m_playerChunkCoords = IntVec2();
	IntVec3 m_playerLocalBlockCoords = IntVec3();
	Chunk* m_playerLocatedChunk = nullptr;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// block management
	void CreateInitialChunks();
	void StartupAllBlocks();

	// dynamic loading
	void UpdateAllActiveChunks();

	void DeactivateChunks();
	void DeactivateFarthestActiveChunk();
	void DeactivateFarthestActiveChunkOutOfPlayerRange();
	void DeactivateChunk(IntVec2 chunkCoords);
	void DeactivateChunk(Chunk* chunkPtr);

	void ActivateChunks();
	void ActivateNearestMissingChunkInPlayerRange();

	void RequestNewChunkGenerationJob(IntVec2 chunkCoords);
	void ActivateNewChunk(Chunk* chunkPtr);
	void RetrieveCompletedChunkGenerationJobAndActivate();

	void WorldInputControl();
	void UpdateOnScreenDisplayMessages();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// lighting process
	void ProcessDirtyLighting();
	void DirtyNeighborChunkMeshWhenAnEdgeAirBlockLightIsProcessed(BlockIter& blockIter);
	void MarkLightingDirty(BlockIter blockIter);
	void ProcessNextDirtyLightBlock(BlockIter blockIter);
	void MarkNeighborLightingDirtyIfTheyAreNotOpaque(BlockIter blockIter);

	void LightInfluenceInitialization(Chunk* chunkPtr);
	void UndirtyAllBlocksInChunk(Chunk* chunkPtr);

	void UpdateSimplerMinerWorldShader();
	void BindSimplerMinerWorldShaderData() const;

	std::deque<BlockIter> m_dirtyLightBlockIters; // double-ended queue, called 'deck'

	SimpleMinerGPUData* m_gpuShaderData = nullptr;
	ConstantBuffer* m_lighting_Fog_CBO = nullptr;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	void CheckIfCaveWormStartsThisChunk(Chunk* chunkPtr);
	void GeneratingPerlinWormCaves(Chunk* chunkPtr);

	std::vector<Capsule3> caveLists;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// player game play in world
	void DigBlock();
	void BuildPlayerBlock();

	void ShootRaycastForCollisionTest(float rayDist);
	SimpleMinerRaycastResult FastRaycastForVoxelGrids(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist);

	void RenderLockedRaycastForDebug() const;

	RaycastResult3D m_lockedRaycastInfo;
	bool m_raycastIsLocked = false;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// time, dynamic day and night changing, lighting
	void UpdateTime();
	
	float m_worldStartTime = 12.f * HOUR_FRACTION_DAY;
	float m_worldDay = 0.f;
	float m_normalWorldTimeScale = 200.f;
	float m_fastWorldTimeScale = 10000.f;

	Rgba8 m_nightSKyFogColor = Rgba8(20, 20, 40); // between 6pm and 6am
	Rgba8 m_noonSkyFogColor = Rgba8(200, 230, 255); // light blue (200,230,255) at high noon
	Rgba8 m_indoorLight = Rgba8(255, 230, 204, 255);

	float m_lightningStrength = 0.f;
	float m_glowStrength = 0.f;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// biomes

	int m_seed = 0;
	bool m_enableFog = true;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// debug
	float m_recordPeroid = 0.5f;
	Timer* m_dataTimer = nullptr;

	std::string m_FPS;
	std::string m_frameMS;

	// dynamic loading chunks
	std::map<IntVec2, Chunk*> m_activeChunks;
	std::set<IntVec2> m_chunksBeingGeneratedOrLoaded;

		Texture* m_blockTexture = nullptr;
};