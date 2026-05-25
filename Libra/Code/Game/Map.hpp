#pragma once
#include "Game/Tile.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Game/Entity.hpp"
#include "Engine/core/HeatMaps.hpp"
#include "Engine/core/XmlUtils.hpp"
#include <vector>
#include <string>

struct RaycastResult2D;
typedef std::vector<Entity*> EntityPtrList;

extern const IntVec2 STEP_EAST;
extern const IntVec2 STEP_SOUTH;
extern const IntVec2 STEP_WEST;
extern const IntVec2 STEP_NORTH;

extern const IntVec2 STEP_EASTSOUTH;
extern const IntVec2 STEP_SOUTHWEST;
extern const IntVec2 STEP_WESTNORTH;
extern const IntVec2 STEP_NORTHEAST;

enum WormDirection
{
	CRAWL_EAST,
	CRAWL_SOUTH,
	CRAWL_WEST,
	CRAWL_NORTH,
	NUM_CRAWLDIRECTION
};

struct MapDefinition
{
public:
	MapDefinition() = default;
	MapDefinition(XmlElement const& tileDefElement);

	std::string		m_name = "not Initialized";
	IntVec2			mapSize = IntVec2(15, 15);
	std::string		border = "DEBUG";
	std::string		mapFill = "DEBUG";
	std::string		bunkerFloors = "DEBUG";
	std::string		bunkerWalls = "DEBUG";
	std::string		entry = "DEBUG";
	std::string		exit = "DEBUG";
	//TileType		sprinkle1 = TileType::DEBUG;
	//TileType		sprinkle2 = TileType::DEBUG;
	//float			sprinkle1_Chance = 0.f;
	//float			sprinkle2_Chance = 0.f;

	std::string		worm1_TileTYpe = "DEBUG";
	std::string		worm2_TileTYpe = "DEBUG";
	int				numWorm_1 = 1;
	int				numWorm_2 = 1;
	int				wormLength_1 = 1;
	int				wormLength_2 = 1;

	int				m_entitySpawnCounts[NUM_ENTITY_TYPES];

	static void InitializeMapDefs(); // call defineTileType to define each tile type definition
	static std::vector<MapDefinition> s_mapDefs;
};

class Map
{
friend class Game;

public:
	Map (MapDefinition inputMapDefinition);
	~Map();

	void	Update(float deltaSeconds);
	void	PopulateTiles(IntVec2 mapDimensions);
	void	SpawnWormToPopulateTiles(int numWorm, int totalSteps, std::string tileType);
	void	SetTileType(int tileX, int tileY, std::string type);
	void	UpdateAllEntities(float deltaSeconds);
	void	ClearDeadGarbageEntities(float deltaSeconds);
	Entity* GenerateExplosionAnimEntity(Vec2 position, EntityType type);
	void	DrawHealthBarForDamagedEntity();

	Vec2	m_healthBoxDimension = Vec2(1.f, 0.3f);
	Vec2	m_healthBarDimension = Vec2(0.8f, 0.2f);
	std::vector<Vertex_PCU> m_healthBarsVerts;

	// Physics collision prevention
	void UpdateEntityPhysicCollision(float deltaSeconds);
	void CheckEntityTypeListCollision( EntityPtrList& listA, EntityPtrList& listB );
	void PushEntitiesOutOfEachOther(Entity& actorA, Entity& acotrB);
	bool DoEntitiesOverlap(Entity const& a, Entity const& b);
	void PushEntitiesOutOfWalls(float deltaSeconds);
	void PushEntityOutOfWalls(Entity& entity, float deltaSeconds);
	void PushEntityOutOfTileIfSolid(Entity& entity, IntVec2 const& tileCoords_InMap);

	// could combine all enemy firing
	void CheckBulletHit(float deltaSeconds);

	// render tile verts and entities verts separately
	void AddVertsForTiles(std::vector<Vertex_PCU>& verts, int tileIndex) const;
	void Render() const;
	void RenderTiles() const;
	void RenderEntities() const;
	void RenderHealthBars() const;
	void RenderDebug() const;
	void RenderHeatMap() const;
	void DrawHeatMap(TileHeatMap heatMap) const;

	TileHeatMap* m_exitDistanceField;
	std::vector<Vertex_PCU> m_exitHeatMapVerts;

	// functions used to get tile index information
	int     GetTileIndex_For_TileCoordinates(IntVec2 tileCoord) const;
	IntVec2 GetTileCoordsInMap_For_WorldPos(Vec2 const& worldPos) const;
	AABB2   GetTileBounds(IntVec2 const& tileIndexCoords_InMap);
	Vec2	GetMapDimensions();
	bool	IsTileOutOfBounds(IntVec2 const& tileCoords_InMap) const;
	bool	IsTileSolid(IntVec2 const& tileIndexCoords_InMap) const; // get a tile's solid information based on the input tile coordinates
	bool	IsTileWater(IntVec2 const& tileIndexCoords_InMap) const;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// managing entities
	void GenerateInitialEnemies();
	EntityPtrList GetEntitiesByType(EntityType type) const; // return the entity type list by inputting the type
	void SpawnNewEntity(EntityType type, Vec2 const& position, float orientationDegrees);
	Entity* CreateNewEntity(EntityType type, Vec2 const& position, float orientationDegrees); // create and add add to map
	Vec2 GetRandomNotSolidNotWaterTilePos();
	void AddEntityToMap(Entity& e); // for the spawned entities to add to the map entity type list
	void ReserveForEntityPtrList();
	void AddEntityToList(Entity* entityPtr, EntityPtrList& list);

	bool CheckEntityIsAlive(Entity* const entity) const;
	bool IsEntityPointerValid_And_TheEntityIsAlive(Entity* entityPtr);
	void RemoveEntityFromMap(Entity& e); // when player is moved or the entity is destroyed
	void RemoveEntityFromList(Entity* entityPtr, EntityPtrList& list);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Raycast related functions
	bool HasLineOfSight(Entity& entity);// construct a Ray from A to B
	RaycastResult2D RaycastVsTiles(Vec2 start, Vec2 rayNormal, float dist);// return true if the raycast has no impact
	bool IsPointInSolid(Vec2 referencePointPos); // check if the rayCast point is inside a solid tile
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// heat maps
	void GenerateTiles_And_CheckIfMapIsSolvable();
	bool CheckIfGeneratedMapIsSolvable();
	void PopulateDistanceField(TileHeatMap& distanceField, IntVec2 startCoords, float maxCost, bool treatWaterAsSolid=true, bool treatScorpioAsSolid = false) const;
	void SetHeatIfLessAndNotSolid( bool& isBlocked, TileHeatMap& distanceField, IntVec2 tileCoords, float compareValue, bool treatWaterAsSolid = true, bool treatScorpioAsSolid = false) const;
	// void PopulateDistanceFieldForEntityPathing(Entity* entityPtr, IntVec2 startCoords, float maxCost) const;

	EntityPtrList	m_allEntitiesOnThisMap;
	EntityPtrList	m_entityPtrListByType[NUM_ENTITY_TYPES];
	// std::vector<EntityPtrList*> m_entityPtrList[NUM_ENTITY_TYPES];
	EntityPtrList	m_actorPtrListByType[NUM_FACTION];
	EntityPtrList	m_bulletPtrListByType[NUM_FACTION];

	MapDefinition		 m_mapDefinition;
	IntVec2			     m_dimensions; // contains the overall wide(x) and high(Y)
	std::vector<Tile>    m_tiles;
};
