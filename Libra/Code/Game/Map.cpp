#include "Engine/Math/AABB2.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/RaycastResult2D.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/Map.hpp"
#include "Game/App.hpp"
#include "Game/Entity.hpp"
#include "Game/Tile.hpp"
#include "Game/Explosion.hpp"
#include "Game/Scorpio.hpp"
#include "Game/Aries.hpp"
#include "Game/Leo.hpp"
#include "Game/Capricorn.hpp"
#include "Game/PlayerTank.hpp"
#include "Game/Tile.hpp"
#include "Game/Bullet.hpp"
#include "Game/Game.hpp"

const IntVec2 STEP_EAST		= IntVec2(1, 0);
const IntVec2 STEP_SOUTH	= IntVec2(0, -1);
const IntVec2 STEP_WEST		= IntVec2(-1, 0);
const IntVec2 STEP_NORTH	= IntVec2(0, 1);

const IntVec2 STEP_EASTSOUTH = IntVec2(1, -1);
const IntVec2 STEP_SOUTHWEST = IntVec2(-1, -1);
const IntVec2 STEP_WESTNORTH = IntVec2(-1, 1);
const IntVec2 STEP_NORTHEAST = IntVec2(1, 1);

extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;
extern App* g_theApp;
extern Game* g_theGame;
extern AudioSystem* g_theAudio;

std::vector<MapDefinition> MapDefinition::s_mapDefs;

void MapDefinition::InitializeMapDefs()
{
	XmlDocument mapDefXml;
	char const* filePath = "Data/Definitions/LibraMapDefinitions.xml";
	XmlResult result = mapDefXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("failed to load xml file"));

	XmlElement* rootElement = mapDefXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "rootElement is nullPtr");

	XmlElement* mapDefElement = rootElement->FirstChildElement();
	while (mapDefElement)
	{
		std::string elementName = mapDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "MapDefinition", Stringf("root cant matchup with the name"));
		MapDefinition* newMapDef = new MapDefinition(*mapDefElement);// calls the constructor function of TileTypeDefinition
		s_mapDefs.push_back(*newMapDef);
		mapDefElement = mapDefElement->NextSiblingElement();
	}
}

// write the map definition based on xml element
MapDefinition::MapDefinition(XmlElement const& mapDefElement)
{
	std::string notFound = "DEBUG";
	m_name = ParseXmlAttribute(mapDefElement, "name", notFound);
	mapSize = ParseXmlAttribute(mapDefElement, "dimensions", IntVec2(7, 7));
	border = ParseXmlAttribute(mapDefElement, "edgeTileType", notFound);
	mapFill = ParseXmlAttribute(mapDefElement, "fillTileType", notFound);
	bunkerFloors = ParseXmlAttribute(mapDefElement, "startFloorTileType", notFound);
	bunkerWalls = ParseXmlAttribute(mapDefElement, "startBunkerWallTileType", notFound);
	entry = ParseXmlAttribute(mapDefElement, "mapEntry", notFound);
	exit = ParseXmlAttribute(mapDefElement, "mapExit", notFound);
	worm1_TileTYpe = ParseXmlAttribute(mapDefElement, "worm1TileType", notFound);
	worm2_TileTYpe = ParseXmlAttribute(mapDefElement, "worm2TileType", notFound);
	numWorm_1 = ParseXmlAttribute(mapDefElement, "worm1Count", 1);
	numWorm_2 = ParseXmlAttribute(mapDefElement, "worm2Count", 1);
	wormLength_1 = ParseXmlAttribute(mapDefElement, "worm1MaxLength", 1);
	wormLength_2 = ParseXmlAttribute(mapDefElement, "worm2MaxLength", 1);

	m_entitySpawnCounts[ENTITY_TYPE_EVIL_SCORPIO] = ParseXmlAttribute(mapDefElement, "scorpioCount", 0);
	m_entitySpawnCounts[ENTITY_TYPE_EVIL_LEO] = ParseXmlAttribute(mapDefElement, "leoCount", 0);
	m_entitySpawnCounts[ENTITY_TYPE_EVIL_ARIES] = ParseXmlAttribute(mapDefElement, "ariesCount", 0);
	m_entitySpawnCounts[ENTITY_TYPE_EVIL_CAPRICORN] = ParseXmlAttribute(mapDefElement, "capricornCount", 0);
	m_entitySpawnCounts[ENTITY_TYPE_GOOD_BULLET] = 0;
	m_entitySpawnCounts[ENTITY_TYPE_EVIL_BULLET] = 0;
	m_entitySpawnCounts[ENTITY_TYPE_GOOD_FLAME] = 0;
	m_entitySpawnCounts[ENTITY_TYPE_EVIL_FLAME] = 0;
	m_entitySpawnCounts[ENTITY_TYPE_EVIL_MISSILE] = 0;
	m_entitySpawnCounts[ENTITY_TYPE_GOOD_PLAYER] = 0;
	m_entitySpawnCounts[ENTITY_TYPE_MUZZLE_FLASH] = 0;
	m_entitySpawnCounts[ENTITY_TYPE_TANK_EXPLOSION] = 0;
	m_entitySpawnCounts[ENTITY_TYPE_BULLET_EXPLOSION] = 0;
}



Map::Map(MapDefinition inputMapDefinition)
{
	m_mapDefinition = inputMapDefinition;
	GenerateTiles_And_CheckIfMapIsSolvable();

	GenerateInitialEnemies();
	ReserveForEntityPtrList();
}

Map::~Map()
{
	for (int i = 0; i < (int)m_allEntitiesOnThisMap.size(); ++i)
	{
		// todo: delete nullptr is fine but some of the entities are not complete deleted
		delete m_allEntitiesOnThisMap[i];
	}
}

void Map::Update(float deltaSeconds)
{
	// UpdateLevelTransition(deltaSeconds); // this function is put into the Game
	UpdateAllEntities(deltaSeconds);
	UpdateEntityPhysicCollision(deltaSeconds);
	CheckBulletHit(deltaSeconds);
	ClearDeadGarbageEntities(deltaSeconds);
	// DrawHealthBarForDamagedEntity();
}

