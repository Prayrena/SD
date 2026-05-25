#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/core/JobSystem.hpp"
#include "Engine/Core/Time.hpp"
#include "ThirdParty/Noise_Squirrel/SmoothNoise.hpp"
#include "ThirdParty/Noise_Squirrel/RawNoise.hpp"
#include "Game/Entity.hpp"
#include "Game/World.hpp"
#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/Player.hpp"
#include <cmath>

static const int k_lightingFogConstantsSlot = 8;

extern InputSystem* g_theInput;
extern Clock* g_theGameClock;
extern Game* g_theGame;
extern JobSystem* g_theJobSystem;
extern App* g_theApp;

World::World()
{
	m_dataTimer = new Timer(m_recordPeroid, g_theGameClock);
	m_dataTimer->Start();

	BlockDef::InitializeBlockDefs();
	BlockTemplate::InitializeBlockTemplates();
}

World::~World()
{
	delete m_dataTimer;

	// todo: loop the map, delete or ++iter then clear
	std::map<IntVec2, Chunk*>::iterator iter;
	for (iter = m_activeChunks.begin(); iter != m_activeChunks.end(); ++iter)
	{
		Chunk* chunk = iter->second;
		if (chunk->m_needsSaving)
		{
			chunk->SaveBlocksDataToFile();
		}

		delete chunk;
		chunk = nullptr;
	}

	m_activeChunks.clear();
}

void World::Startup()
{
	m_gpuShaderData = new SimpleMinerGPUData;
	m_lighting_Fog_CBO = g_theRenderer->CreateConstantBuffer(sizeof(SimpleMinerGPUData));
	g_theGame->m_player->m_buildingBlockDef = BlockDef::GetBlockDefByName("cobbleStone");
	m_worldDay += m_worldStartTime;
	m_seed = g_gameConfigBlackboard.GetValue("WorldSeed", 0);

	std::string spriteSheetPath = "Data/Images/BasicSprites_64x64.png";
	m_blockTexture = g_theRenderer->CreateOrGetTextureFromFile(spriteSheetPath.c_str());

	// fog setting
	m_enableFog = g_gameConfigBlackboard.GetValue("EnableFog", true);
	if (!m_enableFog)
	{
		m_gpuShaderData->m_fogEndDist = 999'999'999.f;
	}
}

void World::Update()
{
	double timeAtStart = GetCurrentTimeSeconds();
	UpdateTime();

	UpdatePlayerLocatedCoords();
	ShootRaycastForCollisionTest(dynamic_cast<Player*>(g_theGame->m_player)->m_raycastDist);

	DeactivateChunks();
	ActivateChunks(); // 1
	RetrieveCompletedChunkGenerationJobAndActivate();

	WorldInputControl();
	g_theGame->m_player->Update(); // 2
	UpdateSimplerMinerWorldShader();

	ProcessDirtyLighting(); // 3
	UpdateAllActiveChunks(); // 4 build new vertex buffer data after lighting is corrected

	if (GetDebugRenderVisibility())
	{
		UpdateOnScreenDisplayMessages();
	}

	double timeAtEnd = GetCurrentTimeSeconds();
	double timeElapsed = timeAtEnd - timeAtStart;
	g_theDevConsole->AddLine(Stringf("worldUpdate = %.02f ms", timeElapsed * 1000.0), Rgba8::RED);
}

