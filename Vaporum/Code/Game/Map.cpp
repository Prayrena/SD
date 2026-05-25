#include "Engine/Math/AABB2.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/RaycastUtils.Hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/core/Image.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/core/Timer.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/App.hpp"
#include "Game/Player.hpp"
#include "Game/Tile.hpp"

extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;
extern App* g_theApp;
extern Game* g_theGame;
extern InputSystem* g_theInput;
extern Clock* g_theGameClock;

std::vector<MapDefinition> MapDefinition::s_mapDefs;

using namespace std;

bool operator>=(Vec3 const& a, Vec3 const& b)
{
	return (a.x >= b.x && a.y >= b.y && a.z >= b.z);
}

bool operator<=(Vec3 const& a, Vec3 const& b)
{
	return (a.x <= b.x && a.y <= b.y && a.z <= b.z);
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

		s_mapDefs.push_back(*newMapDef);
		mapDefElement = mapDefElement->NextSiblingElement();
	}
}

MapDefinition* const MapDefinition::GetByName(std::string const& name)
{
	for (int i = 0; i < (int)s_mapDefs.size(); ++i)
	{
		if (name == s_mapDefs[i].m_name)
		{
			return &s_mapDefs[i];
		}
	}

	// if not found, return nullptr
	return nullptr;
}

MapDefinition::MapDefinition(XmlElement const& mapDefElement)
{
	m_name = ParseXmlAttribute(mapDefElement, "name", "Not found in Xml");

	std::string shaderPath = ParseXmlAttribute(mapDefElement, "overlayShader", "Not found in Xml");
	if (shaderPath == "Data/Shaders/Unlit")
	{
		m_overlayShader = nullptr;
	}
	else
	{
		m_overlayShader = g_theRenderer->CreateOrGetShader(shaderPath.c_str());
	}

	m_gridSize = ParseXmlAttribute(mapDefElement, "gridSize", IntVec2(32, 32));

	m_worldBoundsMin = ParseXmlAttribute(mapDefElement, "worldBoundsMin", Vec3());
	m_worldBoundsMax = ParseXmlAttribute(mapDefElement, "worldBoundsMax", Vec3());

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// read tiles info
	XmlElement const* tilesElement = mapDefElement.FirstChildElement("Tiles");
	if (!tilesElement)
	{
		ERROR_AND_DIE("Missing \"Tiles\" definition under MapDefnition, check if you have Tiles info in XML")
	}
	std::string child1ElementName = tilesElement->Name();

	gridChars = tilesElement->GetText(); // todo: how do I know if the const char* is pointing to a heap memory or stack memory?
	// m_gridChars = tilesElement->Value();
	m_gridSymbols = InterpretCDATA(gridChars);

	XmlElement const* playerUnitsElement = mapDefElement.FirstChildElement("Units");
	while (playerUnitsElement)
	{
		int playerIndex = ParseXmlAttribute(*playerUnitsElement, "player", 0);
		const char* unitChars = playerUnitsElement->GetText();
		std::string unitsSymbols = InterpretCDATA(unitChars);
		m_playerUnitSymbols[playerIndex] = unitsSymbols;

		playerUnitsElement = playerUnitsElement->NextSiblingElement();
	}

	return;
}

std::string MapDefinition::InterpretCDATA(char const* chars)
{
	tileChars = std::string(chars); // todo: how do I know if the std::string is pointing to a heap memory or stack memory?
	Strings tileSymbolsEachLineReversed;
	std::string interpretedString;
	int numInAColumn = SplitStringOnDelimiter(tileSymbolsEachLineReversed, tileChars, "\n", true);
	int numInARow = (int)tileSymbolsEachLineReversed[0].size();

	if (numInAColumn != m_gridSize.y || numInARow != m_gridSize.x) // check if the grid size is the same as the CDATA draws
	{
		ERROR_AND_DIE(Stringf("The CDATA of %s does not mactch up with its grid size, check XML", m_name.c_str()));
	}

	// reorganize the tile symbols
	for (auto it = tileSymbolsEachLineReversed.rbegin(); it != tileSymbolsEachLineReversed.rend(); ++it)
	{
		interpretedString += *it; // Append each line in reverse order
	}
	return interpretedString;
}

