#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/core/HeatMaps.hpp"
#include "Engine/core/XmlUtils.hpp"
#include "Engine/core/Vertex_PCUTBN.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/core/Vertex_PCUTBN.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Game/ActorUID.hpp"
#include "Game/Tile.hpp"
#include "Game/Actor.hpp"
#include "Game/Entity.hpp"
#include "Game/GameCommon.hpp"
#include <vector>
#include <string>

struct RaycastResult3D;
struct LightingConstants;
class  Image;
class VertexBuffer;
class IndexBuffer;
class Shader;

extern const IntVec2 STEP_EAST;
extern const IntVec2 STEP_SOUTH;
extern const IntVec2 STEP_WEST;
extern const IntVec2 STEP_NORTH;

extern const IntVec2 STEP_EASTSOUTH;
extern const IntVec2 STEP_SOUTHWEST;
extern const IntVec2 STEP_WESTNORTH;
extern const IntVec2 STEP_NORTHEAST;

struct SpawnInfo
{
public:
	SpawnInfo(ActorDefinition* actorDef, Vec3 pos, Vec3 velocity, EulerAngles orientation);
	SpawnInfo(XmlElement const& spawnInfoElement);

	ActorDefinition* m_actorDef = nullptr;
	ActorFaction m_actorFaction = ActorFaction::COUNT;

	Vec3		 m_position;
	Vec3		 m_velocity;
	EulerAngles  m_orientation;

	static std::vector<SpawnInfo> m_spawnInfos;

	static ActorFaction GetFactionByString(std::string str);
};

struct MapDefinition
{
public:

	// Actor* GetActorByUID(ActorUID const uid) const;

	MapDefinition() = default;
	~MapDefinition() = default;
	MapDefinition(XmlElement const& tileDefElement);

	std::string		m_name = "not Initialized";
	std::string		m_mapImagePath; // the relative file path name of a .PNG image file

	Shader*			m_shader;
	Image*			m_image;

	Texture*		m_spriteSheetTexture;
	SpriteSheet*	m_spriteSheet;
	IntVec2			m_spriteSheetCount;

	IntVec2			m_mapSize = IntVec2(15, 15);

	float			m_ceilingHeight = 2.f;
	float			m_floorHeight = 0.f;

	std::vector<SpawnInfo> m_spawnInfos;

	static void InitializeMapDefs(); // call defineTileType to define each tile type definition
	static void ClearDefinitions();
	static MapDefinition* const GetByName(std::string const& name);
	static std::vector<MapDefinition> s_mapDefs;
};

class Map
{
friend class Game;

public:
	Map (MapDefinition& inputMapDefinition);
	~Map();

	void Startup();
	void Update();
	void SetTileType(int tileX, int tileY, std::string type);

	void ReadTexelInfoFromImageAndSetTiles();

	void AddVertsAndIndexesForAllTilesInMap();
	void CreateVertexIndexBufferAndCopyFromCPUtoGPU();

	// render tile verts and entities verts separately
	void Render() const;
	void RenderTiles() const;
	Mat44 GetModelMatrix() const;

	// functions used to get tile index information
	int     GetTileIndex_For_TileCoordinates(IntVec2 tileCoord) const;
	IntVec2 GetTileCoordsInMap_For_WorldPos(Vec3 const& worldPos) const;
	AABB2   GetTileBounds(IntVec2 const& tileIndexCoords_InMap);

	AABB3   GetTileBlockBounds(IntVec2 const& tileIndexCoords_InMap);
	AABB3   GetTileBlockBounds(int tileIndex);
	Vec2	GetMapDimensions();
	Tile* const	GetTile(IntVec2 tileCoords);
	bool	IsTileOutOfBounds(IntVec2 const& tileCoords_InMap) const;
	bool	IsPositionInBounds(Vec3 position, float const tolerance = 0.f) const;
	bool	IsTileSolid(IntVec2 const& tileIndexCoords_InMap) const; // get a tile's solid information based on the input tile coordinates
	bool	IsTileHalfHeight(IntVec2 const& tileIndexCoords_InMap) const;

	// map interact functions
	void UpdateKeyAndControllers();

	// collisions
	float worldZCollisionOffset = 0.f; // Avoid the edge of the shape is right on the floor/ceiling

	void UpdatePhysicsCollisions();
	void CollideActors();
	void CollideActorsWithMap();

