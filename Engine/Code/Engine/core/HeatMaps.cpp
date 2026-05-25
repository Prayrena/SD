#include "Engine/core/HeatMaps.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/VertexUtils.hpp"

TileHeatMap::TileHeatMap(IntVec2 const& dimensions)
	:m_dimensions(dimensions)
{

}

void TileHeatMap::SetDefaultHeatValueForAllTiles(float defaultHeatValue)
{
	m_values.clear();
	int num_tiles = m_dimensions.x * m_dimensions.y;
	m_values.resize(num_tiles);// need to resize instead of reserve otherwise the size is 0

	for (int tileIndex = 0; tileIndex < (int)m_values.size(); ++tileIndex)
	{
		m_values[tileIndex] = defaultHeatValue;
	}
}

void TileHeatMap::SetHeatValueForTile(IntVec2 tileCoords, float heatValue)
{
	int tileIndex = GetTileIndexForTileCoords(tileCoords);
	m_values[tileIndex] = heatValue;
}

float TileHeatMap::GetHeatValueAt(IntVec2 tileCoords)
{
	int tileIndex = GetTileIndexForTileCoords(tileCoords);
	return m_values[tileIndex];
}

float TileHeatMap::GetTheMaxHeatValue()
{
	float	maxHeatValue = 0.f;

	// calculate the value range from m_value
	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			int tileIndex = GetTileIndexForTileCoords(IntVec2(tileX, tileY));
			if (m_values[tileIndex] != 999999.f && m_values[tileIndex] > maxHeatValue)
			{
				maxHeatValue = m_values[tileIndex];
			}
		}
	}
	return maxHeatValue;
}

void TileHeatMap::AddVertsForTileHeatMapDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 bounds, FloatRange valueRange, Rgba8 lowColor, Rgba8 highColor, float specialValue, Rgba8 specialColor)
{
	verts.clear();
	verts.reserve(m_dimensions.x * m_dimensions.y * 6);
	int tileIndex;
	AABB2 tileBounds;
	float heatValue;
	Rgba8 tileColor;

	// loop through the whole current map to draw the heat map only for the tiles inside the bounds area
	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			// get the center of the tile to see if the tile is inside the drawing bounds
			Vec2 tileCenterPos((float)tileX + 0.5f, (float)tileY + 0.5f);
			if (bounds.IsPointInside(tileCenterPos))
			{
				tileIndex = GetTileIndexForTileCoords(IntVec2(tileX, tileY));
				tileBounds = AABB2(Vec2((float)tileX, (float)tileY), Vec2((float)tileX + 1.f, (float)tileY + 1.f));
				heatValue = m_values[tileIndex];

				// get the color value
				if (heatValue != specialValue)
				{
					float tileColorFraction = RangeMapClamped(heatValue, valueRange.m_min, valueRange.m_max, 0.f, 1.f);
					tileColor = InterpolateRGB(lowColor, highColor, tileColorFraction);
				}
				else
				{
					tileColor = specialColor;
				}

				AddVertsUVForAABB2D(verts, tileBounds, tileColor);
			}
		}
	}
	return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// <tile helper functions>
int TileHeatMap::GetTileIndexForTileCoords(IntVec2 tileCoords)
{
	int tileIndex = tileCoords.x + tileCoords.y * m_dimensions.x;
	return tileIndex;
}
