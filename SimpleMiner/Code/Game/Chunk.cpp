#include "Engine/Math/AABB2.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/RaycastUtils.Hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/core/Image.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/core/FileUtils.hpp"
#include "Engine/Math/Easing.hpp"
#include "ThirdParty/Noise_Squirrel/SmoothNoise.hpp"
#include "ThirdParty/Noise_Squirrel/RawNoise.hpp"
#include "Game/Game.hpp"
#include "Game/World.hpp"
#include "Game/App.hpp"
#include "Game/Player.hpp"
#include "Game/Block.hpp"
#include "Game/BlockIterator.hpp"
#include "Game/Chunk.hpp"
#include <thread>

Rgba8 const white = Rgba8(255, 255, 255);
Rgba8 const lightGray = Rgba8(230, 230, 230);
Rgba8 const DarkGray = Rgba8(200, 200, 200);

const IntVec2 STEP_EAST = IntVec2(1, 0);
const IntVec2 STEP_SOUTH = IntVec2(0, -1);
const IntVec2 STEP_WEST = IntVec2(-1, 0);
const IntVec2 STEP_NORTH = IntVec2(0, 1);

const IntVec2 STEP_EASTSOUTH = IntVec2(1, -1);
const IntVec2 STEP_SOUTHWEST = IntVec2(-1, -1);
const IntVec2 STEP_WESTNORTH = IntVec2(-1, 1);
const IntVec2 STEP_NORTHEAST = IntVec2(1, 1);

extern RandomNumberGenerator* g_rng;
extern App* g_theApp;
extern Game* g_theGame;
extern World* g_theWorld;
extern JobSystem* g_theJobSystem;

Chunk::Chunk(IntVec2 originCoords)
	:m_chunkCoords(originCoords)
{
}

Chunk::~Chunk()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;
}

void ChunkGenerateJob::Execute()
{
	m_chunk->m_chunkState = ChunkState::ACTIVATING_GENERATING;
	m_chunk->GenerateBiomeFactors();
	m_chunk->CreateInitialBlocks();
	m_chunk->SpawnTreeBlockTemplatesAccordingToBiomeFactors();
	m_chunk->m_chunkState = ChunkState::ACTIVATING_GENERATE_COMPLETE;
}

void Chunk::Startup()
{
	if (CheckIfThereIsSaveFile())
	{
		LoadBlocksDataFromFile();
		g_theWorld->ActivateNewChunk(this);
	}
	else
	{
		// queue up a synchronized threaded job in the background
		ChunkGenerateJob* job = new ChunkGenerateJob(this);
		m_chunkState = ChunkState::CONSTRUCTING;

		g_theJobSystem->QueueJobs(job);
		m_chunkState = ChunkState::ACTIVATING_QUEUED_GENERATE;
		g_theWorld->m_chunksBeingGeneratedOrLoaded.insert(m_chunkCoords);
	}
}
 
void Chunk::Update()
{
	if (m_isMeshDirty && DoAllFourSurroundingNeighborChunksExist())
	{
		// if the chunk need saving, meaning we have not update its save file yet
		// if (!m_needsSaving)
		// {
		// 	if (CheckIfThereIsSaveFile())
		// 	{
		// 		LoadBlocksDataFromFile();
		// 	}
		// 	else
		// 	{
		// 		CreateInitialBlocks();
		// 		SpawnTreeBlockTemplatesAccordingToBiomeFactors();
		// 	}
		// }

		// g_theWorld->LightInfluenceInitialization(this);

		AddVertsForAllBlocksInChunk();
		CreateVertexBuffer();
		CopyVertexBufferFromCPUtoGPU();

		m_isMeshDirty = false;
	}
}

void Chunk::GenerateBiomeFactors()
{
	// terrain height for each column of blocks

	// the terrain height of each block column is based on the global block coordinates of that block's column with a slight random variation
	for (int localX = (0 - TREE_COVER_SIZE_MAX); localX < (CHUNK_SIZE_X + TREE_COVER_SIZE_MAX); ++localX)
	{
		for (int localY = (0 - TREE_COVER_SIZE_MAX); localY < (CHUNK_SIZE_Y + TREE_COVER_SIZE_MAX); ++localY)
		{
			int globalX = localX + m_chunkCoords.x * CHUNK_SIZE_X;
			int globalY = localY + m_chunkCoords.y * CHUNK_SIZE_Y;

			// create a new neighborhood coords system
			int neighborhoodX = localX + TREE_COVER_SIZE_MAX;
			int neighborhoodY = localY + TREE_COVER_SIZE_MAX;
			int neighborhoodColumnIndex = neighborhoodY * NEIGHBORHOOD_SIZE_X + neighborhoodX;
			
			// assignment 2 setting
			// terrainHeightZ[localY * CHUNK_SIZE_X + localX] = 64 +int(30.f * Compute2dPerlinNoise(float(globalX), float(globalY), 200.f, 5, 0.5f, 2.f, true, g_theWorld->m_seed));

			// assignment 3 setting
			// Generate Biome Factors
			float Temperature = 0.5f + 0.5f * Compute2dPerlinNoise(float(globalX), float(globalY), 300.f, 5, 0.5f, 2.f, true, g_theWorld->m_seed); // range from 0 - 1, so we could use all easing functions
			float Humidity = 0.5f + 0.5f * Compute2dPerlinNoise(float(globalX), float(globalY), 600, 2, 0.5f, 2.f, true, g_theWorld->m_seed);
			float Hilliness = 0.5f + 0.5f * Compute2dPerlinNoise(float(globalX), float(globalY), 2000, 3, 0.5f, 2.f, true, g_theWorld->m_seed); // range from 0 - 1, so we could use all easing functions
			float Oceanness = 0.5f + 0.5f * Compute2dPerlinNoise(float(globalX), float(globalY), 1000, 7, 0.5f, 2.f, true, g_theWorld->m_seed);
			
			// Humidity = 1.f - (1.f - Humidity) * (1.f - Oceanness); // both humidity and oceanness will contribute but result will not over 1
			// todo: but this will cause the humidity hardly ever under 0.4f

			// mountain height settings
			int groundHeightZ = 0;
			float terrainNoise = Compute2dPerlinNoise(float(globalX), float(globalY), 100, 5);
			float terrainNoiseFromZeroToOne = fabsf(terrainNoise);
			Hilliness = SmoothStep3(SmoothStep3(Hilliness));

			float terrainHeightAboveRiverBed = terrainNoiseFromZeroToOne * TERRAIN_HEIGHT_ABOVE_WATER_MAX; // heightest mountain
			float terrainHeightAboveWater = terrainHeightAboveRiverBed - RIVER_DEPTH_MAX;

			float terrainHeightAboveWaterBed = terrainHeightAboveWater;
			if (terrainHeightAboveWater > 0.f)
			{
				terrainHeightAboveWater *= Hilliness;
				terrainHeightAboveWaterBed = terrainHeightAboveWater + RIVER_DEPTH_MAX;
			}
			groundHeightZ = (int)((float)RIVER_BED_HEIGHT_Z + terrainHeightAboveWaterBed);

			// ocean
			// Map Oceanness into [0,1] and run a SmoothStep3() easing function on it (perhaps even twice!) 
			// to get more distinctively “ocean” and “non-ocean (land)” biome areas
			Oceanness = SmoothStep3(SmoothStep3(Oceanness));
			// Oceanness < 0.50 = normal terrain
			if (Oceanness >= 0.5f && Oceanness <= 0.75f)
			{
				Oceanness = RangeMap(Oceanness, 0.5f, 0.75f, 0.f, 1.f);
				int OceanLowing = (int)((float)OCEAN_LOWING_MAX * Oceanness);
				groundHeightZ -= OceanLowing;
			}
			else if (Oceanness > 0.75f)
			{
				int OceanLowing = OCEAN_LOWING_MAX;
				groundHeightZ -= OceanLowing;
			}

			// store the biome info
			m_terrainHeightZ[neighborhoodColumnIndex] = groundHeightZ;
			m_temperature[neighborhoodColumnIndex] = Temperature;
			m_humidity[neighborhoodColumnIndex] = Humidity;
		}
	}
}


