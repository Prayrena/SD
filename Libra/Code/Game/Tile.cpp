#include "Game/Tile.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Game/App.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"

//TileTypeDefinition TileTypeDefinition::s_tileDefs[NUM_TILE_TYPES];
std::vector<TileTypeDefinition> TileTypeDefinition::s_tileDefs;
extern SpriteSheet* g_terrainSprites;

void TileTypeDefinition:: InitializeTileDefs()
{
	// remember the sprite need to -1 from the index you count because it starts with 0
	// DefineTileType(TILE_TYPE_SHORT_GRASS,			false, false, IntVec2(0, 0), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_LONG_GRASS,			false, false, IntVec2(0, 1), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_YELLOW_GRASS,			false, false, IntVec2(1, 1), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_DIRT,					false, false, IntVec2(6, 2), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_UNDERGROUND_ROCKFLOOR, false, false, IntVec2(4, 3), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_PAVED,					false, false, IntVec2(3, 6), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_STONE_WALL,			true,  false, IntVec2(5, 5), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_COBBLE,				true,  false, IntVec2(6, 5), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_BRICK_WALL,			true,  false, IntVec2(2, 4), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_PALE_RED_BRICK,		false, false, IntVec2(3, 5), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_ICE_SURFACE,			false, false, IntVec2(2, 2), Rgba8::CYAN);
	// DefineTileType(TILE_TYPE_ICE_CUBE,				true,  false, IntVec2(4, 6), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_IRON_WALL,				true,  false, IntVec2(5, 6), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_WATER_BRICK,			true,  false, IntVec2(6, 6), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_MAP_ENTRY,				false, false, IntVec2(0, 7), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_EXIT,					false, false, IntVec2(1, 7), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_GOLDEN_BRICK,			true,  false, IntVec2(2, 7), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_WATER_SURFACE,			false, true,  IntVec2(5, 7), Rgba8::WHITE);
	// DefineTileType(TILE_TYPE_LAVA,					false, true,  IntVec2(6, 7), Rgba8::WHITE);
	// DefineTileType(DEBUG,							false, false, IntVec2(7, 7), Rgba8::MAGENTA);

	XmlDocument tileDefXml;
	char const* filePath = "Data/Definitions/LibraTileDefinitions.xml";
	XmlResult result = tileDefXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("failed to load xml file"));

	XmlElement* rootElement = tileDefXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "rootElement is nullPtr");

	XmlElement* tileDefElement = rootElement->FirstChildElement();
	while (tileDefElement)
	{
		std::string elementName = tileDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "TileDefinition", Stringf("root cant matchup with the name"));
		TileTypeDefinition* newTileDef = new TileTypeDefinition(*tileDefElement);// calls the constructor function of TileTypeDefinition
		s_tileDefs.push_back(*newTileDef);
		tileDefElement = tileDefElement->NextSiblingElement();
	}
	return;
}

// use xml element to construct a 
TileTypeDefinition::TileTypeDefinition(XmlElement const& tileDefElement)
{
	std::string notFound = "no name element found";
	m_name	  = ParseXmlAttribute(tileDefElement, "name", notFound); // m_name defines the variable type
	m_isSolid = ParseXmlAttribute(tileDefElement, "isSolid", false);
	m_isWater = ParseXmlAttribute(tileDefElement, "isWater", false);
	m_tint    = ParseXmlAttribute(tileDefElement, "tint", Rgba8::WHITE);

	IntVec2 spriteCoords = ParseXmlAttribute(tileDefElement, "spriteCoords", IntVec2(7,7)); // the last one is default value when could not find in xml
	m_UVs = g_terrainSprites->GetSpriteUVs(spriteCoords);
}

// void TileTypeDefinition::DefineTileType(TileType type, bool isSolid, bool isWater, IntVec2 spriteCoords, Rgba8 tintColor)
// {
// 		TileTypeDefinition& tileDef = TileTypeDefinition::s_tileDefs[(int)type];
// 		tileDef.m_isSolid = isSolid;
// 		tileDef.m_isWater = isWater;
// 		tileDef.m_UVs = g_terrainSprites->GetSpriteUVs(spriteCoords);
// 		tileDef.m_tint = tintColor;
// }

// void Tile::SetType(TileType type)
// {
// 		m_tileType = type;
// 		m_tileDef = TileTypeDefinition::s_tileDefs[(int)type];
// }

void Tile::SetType(std::string tileTypeName)
{
	for (int i = 0; i < TileTypeDefinition::s_tileDefs.size(); ++i)
	{
		if (TileTypeDefinition::s_tileDefs[i].m_name == tileTypeName)
		{
			m_tileDef = &TileTypeDefinition::s_tileDefs[i];
			break; // if find the correct tile definition, stop the for loop
		}
	}
}

AABB2 Tile::GetBounds() const
{
	AABB2 boundary;
	boundary.m_mins = Vec2(static_cast<float>(m_tileCoords.x), static_cast<float>(m_tileCoords.y));
	boundary.m_maxs = Vec2(static_cast<float>(m_tileCoords.x + 1), static_cast<float>(m_tileCoords.y + 1));
	return boundary;
}

Rgba8 Tile::GetColor() const
{
	return GetDef().m_tint;
}

bool Tile::IsSolid()
{
	return GetDef().m_isSolid;
}

bool Tile::IsWater()
{
	return GetDef().m_isWater;
}

TileTypeDefinition const& Tile::GetDef() const
{
	return *m_tileDef;
}
