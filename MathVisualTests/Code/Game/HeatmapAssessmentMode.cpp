#include "Game/HeatmapAssessmentMode.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"
#include "Engine/core/DevConsole.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/core/Image.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <set>
#include <queue>
#include <algorithm>

extern Renderer*    g_theRenderer;
extern InputSystem* g_theInput;

//-----------------------------------------------------------------------------------------------
// Returns the elevation modifier for a given average height, per spawn_design_notes.md section 1.3.
// Bands: 0-50m=1.0, 51-200m=0.9, 201-500m=0.7, >500m=0.4
//-----------------------------------------------------------------------------------------------
static float GetElevationModifier(float avgHeight)
{
	if (avgHeight <= 50.f)  return 1.0f;
	if (avgHeight <= 200.f) return 0.9f;
	if (avgHeight <= 500.f) return 0.7f;
	return 0.4f;
}

//-----------------------------------------------------------------------------------------------
// Maps a height value to a grayscale Rgba8 color for the HeightMap display.
// Black = low elevation, white = high elevation. Full opacity.
//-----------------------------------------------------------------------------------------------
static Rgba8 HeightToGrayscale(float height, float minH, float maxH)
{
	float range = maxH - minH;
	if (range < 1.f) range = 1.f;
	float t = (height - minH) / range;
	if (t < 0.f) t = 0.f;
	if (t > 1.f) t = 1.f;
	return InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 255, 255, 255), t);
}

//-----------------------------------------------------------------------------------------------
// Classifies a road_map.png pixel color into a CellType by checking which channel dominates.
//-----------------------------------------------------------------------------------------------
static CellType ClassifyRoadMapPixel(Rgba8 const& color)
{
	if (color.b > 100 && color.b > color.r && color.b > color.g)
		return CellType::WATER;
	if (color.g > 100 && color.g > color.r && color.g > color.b)
		return CellType::SETTLEMENT;
	if (color.r > 100 && color.r > color.g && color.r > color.b)
		return CellType::ROAD;
	return CellType::TERRAIN;
}

//-----------------------------------------------------------------------------------------------
HeatmapAssessmentMode::HeatmapAssessmentMode()
{
}

HeatmapAssessmentMode::~HeatmapAssessmentMode()
{
}

//-----------------------------------------------------------------------------------------------
// Initializes the mode: loads data, classifies cells, builds height grid with multi-pass fill,
// then builds vertex buffers for rendering.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::Startup()
{
	LoadCSVData();
	LoadMapTextures();
	LoadImageData();

	// Derive grid dimensions from actual image size and sample rate
	IntVec2 imgDims = m_zoneHeatmapImage->GetDimensions();
	m_gridWidth     = imgDims.x / HEATMAP_PIXELS_PER_CELL;
	m_gridHeight    = imgDims.y / HEATMAP_PIXELS_PER_CELL;
	m_cellWorldSize = (float)HEATMAP_PIXELS_PER_CELL * HEATMAP_METERS_PER_PIXEL;
	IntVec2 roadDims = m_roadMapImage->GetDimensions();
	g_theDevConsole->AddLine(Stringf("HeatmapAssessment: zone image %dx%d, road image %dx%d -> grid %dx%d (px/cell %d, cell %.0fm)",
		imgDims.x, imgDims.y, roadDims.x, roadDims.y, m_gridWidth, m_gridHeight, HEATMAP_PIXELS_PER_CELL, m_cellWorldSize), Rgba8::WHITE);

	// Set world camera: map right-bottom aligns with window right-bottom
	float mapUnits  = (float)m_gridHeight;
	float aspect    = SCREEN_CAMERA_ORTHO_X / SCREEN_CAMERA_ORTHO_Y;
	float viewWidth = mapUnits * aspect;
	m_mapOffsetX = viewWidth - (float)m_gridWidth;
	m_worldCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(viewWidth, mapUnits));

	// Preprocessing
	BuildZoneColorLookup();
	g_theDevConsole->AddLine(Stringf("Zone colors loaded: %d", (int)m_zoneColors.size()), Rgba8::WHITE);

	CalibrateZoneElevation();
	for (auto& [zoneId, avgH] : m_zoneAvgHeight)
	{
		g_theDevConsole->AddLine(Stringf("  Zone %d: avgHeight=%.1f  elevMod=%.2f", zoneId, avgH, m_zoneElevationMod[zoneId]), Rgba8::GRAY);
	}

	// Multi-pass height grid fill
	BuildCellTypeGrid();
	BuildHeightGrid();          // pass 1: per-cell monster heights
	FillTerrainHeights();       // pass 2: all non-road/non-water empty cells from zone average
	FillRoadHeights();          // pass 3: road cells via Laplacian relaxation
	FillWaterHeights();         // pass 4: water cells via Laplacian relaxation
	SmoothTerrainHeights();     // pass 5: smooth everything except road and water

	// Expected density computation
	BuildRoadDistanceField();
	ComputeSettlementDistances();
	BuildExpectedDensityGrid();

	// Threat level grid
	BuildThreatGrid();

	// Diversity grid
	BuildDiversityGrid();

	BuildBackgroundVerts();
	BuildHeatmapVerts();
	g_theDevConsole->AddLine(Stringf("Heatmap verts built: %d", (int)m_heatmapVerts.size()), Rgba8::WHITE);

	UpdateModeInfo();
}

//-----------------------------------------------------------------------------------------------
// Loads all 4 CSV data files. Dies on failure with a descriptive message.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::LoadCSVData()
{
	bool success = true;
	success = m_monsterPoints.LoadFile("Data/DesignData/monster_points.csv");
	GUARANTEE_OR_DIE(success, "Failed to load monster_points.csv");
	success = m_monsterReference.LoadFile("Data/DesignData/monster_reference.csv");
	GUARANTEE_OR_DIE(success, "Failed to load monster_reference.csv");
	success = m_zoneReference.LoadFile("Data/DesignData/zone_reference.csv");
	GUARANTEE_OR_DIE(success, "Failed to load zone_reference.csv");
	success = m_settlements.LoadFile("Data/DesignData/settlements.csv");
	GUARANTEE_OR_DIE(success, "Failed to load settlements.csv");
}

//-----------------------------------------------------------------------------------------------
// Loads zone_heatmap.png and road_map.png as GPU textures for background rendering.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::LoadMapTextures()
{
	m_zoneHeatmapTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/zone_heatmap.png");
	m_roadMapTexture     = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/road_map.png");
}

//-----------------------------------------------------------------------------------------------
// Loads zone_heatmap.png and road_map.png as CPU-side Images for pixel color sampling.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::LoadImageData()
{
	m_zoneHeatmapImage = g_theRenderer->CreateImageFromFile("Data/Images/zone_heatmap.png");
	m_roadMapImage     = g_theRenderer->CreateImageFromFile("Data/Images/road_map.png");
}