void Chunk::SpawnTreeBlockTemplatesAccordingToBiomeFactors()
{
	// after all blocks are defined
	// rewrite with tree blocks
	// tree density

	for (int localX = (0 - TREE_COVER_SIZE_MAX); localX < (CHUNK_SIZE_X + TREE_COVER_SIZE_MAX); ++localX)
	{
		for (int localY = (0 - TREE_COVER_SIZE_MAX); localY < (CHUNK_SIZE_Y + TREE_COVER_SIZE_MAX); ++localY)
		{
			int globalX = localX + m_chunkCoords.x * CHUNK_SIZE_X;
			int globalY = localY + m_chunkCoords.y * CHUNK_SIZE_Y;

			IntVec2 localCoords = IntVec2(localX, localY);
			IntVec2 treeCoords = GetBiomeCoordsByLocalCoords(IntVec2(localX, localY));
			int treeColumnIndex = GetBiomeIndexByTreeCoords(treeCoords);

			float Forestness = 0.5f + 0.5f * Compute2dPerlinNoise(float(globalX), float(globalY), 90.f, 3, 0.5f, 2.f, true, g_theWorld->m_seed); // rename of "tree density"
			Forestness = SmoothStep3(SmoothStep3(Forestness));
			float treeNoiseMinimumThreshold = RangeMapClamped(Forestness, 0.6f, 1.f, 0.99f, 0.75f);
			float TreeNoise = Get2dNoiseZeroToOne(globalX, globalY, g_theWorld->m_seed);

			// this randomize decides whether the tree should be planted
			if (TreeNoise > treeNoiseMinimumThreshold)
			{
				// calculate all neighbors tree including this point noise 3x3 to see if this one is the highest one is this one
				float max = 0.f;
				for (int posX = (globalX - (TREE_COVER_SIZE_MAX)); posX < (globalX + 2); ++posX)
				{
					for (int posY = (globalY - (TREE_COVER_SIZE_MAX)); posY < (globalY + 2); ++posY)
					{
						float neighborNoise = Get2dNoiseZeroToOne(posX, posY, g_theWorld->m_seed);
						if (neighborNoise > max)
						{
							max = neighborNoise;
						}
					}
				}

				if (TreeNoise == max)
				{
					// check if the the tree planting origin terrain height is above water
					if (m_terrainHeightZ[treeColumnIndex] > WATER_LEVEL_Z)
					{
						// discuss what kind of tree need to plant here
						if (m_humidity[treeColumnIndex] < 0.4f)
						{
							SpawnTreeBlockTemplatesAtColumCoords(BlockTemplate::GetBlockTemplateByName("cactus"), localCoords);
						}
						else if (m_temperature[treeColumnIndex] < 0.4f)
						{
							SpawnTreeBlockTemplatesAtColumCoords(BlockTemplate::GetBlockTemplateByName("spruce"), localCoords);
						}
						else
						{
							// calculate the chance to spawn spruce and oak then decide which tree to spawn
							float spruceChance = RangeMap(m_temperature[treeColumnIndex], 0.4f, 0.7f, 1.f, 0.f);
							if (TreeNoise < spruceChance)
							{
								SpawnTreeBlockTemplatesAtColumCoords(BlockTemplate::GetBlockTemplateByName("spruce"), localCoords);
							}
							else
							{
								SpawnTreeBlockTemplatesAtColumCoords(BlockTemplate::GetBlockTemplateByName("oak"), localCoords);
							}
						}
					}
				}
			}

		}
	}

}