void Map::PopulateTiles(IntVec2 mapDimensions)
{
	// the map size setting must satisfy the min requirement otherwise it will be reset
	if ( (mapDimensions.x - 1) >= ( 5 + 7) && (mapDimensions.y - 1) >= (5 + 7))
	{
		m_dimensions = mapDimensions;
	}
	else
	{
		m_dimensions = IntVec2(13, 13);
	}
	int numOfTiles = m_dimensions.x * m_dimensions.y;

	// initialize the tiles
	m_tiles.resize(numOfTiles);
	for ( int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			int tileIndex = GetTileIndex_For_TileCoordinates(IntVec2(tileX, tileY));
			Tile& t = m_tiles[tileIndex];

			// set the IndexCoords for the tile in the map
			t.m_tileCoords.y = (tileIndex / m_dimensions.x); // row
			t.m_tileCoords.x = (tileIndex % m_dimensions.x); // column
			t.m_tileCoords = IntVec2(tileX, tileY);

			SetTileType(tileX, tileY, m_mapDefinition.mapFill);
		}
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// set the random sprinkle tile type in the map
	//for (int tileX = 1; tileX < m_dimensions.x - 1; ++tileX)
	//{
	//	for (int tileY = 1; tileY < m_dimensions.y - 1; ++tileY)
	//	{
	//		if (g_rng->RollRandomChance(m_mapDefinition.sprinkle1_Chance))
	//		{
	//			SetTileType(tileX, tileY, m_mapDefinition.sprinkle1);
	//		}
	//		else if ( g_rng->RollRandomChance(m_mapDefinition.sprinkle2_Chance) )
	//		{
	//			SetTileType(tileX, tileY, m_mapDefinition.sprinkle2);
	//		}
	//	}
	//}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// worm generation--for continuous cluster/linear pattern generation	
	SpawnWormToPopulateTiles(m_mapDefinition.numWorm_1, 
		m_mapDefinition.wormLength_1,
		m_mapDefinition.worm1_TileTYpe);

	SpawnWormToPopulateTiles(m_mapDefinition.numWorm_2,
		m_mapDefinition.wormLength_2,
		m_mapDefinition.worm2_TileTYpe);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// set the outer boundary tiles as stone
	for ( int tileX = 0; tileX < m_dimensions.x; ++tileX)
	{
		SetTileType(tileX, 0, m_mapDefinition.border);
		SetTileType(tileX, m_dimensions.y-1, m_mapDefinition.border);
	}
	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		SetTileType(0, tileY, m_mapDefinition.border);
		SetTileType(m_dimensions.x - 1, tileY, m_mapDefinition.border);
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// clear up the entrance field with grass
	for (int tileX = 1; tileX < ENTRANCE_SAFEZONE_SIZE + 1; ++tileX)
	{
		for (int tileY = 1; tileY < ENTRANCE_SAFEZONE_SIZE + 1; ++tileY)
		{
			SetTileType(tileX, tileY, m_mapDefinition.bunkerFloors);
		}
	}
	// set the entrance protection stone 
	for (int tileX = 2; tileX < ENTRANCE_SAFEZONE_SIZE; ++tileX)
	{
		SetTileType(tileX, ENTRANCE_SAFEZONE_SIZE - 1, m_mapDefinition.bunkerWalls);
	}
	for (int tileY = 2; tileY < ENTRANCE_SAFEZONE_SIZE; ++tileY)
	{
		SetTileType(ENTRANCE_SAFEZONE_SIZE - 1, tileY, m_mapDefinition.bunkerWalls);
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// clear up the exit field with grass
	for (int tileX = m_dimensions.x-1 -EXIT_SAFEZONE_SIZE; tileX < m_dimensions.x - 1 ; ++tileX)
	{
		for (int tileY = m_dimensions.y-1 -EXIT_SAFEZONE_SIZE; tileY < m_dimensions.y - 1; ++tileY)
		{
			SetTileType(tileX, tileY, m_mapDefinition.bunkerFloors);
		}
	}
	// set the exit protection
	for (int tileX = m_dimensions.x -EXIT_SAFEZONE_SIZE; tileX < m_dimensions.x - 2; ++tileX)
	{
		SetTileType(tileX, m_dimensions.y -EXIT_SAFEZONE_SIZE, m_mapDefinition.bunkerWalls);
	}
	for (int tileY = m_dimensions.y -EXIT_SAFEZONE_SIZE; tileY < m_dimensions.y - 2; ++tileY)
	{
		SetTileType(m_dimensions.x - EXIT_SAFEZONE_SIZE, tileY, m_mapDefinition.bunkerWalls);
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// the entry and exit
	SetTileType(1, 1, m_mapDefinition.entry);
	SetTileType(m_dimensions.x-2, m_dimensions.y-2, m_mapDefinition.exit);
}

void Map::SpawnWormToPopulateTiles(int numWorm, int totalSteps, std::string tileType)
{
	for ( int wormIndex = 0; wormIndex < numWorm; ++wormIndex )
	{
		// random pick a start point inside the bounds
		IntVec2 tileCoords;
		tileCoords.x = g_rng->RollRandomIntInRange(1, (m_dimensions.x - 2));
		tileCoords.y = g_rng->RollRandomIntInRange(1, (m_dimensions.y - 2));
		SetTileType(tileCoords.x, tileCoords.y, tileType);

		for ( int stepIndex = 0; stepIndex < totalSteps; ++stepIndex)
		{
			// every step the worm is going random pick a direction
			int direction = g_rng->RollRandomIntInRange( 0, (NUM_CRAWLDIRECTION-1) );

			if		(direction == 0) { tileCoords += STEP_EAST; }
			else if (direction == 1) { tileCoords += STEP_SOUTH; }
			else if (direction == 2) { tileCoords += STEP_WEST; }
			else					 { tileCoords += STEP_NORTH; }

			if (IsTileOutOfBounds(tileCoords))
			{
				// if the step is going to make the worm out of bounds, then go back
				if		(direction == 0) { tileCoords -= STEP_EAST; }
				else if (direction == 1) { tileCoords -= STEP_SOUTH; }
				else if (direction == 2) { tileCoords -= STEP_WEST; }
				else					 { tileCoords -= STEP_NORTH; }
			}
			else
			{
				SetTileType(tileCoords.x, tileCoords.y, tileType);
			}
		}
	}
}

void Map::GenerateTiles_And_CheckIfMapIsSolvable()
{
	PopulateTiles(m_mapDefinition.mapSize);
	int mapGerneration = 1;
	for (;;)
	{
		if (CheckIfGeneratedMapIsSolvable())
		{
			// if the current map is solvable
			// loop through all the tiles and set solid to the tiles that player is unable to reach, therefore there will only be one kind of solid tile on map, the water need to be set as not solid, otherwise the water will be rewrite
			for (int i = 0; i < m_exitDistanceField->m_dimensions.x; ++i)
			{
				for (int j = 0; j < m_exitDistanceField->m_dimensions.y; ++j)
				{
					if (m_exitDistanceField->GetHeatValueAt(IntVec2(i, j)) == 999999.f)
					{
						int tileIndex = m_exitDistanceField->GetTileIndexForTileCoords(IntVec2(i, j));
						if (!m_tiles[tileIndex].IsWater() && !m_tiles[tileIndex].IsSolid())
						{
							SetTileType(i, j, m_mapDefinition.worm2_TileTYpe);
						}
					}
				}
			}
			DebuggerPrintf("Map generation succeed after %i times\n", mapGerneration);
			return;
		}
		else
		{
			PopulateTiles(m_mapDefinition.mapSize);
			++mapGerneration;
		}
	}
}

// see if the player is able to get to the exit of the map
bool Map::CheckIfGeneratedMapIsSolvable()
{
	if (m_exitDistanceField)
	{
		delete m_exitDistanceField;
	}
	// generate a heat map to see if the exit tile heat value could be modified
	m_exitDistanceField = new TileHeatMap(m_dimensions);

	PopulateDistanceField(*m_exitDistanceField, IntVec2(1, 1), 999999.f, true);

	IntVec2 exitCoords = IntVec2(m_dimensions.x - 2, m_dimensions.y - 2);
	float exitHeatValue = m_exitDistanceField->GetHeatValueAt(exitCoords);

	return exitHeatValue != 999999.f;
}

void Map::SetTileType(int tileX, int tileY, std::string type)
{
	int tileIndex = tileX + (tileY * m_dimensions.x);
	m_tiles[tileIndex].SetType(type);
}

void Map::AddVertsForTiles(std::vector<Vertex_PCU>& verts, int tileIndex) const
{
	Tile const& tile = m_tiles[tileIndex];
	// todo: 
	AABB2 posBounds = tile.GetBounds();
	Rgba8 spriteColor = tile.GetColor();
	TileTypeDefinition const& tileDef = tile.GetDef();
	AABB2 UVs = tileDef.m_UVs;

	AddVertsUVForAABB2D(verts, posBounds, spriteColor, UVs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// update entities by the entity type order in the enum
void Map::UpdateAllEntities(float deltaSeconds)
{
	int entityTypeIndex = 0;
	int& etI = entityTypeIndex;

	// update the entity by the order of enum
	for (etI = 0; etI < NUM_ENTITY_TYPES; ++ etI)
	{
		EntityPtrList& eList = m_entityPtrListByType[etI];
		for (int i = 0; i < (int)eList.size(); ++i)
		{
			if ( CheckEntityIsAlive(eList[i]) )
			{
				eList[i]->Update(deltaSeconds);
			}
		}
	}
}

void Map::UpdateEntityPhysicCollision(float deltaSeconds)
{
	if (!g_theApp->m_noClipMode)
	{
		// player with other entity
		CheckEntityTypeListCollision(m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER], m_entityPtrListByType[ENTITY_TYPE_EVIL_SCORPIO]);
		CheckEntityTypeListCollision(m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER], m_entityPtrListByType[ENTITY_TYPE_EVIL_ARIES]);
		CheckEntityTypeListCollision(m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER], m_entityPtrListByType[ENTITY_TYPE_EVIL_LEO]);

		// the same type entity push its own kind
		CheckEntityTypeListCollision(m_entityPtrListByType[ENTITY_TYPE_EVIL_LEO], m_entityPtrListByType[ENTITY_TYPE_EVIL_LEO]);
		CheckEntityTypeListCollision(m_entityPtrListByType[ENTITY_TYPE_EVIL_ARIES], m_entityPtrListByType[ENTITY_TYPE_EVIL_ARIES]);

		// different kind of enemy push different kind of enemy entity
		CheckEntityTypeListCollision(m_entityPtrListByType[ENTITY_TYPE_EVIL_LEO], m_entityPtrListByType[ENTITY_TYPE_EVIL_ARIES]);
		CheckEntityTypeListCollision(m_entityPtrListByType[ENTITY_TYPE_EVIL_LEO], m_entityPtrListByType[ENTITY_TYPE_EVIL_SCORPIO]);
		CheckEntityTypeListCollision(m_entityPtrListByType[ENTITY_TYPE_EVIL_ARIES], m_entityPtrListByType[ENTITY_TYPE_EVIL_SCORPIO]);

		PushEntitiesOutOfWalls(deltaSeconds);
	}

}

void Map::CheckEntityTypeListCollision(EntityPtrList& listA, EntityPtrList& listB)
{
	for (int i = 0; i < (int)listA.size(); i++)
	{
		if (CheckEntityIsAlive(listA[i]))
		{
			for (int j = 0; j < (int)listB.size(); j++)
			{
				if (CheckEntityIsAlive(listB[j]))
				{
					// if the entity is check the physics collision with itself, skip
					if (listA[i] == listB[j])
					{
						continue;
					}
					if (DoEntitiesOverlap(*listA[i], *listB[j]))
					{
						PushEntitiesOutOfEachOther(*listA[i], *listB[j]);
					}
				}
			}
		}
	}
}

bool Map::DoEntitiesOverlap(Entity const& a, Entity const& b)
{
	Vec2 posA = a.m_position;
	float collisionRadius_A = a.m_physicsRadius;
	Vec2 posB = b.m_position;
	float collisionRadius_B = b.m_physicsRadius;
	float distAB = (posA - posB).GetLength();

	return(distAB < (collisionRadius_A + collisionRadius_B));
}

void Map::PushEntitiesOutOfEachOther(Entity& a, Entity& b)
{
	bool canApushB = a.m_doesPushEntities && b.m_isPushedByEntities;
	bool canBpushA = b.m_doesPushEntities && a.m_isPushedByEntities;
	if (!canApushB && !canBpushA)
	{
		// for aries pushing aries, then push both of them away
		if (a.m_entityType == ENTITY_TYPE_EVIL_ARIES && b.m_entityType == ENTITY_TYPE_EVIL_ARIES)
		{
			PushDiscOutOfEachOther2D(a.m_position, a.m_physicsRadius, b.m_position, b.m_physicsRadius);
			return;
		}
		// if someone is pushing Scorpio, bounce back
		if (b.m_isFixedOnGround)
		{
			Vec2 contactPos = GetNearestPointOnDisc2D(a.m_position, b.m_position, b.m_physicsRadius);
			PushDiscOutOfFixedPoint2D(a.m_position, a.m_physicsRadius, contactPos);
			return;
		}
		return;
	}
	if (canApushB && !canBpushA)
	{
		Vec2 contactPos = GetNearestPointOnDisc2D(b.m_position, a.m_position, a.m_physicsRadius);
		PushDiscOutOfFixedPoint2D(b.m_position, b.m_physicsRadius, contactPos);
	}
	if (!canApushB && canBpushA)
	{
		Vec2 contactPos = GetNearestPointOnDisc2D(a.m_position, b.m_position, b.m_physicsRadius);
		PushDiscOutOfFixedPoint2D(a.m_position, a.m_physicsRadius, contactPos);
	}
	if (canApushB && canBpushA)
	{
		PushDiscOutOfEachOther2D(a.m_position, a.m_physicsRadius, b.m_position, b.m_physicsRadius);
	}
}

/// <Collisions Between Tiles and Entities>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// </summary>
void Map::PushEntitiesOutOfWalls(float deltaSeconds)
{
	int entityTypeIndex = 0;
	int& etI = entityTypeIndex;

	// update the entity by the order of enum
	for (etI = 0; etI < NUM_ENTITY_TYPES; ++etI)
	{
		EntityPtrList& eList = m_entityPtrListByType[etI];
		for (int i = 0; i < (int)eList.size(); ++i)
		{
			if (CheckEntityIsAlive(eList[i]) && !eList[i]->m_isProjectile)
			{
				PushEntityOutOfWalls(*eList[i], deltaSeconds);
			}
		}
	}

}

void Map::PushEntityOutOfWalls(Entity& entity, float deltaSeconds)
{
	UNUSED(deltaSeconds);

	IntVec2 tileCoordsEntityIsOn = GetTileCoordsInMap_For_WorldPos(entity.m_position);

	// push the entity from four sides first
	PushEntityOutOfTileIfSolid(entity, tileCoordsEntityIsOn + STEP_EAST);
	PushEntityOutOfTileIfSolid(entity, tileCoordsEntityIsOn + STEP_SOUTH);
	PushEntityOutOfTileIfSolid(entity, tileCoordsEntityIsOn + STEP_WEST);
	PushEntityOutOfTileIfSolid(entity, tileCoordsEntityIsOn + STEP_NORTH);

	// then push the entity from four corners
	PushEntityOutOfTileIfSolid(entity, tileCoordsEntityIsOn + STEP_EASTSOUTH);
	PushEntityOutOfTileIfSolid(entity, tileCoordsEntityIsOn + STEP_SOUTHWEST);
	PushEntityOutOfTileIfSolid(entity, tileCoordsEntityIsOn + STEP_WESTNORTH);
	PushEntityOutOfTileIfSolid(entity, tileCoordsEntityIsOn + STEP_NORTHEAST);
}

void Map::PushEntityOutOfTileIfSolid(Entity& entity, IntVec2 const& tileCoords_InMap)
{
	if (IsTileOutOfBounds(tileCoords_InMap))
	{
		return;
	}

	int tileIndex = GetTileIndex_For_TileCoordinates(tileCoords_InMap);
	if ( !IsTileSolid(tileCoords_InMap) && !m_tiles[tileIndex].IsWater())// is the tile is not water or solid tile, the tile will not push the entity
	{
		return;
	}

	if ( m_tiles[tileIndex].IsWater() && entity.m_canSwim) // the wall collision will only not kick off entity is the tile is water and the entity could swim
	{
		return;
	}

	AABB2 tileBounds = GetTileBounds(tileCoords_InMap);
	if (PushDiscOutOfFixedAABB2D(entity.m_position, entity.m_physicsRadius, tileBounds))
	{
		return;
	}
}

bool Map::IsTileSolid(IntVec2 const& tileIndexCoords_InMap) const
{
	if (IsTileOutOfBounds(tileIndexCoords_InMap))
	{
		return true;// if is asking the tile off the map return it is solid for better collision and in case m_tiles[tileIndex] is out of range 
	}
	int tileIndex = GetTileIndex_For_TileCoordinates(tileIndexCoords_InMap);	
	Tile tile = m_tiles[tileIndex];
	return tile.IsSolid();
}

bool Map::IsTileWater(IntVec2 const& tileIndexCoords_InMap) const
{
	int tileIndex = GetTileIndex_For_TileCoordinates(tileIndexCoords_InMap);
	Tile tile = m_tiles[tileIndex];
	return tile.IsWater();
}

// see if the tile coords means the tile is off the map
bool Map::IsTileOutOfBounds(IntVec2 const& tileCoords_InMap) const
{
	if ( tileCoords_InMap.x < 0 || tileCoords_InMap.x > (m_dimensions.x - 1) )
	{
		return true;
	}

	if (tileCoords_InMap.y < 0 || tileCoords_InMap.y >(m_dimensions.y - 1))
	{
		return true;
	}

	return false;
}

void Map::ClearDeadGarbageEntities(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	for (int i = 0; i < m_allEntitiesOnThisMap.size(); ++i)
	{
		Entity*& entity = m_allEntitiesOnThisMap[i]; // get a copy of ptr pointing to the same entity

		if (entity)
		{
			if (!entity->isAlive())
			{
				if (entity->m_entityType == ENTITY_TYPE_GOOD_PLAYER)// todo: ???if the player is dead, do not delete, which will likely cause more problem
				{
					continue;
				}
				else
				{
					RemoveEntityFromMap(*entity);
					delete entity;
				}
			}
		}
	}
}

Entity* Map::GenerateExplosionAnimEntity(Vec2 position, EntityType type)
{
	float angle = g_rng->RollRandomFloatInRange(0.f, 360.f);
	Entity* ePtr = CreateNewEntity(static_cast<EntityType>(type), position, angle);
	AddEntityToMap(*ePtr);
	return ePtr;
}

void Map::DrawHealthBarForDamagedEntity()
{
	m_healthBarsVerts.clear();
	int num_bars = (int)m_allEntitiesOnThisMap.size();
	m_healthBarsVerts.reserve(num_bars);

	for (int factionIndex = 0; factionIndex < NUM_FACTION; ++factionIndex)
	{
		for (int enitityIndex = 0; enitityIndex < m_actorPtrListByType[factionIndex].size(); ++enitityIndex)
		{
			Entity*& entity = m_actorPtrListByType[factionIndex][enitityIndex];
			if (entity)
			{
				if (entity->m_health < entity->m_maxHealth && entity->m_health != 0)
				{
					Vec2 healthBarCenter = entity->m_position + Vec2(0.f, -1.f);
					AABB2 healthBox = AABB2((healthBarCenter - m_healthBoxDimension * 0.5f), (healthBarCenter + m_healthBoxDimension * 0.5f));

					Vec2 healthBarDimensions = m_healthBarDimension;
					healthBarDimensions.x = ((float)entity->m_health / (float)entity->m_maxHealth) * m_healthBarDimension.x;
					Vec2 healthBarStartPos = healthBox.m_mins + (m_healthBoxDimension - m_healthBarDimension) * 0.5f;
					AABB2 healthBar = AABB2(healthBarStartPos, (healthBarStartPos + healthBarDimensions));;
					AddVertsForAABB2D(m_healthBarsVerts, healthBox, Rgba8::RED);
					AddVertsForAABB2D(m_healthBarsVerts, healthBar, Rgba8::GREEN);
				}
			}
		}
	}
}

void Map::CheckBulletHit(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	// check with good bullet faction with evil actor faction
	EntityPtrList& goodBulletList = m_bulletPtrListByType[FACTION_GOOD];
	for (int i = 0; i < (int)goodBulletList.size(); ++i)
	{
		Entity*& goodBullet = goodBulletList[i];
		if (IsEntityPointerValid_And_TheEntityIsAlive(goodBullet))
		{
			EntityPtrList& evilActorList = m_actorPtrListByType[FACTION_EVIL];
			for (int j = 0; j < (int)evilActorList.size(); ++j)
			{
				Entity*& evilActor = evilActorList[j];
				if ( IsEntityPointerValid_And_TheEntityIsAlive(evilActor))
				{
					if (DoEntitiesOverlap(*goodBulletList[i], *evilActorList[j]))
					{
						evilActorList[j]->ReactToBulletHit(*dynamic_cast<Bullet*>(goodBulletList[i]));
					}
				}
			}
		}
	}

	// check with evil bullet faction with good actor faction
	EntityPtrList& evilBulletList = m_bulletPtrListByType[FACTION_EVIL];
	for (int i = 0; i < (int)evilBulletList.size(); ++i)
	{
		Entity*& evilBullet = evilBulletList[i];
		if (IsEntityPointerValid_And_TheEntityIsAlive(evilBullet))
		{
			EntityPtrList& goodActorList = m_actorPtrListByType[FACTION_GOOD];
			for (int j = 0; j < (int)goodActorList.size(); ++j)
			{
				Entity*& goodActor = goodActorList[j];
				if (IsEntityPointerValid_And_TheEntityIsAlive(goodActor))
				{
					if (DoEntitiesOverlap(*evilBulletList[i], *goodActorList[j]))
					{
						goodActorList[j]->ReactToBulletHit(*dynamic_cast<Bullet*>(evilBulletList[i]));
					}
				}
			}
		}
	}
	return;
	// pick the m_bulletPtrListByType list and m_actorPtrListByType that has different faction send it to another function
	// this function take two list and do the cross check of entity from each two list
}

/// <Render>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Map::Render() const 
{
	RenderTiles();
	if ( g_theGame->m_showHeatMap )
	{
		RenderHeatMap(); // heat map is drawn under the entities
	}
	RenderEntities();
	RenderHealthBars();
}

void Map::RenderTiles() const
{
	// create a temp list of Vertex_PCU and set the capacity according to the vertexes of this map
	int vertexCountForThisMap = m_dimensions.x * m_dimensions.y * 2 * 3; // each tile has two tris, each tri has three vertexes
	std::vector<Vertex_PCU> tileVerts;
	tileVerts.reserve(vertexCountForThisMap);

	for ( int tileIndex = 0; tileIndex < (int)m_tiles.size(); ++tileIndex)
	{
		AddVertsForTiles(tileVerts, tileIndex);
	}
	g_theRenderer->BindTexture(g_theApp->g_textureID[SPRITESHEET_TILES]);
	g_theRenderer->DrawVertexArray((int)tileVerts.size(), tileVerts.data());
}

void Map::RenderEntities() const
{
	int entityTypeIndex = 0;
	int& etI = entityTypeIndex;

	for (etI = 0; etI < NUM_ENTITY_TYPES; ++etI)
	{
		EntityPtrList const& eList = m_entityPtrListByType[etI];
		for (int i = 0; i < (int)eList.size(); ++i)
		{
			if (CheckEntityIsAlive(eList[i]))
			{
				eList[i]->Render();
			}
		}
	}
}

void Map::RenderHealthBars() const
{

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray((int)m_healthBarsVerts.size(), m_healthBarsVerts.data());
}

void Map::RenderDebug() const
{
	// todo:
	for (int i = 0; i < m_allEntitiesOnThisMap.size(); ++i)
	{
		Entity* const& entity = m_allEntitiesOnThisMap[i];
		if (entity)
		{
			m_allEntitiesOnThisMap[i]->DebugRender();
		}
	}
}

void Map::RenderHeatMap() const
{
	switch (g_theGame->m_heatMapState)
	{
	case HeatMapAnalysis::SOLVABILITY : // will draw the heat map for the player tank
	{ 
		DrawHeatMap(*m_exitDistanceField);
	} break;
	case HeatMapAnalysis::FIRST_LEO_PATHING : // will draw the heat map for the first leo
	{
		Entity* const& leo = m_entityPtrListByType[ENTITY_TYPE_EVIL_LEO][0];
		if (leo)
		{
			DrawHeatMap(*leo->m_heatMap);

			std::vector<Vertex_PCU> verts;
			Vec2 tailPos = leo->m_position + 1600.f * Vec2(1.f, 1.f);
			AddVertsForArrow2D(verts, tailPos, leo->m_position, 0.5f, 0.3f, Rgba8::YELLOW);
			g_theRenderer->BindTexture(nullptr);
			g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
		}
	} break;
	}
}

void Map::DrawHeatMap(TileHeatMap heatMap) const
{
	std::vector<Vertex_PCU> tempHeatMapVerts;
	tempHeatMapVerts.resize((int)(m_dimensions.x * m_dimensions.y * 6));

	AABB2 bounds = AABB2(Vec2::ZERO, Vec2(static_cast<float>(m_dimensions.x), static_cast<float>(m_dimensions.y)));
	float maxHeatValue = heatMap.GetTheMaxHeatValue();
	FloatRange heatMapFloatRange(0.f, maxHeatValue);
	heatMap.AddVertsForTileHeatMapDebugDraw(tempHeatMapVerts, bounds, heatMapFloatRange);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray((int)tempHeatMapVerts.size(), tempHeatMapVerts.data());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// <tile helper functions>
int Map::GetTileIndex_For_TileCoordinates(IntVec2 tileCoord) const
{
	int index = tileCoord.x + tileCoord.y * m_dimensions.x;
	return index;
}

IntVec2 Map::GetTileCoordsInMap_For_WorldPos(Vec2 const& worldPos) const
{
	IntVec2 tileCoord;
	tileCoord.x = RoundDownToInt(worldPos.x);
	tileCoord.y = RoundDownToInt(worldPos.y);
	return tileCoord;
}

AABB2 Map::GetTileBounds(IntVec2 const& tileIndexCoords_InMap)
{
	int indexInList = GetTileIndex_For_TileCoordinates(tileIndexCoords_InMap);
	Tile tile = m_tiles[indexInList];
	//SetTileType(tileIndexCoords_InMap.x, tileIndexCoords_InMap.y, DEBUG);
	return tile.GetBounds();
}

Vec2 Map::GetMapDimensions()
{
	float wide   = static_cast<float>(m_dimensions.x);
	float height = static_cast<float>(m_dimensions.y);
	return Vec2(wide, height);
}

void Map::GenerateInitialEnemies()
{
	for (int entityType = 0; entityType < NUM_ENTITY_TYPES; ++entityType)
	{
		int spawnCount = m_mapDefinition.m_entitySpawnCounts[entityType];
		while (spawnCount > 0)
		{
			// create entity and add to the map entity list
			Vec2 randomPos = GetRandomNotSolidNotWaterTilePos();
			float angle = g_rng->RollRandomFloatInRange(0.f, 360.f);
			SpawnNewEntity(static_cast<EntityType>(entityType), randomPos, angle);
			spawnCount -= 1;
		}
	}
}

EntityPtrList Map::GetEntitiesByType(EntityType type) const
{
	return m_entityPtrListByType[type];
}

void Map::SpawnNewEntity(EntityType type, Vec2 const& position, float orientationDegrees)
{
	Entity* ePtr = CreateNewEntity(type, position, orientationDegrees);
	AddEntityToMap(*ePtr);
}

void Map::AddEntityToMap(Entity& e)
{
	Entity* entityPtr = &e;
	entityPtr->m_map = this;
	AddEntityToList(entityPtr, m_allEntitiesOnThisMap);
	AddEntityToList(entityPtr, m_entityPtrListByType[entityPtr->m_entityType]);// add entity to list according to the entity type
	// add entity to list according to if there is actor of projectile
	// then put them according to their faction
	// todo:???? when should put them into different faction list
	if (entityPtr->m_isAnim)
	{
		return;
	}
	else
	{
		if (entityPtr->m_isProjectile)
		{
			AddEntityToList(entityPtr, m_bulletPtrListByType[e.m_entityFaction]);
		}
		else
		{
			AddEntityToList(entityPtr, m_actorPtrListByType[e.m_entityFaction]);
		}
	}
}

void Map::ReserveForEntityPtrList()
{
	for (int entityType = 0; entityType < NUM_ENTITY_TYPES; ++entityType)
	{
		m_entityPtrListByType[static_cast<EntityType>(entityType)].reserve(m_mapDefinition.m_entitySpawnCounts[static_cast<EntityType>(entityType)]);
		if (entityType == ENTITY_TYPE_GOOD_BULLET)
		{
			m_entityPtrListByType[static_cast<EntityType>(entityType)].reserve(64);
		}
	}
}

Entity* Map::CreateNewEntity(EntityType type, Vec2 const& position, float orientationDegrees)
{
	switch (type)
	{
	case ENTITY_TYPE_EVIL_SCORPIO:
	{
		return new Scorpio(this, type, FACTION_EVIL, position, orientationDegrees);
	}
	case ENTITY_TYPE_EVIL_LEO:
	{
		return new Leo(this, type, FACTION_EVIL, position, orientationDegrees);
	}
	case ENTITY_TYPE_EVIL_ARIES:
	{
		return new Aries(this, type, FACTION_EVIL, position, orientationDegrees );
	}
	case ENTITY_TYPE_EVIL_CAPRICORN:
	{
		return new Capricorn(this, type, FACTION_EVIL, position, orientationDegrees);
	}
	case ENTITY_TYPE_GOOD_BULLET:
	{
		return new Bullet(this, type, FACTION_GOOD, position, orientationDegrees );
	}
	case ENTITY_TYPE_EVIL_BULLET:
	{
		return new Bullet(this, type, FACTION_EVIL, position, orientationDegrees);
	}
	case ENTITY_TYPE_GOOD_FLAME:
	{
		return new Bullet(this, type, FACTION_GOOD, position, orientationDegrees);
	}
	case ENTITY_TYPE_EVIL_FLAME:
	{
		return new Bullet(this, type, FACTION_EVIL, position, orientationDegrees);
	}
	case ENTITY_TYPE_EVIL_MISSILE:
	{
		return new Bullet(this, type, FACTION_EVIL, position, orientationDegrees);
	}
	case ENTITY_TYPE_MUZZLE_FLASH:
	{
		return new Explosion(this, type, FACTION_NEUTRAL, position, orientationDegrees, 0.5f);
	}
	case ENTITY_TYPE_TANK_EXPLOSION:
	{
		return new Explosion(this, type, FACTION_NEUTRAL, position, orientationDegrees, 1.5f);
	}
	case ENTITY_TYPE_BULLET_EXPLOSION:
	{
		return new Explosion(this, type, FACTION_NEUTRAL, position, orientationDegrees, 0.75f);
	}
	default: return nullptr;
	}
}

Vec2 Map::GetRandomNotSolidNotWaterTilePos()
{
	for (;;)// run until find a satisfying spot to spawn
	{
		IntVec2 tileCoord;
		tileCoord.x = g_rng->RollRandomIntInRange(1, (m_dimensions.x - 2));
		tileCoord.y = g_rng->RollRandomIntInRange(1, (m_dimensions.y - 2));
		if (IsTileSolid(tileCoord))
		{
			continue; 
		}
		if (IsTileWater(tileCoord))
		{
			continue;
		}

		// do not spawn inside the entrance safe zone
		if ( tileCoord.x <= ENTRANCE_SAFEZONE_SIZE && tileCoord.y <= ENTRANCE_SAFEZONE_SIZE )
		{
			continue;
		}

		// do not spawn inside the exit safe zone
		if (tileCoord.x >= m_dimensions.x - 1 - EXIT_SAFEZONE_SIZE && tileCoord.y >= m_dimensions.y - 1 - EXIT_SAFEZONE_SIZE)
		{
			continue;
		}

		//should not spawn right at the center of the tile in case could not push out ?????????????????????????????
		Vec2 spawnLocation;
		spawnLocation.x = static_cast<float>(tileCoord.x + 0.5f);
		spawnLocation.y = static_cast<float>(tileCoord.y + 0.5f);
		return spawnLocation;
	}
}


// put every type of entity into its own entity list for better searching by category capability
// todo: when I try to add for the first time, I could not access the the size of the list
void Map::AddEntityToList(Entity* entityPtr, EntityPtrList& list)
{
	// m_entityPtrListByType[entityPtr->m_entityType].reserve(m_mapDefinition.m_entitySpawnCounts[entityPtr->m_entityType]);

	// search for the very first empty space in the list from the start
	for (int i = 0; i < (int)list.size(); ++i)
	{
		if (!list[i])
		{
			list[i] = entityPtr;
			return;
		}
	}

	//if all the slot inside the list is full, grow the index by the size of list by 1 and push back in
	list.push_back(entityPtr);
}

bool Map::CheckEntityIsAlive(Entity* const entity) const
{
	if (entity)
	{
		if (!entity->m_isDead && !entity->m_isGarbage)
		{
			return true;
		}
		return false;
	}
	else
	{
		return false;
	}
}

bool Map::IsEntityPointerValid_And_TheEntityIsAlive(Entity* entityPtr)
{
	if (entityPtr)
	{
		if (entityPtr->isAlive())
		{
			return true;
		}
		return false;
	}
	return false;
}


void Map::RemoveEntityFromMap(Entity& e)
{
	Entity* entityPtr = &e;

	RemoveEntityFromList(entityPtr, m_entityPtrListByType[entityPtr->m_entityType]); // todo:??? may throw an error when in debug mode

	RemoveEntityFromList(entityPtr, m_allEntitiesOnThisMap);

	if (entityPtr->m_isProjectile)
	{
		RemoveEntityFromList(entityPtr, m_bulletPtrListByType[e.m_entityFaction]);
	}
	else
	{
		RemoveEntityFromList(entityPtr, m_actorPtrListByType[e.m_entityFaction]);
	}
}

// remove the entity based on the index in list?
void Map::RemoveEntityFromList(Entity* entityPtr, EntityPtrList& list)
{
	for (int i = 0; i < (int)list.size(); ++i)
	{
		if ( entityPtr == list[i] )
		{
			list[i] = nullptr;
			return;
		}
	}
	// list[entityPtr->m_indexInList] = nullptr;
}


/// <RayCast functions>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Map::HasLineOfSight(Entity& entity)
{
	if (!m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0])
	{
		// ERROR_AND_DIE("The entity is searching for the player tank on map and there is no player on map");
		return false;// if the player do not exist on current retun false
	}

	// if the player is dead, of course no line of sight of the player
	if (m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0]->m_isDead || m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0]->m_isGarbage)
	{
		return false;
	}

	// if the player is too far from the entity, no sight of player
	Entity* playerRef = m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0];
	Vec2 disp = playerRef->m_position - entity.m_position;
	if (disp.GetLength() >= RAYCAST_TOFINDPLAYER_RANGE)
	{
		entity.m_raycastResult.m_didImpact = false;
		entity.m_raycastResult.m_impactDist = 0.f;
		entity.m_raycastResult.m_impactPos = entity.m_position;
		return false;
	}

	// if the player is within range, construct raycast to see if the raycast is blocked by a solid tile
	Vec2 rayNormal = disp.GetNormalized();
	Vec2 dispToPlayer = playerRef->m_position - entity.m_position;
	float dist = dispToPlayer.GetLength();
	entity.m_raycastResult = RaycastVsTiles(entity.m_position, rayNormal, dist);
	if (entity.m_raycastResult.m_didImpact)
	{
		return false;// the aiming is blocked by a solid tile
	}
	else
	{
		return true;;// the entity has a clear sight of the player
	}
}