//-----------------------------------------------------------------------------------------------
// Parses zone_reference.csv to build a lookup table mapping pixel RGB colors to zone IDs.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildZoneColorLookup()
{
	int colId       = m_zoneReference.GetColumnIndex("zone_id");
	int colRgb      = m_zoneReference.GetColumnIndex("map_color_rgb");
	int colCategory = m_zoneReference.GetColumnIndex("zone_category");
	GUARANTEE_OR_DIE(colId >= 0 && colRgb >= 0 && colCategory >= 0,
		"zone_reference.csv missing zone_id, map_color_rgb, or zone_category column");

	for (int i = 0; i < m_zoneReference.GetRowCount(); ++i)
	{
		Strings const& row = m_zoneReference.m_rows[i];
		int zoneId = ParseCsvValue(row, colId, -1);
		std::string rgbStr   = ParseCsvValue(row, colRgb, std::string(""));
		std::string category = ParseCsvValue(row, colCategory, std::string(""));

		Strings rgbParts = SplitStringOnDelimiter(rgbStr, ',');
		if (rgbParts.size() < 3)
			continue;

		ZoneColorEntry entry;
		entry.zoneId = zoneId;
		entry.r = (unsigned char)atoi(rgbParts[0].c_str());
		entry.g = (unsigned char)atoi(rgbParts[1].c_str());
		entry.b = (unsigned char)atoi(rgbParts[2].c_str());
		m_zoneColors.push_back(entry);

		// Store zone category for later use (skip settlements during elevation calibration, etc.)
		m_zoneCategory[zoneId] = category;
	}
}

//-----------------------------------------------------------------------------------------------
// Finds the zone_id whose reference color is closest (Euclidean RGB distance) to the given color.
//-----------------------------------------------------------------------------------------------
int HeatmapAssessmentMode::FindNearestZoneId(Rgba8 const& color) const
{
	int bestId = -1;
	int bestDist = INT_MAX;
	for (ZoneColorEntry const& entry : m_zoneColors)
	{
		int dr = (int)color.r - (int)entry.r;
		int dg = (int)color.g - (int)entry.g;
		int db = (int)color.b - (int)entry.b;
		int dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist)
		{
			bestDist = dist;
			bestId = entry.zoneId;
		}
	}
	return bestId;
}

//-----------------------------------------------------------------------------------------------
// Samples zone_heatmap.png at a world position, accounting for stbi vertical flip.
//-----------------------------------------------------------------------------------------------
Rgba8 HeatmapAssessmentMode::SampleZoneImageAtWorldPos(float worldX, float worldY) const
{
	IntVec2 dims = m_zoneHeatmapImage->GetDimensions();
	int pngX = (int)(worldX / HEATMAP_METERS_PER_PIXEL);
	int pngY = (int)(worldY / HEATMAP_METERS_PER_PIXEL);
	if (pngX < 0) pngX = 0;
	if (pngY < 0) pngY = 0;
	if (pngX >= dims.x) pngX = dims.x - 1;
	if (pngY >= dims.y) pngY = dims.y - 1;
	int imageY = (dims.y - 1) - pngY;
	return m_zoneHeatmapImage->GetTexelColor(IntVec2(pngX, imageY));
}

//-----------------------------------------------------------------------------------------------
// Samples road_map.png at the center pixel of a grid cell, accounting for stbi vertical flip.
//-----------------------------------------------------------------------------------------------
Rgba8 HeatmapAssessmentMode::SampleRoadImageAtGridCell(int col, int row) const
{
	IntVec2 dims = m_roadMapImage->GetDimensions();
	// Center pixel of this grid cell
	int pngX = col * HEATMAP_PIXELS_PER_CELL + HEATMAP_PIXELS_PER_CELL / 2;
	int pngY = row * HEATMAP_PIXELS_PER_CELL + HEATMAP_PIXELS_PER_CELL / 2;
	if (pngX < 0) pngX = 0;
	if (pngY < 0) pngY = 0;
	if (pngX >= dims.x) pngX = dims.x - 1;
	if (pngY >= dims.y) pngY = dims.y - 1;
	int imageY = (dims.y - 1) - pngY;
	return m_roadMapImage->GetTexelColor(IntVec2(pngX, imageY));
}

//-----------------------------------------------------------------------------------------------
// Calibrates zone elevation by averaging monster heights per zone.
// All zones included. Zones with no monsters fall back to default (100m).
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::CalibrateZoneElevation()
{
	int colX = m_monsterPoints.GetColumnIndex("world_x");
	int colY = m_monsterPoints.GetColumnIndex("world_y");
	int colH = m_monsterPoints.GetColumnIndex("height");
	GUARANTEE_OR_DIE(colX >= 0 && colY >= 0 && colH >= 0, "monster_points.csv missing required columns");

	std::map<int, float> zoneSumHeight;
	std::map<int, int>   zoneCount;

	for (int i = 0; i < m_monsterPoints.GetRowCount(); ++i)
	{
		Strings const& row = m_monsterPoints.m_rows[i];
		float wx = ParseCsvValue(row, colX, 0.f);
		float wy = ParseCsvValue(row, colY, 0.f);
		float h  = ParseCsvValue(row, colH, 0.f);

		Rgba8 pixelColor = SampleZoneImageAtWorldPos(wx, wy);
		int zoneId = FindNearestZoneId(pixelColor);
		if (zoneId < 0) continue;

		zoneSumHeight[zoneId] += h;
		zoneCount[zoneId] += 1;
	}

	int zonesWithData = 0;
	int zonesWithoutData = 0;

	for (auto& [zoneId, sumH] : zoneSumHeight)
	{
		float avgH = sumH / (float)zoneCount[zoneId];
		m_zoneAvgHeight[zoneId] = avgH;
		m_zoneElevationMod[zoneId] = GetElevationModifier(avgH);
		++zonesWithData;
		g_theDevConsole->AddLine(Stringf("  Zone %d: avgHeight=%.1f (%d monsters)", zoneId, avgH, zoneCount[zoneId]), Rgba8::GRAY);
	}

	for (ZoneColorEntry const& entry : m_zoneColors)
	{
		if (m_zoneAvgHeight.find(entry.zoneId) != m_zoneAvgHeight.end())
			continue;

		m_zoneAvgHeight[entry.zoneId] = 100.f;
		m_zoneElevationMod[entry.zoneId] = 0.9f;
		++zonesWithoutData;
		g_theDevConsole->AddLine(Stringf("  Zone %d: no monsters, fallback 100m", entry.zoneId), Rgba8::YELLOW);
	}

	g_theDevConsole->AddLine(Stringf("Zone elevation: %d with data, %d fallback",
		zonesWithData, zonesWithoutData),
		zonesWithoutData > 0 ? Rgba8::YELLOW : Rgba8::GREEN);
}

