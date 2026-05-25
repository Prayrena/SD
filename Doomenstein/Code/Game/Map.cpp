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
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/App.hpp"
#include "Game/Player.hpp"
#include "Game/Tile.hpp"

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
extern InputSystem* g_theInput;

std::vector<MapDefinition> MapDefinition::s_mapDefs;

ActorFaction SpawnInfo::GetFactionByString(std::string str)
{
	if (str == "Demon") { return ActorFaction::DEMON; }
	if (str == "Marine") { return ActorFaction::MARINE; }
	ERROR_AND_DIE(Stringf("Could not find matching faction by \"%s\"", str.c_str()));
}

MapDefinition* const MapDefinition::GetByName(std::string const& name)
{
	for (int i = 0; i < (int)s_mapDefs.size(); ++i)
	{
		if ( name == s_mapDefs[i].m_name)
		{
			return &s_mapDefs[i];
		}
	}

	// if not found, return nullptr
	return nullptr;
}

void MapDefinition::InitializeMapDefs()
{
	XmlDocument mapDefXml;
	char const* filePath = "Data/Definitions/MapDefinitions.xml";
	XmlResult result = mapDefXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("failed to load xml file"));

	XmlElement* rootElement = mapDefXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "map definition root Element is nullPtr");

	XmlElement* mapDefElement = rootElement->FirstChildElement();

	while (mapDefElement)
	{
		// read map info
		std::string elementName = mapDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "MapDefinition", Stringf("root cant matchup with the name"));
		MapDefinition* newMapDef = new MapDefinition(*mapDefElement);// calls the constructor function of TileTypeDefinition

		// read spawn info
		XmlElement* spawnInfoElement = mapDefElement->FirstChildElement();
		std::string child1ElementName = spawnInfoElement->Name();
		GUARANTEE_OR_DIE(child1ElementName == "SpawnInfos", Stringf("child name cant matchup with the \"SpawnInfos\""));

		while (spawnInfoElement)
		{
			spawnInfoElement = spawnInfoElement->FirstChildElement();
			std::string child2ElementName = spawnInfoElement->Name();
			GUARANTEE_OR_DIE(child2ElementName == "SpawnInfo", Stringf("child name cant matchup with the \"SpawnInfos\""));
			while (spawnInfoElement)
			{
				SpawnInfo* newSpawnInfoDef = new SpawnInfo(*spawnInfoElement);// calls the constructor function of TileTypeDefinition
				newMapDef->m_spawnInfos.push_back(*newSpawnInfoDef);
				spawnInfoElement = spawnInfoElement->NextSiblingElement();
			}
		}
		s_mapDefs.push_back(*newMapDef); // todo:??? why this need to be after while spawnInfoElement
		mapDefElement = mapDefElement->NextSiblingElement();
	}
}

void MapDefinition::ClearDefinitions()
{
	s_mapDefs.clear();
}

//Actor* MapDefinition::GetActorByUID(ActorUID const uid) const
//{
//	// check UID is valid
//	// get UID index
//	// get actor at index
//	// if null, return nullptr
//	// if actor at index UID != UID, return null
//	// else return actor at index
//}

SpawnInfo::SpawnInfo(XmlElement const& spawnInfoElement)
{
	std::string name = ParseXmlAttribute(spawnInfoElement, "actor", "Named actor not found");
	m_actorDef = ActorDefinition::GetActorDefByString(name);
	name = ParseXmlAttribute(spawnInfoElement, "faction", "faction not found");
	if (name != "faction not found")
	{
		m_actorFaction = ActorDefinition::GetActorFactionByString(name);
	}
	m_position = ParseXmlAttribute(spawnInfoElement, "position", Vec3());
	m_velocity = ParseXmlAttribute(spawnInfoElement, "velocity", Vec3());
	m_orientation = ParseXmlAttribute(spawnInfoElement, "orientation", EulerAngles());
}

SpawnInfo::SpawnInfo(ActorDefinition* actorDef, Vec3 pos, Vec3 velocity, EulerAngles orientation)
	: m_actorDef(actorDef)
	, m_position(pos)
	, m_velocity(velocity)
	, m_orientation(orientation)
{
	m_actorFaction = m_actorDef->m_faction;
}

// write the map definition based on xml element
MapDefinition::MapDefinition(XmlElement const& mapDefElement)
{
	m_name = ParseXmlAttribute(mapDefElement, "name", "Not found in Xml");

	std::string shaderPath = ParseXmlAttribute(mapDefElement, "shader", "Not found in Xml");
	m_shader = g_theRenderer->CreateOrGetShader(shaderPath.c_str());

	m_mapImagePath = ParseXmlAttribute(mapDefElement, "image", "Not found in Xml");
	m_image = new Image(m_mapImagePath.c_str());

	std::string spriteSheetPath = ParseXmlAttribute(mapDefElement, "spriteSheetTexture", "Not found in Xml");
	m_spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile(spriteSheetPath.c_str());

	m_spriteSheetCount = ParseXmlAttribute(mapDefElement, "spriteSheetCellCount", IntVec2(0, 0));
	m_spriteSheet = new SpriteSheet(*m_spriteSheetTexture, m_spriteSheetCount);
}

// MapDefinition::~MapDefinition()
// {
// 	delete m_image;
// 	delete m_spriteSheet;
// }

Map::Map(MapDefinition& inputMapDefinition)
{
	m_mapDefinition = &inputMapDefinition; // todo: *m_mapDefinition = inputMapDefinition will have an error
	m_image = inputMapDefinition.m_image;
	m_dimensions = m_image->GetDimensions();
	m_spriteSheet = inputMapDefinition.m_spriteSheet;
	m_tileTexture = inputMapDefinition.m_spriteSheetTexture;
	m_shader = inputMapDefinition.m_shader;

	m_mapLightingSettings = new LightingConstants(Vec3(2.f, 1.f, -1.f), 0.85f, 0.35f);
}

Map::~Map()
{
	delete m_indexBuffer;
	m_indexBuffer = nullptr;

	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;
}

void Map::Startup()
{
	ReadTexelInfoFromImageAndSetTiles();
	AddVertsAndIndexesForAllTilesInMap();
	CreateVertexIndexBufferAndCopyFromCPUtoGPU();

	GenerateInitialActors();
}

void Map::Update()
{
	UpdateAllActors();
	UpdatePlayerController();
	UpdateKeyAndControllers();
	UpdatePhysicsCollisions();
	DeleteDestoryedActors();
	CheckIfPlayerReachDestination();
}

void Map::ReadTexelInfoFromImageAndSetTiles()
{
	m_mapDefinition->m_mapSize = m_image->GetDimensions();
	int width = m_mapDefinition->m_mapSize.x;
	int height = m_mapDefinition->m_mapSize.y;
	m_tiles.resize(width * height);

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int texelIndex = (y * width) + x;
			Rgba8 texel = m_image->m_texelRgba8Data[texelIndex];
			if (texel.a == 0) // overlook the texel if it is opaque
			{
				break;
			}
			else
			{
				// look through all tile type, see if there is any type has the same texel color rgb
				for (int tileTypeIndex = 0; tileTypeIndex < (int)(TileTypeDefinition::s_tileDefs.size()); ++tileTypeIndex)
				{
					TileTypeDefinition& tileType = TileTypeDefinition::s_tileDefs[tileTypeIndex];
					if (texel.r == tileType.m_mapImagePixelColor.r &&
						texel.g == tileType.m_mapImagePixelColor.g &&
						texel.b == tileType.m_mapImagePixelColor.b)
					{
						SetTileType(x, y, tileType.m_name);
						break;
					}
				}
				if (!m_tiles[texelIndex].m_tileDef)
				{
					ERROR_AND_DIE(Stringf("No tile type found has the same pixel color in %s", m_mapDefinition->m_mapImagePath.c_str()));
				}
			}
		}
	}
}

void Map::SetTileType(int tileX, int tileY, std::string type)
{
	int tileIndex = tileX + (tileY * m_dimensions.x);
	m_tiles[tileIndex].SetTileCoordsAndType(IntVec2(tileX, tileY), type);
}

void Map::AddVertsAndIndexesForAllTilesInMap()
{
	for (int tileIndex = 0; tileIndex < (int)m_tiles.size(); ++tileIndex)
	{
		Tile & tile = m_tiles[tileIndex];

		AABB2 tileBounds = tile.GetBounds();
		Rgba8 spriteColor = tile.GetColor();
		TileTypeDefinition tileDef = tile.GetDef();

		// get the block bounds based on the info of the tile
		// todo: some of the tiles are half tall just to cover the demon's head
		AABB3 blockBounds = GetTileBlockBounds(tileIndex);
		tile.SetBlockBounds(blockBounds); // define each tile with 3D size by the map

		Vec3 BBL;
		Vec3 BBR;
		Vec3 BTR;
		Vec3 BTL;
		Vec3 FBL;
		Vec3 FBR;
		Vec3 FTR;
		Vec3 FTL;
		blockBounds.GetAllEightPointsOfTheCorners(BBL, BBR, BTR, BTL, FBL, FBR, FTR, FTL);

		if (tile.IsSolid()) // this is the wall
		{
			AABB2 const wallUVs = tileDef.GetTileTextureUVsOnSpriteSheet(tileDef.m_wallSpriteCoords, m_spriteSheet);

			// for each wall block, we will construct four quads
			AddVertsForQuad3D(m_vertexPCUTBNs, m_indexArray, FBL, FBR, FTR, FTL, Rgba8::WHITE, wallUVs);
			AddVertsForQuad3D(m_vertexPCUTBNs, m_indexArray, FBR, BBR, BTR, FTR, Rgba8::WHITE, wallUVs);
			AddVertsForQuad3D(m_vertexPCUTBNs, m_indexArray, BBR, BBL, BTL, BTR, Rgba8::WHITE, wallUVs);
			AddVertsForQuad3D(m_vertexPCUTBNs, m_indexArray, BBL, FBL, FTL, BTL, Rgba8::WHITE, wallUVs);

			if (tile.m_tileDef->m_halfHeight)
			{
				AABB2 ceilUVs = tileDef.GetTileTextureUVsOnSpriteSheet(tileDef.m_ceilingSpriteCoords, m_spriteSheet);

				BTL += Vec3(0.f, 0.f, m_mapDefinition->m_ceilingHeight * 0.5f);
				BTR += Vec3(0.f, 0.f, m_mapDefinition->m_ceilingHeight * 0.5f);
				FTR += Vec3(0.f, 0.f, m_mapDefinition->m_ceilingHeight * 0.5f);
				FTL += Vec3(0.f, 0.f, m_mapDefinition->m_ceilingHeight * 0.5f);
				AddVertsForQuad3D(m_vertexPCUTBNs, m_indexArray, BTL, BTR, FTR, FTL, Rgba8::WHITE, ceilUVs);
			}
		}
		else // this is the floor and ceiling
		{
			AABB2 floorUVs = tileDef.GetTileTextureUVsOnSpriteSheet(tileDef.m_floorSpriteCoords, m_spriteSheet);
			AABB2 ceilUVs = tileDef.GetTileTextureUVsOnSpriteSheet(tileDef.m_ceilingSpriteCoords, m_spriteSheet);

			// we will only have to construct two quad - top and bottom
			AddVertsForQuad3D(m_vertexPCUTBNs, m_indexArray, FBL, FBR, BBR, BBL, Rgba8::WHITE, floorUVs);
			AddVertsForQuad3D(m_vertexPCUTBNs, m_indexArray, BTL, BTR, FTR, FTL, Rgba8::WHITE, ceilUVs);
		}
	}
}