RaycastResult2D Map::RaycastVsTiles(Vec2 start, Vec2 rayNormal, float dist)
{
	RaycastResult2D result;

	// todo: if the start of the rayast start inside the tile, should return m_didImpact true
	// if (IsPointInSolid(start))
	// {
	// 	
	// }

	for (int i = 0; i < (int)RAYCAST_NUM_STEPS; ++i)
	{
		float raycastDist = RAYCAST_UNITS_PER_STEP * static_cast<float>(i);
		Vec2 raycastEndPrt = start + (rayNormal * raycastDist);
		// if the raycast hit a solid tile
		if (raycastDist >= dist)
		{
			break;
		}
		if (IsPointInSolid(raycastEndPrt))
		{
			result.m_didImpact = true;
			result.m_impactPos = raycastEndPrt;
			result.m_impactDist = raycastDist;
			// calculate the impact normal
			float hitDist = RAYCAST_UNITS_PER_STEP * static_cast<float>(i-1);
			Vec2 lastPrtBeforeHit = start + (rayNormal * hitDist);
			IntVec2 lastHitTileCoords = GetTileCoordsInMap_For_WorldPos(lastPrtBeforeHit);
			int lastHitTileIndex = GetTileIndex_For_TileCoordinates(lastHitTileCoords);
			IntVec2 hitTileCoords = GetTileCoordsInMap_For_WorldPos(raycastEndPrt);
			int hitTileIndex = GetTileIndex_For_TileCoordinates(hitTileCoords);
			//todo:??? sometimes the bullet is out of bounds
			if (IsTileOutOfBounds(lastHitTileCoords) || IsTileOutOfBounds(hitTileCoords))
			{
				break;
			}
			result.m_impactNormal = m_tiles[lastHitTileIndex].GetBounds().m_mins - m_tiles[hitTileIndex].GetBounds().m_mins;
			result.m_impactNormal = result.m_impactNormal.GetNormalized();
			return result;
		}
	}
	// if the whole loop is finished and the raycast has not reach a solid tile
	// then the raycast did not hit the tile
	result.m_didImpact = false;
	result.m_impactPos = start + (rayNormal * dist);
	result.m_impactDist = dist;
	result.m_impactNormal = -rayNormal;
	return result;
}