//-----------------------------------------------------------------------------------------------
// Classifies each grid cell as terrain/water/settlement/road by scanning ALL pixels in the cell.
// If ANY pixel in the cell is road or water, the entire cell is classified as that type.
// Priority: WATER > ROAD > SETTLEMENT > TERRAIN (water and road take precedence for visibility).
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildCellTypeGrid()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_cellTypeGrid.resize(totalCells, CellType::TERRAIN);
	IntVec2 dims = m_roadMapImage->GetDimensions();

	int waterCount = 0, settlementCount = 0, roadCount = 0;

	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			// Scan all pixels within this cell's region
			int pxStartX = col * HEATMAP_PIXELS_PER_CELL;
			int pxStartY = row * HEATMAP_PIXELS_PER_CELL;
			int pxEndX   = pxStartX + HEATMAP_PIXELS_PER_CELL;
			int pxEndY   = pxStartY + HEATMAP_PIXELS_PER_CELL;
			if (pxEndX > dims.x) pxEndX = dims.x;
			if (pxEndY > dims.y) pxEndY = dims.y;

			bool hasWater = false;
			bool hasRoad = false;
			bool hasSettlement = false;

			for (int py = pxStartY; py < pxEndY; ++py)
			{
				for (int px = pxStartX; px < pxEndX; ++px)
				{
					// Flip Y for stbi-loaded image
					int imageY = (dims.y - 1) - py;
					Rgba8 pixel = m_roadMapImage->GetTexelColor(IntVec2(px, imageY));
					CellType pixelType = ClassifyRoadMapPixel(pixel);

					if (pixelType == CellType::WATER)      hasWater = true;
					if (pixelType == CellType::ROAD)       hasRoad = true;
					if (pixelType == CellType::SETTLEMENT) hasSettlement = true;
				}
			}

			// Priority: water > road > settlement > terrain
			CellType type = CellType::TERRAIN;
			if (hasSettlement) type = CellType::SETTLEMENT;
			if (hasRoad)       type = CellType::ROAD;
			if (hasWater)      type = CellType::WATER;

			m_cellTypeGrid[row * m_gridWidth + col] = type;

			if (type == CellType::WATER)      ++waterCount;
			if (type == CellType::SETTLEMENT) ++settlementCount;
			if (type == CellType::ROAD)       ++roadCount;
		}
	}

	// Dilate road cells by 1 cell to account for players running off-track
	std::vector<bool> isRoad(totalCells, false);
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (m_cellTypeGrid[idx] == CellType::ROAD)
			isRoad[idx] = true;
	}
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (isRoad[idx]) continue;
		if (m_cellTypeGrid[idx] == CellType::WATER) continue; // don't overwrite water

		int row = idx / m_gridWidth;
		int col = idx % m_gridWidth;
		bool nearRoad = false;
		if (row > 0              && isRoad[idx - m_gridWidth]) nearRoad = true;
		if (row < m_gridHeight-1 && isRoad[idx + m_gridWidth]) nearRoad = true;
		if (col > 0              && isRoad[idx - 1])           nearRoad = true;
		if (col < m_gridWidth-1  && isRoad[idx + 1])           nearRoad = true;

		if (nearRoad)
		{
			m_cellTypeGrid[idx] = CellType::ROAD;
			++roadCount;
		}
	}

	g_theDevConsole->AddLine(Stringf("Cell types: %d water, %d settlement, %d road (dilated), %d terrain",
		waterCount, settlementCount, roadCount, totalCells - waterCount - settlementCount - roadCount), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Pass 1: Fills the height grid using per-cell monster height averages.
// Cells with no monsters get -1 (missing).
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildHeightGrid()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_heightGrid.resize(totalCells, -1.f);

	std::vector<float> cellSumHeight(totalCells, 0.f);
	m_actualCountGrid.resize(totalCells, 0);

	int colX = m_monsterPoints.GetColumnIndex("world_x");
	int colY = m_monsterPoints.GetColumnIndex("world_y");
	int colH = m_monsterPoints.GetColumnIndex("height");

	for (int i = 0; i < m_monsterPoints.GetRowCount(); ++i)
	{
		Strings const& row = m_monsterPoints.m_rows[i];
		float wx = ParseCsvValue(row, colX, 0.f);
		float wy = ParseCsvValue(row, colY, 0.f);
		float h  = ParseCsvValue(row, colH, 0.f);

		int col = (int)(wx / m_cellWorldSize);
		int r   = (int)(wy / m_cellWorldSize);
		if (col < 0) col = 0;
		if (r < 0)   r = 0;
		if (col >= m_gridWidth)  col = m_gridWidth - 1;
		if (r >= m_gridHeight)   r = m_gridHeight - 1;

		int idx = r * m_gridWidth + col;
		cellSumHeight[idx] += h;
		m_actualCountGrid[idx] += 1;
	}

	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (m_actualCountGrid[idx] > 0)
			m_heightGrid[idx] = cellSumHeight[idx] / (float)m_actualCountGrid[idx];
	}

	g_theDevConsole->AddLine(Stringf("Pass 1 (monster heights): %d missing", CountMissingCells()), Rgba8::YELLOW);
}

//-----------------------------------------------------------------------------------------------
// Pass 2: Fills all empty cells that aren't ROAD or WATER using the zone average height.
// This covers terrain and settlement cells. The result is blocky (one value per zone),
// which gets smoothed in pass 5.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::FillTerrainHeights()
{
	int filled = 0;
	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			int idx = row * m_gridWidth + col;
			if (m_heightGrid[idx] >= 0.f) continue; // already has data

			CellType type = m_cellTypeGrid[idx];
			if (type == CellType::ROAD || type == CellType::WATER) continue;

			// Sample zone at cell center and use zone average height
			float worldX = ((float)col + 0.5f) * m_cellWorldSize;
			float worldY = ((float)row + 0.5f) * m_cellWorldSize;
			Rgba8 pixelColor = SampleZoneImageAtWorldPos(worldX, worldY);
			int zoneId = FindNearestZoneId(pixelColor);

			if (zoneId >= 0 && m_zoneAvgHeight.find(zoneId) != m_zoneAvgHeight.end())
			{
				m_heightGrid[idx] = m_zoneAvgHeight[zoneId];
				++filled;
			}
		}
	}

	g_theDevConsole->AddLine(Stringf("Pass 2 (terrain zone avg): filled %d, %d missing", filled, CountMissingCells()), Rgba8::YELLOW);
}

//-----------------------------------------------------------------------------------------------
// Pass 3: Fills road cells via Laplacian relaxation.
// Roads cross zone boundaries, so using zone average would create abrupt height jumps.
// Laplacian relaxation smoothly interpolates between terrain on either side of the road.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::FillRoadHeights()
{
	std::vector<int> roadCells;
	for (int idx = 0; idx < m_gridWidth * m_gridHeight; ++idx)
	{
		if (m_cellTypeGrid[idx] == CellType::ROAD && m_heightGrid[idx] < 0.f)
			roadCells.push_back(idx);
	}

	if (roadCells.empty())
	{
		g_theDevConsole->AddLine("Pass 3 (road): no road cells to fill", Rgba8::GREEN);
		return;
	}

	int const maxIterations = 300;
	for (int iter = 0; iter < maxIterations; ++iter)
	{
		int changed = 0;
		for (int idx : roadCells)
		{
			int row = idx / m_gridWidth;
			int col = idx % m_gridWidth;

			float sum = 0.f;
			int count = 0;

			if (row > 0              && m_heightGrid[idx - m_gridWidth] >= 0.f) { sum += m_heightGrid[idx - m_gridWidth]; ++count; }
			if (row < m_gridHeight-1 && m_heightGrid[idx + m_gridWidth] >= 0.f) { sum += m_heightGrid[idx + m_gridWidth]; ++count; }
			if (col > 0              && m_heightGrid[idx - 1]           >= 0.f) { sum += m_heightGrid[idx - 1];           ++count; }
			if (col < m_gridWidth-1  && m_heightGrid[idx + 1]           >= 0.f) { sum += m_heightGrid[idx + 1];           ++count; }

			if (count > 0)
			{
				float newVal = sum / (float)count;
				if (m_heightGrid[idx] < 0.f || fabsf(m_heightGrid[idx] - newVal) > 0.01f)
					++changed;
				m_heightGrid[idx] = newVal;
			}
		}

		if (changed == 0)
		{
			g_theDevConsole->AddLine(Stringf("Pass 3 (road): converged in %d iterations", iter + 1), Rgba8::GREEN);
			break;
		}
	}

	int stillMissing = 0;
	for (int idx : roadCells)
	{
		if (m_heightGrid[idx] < 0.f) ++stillMissing;
	}
	g_theDevConsole->AddLine(Stringf("Pass 3 (road): %d/%d filled, %d still missing",
		(int)roadCells.size() - stillMissing, (int)roadCells.size(), stillMissing),
		stillMissing > 0 ? Rgba8::YELLOW : Rgba8::GREEN);
}