Map::Map(MapDefinition& inputMapDefinition)
{
	m_mapDefinition = &inputMapDefinition; 
	m_gridSize = m_mapDefinition->m_gridSize;
}

Map::~Map()
{
	if (m_vertexBuffer)
	{
		delete m_vertexBuffer;
		m_vertexBuffer = nullptr;
	}	
	
	if (m_indexBuffer)
	{
		delete m_indexBuffer;
		m_indexBuffer = nullptr;
	}

	for (int i = 0; i < (int)m_units.size(); ++i)
	{
		delete m_units[i];
	}

	if (m_splineFlowingTimer)
	{
		delete m_splineFlowingTimer;
	}
}

void Map::Startup()
{
	UpdateCameraBoundsByMapDef();
	SetTilesFromXmlText();
	AddVertsForTileInWorldBounds();
	CreateVertexIndexBufferAndCopyFromCPUtoGPU();

	GenerateUnits();
	m_splineFlowingTimer = new Timer(m_splineFlowingInterval, g_theGameClock);
}

void Map::Update()
{
	UpdateUnitMoveTargetPath();
	if (g_theGame->m_currentTurnState == TurnState::UNIT_SELECTED_MOVE)
	{
		AddingVertsForMovingPathSpline();
	}
}

void Map::Render() const
{
	RenderTiles();
	if (g_theGame->m_currentTurnState == TurnState::UNIT_SELECTED_MOVE)
	{
		RenderTilesInUnitRange();
		RenderUnitMovementPath();
		RenderMovingPath();
	}
	if (g_theGame->m_currentTurnState == TurnState::UNIT_SELECTED_ATTACK)
	{
		RenderTilesForAttackTargets();
	}
}

void Map::RenderTiles() const
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->SetModelConstants(GetModelMatrix());
	g_theRenderer->BindShader(m_mapDefinition->m_overlayShader);
	// g_theRenderer->SetLightingConstants(*m_mapLightingSettings);
	g_theRenderer->DrawVertexArrayWithIndexArray(m_vertexBuffer, m_indexBuffer, (int)(m_indexArray.size()));
}

void Map::RenderTilesInUnitRange() const
{
	Unit* selectedUnit = g_theGame->m_selectedUnit;
	Rgba8 whiteTransparent(255, 255, 255, 100);
	if (selectedUnit && selectedUnit->m_player->m_netState == NetState::LOCAL && !selectedUnit->IsMoved())
	{
		vector<Vertex_PCU> rangeVerts;
		vector<unsigned int> rangeIndexes;

		for (int i = 0; i < (int)m_unitMovementRange.size(); ++i)
		{
			AddVertsForHexagon(rangeVerts, rangeIndexes, m_unitMovementRange[i]->m_centerWorldPos, m_tileInRadius, 0.f, true, whiteTransparent);
		}

		if (!rangeVerts.empty())
		{
			VertexBuffer* rangeVertBuffer = g_theRenderer->CreateVertexBuffer((size_t)(rangeVerts.size()), sizeof(Vertex_PCU));
			IndexBuffer* rangeIndexBuffer = g_theRenderer->CreateIndexBuffer((size_t)(rangeIndexes.size()));

			size_t vertexSize = sizeof(Vertex_PCU);
			size_t vertexArrayDataSize = (rangeVerts.size()) * vertexSize;
			g_theRenderer->CopyCPUToGPU(rangeVerts.data(), vertexArrayDataSize, rangeVertBuffer);

			size_t indexSize = sizeof(int);
			size_t indexArrayDataSize = rangeIndexes.size() * indexSize;
			g_theRenderer->CopyCPUToGPU(rangeIndexes.data(), indexArrayDataSize, rangeIndexBuffer);

			g_theRenderer->SetBlendMode(BlendMode::ALPHA);
			g_theRenderer->BindTexture(nullptr);
			g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
			g_theRenderer->SetDepthMode(DepthMode::DISABLED);
			g_theRenderer->SetModelConstants(GetModelMatrix());
			g_theRenderer->BindShader(m_mapDefinition->m_overlayShader);
			g_theRenderer->DrawVertexArrayWithIndexArray(rangeVertBuffer, rangeIndexBuffer, (int)(rangeIndexes.size()));

			delete rangeVertBuffer;
			delete rangeIndexBuffer;
		}
	}
}