void Map::CreateVertexIndexBufferAndCopyFromCPUtoGPU()
{
	// create vertex buffer and index buffer
	m_vertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_vertexPCUTBNs.size()), sizeof(Vertex_PCUTBN));
	m_indexBuffer = g_theRenderer->CreateIndexBuffer((size_t)(m_indexArray.size()));

	size_t vertexSize = sizeof(Vertex_PCUTBN);
	size_t vertexArrayDataSize = (m_vertexPCUTBNs.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_vertexPCUTBNs.data(), vertexArrayDataSize, m_vertexBuffer);

	size_t indexSize = sizeof(int);
	size_t indexArrayDataSize = m_indexArray.size() * indexSize;
	g_theRenderer->CopyCPUToGPU(m_indexArray.data(), indexArrayDataSize, m_indexBuffer);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// update entities by the entity type order in the enum

/// <Collisions Between Tiles and Entities>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Map::UpdatePhysicsCollisions()
{
	CollideActors();
	CollideActorsWithMap();
}

void Map::CollideActors()
{
	ActorPtrList& actors = m_actorList;

	for (int i = 0; i < (int)actors.size(); i++)
	{
		if (CheckIfActorExistAndIsAlive(actors[i])) // collide only when the actor is alive
		{
			for (int j = (i + 1); j < (int)actors.size(); j++)
			{
				if (CheckIfActorExistAndIsAlive(actors[j]))
				{
					// if the entity is check the physics collision with itself, skip
					if (actors[i] == actors[j])
					{
						continue;
					}
					else
					{
						// if both actors do collide with actors in definition
						if (actors[i]->m_actorDef->m_collidesWithActors || actors[j]->m_actorDef->m_collidesWithActors)
						{
							CollideActors(actors[i], actors[j]);
						}
					}
				}
			}
		}
	}
}

void Map::CollideActors(Actor* A, Actor* B)
{
	// If two actors overlap on the z - axis, then push their 2D discs out of each other

	if (DoActorsOverlapInSpace(*A, *B))
	{

		// if the energy shield overlap with its owner, if so, no collision detection
		if (A->m_actorDef->m_bulletproof || B->m_actorDef->m_bulletproof)
		{
			if (GetActorByUID(A->m_shieldOwnerID) == B || GetActorByUID(B->m_shieldOwnerID) == A)
			{
				return;
			}
		}

		// // if both actors are static, do nothing
		// if (A->m_isStatic && B->m_isStatic)
		// {
		// 	return;
		// }
		// // if one is static, another is movable, push movable one out of static one
		// if (A->m_isStatic && !B->m_isStatic)
		// {
		// 	PushMovableActorOutOfStaticActor(*B, *A);
		// }
		// else if (!A->m_isStatic && B->m_isStatic)
		// {
		// 	PushMovableActorOutOfStaticActor(*A, *B);
		// }
		// // if both are movable, push them out of each other
		// else if (!A->m_isStatic && !B->m_isStatic)
		// {
		// 	PushActorsOutOfEachOtherInXY(*A, *B);
		// }
		// 
		// if both actors are static, do nothing

		// first going to check if A or B is the owner to the other one
		// if so, skip collision
		if (A->m_ownerUID != ActorUID::INVALID || B->m_ownerUID != ActorUID::INVALID)
		{
			Actor* AOwner = GetActorByUID(A->m_ownerUID);
			Actor* BOwner = GetActorByUID(B->m_ownerUID);
			if (AOwner)
			{
				if (AOwner == B)
				{
					return;
				}
			}
			if (BOwner)
			{
				if (BOwner == A)
				{
					return;
				}
			}
		}

		// for Gold, if one is shield, the other is projectile, do on collide
		if (A->m_actorDef->m_bulletproof && B->m_actorDef->m_isProjectile)
		{
			A->OnCollide(B);
			B->OnCollide(A);
			return;
		}

		// shield do not collide with any other actors
		if (A->m_actorDef->m_bulletproof || B->m_actorDef->m_bulletproof)
		{
			return;
		}

		// todo: 4/8 whether simulated is not related to collide with actor or map?
		if (!A->m_actorDef->m_simulated && !B->m_actorDef->m_simulated)
		{
			return;
		}
		// if one is static, another is movable, push movable one out of static one
		if (!A->m_actorDef->m_simulated && B->m_actorDef->m_simulated)
		{
			PushMovableActorOutOfStaticActor(*B, *A);
		}
		else if (A->m_actorDef->m_simulated && !B->m_actorDef->m_simulated && !B->m_actorDef->m_isHomingMissile)
		{
			PushMovableActorOutOfStaticActor(*A, *B);
		}
		// if both are movable, push them out of each other
		else if (A->m_actorDef->m_simulated && B->m_actorDef->m_simulated)
		{
			PushActorsOutOfEachOtherInXY(*A, *B);
		}
		// suppose A is demon, B is projectile
		A->OnCollide(B); // do nothing
		B->OnCollide(A); // projectile die, demon has impulse, damage
	}
}

void Map::CollideActorsWithMap()
{
	ActorPtrList& actors = m_actorList;

	for (int i = 0; i < (int)actors.size(); i++)
	{
		if (CheckIfActorExistAndNotDestroyed(actors[i]))
		{
			if (actors[i]->m_actorDef->m_collidesWithWorld)
			{
				CollideActorWithMap(actors[i]);
			}
		}
	}
}

