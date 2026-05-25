#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/core/XmlUtils.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include <vector>

struct TileTypeDefinition
{
public:
	TileTypeDefinition(XmlElement const& tileDefElement);

	std::string		m_name;
	bool			m_isSolid = false;
	bool			m_halfHeight = false;

	Rgba8	m_tint = Rgba8::WHITE;

	Rgba8   m_mapImagePixelColor = Rgba8(0, 0, 0, 0);

	// IntVec2(999, 999) means is not designated
	IntVec2 m_floorSpriteCoords = IntVec2(999, 999);
	IntVec2 m_ceilingSpriteCoords = IntVec2(999, 999);
	IntVec2 m_wallSpriteCoords = IntVec2(999, 999);

	AABB2   GetTileTextureUVsOnSpriteSheet(IntVec2 spriteCoords, SpriteSheet* spriteSheet);

	static void InitializeTileDefs(); // call defineTileType to define each tile type definition
	static std::vector<TileTypeDefinition> s_tileDefs;
};



struct Tile
{
public:
	void SetType(std::string tileTypeName);
	void SetTileCoordsAndType(IntVec2 coords, std::string tileTypeName);

	AABB2 GetBounds() const; // return world pos
	void  SetBlockBounds(AABB3 blockDefinedByMap);
	AABB3 GetBlockBounds() const;
	Rgba8 GetColor();
	TileTypeDefinition GetDef();

	bool IsSolid();

	IntVec2 m_tileCoords = IntVec2(0, 0);
	AABB3	m_bounds;

	TileTypeDefinition const* m_tileDef = nullptr;
};