void Map::RenderUnitMovementPath() const
{
	Unit* selectedUnit = g_theGame->m_selectedUnit;

	if (selectedUnit && selectedUnit->m_player->m_netState == NetState::LOCAL && !m_unitMovementPath.empty() && !selectedUnit->IsMoved())
	{
		vector<Vertex_PCU> pathVerts;
		vector<unsigned int> pathIndexes;
		Rgba8 whiteTransparent(255, 255, 255, 200);

		for (int i = 0; i < (int)m_unitMovementPath.size(); ++i)
		{
			AddVertsForHexagon(pathVerts, pathIndexes, m_unitMovementPath[i]->m_centerWorldPos, m_tileInRadius, 0.f, true, whiteTransparent);
		}		
		
		for (int i = 0; i < (int)m_unitMovementPath.size(); ++i)
		{
			Rgba8 color = g_theGame->m_playerSelectedUnitColorMap[g_theGame->m_players[g_theGame->m_currentTurnPlayerIndex]];
			AddVertsForHexagonFrame(pathVerts, pathIndexes, m_unitMovementPath[i]->m_centerWorldPos, m_tileInRadius, m_gridHalfOffset * 2.f, 0.f, true, color);
		}

		if (!pathVerts.empty())
		{
			VertexBuffer* rangeVertBuffer = g_theRenderer->CreateVertexBuffer((size_t)(pathVerts.size()), sizeof(Vertex_PCU));
			IndexBuffer* rangeIndexBuffer = g_theRenderer->CreateIndexBuffer((size_t)(pathIndexes.size()));

			size_t vertexSize = sizeof(Vertex_PCU);
			size_t vertexArrayDataSize = (pathVerts.size()) * vertexSize;
			g_theRenderer->CopyCPUToGPU(pathVerts.data(), vertexArrayDataSize, rangeVertBuffer);

			size_t indexSize = sizeof(int);
			size_t indexArrayDataSize = pathIndexes.size() * indexSize;
			g_theRenderer->CopyCPUToGPU(pathIndexes.data(), indexArrayDataSize, rangeIndexBuffer);

			g_theRenderer->SetBlendMode(BlendMode::ALPHA);
			g_theRenderer->BindTexture(nullptr);
			g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
			g_theRenderer->SetDepthMode(DepthMode::DISABLED);
			g_theRenderer->SetModelConstants(GetModelMatrix());
			g_theRenderer->BindShader(m_mapDefinition->m_overlayShader);
			g_theRenderer->DrawVertexArrayWithIndexArray(rangeVertBuffer, rangeIndexBuffer, (int)(pathIndexes.size()));

			delete rangeVertBuffer;
			delete rangeIndexBuffer;
		}
	}
}