void World::Render() const
{
	double timeAtStart = GetCurrentTimeSeconds();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// chunks drawing
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->BindTexture(m_blockTexture);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetModelConstants(Mat44());
	BindSimplerMinerWorldShaderData();
	g_theRenderer->BindShader(g_theApp->g_shaders[WORLD]);

	std::map<IntVec2, Chunk*>::const_iterator iter;
	for (iter = m_activeChunks.begin(); iter != m_activeChunks.end(); ++iter)
	{
		iter->second->Render();
	}

	if (g_theApp->m_debugMode)
	{
		for (iter = m_activeChunks.begin(); iter != m_activeChunks.end(); ++iter)
		{
			iter->second->DrawDebugRender();
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	RenderPlayerAimingSurface();

	RenderDebug();

	double timeAtEnd = GetCurrentTimeSeconds();
	double timeElapsed = timeAtEnd - timeAtStart;
	g_theDevConsole->AddLine(Stringf("worldRender = %.02f ms", timeElapsed * 1000.0), Rgba8::GREEN);
}

void World::RenderDebug() const
{
	if (m_raycastIsLocked)
	{
		RenderLockedRaycastForDebug();
	}
}

void World::CreateInitialChunks()
{
	// (for Assignment 1 only) only 4 chunks are activated : at chunk coordinates(0, 0), (2, 0), (2, 1) and (2, -1)
	// Chunk* chunk_1 = new Chunk(IntVec2(0, 0));
	// m_activeChunks.push_back(chunk_1);	
	// 
	// Chunk* chunk_2 = new Chunk(IntVec2(2, 0));
	// m_activeChunks.push_back(chunk_2);	
	// 
	// Chunk* chunk_3 = new Chunk(IntVec2(2, 1));
	// m_activeChunks.push_back(chunk_3);	
	// 
	// Chunk* chunk_4 = new Chunk(IntVec2(2, -1));
	// m_activeChunks.push_back(chunk_4);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void World::UpdatePlayerLocatedCoords()
{
	// calculate located chunk coords
	Vec3 playerWorldPos = g_theGame->m_player->m_position;
	m_playerChunkCoords = GetChunkCoordsForWorldPos(playerWorldPos);

	// get located chunk ptr
	std::map<IntVec2, Chunk*>::iterator iter = m_activeChunks.find(m_playerChunkCoords);
	if (iter != m_activeChunks.end())
	{
		m_playerLocatedChunk = iter->second;
		m_playerLocalBlockCoords = m_playerLocatedChunk->GetlocalBlockCoordsForWorldPos(playerWorldPos);
	}
	else
	{
		m_playerLocatedChunk = nullptr;

		m_playerLocalBlockCoords = BAD_BLOCK_COORDS;
	}
}

void World::RenderPlayerAimingSurface() const
{
	Chunk* chunkPtr = m_playerAimingRaycastResult.m_aimedBlockIter.m_chunk;
	if (m_playerAimingRaycastResult.m_aimedBlockIter.GetBlock() && m_playerAimingRaycastResult.m_didImpact)
	{
		Vec3 pts[8];
		AABB3 bounds = chunkPtr->GetBlockBoundsInWorldSpace(m_playerAimingRaycastResult.m_aimedBlockIter.m_blockIndex);
		bounds.GetCornerPoints(pts);

		Vec3& BBR = pts[0];
		Vec3& BTR = pts[1];
		Vec3& TBR = pts[2];
		Vec3& TTR = pts[3];

		Vec3& BBL = pts[4];
		Vec3& BTL = pts[5];
		Vec3& TBL = pts[6];
		Vec3& TTL = pts[7];

		std::vector<Vertex_PCU> verts;
		// based on the raycast normal result draw the quad frame
		Vec3 normal = m_playerAimingRaycastResult.m_impactNormal;
		Vec3 BL = Vec3();
		Vec3 BR = Vec3();
		Vec3 TR = Vec3();
		Vec3 TL = Vec3();

		if (normal == Vec3(0.f, 0.f, 1.f)) // top
		{
			BL = TBL;
			BR = TBR;
			TR = TTR;
			TL = TTL;
		}		
		else if (normal == Vec3(0.f, 0.f, -1.f)) // bottom
		{
			BL = BTL;
			BR = BTR;
			TR = BBR;
			TL = BBL;
		}
		else if (normal == Vec3(-1.f, 0.f, 0.f)) // left
		{
			BL = BTL;
			BR = BBL;
			TR = TBL;
			TL = TTL;
		}
		else if (normal == Vec3(1.f, 0.f, 0.f)) // right
		{
			BL = BBR;
			BR = BTR;
			TR = TTR;
			TL = TBR;
		}
		else if (normal == Vec3(0.f, -1.f, 0.f)) // back
		{
			BL = BBL;
			BR = BBR;
			TR = TBR;
			TL = TBL;
		}		
		else if (normal == Vec3(0.f, 1.f, 0.f)) // front
		{
			BL = TTL;
			BR = TTR;
			TR = BTR;
			TL = BTL;
		}
		else
		{
			return;
			// ERROR_RECOVERABLE("A wired raycast normal result");
		}
		AddVertsForQuad3DFrame(verts, BL, BR, TR, TL, 0.f, 0.1f, Rgba8::CYAN);

		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetModelConstants(Mat44());
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray(int(verts.size()), verts.data());
	}
}

IntVec2 World::GetChunkCoordsForWorldPos(Vec3 worldPos)
{
	constexpr float oneOverChunkSizeX = 1.f / (float)CHUNK_SIZE_X;
	constexpr float oneOverChunkSizeY = 1.f / (float)CHUNK_SIZE_Y;
	int chunkCoordsX = int(floorf(worldPos.x * oneOverChunkSizeX));
	int chunkCoordsY = int(floorf(worldPos.y * oneOverChunkSizeY));
	return IntVec2(chunkCoordsX, chunkCoordsY);
}

void World::StartupAllBlocks()
{
	// for (int i = 0; i < (int)m_activeChunks.size(); ++i)
	// {
	// 	m_activeChunks[i]->Startup();
	// }
}

void World::UpdateAllActiveChunks()
{
	std::map<IntVec2, Chunk*>::iterator iter;
	for (iter = m_activeChunks.begin(); iter != m_activeChunks.end(); ++iter)
	{
		iter->second->Update();
	}
}

void World::DeactivateChunks()
{
	if (m_activeChunks.size() > MAX_CHUNKS)
	{
		DeactivateFarthestActiveChunk();
	}

	DeactivateFarthestActiveChunkOutOfPlayerRange();
}

void World::ActivateChunks()
{
	if (m_activeChunks.size() < MAX_CHUNKS)
	{
		ActivateNearestMissingChunkInPlayerRange();
	}
}

// if the current active chunks are too many, we will find a farthest one and remove it
void World::DeactivateFarthestActiveChunk()
{
	// Get the farthest active chunk from player's located chunk
	int minDistSqr = 0;
	IntVec2 deactivateChunkCoords = BAD_CHUNK_COORDS;
	std::map<IntVec2, Chunk*>::iterator iter;
	for (iter = m_activeChunks.begin(); iter != m_activeChunks.end(); ++iter)
	{
		Chunk*& chunk = iter->second;
		int disSqr = chunk->m_chunkCoords.GetLengthSquaredToThisCoords(m_playerChunkCoords);
		if (disSqr > minDistSqr)
		{
			minDistSqr = disSqr;
			deactivateChunkCoords = chunk->m_chunkCoords;
		}
	}

	// remove the active chunk from map
	if (deactivateChunkCoords != BAD_CHUNK_COORDS)
	{
		DeactivateChunk(deactivateChunkCoords);
	}
}

// find any chunk that is too far away from the player and delete it
void World::DeactivateFarthestActiveChunkOutOfPlayerRange()
{
	int maxDistSqr = int(CHUNK_DACTIVATE_RANGE * CHUNK_DACTIVATE_RANGE);
	IntVec2 deactivateChunkCoords = BAD_CHUNK_COORDS;

	std::map<IntVec2, Chunk*>::iterator iter;
	for (iter = m_activeChunks.begin(); iter != m_activeChunks.end(); ++iter)
	{
		Chunk*& chunk = iter->second;
		int disSqr = chunk->m_chunkCoords.GetLengthSquaredToThisCoords(m_playerChunkCoords) * CHUNK_SIZE_X * CHUNK_SIZE_Y;
		if (disSqr > maxDistSqr)
		{
			maxDistSqr = disSqr;
			deactivateChunkCoords = chunk->m_chunkCoords;
		}
	}

	// remove the active chunk from map
	if (deactivateChunkCoords != BAD_CHUNK_COORDS)
	{
		DeactivateChunk(deactivateChunkCoords);
	}
}

void World::DeactivateChunk(IntVec2 chunkCoords)
{
	std::map<IntVec2, Chunk*>::iterator iter;
	iter = m_activeChunks.find(chunkCoords);
	Chunk*& chunk = iter->second;
	chunk->m_chunkState = ChunkState::DEACTIVATING_QUEUED_SAVE;

	if (chunk->m_needsSaving)
	{
		chunk->SaveBlocksDataToFile();
	}

	chunk->m_chunkState = ChunkState::DECONSTRUCTING;
	delete chunk;
	m_activeChunks.erase(iter);

	// Update 2D Doubly Linked List
	iter = m_activeChunks.find(chunkCoords + IntVec2(1, 0));
	if (iter != m_activeChunks.end())
	{
		iter->second->m_westNeighbor = nullptr;
	}

	iter = m_activeChunks.find(chunkCoords + IntVec2(-1, 0));
	if (iter != m_activeChunks.end())
	{
		iter->second->m_eastNeighbor = nullptr;
	}

	iter = m_activeChunks.find(chunkCoords + IntVec2(0, 1));
	if (iter != m_activeChunks.end())
	{
		iter->second->m_northNeighbor = nullptr;
	}

	iter = m_activeChunks.find(chunkCoords + IntVec2(0, -1));
	if (iter != m_activeChunks.end())
	{
		iter->second->m_northNeighbor = nullptr;
	}

	UndirtyAllBlocksInChunk(chunk);
}

void World::DeactivateChunk(Chunk* chunkPtr)
{
	IntVec2& chunkCoords = chunkPtr->m_chunkCoords;
	std::map<IntVec2, Chunk*>::iterator iter;
	iter = m_activeChunks.find(chunkCoords);

	if (chunkPtr->m_needsSaving)
	{
		chunkPtr->SaveBlocksDataToFile();
	}

	UndirtyAllBlocksInChunk(chunkPtr);
	delete chunkPtr;
	m_activeChunks.erase(iter);

	// Update 2D Doubly Linked List
	iter = m_activeChunks.find(chunkCoords + IntVec2(1, 0));
	if (iter != m_activeChunks.end())
	{
		iter->second->m_westNeighbor = nullptr;
	}

	iter = m_activeChunks.find(chunkCoords + IntVec2(-1, 0));
	if (iter != m_activeChunks.end())
	{
		iter->second->m_eastNeighbor = nullptr;
	}

	iter = m_activeChunks.find(chunkCoords + IntVec2(0, 1));
	if (iter != m_activeChunks.end())
	{
		iter->second->m_northNeighbor = nullptr;
	}

	iter = m_activeChunks.find(chunkCoords + IntVec2(0, -1));
	if (iter != m_activeChunks.end())
	{
		iter->second->m_northNeighbor = nullptr;
	}
}

void World::ActivateNearestMissingChunkInPlayerRange()
{
	// search through the chess board to find the closest one to the player
	IntVec2 chunkStartCoords = IntVec2((m_playerChunkCoords.x - MAX_CHUNK_RADIUS_X), (m_playerChunkCoords.y - MAX_CHUNK_RADIUS_Y));
	IntVec2 chunkEndCoords = IntVec2((m_playerChunkCoords.x + MAX_CHUNK_RADIUS_X), (m_playerChunkCoords.y + MAX_CHUNK_RADIUS_Y));

	IntVec2 chunkCoordsNeedToActivate = BAD_CHUNK_COORDS;
	int minDistSqr = int(CHUNK_ACTIVATE_RANGE * CHUNK_ACTIVATE_RANGE);

	for (int i = chunkStartCoords.x; i <= chunkEndCoords.x; ++i)
	{
		for (int j = chunkStartCoords.y; j <= chunkEndCoords.y; ++j)
		{
			IntVec2 chunkCoords(i, j);
			int distSqr = chunkCoords.GetLengthSquaredToThisCoords(m_playerChunkCoords) * CHUNK_SIZE_X * CHUNK_SIZE_Y;
			if (distSqr < minDistSqr)
			{
				// if this chunk is not being generated or queued in the job system
				auto found = m_chunksBeingGeneratedOrLoaded.find(chunkCoords);
				if (found == m_chunksBeingGeneratedOrLoaded.end())
				{
					// this waiting to be activate chunk also need to be not in the activate list
					std::map<IntVec2, Chunk*>::iterator iter;
					iter = m_activeChunks.find(chunkCoords);

					if (iter == m_activeChunks.end())
					{
						chunkCoordsNeedToActivate = chunkCoords;
						minDistSqr = distSqr;
					}
				}

			}
		}
	}

	// if the result has been modified
	if (chunkCoordsNeedToActivate != BAD_CHUNK_COORDS)
	{
		RequestNewChunkGenerationJob(chunkCoordsNeedToActivate);
	}
}

void World::RequestNewChunkGenerationJob(IntVec2 chunkCoords)
{
	if (g_theJobSystem->GetNumQueuedJobs() <= MAX_QUEUEDJOBS_CHUNKGENERATION)
	{
		Chunk* chunk = new Chunk(chunkCoords);

		chunk->Startup();
		// CheckIfCaveWormStartsThisChunk(chunk);
	}
}

void World::ActivateNewChunk(Chunk* chunk)
{
	IntVec2 chunkCoords = chunk->m_chunkCoords;

	// Update 2D Doubly Linked List
	std::map<IntVec2, Chunk*>::iterator iter;

	iter = m_activeChunks.find(chunkCoords + IntVec2(1, 0));
	if (iter != m_activeChunks.end())
	{
		chunk->m_eastNeighbor = iter->second;
		chunk->m_eastNeighbor->m_westNeighbor = chunk;
	}

	iter = m_activeChunks.find(chunkCoords + IntVec2(-1, 0));
	if (iter != m_activeChunks.end())
	{
		chunk->m_westNeighbor = iter->second;
		chunk->m_westNeighbor->m_eastNeighbor = chunk;
	}

	iter = m_activeChunks.find(chunkCoords + IntVec2(0, 1));
	if (iter != m_activeChunks.end())
	{
		chunk->m_northNeighbor = iter->second;
		chunk->m_northNeighbor->m_southNeighbor = chunk;
	}

	iter = m_activeChunks.find(chunkCoords + IntVec2(0, -1));
	if (iter != m_activeChunks.end())
	{
		chunk->m_southNeighbor = iter->second;
		chunk->m_southNeighbor->m_northNeighbor = chunk;
	}

	m_activeChunks[chunkCoords] = chunk;
	chunk->m_chunkState = ChunkState::ACTIVE;

	LightInfluenceInitialization(chunk);
}

void World::RetrieveCompletedChunkGenerationJobAndActivate()
{
	ChunkGenerateJob* chunkGenerationJob = dynamic_cast<ChunkGenerateJob*>(g_theJobSystem->RetrieveCompletedJobs(nullptr));
	if (chunkGenerationJob)
	{
		m_chunksBeingGeneratedOrLoaded.erase(chunkGenerationJob->m_chunk->m_chunkCoords);
		ActivateNewChunk(chunkGenerationJob->m_chunk);
		delete chunkGenerationJob;
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void World::WorldInputControl()
{
	// deactivate all the chunks and reactivate
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		std::map<IntVec2, Chunk*>::iterator iter;
		for (iter = m_activeChunks.begin(); iter != m_activeChunks.end(); ++iter)
		{
			Chunk* chunk = iter->second;
			if (chunk->m_needsSaving)
			{
				chunk->SaveBlocksDataToFile();
			}

			delete chunk;
			chunk = nullptr;
		}

		m_activeChunks.clear();
	}

	if (g_theInput->WasKeyJustPressed('R'))
	{
		if (m_raycastIsLocked)
		{
			m_raycastIsLocked = false;
		}
		else
		{
			m_raycastIsLocked = true;
		}
	}
}

void World::UpdateOnScreenDisplayMessages()
{
	Vec2  controlAlignment = Vec2(0.f, .99f);
	Vec2  dataAlignment = Vec2(0.f, 0.91f);
	Vec2  timeAlignment = Vec2(0.f, 0.87f);
	Vec2  typeAlignment = Vec2(0.5f, 0.05f);
	float fontSize = 24.f;

	std::string controlInstruction = Stringf("WASD = fly horizontal, QE = fly vertical(SHIFT = fast), F1 = debug bounds, F8 = regenerate, LMB=dig below, RMB=place below\nNumKey 1 Select CobberStone, 2 select GlowStone, Press Y to pass time faster");
	DebugAddScreenText(controlInstruction, Vec2(0.f, 0.f), fontSize, controlAlignment, -1.f, Rgba8::Naples_Yellow, Rgba8::Naples_Yellow);

	// for the top of the line we are always drawing the position and orientation
	// player position
	// std::string playerPosition = Stringf("PlayerPosition: %.2f, %.2f, %.2f", g_debugPosition.x, g_debugPosition.y, g_debugPosition.z);
	// DebugAddScreenText(playerPosition, Vec2(0.f, 0.f), fontSize, dataAlignment, -1.f, );

	float		time;
	float		timeScale = g_theGameClock->GetTimeScale();
	int numChunks = int(m_activeChunks.size());
	int numBlocks = numChunks * CHUNK_BLOCKS_TOTAL;
	unsigned int numVerts = 0;

	std::map<IntVec2, Chunk*>::iterator iter;
	for (iter = m_activeChunks.begin(); iter != m_activeChunks.end(); ++iter)
	{
		Chunk* chunk = iter->second;
		if (chunk->m_vertexBuffer)
		{
			numVerts += unsigned int(chunk->m_vertexBuffer->m_size);
		}
	}

	if (g_theGameClock)
	{
		time = g_theGameClock->GetTotalSeconds();
		if (g_theGameClock->IsPaused())
		{
			m_FPS = "Paused";
			m_frameMS = "Paused";
			timeScale = 0.f;
		}
		else
		{
			if (m_dataTimer->HasPeroidElapsed())
			{
				int framePerSecond = int(1.f / g_theGameClock->GetDeltaSeconds());
				m_FPS = Stringf("%i", framePerSecond);
				m_frameMS = Stringf("%i", int(1000.f / float(framePerSecond)));

				m_dataTimer->Restart();
			}
		}
	}
	else
	{
		time = 0.f;
		timeScale = 0.f;
	}

	std::string num_Chunks_Blocks_Verts_frameMS_FPS = Stringf("NumChunks=%i / %i, NumBlocks=%i, NumVerts=%i, frameMS=%s (%s FPS)", numChunks, MAX_CHUNKS, numBlocks, numVerts, m_frameMS.c_str(), m_FPS.c_str());
	DebugAddScreenText(num_Chunks_Blocks_Verts_frameMS_FPS, Vec2(0.f, 0.f), fontSize, dataAlignment, -1.f, Rgba8::CYAN, Rgba8::CYAN);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	double NumDays = 0;
	float fractionOfDay = (float)modf(m_worldDay, &NumDays);
	std::string worldDay = Stringf("Days=%i, Fraction_day=%.2f", (int)NumDays, fractionOfDay);
	DebugAddScreenText(worldDay, Vec2(0.f, 0.f), fontSize, timeAlignment, -1.f, Rgba8::CYAN, Rgba8::CYAN);

	std::string buildingType = Stringf("Building Type: %s", g_theGame->m_player->m_buildingBlockDef.m_name.c_str());
	DebugAddScreenText(buildingType, Vec2(0.f, 0.f), fontSize, typeAlignment, -1.f, Rgba8::Naples_Yellow, Rgba8::Naples_Yellow);

	Vec2  jobsAlignment = Vec2(0.f, .5f);

	// display how many jobs are queued
	std::string numQueuedJobs = Stringf("QueuedJobs = %i", g_theJobSystem->GetNumQueuedJobs());
	DebugAddScreenText(numQueuedJobs, Vec2(0.f, 0.f), fontSize, jobsAlignment, -1.f, Rgba8::Naples_Yellow, Rgba8::Naples_Yellow);

	
}

// processes and propagates all dirty light blocks until none remain
void World::ProcessDirtyLighting()
{
	while (!m_dirtyLightBlockIters.empty())
	{
		ProcessNextDirtyLightBlock(m_dirtyLightBlockIters.front());
		m_dirtyLightBlockIters.pop_front();
	}
}

// only dirty the next block instead of the whole neighbor chunk
void World::DirtyNeighborChunkMeshWhenAnEdgeAirBlockLightIsProcessed(BlockIter& blockIter)
{
	if (blockIter.IsNonOpaque())
	{
		Chunk* chunkPtr = blockIter.m_chunk;
		IntVec3 coords = blockIter.m_chunk->GetlocalBlockCoordsForIndex(blockIter.m_blockIndex);

		Chunk* chunkNeighbor = chunkPtr->m_westNeighbor;
		if (coords.x == 0 && chunkNeighbor)
		{
			MarkLightingDirty(blockIter.GetWestNeighbor());
			chunkNeighbor->m_isMeshDirty = true;
		}

		chunkNeighbor = chunkPtr->m_eastNeighbor;
		if (coords.x == (CHUNK_SIZE_X - 1) && chunkNeighbor)
		{
			MarkLightingDirty(blockIter.GetEastNeighbor());
			chunkNeighbor->m_isMeshDirty = true;
		}

		chunkNeighbor = chunkPtr->m_southNeighbor;
		if (coords.y == 0 && chunkNeighbor)
		{
			MarkLightingDirty(blockIter.GetSouthNeighbor());
			chunkNeighbor->m_isMeshDirty = true;
		}
		
		chunkNeighbor = chunkPtr->m_northNeighbor;
		if (coords.y == (CHUNK_SIZE_Y - 1) && chunkNeighbor)
		{
			MarkLightingDirty(blockIter.GetNorthNeighbor());
			chunkNeighbor->m_isMeshDirty = true;
		}
	}
}

// adds a BlockIterator to the back of the dirty light queue IF that block is not already flagged as dirty (BLOCK_BIT_IS_LIGHT_DIRTY), 
// and also sets that flag
void World::MarkLightingDirty(BlockIter blockIter)
{
	if (blockIter.GetBlock())
	{
		if (!blockIter.GetBlock()->IsLightDirty())
		{
			blockIter.GetBlock()->SetLightDirty(true);
			m_dirtyLightBlockIters.push_back(blockIter);
		}
	}
}

// recomputes its lighting (see Dirty Light Processing, below), and clears its BLOCK_BIT_IS_LIGHT_DIRTY flag
void World::ProcessNextDirtyLightBlock(BlockIter blockIter)
{
	blockIter.GetBlock()->SetLightDirty(false); // clear flag

	// calculate value from the neighbor first
	int highestNeighborIndoorLight = 0;
	int highestNeighborOutdoorLight = 0;

	if (blockIter.IsNonOpaque())
	{
		// east
		BlockIter neighborBlockIter = blockIter.GetEastNeighbor();
		int neighborIndoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
		int neighborOutdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
		highestNeighborIndoorLight = GetMax(neighborIndoorLight, highestNeighborIndoorLight);
		highestNeighborOutdoorLight = GetMax(neighborOutdoorLight, highestNeighborOutdoorLight);

		// west
		neighborBlockIter = blockIter.GetWestNeighbor();
		neighborIndoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
		neighborOutdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
		highestNeighborIndoorLight = GetMax(neighborIndoorLight, highestNeighborIndoorLight);
		highestNeighborOutdoorLight = GetMax(neighborOutdoorLight, highestNeighborOutdoorLight);

		// south
		neighborBlockIter = blockIter.GetSouthNeighbor();
		neighborIndoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
		neighborOutdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
		highestNeighborIndoorLight = GetMax(neighborIndoorLight, highestNeighborIndoorLight);
		highestNeighborOutdoorLight = GetMax(neighborOutdoorLight, highestNeighborOutdoorLight);

		// North
		neighborBlockIter = blockIter.GetNorthNeighbor();
		neighborIndoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
		neighborOutdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
		highestNeighborIndoorLight = GetMax(neighborIndoorLight, highestNeighborIndoorLight);
		highestNeighborOutdoorLight = GetMax(neighborOutdoorLight, highestNeighborOutdoorLight);

		// Upper
		neighborBlockIter = blockIter.GetUpperLevelNeighbor();
		neighborIndoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
		neighborOutdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
		highestNeighborIndoorLight = GetMax(neighborIndoorLight, highestNeighborIndoorLight);
		highestNeighborOutdoorLight = GetMax(neighborOutdoorLight, highestNeighborOutdoorLight);

		// Lower
		neighborBlockIter = blockIter.GetLowerLevelNeighbor();
		neighborIndoorLight = neighborBlockIter.GetBlock()->GetIndoorLighting();
		neighborOutdoorLight = neighborBlockIter.GetBlock()->GetOutdoorLighting();
		highestNeighborIndoorLight = GetMax(neighborIndoorLight, highestNeighborIndoorLight);
		highestNeighborOutdoorLight = GetMax(neighborOutdoorLight, highestNeighborOutdoorLight);
	}

	// define that one block's light equals to its highest neighbor's value -1
	uint8_t selfIndoorLight = blockIter.GetBlock()->GetBlockDef().m_indoorLight;
	uint8_t selfOutdoorLight = (blockIter.GetBlock()->IsBlockSky()) ? MAX_LIGHT_VALUE : 0;
	uint8_t correctIndoorLight = (uint8_t)GetMax((highestNeighborIndoorLight - 1), selfIndoorLight);
	uint8_t correctOutdoorLight = (uint8_t)GetMax((highestNeighborOutdoorLight - 1), selfOutdoorLight); // underflow of uint_8

	// if the lighting is correct, do not infect its neighbors
	// if it is not correct, infect all its neighbors
	uint8_t currentIndoorLight = blockIter.GetBlock()->GetIndoorLighting();
	uint8_t currentOutdoorLight = blockIter.GetBlock()->GetOutdoorLighting();

	if (currentIndoorLight != correctIndoorLight || currentOutdoorLight != correctOutdoorLight)
	{
		blockIter.GetBlock()->SetIndoorLightingInfulence(correctIndoorLight);
		blockIter.GetBlock()->SetOutdoorLightingInfulence(correctOutdoorLight);
		MarkNeighborLightingDirtyIfTheyAreNotOpaque(blockIter); // if the block is on the edges, it will tell nearby chunk the light is dirty?
		blockIter.m_chunk->m_isMeshDirty = true; // todo: maybe just re add verts and create buffer instead of go through all the update process?
	}
}

// called on neighbors when my light influences change
// e.g. player build a glow stone, let non-opaque neighbor - air, mark light dirty
void World::MarkNeighborLightingDirtyIfTheyAreNotOpaque(BlockIter blockIter)
{
	// East
	BlockIter neighborBlockIter = blockIter.GetEastNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.IsNonOpaque())
		{
			MarkLightingDirty(neighborBlockIter);
		}
	}
	// west
	neighborBlockIter = blockIter.GetWestNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.IsNonOpaque())
		{
			MarkLightingDirty(neighborBlockIter);
		}
	}
	// south
	neighborBlockIter = blockIter.GetSouthNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.IsNonOpaque())
		{
			MarkLightingDirty(neighborBlockIter);
		}
	}
	// north
	neighborBlockIter = blockIter.GetNorthNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.IsNonOpaque())
		{
			MarkLightingDirty(neighborBlockIter);
		}
	}	
	// upper
	neighborBlockIter = blockIter.GetUpperLevelNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.IsNonOpaque())
		{
			MarkLightingDirty(neighborBlockIter);
		}
	}	
	// lower
	neighborBlockIter = blockIter.GetLowerLevelNeighbor();
	if (neighborBlockIter.GetBlock())
	{
		if (neighborBlockIter.IsNonOpaque())
		{
			MarkLightingDirty(neighborBlockIter);
		}
	}
}