//-----------------------------------------------------------------------------------------------
// Pass 4: Fills water cells via Laplacian relaxation.
// Same approach as settlement fill — iteratively averages from neighbors.
// Since terrain and settlement cells are already filled, water boundaries have known heights.
// The iterative averaging naturally produces per-branch adaptive gradients (short branch = steep,
// long branch = gentle), handling junctions smoothly without explicit skeleton extraction.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::FillWaterHeights()
{
	// Collect indices of all water cells that need filling
	std::vector<int> waterCells;
	for (int idx = 0; idx < m_gridWidth * m_gridHeight; ++idx)
	{
		if (m_cellTypeGrid[idx] == CellType::WATER && m_heightGrid[idx] < 0.f)
			waterCells.push_back(idx);
	}

	if (waterCells.empty())
	{
		g_theDevConsole->AddLine("Pass 4 (water): no water cells to fill", Rgba8::GREEN);
		return;
	}

	// Iterative relaxation: each water cell = average of valid neighbors
	int const maxIterations = 500;
	for (int iter = 0; iter < maxIterations; ++iter)
	{
		int changed = 0;
		for (int idx : waterCells)
		{
			int row = idx / m_gridWidth;
			int col = idx % m_gridWidth;

			float sum = 0.f;
			int count = 0;

			if (row > 0              && m_heightGrid[idx - m_gridWidth] >= 0.f) { sum += m_heightGrid[idx - m_gridWidth]; ++count; }
			if (row < m_gridHeight-1 && m_heightGrid[idx + m_gridWidth] >= 0.f) { sum += m_heightGrid[idx + m_gridWidth]; ++count; }
			if (col > 0              && m_heightGrid[idx - 1]           >= 0.f) { sum += m_heightGrid[idx - 1];           ++count; }
			if (col < m_gridWidth-1  && m_heightGrid[idx + 1]           >= 0.f) { sum += m_heightGrid[idx + 1];           ++count; }

			if (count > 0)
			{
				float newVal = sum / (float)count;
				if (m_heightGrid[idx] < 0.f || fabsf(m_heightGrid[idx] - newVal) > 0.01f)
					++changed;
				m_heightGrid[idx] = newVal;
			}
		}

		if (changed == 0)
		{
			g_theDevConsole->AddLine(Stringf("Pass 4 (water): converged in %d iterations", iter + 1), Rgba8::GREEN);
			break;
		}
	}

	// Count how many water cells are still missing after relaxation
	int stillMissingWater = 0;
	for (int idx : waterCells)
	{
		if (m_heightGrid[idx] < 0.f) ++stillMissingWater;
	}
	int totalRemaining = CountMissingCells();
	g_theDevConsole->AddLine(Stringf("Pass 4 (water): %d/%d filled, %d water still missing, %d total missing",
		(int)waterCells.size() - stillMissingWater, (int)waterCells.size(), stillMissingWater, totalRemaining),
		totalRemaining > 0 ? Rgba8::YELLOW : Rgba8::GREEN);
}