void Map::GenerateUnitMovingPath()
{
	if (m_unitMovingPathSpline)
	{
		delete m_unitMovingPathSpline;
		m_unitMovingPathSpline = nullptr;
	}

	// based on the tiles of the path, get points on the spline
	vector<Vec3> splineSamplePoints;
	Vec3 startVel = Vec3();
	Vec3 endVel = Vec3();
	for (int i = 0; i < (int)m_unitMovementPath.size(); ++i)
	{
		Vec3 pos = Vec3();
		if (i == 0)	// start
		{
			splineSamplePoints.push_back(m_unitMovementPath[0]->m_centerWorldPos);
			continue;			
		}
		else if ( i > 1)	// incase a sharpe turn at start
		{
			// for the other tiles, we need to add a control point that connects its previous tile and itself
			Tile const*	currentTile = m_unitMovementPath[i];
			Tile const*	previousTile = m_unitMovementPath[i - 1];

			pos = (currentTile->m_centerWorldPos + previousTile->m_centerWorldPos) * 0.5f;
			splineSamplePoints.push_back(pos);
		}

		if (i == ((int)m_unitMovementPath.size() - 1))	// end tile
		{
			Vec3 endSamplePoint = m_unitMovementPath.back()->m_centerWorldPos;
			splineSamplePoints.push_back(endSamplePoint);
			Vec3 unitEndingDirection = (endSamplePoint - pos).GetNormalized();
			endVel = g_theGame->m_selectedUnit->m_tankMovingSpeed * 0.3f * unitEndingDirection;	// bug: if the speed is too high, we might get a twist
		}
	}

	if ((int)splineSamplePoints.size() == 1)
	{
		return;
	}

	// for (int i = 0; i < (int)splineSamplePoints.size(); ++i)
	// {
	// 	DebugAddWorldWireSphere(splineSamplePoints[i], 0.03f, 999.f, Rgba8::YELLOW);
	// }

	startVel = g_theGame->m_selectedUnit->m_tankMovingSpeed * (g_theGame->m_selectedUnit->m_orientation.GetForwardIBasis());
	m_unitMovingPathSpline = new CatmullRomSpline3D(splineSamplePoints, startVel, endVel);

	m_splineFlowingTimer->Start();
}

void Map::AddingVertsForMovingPathSpline()
{
	if (m_splineFlowingTimer->HasPeroidElapsed())
	{
		++m_drawingSectionIndex;
		if (m_drawingSectionIndex == m_drawingSectionNum)
		{
			m_drawingSectionIndex = 1;
		}
		m_splineFlowingTimer->Restart();
	}

	if (m_unitMovingPathSpline)
	{
		m_pathSplineVertexs.clear();
		AddVertsForCubicCatmullRomCurve(m_pathSplineVertexs, *m_unitMovingPathSpline, m_drawingSplineRadius, Rgba8::GREENISH_GRAY, m_drawingSectionNum, m_drawingSectionIndex, true);
	}
}

void Map::RenderMovingPath() const
{
	if (m_unitMovingPathSpline)
	{
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		g_theRenderer->SetModelConstants(Mat44());
		g_theRenderer->BindShader(m_mapDefinition->m_overlayShader);
		g_theRenderer->DrawVertexArray((int)m_pathSplineVertexs.size(), m_pathSplineVertexs.data());
	}
}

void Map::RenderTilesForAttackTargets() const
{
	vector<Vertex_PCU> attackVerts;
	vector<unsigned int> attackIndexes;
	Rgba8 attackColor;
	if (g_theGame->m_currentTurnPlayerIndex == 1)
	{
		attackColor = g_theGame->m_playerSelectedUnitColorMap[g_theGame->m_players[0]];
	}
	else if (g_theGame->m_currentTurnPlayerIndex == 0)
	{
		attackColor = g_theGame->m_playerSelectedUnitColorMap[g_theGame->m_players[1]];
	}

	float inRadius = m_tileInRadius - m_gridHalfOffset;
	for (int i = 0; i < (int)m_unitAttackTiles.size(); ++i)
	{
		AddVertsForHexagon(attackVerts, attackIndexes, m_unitAttackTiles[i]->m_centerWorldPos, inRadius, 0.f, true, attackColor);
	}

	if (!attackVerts.empty())
	{
		VertexBuffer* attackVertBuffer = g_theRenderer->CreateVertexBuffer((size_t)(attackVerts.size()), sizeof(Vertex_PCU));
		IndexBuffer* attackIndexBuffer = g_theRenderer->CreateIndexBuffer((size_t)(attackIndexes.size()));

		size_t vertexSize = sizeof(Vertex_PCU);
		size_t vertexArrayDataSize = (attackVerts.size()) * vertexSize;
		g_theRenderer->CopyCPUToGPU(attackVerts.data(), vertexArrayDataSize, attackVertBuffer);

		size_t indexSize = sizeof(int);
		size_t indexArrayDataSize = attackIndexes.size() * indexSize;
		g_theRenderer->CopyCPUToGPU(attackIndexes.data(), indexArrayDataSize, attackIndexBuffer);

		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetModelConstants(GetModelMatrix());
		g_theRenderer->BindShader(m_mapDefinition->m_overlayShader);
		g_theRenderer->DrawVertexArrayWithIndexArray(attackVertBuffer, attackIndexBuffer, (int)(attackIndexes.size()));

		delete attackVertBuffer;
		delete attackIndexBuffer;
	}
}