bool Map::DoActorsOverlapInSpace(Actor const& a, Actor const& b)
{
	// each collision Zcylinder is in local space
	// we need to transfer the Zcylinder in world space
	ZCylinder collisionA = a.GetCylinderCollisionInWorldSpace();
	ZCylinder collisionB = b.GetCylinderCollisionInWorldSpace();

	if (DoZCylindersOverlap3D(collisionA, collisionB))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Map::PushActorsOutOfEachOtherInXY(Actor& actorA, Actor& actorB)
{
	// each collision Zcylinder is in local space
	// we need to transfer the Zcylinder in world space
	ZCylinder collisionA = actorA.GetCylinderCollisionInWorldSpace();
	ZCylinder collisionB = actorB.GetCylinderCollisionInWorldSpace();

	PushDiscOutOfEachOther2D(collisionA.CenterXY, collisionA.Radius,
		collisionB.CenterXY, collisionB.Radius);

	// transform the new position of the Zcylinder collision to local space to set m_position
	// Mat44 worldToLocalAMatrix = actorA.GetModelMatrix().GetOrthonormalInverse();
	// actorA.m_position.x = worldToLocalAMatrix.TransformVectorQuantity3D(Vec3(collisionA.CenterXY, 0.f)).x;
	// actorA.m_position.y = worldToLocalAMatrix.TransformVectorQuantity3D(Vec3(collisionA.CenterXY, 0.f)).y;
	// 
	// Mat44 worldToLocalBMatrix = actorB.GetModelMatrix().GetOrthonormalInverse();
	// actorB.m_position.x = worldToLocalBMatrix.TransformVectorQuantity3D(Vec3(collisionB.CenterXY, 0.f)).x;
	// actorB.m_position.y = worldToLocalBMatrix.TransformVectorQuantity3D(Vec3(collisionB.CenterXY, 0.f)).y;

	actorA.m_position.x = collisionA.CenterXY.x;
	actorA.m_position.y = collisionA.CenterXY.y;

	actorB.m_position.x = collisionB.CenterXY.x;
	actorB.m_position.y = collisionB.CenterXY.y;
}

void Map::PushMovableActorOutOfStaticActor(Actor& movableActor, Actor& staticActor)
{
	// each collision Zcylinder is in local space
	// we need to transfer the Zcylinder in world space
	ZCylinder collisionA = movableActor.GetCylinderCollisionInWorldSpace();
	ZCylinder collisionB = staticActor.GetCylinderCollisionInWorldSpace();

	PushDiscOutOfFixedDisc2D(collisionA.CenterXY, collisionA.Radius,
		collisionB.CenterXY, collisionB.Radius);

	// transform the new position of the Zcylinder collision to local space to set m_position
	Mat44 worldToLocalMatrix = movableActor.GetModelMatrix().GetOrthonormalInverse();
	movableActor.m_position.x = worldToLocalMatrix.TransformVectorQuantity3D(Vec3(collisionA.CenterXY, 0.f)).x;
	movableActor.m_position.y = worldToLocalMatrix.TransformVectorQuantity3D(Vec3(collisionA.CenterXY, 0.f)).y;
}

void Map::CollideActorWithMap(Actor* actor)
{
	ZCylinder collision = actor->GetCylinderCollisionInWorldSpace();
	bool collideWithMap = false;

	// push actor upwards above the floor
	if (collision.MinMaxZ.m_min < (m_mapDefinition->m_floorHeight + worldZCollisionOffset))
	{
		actor->m_position.z = (m_mapDefinition->m_floorHeight + worldZCollisionOffset);
		collideWithMap = true;
	}
	
	// push actor downwards under the ceiling
	if (collision.MinMaxZ.m_max > ((m_mapDefinition->m_ceilingHeight) - worldZCollisionOffset))
	{
		float actorHeight = actor->m_collision.MinMaxZ.GetRangeLength();
		actor->m_position.z = (m_mapDefinition->m_ceilingHeight - actorHeight - worldZCollisionOffset);
		collideWithMap = true;
	}

	// push actor out of walls
	IntVec2 tileCoordsEntityIsOn = GetTileCoordsInMap_For_WorldPos(actor->m_position);
	// push the entity from four sides first
	if (PushActorOutOfTileIfSolid(*actor, tileCoordsEntityIsOn + STEP_EAST))
	{
		collideWithMap = true;
	}
	if (PushActorOutOfTileIfSolid(*actor, tileCoordsEntityIsOn + STEP_SOUTH))
	{
		collideWithMap = true;
	}
	if (PushActorOutOfTileIfSolid(*actor, tileCoordsEntityIsOn + STEP_WEST))
	{
		collideWithMap = true;
	}
	if (PushActorOutOfTileIfSolid(*actor, tileCoordsEntityIsOn + STEP_NORTH))
	{
		collideWithMap = true;
	}

	// then push the entity from four corners
	if (PushActorOutOfTileIfSolid(*actor, tileCoordsEntityIsOn + STEP_EASTSOUTH))
	{
		collideWithMap = true;
	}
	if (PushActorOutOfTileIfSolid(*actor, tileCoordsEntityIsOn + STEP_SOUTHWEST))
	{
		collideWithMap = true;
	}
	if (PushActorOutOfTileIfSolid(*actor, tileCoordsEntityIsOn + STEP_WESTNORTH))
	{
		collideWithMap = true;
	}
	if (PushActorOutOfTileIfSolid(*actor, tileCoordsEntityIsOn + STEP_NORTHEAST))
	{
		collideWithMap = true;
	}

	if (collideWithMap)
	{
		actor->OnCollide(nullptr);
	}
}

bool Map::PushActorOutOfTileIfSolid(Actor& actor, IntVec2 const& tileCoords)
{
	if (IsTileOutOfBounds(tileCoords))
	{
		return false;
	}

	if (!IsTileSolid(tileCoords))// is the tile is not water or solid tile, the tile will not push the entity
	{
		return false;
	}

	// see if the wall is half height and block by the 
	if (IsTileHalfHeight(tileCoords) && ((actor.GetCylinderCollisionInWorldSpace().MinMaxZ.m_min >= m_mapDefinition->m_ceilingHeight * 0.5f)))
	{
		return false;
	}

	AABB2 tileBounds = GetTileBounds(tileCoords);
	Vec2 newPosition = Vec2(actor.m_position);
	bool hitWall = PushDiscOutOfFixedAABB2D(newPosition, actor.m_collision.Radius, tileBounds);
	actor.m_position.x = newPosition.x;
	actor.m_position.y = newPosition.y;
	return hitWall;
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

bool Map::IsTileHalfHeight(IntVec2 const& tileIndexCoords_InMap) const
{
	if (IsTileOutOfBounds(tileIndexCoords_InMap))
	{
		return false;// if is asking the tile off the map return it is solid for better collision and in case m_tiles[tileIndex] is out of range 
	}
	int tileIndex = GetTileIndex_For_TileCoordinates(tileIndexCoords_InMap);
	Tile tile = m_tiles[tileIndex];
	if (tile.m_tileDef->m_halfHeight)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Map::UpdateKeyAndControllers()
{
	// debug render assignment stuff
	// if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	// {
	// 	ShootRaycastForCollisionTest(0.25f);
	// }
}

void Map::ShootRaycastForCollisionTest(float rayDist)
{
	Vec3 rayStart = g_theGame->m_playersList[0]->m_position;
	Vec3 rayDisp = Vec3(rayDist, 0.f, 0.f);
	Vec3 rayFwdNormal = g_theGame->m_playersList[0]->GetModelMatrix().TransformVectorQuantity3D(rayDisp).GetNormalized();
	Vec3 rayEnd = rayStart + rayFwdNormal * rayDist;
	// DebugAddWorldLine(rayStart, rayEnd, 0.01f, 10.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);

	RaycastResult3D result = RaycastAll(rayStart, rayFwdNormal, rayDist);
	if (result.m_didImpact)
	{
		// DebugAddWorldPoint(result.m_impactPos, 0.06f, 10.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
		Vec3 arrowTip = result.m_impactPos + result.m_impactNormal * 0.3f;
		// DebugAddWorldArrow(result.m_impactPos, arrowTip, 0.03f, 10.f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);
	}
}

Actor* Map::CollisionTestForRaycastWeaponFiring(Vec3 rayStart, Vec3 fwdNormal, float rayDist, Actor* attacker, ActorPtrList enemies)
{
	UNUSED(attacker);

	Vec3 rayEnd = rayStart + fwdNormal * rayDist;
	
	Actor* closestHitActor = RaycastWeaponTestForActorList(rayStart, fwdNormal, rayDist, enemies);

	// float rayDisplayDuration = 1.f;
	// float hitDisplayDuration = 3.f;

	// if the raycast could hit the marine collision
	if (closestHitActor)
	{
		// we are going to check if the raycast going to hit wall or ceil or floor first
		RaycastResult3D resultInZ = RaycastWorldZ(rayStart, fwdNormal, rayDist);
		RaycastResult3D resultForTiles = FastRaycastForVoxelGrids(rayStart, fwdNormal, rayDist);

		// get the shortest impact dist
		float shortestDist = rayDist;
		float actorDist = closestHitActor->m_raycastResult.m_impactDist;
		if (resultInZ.m_impactDist < shortestDist)
		{
			shortestDist = resultInZ.m_impactDist;
		}
		if (actorDist < shortestDist)
		{
			shortestDist = actorDist;
		}
		if (resultForTiles.m_impactDist < shortestDist)
		{
			shortestDist = resultForTiles.m_impactDist;
		}

		// see which dist it belongs to
		if (shortestDist == rayDist)
		{
			// DebugAddWorldLine(rayStart, rayEnd, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			return nullptr;
		}
		else if (shortestDist == resultInZ.m_impactDist)
		{
			// DebugAddWorldLine(rayStart, resultInZ.m_impactPos, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			// hit point and reflect arrow
			// DebugAddWorldPoint(resultInZ.m_impactPos, 0.06f, hitDisplayDuration, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			// Vec3 arrowTip = resultInZ.m_impactPos + resultInZ.m_impactNormal * 0.3f;
			// DebugAddWorldArrow(resultInZ.m_impactPos, arrowTip, 0.03f, hitDisplayDuration, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);
			AddRicochetVFX(resultInZ.m_impactPos, resultInZ.m_impactNormal, m_ricochetRadius, m_ricochetHeight, m_ricochetDuration, Rgba8::YELLOW, Rgba8::YELLOW);

			SpawnBulletHitOnEnvirnment(resultInZ.m_impactPos);

			return nullptr;
		}
		else if (shortestDist == actorDist)
		{
			// DebugAddWorldLine(rayStart, closestHitActor->m_raycastResult.m_impactPos, 0.01f, rayDisplayDuration, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::X_RAY);

			// hit point and reflect arrow
			RaycastResult3D& actorRaycastResult = closestHitActor->m_raycastResult;
			// DebugAddWorldPoint(actorRaycastResult.m_impactPos, 0.06f, hitDisplayDuration, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			// Vec3 arrowTip = actorRaycastResult.m_impactPos + actorRaycastResult.m_impactNormal * 0.3f;
			// DebugAddWorldArrow(actorRaycastResult.m_impactPos, arrowTip, 0.03f, hitDisplayDuration, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);
			
			if (closestHitActor->m_actorDef->m_bulletproof) // hit shield
			{
				// generate the shield bullet reflect animation
				// do dot product see if hit position hit on side or in front
				if (DotProduct3D(closestHitActor->m_raycastResult.m_impactNormal, closestHitActor->m_orientation.GetForwardIBasis()) > cosf(ConvertDegreesToRadians(60.f)))
				{
				Vec3 normal = g_rng->GetRandomDirectionInCone(closestHitActor->m_raycastResult.m_impactNormal, m_VFXrange);
				AddRicochetVFX(closestHitActor->m_raycastResult.m_impactPos, normal, m_ricochetRadius, m_ricochetHeight, m_ricochetDuration, Rgba8::YELLOW, Rgba8::YELLOW);
				}
				else // hit the enemy
				{
					SpawnBloodSplatterOnHitEnemy(actorRaycastResult.m_impactPos);
				}
			}
			else // hit the enemy
			{
				SpawnBloodSplatterOnHitEnemy(actorRaycastResult.m_impactPos);
			}

			return closestHitActor;
		}
		else if (shortestDist == resultForTiles.m_impactDist)
		{
			// DebugAddWorldLine(rayStart, resultForTiles.m_impactPos, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			// hit point and reflect arrow
			// DebugAddWorldPoint(resultForTiles.m_impactPos, 0.06f, hitDisplayDuration, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			// Vec3 arrowTip = resultForTiles.m_impactPos + resultForTiles.m_impactNormal * 0.3f;
			// DebugAddWorldArrow(resultForTiles.m_impactPos, arrowTip, 0.03f, hitDisplayDuration, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);
			AddRicochetVFX(resultForTiles.m_impactPos, resultForTiles.m_impactNormal, m_ricochetRadius, m_ricochetHeight, m_ricochetDuration, Rgba8::YELLOW, Rgba8::YELLOW);

			SpawnBulletHitOnEnvirnment(resultForTiles.m_impactPos);

			return nullptr;
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		// we are going to check if the raycast going to hit wall or ceil or floor first
		RaycastResult3D resultInZ = RaycastWorldZ(rayStart, fwdNormal, rayDist);
		RaycastResult3D resultForTiles = FastRaycastForVoxelGrids(rayStart, fwdNormal, rayDist);

		// get the shortest impact dist
		float shortestDist = rayDist;
		if (resultInZ.m_impactDist < shortestDist)
		{
			shortestDist = resultInZ.m_impactDist;
		}
		if (resultForTiles.m_impactDist < shortestDist)
		{
			shortestDist = resultForTiles.m_impactDist;
		}

		// see which dist it belongs to
		if (shortestDist == rayDist)
		{
			// DebugAddWorldLine(rayStart, rayEnd, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			return nullptr;
		}
		else if (shortestDist == resultInZ.m_impactDist)
		{
			// DebugAddWorldLine(rayStart, resultInZ.m_impactPos, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			// hit point and reflect arrow
			// DebugAddWorldPoint(resultInZ.m_impactPos, 0.06f, hitDisplayDuration, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			// Vec3 arrowTip = resultInZ.m_impactPos + resultInZ.m_impactNormal * 0.3f;
			// DebugAddWorldArrow(resultInZ.m_impactPos, arrowTip, 0.03f, hitDisplayDuration, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);
			AddRicochetVFX(resultInZ.m_impactPos, resultInZ.m_impactNormal, m_ricochetRadius, m_ricochetHeight, m_ricochetDuration, Rgba8::YELLOW, Rgba8::YELLOW);
			SpawnBulletHitOnEnvirnment(resultInZ.m_impactPos);

			return nullptr;
		}
		else if (shortestDist == resultForTiles.m_impactDist)
		{
			// DebugAddWorldLine(rayStart, resultForTiles.m_impactPos, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			// hit point and reflect arrow
			// DebugAddWorldPoint(resultForTiles.m_impactPos, 0.06f, hitDisplayDuration, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			// Vec3 arrowTip = resultForTiles.m_impactPos + resultForTiles.m_impactNormal * 0.3f;
			// DebugAddWorldArrow(resultForTiles.m_impactPos, arrowTip, 0.03f, hitDisplayDuration, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);
			AddRicochetVFX(resultForTiles.m_impactPos, resultForTiles.m_impactNormal, m_ricochetRadius, m_ricochetHeight, m_ricochetDuration, Rgba8::YELLOW, Rgba8::YELLOW);

			SpawnBulletHitOnEnvirnment(resultForTiles.m_impactPos);

			return nullptr;
		}
		else
		{
			return nullptr;
		}
	}
}

Actor* Map::LockingTestForLockOnWeaponFiring(Vec3 rayStart, Vec3 fwdNormal, float rayDist, Actor* attacker, ActorPtrList enemies, Timer* lockOnTImer, Player* player /*= nullptr*/)
{
	// todo: use the actor to see it has a player controller, if yes, get it index, set the player's bool to display the locking ring, check 
	UNUSED(lockOnTImer);
	UNUSED(attacker);
	Vec3 rayEnd = rayStart + fwdNormal * rayDist;

	Actor* closestHitActor = RaycastWeaponTestForActorList(rayStart, fwdNormal, rayDist, enemies, 3.f);

	// float rayDisplayDuration = 0.02f;
	// float hitDisplayDuration = 0.01f;

	// if the raycast could hit the marine collision
	if (closestHitActor)
	{
		// we are going to check if the raycast going to hit wall or ceil or floor first
		RaycastResult3D resultInZ = RaycastWorldZ(rayStart, fwdNormal, rayDist);
		// RaycastResult3D resultForTiles = FastRaycastForVoxelGrids(rayStart, fwdNormal, rayDist);

		// get the shortest impact dist
		float shortestDist = rayDist;
		float actorDist = closestHitActor->m_raycastResult.m_impactDist;
		if (resultInZ.m_impactDist < shortestDist)
		{
			shortestDist = resultInZ.m_impactDist;
		}
		if (actorDist < shortestDist)
		{
			shortestDist = actorDist;
		}
		// if (resultForTiles.m_impactDist < shortestDist)
		// {
		// 	shortestDist = resultForTiles.m_impactDist;
		// }

		// see which dist it belongs to
		if (shortestDist == rayDist)
		{
			// DebugAddWorldLine(rayStart, rayEnd, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			if (player)
			{
				for (int i = 0; i < (int)g_theGame->m_playersList.size(); ++i)
				{
					Player*& playerController = g_theGame->m_playersList[i];
					if (playerController == player)
					{
						playerController->m_inLockingAimingMode = true;
						playerController->m_lockTargetPos = Vec2 (0.5f, 0.5f);
					}
				}
			}

			return nullptr;
		}
		else if (shortestDist == resultInZ.m_impactDist)
		{
			// DebugAddWorldLine(rayStart, resultInZ.m_impactPos, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			if (player)
			{
				for (int i = 0; i < (int)g_theGame->m_playersList.size(); ++i)
				{
					Player*& playerController = g_theGame->m_playersList[i];
					if (playerController == player)
					{
						playerController->m_inLockingAimingMode = true;
						playerController->m_lockTargetPos = Vec2(0.5f, 0.5f);
					}
				}
			}

			// hit point and reflect arrow
			// DebugAddWorldPoint(resultInZ.m_impactPos, 0.06f, hitDisplayDuration, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			// Vec3 arrowTip = resultInZ.m_impactPos + resultInZ.m_impactNormal * 0.3f;
			// DebugAddWorldArrow(resultInZ.m_impactPos, arrowTip, 0.03f, hitDisplayDuration, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);

			return nullptr;
		}
		else if (shortestDist == actorDist)
		{
			// DebugAddWorldLine(rayStart, closestHitActor->m_raycastResult.m_impactPos, 0.01f, rayDisplayDuration, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::X_RAY);

			// hit point and reflect arrow
			RaycastResult3D& actorRaycastResult = closestHitActor->m_raycastResult;
			// DebugAddWorldPoint(actorRaycastResult.m_impactPos, 0.06f, hitDisplayDuration, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			Vec3 arrowTip = actorRaycastResult.m_impactPos + actorRaycastResult.m_impactNormal * 0.3f;
			// DebugAddWorldArrow(actorRaycastResult.m_impactPos, arrowTip, 0.03f, hitDisplayDuration, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);

			// SpawnBloodSplatterOnHitEnemy(actorRaycastResult.m_impactPos); // spear's projectile will spawn the blood splatter
				// if player is using the lock on target, the raycast hit the enemy, there position will show on view port
			if (player)
			{
				for (int i = 0; i < (int)g_theGame->m_playersList.size(); ++i)
				{
					Player*& playerController = g_theGame->m_playersList[i];
					if (playerController == player)
					{
						playerController->m_inLockingAimingMode = true;
						Vec3 hittingTarget = closestHitActor->m_position + Vec3(0.f, 0.f, (closestHitActor->m_actorDef->m_physicsHeight + closestHitActor->m_actorDef->m_physicsFootHeight) * 0.5f);
						playerController->m_lockTargetPos = playerController->m_worldCamera.GetViewportNormolizedPositionForWorldPosition(hittingTarget);
					}
				}
			}

			return closestHitActor;
		}
		// else if (shortestDist == resultForTiles.m_impactDist)
		// {
		// 	DebugAddWorldLine(rayStart, resultForTiles.m_impactPos, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);
		// 
		// 	// hit point and reflect arrow
		// 	// DebugAddWorldPoint(resultForTiles.m_impactPos, 0.06f, hitDisplayDuration, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
		// 	// Vec3 arrowTip = resultForTiles.m_impactPos + resultForTiles.m_impactNormal * 0.3f;
		// 	// DebugAddWorldArrow(resultForTiles.m_impactPos, arrowTip, 0.03f, hitDisplayDuration, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);
		// 
		// 	return nullptr;
		// }
		// else
		// {
		// 	return nullptr;
		// }
	}
	else
	{
		// reset the player aiming indicator to the center of screen
		if (player)
		{
			for (int i = 0; i < (int)g_theGame->m_playersList.size(); ++i)
			{
				Player*& playerController = g_theGame->m_playersList[i];
				if (playerController == player)
				{
					playerController->m_inLockingAimingMode = true;
					// playerController->m_lockTargetPos = Vec2(0.5f, 0.5f);
				}
			}
		}

		// we are going to check if the raycast going to hit wall or ceil or floor first
		RaycastResult3D resultInZ = RaycastWorldZ(rayStart, fwdNormal, rayDist);
		RaycastResult3D resultForTiles = FastRaycastForVoxelGrids(rayStart, fwdNormal, rayDist);

		// get the shortest impact dist
		float shortestDist = rayDist;
		if (resultInZ.m_impactDist < shortestDist)
		{
			shortestDist = resultInZ.m_impactDist;
		}
		if (resultForTiles.m_impactDist < shortestDist)
		{
			shortestDist = resultForTiles.m_impactDist;
		}

		// see which dist it belongs to
		if (shortestDist == rayDist)
		{
			// DebugAddWorldLine(rayStart, rayEnd, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			return nullptr;
		}
		else if (shortestDist == resultInZ.m_impactDist)
		{
			// DebugAddWorldLine(rayStart, resultInZ.m_impactPos, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			// hit point and reflect arrow
			// DebugAddWorldPoint(resultInZ.m_impactPos, 0.06f, hitDisplayDuration, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			// Vec3 arrowTip = resultInZ.m_impactPos + resultInZ.m_impactNormal * 0.3f;
			// DebugAddWorldArrow(resultInZ.m_impactPos, arrowTip, 0.03f, hitDisplayDuration, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);

			return nullptr;
		}
		else if (shortestDist == resultForTiles.m_impactDist)
		{
			// DebugAddWorldLine(rayStart, resultForTiles.m_impactPos, 0.01f, rayDisplayDuration, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);

			// hit point and reflect arrow
			// DebugAddWorldPoint(resultForTiles.m_impactPos, 0.06f, hitDisplayDuration, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
			// Vec3 arrowTip = resultForTiles.m_impactPos + resultForTiles.m_impactNormal * 0.3f;
			// DebugAddWorldArrow(resultForTiles.m_impactPos, arrowTip, 0.03f, hitDisplayDuration, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USE_DEPTH);

			return nullptr;
		}
		else
		{
			return nullptr;
		}
	}
	return nullptr;
}

Actor* Map::RaycastWeaponTestForActorList(Vec3 const& rayStart, Vec3 fwdNormal, float rayDist, ActorPtrList actorList, float collisionScale /*= 1.f*/) const
{
	float shortestImpactDist = rayDist;
	Actor* closestActor = nullptr;

	if (!actorList.empty())
	{
		// collect all actor raycast results and get the shortest dist
		for (int i = 0; i < (int)actorList.size(); ++i)
		{
			ZCylinder collision = actorList[i]->GetCylinderCollisionInWorldSpace();
			collision.SetUniformScale(collisionScale); // todo: get the diameter in world and the distance on screen, match this two up

			RaycastResult3D result = RaycastVsCylinderZ3D(rayStart, fwdNormal, rayDist, collision);
			actorList[i]->m_raycastResult = result;
			if (result.m_didImpact)
			{
				if (result.m_impactDist < shortestImpactDist) // update the wanted shortest hit result when we get a new winner
				{
					closestActor = actorList[i];
					shortestImpactDist = result.m_impactDist;
				}
			}
		}

		// if we have a shortest impact dist
		if (shortestImpactDist != rayDist)
		{
			return closestActor;
		}
		else return nullptr;
	}
	else
	{
		return nullptr;
	}
}

RaycastResult3D Map::RaycastAll(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) const
{
	RaycastResult3D resultInZ = RaycastWorldZ(rayStart, rayFwdNormal, rayDist);
	RaycastResult3D resultForActors = RaycastWorldActors(rayStart, rayFwdNormal, rayDist);
	RaycastResult3D resultForTiles = FastRaycastForVoxelGrids(rayStart, rayFwdNormal, rayDist);

	// get the shortest impact dist
	float shortestDist = rayDist;
	if (resultInZ.m_impactDist < shortestDist)
	{
		shortestDist = resultInZ.m_impactDist;
	}
	if (resultForActors.m_impactDist < shortestDist)
	{
		shortestDist = resultForActors.m_impactDist;
	}
	if (resultForTiles.m_impactDist < shortestDist)
	{
		shortestDist = resultForTiles.m_impactDist;
	}

	// according to the shortest impact distant, return its raycast results
	Vec3 rayEnd = rayStart + rayFwdNormal * rayDist;

	RaycastResult3D missResult;
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

	if (shortestDist == rayDist)
	{
		return missResult;
	}
	else if (shortestDist == resultInZ.m_impactDist)
	{
		return resultInZ;
	}
	else if (shortestDist == resultForActors.m_impactDist)
	{
		return resultForActors;
	}
	else if (shortestDist == resultForTiles.m_impactDist)
	{
		return resultForTiles;
	}
	else
	{
		return missResult;
	}
}

RaycastResult3D Map::RaycastWorldXY(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) const
{
	return FastRaycastForVoxelGrids(rayStart, rayFwdNormal, rayDist);
}

RaycastResult3D Map::FastRaycastForVoxelGrids(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) const
{
	// Initialization setup
	Vec3 rayEnd = rayStart + rayFwdNormal * rayDist;
	bool impactSolidTile = false;

	// if it misses, we will use this result
	RaycastResult3D missResult;
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
	RaycastResult3D hitResult;

	hitResult.m_rayFwdNormal = rayFwdNormal;
	hitResult.m_rayStartPos = rayStart;
	hitResult.m_rayDist = rayDist;
	 
	// we are projecting the raycast on ground to discuss the situation
	// So for the rayDist, we also need to project it onto the ground
	Vec3 ray2DNormal = Vec3(rayFwdNormal.x, rayFwdNormal.y, 0.f).GetNormalized();
	if (rayFwdNormal.GetLength() == 0.f)
	{
		ERROR_AND_DIE("The input raycast forward normal length is 0.f");
	}
	float ray2DDist = rayDist * (Vec3(rayFwdNormal.x, rayFwdNormal.y, 0.f).GetLength() / rayFwdNormal.GetLength());

	// Get first tile and start pos
	IntVec2 originTile;
	originTile.x = int(floor(rayStart.x));
	originTile.y = int(floor(rayStart.y));
	IntVec2 currentTile = originTile;
	Vec3 currentPos = rayStart;

	if (IsTileSolid(originTile)) // starts inside a solid tile
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

	// calculate which grid wall the raycast is going to hit in X and Y for the next one
	int numXWall = 0;
	int numYWall = 0;
	// and we are also calculate the fraction from start to the wall impact in X and Y to rayDist for comparison
	bool  outOfDistance = false;
	float tx = 1.f;  // max fraction in X direction
	float ty = 1.f;  // max fraction in Y direction

	RaycastResult3D result = *(new RaycastResult3D());

	while (!outOfDistance && !impactSolidTile ) // if the testing point is not out of distance, we will loop through
	{
		tx = 1.f;
		ty = 1.f;
		// todo: precision issue
		// for marching each grid forward, we'll get the time travel in X and Y to see which is sooner
		// but before that, if the raycast is vertical or horizontal, we need to avoid case which divides 0.f
		if (rayFwdNormal.x == 0.f) // vertical raycast
		{
			// only need to calculate in Y direction
			if (rayFwdNormal.y > 0.f)
			{
				numYWall = (int)floor(currentPos.y + 1.0f);
				ty = (numYWall - rayStart.y) / (ray2DNormal.y * ray2DDist);
			}
			else if (rayFwdNormal.y < 0.f)
			{
				numYWall = (int)ceil(currentPos.y - 1.0);
				ty = (numYWall - rayStart.y) / (ray2DNormal.y * ray2DDist);
			}
		}
		else if (rayFwdNormal.y == 0.f) // horizontal raycast
		{
			// only need to calculate in X direction
			if (rayFwdNormal.x > 0.f)
			{
				numXWall = (int)floor(currentPos.x + 1.0);
				tx = (numXWall - rayStart.x) / (ray2DNormal.x * ray2DDist);
			}
			else if (rayFwdNormal.x < 0.f)
			{
				numXWall = (int)ceil(currentPos.x - 1.0);
				tx = (numXWall - rayStart.x) / (ray2DNormal.x * ray2DDist);
			}
		}
		else // we need to calculate both x and y direction
		{
			// calculate in X direction
			if (rayFwdNormal.x > 0.f)
			{
				numXWall = (int)floor(currentPos.x + 1.0);
				tx = (numXWall - rayStart.x) / (ray2DNormal.x * ray2DDist);
			}
			else if (rayFwdNormal.x < 0.f)
			{
				numXWall = (int)ceil(currentPos.x - 1.0);
				tx = (numXWall - rayStart.x) / (ray2DNormal.x * ray2DDist);
			}
			// calculate in Y direction
			if (rayFwdNormal.y > 0.f)
			{
				numYWall = (int)floor(currentPos.y + 1.0);
				ty = (numYWall - rayStart.y) / (ray2DNormal.y * ray2DDist);
			}
			else if (rayFwdNormal.y < 0.f)
			{
				numYWall = (int)ceil(currentPos.y - 1.0);
				ty = (numYWall - rayStart.y) / (ray2DNormal.y * ray2DDist);
			}
		}

		// compare the two and take the quickest one
		// and uses the time to calculate the new impact pos
		Vec3 impactPos;

		if (tx < ty)
		{
			if (rayFwdNormal.x >= 0.f)
			{
				++currentTile.x;
			}
			else
			{
				--currentTile.x;
			}
			currentPos.x = rayStart.x + ray2DNormal.x * ray2DDist * tx;

			if (tx <= 1.f && tx >= 0.f && IsTileSolid(currentTile) && !IsTileOutOfBounds(currentTile)) // hit
			{
				impactPos = rayStart + rayFwdNormal * rayDist * tx;
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = rayDist * tx;
				hitResult.m_impactPos = impactPos;

				// opposite of the raycast
				hitResult.m_impactNormal = Vec3(rayFwdNormal.x * (-1.f), 0.f, 0.f).GetNormalized();

				impactSolidTile = true;
			}
			else // miss
			{
				// check if the ray is out of ray dist
				tx > 1.f ? outOfDistance = true : outOfDistance = false;
			}
		}
		//else if (tx == ty) // should either take a step for X or Y direction
		//{
		//	++currentTile.x;
		//	impactPos = rayStart + rayFwdNormal * rayDist * tx;
		//	currentPos = impactPos;
		//
		//	if (tx <= 1 && tx >= 0 && IsTileSolid(currentTile) && TileIsOutOfBounds(currentTile)) // hit
		//	{
		//		hitResult.m_didImpact = true;
		//		hitResult.m_impactDist = rayDist * tx;
		//		hitResult.m_impactPos = impactPos;
		//
		//		// opposite of the raycast
		//		hitResult.m_rayFwdNormal = (rayFwdNormal * (-1.f));
		//
		//		impactSolidTile = true;
		//	}
		//	else // miss
		//	{
		//		tx > 1.f ? outOfDistance = true : outOfDistance = false;
		//	}
		//}
		else // take step towards y
		{
			if (rayFwdNormal.y >= 0.f)
			{
				// currentTile.y += ((int)floorf(currentPos.y + 1.f) - currentTile.y);
				++currentTile.y;
				// currentTile.y = (int)floorf(currentPos.y + 1.f);
			}
			else
			{
				// currentTile.y += ((int)ceilf(currentPos.y - 1.f) - currentTile.y);
				--currentTile.y;
				// currentTile.y = (int)ceilf(currentPos.y - 1.f);
			}
			//currentTile.y = numYWall;
			currentPos.y = rayStart.y + ray2DNormal.y * ray2DDist * ty;

			if (ty <= 1.f && ty >= 0.f && IsTileSolid(currentTile) && !IsTileOutOfBounds(currentTile)) // hit
			{
				impactPos = rayStart + rayFwdNormal * rayDist * ty;
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = rayDist * ty;
				hitResult.m_impactPos = impactPos;

				if (IsTileHalfHeight(currentTile))
				{
					if (impactPos.z > (m_mapDefinition->m_ceilingHeight * 0.5f))
					{
						ty > 1.f ? outOfDistance = true : outOfDistance = false;
					}
					else
					{
						// the impact is in Y direction, facing the raycast
						hitResult.m_impactNormal = Vec3(0.f, rayFwdNormal.y * (-1.f), 0.f).GetNormalized();

						impactSolidTile = true;
					}
				}
				else
				{
					// the impact is in Y direction, facing the raycast
					hitResult.m_impactNormal = Vec3(0.f, rayFwdNormal.y * (-1.f), 0.f).GetNormalized();

					impactSolidTile = true;
				}
			}
			else // miss
			{
				ty > 1.f ? outOfDistance = true : outOfDistance = false;
			}
		}
	}

	// for this result, if it is above the m_ceilingHeight, then it is not valid
	if (impactSolidTile && hitResult.m_impactPos.z >= m_mapDefinition->m_floorHeight && hitResult.m_impactPos.z <= m_mapDefinition->m_ceilingHeight)
	{
		std::string impactPos = Stringf("impact position = %.2f, %.2f, %.2f", hitResult.m_impactPos.x, hitResult.m_impactPos.y, hitResult.m_impactPos.z);

		// DebugAddMessage(impactPos, -1.f, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
		// DebugAddMessage(impactPos, 5.f, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
		// DebugAddWorldPoint(Vec3(currentTile.x, currentTile.y, 0.5f), 3.f, 10.f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::X_RAY);
		return hitResult;
	}
	else
	{
		return missResult;
	}
}

RaycastResult3D Map::RaycastWorldZ(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) const
{
	// Initialization setup
	Vec3 rayEnd = rayStart + rayFwdNormal * rayDist;

	// if it misses, we will use this result
	RaycastResult3D missResult;
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
	RaycastResult3D hitResult;

	hitResult.m_rayFwdNormal = rayFwdNormal;
	hitResult.m_rayStartPos = rayStart;
	hitResult.m_rayDist = rayDist;

	// we will discuss based on the direction in Z
	if (rayFwdNormal.z == 0.f) // looking forward, no collision with floor or ceiling
	{
		return missResult;
	}
	else if (rayFwdNormal.z < 0.f) // looking down, we need to check if we have chance to hit the floor
	{
		if (rayStart.z <= m_mapDefinition->m_floorHeight) // we are standing below the floor, miss
		{
			return missResult;
		}
		else // we standing above the ground
		{
			// the only way to see the ceiling is above the ceiling but it is then invisible
			// so we are only checking with the floor
			float height = rayStart.z - m_mapDefinition->m_floorHeight;
			if (rayDist == 0.f)
			{
				ERROR_AND_DIE("The raycast distance is 0.f");
			}
			float t = height / (rayFwdNormal.z * (-1.f) * rayDist);
			if ( t >= 0.f && t <= 1.f) // hit
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = t * rayDist;
				hitResult.m_impactPos = rayStart + rayFwdNormal * hitResult.m_impactDist;

				hitResult.m_impactNormal = Vec3(0.f, 0.f, 1.f);

				return hitResult;
			}
			else // miss
			{
				return missResult;
			}
		}
	}
	else // direction.z > 0.f // looking up
	{
		// we need to check if we have chance to hit the ceil
		if (rayStart.z >= m_mapDefinition->m_ceilingHeight) // we are standing above the ceiling, miss
		{
			return missResult;
		}
		else // we are below the ceiling
		{
			// the only way to see the ceiling is above the ceiling but it is then invisible
			// so we are only checking with the floor
			float height = m_mapDefinition->m_ceilingHeight - rayStart.z;
			float t = height / (rayFwdNormal.z * rayDist);
			if (t >= 0.f && t <= 1.f) // hit
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = t * rayDist;
				hitResult.m_impactPos = rayStart + rayFwdNormal * hitResult.m_impactDist;

				hitResult.m_impactNormal = Vec3(0.f, 0.f, -1.f);

				return hitResult;
			}
			else // miss
			{
				return missResult;
			}
		}
	}
}

RaycastResult3D Map::RaycastWorldActors(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayDist) const
{
	float shortestImpactDist = 9999999.f;
	std::vector<RaycastResult3D> collisionResults;

	// collect all actor raycast results and get the shortest dist
	if (!m_actorList.empty())
	{
		for (int i = 0; i < (int)m_actorList.size(); ++i)
		{
			RaycastResult3D result = RaycastVsCylinderZ3D(rayStart, rayFwdNormal, rayDist, m_actorList[i]->GetCylinderCollisionInWorldSpace());
			if (result.m_didImpact)
			{
				collisionResults.push_back(result);
				if (result.m_impactDist < shortestImpactDist)
				{
					shortestImpactDist = result.m_impactDist;
				}
			}
		}
	}

	// if we have a shortest impact dist
	if (shortestImpactDist != 9999999.f)
	{
		// we are going to find whose value it gets from
		for (int i = 0; i < (int)collisionResults.size(); ++i)
		{
			// it also needs to did impact the actor
			if (collisionResults[i].m_impactDist == shortestImpactDist && collisionResults[i].m_didImpact)
			{
				return collisionResults[i];
			}
		}
	}

	// for the rest of the case, we'll just return a miss result
	RaycastResult3D missResult;
	Vec3 rayEnd = rayStart + rayFwdNormal * rayDist;

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

	return missResult;
}

Actor* Map::RaycastActorList(Vec3 const& rayStart, float rayDist, ActorPtrList actorList) const
{
	float shortestImpactDist = 9999999.f;
	Actor* closestActor = nullptr;
	
	if (!actorList.empty())
	{
		// collect all actor raycast results and get the shortest dist
		for (int i = 0; i < (int)actorList.size(); ++i)
		{
			Vec3 rayFwdNormal = (actorList[i]->m_position - rayStart).GetNormalized();
			RaycastResult3D result = RaycastVsCylinderZ3D(rayStart, rayFwdNormal, rayDist, actorList[i]->GetCylinderCollisionInWorldSpace());
			actorList[i]->m_raycastResult = result;
			if (result.m_didImpact)
			{
				if (result.m_impactDist < shortestImpactDist) // update the wanted shortest hit result when we get a new winner
				{
					closestActor = actorList[i];
					shortestImpactDist = result.m_impactDist;
				}
			}
		}

		// if we have a shortest impact dist
		if (shortestImpactDist != 9999999.f)
		{
			return closestActor;
		}
		else return nullptr;
	}
	else
	{
		return nullptr;
	}

}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// actor functions
void Map::GenerateInitialActors()
{
	// float fakeEnemyRadius = 0.35f;
	// float fakeEnemyHeight = 0.75f;
	// Rgba8 fakeEnemyColor = Rgba8::RED;
	// ZCylinder enemyCollision;
	// enemyCollision.CenterXY = Vec2();
	// enemyCollision.Radius = fakeEnemyRadius;
	// enemyCollision.MinMaxZ = FloatRange(0.f, fakeEnemyHeight);
	// 
	// Actor* fakeEnemyA = new Actor(this, Vec3(7.5f, 8.5f, 0.25f), EulerAngles(), fakeEnemyColor, enemyCollision);
	// fakeEnemyA->m_name = "fakeEnemyA";
	// m_actorsInThisMap.push_back(fakeEnemyA);
	// 
	// Actor* fakeEnemyB = new Actor(this, Vec3(8.5f, 8.5f, 0.125f), EulerAngles(), fakeEnemyColor, enemyCollision);
	// fakeEnemyB->m_name = "fakeEnemyB";
	// m_actorsInThisMap.push_back(fakeEnemyB);
	// 
	// Actor* fakeEnemyC = new Actor(this, Vec3(9.5f, 8.5f, 0.f), EulerAngles(), fakeEnemyColor, enemyCollision);
	// fakeEnemyC->m_name = "fakeEnemyC";
	// m_actorsInThisMap.push_back(fakeEnemyC);
	// 
	// float fakeProjectileRadius = 0.0625f;
	// float fakeProjectileHeight = 0.125f;
	// Rgba8 fakeProjectileColor = Rgba8::BLUE;
	// ZCylinder projectileCollision;
	// projectileCollision.CenterXY = Vec2();
	// projectileCollision.Radius = fakeProjectileRadius;
	// projectileCollision.MinMaxZ = FloatRange(0.f, fakeProjectileHeight);
	// 
	// Actor* fakeProjectile = new Actor(this, Vec3(5.5f, 8.5f, 0.0625f), EulerAngles(), fakeProjectileColor, projectileCollision, false);
	// fakeProjectile->m_name = "fakeProjectile";
	// m_actorsInThisMap.push_back(fakeProjectile);
	
	std::vector<SpawnInfo>& spawnInfos = m_mapDefinition->m_spawnInfos;
	m_actorList.clear();

	for (int i = 0; i < (int)spawnInfos.size(); ++i)
	{
		if (spawnInfos[i].m_actorDef->m_actorName == "Marine")
		{
			continue;
		}
		else
		{
			Actor* actorPtr = SpawnActorAndAddToMapActorList(spawnInfos[i]); // the actor is spawn and added to the map actor list

			if (actorPtr->m_actorDef->m_isShielded)
			{
				SpawnShieldForActor(actorPtr);
			}

			SpawnAIControllerAndPossessEnemy(actorPtr);
		}
	}

	SpawnPlayersAndPossessedByPlayerControllers();

	for (int i = 0; i < (int)m_actorList.size(); ++i)
	{
		m_actorList[i]->Startup();
	}
}

bool Map::CheckIfActorExistAndShouldBeDestroyed(Actor* actor) const 
{
	if (actor)
	{
		if (actor->m_isDestroyed)
		{
			return true;
		}
	}
	else
	{
		return false;
	}
	return false;
}

bool Map::CheckIfActorExistAndNotDestroyed(Actor* actor) const
{
	if (actor)
	{
		if (!actor->m_isDestroyed)
		{
			return true;
		}
	}
	else
	{
		return false;
	}
	return false;
}

bool Map::CheckIfActorExistAndIsAlive(Actor* actor) const
{
	if (actor)
	{
		if (!actor->m_isDead)
		{
			return true;
		}
	}
	else
	{
		return false;
	}
	return false;
}

// Spawn a marine actor at a random spawn point and possess it with the player.
void Map::SpawnPlayersAndPossessedByPlayerControllers()
{
	// get all the spawn point info
	std::vector<SpawnInfo> m_spawnPointInfos;
	int numSpawnInfos = (int)(m_mapDefinition->m_spawnInfos.size());
	for (size_t i = 0; i < numSpawnInfos; i++)
	{
		if (m_mapDefinition->m_spawnInfos[i].m_actorDef->m_actorName == "SpawnPoint")
		{
			m_spawnPointInfos.push_back(m_mapDefinition->m_spawnInfos[i]);
		}
	}
	int numSpawnPoints = (int)m_spawnPointInfos.size();


	for (int i = 0; i < (int)g_theGame->m_playersList.size(); ++i)
	{
		Player*& playerController = g_theGame->m_playersList[i];

		Actor* controlledActor = GetActorByUID(playerController->m_actorUID);
		if (!controlledActor)
		{
			playerController->m_map = this;
			if (playerController != nullptr)
			{
				// spawn the actor
				int spawnInfoIndex = g_rng->RollRandomIntInRange(0, (numSpawnPoints - 1));
				// modify the spawn info from spawn point to marine, so we could use the position and orientation of the spawn info
				SpawnInfo playerSpawnInfo = m_spawnPointInfos[spawnInfoIndex];
				// we are setting it as a marine
				playerSpawnInfo.m_actorDef = ActorDefinition::GetActorDefByString("Marine");

				Actor* marine = SpawnActorAndAddToMapActorList(playerSpawnInfo);
				marine->Startup();

				// possess with the player controller
				// g_thePlayerController->RespawnedAndReset();
				playerController->Possess(marine);
				marine->OnPossessed(playerController);
			}
		}
	}
}

Actor* Map::SpawnActor(SpawnInfo const& spawnInfo)
{
	// generate a UID and assign to the actor
	// salt
	++m_actorSalt;
	if (m_actorSalt > Map::MAX_ACTOR_SALT) // 0xFFFF FFFF
	{
		m_actorSalt = 0x00000000u;
	}
	// index
	int  index = 0;
	bool emptySpaceInList = false;
	for (int i = 0; i < (int)m_actorList.size(); ++i)
	{
		if (m_actorList[i] == nullptr)
		{
			emptySpaceInList = true;
			index = i;
		}
	}
	if (!emptySpaceInList)
	{
		index = (int)m_actorList.size();
	}
	ActorUID uid(m_actorSalt, index);

	Actor* newActor = new Actor(spawnInfo, this, uid);

	return newActor;
}

Actor* Map::SpawnActorAndAddToMapActorList(SpawnInfo const& spawnInfo)
{
	// generate a UID and assign to the actor
	// salt
	++m_actorSalt;
	if (m_actorSalt > Map::MAX_ACTOR_SALT) // 0xFFFF FFFF
	{
		m_actorSalt = 0x00000000u;
	}
	// index
	int  index = 0;
	bool emptySpaceInList = false;
	for (int i = 0; i < (int)m_actorList.size(); ++i)
	{
		if (m_actorList[i] == nullptr)
		{
			emptySpaceInList = true;
			index = i;
		}
	}
	if (!emptySpaceInList)
	{
		index = (int)m_actorList.size();
	}

	// create new actor
	ActorUID uid(m_actorSalt, index);
	Actor* newActor = new Actor(spawnInfo, this, uid);

	// add to map list
	if (!emptySpaceInList)
	{
		m_actorList.push_back(newActor);
	}
	else
	{
		m_actorList[index] = newActor;
	}

	return newActor;
}

Actor* Map::SpawnBulletHitOnEnvirnment(Vec3 pos)
{
	ActorDefinition* def = GetActorDefByString("BulletHit");

	Vec3 direction = Vec3();
	Vec3 vel = Vec3();
	EulerAngles orientation = EulerAngles();

	SpawnInfo* bulletHitInfo = new SpawnInfo(def, pos, vel, orientation);
	Actor* bulletHitActor = SpawnActorAndAddToMapActorList(*bulletHitInfo);
	bulletHitActor->Startup();

	return bulletHitActor;
}

Actor* Map::SpawnBloodSplatterOnHitEnemy(Vec3 pos)
{
	ActorDefinition* def = GetActorDefByString("BloodSplatter");

	Vec3 direction = Vec3();
	Vec3 vel = Vec3();
	EulerAngles orientation = EulerAngles();

	SpawnInfo* bloodSpawnInfo = new SpawnInfo(def, pos, vel, orientation);
	Actor* bloodActor = SpawnActorAndAddToMapActorList(*bloodSpawnInfo);
	bloodActor->Startup();

	return bloodActor;
}

AIController* Map::SpawnAIControllerAndPossessEnemy(Actor* actorPtr)
{
	// Create geometry if we are visible
	// if (actorPtr->m_actorDef->m_visible)
	// {
	// 	float radius = actorPtr->m_actorDef->m_physicsRadius;
	// 	float height = actorPtr->m_actorDef->m_physicsHeight;
	// 	actorPtr->m_collision = ZCylinder(Vec2(), radius, FloatRange(0.f, height));
	// 	actorPtr->InitializeLocalVerts();
	// } // this is changed into player

	// Create an AI controller if we are AI enabled
	if (actorPtr->m_actorDef->m_aiEnabled)
	{
		AIController* newAIController = new AIController(this, actorPtr->m_actorUID);
		actorPtr->m_AIController = newAIController;
		newAIController->Possess(actorPtr);
		actorPtr->OnPossessed(newAIController);

		return newAIController;
	}

	return nullptr;
}

void Map::SpawnShieldForActor(Actor* shieldedActor)
{
	ActorDefinition* shieldDef = GetActorDefByString("EnergyShield");
	SpawnInfo spawnInfo(shieldDef, shieldedActor->m_position, shieldedActor->m_velocity, shieldedActor->m_orientation);
	Actor* shield = SpawnActorAndAddToMapActorList(spawnInfo);
	shield->m_shieldOwnerID = shieldedActor->m_actorUID;
}

void Map::CheckIfPlayerReachDestination()
{
	Vec3 exitPos;
	for (int i = 0; i < (int)m_actorList.size(); ++i)
	{
		if (m_actorList[i])
		{
			if (m_actorList[i]->m_actorDef->m_actorName == "Destination")
			{
				exitPos = m_actorList[i]->m_position;
			}
		}
	}

	// check if the player is close to the target
	if (g_theGame->m_playersList.size() >= 1)
	{
		if (GetDistance3D(g_theGame->m_playersList[0]->m_position, exitPos) < 1.5f)
		{
			g_theGame->m_playerWins = true;
		}
	}
}

// Dereference an actor UIDand return an actor pointer
Actor* Map::GetActorByUID(ActorUID const uid) const
{
	// if the uid is invalid, which means it has not been initialized, return nullptr
	if (uid == ActorUID::INVALID)
	{
		return nullptr;
	}

	unsigned int listIndex = uid.GetIndex();
	// Get the index from the actor UID.If that slot in our list of actors is null, return null
	if (!m_actorList[listIndex])
	{
		return nullptr;
	}
	else
	{
		// If that slot in our list of actors contains an actor, check if that actor’s UID matches the input actor UID
		if (m_actorList[listIndex]->m_actorUID != uid) // If they do not match, return null
		{
			return nullptr;
		}
		else
		{
			// Otherwise, return the actor pointer at that index.
			return m_actorList[listIndex];
		}
	}
}

ActorDefinition* Map::GetActorDefByString(std::string actorName) const
{
	for (int i = 0; i < (int)ActorDefinition::s_actorDefs.size(); ++i)
	{
		if (ActorDefinition::s_actorDefs[i]->m_actorName == actorName)
		{
			return ActorDefinition::s_actorDefs[i];
		}
	}

	ERROR_AND_DIE("Did not find any actor definition by the actor name");
}

ActorPtrList Map::GetActorsOfDifferentFaction(ActorFaction faction)
{
	ActorPtrList actorList;
	for (int i = 0; i < (int)m_actorList.size(); ++i)
	{
		if (CheckIfActorExistAndIsAlive(m_actorList[i]))
		{
			if (m_actorList[i]->m_actorDef->m_faction != faction)
			{
				if (m_actorList[i]->m_actorDef->m_canBePossessed)
				{
					actorList.push_back(m_actorList[i]);
				}
			}
		}
	}

	return actorList;
}

ActorPtrList Map::GetActorsExceptSelf(Actor* myself)
{
	ActorPtrList actorList;
	for (int i = 0; i < (int)m_actorList.size(); ++i)
	{
		if (CheckIfActorExistAndIsAlive(m_actorList[i]))
		{
			if (m_actorList[i]->m_actorDef->m_collidesWithWorld && m_actorList[i] != myself)
			{
				actorList.push_back(m_actorList[i]);
			}
		}
	}

	return actorList;
}

// loop through the actor ptr list to get the one that is closet to the player camera
Actor* Map::GetClosestVisibleEnemy(Actor* toThisActor)
{
	std::vector<Actor*> targetActorList;
	Vec3 fwdNormal = toThisActor->GetForwardNormal();
	float rayDist = toThisActor->m_actorDef->m_sightRadius;
	ActorFaction friendFaction = toThisActor->GetActorFaction();

	for (int i = 0; i < (int)m_actorList.size(); ++i)
	{
		if (CheckIfActorExistAndIsAlive(m_actorList[i]))
		{
			if (m_actorList[i]->m_actorDef->m_faction != friendFaction && m_actorList[i]->m_actorDef->m_faction != ActorFaction::NEUTRAL)
			{
				Vec3 disp = m_actorList[i]->m_position - toThisActor->m_position;
				float dist = disp.GetLength();
				// first check if the marine is in range
				if (dist <= rayDist)
				{
					// check if marine is in the FOV
					disp = disp.GetNormalized();
					float dotProduct = DotProduct3D(disp, fwdNormal);
					if (dotProduct >= CosDegrees(toThisActor->m_actorDef->m_sightAngle * 0.5f))
					{
						targetActorList.push_back(m_actorList[i]);
					}
				}
			}
		}
	}
	
	// if there is no target in range
	if (targetActorList.empty())
	{
		return nullptr;
	}

	// if there is any target in range then do raycast test see if the ray towards the marine is blocked
	Vec3 rayStart = toThisActor->m_position + Vec3(0.f, 0.f, toThisActor->m_actorDef->m_eyeHeight);
	Actor* closestEnemyActor = RaycastActorList(rayStart, rayDist, targetActorList);

	// if the raycast could hit the marine collision
	if (closestEnemyActor)
	{
		// we are going to check if the raycast going to hit wall or ceil or floor first
		fwdNormal = (closestEnemyActor->m_position - rayStart).GetNormalized();
		RaycastResult3D resultInZ = RaycastWorldZ(rayStart, fwdNormal, rayDist);
		RaycastResult3D resultForTiles = FastRaycastForVoxelGrids(rayStart, fwdNormal, rayDist);

		// get the shortest impact dist
		float shortestDist = rayDist;
		float actorDist = closestEnemyActor->m_raycastResult.m_impactDist;
		if (resultInZ.m_impactDist < shortestDist)
		{
			shortestDist = resultInZ.m_impactDist;
		}
		if (actorDist < shortestDist)
		{
			shortestDist = actorDist;
		}
		if (resultForTiles.m_impactDist < shortestDist)
		{
			shortestDist = resultForTiles.m_impactDist;
		}

		// see which dist it belongs to
		if (shortestDist == rayDist)
		{
			// DebugAddWorldLine(rayStart, resultInZ.m_impactPos, 0.01f, 0.f, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);
			return nullptr;
		}
		else if (shortestDist == resultInZ.m_impactDist)
		{
			// DebugAddWorldLine(rayStart, resultInZ.m_impactPos, 0.01f, 0.f, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);
			return nullptr;
		}
		else if (shortestDist == actorDist)
		{
			// DebugAddWorldLine(rayStart, closestEnemyActor->m_raycastResult.m_impactPos, 0.01f, 0.f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::X_RAY);
			return closestEnemyActor;
		}
		else if (shortestDist == resultForTiles.m_impactDist)
		{
			// DebugAddWorldLine(rayStart, resultForTiles.m_impactPos, 0.01f, 0.f, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);
			return nullptr;
		}
		else
		{
			return nullptr;
		}
	}
	else return nullptr;
}

// Have the player controller possess the next actor in the list that can be possessed
Actor* Map::DebugPossessNext(Actor* currentActor)
{
	// take current actor and get the wanted index in list
	int actorIndex = currentActor->m_actorUID.GetIndex();
	++actorIndex;
	int numActors = (int)m_actorList.size();
	if (actorIndex >= numActors)
	{
		actorIndex = 0;
	}

	// loop through all the actors on the map find the next actor in the list that can be possessed
	for (int i = actorIndex; i < numActors; i++)
	{
		if (CheckIfActorExistAndNotDestroyed(m_actorList[i]))
		{
			if (m_actorList[i]->m_actorDef->m_canBePossessed)
			{
				return m_actorList[i];
			}
			else
			{
				continue;
			}
		}
		else
		{
			if (i == (numActors - 1))
			{
				i = 0;
			}
			continue;
		}
	}

	return nullptr;
}

void Map::UpdateAllActors()
{
	if (!m_actorList.empty())
	{
		for (int i = 0; i < m_actorList.size(); ++i)
		{
			// we are updating all the actors but the player is update elsewhere
			// update the actor even it is dead, because we need to update its destroy counter
			if (CheckIfActorExistAndNotDestroyed(m_actorList[i]))
			{
				m_actorList[i]->Update();
			}
		}
	}

	// player camera: debug mode(free flying) or possess mode
}

void Map::UpdatePlayerController()
{
	for (int i = 0; i < (int)g_theGame->m_playersList.size(); ++i)
	{
		g_theGame->m_playersList[i]->Update();
	}
}

void Map::RenderAllActors() const
{
	if (!m_actorList.empty())
	{
		for (int i = 0; i < m_actorList.size(); ++i)
		{
			if (CheckIfActorExistAndNotDestroyed(m_actorList[i]))
			{
				m_actorList[i]->Render();
			}
		}
	}
}

void Map::DeleteDestoryedActors()
{
	if (!m_actorList.empty())
	{
		for (int i = 0; i < m_actorList.size(); ++i)
		{
			if (CheckIfActorExistAndShouldBeDestroyed(m_actorList[i]))
			{
				Player* playerController = dynamic_cast<Player*>(m_actorList[i]->m_controller);

				delete m_actorList[i];
				m_actorList[i] = nullptr;

				if (playerController) // if it is the player controller, we are going to set its camera according to the actor setting
				{
					SpawnPlayersAndPossessedByPlayerControllers();
				}
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
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

bool Map::IsPositionInBounds(Vec3 position, float const tolerance /*= 0.f*/) const
{
	bool withinFloorAndCeiling = false;
	bool withinX = false;
	bool withinY = false;
	// check Z
	if ((position.z + abs(tolerance)) >= m_mapDefinition->m_floorHeight && (position.z - abs(tolerance)) <= m_mapDefinition->m_ceilingHeight)
	{
		withinFloorAndCeiling = true;
	}
	// check X
	if ((position.x + abs(tolerance)) >= 0.f && (position.x - abs(tolerance)) <= (float)(m_dimensions.x))
	{
		withinX = true;
	}
	// check Y
	if ((position.y + abs(tolerance)) >= 0.f && (position.y - abs(tolerance)) <= (float)(m_dimensions.y))
	{
		withinX = true;
	}

	// if all conditions are met, only return true
	if (withinFloorAndCeiling && withinX && withinY)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/// <Render>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Map::Render() const 
{
	RenderTiles();
	RenderAllActors();
}

void Map::RenderTiles() const
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->BindTexture(m_tileTexture);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetModelConstants(GetModelMatrix());
	g_theRenderer->BindShader(m_shader);
	g_theRenderer->SetLightingConstants(*m_mapLightingSettings);
	g_theRenderer->DrawVertexArrayWithIndexArray(m_vertexBuffer, m_indexBuffer, (int)(m_indexArray.size()));
}

Mat44 Map::GetModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_mapOrigin);
	Mat44 orientationMat = m_mapOrientation.GetAsMatrix_XFwd_YLeft_ZUp();
	transformMat.Append(orientationMat);
	return transformMat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// <tile helper functions>
int Map::GetTileIndex_For_TileCoordinates(IntVec2 tileCoord) const
{
	int index = tileCoord.x + tileCoord.y * m_dimensions.x;
	return index;
}

IntVec2 Map::GetTileCoordsInMap_For_WorldPos(Vec3 const& worldPos) const
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
	return tile.GetBounds();
}

AABB3 Map::GetTileBlockBounds(IntVec2 const& tileIndexCoords_InMap)
{
	int indexInList = GetTileIndex_For_TileCoordinates(tileIndexCoords_InMap);
	Tile tile = m_tiles[indexInList];
	AABB2 tileBounds = tile.GetBounds();

	// get the block bounds based on the info of the tile
	Vec2 BL = tileBounds.m_mins;
	Vec2 TR = tileBounds.m_maxs;
	AABB3 blockBounds;

	if (tile.m_tileDef->m_halfHeight)
	{
		blockBounds = AABB3(Vec3(BL.x, BL.y, m_mapDefinition->m_floorHeight), Vec3(TR.x, TR.y, m_mapDefinition->m_ceilingHeight * 0.5f));
	}
	else
	{
		blockBounds = AABB3(Vec3(BL.x, BL.y, m_mapDefinition->m_floorHeight), Vec3(TR.x, TR.y, m_mapDefinition->m_ceilingHeight));
	}
	return blockBounds;
}

AABB3 Map::GetTileBlockBounds(int tileIndex)
{
	Tile tile = m_tiles[tileIndex];
	AABB2 tileBounds = tile.GetBounds();

	// get the block bounds based on the info of the tile
	Vec2 BL = tileBounds.m_mins;
	Vec2 TR = tileBounds.m_maxs;
	AABB3 blockBounds;
	if (tile.m_tileDef->m_halfHeight)
	{
		blockBounds = AABB3(Vec3(BL.x, BL.y, m_mapDefinition->m_floorHeight), Vec3(TR.x, TR.y, m_mapDefinition->m_ceilingHeight * 0.5f));
	}
	else
	{
		blockBounds = AABB3(Vec3(BL.x, BL.y, m_mapDefinition->m_floorHeight), Vec3(TR.x, TR.y, m_mapDefinition->m_ceilingHeight));
	}
	return blockBounds;
}

Vec2 Map::GetMapDimensions()
{
	float wide   = static_cast<float>(m_dimensions.x);
	float height = static_cast<float>(m_dimensions.y);
	return Vec2(wide, height);
}

Tile* const Map::GetTile(IntVec2 tileCoords)
{
	int tileIndex = GetTileIndex_For_TileCoordinates(tileCoords);
	return &m_tiles[tileIndex];
}