// The result should be that all blocks have zero indoor light influence, and direct-sky blocks (only) have a max (15) outdoor influence; 
// all non-sky blocks have zero outdoor light influence.
// Light - emitting blocks(e.g.glowstone) are dirty, with(incorrect) zero light influence.
// Edge boundary air blocks with chunk neighbors are dirty, as are non - sky air neighbors of sky blocks.
void World::LightInfluenceInitialization(Chunk* chunkPtr)
{
	for (int x = 0; x < CHUNK_SIZE_X; ++x)
	{
		for (int y = 0; y < CHUNK_SIZE_Y; ++y)
		{
			bool connectedToSky = true;

			for (int z = (CHUNK_SIZE_Z - 1); z >= 0; --z)
			{
				// Loop through each block in the chunk; if it has a block type that emits light, mark it dirty
				int index = chunkPtr->GetIndexForLocalCoordinates(IntVec3(x, y, z));
				BlockDef def = chunkPtr->m_blocks[index].GetBlockDef();
				Block& block = chunkPtr->m_blocks[index];
				BlockIter blockIter = BlockIter(chunkPtr, index);

				if (def.m_isEmissive)
				{
					MarkLightingDirty(blockIter);
				}

				// Descend each column downward from the top, flagging blocks as SKY, stopping at first opaque
				if (connectedToSky && !def.m_isOpaque)
				{
					block.SetIsBlockSky(true);
				}
				else
				{
					connectedToSky = false;
					block.SetIsBlockSky(false);
				}

				// Mark non-opaque boundary blocks touching any existing neighboring chunk (NSEW) as dirty
				if (block.IsBlockSky())
				{
					if (x == 0 && chunkPtr->m_westNeighbor)
					{
						if (blockIter.GetWestNeighbor().IsNonOpaque())
						{
							MarkLightingDirty(blockIter);
						}
					}
					else if (x == (CHUNK_SIZE_X - 1) && chunkPtr->m_eastNeighbor)
					{
						if (blockIter.GetEastNeighbor().IsNonOpaque())
						{
							MarkLightingDirty(blockIter);
						}
					}
					else if (y == 0 && chunkPtr->m_southNeighbor)
					{
						if (blockIter.GetSouthNeighbor().IsNonOpaque())
						{
							MarkLightingDirty(blockIter);
						}
					}
					else if (y == (CHUNK_SIZE_Y - 1) && chunkPtr->m_northNeighbor)
					{
						if (blockIter.GetNorthNeighbor().IsNonOpaque())
						{
							MarkLightingDirty(blockIter);
						}
					}
				}
			}
		}
	}

	for (int x = 0; x < CHUNK_SIZE_X; ++x)
	{
		for (int y = 0; y < CHUNK_SIZE_Y; ++y)
		{
			for (int z = (CHUNK_SIZE_Z - 1); z >= 0; --z)
			{
				int index = chunkPtr->GetIndexForLocalCoordinates(IntVec3(x, y, z));
				BlockDef def = chunkPtr->m_blocks[index].GetBlockDef();
				Block& block = chunkPtr->m_blocks[index];
				BlockIter blockIter = BlockIter(chunkPtr, index);

				// Descend each column again until first opaque; 
				// set each sky block’s outdoor light influence to maximum (15)
				// AND mark its non-opaque non-sky horizontal (NSEW only) neighbors dirty
				// e.g like the air blocks under the tree
				if (block.IsBlockSky())
				{
					// east
					BlockIter neighborBlockIter = blockIter.GetEastNeighbor();
					if (neighborBlockIter.GetBlock())
					{
						if (neighborBlockIter.IsNonOpaqueAndNonSky())
						{
							MarkLightingDirty(neighborBlockIter);
						}
					}					
					// west
					neighborBlockIter = blockIter.GetWestNeighbor();
					if (neighborBlockIter.GetBlock())
					{
						if (neighborBlockIter.IsNonOpaqueAndNonSky())
						{
							MarkLightingDirty(neighborBlockIter);
						}
					}					
					// south
					neighborBlockIter = blockIter.GetSouthNeighbor();
					if (neighborBlockIter.GetBlock())
					{
						if (neighborBlockIter.IsNonOpaqueAndNonSky())
						{
							MarkLightingDirty(neighborBlockIter);
						}
					}					
					// north
					neighborBlockIter = blockIter.GetNorthNeighbor();
					if (neighborBlockIter.GetBlock())
					{
						if (neighborBlockIter.IsNonOpaqueAndNonSky())
						{
							MarkLightingDirty(neighborBlockIter);
						}
					}
				}

			}
		}
	}
}