//-----------------------------------------------------------------------------------------------
// Pass 5: Smooths the entire height grid except road and water cells.
// Uses Laplacian relaxation to blend the blocky zone-average boundaries into gradients.
// Road and water cells keep their values from passes 3-4 (hard edges preserved).
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::SmoothTerrainHeights()
{
	// Find cells at zone edges where height jumps sharply to a neighbor.
	// Each cell is smoothed exactly once — replaced with the average of its 4 neighbors.
	// No expansion, no iteration — a single pass to soften hard zone boundary edges.
	float const heightJumpThreshold = 20.f;
	int smoothed = 0;

	// Take a snapshot so we read original values, not partially-smoothed ones
	std::vector<float> original = m_heightGrid;

	for (int row = 1; row < m_gridHeight - 1; ++row)
	{
		for (int col = 1; col < m_gridWidth - 1; ++col)
		{
			int idx = row * m_gridWidth + col;

			CellType type = m_cellTypeGrid[idx];
			if (type == CellType::ROAD || type == CellType::WATER) continue;

			float h = original[idx];
			if (h < 0.f) continue;

			// Check if any neighbor has a sharp height difference
			float hUp    = original[idx - m_gridWidth];
			float hDown  = original[idx + m_gridWidth];
			float hLeft  = original[idx - 1];
			float hRight = original[idx + 1];

			bool hasJump = false;
			if (hUp    >= 0.f && fabsf(h - hUp)    > heightJumpThreshold) hasJump = true;
			if (hDown  >= 0.f && fabsf(h - hDown)  > heightJumpThreshold) hasJump = true;
			if (hLeft  >= 0.f && fabsf(h - hLeft)  > heightJumpThreshold) hasJump = true;
			if (hRight >= 0.f && fabsf(h - hRight) > heightJumpThreshold) hasJump = true;

			if (!hasJump) continue;

			// Replace with average of valid neighbors (read from snapshot)
			float sum = 0.f;
			int count = 0;
			if (hUp    >= 0.f) { sum += hUp;    ++count; }
			if (hDown  >= 0.f) { sum += hDown;  ++count; }
			if (hLeft  >= 0.f) { sum += hLeft;  ++count; }
			if (hRight >= 0.f) { sum += hRight; ++count; }

			if (count > 0)
			{
				m_heightGrid[idx] = sum / (float)count;
				++smoothed;
			}
		}
	}

	g_theDevConsole->AddLine(Stringf("Pass 5 (smooth edges): %d cells smoothed once (threshold=%.0fm)",
		smoothed, heightJumpThreshold), Rgba8::GREEN);
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
// Builds the threat level grid: per-cell average monster threat_level from monster_reference.csv.
// Empty cells are filled via Laplacian relaxation to create a continuous threat field.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildThreatGrid()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_threatGrid.resize(totalCells, -1.f);

	// Build monster_type -> threat_level lookup from monster_reference.csv
	int colType   = m_monsterReference.GetColumnIndex("monster_type");
	int colThreat = m_monsterReference.GetColumnIndex("threat_level");
	GUARANTEE_OR_DIE(colType >= 0 && colThreat >= 0, "monster_reference.csv missing monster_type or threat_level");

	std::map<std::string, float> threatLookup;
	for (int i = 0; i < m_monsterReference.GetRowCount(); ++i)
	{
		Strings const& row = m_monsterReference.m_rows[i];
		std::string mtype = ParseCsvValue(row, colType, std::string(""));
		float threat      = ParseCsvValue(row, colThreat, 1.f);
		threatLookup[mtype] = threat;
	}

	// Accumulate threat per cell from monster positions
	std::vector<float> cellSumThreat(totalCells, 0.f);
	int colX    = m_monsterPoints.GetColumnIndex("world_x");
	int colY    = m_monsterPoints.GetColumnIndex("world_y");
	int colMT   = m_monsterPoints.GetColumnIndex("monster_type");

	for (int i = 0; i < m_monsterPoints.GetRowCount(); ++i)
	{
		Strings const& row = m_monsterPoints.m_rows[i];
		float wx  = ParseCsvValue(row, colX, 0.f);
		float wy  = ParseCsvValue(row, colY, 0.f);
		std::string mtype = ParseCsvValue(row, colMT, std::string(""));

		int col = (int)(wx / m_cellWorldSize);
		int r   = (int)(wy / m_cellWorldSize);
		if (col < 0) col = 0;
		if (r < 0)   r = 0;
		if (col >= m_gridWidth)  col = m_gridWidth - 1;
		if (r >= m_gridHeight)   r = m_gridHeight - 1;

		int idx = r * m_gridWidth + col;
		float threat = 1.f;
		auto it = threatLookup.find(mtype);
		if (it != threatLookup.end())
			threat = it->second;

		cellSumThreat[idx] += threat;
	}

	// Compute average threat for cells with monsters
	int filledCells = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (m_actualCountGrid[idx] > 0)
		{
			m_threatGrid[idx] = cellSumThreat[idx] / (float)m_actualCountGrid[idx];
			++filledCells;
		}
	}

	// For empty cells, accumulate threat contributions from ALL monster cells within range.
	// Each monster cell contributes: threat * (1 - distance / falloffRange).
	// A cell surrounded by many monster cells gets higher total threat than one with a single neighbor.
	float const threatFalloffRange = 500.f;
	int const falloffCellRadius = (int)(threatFalloffRange / m_cellWorldSize);

	// Collect all monster cell positions and their threat values
	struct MonsterCellInfo { int col; int row; float threat; };
	std::vector<MonsterCellInfo> monsterCells;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (m_actualCountGrid[idx] > 0)
		{
			MonsterCellInfo info;
			info.col = idx % m_gridWidth;
			info.row = idx / m_gridWidth;
			info.threat = m_threatGrid[idx];
			monsterCells.push_back(info);
		}
	}

	// For each empty cell, sum distance-attenuated threat from all monster cells in range
	int filledCellCount = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (m_threatGrid[idx] >= 0.f)
			continue; // has direct monster data

		int cellCol = idx % m_gridWidth;
		int cellRow = idx / m_gridWidth;

		float totalThreat = 0.f;
		float totalWeight = 0.f;
		float maxWeight   = 0.f;

		for (MonsterCellInfo const& mc : monsterCells)
		{
			// Quick bounding box reject
			if (abs(mc.col - cellCol) > falloffCellRadius || abs(mc.row - cellRow) > falloffCellRadius)
				continue;

			float dx = (float)(cellCol - mc.col) * m_cellWorldSize;
			float dy = (float)(cellRow - mc.row) * m_cellWorldSize;
			float dist = sqrtf(dx * dx + dy * dy);

			if (dist > threatFalloffRange)
				continue;

			float weight = 1.f - (dist / threatFalloffRange);
			totalThreat += mc.threat * weight;
			totalWeight += weight;
			if (weight > maxWeight) maxWeight = weight;
		}

		if (totalWeight > 0.f)
		{
			// avgThreat = what threat level is nearby
			// maxWeight = how close the nearest monster is
			// depth = totalWeight normalized: how many layers of monsters surround this cell.
			//   Edge cell with few nearby monsters = low depth.
			//   Deep inside dense cluster = high depth.
			//   Normalizer: ~20 nearby cells at medium weight ≈ fully embedded.
			float avgThreat = totalThreat / totalWeight;
			float depth = totalWeight / 20.f;
			if (depth > 1.f) depth = 1.f;

			m_threatGrid[idx] = avgThreat * maxWeight * depth;
			++filledCellCount;
		}
		else
		{
			m_threatGrid[idx] = 0.f; // beyond range of any monster
		}
	}

	g_theDevConsole->AddLine(Stringf("ThreatGrid: %d cells with monsters, %d filled by IDW (%.0fm range)",
		filledCells, filledCellCount, threatFalloffRange), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Builds the diversity grid: Shannon entropy of monster types per cell.
// H = -sum(p_i * log2(p_i)) where p_i = count of type i / total count in cell.
// Cells with 0-1 monsters get -1 (no meaningful diversity). No fill for empty cells.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildDiversityGrid()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_diversityGrid.resize(totalCells, -1.f);

	// Build per-cell type histograms from monster positions
	int colX  = m_monsterPoints.GetColumnIndex("world_x");
	int colY  = m_monsterPoints.GetColumnIndex("world_y");
	int colMT = m_monsterPoints.GetColumnIndex("monster_type");

	// Map each cell to a type->count histogram
	std::vector<std::map<std::string, int>> cellTypeCount(totalCells);

	for (int i = 0; i < m_monsterPoints.GetRowCount(); ++i)
	{
		Strings const& row = m_monsterPoints.m_rows[i];
		float wx = ParseCsvValue(row, colX, 0.f);
		float wy = ParseCsvValue(row, colY, 0.f);
		std::string mtype = ParseCsvValue(row, colMT, std::string(""));

		int col = (int)(wx / m_cellWorldSize);
		int r   = (int)(wy / m_cellWorldSize);
		if (col < 0) col = 0;
		if (r < 0)   r = 0;
		if (col >= m_gridWidth)  col = m_gridWidth - 1;
		if (r >= m_gridHeight)   r = m_gridHeight - 1;

		cellTypeCount[r * m_gridWidth + col][mtype] += 1;
	}

	// Compute Shannon entropy per cell
	int cellsWithDiversity = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		int total = m_actualCountGrid[idx];
		if (total < 2) continue; // need at least 2 monsters for meaningful diversity

		float entropy = 0.f;
		for (auto& [type, count] : cellTypeCount[idx])
		{
			float p = (float)count / (float)total;
			if (p > 0.f)
				entropy -= p * log2f(p);
		}

		m_diversityGrid[idx] = entropy;
		++cellsWithDiversity;
	}

	g_theDevConsole->AddLine(Stringf("DiversityGrid: %d/%d cells with monster data", cellsWithDiversity, totalCells), Rgba8::WHITE);

	// Fill empty cells via IDW from cells with diversity data, within 500m range.
	// Needed for final scoring — empty cells can't be 0 if surrounded by diverse areas.
	float const diversityFalloffRange = 500.f;
	int const falloffCellRadius = (int)(diversityFalloffRange / m_cellWorldSize);

	struct DiversityCellInfo { int col; int row; float entropy; };
	std::vector<DiversityCellInfo> diversityCells;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (m_diversityGrid[idx] >= 0.f)
		{
			DiversityCellInfo info;
			info.col = idx % m_gridWidth;
			info.row = idx / m_gridWidth;
			info.entropy = m_diversityGrid[idx];
			diversityCells.push_back(info);
		}
	}

	int filledByIDW = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (m_diversityGrid[idx] >= 0.f) continue;

		int cellCol = idx % m_gridWidth;
		int cellRow = idx / m_gridWidth;

		float totalVal = 0.f;
		float totalWeight = 0.f;
		float maxWeight = 0.f;

		for (DiversityCellInfo const& dc : diversityCells)
		{
			if (abs(dc.col - cellCol) > falloffCellRadius || abs(dc.row - cellRow) > falloffCellRadius)
				continue;

			float dx = (float)(cellCol - dc.col) * m_cellWorldSize;
			float dy = (float)(cellRow - dc.row) * m_cellWorldSize;
			float dist = sqrtf(dx * dx + dy * dy);
			if (dist > diversityFalloffRange) continue;

			float weight = 1.f - (dist / diversityFalloffRange);
			totalVal += dc.entropy * weight;
			totalWeight += weight;
			if (weight > maxWeight) maxWeight = weight;
		}

		if (totalWeight > 0.f)
		{
			float avgDiv = totalVal / totalWeight;
			float depth = totalWeight / 20.f;
			if (depth > 1.f) depth = 1.f;
			m_diversityGrid[idx] = avgDiv * maxWeight * depth;
			++filledByIDW;
		}
		else
		{
			m_diversityGrid[idx] = 0.f;
		}
	}

	g_theDevConsole->AddLine(Stringf("DiversityGrid: %d filled by IDW (%.0fm range)",
		filledByIDW, diversityFalloffRange), Rgba8::WHITE);
}