void Map::RenderUnits() const
{
	for (int i = 0; i < (int)m_units.size(); ++i)
	{
		m_units[i]->Render();
	}
}

void Map::UpdatePlayerUnitsAtTurnStart(int playerIndex)
{
	for (int i = 0; i < (int)m_units.size(); ++i)
	{
		if ( m_units[i]->m_player == g_theGame->m_players[playerIndex])
		{
			m_units[i]->m_hasAttacked = false;
			m_units[i]->m_hasMoved = false;

			m_units[i]->m_startHexIndex = m_units[i]->m_currentHexIndex;
		}
	}
}

std::vector<Tile const*> Map::GetNeighbors(const Tile& currentTile) const
{
	// Directions for hex neighbors in axial coordinates
	static const std::vector<IntVec2> directions = {
		IntVec2{1, 0}, 
		IntVec2{1, -1}, 
		IntVec2{0, -1}, 
		IntVec2{-1, 0}, 
		IntVec2{-1, 1}, 
		IntVec2{0, 1}
	};

	std::vector<Tile const*> neighbors;
	for (const auto& dir : directions) 
	{
		IntVec2 neighborCoords = currentTile.m_tileCoords + dir;

		// Find the neighbor tile in the list of all tiles
		auto neighbor = std::find_if(m_tiles.begin(), m_tiles.end(), [&](const Tile& tile) 
		{
			return tile.m_tileCoords == neighborCoords;
		});

		// Ensure the tile exists and is not blocked and is in boundary
		if (neighbor != m_tiles.end() && !neighbor->m_tileDef->m_isBlocked &&
			neighbor->m_centerWorldPos >= m_mapDefinition->m_worldBoundsMin &&
			neighbor->m_centerWorldPos <= m_mapDefinition->m_worldBoundsMax)
		{
			neighbors.push_back(&*neighbor);
		}
	}
	return neighbors;
}

void Map::UpdateUnitMoveTargetPath()
{
	Unit* selectedUnit = g_theGame->m_selectedUnit;
	if (selectedUnit)
	{
		if (!selectedUnit->m_hasMoved)
		{
			// if current focused tile is detected for A* once, not going to do second time
			if (selectedUnit &&
				g_theGame->m_selectedHexIndex != m_moveTargetTileIndexLastTime 
				&& g_theGame->m_selectedHexIndex != INVALID_HEX_INDEX)
			{
				// do A star path finding, if it takes more step than unit range, clear the path
				m_unitMovementPath = AStarPathfinding(&m_tiles[selectedUnit->m_currentHexIndex], &m_tiles[g_theGame->m_selectedHexIndex]);
				m_moveTargetTileIndexLastTime = g_theGame->m_selectedHexIndex;

				if ((m_unitMovementPath.size() - 1) > selectedUnit->m_unitDef->m_movementRange)
				{
					m_unitMovementPath.clear();
					if (m_unitMovingPathSpline)
					{
						delete m_unitMovingPathSpline;
						m_unitMovingPathSpline = nullptr;
					}
					m_mouseInUnitMovementRange = false;
				}
				else
				{
					m_mouseInUnitMovementRange = true;
					GenerateUnitMovingPath();
				}
			}
		}
	}
	else
	{
		m_unitMovementPath.clear();
		if (m_unitMovingPathSpline)
		{
			delete m_unitMovingPathSpline;
			m_unitMovingPathSpline = nullptr;
		}
		m_mouseInUnitMovementRange = false;
	}
}

