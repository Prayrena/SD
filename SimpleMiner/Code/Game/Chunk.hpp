#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/core/HeatMaps.hpp"
#include "Engine/core/XmlUtils.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Game/Block.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/core/JobSystem.hpp"
#include <vector>
#include <string>

class VertexBuffer;
class Shader;
class BlockIter;

enum class ChunkState
{
	CONSTRUCTING,					// [set by main thread] Initial Chunk::m_state value during early construction

	ACTIVATING_QUEUED_LOAD,			// [set by main thread] Chunk has been added to the loading queue
	ACTIVATING_LOADING,				// [set by disk thread] Chunk is being loaded & populated by disk i/o thread
	ACTIVATING_LOAD_COMPLETE,		// [set by disk thread] Chunk is done loading and ready for main thread to claim

	ACTIVATING_QUEUED_GENERATE,		// [set by main thread] Chunk has been added to the generating queue
	ACTIVATING_GENERATING,			// [set by generator thread] Chunk is being generated & populated by a generator thread
	ACTIVATING_GENERATE_COMPLETE,	// [set by generator thread] Chunk is done generating and ready for the main thread to claim

	ACTIVE,							// [set by main thread] Chunk is in m_activateChunks; only main thread can touch it. Lighting, mesh building allowed

	DEACTIVATING_QUEUED_SAVE,		// [set by main thread] Chunk is being deactivated, and is being queued for save
	DEACTIVATING_SAVING,			// [set by disk thread] Chunk is being compressed and saved by disk i/o thread
	DEACTIVATING_SAVE_COMPLETE,		// [set by disk thread] Chunk has been saved and is ready for main thread to claim
	DECONSTRUCTING,					// [set by main thread] Chunk is being destroyed by main thread

	NUM_CHUNK_STATES
};

class Chunk
{
public:
	Chunk(IntVec2 originCoords);
	~Chunk();

	void Startup();
	void Update();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Block management
	void	CreateInitialBlocks();
	void	SetBlockTypeForLocalBlock(std::string blcokTypeName, IntVec3 const& localCoords);
	int		GetIndexForLocalCoordinates(IntVec3 const& localCoords);
	IntVec3 GetlocalBlockCoordsForIndex(int localBlockIndex);
	IntVec3 GetlocalBlockCoordsForWorldPos(Vec3 const& worldPos);

	AABB3	GetBlockBoundsInLocalSpace(IntVec3 const& localCoords);
	AABB3	GetBlockBoundsInWorldSpace(IntVec3 const& localCoords);
	AABB3	GetBlockBoundsInWorldSpace(int blockIndex);
	int		GetFirstSolidBlockUnderInputCoords(IntVec3 const& standingLocalCoords);

	bool IfTheBlockIsAtTheBottomOfTheChunk(IntVec3 const& blockCoords);
	bool IfTheBlockIsAirOrWaterOrOutOfChunk(IntVec3 const& blockCoords);
	bool IfTheBlockIsAirOrOutOfChunk(IntVec3 const& blockCoords);
	bool CheckIfTheBlockCoordsIsInsideChunkBound(IntVec3 const& blockCoords);

	// Block* m_blocks = nullptr; // pointer to the first block array
	// m_blocks = new Block[CHUNK_BLOCKS_TOTAL]; // call new block 32768 times
	// Block* m_blocks[32768]; // this is worst because the block is stored scattered everywhere in memory
	Block  m_blocks[CHUNK_BLOCKS_TOTAL]; //pack all the block data together

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// player game play action
	bool DigBlock(int blockIndex);
	void SetBlock(int blockIndex, BlockDef const& def);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Rendering
	void Render() const;
	void RenderBlocks() const;
	Mat44 GetModelMatrix() const;

	void AddVertsForAllBlocksInChunk();
	void DrawDebugRender() const;
	void AddVertsForBlock(int localBlockIndex);
	void CreateVertexBuffer();
	void CopyVertexBufferFromCPUtoGPU();


	bool	m_needsSaving = false;
	bool	m_isMeshDirty = true; // meaning the meshes data needed to be updated

	std::vector<Vertex_PCU> m_blockVerts;
	VertexBuffer* m_vertexBuffer;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// saving and loading
	void SaveBlocksDataToFile();
	bool CheckIfThereIsSaveFile();
	void LoadBlocksDataFromFile();
	bool CheckIfSaveFileMatchWorldSeedNumber(std::string fileName);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// chunk physics info
	Vec3	GetChunkWorldOrigin() const;

	IntVec2				 m_chunkCoords = IntVec2(0, 0);
	EulerAngles			 m_chunkOrientation;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// biome factors
	void GenerateBiomeFactors();
	void SpawnTreeBlockTemplatesAccordingToBiomeFactors();
	void SpawnTreeBlockTemplatesAtColumCoords(BlockTemplate treeTemplate, IntVec2 localCoords);
	bool DoAllEightSurroundingNeighborChunksExist();
	bool DoAllFourSurroundingNeighborChunksExist();

	IntVec2 GetBiomeCoordsByLocalCoords(IntVec2 localCoords);
	IntVec3 GetLocalCoordsByTreeCoords(IntVec3 treeCoords);
	int GetBiomeIndexByTreeCoords(IntVec2 neighboorhoodCoords);
	bool AreTheTreeNeighborCoordsInChunk(IntVec3 neighborhoodCoords);
	bool AreTheLocalCoordsInChunk(IntVec3 localdCoords);

	// store more than CHUNK_BLOCKS_PER_LAYER
	int	  m_terrainHeightZ[NEIGHBOOR_ARRAY_SIZE];
	float m_humidity[NEIGHBOOR_ARRAY_SIZE];
	float m_temperature[NEIGHBOOR_ARRAY_SIZE];

	bool m_needToPlantTrees = true;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// chunk neighbor info
	Chunk* m_eastNeighbor = nullptr;  // null if not Active
	Chunk* m_westNeighbor = nullptr;  // null if not Active
	Chunk* m_northNeighbor = nullptr;  // null if not Active
	Chunk* m_southNeighbor = nullptr;  // null if not Active

	std::atomic<ChunkState> m_chunkState = ChunkState::NUM_CHUNK_STATES;
};

struct ChunkGenerateJob : public Job
{
public:
	ChunkGenerateJob(Chunk* chunkPtr) 
		: m_chunk(chunkPtr)
	{}

	virtual void Execute() override;

	Chunk* m_chunk = nullptr;
};