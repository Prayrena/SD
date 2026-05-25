#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include <vector>

class Map;

class TileHeatMap 
{
public:
	TileHeatMap(IntVec2 const& dimensions);
	~TileHeatMap() {};

public:
	void	SetDefaultHeatValueForAllTiles(float defaultHeatValue);
	void	SetHeatValueForTile(IntVec2 tileCoords, float heatValue);
	float	GetHeatValueAt(IntVec2 tileCoords);
	float	GetTheMaxHeatValue();

	// todo: generate opacity map based on solid map
	void	GenerateAnotherHeatMap_BasedOnKnownHeatMap(IntVec2 startCoords, float maxHeat, TileHeatMap const& solidMap, FloatRange newSolidEvaluationRange = FloatRange::ZERO_TO_ONE);

	void	AddVertsForTileHeatMapDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 bounds, FloatRange valueRange = FloatRange::ZERO_TO_ONE, 
		Rgba8 lowColor=Rgba8(0,0,0,255), Rgba8 highColor=Rgba8(255,255,255,100), float specialValue=999999.f, Rgba8 specialColor=Rgba8(255,0,255));
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// tile helper functions
	int		GetTileIndexForTileCoords(IntVec2 tileCoords);

	IntVec2 m_dimensions;
	std::vector<float> m_values;
};