bool Map::MoveTargetIsInRange() const
{
	return (!m_unitMovementPath.empty());
}

std::vector<Tile const*> Map::AStarPathfinding(Tile* startTile, Tile* goalTile) const
{
	std::priority_queue<
		std::pair<int, Tile const*>,				// Store cost and tile pointer
		std::vector<std::pair<int, Tile const*>>,	// Underlying container
		std::greater<>								// Compare: A comparison function to determine priority (default: std::less<Type> for max-heap).
	> openSet;

	// Maps to track the cost from start and estimated total cost (fScore)
	std::map<Tile const*, int> gScore;				// The cost from the tile to start tile, use this score to determine which is better neighbor, Dijkstra only use this to decide which is better route
	std::map<Tile const*, int> fScore;				// Estimated total cost (g + h)
	std::map<Tile const*, Tile const*> cameFrom;	// Path reconstruction map

	// Initialize start tile with gScore = 0 and heuristic (h) as its fScore
	gScore[startTile] = 0;
	fScore[startTile] = startTile->m_tileCoords.GetManhattanDistBetweenHexCoords(goalTile->m_tileCoords);
	openSet.push({ fScore[startTile], startTile });		// int, Tile*

	// Main loop: Process tiles until the goal is reached or no tiles are left
	while (!openSet.empty()) 
	{
		// Get the tile with the lowest fScore from the priority queue
		Tile const* currentTile = openSet.top().second;
		openSet.pop();

		// If we've reached the goal, reconstruct the path and return it
		if (currentTile == goalTile) 
		{
			std::vector<Tile const*> path;
			while (currentTile != startTile) 
			{
				path.push_back(currentTile);
				currentTile = cameFrom[currentTile];
			}
			path.push_back(startTile);
			std::reverse(path.begin(), path.end());  // Start-to-goal order
			return path;
		}

		// Explore each valid neighbor of the current tile
		for (Tile const* neighbor : GetNeighbors(*currentTile)) 
		{
			// Calculate tentative gScore for the neighbor
			int tentativeGScore = gScore[currentTile] + 1; // Assuming uniform movement cost: each step is 1 cost

			// If this path to the neighbor is better than previously known
			if (!gScore.count(neighbor) || tentativeGScore < gScore[neighbor]) // for .count, 0 is found, so this line means(no neighbor or gSore is lower)
			{
				cameFrom[neighbor] = currentTile;		// Update path
				gScore[neighbor] = tentativeGScore;		// Update gScore
				fScore[neighbor] = gScore[neighbor] + neighbor->m_tileCoords.GetManhattanDistBetweenHexCoords(goalTile->m_tileCoords);	// gScore + dist to goal tile

				// Add neighbor to the open set with updated priority
				openSet.push({ fScore[neighbor], neighbor });
			}
		}
	}

	// If we exit the loop without finding a path, return an empty path
	return {};
}

Mat44 Map::GetModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_mapOrigin);
	Mat44 orientationMat = m_mapOrientation.GetAsMatrix_XFwd_YLeft_ZUp();
	transformMat.Append(orientationMat);
	return transformMat;
}

void Map::UpdateSelectedUnitMovementRange()
{
	Unit* selectedUnit = g_theGame->m_selectedUnit;
	m_unitMovementRange.clear();
	if (selectedUnit && selectedUnit->m_player->m_netState == NetState::LOCAL)
	{
		IntVec2 unitCoords = m_tiles[selectedUnit->m_currentHexIndex].m_tileCoords;
		for (int i = 0; i < (int)m_tiles.size(); ++i)
		{
			Tile const* tile = &m_tiles[i];
			int dist = (int)AStarPathfinding(const_cast<Tile*>(&m_tiles[i]), const_cast<Tile*>(&m_tiles[GetTileIndex_For_TileCoordinates(unitCoords)])).size() - 1;
			if (dist <= selectedUnit->m_unitDef->m_movementRange && !tile->m_tileDef->m_isBlocked &&
				tile->m_centerWorldPos >= m_mapDefinition->m_worldBoundsMin &&
				tile->m_centerWorldPos <= m_mapDefinition->m_worldBoundsMax)
			{
				m_unitMovementRange.push_back(tile);
			}
		}
	}
}

