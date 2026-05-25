#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/core/HeatMaps.hpp"
#include "Engine/core/XmlUtils.hpp"
#include "Engine/core/Vertex_PCUTBN.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/core/Vertex_PCUTBN.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Game/Tile.hpp"
#include "Game/Entity.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Unit.hpp"
#include <vector>
#include <string>

struct	RaycastResult3D;
struct	LightingConstants;
class	Image;
class	VertexBuffer;
class	IndexBuffer;
class   Shader;

bool operator>=(Vec3 const& a, Vec3 const& b);
bool operator<=(Vec3 const& a, Vec3 const& b);

struct MapDefinition
{
public:

	MapDefinition() = default;
	~MapDefinition() = default;
	MapDefinition(XmlElement const& tileDefElement);

	std::string		m_name = "not Initialized";
	std::string		m_mapImagePath; // the relative file path name of a .PNG image file

	Shader*			m_overlayShader = nullptr;

	// overlay hex texture, currently not in use
	// Image*			m_image = nullptr;
	// Texture*		m_spriteSheetTexture = nullptr;
	// SpriteSheet*	m_spriteSheet = nullptr;
	// IntVec2			m_spriteSheetCount;

	IntVec2						m_gridSize = IntVec2(15, 15);
	std::string					m_gridSymbols;
	std::map<int, std::string>	m_playerUnitSymbols;

	Vec3			m_worldBoundsMin;
	Vec3			m_worldBoundsMax;

	char const* gridChars;
	std::string tileChars;

	std::string InterpretCDATA(char const* chars);

	static void InitializeMapDefs();
	static void ClearMapDefinitions();
	static MapDefinition* const GetByName(std::string const& name);
	static std::vector<MapDefinition> s_mapDefs;
};

class Map
{
	friend class Game;

public:
	Map(MapDefinition& inputMapDefinition);
	~Map();

	void Startup();
	void Update();

	void	Render() const;
	void	RenderTiles() const;
	void	RenderTilesInUnitRange() const;
	Mat44	GetModelMatrix() const;

	// find the closest path to the focused tile
	std::vector<Tile const*>	GetNeighbors(const Tile& currentTile) const;
	void						UpdateUnitMoveTargetPath();
	bool						m_mouseInUnitMovementRange = false;
	std::vector<Tile const*>	AStarPathfinding(Tile* start, Tile* goal) const;
	void						RenderUnitMovementPath() const;
	std::vector<Tile const*>	m_unitMovementPath;

	void						GenerateUnitMovingPath();
	CatmullRomSpline3D*			m_unitMovingPathSpline = nullptr;
	void						AddingVertsForMovingPathSpline();
	void						RenderMovingPath() const;
	std::vector<Vertex_PCU>		m_pathSplineVertexs;
	Timer*						m_splineFlowingTimer = nullptr;
	float						m_splineFlowingInterval = 0.016f;
	float						m_drawingSplineRadius = 0.03f;
	int							m_drawingSectionNum = 9;
	int							m_drawingSectionIndex = 1;

	
	int							m_moveTargetTileIndexLastTime = 0;
	bool						MoveTargetIsInRange() const;

	void						UpdateSelectedUnitMovementRange();
	std::vector<Tile const*>	m_unitMovementRange;

	void						UpdateTilesInAttackRange();
	void						RenderTilesForAttackTargets() const;
	std::vector<Tile const*>	m_unitAttackTiles;

	void						CheckAndRemoveTheUnitWhenItDies(Unit* unit);

	void	UpdateCameraBoundsByMapDef();

	// Units
	void	GenerateUnits();
	Player*	CheckWhichPlayerUnitIsOnTile(int tileIndex);
	Unit*	GetUnitCurrentlyOnThisTile(int tileIndex);
	int		GetUnitIndex(Unit* unit);
	void	RenderUnits() const;
	std::vector<Unit*> m_units;

	void	UpdatePlayerUnitsAtTurnStart(int playerIndex);

	// collisions
	float worldZCollisionOffset = 0.f; // Avoid the edge of the shape is right on the floor/ceiling

	// tiles
	void SetTilesFromXmlText();
	void SetTileType(int tileX, int tileY, TileTypeDefinition const* type);
	void AddVertsForTileInWorldBounds();
	void CreateVertexIndexBufferAndCopyFromCPUtoGPU();


	MapDefinition*		m_mapDefinition;
	IntVec2			    m_gridSize; // contains the overall wide(x) and high(Y)
	std::vector<Tile>   m_tiles;

	float				m_gridHalfOffset = 0.03f;
	float				m_tileInRadius = 0.5f;

	// index
	int     GetTileIndex_For_TileCoordinates(IntVec2 tileCoord) const;
	IntVec2 GetTileCoords_For_TileIndex(int tileIndex) const;

	// map physics info
	Vec3				 m_mapOrigin;
	EulerAngles			 m_mapOrientation;

	std::vector<Vertex_PCU>		m_vertexs;
	std::vector<unsigned int>	m_indexArray;

	VertexBuffer*				m_vertexBuffer = nullptr;
	IndexBuffer*				m_indexBuffer = nullptr;
};