// Builds road distance field via multi-source BFS from all road-classified cells.
// Each cell gets the distance in meters to the nearest road cell.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildRoadDistanceField()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_roadDistGrid.resize(totalCells, -1.f);

	std::queue<int> queue;

	// Seed BFS with all road cells at distance 0
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (m_cellTypeGrid[idx] == CellType::ROAD)
		{
			m_roadDistGrid[idx] = 0.f;
			queue.push(idx);
		}
	}

	int roadCellCount = (int)queue.size();

	// BFS flood fill: each step = 1 cell = m_cellWorldSize meters
	while (!queue.empty())
	{
		int idx = queue.front();
		queue.pop();

		int row = idx / m_gridWidth;
		int col = idx % m_gridWidth;
		float nextDist = m_roadDistGrid[idx] + m_cellWorldSize;

		int neighbors[4] = {
			(row > 0)              ? idx - m_gridWidth : -1,
			(row < m_gridHeight-1) ? idx + m_gridWidth : -1,
			(col > 0)              ? idx - 1           : -1,
			(col < m_gridWidth-1)  ? idx + 1           : -1
		};

		for (int n : neighbors)
		{
			if (n >= 0 && m_roadDistGrid[n] < 0.f)
			{
				m_roadDistGrid[n] = nextDist;
				queue.push(n);
			}
		}
	}

	g_theDevConsole->AddLine(Stringf("Road distance field: %d road cells", roadCellCount), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Builds settlement distance field using Euclidean distance from each cell center to the
// nearest settlement position in settlements.csv.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::ComputeSettlementDistances()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_nearestSettlementDist.resize(totalCells, 99999.f);

	// Parse settlement positions
	int colX = m_settlements.GetColumnIndex("world_x");
	int colY = m_settlements.GetColumnIndex("world_y");
	GUARANTEE_OR_DIE(colX >= 0 && colY >= 0, "settlements.csv missing world_x or world_y column");

	struct SettlementPos { float x; float y; };
	std::vector<SettlementPos> positions;
	for (int i = 0; i < m_settlements.GetRowCount(); ++i)
	{
		Strings const& row = m_settlements.m_rows[i];
		SettlementPos pos;
		pos.x = ParseCsvValue(row, colX, 0.f);
		pos.y = ParseCsvValue(row, colY, 0.f);
		positions.push_back(pos);
	}

	// For each cell, find distance to nearest settlement
	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			float cellX = ((float)col + 0.5f) * m_cellWorldSize;
			float cellY = ((float)row + 0.5f) * m_cellWorldSize;

			float minDist = 99999.f;
			for (SettlementPos const& s : positions)
			{
				float dx = cellX - s.x;
				float dy = cellY - s.y;
				float dist = sqrtf(dx * dx + dy * dy);
				if (dist < minDist) minDist = dist;
			}

			m_nearestSettlementDist[row * m_gridWidth + col] = minDist;
		}
	}

	g_theDevConsole->AddLine(Stringf("Settlement distance field: %d settlements", (int)positions.size()), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Returns the zone base density per spawn_design_notes.md section 1.1.
// Wilderness(1-7)=1.0, Functional(11-23)=1.75, Settlement core(33,35)=0.2, Perimeter(34,36)=0.5
//-----------------------------------------------------------------------------------------------
float HeatmapAssessmentMode::GetZoneBaseDensity(int zoneId) const
{
	if (zoneId >= 1  && zoneId <= 7)  return 1.0f;   // wilderness
	if (zoneId >= 11 && zoneId <= 23) return 1.75f;   // functional (midpoint of 1.5-2.0)
	if (zoneId == 33 || zoneId == 35) return 0.2f;    // settlement core (midpoint of 0.1-0.3)
	if (zoneId == 34 || zoneId == 36) return 0.5f;    // settlement perimeter (midpoint of 0.4-0.6)
	return 1.0f; // fallback
}

//-----------------------------------------------------------------------------------------------
// Returns road distance modifier per spawn_design_notes.md section 1.2.
//-----------------------------------------------------------------------------------------------
float HeatmapAssessmentMode::GetRoadModifier(float distMeters) const
{
	if (distMeters > 500.f)  return 1.0f;
	if (distMeters > 200.f)  return 0.85f;
	if (distMeters > 50.f)   return 0.70f;
	return 0.55f;
}

//-----------------------------------------------------------------------------------------------
// Returns settlement distance modifier per spawn_design_notes.md section 1.4.
//-----------------------------------------------------------------------------------------------
float HeatmapAssessmentMode::GetSettlementModifier(float distMeters) const
{
	if (distMeters > 1000.f) return 1.0f;
	if (distMeters > 500.f)  return 0.8f;
	if (distMeters > 150.f)  return 0.5f;
	return 0.2f;
}

//-----------------------------------------------------------------------------------------------
// Builds the expected density grid by applying the full design formula per cell:
// expected = zone_base_density × road_modifier × elevation_modifier × settlement_modifier
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildExpectedDensityGrid()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_expectedDensityGrid.resize(totalCells, 0.f);
	m_cellZoneIdGrid.resize(totalCells, -1);

	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			int idx = row * m_gridWidth + col;

			// Get zone ID for this cell
			float worldX = ((float)col + 0.5f) * m_cellWorldSize;
			float worldY = ((float)row + 0.5f) * m_cellWorldSize;
			Rgba8 pixelColor = SampleZoneImageAtWorldPos(worldX, worldY);
			int zoneId = FindNearestZoneId(pixelColor);
			m_cellZoneIdGrid[idx] = zoneId;

			// Four modifiers from the design formula
			float zoneDensity    = GetZoneBaseDensity(zoneId);
			float roadMod        = GetRoadModifier(m_roadDistGrid[idx]);
			float elevMod        = GetElevationModifier(m_heightGrid[idx] >= 0.f ? m_heightGrid[idx] : 100.f);
			float settlementMod  = GetSettlementModifier(m_nearestSettlementDist[idx]);

			m_expectedDensityGrid[idx] = zoneDensity * roadMod * elevMod * settlementMod;
		}
	}

	// Find range for DevConsole reporting
	float minD = 99999.f, maxD = -99999.f;
	for (float d : m_expectedDensityGrid)
	{
		if (d < minD) minD = d;
		if (d > maxD) maxD = d;
	}
	g_theDevConsole->AddLine(Stringf("Expected density: range [%.3f, %.3f]", minD, maxD), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Returns how many cells in the height grid are still missing (value < 0).
//-----------------------------------------------------------------------------------------------
int HeatmapAssessmentMode::CountMissingCells() const
{
	int count = 0;
	for (float h : m_heightGrid)
	{
		if (h < 0.f) ++count;
	}
	return count;
}

//-----------------------------------------------------------------------------------------------
// Builds the background vertex buffer: a single textured quad for zone_heatmap.png.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildBackgroundVerts()
{
	m_backgroundVerts.clear();
	AABB2 mapBounds(Vec2(m_mapOffsetX, 0.f), Vec2(m_mapOffsetX + (float)m_gridWidth, (float)m_gridHeight));
	AddVertsUVForAABB2D(m_backgroundVerts, mapBounds, Rgba8::WHITE, AABB2::ZERO_TO_ONE);
}

//-----------------------------------------------------------------------------------------------
// Builds the heatmap overlay vertex buffer for the current heatmap type.
// ZONE_REFERENCE = no overlay. HEIGHT = grayscale grid, magenta for missing.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildHeatmapVerts()
{
	m_heatmapVerts.clear();

	if (m_currentHeatmap == HeatmapType::ZONE_REFERENCE)
		return;

	if (m_currentHeatmap == HeatmapType::HEIGHT)
	{
		// Find height range for normalization, skipping missing cells
		float minH = 99999.f;
		float maxH = -99999.f;
		for (float h : m_heightGrid)
		{
			if (h < 0.f) continue;
			if (h < minH) minH = h;
			if (h > maxH) maxH = h;
		}

		// Build colored quads: black=low, white=high, magenta=missing
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float h = m_heightGrid[row * m_gridWidth + col];
				Rgba8 color = (h < 0.f) ? Rgba8::MAGENTA : HeightToGrayscale(h, minH, maxH);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::ACTUAL_DENSITY)
	{
		// Find max monster count for normalization
		int maxCount = 0;
		for (int c : m_actualCountGrid)
		{
			if (c > maxCount) maxCount = c;
		}
		if (maxCount < 1) maxCount = 1;

		// Log scale normalization so the gradient is visible across all density levels.
		// Linear would compress most cells to near-black due to outlier cells with high counts.
		float logMax = logf((float)maxCount + 1.f);

		// Build colored quads: black=no monsters, white=highest density
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				int count = m_actualCountGrid[row * m_gridWidth + col];
				float t = logf((float)count + 1.f) / logMax;
				Rgba8 color = InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 255, 255, 255), t);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::THREAT_LEVEL)
	{
		// Threat ranges from 1 (M01) to 5 (M09). Normalize to [0,1] over that range.
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float threat = m_threatGrid[row * m_gridWidth + col];
				float t = threat / 5.f; // map [0,5] -> [0,1]
				if (t < 0.f) t = 0.f;
				if (t > 1.f) t = 1.f;
				Rgba8 color = InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 255, 255, 255), t);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::DIVERSITY)
	{
		// Max possible entropy = log2(9) ≈ 3.17 for 9 monster types
		float maxEntropy = log2f(9.f);

		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float d = m_diversityGrid[row * m_gridWidth + col];
				float t = (d > 0.f) ? (d / maxEntropy) : 0.f;
				if (t > 1.f) t = 1.f;

				Rgba8 color = InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 255, 255, 255), t);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::ROAD_DISTANCE)
	{
		// Visualize road influence range per design doc:
		// <50m = full white (on/near road), 50-500m = gradient falloff, >500m = black (no effect)
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float d = m_roadDistGrid[row * m_gridWidth + col];

				float t = 0.f; // default black (>500m)
				if (d < 50.f)
					t = 1.f; // full white: on or near road
				else if (d < 500.f)
					t = 1.f - (d - 50.f) / (500.f - 50.f); // linear falloff from white to black

				Rgba8 color = InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 255, 255, 255), t);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::EXPECTED_DENSITY)
	{
		// Find density range for normalization
		float minD = 99999.f;
		float maxD = -99999.f;
		for (float d : m_expectedDensityGrid)
		{
			if (d < minD) minD = d;
			if (d > maxD) maxD = d;
		}

		// Build colored quads: black=low density, white=high density
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float d = m_expectedDensityGrid[row * m_gridWidth + col];
				float range = maxD - minD;
				if (range < 0.001f) range = 0.001f;
				float t = (d - minD) / range;
				if (t < 0.f) t = 0.f;
				if (t > 1.f) t = 1.f;
				Rgba8 color = InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 255, 255, 255), t);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------