void Chunk::SpawnTreeBlockTemplatesAtColumCoords(BlockTemplate treeTemplate, IntVec2 localCoords)
{
	// base on the local column coords, find which chunk to plant the tree - origin
	// get the local coords of the chunk and get the height of the column
	IntVec2 treeCoords = GetBiomeCoordsByLocalCoords(localCoords);
	int m_treeColumnIndex = GetBiomeIndexByTreeCoords(treeCoords);
	int groundHeightZ = m_terrainHeightZ[m_treeColumnIndex];
	bool canPlantTreeHere = true;

	// if there local coords is outside the current, check the nearby 
	
	// for (int localZ = CHUNK_SIZE_Z; localZ >= groundHeightZ; --localZ)
	// {
	// 	int blockIndex = chunkPtr->GetIndexForLocalCoordinates(IntVec3(localCoords.x, localCoords.y, localZ));
	// 	if (localZ == groundHeightZ)
	// 	{
	// 		if (chunkPtr->m_blocks[blockIndex].IsSolid())
	// 		{
	// 			canPlantTreeHere = true;
	// 		}
	// 	}
	// 	else
	// 	{
	// 		// if there is anything visible on the grass ground, do not spawn a tree
	// 		if (!chunkPtr->m_blocks[blockIndex].IsVisible())
	// 		{
	// 			break;
	// 		}
	// 	}
	// }

	if (canPlantTreeHere)
	{
		IntVec3 treeLocalOrigin = IntVec3(localCoords.x, localCoords.y, groundHeightZ);

		// get all the tree block info and set the block type of the chunk
		for (int blockIndex = 0; blockIndex < (int)treeTemplate.m_entries.size(); ++blockIndex)
		{
			BlockTemplateEntry& entry = treeTemplate.m_entries[blockIndex];
			IntVec3 entryLocalCoords = treeLocalOrigin + entry.offset;
			// if (Get2dNoiseZeroToOne(globalX, globalY, g_theWorld->m_seed) < entry.chanceToSpawn) // will cause light influence issue, some block is black
			// {
				// if this tree's block is in this chunk's space, rewrite the type
				if (AreTheLocalCoordsInChunk(entryLocalCoords))
				{
					int treeblockIndex = GetIndexForLocalCoordinates(entryLocalCoords);
					m_blocks[treeblockIndex].SetType(entry.blockDef);
				}
			// }
		}
	}
}


IntVec2 Chunk::GetBiomeCoordsByLocalCoords(IntVec2 localCoords)
{
	IntVec2 neighborHoodCoords;
	neighborHoodCoords.x = localCoords.x + TREE_COVER_SIZE_MAX;
	neighborHoodCoords.y = localCoords.y + TREE_COVER_SIZE_MAX;
	return neighborHoodCoords;
}

IntVec3 Chunk::GetLocalCoordsByTreeCoords(IntVec3 treeCoords)
{
	IntVec3 localCoords;
	localCoords.x = treeCoords.x - TREE_COVER_SIZE_MAX;
	localCoords.y = treeCoords.y - TREE_COVER_SIZE_MAX;
	localCoords.z = treeCoords.z;
	return localCoords;
}

int Chunk::GetBiomeIndexByTreeCoords(IntVec2 neighboorhoodCoords)
{
	return (neighboorhoodCoords.y * (CHUNK_SIZE_X + 2 * TREE_COVER_SIZE_MAX) + neighboorhoodCoords.x);
}

bool Chunk::AreTheTreeNeighborCoordsInChunk(IntVec3 neighborhoodCoords)
{
	IntVec3 localCoords;
	localCoords.x = neighborhoodCoords.x - TREE_COVER_SIZE_MAX;
	localCoords.y = neighborhoodCoords.y - TREE_COVER_SIZE_MAX;
	localCoords.z = neighborhoodCoords.z;
	if (localCoords.x >= 0 && localCoords.x < CHUNK_SIZE_X)
	{
		if (localCoords.y >= 0 && localCoords.y < CHUNK_SIZE_Y)
		{
			if (localCoords.z >= 0 && localCoords.z < CHUNK_SIZE_Z)
			{
				return true;
			}
		}
	}

	return false;
}

bool Chunk::AreTheLocalCoordsInChunk(IntVec3 localdCoords)
{
	if (localdCoords.x >= 0 && localdCoords.x < CHUNK_SIZE_X)
	{
		if (localdCoords.y >= 0 && localdCoords.y < CHUNK_SIZE_Y)
		{
			if (localdCoords.z >= 0 && localdCoords.z < CHUNK_SIZE_Z)
			{
				return true;
			}
		}
	}

	return false;
}

void Chunk::CreateInitialBlocks()
{
	// for each column, based on the height, it is given of different blockDefs
	for (int localX = 0; localX < CHUNK_SIZE_X; ++localX)
	{
		for (int localY = 0; localY < CHUNK_SIZE_Y; ++localY)
		{
			// The block at terrainHeightZ in each column is “grass”
			// All blocks above this in any column are set to “air”
			// random(3~4) blocks below this are set to “dirt”
			// we first get the dirt number for each column
			int numDirt = g_rng->RollRandomIntInRange(3, 4);
			IntVec2 treeCoords = GetBiomeCoordsByLocalCoords(IntVec2(localX, localY));
			int biomeIndex = GetBiomeIndexByTreeCoords(treeCoords);
			int terrainHeight = m_terrainHeightZ[biomeIndex];

			for (int localZ = 0; localZ < CHUNK_SIZE_Z; ++localZ)
			{
				// And all blocks below dirt (down to the bottom of the world) are set to “stone”
				// For any “stone” block to be replaced by a rare ore block type: 5% chance to become “coal”; 
				// if not, 2% chance to become “iron”; 
				// if not, 0.5% chance to become “gold”; 
				// if not, 0.1% chance to become “diamond”.
				if (localZ < (terrainHeight - numDirt))
				{
					if (g_rng->RollRandomFloatZeroToOne() < 0.05f)
					{
						SetBlockTypeForLocalBlock("coal", IntVec3(localX, localY, localZ));
					}
					else if (g_rng->RollRandomFloatZeroToOne() < 0.02f)
					{
						SetBlockTypeForLocalBlock("iron", IntVec3(localX, localY, localZ));
					}
					else if (g_rng->RollRandomFloatZeroToOne() < 0.005f)
					{
						SetBlockTypeForLocalBlock("gold", IntVec3(localX, localY, localZ));
					}
					else if (g_rng->RollRandomFloatZeroToOne() < 0.001f)
					{
						SetBlockTypeForLocalBlock("diamond", IntVec3(localX, localY, localZ));
					}
					else
					{
						SetBlockTypeForLocalBlock("stone", IntVec3(localX, localY, localZ));
					}
				}
				else if (localZ >= (terrainHeight - numDirt) && localZ < terrainHeight)
				{
					// Grass and Dirt blocks on or near the surface are instead replaced with sand blocks
					if (m_humidity[biomeIndex] < 0.4f)
					{
						SetBlockTypeForLocalBlock("sand", IntVec3(localX, localY, localZ));
					}
					else if (g_rng->RollRandomFloatZeroToOne() < 0.5f)
					{
						SetBlockTypeForLocalBlock("dirt", IntVec3(localX, localY, localZ));
					}
					else
					{
						SetBlockTypeForLocalBlock("grassBrick", IntVec3(localX, localY, localZ));
					}
				}
				else if (localZ == terrainHeight)
				{
					// If Humidity is moderate or low (say, below 0.6 or 0.7)
					// replace grass blocks (only) exactly at sea level with sand blocks instead.  
					// Areas of moderate humidity should have beaches; wet areas (high humidity) should not.
					if (m_humidity[biomeIndex] < 0.6f && terrainHeight == WATER_LEVEL_Z)
					{
						SetBlockTypeForLocalBlock("sand", IntVec3(localX, localY, localZ));
					}
					else if (m_humidity[biomeIndex] < 0.4f)
					{
						SetBlockTypeForLocalBlock("sand", IntVec3(localX, localY, localZ));
					}
					else
					{
						SetBlockTypeForLocalBlock("grass", IntVec3(localX, localY, localZ));
					}
				}
				else if (localZ > terrainHeight)
				{
					// Any blocks that would otherwise be “air” but whose z block coordinate is <= CHUNK_HEIGHT/2 is “water” instead
					if (localZ <= WATER_LEVEL)
					{
						if (m_temperature[biomeIndex] < 0.4f)
						{
							SetBlockTypeForLocalBlock("ice", IntVec3(localX, localY, localZ));
						}
						else
						{
							SetBlockTypeForLocalBlock("water", IntVec3(localX, localY, localZ));
						}
					}
					else
					{
						SetBlockTypeForLocalBlock("air", IntVec3(localX, localY, localZ));
					}
				}				
			}
		}
	}
}

