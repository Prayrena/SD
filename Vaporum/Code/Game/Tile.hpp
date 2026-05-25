#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/core/XmlUtils.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include <vector>

extern const Vec2 P_i;
extern const Vec2 P_j;

struct TileTypeDefinition
{
public:
	TileTypeDefinition(XmlElement const& tileDefElement);

	std::string		m_name;
	bool			m_isBlocked = false;

	Rgba8	m_tint = Rgba8::WHITE;

	char   m_tileSymbol = ' ';

	AABB2   GetTileTextureUVsOnSpriteSheet(IntVec2 spriteCoords, SpriteSheet* spriteSheet);

	static void InitializeTileDefs(); // call defineTileType to define each tile type definition
	static std::vector<TileTypeDefinition> s_tileDefs;

	static TileTypeDefinition const* GetBySymbol(char symbol);
};


struct Tile
{
	Tile(){}
	~Tile(){}
	void SetType(std::string tileTypeName);
	void SetTileCoordsAndType(IntVec2 coords, TileTypeDefinition const* typeDef);

	AABB2 GetBounds() const; // return world pos
	void  SetBlockBounds(AABB3 blockDefinedByMap);
	AABB3 GetBlockBounds() const;
	Rgba8 GetColor();
	TileTypeDefinition GetDef();

	bool IsSolid();

	Vec3 m_boundaries[6];

	int		m_floorIndex = 0;
	IntVec2 m_tileCoords = IntVec2(0, 0);
	Vec3	m_centerWorldPos;

	TileTypeDefinition const* m_tileDef = nullptr;
};
