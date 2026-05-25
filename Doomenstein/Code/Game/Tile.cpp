#include "Engine/Renderer/SpriteSheet.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Game/Tile.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"

//TileTypeDefinition TileTypeDefinition::s_tileDefs[NUM_TILE_TYPES];
std::vector<TileTypeDefinition> TileTypeDefinition::s_tileDefs;

void TileTypeDefinition:: InitializeTileDefs()
{
	XmlDocument tileDefXml;
	char const* filePath = "Data/Definitions/TileDefinitions.xml";
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
}

// use xml element to define a tile type
TileTypeDefinition::TileTypeDefinition(XmlElement const& tileDefElement)
{
	std::string notFound = "no name element found";
	m_name	  = ParseXmlAttribute(tileDefElement, "name", notFound); // m_name defines the variable type
	m_isSolid = ParseXmlAttribute(tileDefElement, "isSolid", false);
	m_halfHeight = ParseXmlAttribute(tileDefElement, "halfHeight", false);
	m_mapImagePixelColor = ParseXmlAttribute(tileDefElement, "mapImagePixelColor", Rgba8::MAGENTA);

	if (m_halfHeight)
	{
		m_wallSpriteCoords = ParseXmlAttribute(tileDefElement, "wallSpriteCoords", IntVec2(999, 999));
		m_ceilingSpriteCoords = ParseXmlAttribute(tileDefElement, "ceilingSpriteCoords", IntVec2(999, 999));
	}
	else if (m_isSolid) // this block has back forth left and right quad
	{
		m_wallSpriteCoords = ParseXmlAttribute(tileDefElement, "wallSpriteCoords", IntVec2(999, 999));
	}
	else // this block has top and bottom quad
	{
		m_floorSpriteCoords = ParseXmlAttribute(tileDefElement, "floorSpriteCoords", IntVec2(999, 999));
		m_ceilingSpriteCoords = ParseXmlAttribute(tileDefElement, "ceilingSpriteCoords", IntVec2(999, 999));
	}

	m_tint = ParseXmlAttribute(tileDefElement, "tint", Rgba8::WHITE);
}

AABB2 TileTypeDefinition::GetTileTextureUVsOnSpriteSheet(IntVec2 spriteCoords, SpriteSheet* spriteSheet)
{
	return spriteSheet->GetSpriteUVs(spriteCoords);
}

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

void Tile::SetTileCoordsAndType(IntVec2 coords, std::string tileTypeName)
{
	m_tileCoords = coords;
	SetType(tileTypeName);
}

AABB2 Tile::GetBounds() const
{
	AABB2 boundary;
	boundary.m_mins = Vec2(static_cast<float>(m_tileCoords.x), static_cast<float>(m_tileCoords.y));
	boundary.m_maxs = Vec2(static_cast<float>(m_tileCoords.x + 1), static_cast<float>(m_tileCoords.y + 1));
	return boundary;
}

void Tile::SetBlockBounds(AABB3 blockDefinedByMap)
{
	m_bounds = blockDefinedByMap;
}

AABB3 Tile::GetBlockBounds() const
{
	return m_bounds;
}

Rgba8 Tile::GetColor()
{
	return GetDef().m_tint;
}

bool Tile::IsSolid()
{
	return GetDef().m_isSolid;
}

TileTypeDefinition Tile::GetDef()
{
	return *m_tileDef;
}