void Chunk::SetBlockTypeForLocalBlock(std::string blcokTypeName, IntVec3 const& localCoords)
{
	int index = GetIndexForLocalCoordinates(localCoords);
	m_blocks[index].SetType(blcokTypeName);
}

int Chunk::GetIndexForLocalCoordinates(IntVec3 const& localCoords)
{
	// same to this
	// int index = localCoords.x + localCoords.y * CHUNK_SIZE_X + localCoords.z * CHUNK_BLOCKS_PER_LAYER;
	int index = localCoords.x | (localCoords.y << CHUNK_BITS_X) | (localCoords.z << (CHUNK_BITS_X + CHUNK_BITS_Y));
	return index;
}

IntVec3 Chunk::GetlocalBlockCoordsForIndex(int localBlockIndex)
{
	IntVec3 localBlockCoords;
	localBlockCoords.x = localBlockIndex & CHUNK_MASK_X;
	localBlockCoords.y = (localBlockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;
	localBlockCoords.z = localBlockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);
	return localBlockCoords;
}

IntVec3 Chunk::GetlocalBlockCoordsForWorldPos(Vec3 const& worldPos)
{
	int localBlockX = int(floorf(worldPos.x)) - (m_chunkCoords.x * CHUNK_SIZE_X);
	int localBlockY = int(floorf(worldPos.y)) - (m_chunkCoords.y * CHUNK_SIZE_Y);
	int localBlockZ = int(floorf(worldPos.z));
	return IntVec3(localBlockX, localBlockY, localBlockZ);
}

AABB3 Chunk::GetBlockBoundsInLocalSpace(IntVec3 const& localCoords)
{
	AABB3 blockGlobalBounds;
	blockGlobalBounds.m_mins = Vec3(float(localCoords.x), float(localCoords.y), float(localCoords.z));
	blockGlobalBounds.m_maxs = blockGlobalBounds.m_mins + Vec3(1.f, 1.f, 1.f);

	Vec3 chunkWorldOrigin = GetChunkWorldOrigin();
	blockGlobalBounds.m_mins += chunkWorldOrigin;
	blockGlobalBounds.m_maxs += chunkWorldOrigin;
	return blockGlobalBounds;
}

AABB3 Chunk::GetBlockBoundsInWorldSpace(IntVec3 const& localCoords)
{	
	AABB3 blockGlobalBounds;
	blockGlobalBounds.m_mins = Vec3(float(localCoords.x), float(localCoords.y), float(localCoords.z)) + Vec3(float(m_chunkCoords.x), float(m_chunkCoords.y), 0.f);
	blockGlobalBounds.m_maxs = blockGlobalBounds.m_mins + Vec3(1.f, 1.f, 1.f);
	return blockGlobalBounds;
}

AABB3 Chunk::GetBlockBoundsInWorldSpace(int blockIndex)
{
	IntVec3 blockLocalCoords = GetlocalBlockCoordsForIndex(blockIndex);

	AABB3 blockGlobalBounds;
	blockGlobalBounds.m_mins = Vec3(float(blockLocalCoords.x), float(blockLocalCoords.y), float(blockLocalCoords.z)) + Vec3(float(m_chunkCoords.x * CHUNK_SIZE_X), float(m_chunkCoords.y * CHUNK_SIZE_X), 0.f);
	blockGlobalBounds.m_maxs = blockGlobalBounds.m_mins + Vec3(1.f, 1.f, 1.f);
	return blockGlobalBounds;
}

int Chunk::GetFirstSolidBlockUnderInputCoords(IntVec3 const& standingLocalCoords)
{
	for (int z = (CHUNK_SIZE_Z - 1); z >= 0; --z)
	{
		int index = GetIndexForLocalCoordinates(IntVec3(standingLocalCoords.x, standingLocalCoords.y, z));
		if (m_blocks[index].GetBlockDef().m_isSolid)
		{
			return index;
		}
	}
	return 0;
}

bool Chunk::IfTheBlockIsAtTheBottomOfTheChunk(IntVec3 const& blockCoords)
{
	return blockCoords.z == 0;
}