	void CollideActors(Actor* A, Actor* B);
	bool DoActorsOverlapInSpace(Actor const& a, Actor const& b);
	void PushActorsOutOfEachOtherInXY(Actor& actorA, Actor& actorB);
	void PushMovableActorOutOfStaticActor(Actor& movableActor, Actor& staticActor);
	bool PushActorOutOfTileIfSolid(Actor& actor, IntVec2 const& tileCoords);
	void CollideActorWithMap(Actor* actor);

	void ShootRaycastForCollisionTest(float rayDist);

	// raycast functions
	RaycastResult3D RaycastAll(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) const;
	RaycastResult3D FastRaycastForVoxelGrids(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) const;
	RaycastResult3D RaycastWorldXY(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) const;
	RaycastResult3D RaycastWorldZ(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) const;
	RaycastResult3D RaycastWorldActors(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) const;

	// tiles
	MapDefinition*		 m_mapDefinition;
	IntVec2			     m_dimensions; // contains the overall wide(x) and high(Y)
	std::vector<Tile>    m_tiles;
	Image*				 m_image;

	// map physics info
	Vec3				 m_mapOrigin;
	EulerAngles			 m_mapOrientation;

	// actors
	ActorPtrList		 m_actorList;
	bool				 CheckIfActorExistAndShouldBeDestroyed(Actor* actor) const;
	bool				 CheckIfActorExistAndNotDestroyed(Actor* actor) const;
	bool				 CheckIfActorExistAndIsAlive(Actor* actor) const;

	Actor*			SpawnActor(SpawnInfo const& spawnInfo); // a reference could not be reassigned after assigned
	Actor*			SpawnActorAndAddToMapActorList(SpawnInfo const& spawnInfo);
	Actor*			SpawnBulletHitOnEnvirnment(Vec3 pos);
	Actor*			SpawnBloodSplatterOnHitEnemy(Vec3 pos);

	Actor*			 GetActorByUID(ActorUID const UID) const;
	ActorDefinition* GetActorDefByString(std::string actorName) const;

	void			SpawnPlayersAndPossessedByPlayerControllers();
	AIController* SpawnAIControllerAndPossessEnemy(Actor* actorPtr);
	void		  SpawnShieldForActor(Actor* shieldedActor);

	// victory requirement
	void	CheckIfPlayerReachDestination();

	// weapon firing
	ActorPtrList GetActorsOfDifferentFaction(ActorFaction faction);
	ActorPtrList GetActorsExceptSelf(Actor* myself);
	Actor* CollisionTestForRaycastWeaponFiring(Vec3 rayStart, Vec3 fwdNormal, float rayDist, Actor* attacker, ActorPtrList enemies);
	Actor* LockingTestForLockOnWeaponFiring(Vec3 rayStart, Vec3 fwdNormal, float rayDist, Actor* attacker, ActorPtrList enemies, Timer* lockOnTImer, Player* player = nullptr);
	Actor* RaycastWeaponTestForActorList(Vec3 const& rayStart, Vec3 fwdNormal, float rayDist, ActorPtrList actorList, float collisionScale = 1.f) const;

	Actor*	GetClosestVisibleEnemy(Actor* toThisActor);
	Actor*  RaycastActorList(Vec3 const& rayStart, float rayDist, ActorPtrList actorList) const; // get the closest actor in the list without obstacle in the middle
	Actor*	DebugPossessNext(Actor* currentActor);

	void	GenerateInitialActors();
	void	UpdateAllActors();
	void	UpdatePlayerController();
	void	RenderAllActors() const;
	void	DeleteDestoryedActors();

	// VFX settings
	float m_ricochetRadius = 0.018f;
	float m_ricochetHeight = 0.48f;
	float m_ricochetDuration = 0.06f;
	FloatRange m_VFXrange = FloatRange(0.f, 10.f);

	// render vectors
	LightingConstants*	 m_mapLightingSettings;
	Shader*				 m_shader;
	Texture*			 m_tileTexture;
	SpriteSheet*		 m_spriteSheet;

	std::vector<Vertex_PCUTBN> m_vertexPCUTBNs;
	VertexBuffer* m_vertexBuffer;
	std::vector<unsigned int>  m_indexArray;
	IndexBuffer* m_indexBuffer;

protected:
	static unsigned int const MAX_ACTOR_SALT = 0x0000FFFEu;
	unsigned int m_actorSalt = MAX_ACTOR_SALT;
};