bool Map::IsPointInSolid(Vec2 referencePointPos)
{
	IntVec2 tileCoord =  GetTileCoordsInMap_For_WorldPos(referencePointPos);
	return IsTileSolid(tileCoord);
}

void Map::PopulateDistanceField(TileHeatMap& distanceField, IntVec2 startCoords, float maxCost, bool treatWaterAsSolid, bool treatScorpioAsSolid) const
{
	distanceField.SetDefaultHeatValueForAllTiles(maxCost);
	distanceField.SetHeatValueForTile(startCoords, 0.f);

	float currentHeatValue = 0.f;
	bool isBlocked = false;
	while (!isBlocked)// if it is not blocked
	{
		isBlocked = true; // see if the neighbor can turn isBlocked to false, then the while loop could continue to loop
		float nextHeatValue = currentHeatValue + 1.f;

		for ( int tileY = 0; tileY < m_dimensions.y; ++tileY)
		{
			for ( int tileX = 0; tileX < m_dimensions.x; ++tileX )
			{
				IntVec2 tileCoords(tileX, tileY);
				if ( distanceField.GetHeatValueAt(tileCoords)==currentHeatValue )
				{
					SetHeatIfLessAndNotSolid( isBlocked, distanceField, tileCoords + STEP_EAST, nextHeatValue, treatWaterAsSolid, treatScorpioAsSolid);
					SetHeatIfLessAndNotSolid( isBlocked, distanceField, tileCoords + STEP_WEST, nextHeatValue, treatWaterAsSolid, treatScorpioAsSolid);
					SetHeatIfLessAndNotSolid(isBlocked, distanceField, tileCoords + STEP_SOUTH, nextHeatValue, treatWaterAsSolid, treatScorpioAsSolid);
					SetHeatIfLessAndNotSolid(isBlocked, distanceField, tileCoords + STEP_NORTH, nextHeatValue, treatWaterAsSolid, treatScorpioAsSolid);
				}
			}
		}
		currentHeatValue += 1.f;
	}
}