bool Chunk::IfTheBlockIsAirOrWaterOrOutOfChunk(IntVec3 const& blockCoords)
{
	int index = GetIndexForLocalCoordinates(blockCoords);
	if (CheckIfTheBlockCoordsIsInsideChunkBound(blockCoords))
	{
		return true;
	}
	else
	{
		if (m_blocks[index].GetBlockDef() == BlockDef::GetBlockDefByName("air") || m_blocks[index].GetBlockDef() == BlockDef::GetBlockDefByName("water"))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool Chunk::IfTheBlockIsAirOrOutOfChunk(IntVec3 const& blockCoords)
{
	int index = GetIndexForLocalCoordinates(blockCoords);
	if (!CheckIfTheBlockCoordsIsInsideChunkBound(blockCoords))
	{
		return true;
	}
	else
	{
		if (m_blocks[index].GetBlockDef() == BlockDef::GetBlockDefByName("air"))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool Chunk::CheckIfTheBlockCoordsIsInsideChunkBound(IntVec3 const& blockCoords)
{
	if (blockCoords.x >= 0 && blockCoords.x <= (CHUNK_SIZE_X - 1) &&
		blockCoords.y >= 0 && blockCoords.y <= (CHUNK_SIZE_Y - 1) &&
		blockCoords.z >= 0 && blockCoords.z <= (CHUNK_SIZE_Z - 1))
	{
		return true;
	}
	else return false;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
bool Chunk::DigBlock(int blockIndex)
{
	m_blocks[blockIndex].SetType("air");
	m_isMeshDirty = true;
	m_needsSaving = true;
	return true;
}

void Chunk::SetBlock(int blockIndex, BlockDef const& def)
{
	m_blocks[blockIndex].SetType(def);
	m_isMeshDirty = true;
	m_needsSaving = true;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::Render() const
{
	RenderBlocks();
}

void Chunk::RenderBlocks() const
{
	if (m_vertexBuffer)
	{
		g_theRenderer->DrawVertexBuffer(m_vertexBuffer, (int)(m_vertexBuffer->m_size));
	}
}

Mat44 Chunk::GetModelMatrix() const
{
	Mat44 transformMat;
	Vec3 chunkWorldOrigin = GetChunkWorldOrigin();
	transformMat.SetTranslation3D(chunkWorldOrigin);
	Mat44 orientationMat = m_chunkOrientation.GetAsMatrix_XFwd_YLeft_ZUp();
	transformMat.Append(orientationMat);
	return transformMat;
}

void Chunk::AddVertsForAllBlocksInChunk()
{
	m_blockVerts.clear();
	m_blockVerts.reserve(CHUNK_BLOCKS_TOTAL * 36);

	for (int i = 0; i < CHUNK_BLOCKS_TOTAL; ++i)
	{
		AddVertsForBlock(i);
	}
}

void Chunk::DrawDebugRender() const
{
	std::vector<Vertex_PCU> debugVerts;

	Vec3 mins(0.f, 0.f, 0.f);
	Vec3 maxs(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z);
	AABB3 chunkBounds(mins, maxs);

	AddVertsForAABB3Frame(debugVerts, chunkBounds);

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetModelConstants(GetModelMatrix());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(int(debugVerts.size()), debugVerts.data());
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::AddVertsForBlock(int localBlockIndex)
{
	BlockDef const& blockDef = m_blocks[localBlockIndex].GetBlockDef();
	IntVec3 localBlockCoords = GetlocalBlockCoordsForIndex(localBlockIndex);

	// if the blockDef is invisible, no need to add verts
	if (!blockDef.m_isVisible)
	{
		return;
	}

	AABB3 blockBounds = GetBlockBoundsInLocalSpace(localBlockCoords);

	// Get all the points of this block
	Vec3 BBL;
	Vec3 BBR;
	Vec3 BTR;
	Vec3 BTL;
	Vec3 FBL;
	Vec3 FBR;
	Vec3 FTR;
	Vec3 FTL;
	blockBounds.GetAllEightPointsOfTheCorners(BBL, BBR, BTR, BTL, FBL, FBR, FTR, FTL);

	// get UVs for blocks' all surfaces
	AABB2 const& wallUVs = blockDef.GetTileTextureUVsOnSpriteSheet(blockDef.m_sideSpriteCoords);
	AABB2 floorUVs = blockDef.GetTileTextureUVsOnSpriteSheet(blockDef.m_bottomSpriteCoords);
	AABB2 ceilUVs = blockDef.GetTileTextureUVsOnSpriteSheet(blockDef.m_topSpriteCoords);

	BlockIter blockIter(this, localBlockIndex);
	BlockIter neighborBlockIter = blockIter.GetNorthNeighbor();
	uint8_t outdoorLight = 0;
	uint8_t indoorLight = 0;
	Rgba8 faceTint;
	faceTint.b = 127;
	// north face
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.GetBlock()->IsVisible() == false)
		{
			outdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
			indoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();

			faceTint.r = (unsigned char)RangeMapClamped(outdoorLight, 0.f, 15.f, 0.f, 255.f);
			faceTint.g = (unsigned char)RangeMapClamped(indoorLight, 0.f, 15.f, 0.f, 255.f);

			AddVertsForQuad3D(m_blockVerts, FBR, BBR, BTR, FTR, faceTint, wallUVs); // north
		}
	}
	// south
	neighborBlockIter = blockIter.GetSouthNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.GetBlock()->IsVisible() == false)
		{
			Block block = *neighborBlockIter.GetBlock();
			outdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
			indoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
			faceTint.r = (unsigned char)RangeMapClamped(outdoorLight, 0.f, 15.f, 0.f, 255.f);
			faceTint.g = (unsigned char)RangeMapClamped(indoorLight, 0.f, 15.f, 0.f, 255.f);

			AddVertsForQuad3D(m_blockVerts, BBL, FBL, FTL, BTL, faceTint, wallUVs); // south
		}
	}
	// east
	neighborBlockIter = blockIter.GetEastNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.GetBlock()->IsVisible() == false)
		{
			Block block = *neighborBlockIter.GetBlock();
			outdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
			indoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
			faceTint.r = (unsigned char)RangeMapClamped(outdoorLight, 0.f, 15.f, 0.f, 255.f);
			faceTint.g = (unsigned char)RangeMapClamped(indoorLight, 0.f, 15.f, 0.f, 255.f);

			AddVertsForQuad3D(m_blockVerts, FBL, FBR, FTR, FTL, faceTint, wallUVs); // east
		}
	}
	// west
	neighborBlockIter = blockIter.GetWestNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.GetBlock()->IsVisible() == false)
		{
			Block block = *neighborBlockIter.GetBlock();
			outdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
			indoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
			faceTint.r = (unsigned char)RangeMapClamped(outdoorLight, 0.f, 15.f, 0.f, 255.f);
			faceTint.g = (unsigned char)RangeMapClamped(indoorLight, 0.f, 15.f, 0.f, 255.f);

			AddVertsForQuad3D(m_blockVerts, BBR, BBL, BTL, BTR, faceTint, wallUVs); // west
		}
	}	
	// bottom
	neighborBlockIter = blockIter.GetLowerLevelNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.GetBlock()->IsVisible() == false)
		{
			Block block = *neighborBlockIter.GetBlock();
			outdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
			indoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
			faceTint.r = (unsigned char)RangeMapClamped(outdoorLight, 0.f, 15.f, 0.f, 255.f);
			faceTint.g = (unsigned char)RangeMapClamped(indoorLight, 0.f, 15.f, 0.f, 255.f);

			AddVertsForQuad3D(m_blockVerts, BBR, FBR, FBL, BBL, faceTint, floorUVs); // bottom
		}
	}	
	// top
	neighborBlockIter = blockIter.GetUpperLevelNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.GetBlock()->IsVisible() == false)
		{
			Block block = *neighborBlockIter.GetBlock();
			outdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
			indoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
			faceTint.r = (unsigned char)RangeMapClamped(outdoorLight, 0.f, 15.f, 0.f, 255.f);
			faceTint.g = (unsigned char)RangeMapClamped(indoorLight, 0.f, 15.f, 0.f, 255.f);

			AddVertsForQuad3D(m_blockVerts, BTL, FTL, FTR, BTR, faceTint, ceilUVs); // top
		}
	} 
	// assignment 2
	//// we are trying to decrease mesh faces here
	//int upperLayerBlockIndex = localBlockIndex + CHUNK_BLOCKS_PER_LAYER;
	//if (CheckIfTheBlockCoordsIsInsideChunkBound(localBlockCoords))
	//{
	//	// if this block's upper layer block exist and it is air
	//	if (m_blocks[upperLayerBlockIndex].GetBlockDef() == BlockDef::GetBlockDefForName("air"))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, BTL, FTL, FTR, BTR, white, ceilUVs); // top

	//		// then we have to check if its neighbor is air or we do not need to draw it
	//		//	East and west faces are tinted light gray (230, 230, 230)
	//		//	North and south faces are a slightly darker gray (200, 200, 200)
	//		if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(1, 0, 0)))
	//		{
	//			AddVertsForQuad3D(m_blockVerts, FBL, FBR, FTR, FTL, lightGray, wallUVs); // east
	//		}
	//		if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(-1, 0, 0)))
	//		{
	//			AddVertsForQuad3D(m_blockVerts, BBR, BBL, BTL, BTR, lightGray, wallUVs); // west
	//		}
	//		if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(0, 1, 0)))
	//		{
	//			AddVertsForQuad3D(m_blockVerts, FBR, BBR, BTR, FTR, DarkGray, wallUVs); // north
	//		}
	//		if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(0, -1, 0)))
	//		{
	//			AddVertsForQuad3D(m_blockVerts, BBL, FBL, FTL, BTL, DarkGray, wallUVs); // south
	//		}
	//	}
	//	else		// if this block's upper layer block exist and it is not air, check surroundings
	//	{
	//		if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(1, 0, 0)))
	//		{
	//			AddVertsForQuad3D(m_blockVerts, FBL, FBR, FTR, FTL, lightGray, wallUVs); // east
	//		}
	//		if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(-1, 0, 0)))
	//		{
	//			AddVertsForQuad3D(m_blockVerts, BBR, BBL, BTL, BTR, lightGray, wallUVs); // west
	//		}
	//		if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(0, 1, 0)))
	//		{
	//			AddVertsForQuad3D(m_blockVerts, FBR, BBR, BTR, FTR, DarkGray, wallUVs); // north
	//		}
	//		if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(0, -1, 0)))
	//		{
	//			AddVertsForQuad3D(m_blockVerts, BBL, FBL, FTL, BTL, DarkGray, wallUVs); // south
	//		}
	//	}
	//}
	//else // if this block's upper layer block does not exist, meaning this is the top block of a chunk
	//{
	//	AddVertsForQuad3D(m_blockVerts, BTL, FTL, FTR, BTR, white, ceilUVs); // top

	//	// then we have to check if its neighbor is air or we do not need to draw it
	//	//	East and west faces are tinted light gray (230, 230, 230)
	//	//	North and south faces are a slightly darker gray (200, 200, 200)
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(1, 0, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, FBL, FBR, FTR, FTL, lightGray, wallUVs); // east
	//	}
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(-1, 0, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, BBR, BBL, BTL, BTR, lightGray, wallUVs); // west
	//	}
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(0, 1, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, FBR, BBR, BTR, FTR, DarkGray, wallUVs); // north
	//	}
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(0, -1, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, BBL, FBL, FTL, BTL, DarkGray, wallUVs); // south
	//	}
	//}
	// assignment 1
	//if (m_blocks[localBlockIndex].GetBlockType() == BlockDef::GetBlockDefForName("grass") && localBlockCoords.z >= WATER_LEVEL)
	//{
	//	AddVertsForQuad3D(m_blockVerts, BTL, FTL, FTR, BTR, white, ceilUVs); // top

	//	// then we have to check if its neighbor is air or we do not need to draw it
	//	//	East and west faces are tinted light gray (230, 230, 230)
	//	//	North and south faces are a slightly darker gray (200, 200, 200)
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(1, 0, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, FBL, FBR, FTR, FTL, lightGray, wallUVs); // east
	//	}			
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(-1, 0, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, BBR, BBL, BTL, BTR, lightGray, wallUVs); // west
	//	}
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(0, 1, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, FBR, BBR, BTR, FTR, DarkGray, wallUVs); // north
	//	}		
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(0, -1, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, BBL, FBL, FTL, BTL, DarkGray, wallUVs); // south
	//	}
	//}
	//else
	//{
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(1, 0, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, FBL, FBR, FTR, FTL, lightGray, wallUVs); // east
	//	}
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(-1, 0, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, BBR, BBL, BTL, BTR, lightGray, wallUVs); // west
	//	}
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(0, 1, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, FBR, BBR, BTR, FTR, DarkGray, wallUVs); // north
	//	}
	//	if (IfTheBlockIsAirOrOutOfChunk(localBlockCoords + IntVec3(0, -1, 0)))
	//	{
	//		AddVertsForQuad3D(m_blockVerts, BBL, FBL, FTL, BTL, DarkGray, wallUVs); // south
	//	}

	//	// if(localBlockCoords.x == 0 )
	//	// {
	//	// 	AddVertsForQuad3D(m_blockVerts, BBR, BBL, BTL, BTR, lightGray, wallUVs); // west
	//	// }	
	//	// if(localBlockCoords.x == (CHUNK_SIZE_X - 1))
	//	// {
	//	// 	AddVertsForQuad3D(m_blockVerts, FBL, FBR, FTR, FTL, lightGray, wallUVs); // east
	//	// }	
	//	// if(localBlockCoords.y == 0)
	//	// {
	//	// 	AddVertsForQuad3D(m_blockVerts, FBR, BBR, BTR, FTR, DarkGray, wallUVs); // north
	//	// }
	//	// if (localBlockCoords.y == (CHUNK_SIZE_Y - 1))
	//	// {
	//	// 	AddVertsForQuad3D(m_blockVerts, BBL, FBL, FTL, BTL, DarkGray, wallUVs); // south
	//	// }

	//	// draw the top of the water level block top surface
	//	if (m_blocks[localBlockIndex].GetBlockType() == BlockDef::GetBlockDefForName("water") && localBlockCoords.z == WATER_LEVEL)
	//	{
	//		AddVertsForQuad3D(m_blockVerts, BTL, FTL, FTR, BTR, white, ceilUVs); // top
	//	}
	//}
	//// no need to draw the bottom quad now
	////  Top and bottom block faces should have their vertex colors tinted white (255, 255, 255)
	//// AddVertsForQuad3D(m_blockVerts, FBL, FBR, BBR, BBL, white, floorUVs);
}