void Map::UpdateTilesInAttackRange()
{
	m_unitAttackTiles.clear();

	Unit* selectedUnit =  g_theGame->m_selectedUnit;
	int rangeMin = selectedUnit->m_unitDef->m_groundAttackRangeMin;
	int rangeMax = selectedUnit->m_unitDef->m_groundAttackRangeMax;

	for (int i = 0; i < (int)m_units.size(); ++i)
	{
		Unit* targetUnit = m_units[i];
		if (targetUnit->m_player != g_theGame->m_players[g_theGame->m_currentTurnPlayerIndex])
		{
			int dist = (int)AStarPathfinding(&m_tiles[selectedUnit->m_currentHexIndex], &m_tiles[targetUnit->m_currentHexIndex]).size() - 1;
			if (dist <= rangeMax && dist >= rangeMin)
			{
				m_unitAttackTiles.push_back(&m_tiles[targetUnit->m_currentHexIndex]);
			}
		}
	}
}

void Map::CheckAndRemoveTheUnitWhenItDies(Unit* unit)
{
	if (unit->m_unitHealth <= 0)
	{
		// Find the pointer in the vector
		auto it = std::find(m_units.begin(), m_units.end(), unit);

		if (unit == g_theGame->m_defensingUnit)
		{
			g_theGame->m_defensingUnit = nullptr;
		}

		// If the pointer is found
		if (it != m_units.end()) 
		{
			delete* it;               // Delete the object being pointed to
			m_units.erase(it);        // Remove the pointer from the vector
		}
	}
}

void Map::UpdateCameraBoundsByMapDef()
{
	for (int i = 0; i < (int)g_theGame->m_players.size(); ++i)
	{
		if (g_theGame->m_players[i])
		{
			g_theGame->m_players[i]->m_mapBoundsMin = m_mapDefinition->m_worldBoundsMin;
			g_theGame->m_players[i]->m_mapBoundsMax = m_mapDefinition->m_worldBoundsMax;
		}
	}
}

void Map::GenerateUnits()
{
	// set player 1 and player 2 units
	for (auto it = m_mapDefinition->m_playerUnitSymbols.begin(); it != m_mapDefinition->m_playerUnitSymbols.end(); ++it)
	{
		int playerIndex = it->first;
		std::string unitSymbols = it->second;
		for (int columnIndex = 0; columnIndex < (int)m_gridSize.y; ++columnIndex)
		{
			for (int rowIndex = 0; rowIndex < (int)m_gridSize.y; ++rowIndex)
			{
				int tileIndex = GetTileIndex_For_TileCoordinates(IntVec2(rowIndex, columnIndex));
				UnitDefinition const* unitDef = UnitDefinition::GetDefBySymbol(unitSymbols[tileIndex]);
				if (!unitDef)
				{
					continue;
				}
				else
				{
					Unit* newUnit = new Unit(unitDef, this, g_theGame->m_players[playerIndex - 1], tileIndex);
					newUnit->Startup();
					m_units.push_back(newUnit);
				}
			}
		}
	}
}

Player* Map::CheckWhichPlayerUnitIsOnTile(int tileIndex)
{
	if (tileIndex == INVALID_HEX_INDEX)
	{
		return nullptr;
	}

	for (int i = 0; i < (int)m_units.size(); ++i)
	{
		if (m_units[i]->m_currentHexIndex == tileIndex)
		{
			return m_units[i]->m_player;
		}
	}

	return nullptr;
}