void Map::SetHeatIfLessAndNotSolid(bool& isBlocked, TileHeatMap& distantField, IntVec2 tileCoords, float compareValue, bool treatWaterAsSolid, bool treatScorpioAsSolid) const
{
	// if the Scorpio needed to be considered as a solid tile, then we'll check all living scorpio position
	std::vector<IntVec2> scorpioOccupiedTileCoords;
	if (treatScorpioAsSolid)
	{
		for (int i = 0; i < (int)m_entityPtrListByType[ENTITY_TYPE_EVIL_SCORPIO].size(); ++i)
		{
			Entity* const& scorpio = m_entityPtrListByType[ENTITY_TYPE_EVIL_SCORPIO][i];
			if (scorpio)
			{
				if (CheckEntityIsAlive(scorpio))
				{
					IntVec2 scorpioTileCoords = GetTileCoordsInMap_For_WorldPos(scorpio->m_position);
					scorpioOccupiedTileCoords.push_back(scorpioTileCoords);
				}
			}
		}
	}
	//if there is a scorpio occupying this tile, then it heat value will not be modified
	for (int i = 0; i < scorpioOccupiedTileCoords.size(); ++i)
	{
		if (tileCoords == scorpioOccupiedTileCoords[i])
		{
			return;
		}
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// if the tile is out of bound, consider it is unreachable
	if (IsTileOutOfBounds(tileCoords))
	{
		return;
	}
	// if the tile is solid or is water, consider it is unreachable
	if (IsTileSolid(tileCoords))
	{
		return;
	}
	// if the waster is treat as solid, do not modify it, otherwise modify as usual tile
	int tileIndex = distantField.GetTileIndexForTileCoords(tileCoords);
	if ( treatWaterAsSolid && m_tiles[tileIndex].m_tileDef->m_isWater )
	{
		return;
	}
	// compare the heat value record
	if (compareValue < distantField.m_values[tileIndex])
	{
		distantField.m_values[tileIndex] = compareValue;
		isBlocked = false;
	}
}