// Returns the display name for the current heatmap type.
//-----------------------------------------------------------------------------------------------
char const* HeatmapAssessmentMode::GetHeatmapName() const
{
	switch (m_currentHeatmap)
	{
	case HeatmapType::ZONE_REFERENCE:   return "Zone Reference";
	case HeatmapType::EXPECTED_DENSITY: return "ExpectedDensity";
	case HeatmapType::ACTUAL_DENSITY:   return "ActualDensity";
	case HeatmapType::THREAT_LEVEL:     return "ThreatLevel";
	case HeatmapType::DIVERSITY:        return "Diversity";
	case HeatmapType::HEIGHT:           return "HeightMap";
	case HeatmapType::ROAD_DISTANCE:    return "RoadDistance";
	default:                            return "Unknown";
	}
}

//-----------------------------------------------------------------------------------------------
// Returns legend entries (color swatch + label) for the current heatmap.
//-----------------------------------------------------------------------------------------------
std::vector<HeatmapAssessmentMode::LegendEntry> HeatmapAssessmentMode::GetLegendEntries() const
{
	std::vector<LegendEntry> entries;

	switch (m_currentHeatmap)
	{
	case HeatmapType::ZONE_REFERENCE:
		entries.push_back({Rgba8(190, 210, 100, 255), "Wilderness (zone 1-7)"});
		entries.push_back({Rgba8(200, 120,  80, 255), "Functional (zone 11-23)"});
		entries.push_back({Rgba8(100, 100, 200, 255), "Settlement (zone 33-36)"});
		break;
	case HeatmapType::HEIGHT:
		entries.push_back({Rgba8(  0,   0,   0, 255), "0-50m (modifier 1.0)"});
		entries.push_back({Rgba8( 75,  75,  75, 255), "51-200m (modifier 0.9)"});
		entries.push_back({Rgba8(170, 170, 170, 255), "201-500m (modifier 0.7)"});
		entries.push_back({Rgba8(255, 255, 255, 255), ">500m (modifier 0.4)"});
		entries.push_back({Rgba8::MAGENTA,             "No data"});
		break;
	case HeatmapType::ROAD_DISTANCE:
		entries.push_back({Rgba8(255, 255, 255, 255), "On road (0m)"});
		entries.push_back({Rgba8(170, 170, 170, 255), "<50m (mod 0.55)"});
		entries.push_back({Rgba8(115, 115, 115, 255), "50-200m (mod 0.70)"});
		entries.push_back({Rgba8( 60,  60,  60, 255), "200-500m (mod 0.85)"});
		entries.push_back({Rgba8(  0,   0,   0, 255), ">500m (mod 1.0)"});
		break;
	case HeatmapType::EXPECTED_DENSITY:
		entries.push_back({Rgba8(255, 255, 255, 255), "High expected density"});
		entries.push_back({Rgba8(128, 128, 128, 255), "Medium expected density"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Low expected density"});
		break;
	case HeatmapType::ACTUAL_DENSITY:
		entries.push_back({Rgba8(255, 255, 255, 255), "High monster count"});
		entries.push_back({Rgba8(128, 128, 128, 255), "Medium monster count"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "No monsters"});
		break;
	case HeatmapType::THREAT_LEVEL:
		entries.push_back({Rgba8(255, 255, 255, 255), "Threat 5 (M09 Boss)"});
		entries.push_back({Rgba8(190, 190, 190, 255), "Threat 4 (M05/M06 Elite)"});
		entries.push_back({Rgba8(128, 128, 128, 255), "Threat 3 (M02/M03 Special)"});
		entries.push_back({Rgba8( 64,  64,  64, 255), "Threat 2 (M04 Runner)"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Threat 1 (M01 Wanderer)"});
		break;
	case HeatmapType::DIVERSITY:
		entries.push_back({Rgba8(255, 255, 255, 255), "High diversity (many types)"});
		entries.push_back({Rgba8(128, 128, 128, 255), "Medium diversity"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Low / no monsters"});
		break;
	default:
		break;
	}

	return entries;
}

//-----------------------------------------------------------------------------------------------
// Renders the legend panel at the left-bottom corner with colored squares + text labels.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::RenderLegend() const
{
	std::vector<LegendEntry> entries = GetLegendEntries();
	if (entries.empty())
		return;

	// Use screen camera coordinates, matching the font size from RenderScreenMessage
	AABB2 screenBounds = m_screenCamera.GetCameraBounds();
	float screenHeight = screenBounds.m_maxs.y - screenBounds.m_mins.y;
	float textLineHeight = screenHeight / m_gameModeConfig.m_numMessageOnScreen;
	float fontHeight = m_gameModeConfig.m_lineHeightAndTextBoxRatio * textLineHeight;

	float const padding    = 10.f;
	float const squareSize = fontHeight * 1.2f;
	float const lineHeight = textLineHeight;
	float const textLeft   = padding + squareSize + 8.f;
	float const textRight  = screenBounds.m_maxs.x * 0.3f;  // legend uses left 30% of screen

	std::vector<Vertex_PCU> legendVerts;
	std::vector<Vertex_PCU> textVerts;

	// Title above the entries
	float titleY = padding + (float)entries.size() * lineHeight;
	AABB2 titleBox(Vec2(padding, titleY), Vec2(textRight, titleY + lineHeight));
	m_gameModeConfig.m_font->AddVertsForTextInBox2D(
		textVerts, GetHeatmapName(), titleBox, fontHeight * 1.2f,
		Vec2(0.f, 0.5f), Rgba8::WHITE, 0.7f,
		m_gameModeConfig.m_cellAspect, TextDrawMode::OVERRUN
	);

	// Legend entries stacked from bottom up
	for (int i = 0; i < (int)entries.size(); ++i)
	{
		LegendEntry const& entry = entries[i];
		float y = padding + (float)i * lineHeight;

		AABB2 squareBounds(Vec2(padding, y), Vec2(padding + squareSize, y + squareSize));
		AddVertsForAABB2D(legendVerts, squareBounds, entry.color);

		AABB2 labelBox(Vec2(textLeft, y), Vec2(textRight, y + squareSize));
		m_gameModeConfig.m_font->AddVertsForTextInBox2D(
			textVerts, entry.label, labelBox, fontHeight,
			Vec2(0.f, 0.5f), Rgba8::WHITE, 0.6f,
			m_gameModeConfig.m_cellAspect, TextDrawMode::OVERRUN
		);
	}

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray((int)legendVerts.size(), legendVerts.data());

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(&m_gameModeConfig.m_font->GetTexture());
	g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
}

//-----------------------------------------------------------------------------------------------
// Handles N/M key input to cycle through heatmap types.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	bool changed = false;
	int current = (int)m_currentHeatmap;
	int count   = (int)HeatmapType::COUNT;

	if (g_theInput->WasKeyJustPressed('N'))
	{
		current = (current + 1) % count;
		changed = true;
	}
	if (g_theInput->WasKeyJustPressed('M'))
	{
		current = (current - 1 + count) % count;
		changed = true;
	}

	if (changed)
	{
		m_currentHeatmap = (HeatmapType)current;
		BuildHeatmapVerts();
		UpdateModeInfo();
	}
}

//-----------------------------------------------------------------------------------------------
// Renders background image, heatmap overlay, legend, and UI text.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();

	// Background zone image
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->BindTexture(m_zoneHeatmapTexture);
	g_theRenderer->DrawVertexArray((int)m_backgroundVerts.size(), m_backgroundVerts.data());

	// Heatmap overlay
	if (!m_heatmapVerts.empty())
	{
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray((int)m_heatmapVerts.size(), m_heatmapVerts.data());
	}

	g_theRenderer->EndCamera(m_worldCamera);

	// Screen layer: UI text + legend (both in screen camera coordinates)
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderScreenMessage();
	RenderLegend();
	g_theRenderer->EndCamera(m_screenCamera);
}

//-----------------------------------------------------------------------------------------------
// Cleans up CPU-side Images (caller owns pointers from CreateImageFromFile).
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::Shutdown()
{
	if (m_zoneHeatmapImage) { delete m_zoneHeatmapImage; m_zoneHeatmapImage = nullptr; }
	if (m_roadMapImage)     { delete m_roadMapImage;     m_roadMapImage = nullptr; }
}

//-----------------------------------------------------------------------------------------------
// Updates the on-screen status text.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::UpdateModeInfo()
{
	m_modeName = "Heatmap Assessment Mode";
	m_controlInstruction = "N/M: Switch Heatmap | F6/F7: Switch Mode";
	m_testString = Stringf("[%d/%d] %s  |  Monsters: %d  |  Zones: %d  |  Settlements: %d",
		(int)m_currentHeatmap + 1, (int)HeatmapType::COUNT, GetHeatmapName(),
		m_monsterPoints.GetRowCount(),
		m_zoneReference.GetRowCount(),
		m_settlements.GetRowCount());
}