Unit* Map::GetUnitCurrentlyOnThisTile(int tileIndex)
{
	if (tileIndex == INVALID_HEX_INDEX)
	{
		return nullptr;
	}

	for (int i = 0; i < (int)m_units.size(); ++i)
	{
		if (m_units[i]->m_currentHexIndex == tileIndex)
		{
			return m_units[i];
		}
	}

	return nullptr;
}

int Map::GetUnitIndex(Unit* unit)
{
	for (int i = 0; i < (int)m_units.size(); ++i)
	{
		if (m_units[i] == unit)
		{
			return i;
		}
	}

	return 999;
}

void Map::SetTilesFromXmlText()
{
	int numTiles = m_gridSize.x * m_gridSize.y;
	m_tiles.resize(numTiles);

	// set all the tiles type by the text
	for (int columnIndex = 0; columnIndex < (int)m_gridSize.y; ++columnIndex)
	{
		for (int rowIndex = 0; rowIndex < (int)m_gridSize.y; ++rowIndex)
		{
			int tileIndex = GetTileIndex_For_TileCoordinates(IntVec2(rowIndex, columnIndex));
			char tileSymbol = m_mapDefinition->m_gridSymbols[tileIndex];
			TileTypeDefinition const* tileTypeDef = TileTypeDefinition::GetBySymbol(tileSymbol);
			SetTileType(rowIndex, columnIndex, tileTypeDef);
		}
	}
}

void Map::SetTileType(int tileX, int tileY, TileTypeDefinition const* type)
{
	int tileIndex = tileX + (tileY * m_gridSize.x);
	m_tiles[tileIndex].SetTileCoordsAndType(IntVec2(tileX, tileY), type);
}

void Map::AddVertsForTileInWorldBounds()
{
	// check if the center of the hexagon is in the world bounds
	// otherwise, don't draw it
	for (int i = 0; i < (int)m_tiles.size(); ++i)
	{
		Tile& tile = m_tiles[i];
		Vec3& center = tile.m_centerWorldPos;
		if (tile.m_centerWorldPos >= m_mapDefinition->m_worldBoundsMin &&
			tile.m_centerWorldPos <= m_mapDefinition->m_worldBoundsMax)
		{
			if (tile.m_tileDef->m_isBlocked)
			{
				AddVertsForHexagon(m_vertexs, m_indexArray, center, (m_tileInRadius - m_gridHalfOffset), 0.f, true, Rgba8::BLACK_TRANSPARENT);	// for render info
				AddBoundryVerticesForHexagon(tile.m_boundaries, center, m_tileInRadius);	// for boundary info
			}
			else
			{
				AddVertsForHexagonFrame(m_vertexs, m_indexArray, center, m_tileInRadius, m_gridHalfOffset);	// for render info
				AddBoundryVerticesForHexagon(tile.m_boundaries, center, m_tileInRadius);	// for boundary info
			}
		}
	}
}

void Map::CreateVertexIndexBufferAndCopyFromCPUtoGPU()
{
	// create vertex buffer and index buffer
	m_vertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_vertexs.size()), sizeof(Vertex_PCU));
	m_indexBuffer = g_theRenderer->CreateIndexBuffer((size_t)(m_indexArray.size()));

	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArrayDataSize = (m_vertexs.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_vertexs.data(), vertexArrayDataSize, m_vertexBuffer);

	size_t indexSize = sizeof(int);
	size_t indexArrayDataSize = m_indexArray.size() * indexSize;
	g_theRenderer->CopyCPUToGPU(m_indexArray.data(), indexArrayDataSize, m_indexBuffer);
}

int Map::GetTileIndex_For_TileCoordinates(IntVec2 tileCoord) const
{
	int tileIndex = tileCoord.x + (tileCoord.y * m_gridSize.x);
	return tileIndex;
}

IntVec2 Map::GetTileCoords_For_TileIndex(int tileIndex) const
{
	IntVec2 coords;
	coords.y = tileIndex / m_gridSize.x;
	coords.x = tileIndex % m_gridSize.x;
	return coords;
}