// scans the dirty queue, removes all blocks from that chunk
void World::UndirtyAllBlocksInChunk(Chunk* chunkPtr)
{
	std::deque<BlockIter>::iterator iter;
	for (iter = m_dirtyLightBlockIters.begin(); iter != m_dirtyLightBlockIters.end();)
	{
		if (iter->m_chunk == chunkPtr)
		{
			// The erase() method returns a new (valid) iterator that points to the next element after the deleted one.
			iter = m_dirtyLightBlockIters.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

void World::UpdateSimplerMinerWorldShader()
{
	// get the correct fog / sky color according to the time of the day
	double NumDays = 0;
	float fractionOfDay = (float)modf(m_worldDay, &NumDays);
	Rgba8 fogDayColor;
	Rgba8 outdoorLight;
	Rgba8 indoorLight = m_indoorLight;
	// between 6pm and 6am
	if (fractionOfDay < (6.f * HOUR_FRACTION_DAY) || fractionOfDay > (18.f * HOUR_FRACTION_DAY))
	{
		fogDayColor = m_nightSKyFogColor;
		outdoorLight = m_nightSKyFogColor;
	}
	else
	{
		float fractionToNoon = std::fabsf(fractionOfDay - 0.5f);
		float fraction = RangeMap(fractionToNoon, 0.f, 6.f * HOUR_FRACTION_DAY, 0.f, 1.f);
		fogDayColor = InterpolateRGBA(m_noonSkyFogColor, m_nightSKyFogColor, fraction);
		outdoorLight = InterpolateRGBA(Rgba8::WHITE, m_nightSKyFogColor, fraction);
	}

	// lighting and outdoor light
	m_lightningStrength = Compute1dPerlinNoise(m_worldDay, 0.006f, 9); // how often to get lightning, the smaller, the easier
	m_lightningStrength = RangeMapClamped(m_lightningStrength, 0.6f, 0.9f, 0.f, 1.f); // how easy to get the lighting, higher value, harder to get

	fogDayColor = InterpolateRGBA(fogDayColor, Rgba8::WHITE, m_lightningStrength);
	outdoorLight = InterpolateRGBA(outdoorLight, Rgba8::WHITE, m_lightningStrength);

	// indoor glow light
	m_glowStrength = Compute1dPerlinNoise(m_worldDay, 0.001f, 5);
	m_glowStrength = RangeMapClamped(m_glowStrength, -1.f, 1.f, 0.8f, 1.f);

	m_gpuShaderData->m_fogColor = Vec4(fogDayColor);
	m_gpuShaderData->m_cameraPos = Vec4(g_theGame->m_player->m_position);
	m_gpuShaderData->m_indoorColor = (Vec4(indoorLight) * m_glowStrength);
	m_gpuShaderData->m_outdoorColor = Vec4(outdoorLight);
}

void World::BindSimplerMinerWorldShaderData() const
{
	g_theRenderer->CopyCPUToGPU(m_gpuShaderData, sizeof(SimpleMinerGPUData), m_lighting_Fog_CBO);
	g_theRenderer->BindConstantBuffer(k_lightingFogConstantsSlot, m_lighting_Fog_CBO);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void World::CheckIfCaveWormStartsThisChunk(Chunk* chunkPtr)
{
	IntVec2 globalCoords = chunkPtr->m_chunkCoords;
	float caveNoise = Get2dNoiseZeroToOne(globalCoords.x, globalCoords.y, m_seed);

	// calculate all neighbors tree including this point noise 3x3 to see if this one is the highest one is this one
	float maxCaveNoise = 0.f;
	for (int posX = (globalCoords.x - CAVE_COVER_CHUNK_TIMES); posX < (globalCoords.x + CAVE_COVER_CHUNK_TIMES); ++posX)
	{
		for (int posY = (globalCoords.y - CAVE_COVER_CHUNK_TIMES); posY < (globalCoords.y + CAVE_COVER_CHUNK_TIMES); ++posY)
		{
			float neighborNoise = Get2dNoiseZeroToOne(posX, posY, m_seed);
			if (neighborNoise > maxCaveNoise)
			{
				maxCaveNoise = neighborNoise;
			}
		}
	}

	if (caveNoise == maxCaveNoise)
	{
		GeneratingPerlinWormCaves(chunkPtr);
	}
}

void World::GeneratingPerlinWormCaves(Chunk* chunkPtr)
{
	// locate the origin block of the worm from the chunk
	IntVec2 globalCoords = chunkPtr->m_chunkCoords;
	float posNoise = 0.5f + 0.5f * Get2dNoiseZeroToOne(globalCoords.x, globalCoords.y, m_seed);
	IntVec2 caveStartPos(int(posNoise * float(CHUNK_SIZE_X)), int(posNoise * float(CHUNK_SIZE_Y)));
	IntVec2 biomeCoords = chunkPtr->GetBiomeCoordsByLocalCoords(caveStartPos);
	int biomeIndex = chunkPtr->GetBiomeIndexByTreeCoords(biomeCoords);
	int terrainHeight = chunkPtr->m_terrainHeightZ[biomeIndex];
	IntVec3 caveOriginPos(caveStartPos.x, caveStartPos.y, int(posNoise * float(terrainHeight)));

	//
	// float caveWormLength = 0.f;
	// float pitchNoise = Compute1dPerlinNoise(caveWormLength, 1.f, 5, 0.5f, 2.f, true, m_seed);
	// float pitchDeltaDegrees = RangeMapClamped(pitchNoise, -1.f, 1.f, CAVE_PITCH_MAX * (-1.f), CAVE_PITCH_MAX);
	// 
	// float yawNoise = Compute1dPerlinNoise(caveWormLength, 1.f, 5, 0.5f, 2.f, true, (m_seed + 1));
	// float yawDeltaDegrees = RangeMapClamped(yawNoise, -1.f, 1.f, CAVE_YAW_MAX * (-1.f), CAVE_YAW_MAX);

}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void World::DigBlock()
{
	if (m_playerAimingRaycastResult.m_didImpact)
	{
		UpdatePlayerLocatedCoords();

		// get the ptr to the chunk that the player is current at
		m_playerAimingRaycastResult.m_aimedBlockIter.m_chunk->DigBlock(m_playerAimingRaycastResult.m_aimedBlockIter.m_blockIndex);

		// put it into the light deque
		BlockIter digIter = m_playerAimingRaycastResult.m_aimedBlockIter;
		IntVec3 aimedBlockCoords = m_playerAimingRaycastResult.m_aimedBlockIter.m_chunk->GetlocalBlockCoordsForIndex(digIter.m_blockIndex);
		MarkLightingDirty(digIter);
		// if this is an air block and it is at the edge of the chunk
		// we need to notify the neighbor chunk that he need to rebuild
		// otherwise the block will next to this air block will not have any mesh
		DirtyNeighborChunkMeshWhenAnEdgeAirBlockLightIsProcessed(digIter);

		// If the block ABOVE is flagged SKY, descend downward starting here until you reach the first opaque block directly below it
		// flagging each non-opaque block descended as SKY and dirtying it
		// Example: “break through?the roof of a cave, and a beam of sunlight streams in down to the floor

		BlockIter blockAboveIter = digIter.GetUpperLevelNeighbor();
		if (blockAboveIter.GetBlock()->IsBlockSky())
		{
			for (int z = (aimedBlockCoords.z - 1); z >= 0; --z)
			{
				IntVec3 blockBelow(aimedBlockCoords.x, aimedBlockCoords.y, z);
				int blockBelowIndex = digIter.m_chunk->GetIndexForLocalCoordinates(blockBelow);
				BlockIter blockBelowIter(digIter.m_chunk, blockBelowIndex);
				if (!blockBelowIter.GetBlock()->IsOpaque())
				{
					blockBelowIter.GetBlock()->SetIsBlockSky(true);
					MarkLightingDirty(blockBelowIter);
				}
				else
				{
					break;
				}
			}
		}
	}
}

void World::BuildPlayerBlock()
{
	if (m_playerAimingRaycastResult.m_didImpact)
	{
		UpdatePlayerLocatedCoords();

		BlockIter aimedBlockIter = m_playerAimingRaycastResult.m_aimedBlockIter;
		BlockIter placeBlockIter;
		Vec3 normal = m_playerAimingRaycastResult.m_impactNormal;
		// based on which direction the raycast normal it is point to, change the index
		if (normal == Vec3(0.f, 0.f, 1.f)) // top
		{
			placeBlockIter = aimedBlockIter.GetUpperLevelNeighbor();
		}
		else if (normal == Vec3(0.f, 0.f, -1.f)) // bottom
		{
			placeBlockIter = aimedBlockIter.GetLowerLevelNeighbor();
		}
		else if (normal == Vec3(-1.f, 0.f, 0.f)) // left
		{
			placeBlockIter = aimedBlockIter.GetWestNeighbor();
		}
		else if (normal == Vec3(1.f, 0.f, 0.f)) // right
		{
			placeBlockIter = aimedBlockIter.GetEastNeighbor();
		}
		else if (normal == Vec3(0.f, -1.f, 0.f)) // back
		{
			placeBlockIter = aimedBlockIter.GetSouthNeighbor();
		}
		else if (normal == Vec3(0.f, 1.f, 0.f)) // front
		{
			placeBlockIter = aimedBlockIter.GetNorthNeighbor();
		}


		aimedBlockIter.m_chunk->SetBlock(placeBlockIter.m_blockIndex, g_theGame->m_player->m_buildingBlockDef);

		// put it into the light deque
		MarkLightingDirty(placeBlockIter);

		// clearing all SKY flags directly below (until you reach opaque) and dirtying their lighting
		// If the (air) block replaced was flagged as SKY ?and the new block is opaque ?clear the SKY flag and descend downward, 
		// Example: “plug up" a vertical mineshaft, cutting off sunlight and plunging the cave into darkness
		if (placeBlockIter.GetBlock()->IsBlockSky() && !placeBlockIter.IsNonOpaque())
		{
			placeBlockIter.GetBlock()->SetIsBlockSky(false);

			BlockIter skyBlockIter = placeBlockIter.GetLowerLevelNeighbor();
			while (skyBlockIter.GetBlock() && skyBlockIter.GetBlock()->IsBlockSky())
			{
				skyBlockIter.GetBlock()->SetIsBlockSky(false);
				MarkLightingDirty(skyBlockIter);
				skyBlockIter = skyBlockIter.GetLowerLevelNeighbor();
			}
		}
	}
}
 
void World::ShootRaycastForCollisionTest(float rayDist)
{
	Vec3 rayStart = g_theGame->m_player->m_position;
	Vec3 rayDisp = Vec3(rayDist, 0.f, 0.f);
	Vec3 rayFwdNormal = g_theGame->m_player->GetModelMatrix().TransformVectorQuantity3D(rayDisp).GetNormalized();
	// Vec3 rayEnd = rayStart + rayFwdNormal * rayDist;
	// DebugAddWorldLine(rayStart, rayEnd, 0.01f, 0.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);

	m_playerAimingRaycastResult = FastRaycastForVoxelGrids(rayStart, rayFwdNormal, rayDist);

	if (m_playerAimingRaycastResult.m_didImpact)
	{
		DebugAddWorldPoint(m_playerAimingRaycastResult.m_impactPos, 0.06f, 0.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);
		Vec3 arrowTip = m_playerAimingRaycastResult.m_impactPos + m_playerAimingRaycastResult.m_impactNormal * 0.3f;
		DebugAddWorldArrow(m_playerAimingRaycastResult.m_impactPos, arrowTip, 0.03f, 0.f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::X_RAY);
	}

	if (!m_raycastIsLocked) // when the raycast is not locked, remember its info
	{
		m_lockedRaycastInfo = m_playerAimingRaycastResult;
	}
}

SimpleMinerRaycastResult World::FastRaycastForVoxelGrids(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist)
{
	// Initialization setup
	Vec3 rayEnd = rayStart + rayFwdNormal * rayDist;
	bool impactSolidTile = false;

	// if it misses, we will use this result
	SimpleMinerRaycastResult missResult;
	missResult.m_didImpact = false;
	missResult.m_impactDist = rayDist;
	missResult.m_impactPos = rayEnd;
	missResult.m_impactNormal = rayFwdNormal;

	missResult.m_didExit = false;
	missResult.m_travelDistInShape = 0.f;
	missResult.m_exitPos = rayStart;
	missResult.m_exitNormal = rayFwdNormal;

	missResult.m_rayFwdNormal = rayFwdNormal;
	missResult.m_rayStartPos = rayStart;
	missResult.m_rayDist = rayDist;

	// if it hit, we will use this result
	SimpleMinerRaycastResult hitResult;

	hitResult.m_rayFwdNormal = rayFwdNormal;
	hitResult.m_rayStartPos = rayStart;
	hitResult.m_rayDist = rayDist;

	// we are projecting the raycast on ground to discuss the situation
	// So for the rayDist, we also need to project it onto the ground
	Vec3 rayXYNormal = Vec3(rayFwdNormal.x, rayFwdNormal.y, 0.f).GetNormalized();
	Vec3 rayYZNormal = Vec3(0.f, rayFwdNormal.y, rayFwdNormal.z).GetNormalized();
	if (rayFwdNormal.GetLength() == 0.f)
	{
		ERROR_AND_DIE("The input raycast forward normal length is 0.f");
	}
	float rayXYDist = rayDist * (Vec3(rayFwdNormal.x, rayFwdNormal.y, 0.f).GetLength() / rayFwdNormal.GetLength());
	float rayYZDist = rayDist * (Vec3(0.f, rayFwdNormal.y, rayFwdNormal.z).GetLength() / rayFwdNormal.GetLength());

	// check if the raycast starts inside a solid block
	IntVec3 blockCoords = m_playerLocalBlockCoords;
	int blockIndex = m_playerLocatedChunk->GetIndexForLocalCoordinates(m_playerLocalBlockCoords);
	BlockIter blockIter = BlockIter(m_playerLocatedChunk, blockIndex);
	Block* blockPtr = blockIter.GetBlock();

	if (blockPtr)
	{
		if (blockPtr->GetBlockDef().m_isSolid)
		{
			hitResult.m_didImpact = true;
			hitResult.m_impactDist = 0.f;
			hitResult.m_impactPos = rayStart;
			hitResult.m_impactNormal = rayFwdNormal * (-1.f);

			hitResult.m_didExit = false;
			hitResult.m_travelDistInShape = 0.f;
			hitResult.m_exitPos = rayStart;
			hitResult.m_exitNormal = rayFwdNormal;

			return hitResult;
		}
	}
	 
	// Get first tile and start pos
	Vec3 currentPos = rayStart;

	// calculate which grid wall the raycast is going to hit in X and Y for the next one
	int numXWall = 0;
	int numYWall = 0;
	int numZWall = 0;
	// and we are also calculate the fraction from start to the wall impact in X and Y to rayDist for comparison
	bool  outOfDistance = false;
	float tx = 1.f;  // max fraction in X direction
	float ty = 1.f;  // max fraction in Y direction
	float tz = 1.f;  // max fraction in Z direction

	while (!outOfDistance && !impactSolidTile) // if the testing point is not out of distance, we will loop through
	{
		tx = 1.f;
		ty = 1.f;
		tz = 1.f;

		// for marching each grid forward, we'll get the time travel in X and Y to see which is sooner
		// but before that, if the raycast is vertical or horizontal, we need to avoid case which divides 0.f
		if (rayFwdNormal.x == 0.f) // vertical raycast
		{
			// only need to calculate in Y direction
			if (rayFwdNormal.y > 0.f)
			{
				numYWall = (int)floor(currentPos.y + 1.0f);
				ty = (numYWall - rayStart.y) / (rayXYNormal.y * rayXYDist);
			}
			else if (rayFwdNormal.y < 0.f)
			{
				numYWall = (int)ceil(currentPos.y - 1.0);
				ty = (numYWall - rayStart.y) / (rayXYNormal.y * rayXYDist);
			}
		}
		else if (rayFwdNormal.y == 0.f) // horizontal raycast
		{
			// only need to calculate in X direction
			if (rayFwdNormal.x > 0.f)
			{
				numXWall = (int)floor(currentPos.x + 1.0f);
				tx = (numXWall - rayStart.x) / (rayXYNormal.x * rayXYDist);
			}
			else if (rayFwdNormal.x < 0.f)
			{
				numXWall = (int)ceil(currentPos.x - 1.0f);
				tx = (numXWall - rayStart.x) / (rayXYNormal.x * rayXYDist);
			}
		}
		else if (rayFwdNormal.z == 0.f) // Upwards and downwards raycast
		{
			// only need to calculate in Z direction
			if (rayFwdNormal.z > 0.f)
			{
				numZWall = (int)floor(currentPos.z + 1.0f);
				tz = (numZWall - rayStart.z) / (rayYZNormal.z * rayYZDist);
			}
			else if (rayFwdNormal.x < 0.f)
			{
				numXWall = (int)ceil(currentPos.x - 1.0f);
				tz = (numZWall - rayStart.z) / (rayYZNormal.z * rayYZDist);
			}
		}
		else // we need to calculate both x and y direction
		{
			// calculate in X direction
			if (rayFwdNormal.x > 0.f)
			{
				numXWall = (int)floor(currentPos.x + 1.0f);
				tx = (numXWall - rayStart.x) / (rayXYNormal.x * rayXYDist);
			}
			else if (rayFwdNormal.x < 0.f)
			{
				numXWall = (int)ceil(currentPos.x - 1.0f);
				tx = (numXWall - rayStart.x) / (rayXYNormal.x * rayXYDist);
			}
			// calculate in Y direction
			if (rayFwdNormal.y > 0.f)
			{
				numYWall = (int)floor(currentPos.y + 1.0f); // -2.00000048
				ty = (numYWall - rayStart.y) / (rayXYNormal.y * rayXYDist);
			}
			else if (rayFwdNormal.y < 0.f)
			{
				numYWall = (int)ceil(currentPos.y - 1.0f);
				ty = (numYWall - rayStart.y) / (rayXYNormal.y * rayXYDist);
			}
			// calculate in Z direction
			if (rayFwdNormal.z > 0.f)
			{
				numZWall = (int)floor(currentPos.z + 1.0f);
				tz = (numZWall - rayStart.z) / (rayYZNormal.z * rayYZDist);
			}
			else if (rayFwdNormal.z < 0.f)
			{
				numZWall = (int)ceil(currentPos.z - 1.0f);
				tz = (numZWall - rayStart.z) / (rayYZNormal.z * rayYZDist);
			}
		}

		// compare the three and take the quickest one
		// and uses the time to calculate the new impact pos
		Vec3 impactPos;
		impactSolidTile = false;

		if (tx < ty && tx < tz)
		{
			if (rayFwdNormal.x >= 0.f)
			{
				blockIter = blockIter.GetEastNeighbor();
				++blockCoords.x;
			}
			else
			{
				blockIter = blockIter.GetWestNeighbor();
				--blockCoords.x;
			}
			currentPos = rayStart + rayFwdNormal * rayDist * tx;
			currentPos.x = float(round(currentPos.x));

			Block* block = blockIter.GetBlock();

			if (tx <= 1.f && tx >= 0.f && block) // hit
			{
				if (block->GetBlockDef().m_isSolid)
				{
					hitResult.m_didImpact = true;
					hitResult.m_impactDist = rayDist * tx;
					hitResult.m_impactPos = currentPos;
					hitResult.m_aimedBlockIter = blockIter;				     
					hitResult.m_impactNormal = Vec3(rayFwdNormal.x * (-1.f), 0.f, 0.f).GetNormalized(); // opposite of the raycast
					return hitResult;
				}
				else // miss
				{
					// check if the ray is out of ray dist
					tx > 1.f ? outOfDistance = true : outOfDistance = false;
				}
			}
			else // miss
			{
				// check if the ray is out of ray dist
				tx > 1.f ? outOfDistance = true : outOfDistance = false;
			}
		}
		else if (ty < tx && ty < tz) // take step towards y
		{
			if (rayFwdNormal.y >= 0.f)
			{
				blockIter = blockIter.GetNorthNeighbor();
				++blockCoords.y; 
			}
			else
			{
				blockIter = blockIter.GetSouthNeighbor();
				--blockCoords.y;
			}
			//currentTile.y = numYWall;
			currentPos = rayStart + rayFwdNormal * rayDist * ty;
			currentPos.y = float(round(currentPos.y));

			Block* block = blockIter.GetBlock();

			if (ty <= 1.f && ty >= 0.f && block) // hit
			{
				if (block->GetBlockDef().m_isSolid)
				{
					hitResult.m_didImpact = true;
					hitResult.m_impactDist = rayDist * ty;
					hitResult.m_impactPos = currentPos;
					hitResult.m_aimedBlockIter = blockIter;
					hitResult.m_impactNormal = Vec3(0.f, rayFwdNormal.y * (-1.f), 0.f).GetNormalized(); // the impact is in Y direction, facing the raycast
					return hitResult;
				}
				else // miss
				{
					ty > 1.f ? outOfDistance = true : outOfDistance = false;
				}
			}
			else // miss
			{
				ty > 1.f ? outOfDistance = true : outOfDistance = false;
			}
		}
		else // take step towards z
		{
			if (rayFwdNormal.z >= 0.f)
			{
				blockIter = blockIter.GetUpperLevelNeighbor();
				++blockCoords.z;
			}
			else
			{
				blockIter = blockIter.GetLowerLevelNeighbor();
				--blockCoords.z;
			}
			//currentTile.y = numYWall;
			currentPos = rayStart + rayFwdNormal * rayDist * tz;
			currentPos.z = float(round(currentPos.z));

			Block* block = blockIter.GetBlock();

			if (tz <= 1.f && tz >= 0.f && block) // hit
			{
				if (block->GetBlockDef().m_isSolid)
				{
					hitResult.m_didImpact = true;
					hitResult.m_impactDist = rayDist * tz;
					hitResult.m_impactPos = currentPos;
					hitResult.m_aimedBlockIter = blockIter;
					hitResult.m_impactNormal = Vec3(0.f, 0.f, rayFwdNormal.z * (-1.f)).GetNormalized(); // the impact is in Z direction, facing the raycast
					return hitResult;
				}
				else // miss
				{
					tz > 1.f ? outOfDistance = true : outOfDistance = false;
				}
			}
			else // miss
			{
				tz > 1.f ? outOfDistance = true : outOfDistance = false;
			}
		}
	}
		
	return missResult;
}

void World::RenderLockedRaycastForDebug() const
{
	std::vector <Vertex_PCU> debugVerts_depthOn;
	std::vector <Vertex_PCU> debugVerts_depthOff;

	float radius = 0.05f;
	Vec3 arrowStartPos = m_lockedRaycastInfo.m_rayStartPos + m_lockedRaycastInfo.m_rayFwdNormal * m_lockedRaycastInfo.m_impactDist * 0.9f;
	Vec3 rayEndPos = m_lockedRaycastInfo.m_rayStartPos + m_lockedRaycastInfo.m_rayFwdNormal * m_lockedRaycastInfo.m_rayDist;
	if (m_lockedRaycastInfo.m_didImpact)
	{
		// for raycast
		AddVertsForCylinder3D(debugVerts_depthOn, m_lockedRaycastInfo.m_rayStartPos, arrowStartPos, radius * 1.f, Rgba8::RED);
		AddVertsForCone3D(debugVerts_depthOn, arrowStartPos, m_lockedRaycastInfo.m_impactPos, radius * 2.f, Rgba8::RED);

		AddVertsForCylinder3D(debugVerts_depthOff, m_lockedRaycastInfo.m_rayStartPos, rayEndPos, radius * .5f, Rgba8::GRAY);

		// impact sphere
		AddVertsForSphere3D(debugVerts_depthOff, m_lockedRaycastInfo.m_impactPos, radius * 2.f, Rgba8::WHITE, AABB2::ZERO_TO_ONE);

		// for impact normal
		Vec3 impactNormalTip = m_lockedRaycastInfo.m_impactPos + m_lockedRaycastInfo.m_impactNormal * 1.f;
		AddVertsForCylinder3D(debugVerts_depthOff, m_lockedRaycastInfo.m_impactPos, impactNormalTip, radius, Rgba8::YELLOW);
		AddVertsForCone3D(debugVerts_depthOff, impactNormalTip, (impactNormalTip + m_lockedRaycastInfo.m_impactNormal * 0.5f), radius * 2.5f, Rgba8::YELLOW);
	}
	else
	{
		AddVertsForCylinder3D(debugVerts_depthOn, m_lockedRaycastInfo.m_rayStartPos, arrowStartPos, radius * 1.f, Rgba8::GREEN);
		AddVertsForCone3D(debugVerts_depthOn, arrowStartPos, rayEndPos, radius * 2.f, Rgba8::GREEN);
	}

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->DrawVertexArray((int)debugVerts_depthOff.size(), debugVerts_depthOff.data());

	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->DrawVertexArray((int)debugVerts_depthOn.size(), debugVerts_depthOn.data());
}

void World::UpdateTime()
{
	// Time of day is the fractional [0,1) part of world-time, where .0=midnight, .25=dawn (6am), .5=noon, .75=dusk (6pm).  
	// Therefore, a world time of 5.27 is just after dawn on the 6th day.
	float worldTimeScale = 0.f;
	if (g_theInput->IsKeyDown('Y'))
	{
		worldTimeScale = m_fastWorldTimeScale;
	}
	else
	{
		worldTimeScale = m_normalWorldTimeScale;
	}

	m_worldDay += ((g_theGameClock->GetDeltaSeconds() * worldTimeScale) / (60.f * 60.f * 24.f));
}

// used to define the comparison of two IntVec2
// todo: how to define the operator outside the intVec2 class
// e.g. compare (3, 2) and (2, 3)
bool operator<(IntVec2 const& a, IntVec2 const& b)
{
	if (a.y < b.y)
	{
		return true;
	}
	else if (a.y > b.y)
	{
		return false;
	}
	else
	{
		return (a.x < b.x);
	}
}

