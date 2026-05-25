#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/core/XmlUtils.hpp"
#include <vector>

//enum TileType
//{
//	TILE_TYPE_UNKNOW = -1,
//	TILE_TYPE_SHORT_GRASS,
//	TILE_TYPE_LONG_GRASS,
//	TILE_TYPE_YELLOW_GRASS,
//	TILE_TYPE_DIRT,
//	TILE_TYPE_UNDERGROUND_ROCKFLOOR,
//	TILE_TYPE_PAVED,
//	TILE_TYPE_STONE_WALL,
//	TILE_TYPE_COBBLE,
//	TILE_TYPE_BRICK_WALL,
//	TILE_TYPE_PALE_RED_BRICK,
//	TILE_TYPE_ICE_SURFACE,
//	TILE_TYPE_ICE_CUBE,
//	TILE_TYPE_IRON_WALL,
//	TILE_TYPE_WATER_BRICK,
//	TILE_TYPE_MAP_ENTRY,
//	TILE_TYPE_EXIT,
//	TILE_TYPE_GOLDEN_BRICK,
//	TILE_TYPE_WATER_SURFACE,
//	TILE_TYPE_LAVA,
//	DEBUG,
//	NUM_TILE_TYPES
//};

struct TileTypeDefinition
{
public:
	TileTypeDefinition(XmlElement const& tileDefElement);

	std::string		m_name;
	bool			m_isSolid = false;
	bool			m_isWater = false;
	
	AABB2	m_UVs = AABB2::ZERO_TO_ONE;
	Rgba8	m_tint = Rgba8::WHITE;

	// static TileTypeDefinition s_tileDefs[NUM_TILE_TYPES]; // declaration only for this global in the .hpp
	// static void DefineTileType(TileType type, bool isSolid, bool isWater, IntVec2 spriteCoords, Rgba8 tintColor);//is solid, UV, tint
	
	static void InitializeTileDefs(); // call defineTileType to define each tile type definition
	static std::vector<TileTypeDefinition> s_tileDefs;
};



struct Tile
{
public:
	void SetType(std::string tileTypeName);
	//void SetType(TileTypeDefinition* tileDef);
	//void SetType(TileType type);
	AABB2 GetBounds() const; // return world pos
	Rgba8 GetColor() const;
	TileTypeDefinition const& GetDef() const;
	bool IsSolid();
	bool IsWater();

	IntVec2 m_tileCoords;
	// TileType m_tileType = DEBUG;
	// TileTypeDefinition m_tileDef;
	TileTypeDefinition const* m_tileDef;
};
