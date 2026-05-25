#include "Engine/Renderer/SpriteSheet.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Game/Tile.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"

const Vec2 P_i = Vec2(0.866f, 0.5f);
const Vec2 P_j = Vec2(0, 1.f);

std::vector<TileTypeDefinition> TileTypeDefinition::s_tileDefs;

const TileTypeDefinition* TileTypeDefinition::GetBySymbol(char symbol)
{
	for (int i = 0; i < (int)s_tileDefs.size(); ++i)
	{
		if (symbol == s_tileDefs[i].m_tileSymbol)
		{
			return &s_tileDefs[i];
		}
	}

	// if not found, return nullptr
	return nullptr;
}

void TileTypeDefinition::InitializeTileDefs()
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
	std::string notFound = "tile name undefined";
	m_name = ParseXmlAttribute(tileDefElement, "name", notFound); // m_name defines the variable type
	m_isBlocked = ParseXmlAttribute(tileDefElement, "isBlocked", false);
	m_tileSymbol = ParseXmlAttribute(tileDefElement, "symbol", ' ');

	m_tint = ParseXmlAttribute(tileDefElement, "tint", Rgba8::WHITE);
}

void Tile::SetTileCoordsAndType(IntVec2 coords, TileTypeDefinition const* typeDef)
{
	m_tileCoords = coords;
	m_tileDef = typeDef;

	// based on hex grid coordinates, get the center position in the world
	m_centerWorldPos.x = P_i.x * (float)coords.x;
	m_centerWorldPos.y = P_i.y * (float)coords.x + P_j.y * (float)coords.y;
	m_centerWorldPos.z = (float)m_floorIndex;
}