void Chunk::CreateVertexBuffer()
{
	if (m_vertexBuffer)
	{
		delete m_vertexBuffer;
	}
	size_t vertexSize = sizeof(Vertex_PCU);
	m_vertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_blockVerts.size()), vertexSize);
}

void Chunk::CopyVertexBufferFromCPUtoGPU()
{
	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArrayDataSize = (m_blockVerts.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_blockVerts.data(), vertexArrayDataSize, m_vertexBuffer);

	// swap the verts data into a temporary vector array
	std::vector<Vertex_PCU> tempVerts;
	tempVerts.swap(m_blockVerts);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::SaveBlocksDataToFile()
{
	std::vector<uint8_t> inBuffer;
	// Chunk files always start with the four character code (4CC) “GCHK” – for Guildhall Chunk
	inBuffer.push_back('G');
	inBuffer.push_back('C');
	inBuffer.push_back('H');
	inBuffer.push_back('K');

	inBuffer.push_back(2); // version 2 of the .chunk file format
	inBuffer.push_back(4); // CHUNK_BITS_X
	inBuffer.push_back(4); // CHUNK_BITS_Y
	inBuffer.push_back(7); // CHUNK_BITS_Z

	//
	uint8_t defIndex = 0;
	uint8_t numBlocksOfSameKind = 0;

	for (int i = 0; i < CHUNK_BLOCKS_TOTAL; ++i)
	{
		defIndex = m_blocks[i].m_blockDefIndex;
		numBlocksOfSameKind = 1;
		for (int j = (i + 1); j < CHUNK_BLOCKS_TOTAL; ++j)
		{
			if (defIndex == m_blocks[j].m_blockDefIndex && numBlocksOfSameKind < 255)
			{
				++numBlocksOfSameKind;
				++i;
			}
			else
			{
				break;
			}
		}
		inBuffer.push_back(defIndex);
		inBuffer.push_back(numBlocksOfSameKind);
	}

	// set up save file name
	unsigned char worldSeed = (unsigned char)g_theWorld->m_seed;
	std::string saveFolderPathName = Stringf("Saves/World_%u", worldSeed);
	std::string fileName = Stringf("%s/Chunk(%i, %i).chunk", saveFolderPathName.c_str(), m_chunkCoords.x, m_chunkCoords.y);

	if (!FileWriteFromBuffer(inBuffer, fileName))
	{
		ERROR_AND_DIE("Have problem saving the chunk data");
	}
	m_needsSaving = false;
}

bool Chunk::CheckIfThereIsSaveFile()
{
	unsigned char worldSeed = (unsigned char)g_theWorld->m_seed;
	std::string saveFolderPathName = Stringf("Saves/World_%u", worldSeed);

	std::string fileName = Stringf("%s/Chunk(%i, %i).chunk", saveFolderPathName.c_str(), m_chunkCoords.x, m_chunkCoords.y);
	bool hasSaveFile = IfThisFileCouldBeRead(fileName);
	if (!hasSaveFile)
	{
		return false;
	}
	else
	{
		if (CheckIfSaveFileMatchWorldSeedNumber(fileName))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool Chunk::CheckIfSaveFileMatchWorldSeedNumber(std::string fileName)
{
	std::vector<uint8_t> tempBuffer;
	FileReadToBuffer(tempBuffer, fileName);

	// verify the beginning of the file to be correct chunk file
	// including the if the save file match with the world seed number
	if (tempBuffer[0] == 'G' &&
		tempBuffer[1] == 'C' &&
		tempBuffer[2] == 'H' &&
		tempBuffer[3] == 'K' &&

		tempBuffer[4] == 2 &&

		tempBuffer[5] == CHUNK_BITS_X &&
		tempBuffer[6] == CHUNK_BITS_Y &&
		tempBuffer[7] == CHUNK_BITS_Z)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Chunk::LoadBlocksDataFromFile()
{
	unsigned char worldSeed = (unsigned char)g_theWorld->m_seed;
	std::string saveFolderPathName = Stringf("Saves/World_%u", worldSeed);

	std::string fileName = Stringf("%s/Chunk(%i, %i).chunk", saveFolderPathName.c_str(), m_chunkCoords.x, m_chunkCoords.y);
	std::vector<uint8_t> outBuffer;
	FileReadToBuffer(outBuffer, fileName);

	// verify the beginning of the file to be correct chunk file
	if (outBuffer[0] == 'G' &&
		outBuffer[1] == 'C' &&
		outBuffer[2] == 'H' &&
		outBuffer[3] == 'K' &&

		outBuffer[5] == CHUNK_BITS_X &&
		outBuffer[6] == CHUNK_BITS_Y &&
		outBuffer[7] == CHUNK_BITS_Z)
	{
		uint8_t defIndex = 0;
		uint8_t numBlocksOfSameKind = 1;
		int blockIndex = 0;

		// decipher the data
		for (int i = 8; i < (outBuffer.size() - 1) ; i += 2)
		{
			defIndex = outBuffer[i];
			numBlocksOfSameKind = outBuffer[i + 1];

			// write into block data
			for (int j = numBlocksOfSameKind; j > 0; --j)
			{
				m_blocks[blockIndex].m_blockDefIndex = defIndex;
				m_blocks[blockIndex].UpdateBlockBitFlagsByBlockDef(); // otherwise, the block bit flag will all be zero
				++blockIndex;
			}
		}
	}
	else
	{
		ERROR_AND_DIE(Stringf("Error reading %s, wrong format", fileName.c_str())); 
	}
}

// takes the chunk coordinates in the world and transform it into world location
Vec3 Chunk::GetChunkWorldOrigin() const
{
	Vec3 worldLocation;
	worldLocation.x = float(m_chunkCoords.x * CHUNK_SIZE_X);
	worldLocation.y = float(m_chunkCoords.y * CHUNK_SIZE_Y);
	worldLocation.z = 0.f;
	return worldLocation;
}

bool Chunk::DoAllEightSurroundingNeighborChunksExist()
{
	if (m_eastNeighbor && m_westNeighbor && m_northNeighbor && m_southNeighbor)
	{
		if (m_eastNeighbor->m_northNeighbor && m_eastNeighbor->m_southNeighbor)
		{
			if (m_westNeighbor->m_northNeighbor && m_westNeighbor->m_southNeighbor)
			{
				return true;
			}
		}

		return false;
	}
	else
	{
		return false;
	}
}

bool Chunk::DoAllFourSurroundingNeighborChunksExist()
{
	if (m_eastNeighbor && m_westNeighbor && m_northNeighbor && m_southNeighbor)
	{
		return true;
	}
	else
	{
		return false;
	}
}

////----------------------------------------------------------------------------------------------------------------------------------------------------
//constexpr int waterlevelZ = CHUNK_SIZE_X / 2; // consider the river and ocean share the same water level
//constexpr int maxRiverDepth = 5;
//constexpr int riverBedZ = waterlevelZ - maxRiverDepth;
//constexpr int maxTerrainHeighAboveWater = CHUNK_SIZE_Z - waterlevelZ; // the part where moutain stands above the water level
//
//float  terrainHeightAboveRiverBed = terrainNoiseFromZeroToOne * maxTerrainHeighAboveWater; // heightest moutain
//float terrainHeightAboveWater = terrainHeightAboveRiverBed - maxRiverDepth;
//
//if (terrainHeightAboveWater > 0.f)
//{
//	terrainHeightAboveWater *= hilliness;
//	float terrainHeightAboveWaterBed = terrainHeightAboveWater + maxRiverDepth;
//}
//
//groundHeightZ = riverBedZ + terrainHeightAboveWaterBed;
//
////----------------------------------------------------------------------------------------------------------------------------------------------------
//float temperature = 0.5f + 0.5f * Compute2dPerlinNoise(); // range from 0 - 1, so we could use all easing functions
//float hilliness = 0.5f + 0.5f * Compute2dPerlinNoise(2000, 3); // range from 0 - 1, so we could use all easing functions
//float Oceanness = 0.5f + 0.5f * Compute2dPerlinNoise(2000, 7);
//float Humidity = 0.5f + 0.5f * Compute2dPerlinNoise(800, 5);
//float terrainNoiseFromZeroToOne = ;
//float terrainHie
//
//float hilliness = SmoothStep3(SmoothStep3(hilliness));
//float terrainNoise = fabsf(terrainNoiseFromZeroToOne);
//float moutainElevation = hillness * maxMoutainElevation;
//float terrainScale = moutainElevation + maxRiverDepth;
//
//
//// smooth out the moutains over the water level, africa has low hilliness and tibet and high hilliness
//int reginalMountainElevation = maxMountainElevation * hilliness;
//float actualElevationHere = terrainNoiseFromZeroToOne * reginalMountainElevation;
//
//int maxTerrainHeightFromRiverBottom = riverDepth + reginalMountainElevation;
//int terrainHeightFromRiverBottom = int(terrainScale * maxTerrainHeightFromRiverBottom); // how much coming out of river
//int groundHeightZ = OceanHeightZ - riverDepth + terrainHeightFromRiverBottom;
//
////----------------------------------------------------------------------------------------------------------------------------------------------------
//constexpr int maxOceanLowering = 30;
//int OceanLowing = maxOceanLowering * Oceanness;
//Oceanness = SmoothStep3(SmoothStep3(Oceanness));
//groundHeightZ -= OceanLowing;
//terrainNoiseFromZeroToOne = SmoothStop3(SmoothStop3(terrainNoiseFromZeroToOne)); // cayon
//// Humidity = 1 - (1 - Humidity)(1 - Oceanness); // both humidity and oceanness will contirbute but result will not over 1
//
//struct BlockTemplateEntry
//{
//	// float chanceToSpawn;
//	BlockDef def;
//	IntVec3 offset;
//};
//
//struct BlockTemplate
//{
//	BlockTemplateEntry m_entries[12];
//};
//for (int i = 0; i < TreeToDoTemplateList.size(); ++ i)
//{
//	SpawnTreeBlockTemplate(TreeToDoTemplateList.m_OakBlockTemplate); // do not place the tree if there is only air there
//}


