#include "Game/HeatmapAssessmentMode.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"
#include "Engine/core/DevConsole.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/core/FileUtils.hpp"
#include "Engine/core/HeatMaps.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/core/Image.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <set>
#include <queue>
#include <algorithm>
#include <cstring>
#include <cmath>

extern Renderer*    g_theRenderer;
extern InputSystem* g_theInput;

//-----------------------------------------------------------------------------------------------
static HeatmapType const* GetVisibleHeatmapOrder(int& outCount)
{
	static HeatmapType const order[] = {
		HeatmapType::ZONE_REFERENCE,
		HeatmapType::HEIGHT,
		HeatmapType::EXPECTED_DENSITY,
		HeatmapType::ACTUAL_DENSITY,
		HeatmapType::DENSITY_DEVIATION,
		HeatmapType::ROAD_DISTANCE,
		HeatmapType::SETTLEMENT_DISTANCE,
		HeatmapType::ACCESSIBILITY,
		HeatmapType::ENCOUNTER_RISK,
		HeatmapType::MONSTER_TYPE_COUNT,
		HeatmapType::DIVERSITY,
		HeatmapType::DIVERSITY_ACCESSIBILITY_DEVIATION,
		HeatmapType::THREAT_LEVEL,
		HeatmapType::THREAT_ALIGNMENT_DEVIATION,
		HeatmapType::FINAL_SCORE,
		HeatmapType::OPTIMIZED_PROBLEM_SCORE,
	};

	outCount = (int)(sizeof(order) / sizeof(order[0]));
	return order;
}

//-----------------------------------------------------------------------------------------------
static int GetVisibleHeatmapIndex(HeatmapType type)
{
	int count = 0;
	HeatmapType const* order = GetVisibleHeatmapOrder(count);
	for (int i = 0; i < count; ++i)
	{
		if (order[i] == type)
			return i;
	}
	return 0;
}

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
static float Clamp01(float value)
{
	if (value < 0.f) return 0.f;
	if (value > 1.f) return 1.f;
	return value;
}

//-----------------------------------------------------------------------------------------------
static float GetPercentileFromSortedValues(std::vector<float> const& sortedValues, float percentileZeroToOne)
{
	if (sortedValues.empty())
	{
		return 0.f;
	}

	float t = Clamp01(percentileZeroToOne);
	int index = (int)((float)(sortedValues.size() - 1) * t);
	return sortedValues[index];
}

//-----------------------------------------------------------------------------------------------
static Rgba8 SignedDeviationToColor(float signedDeviation)
{
	if (signedDeviation > 0.f)
	{
		return InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 0, 0, 255), Clamp01(signedDeviation));
	}
	else if (signedDeviation < 0.f)
	{
		return InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(0, 80, 255, 255), Clamp01(-signedDeviation));
	}

	return Rgba8(0, 0, 0, 255);
}

static Rgba8 const DENSITY_DEVIATION_NOT_SCORED_COLOR = Rgba8(0, 0, 0, 255);

//-----------------------------------------------------------------------------------------------
// Classifies a road_map.png pixel color into a CellType by checking which channel dominates.
//-----------------------------------------------------------------------------------------------
static CellType ClassifyRoadMapPixel(Rgba8 const& color)
{
	// road_map.png has anti-aliased edges: intended road/water/settlement pixels can have
	// values like (0,0,1), not only full-intensity (0,0,255). Use dominant channel instead
	// of a high threshold so thin waterways are not accidentally treated as terrain.
	if (color.b > 0 && color.b > color.r && color.b > color.g)
		return CellType::WATER;
	if (color.g > 0 && color.g > color.r && color.g > color.b)
		return CellType::SETTLEMENT;
	if (color.r > 0 && color.r > color.g && color.r > color.b)
		return CellType::ROAD;
	return CellType::TERRAIN;
}

//-----------------------------------------------------------------------------------------------
static char const* GetCellTypeName(CellType type)
{
	switch (type)
	{
	case CellType::TERRAIN:    return "terrain";
	case CellType::WATER:      return "water";
	case CellType::SETTLEMENT: return "settlement";
	case CellType::ROAD:       return "road";
	default:                   return "unknown";
	}
}

//-----------------------------------------------------------------------------------------------
static bool ShouldScoreDensityDeviationCell(CellType type)
{
	// Water comes from road_map.png, not zone_reference.csv. It is not a spawn surface, so it
	// should not receive expected monster density or affect land-density percentile thresholds.
	// Roads stay scored: spawn_design_notes.md gives roads a density modifier of 0.55 near the
	// road, not 0.0, so road corridors should still be evaluated against a lower expectation.
	return type != CellType::WATER;
}

//-----------------------------------------------------------------------------------------------
static float GetExpectedThreatFromZoneLabel(std::string const& baseThreatLevel)
{
	// zone_reference.csv defines zone design threat as 低/中/高. Use center values rather
	// than 1/3/5 so low zones can still contain weak monsters and high zones are not too loose.
	if (baseThreatLevel == "低") return 1.5f;
	if (baseThreatLevel == "中") return 2.5f;
	if (baseThreatLevel == "高") return 3.5f;
	return 2.5f;
}

//-----------------------------------------------------------------------------------------------
static int CountTextLines(std::string const& text)
{
	int lines = 1;
	for (char c : text)
	{
		if (c == '\n') ++lines;
	}
	return lines;
}

//-----------------------------------------------------------------------------------------------
static std::string WrapTextToWidth(BitmapFont* font, std::string const& text, float maxWidth, float cellHeight, float cellAspect)
{
	if (font == nullptr || text.empty())
	{
		return text;
	}

	std::string wrapped;
	std::string line;
	std::string word;

	auto appendWord = [&]()
	{
		if (word.empty()) return;

		std::string candidate = line.empty() ? word : line + " " + word;
		if (font->GetTextWidth(cellHeight, candidate, cellAspect) <= maxWidth)
		{
			line = candidate;
			word.clear();
			return;
		}

		if (!line.empty())
		{
			if (!wrapped.empty()) wrapped += "\n";
			wrapped += line;
			line.clear();
		}

		for (char c : word)
		{
			std::string charCandidate = line + c;
			if (!line.empty() && font->GetTextWidth(cellHeight, charCandidate, cellAspect) > maxWidth)
			{
				if (!wrapped.empty()) wrapped += "\n";
				wrapped += line;
				line.clear();
			}
			line.push_back(c);
		}
		word.clear();
	};

	for (char c : text)
	{
		if (c == ' ' || c == '\n')
		{
			appendWord();
			if (c == '\n')
			{
				if (!wrapped.empty()) wrapped += "\n";
				wrapped += line;
				line.clear();
			}
		}
		else
		{
			word.push_back(c);
		}
	}

	appendWord();
	if (!line.empty())
	{
		if (!wrapped.empty()) wrapped += "\n";
		wrapped += line;
	}
	return wrapped;
}

//-----------------------------------------------------------------------------------------------
static TileHeatMap MakeTileHeatMapFromFloatGrid(IntVec2 const& dimensions, std::vector<float> const& values)
{
	TileHeatMap heatMap(dimensions);
	heatMap.m_values = values;
	return heatMap;
}

//-----------------------------------------------------------------------------------------------
static TileHeatMap MakeTileHeatMapFromIntGrid(IntVec2 const& dimensions, std::vector<int> const& values)
{
	TileHeatMap heatMap(dimensions);
	heatMap.m_values.resize(values.size());
	for (int i = 0; i < (int)values.size(); ++i)
	{
		heatMap.m_values[i] = (float)values[i];
	}
	return heatMap;
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
	BuildAccessibilityGrid();
	BuildExpectedDensityGrid();
	BuildDensityDeviationGrid();

	// Threat level grid
	BuildThreatGrid();

	// Diversity grid
	BuildDiversityGrid();

	// Score maps
	BuildEncounterRiskGrid();
	BuildDiversityAccessibilityDeviationGrid();
	BuildThreatAlignmentDeviationGrid();
	BuildFinalScoreGrid();
	BuildOptimizedDistributionGrids();
	ExportAllHeatmapCsvImageData();

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
	int colThreat   = m_zoneReference.GetColumnIndex("base_threat_level");
	GUARANTEE_OR_DIE(colId >= 0 && colRgb >= 0 && colCategory >= 0 && colThreat >= 0,
		"zone_reference.csv missing zone_id, map_color_rgb, zone_category, or base_threat_level column");

	for (int i = 0; i < m_zoneReference.GetRowCount(); ++i)
	{
		Strings const& row = m_zoneReference.m_rows[i];
		int zoneId = ParseCsvValue(row, colId, -1);
		std::string rgbStr   = ParseCsvValue(row, colRgb, std::string(""));
		std::string category = ParseCsvValue(row, colCategory, std::string(""));
		std::string threatLevel = ParseCsvValue(row, colThreat, std::string("中"));

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
		m_zoneExpectedThreat[zoneId] = GetExpectedThreatFromZoneLabel(threatLevel);
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
// Builds the diversity grid using Shannon entropy:
//   H = -sum(p_i * log2(p_i))
// where p_i is the probability of monster type i within the sampled distribution.
//
// m_rawDiversityGrid stores diagnostic entropy inside each exact 20m cell.
// m_diversityGrid stores displayed local encounter diversity by merging nearby type counts
// with Gaussian falloff. Recomputing entropy from weighted counts preserves type balance;
// averaging neighbor entropy values would hide whether one monster type dominates.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildDiversityGrid()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_rawDiversityGrid.assign(totalCells, -1.f);
	m_diversityGrid.assign(totalCells, 0.f);
	m_diversityConfidenceGrid.assign(totalCells, 0.f);

	// Build per-cell type histograms from monster positions
	int colX  = m_monsterPoints.GetColumnIndex("world_x");
	int colY  = m_monsterPoints.GetColumnIndex("world_y");
	int colMT = m_monsterPoints.GetColumnIndex("monster_type");

	// Map each cell to a type->count histogram
	std::vector<std::map<std::string, int>> cellTypeCount(totalCells);
	m_monsterTypeCountGrid.resize(totalCells, 0);

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

	for (int idx = 0; idx < totalCells; ++idx)
	{
		m_monsterTypeCountGrid[idx] = (int)cellTypeCount[idx].size();
	}

	// Compute raw Shannon entropy per cell for diagnostics.
	int rawEntropyCells = 0;
	int rawMonoTypeCells = 0;
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

		m_rawDiversityGrid[idx] = entropy;
		++rawEntropyCells;
		if (entropy <= 0.f) ++rawMonoTypeCells;
	}

	// A diversity cell represents local encounter variety: strongest within about 80m,
	// fading smoothly through a 240m sampling tail. Confidence fades instead of snapping
	// low-sample cells to black.
	float const diversitySigma = 80.f;
	float const diversitySampleRadius = 240.f;
	float const confidenceFullSample = 3.f;
	int const falloffCellRadius = (int)(diversitySampleRadius / m_cellWorldSize);

	int confidentLocalCells = 0;
	int lowConfidenceLocalCells = 0;
	int emptyCells = 0;
	std::vector<float> positiveLocalDiversityValues;
	positiveLocalDiversityValues.reserve(totalCells);
	for (int cellRow = 0; cellRow < m_gridHeight; ++cellRow)
	{
		for (int cellCol = 0; cellCol < m_gridWidth; ++cellCol)
		{
			std::map<std::string, float> weightedTypeCount;
			float totalWeightedCount = 0.f;

			int minRow = cellRow - falloffCellRadius;
			int maxRow = cellRow + falloffCellRadius;
			int minCol = cellCol - falloffCellRadius;
			int maxCol = cellCol + falloffCellRadius;
			if (minRow < 0) minRow = 0;
			if (minCol < 0) minCol = 0;
			if (maxRow >= m_gridHeight) maxRow = m_gridHeight - 1;
			if (maxCol >= m_gridWidth)  maxCol = m_gridWidth - 1;

			for (int sampleRow = minRow; sampleRow <= maxRow; ++sampleRow)
			{
				for (int sampleCol = minCol; sampleCol <= maxCol; ++sampleCol)
				{
					int sampleIdx = sampleRow * m_gridWidth + sampleCol;
					if (m_actualCountGrid[sampleIdx] <= 0) continue;

					float dx = (float)(sampleCol - cellCol) * m_cellWorldSize;
					float dy = (float)(sampleRow - cellRow) * m_cellWorldSize;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist > diversitySampleRadius) continue;

					float sigmaRatio = dist / diversitySigma;
					float weight = expf(-0.5f * sigmaRatio * sigmaRatio);
					for (auto& [type, count] : cellTypeCount[sampleIdx])
					{
						float weightedCount = (float)count * weight;
						weightedTypeCount[type] += weightedCount;
						totalWeightedCount += weightedCount;
					}
				}
			}

			int idx = cellRow * m_gridWidth + cellCol;
			if (totalWeightedCount <= 0.f)
			{
				m_diversityGrid[idx] = 0.f;
				++emptyCells;
				continue;
			}

			float entropy = 0.f;
			for (auto& [type, count] : weightedTypeCount)
			{
				float p = count / totalWeightedCount;
				if (p > 0.f)
					entropy -= p * log2f(p);
			}

			float confidence = totalWeightedCount / confidenceFullSample;
			if (confidence > 1.f) confidence = 1.f;
			m_diversityConfidenceGrid[idx] = confidence;
			m_diversityGrid[idx] = entropy * confidence;
			if (m_diversityGrid[idx] > 0.f)
			{
				positiveLocalDiversityValues.push_back(m_diversityGrid[idx]);
			}

			if (confidence >= 1.f)
				++confidentLocalCells;
			else
				++lowConfidenceLocalCells;
		}
	}

	float p10LocalDiversity = 0.f;
	float p95LocalDiversity = 0.f;
	float maxLocalDiversity = 0.f;
	if (!positiveLocalDiversityValues.empty())
	{
		std::sort(positiveLocalDiversityValues.begin(), positiveLocalDiversityValues.end());
		p10LocalDiversity = GetPercentileFromSortedValues(positiveLocalDiversityValues, 0.10f);
		p95LocalDiversity = GetPercentileFromSortedValues(positiveLocalDiversityValues, 0.95f);
		maxLocalDiversity = positiveLocalDiversityValues.back();
	}

	g_theDevConsole->AddLine(Stringf("DiversityGrid raw: %d entropy cells, %d mono-type cells", rawEntropyCells, rawMonoTypeCells), Rgba8::WHITE);
	g_theDevConsole->AddLine(Stringf("DiversityGrid local: %d confident, %d low-confidence, %d empty (sigma %.0fm, sample %.0fm)",
		confidentLocalCells, lowConfidenceLocalCells, emptyCells, diversitySigma, diversitySampleRadius), Rgba8::WHITE);
	g_theDevConsole->AddLine(Stringf("DiversityGrid local range: p10 %.3f, p95 %.3f, max %.3f",
		p10LocalDiversity, p95LocalDiversity, maxLocalDiversity), Rgba8::WHITE);
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
// Context justification for remote/harder content. Functional zones can justify more risk;
// settlement zones justify less because they should be safer and easier to read.
//-----------------------------------------------------------------------------------------------
float HeatmapAssessmentMode::GetZoneRiskJustification(int zoneId) const
{
	if (zoneId >= 11 && zoneId <= 23) return 0.75f; // functional / high-risk spaces
	if (zoneId >= 1  && zoneId <= 7)  return 0.40f; // wilderness can support moderate exploration depth
	if (zoneId == 34 || zoneId == 36) return 0.18f; // settlement perimeter
	if (zoneId == 33 || zoneId == 35) return 0.05f; // settlement core
	return 0.35f;
}

//-----------------------------------------------------------------------------------------------
// Builds an accessibility state map: white = easier player access, black = remote.
// This is not the spawn density modifier. It represents traversal/reachability from roads and
// settlements, so either nearby roads or nearby settlements can make a cell accessible.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildAccessibilityGrid()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_accessibilityGrid.resize(totalCells, 0.f);

	float minA = 99999.f;
	float maxA = -99999.f;

	for (int idx = 0; idx < totalCells; ++idx)
	{
		float roadDist = m_roadDistGrid[idx];
		float roadAccess = 0.f;
		if (roadDist <= 50.f)
		{
			roadAccess = 1.f;
		}
		else if (roadDist < 500.f)
		{
			roadAccess = 1.f - ((roadDist - 50.f) / (500.f - 50.f));
		}

		float settlementDist = m_nearestSettlementDist[idx];
		float settlementAccess = 0.f;
		if (settlementDist <= 150.f)
		{
			settlementAccess = 1.f;
		}
		else if (settlementDist < 1000.f)
		{
			settlementAccess = 1.f - ((settlementDist - 150.f) / (1000.f - 150.f));
		}

		float accessibility = 1.f - (1.f - roadAccess) * (1.f - settlementAccess);
		if (accessibility < 0.f) accessibility = 0.f;
		if (accessibility > 1.f) accessibility = 1.f;

		m_accessibilityGrid[idx] = accessibility;
		if (accessibility < minA) minA = accessibility;
		if (accessibility > maxA) maxA = accessibility;
	}

	g_theDevConsole->AddLine(Stringf("Accessibility: range [%.3f, %.3f] (road <=500m, settlement <=1000m)",
		minA, maxA), Rgba8::WHITE);
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

			if (!ShouldScoreDensityDeviationCell(m_cellTypeGrid[idx]))
			{
				m_expectedDensityGrid[idx] = 0.f;
				continue;
			}

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
// Builds a signed density deviation score map.
// Blue = local monster share is below the relative design expectation; red = above expectation.
// ExpectedDensity is a relative allocation weight, so compare shares instead of treating it as a direct count.
// Compare local neighborhoods instead of exact 20m cells; point counts are too sparse per cell.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildDensityDeviationGrid()
{
	BuildDensityDeviationGridForCounts(
		m_actualCountGrid,
		m_densityDeviationGrid,
		"DensityDeviation",
		"Data/Debug/HeatmapAssessment/density_deviation_debug.csv");
}

//-----------------------------------------------------------------------------------------------
// Shared density-deviation builder used by both the original and optimized distributions.
// The countGrid parameter lets the optimizer reuse the exact same evaluation logic after it has
// redistributed monsters, which keeps the before/after comparison honest.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildDensityDeviationGridForCounts(std::vector<int> const& countGrid, std::vector<float>& outDeviationGrid, char const* debugLabel, char const* debugCsvPath)
{
	int totalCells = m_gridWidth * m_gridHeight;
	outDeviationGrid.assign(totalCells, 0.f);
	bool const shouldWriteDebugCsv = debugCsvPath != nullptr && debugCsvPath[0] != '\0';

	float totalExpectedWeight = 0.f;
	int totalActualCount = 0;
	int ignoredCells = 0;
	int ignoredMonsterCount = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (!ShouldScoreDensityDeviationCell(m_cellTypeGrid[idx]))
		{
			++ignoredCells;
			ignoredMonsterCount += countGrid[idx];
			continue;
		}

		totalExpectedWeight += m_expectedDensityGrid[idx];
		totalActualCount += countGrid[idx];
	}

	float const densityRadius = 100.f;
	float const densityRadiusSq = densityRadius * densityRadius;
	int const densityCellRadius = (int)ceilf(densityRadius / m_cellWorldSize);
	float const deadZonePercentile = 0.60f;
	float const strongDeviationPercentile = 0.95f;
	float localMonsterDeadZone = 0.f;

	std::vector<float> localActualValues(totalCells, 0.f);
	std::vector<float> localExpectedWeightValues(totalCells, 0.f);
	std::vector<float> shareDeltaValues(totalCells, 0.f);
	std::vector<float> equivalentMonsterDeltaValues(totalCells, 0.f);
	std::vector<float> absEquivalentMonsterDeltas;
	absEquivalentMonsterDeltas.reserve(totalCells);

	int tooFewCells = 0;
	int tooManyCells = 0;
	int nearExpectedCells = 0;
	float maxUnder = 0.f;
	float maxOver = 0.f;
	int bucketUnderStrong = 0;
	int bucketUnderMedium = 0;
	int bucketUnderWeak = 0;
	int bucketNear = 0;
	int bucketOverWeak = 0;
	int bucketOverMedium = 0;
	int bucketOverStrong = 0;

	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			int idx = row * m_gridWidth + col;
			if (!ShouldScoreDensityDeviationCell(m_cellTypeGrid[idx]))
			{
				continue;
			}

			float localActual = 0.f;
			float localExpectedWeight = 0.f;

			int minRow = row - densityCellRadius;
			int maxRow = row + densityCellRadius;
			int minCol = col - densityCellRadius;
			int maxCol = col + densityCellRadius;
			if (minRow < 0) minRow = 0;
			if (minCol < 0) minCol = 0;
			if (maxRow >= m_gridHeight) maxRow = m_gridHeight - 1;
			if (maxCol >= m_gridWidth)  maxCol = m_gridWidth - 1;

			for (int sampleRow = minRow; sampleRow <= maxRow; ++sampleRow)
			{
				for (int sampleCol = minCol; sampleCol <= maxCol; ++sampleCol)
				{
					float dx = (float)(sampleCol - col) * m_cellWorldSize;
					float dy = (float)(sampleRow - row) * m_cellWorldSize;
					float distSq = dx * dx + dy * dy;
					if (distSq > densityRadiusSq) continue;

					int sampleIdx = sampleRow * m_gridWidth + sampleCol;
					if (!ShouldScoreDensityDeviationCell(m_cellTypeGrid[sampleIdx]))
					{
						continue;
					}

					localActual += (float)countGrid[sampleIdx];
					localExpectedWeight += m_expectedDensityGrid[sampleIdx];
				}
			}

			localActualValues[idx] = localActual;
			localExpectedWeightValues[idx] = localExpectedWeight;

			if (totalActualCount > 0 && totalExpectedWeight > 0.f)
			{
				// ExpectedDensity is a weight field. Convert the local actual and expected
				// neighborhoods into global shares before comparing them.
				float actualShare = localActual / (float)totalActualCount;
				float expectedShare = localExpectedWeight / totalExpectedWeight;
				float shareDelta = actualShare - expectedShare;
				// Express share error in "equivalent monsters" so percentile thresholds are
				// interpretable and stable across maps with different total monster counts.
				float equivalentMonsterDelta = shareDelta * (float)totalActualCount;

				shareDeltaValues[idx] = shareDelta;
				equivalentMonsterDeltaValues[idx] = equivalentMonsterDelta;
				absEquivalentMonsterDeltas.push_back(fabsf(equivalentMonsterDelta));
			}
		}
	}

	float p60AbsEquivalentDelta = 0.f;
	float p95AbsEquivalentDelta = 1.f;
	if (!absEquivalentMonsterDeltas.empty())
	{
		std::sort(absEquivalentMonsterDeltas.begin(), absEquivalentMonsterDeltas.end());
		p60AbsEquivalentDelta = GetPercentileFromSortedValues(absEquivalentMonsterDeltas, deadZonePercentile);
		p95AbsEquivalentDelta = GetPercentileFromSortedValues(absEquivalentMonsterDeltas, strongDeviationPercentile);
		// Treat the lower 60% of local mismatches as noise/acceptable variation. The top 5%
		// becomes full color intensity, making the map read as a priority map instead of static.
		localMonsterDeadZone = p60AbsEquivalentDelta;
		if (p95AbsEquivalentDelta < localMonsterDeadZone + 0.001f)
		{
			p95AbsEquivalentDelta = localMonsterDeadZone + 0.001f;
		}
	}

	std::string debugCsv;
	if (shouldWriteDebugCsv)
	{
		debugCsv.reserve((size_t)totalCells * 96);
		debugCsv += "col,row,cell_type,is_scored,actual_count,expected_weight,local_actual,local_expected_weight,actual_share,expected_share,share_delta,equivalent_monster_delta,normalized_deviation\n";
	}

	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			int idx = row * m_gridWidth + col;
			bool isScored = ShouldScoreDensityDeviationCell(m_cellTypeGrid[idx]);
			float localActual = localActualValues[idx];
			float localExpectedWeight = localExpectedWeightValues[idx];
			float actualShare = totalActualCount > 0 ? localActual / (float)totalActualCount : 0.f;
			float expectedShare = totalExpectedWeight > 0.f ? localExpectedWeight / totalExpectedWeight : 0.f;
			float shareDelta = shareDeltaValues[idx];
			float equivalentMonsterDelta = equivalentMonsterDeltaValues[idx];

			float signedDeviation = 0.f;
			float magnitude = fabsf(equivalentMonsterDelta);
			if (isScored && magnitude > localMonsterDeadZone)
			{
				float normalizedMagnitude = (magnitude - localMonsterDeadZone) / (p95AbsEquivalentDelta - localMonsterDeadZone);
				if (normalizedMagnitude > 1.f) normalizedMagnitude = 1.f;
				signedDeviation = equivalentMonsterDelta < 0.f ? -normalizedMagnitude : normalizedMagnitude;
			}

			outDeviationGrid[idx] = signedDeviation;

			if (shouldWriteDebugCsv)
			{
				debugCsv += Stringf("%d,%d,%s,%d,%d,%.6f,%.6f,%.6f,%.9f,%.9f,%.9f,%.6f,%.6f\n",
					col, row,
					GetCellTypeName(m_cellTypeGrid[idx]),
					isScored ? 1 : 0,
					countGrid[idx],
					m_expectedDensityGrid[idx],
					localActual,
					localExpectedWeight,
					actualShare,
					expectedShare,
					shareDelta,
					equivalentMonsterDelta,
					signedDeviation);
			}

			if (!isScored)
			{
				continue;
			}

			float deviationMagnitude = fabsf(signedDeviation);
			if (deviationMagnitude < 0.1f)
			{
				++nearExpectedCells;
				++bucketNear;
			}
			else if (signedDeviation < 0.f)
			{
				++tooFewCells;
				if (deviationMagnitude > maxUnder) maxUnder = deviationMagnitude;
				if (deviationMagnitude < 0.33f) ++bucketUnderWeak;
				else if (deviationMagnitude < 0.66f) ++bucketUnderMedium;
				else ++bucketUnderStrong;
			}
			else if (signedDeviation > 0.f)
			{
				++tooManyCells;
				if (signedDeviation > maxOver) maxOver = signedDeviation;
				if (deviationMagnitude < 0.33f) ++bucketOverWeak;
				else if (deviationMagnitude < 0.66f) ++bucketOverMedium;
				else ++bucketOverStrong;
			}
		}
	}

	g_theDevConsole->AddLine(Stringf("%s: %d near, %d too few, %d too many, max under %.2f, max over %.2f (%.0fm local radius)",
		debugLabel, nearExpectedCells, tooFewCells, tooManyCells, maxUnder, maxOver, densityRadius), Rgba8::WHITE);
	g_theDevConsole->AddLine(Stringf("%s share mode: scored actual %d, expected weight %.1f, ignored water cells %d, ignored monsters %d",
		debugLabel, totalActualCount, totalExpectedWeight, ignoredCells, ignoredMonsterCount), Rgba8::WHITE);
	g_theDevConsole->AddLine(Stringf("%s thresholds: p%.0f dead zone %.2f, p%.0f full color %.2f",
		debugLabel, deadZonePercentile * 100.f, localMonsterDeadZone, strongDeviationPercentile * 100.f, p95AbsEquivalentDelta), Rgba8::WHITE);
	g_theDevConsole->AddLine(Stringf("%s buckets: blue weak/med/strong %d/%d/%d, near %d, red weak/med/strong %d/%d/%d",
		debugLabel, bucketUnderWeak, bucketUnderMedium, bucketUnderStrong, bucketNear,
		bucketOverWeak, bucketOverMedium, bucketOverStrong), Rgba8::WHITE);
	if (shouldWriteDebugCsv)
	{
		if (FileWriteFromString(debugCsv, debugCsvPath))
		{
			g_theDevConsole->AddLine(Stringf("%s debug CSV: %s", debugLabel, debugCsvPath), Rgba8::WHITE);
		}
		else
		{
			g_theDevConsole->AddLine(Stringf("%s debug CSV write failed: %s", debugLabel, debugCsvPath), Rgba8::YELLOW);
		}
	}
}

//-----------------------------------------------------------------------------------------------
// Builds an encounter risk map from player accessibility and local monster density.
// This intentionally ignores monster threat: threat severity is evaluated separately by
// ThreatAlignmentDeviation against zone_reference.csv base_threat_level.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildEncounterRiskGrid()
{
	BuildEncounterRiskGridForCounts(m_actualCountGrid, m_encounterRiskGrid, "EncounterRisk");
}

//-----------------------------------------------------------------------------------------------
// Shared encounter-risk builder.
// Risk intentionally stays "accessibility x local monster pressure"; optimized threat is scored
// separately, so this map answers only "will the player meet monsters here often?"
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildEncounterRiskGridForCounts(std::vector<int> const& countGrid, std::vector<float>& outRiskGrid, char const* debugLabel)
{
	int totalCells = m_gridWidth * m_gridHeight;
	outRiskGrid.assign(totalCells, 0.f);

	float const localRadius = 100.f;
	int const localCellRadius = (int)(localRadius / m_cellWorldSize);

	std::vector<float> localActualValues(totalCells, 0.f);
	std::vector<float> occupiedLocalActualValues;
	occupiedLocalActualValues.reserve(totalCells);

	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			float localActual = 0.f;

			int minRow = row - localCellRadius;
			int maxRow = row + localCellRadius;
			int minCol = col - localCellRadius;
			int maxCol = col + localCellRadius;
			if (minRow < 0) minRow = 0;
			if (minCol < 0) minCol = 0;
			if (maxRow >= m_gridHeight) maxRow = m_gridHeight - 1;
			if (maxCol >= m_gridWidth)  maxCol = m_gridWidth - 1;

			for (int sampleRow = minRow; sampleRow <= maxRow; ++sampleRow)
			{
				for (int sampleCol = minCol; sampleCol <= maxCol; ++sampleCol)
				{
					float dx = (float)(sampleCol - col) * m_cellWorldSize;
					float dy = (float)(sampleRow - row) * m_cellWorldSize;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist > localRadius) continue;

					int sampleIdx = sampleRow * m_gridWidth + sampleCol;
					int monsterCount = countGrid[sampleIdx];
					if (monsterCount <= 0) continue;

					localActual += (float)monsterCount;
				}
			}

			int idx = row * m_gridWidth + col;
			localActualValues[idx] = localActual;
			if (localActual > 0.f)
			{
				occupiedLocalActualValues.push_back(localActual);
			}
		}
	}

	float p95LocalActual = 1.f;
	if (!occupiedLocalActualValues.empty())
	{
		std::sort(occupiedLocalActualValues.begin(), occupiedLocalActualValues.end());
		p95LocalActual = GetPercentileFromSortedValues(occupiedLocalActualValues, 0.95f);
		if (p95LocalActual < 1.f) p95LocalActual = 1.f;
	}

	std::vector<float> rawRisks(totalCells, 0.f);
	std::vector<float> positiveRawRisks;
	positiveRawRisks.reserve(totalCells);

	for (int idx = 0; idx < totalCells; ++idx)
	{
		float localActual = localActualValues[idx];
		if (localActual <= 0.f)
		{
			continue;
		}

		float access = Clamp01(m_accessibilityGrid[idx]);
		float densityPressure = Clamp01(localActual / p95LocalActual);
		float rawRisk = access * densityPressure;
		rawRisks[idx] = rawRisk;
		if (rawRisk > 0.f)
		{
			positiveRawRisks.push_back(rawRisk);
		}
	}

	float deadZone = 0.f;
	float strongRisk = 1.f;
	if (!positiveRawRisks.empty())
	{
		std::sort(positiveRawRisks.begin(), positiveRawRisks.end());
		deadZone = GetPercentileFromSortedValues(positiveRawRisks, 0.60f);
		strongRisk = GetPercentileFromSortedValues(positiveRawRisks, 0.95f);
		if (strongRisk < deadZone + 0.001f) strongRisk = deadZone + 0.001f;
	}

	int emptyCells = 0;
	int lowRiskCells = 0;
	int highRiskCells = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (localActualValues[idx] <= 0.f)
		{
			++emptyCells;
			continue;
		}

		float rawRisk = rawRisks[idx];
		if (rawRisk <= deadZone)
		{
			++lowRiskCells;
			continue;
		}

		float normalizedRisk = (rawRisk - deadZone) / (strongRisk - deadZone);
		outRiskGrid[idx] = Clamp01(normalizedRisk);
		++highRiskCells;
	}

	g_theDevConsole->AddLine(Stringf("%s: %d high-risk, %d low-risk, %d no local monsters (%.0fm radius, p60 %.3f, p95 %.3f)",
		debugLabel, highRiskCells, lowRiskCells, emptyCells, localRadius, deadZone, strongRisk), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Scores cells whose local monster diversity is too low.
// The design goal here is simple: repeated encounters should not be dominated by one type.
// Accessibility is intentionally not used; this is a pure encounter-variety problem map.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildDiversityAccessibilityDeviationGrid()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_diversityAccessibilityDeviationGrid.assign(totalCells, 0.f);

	float const evidenceRadius = 160.f;
	int const evidenceCellRadius = (int)(evidenceRadius / m_cellWorldSize);

	std::vector<float> localActualValues(totalCells, 0.f);
	std::vector<float> evidenceDiversityValues;
	evidenceDiversityValues.reserve(totalCells);

	int evidenceCells = 0;
	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			float localActual = 0.f;

			int minRow = row - evidenceCellRadius;
			int maxRow = row + evidenceCellRadius;
			int minCol = col - evidenceCellRadius;
			int maxCol = col + evidenceCellRadius;
			if (minRow < 0) minRow = 0;
			if (minCol < 0) minCol = 0;
			if (maxRow >= m_gridHeight) maxRow = m_gridHeight - 1;
			if (maxCol >= m_gridWidth)  maxCol = m_gridWidth - 1;

			for (int sampleRow = minRow; sampleRow <= maxRow; ++sampleRow)
			{
				for (int sampleCol = minCol; sampleCol <= maxCol; ++sampleCol)
				{
					float dx = (float)(sampleCol - col) * m_cellWorldSize;
					float dy = (float)(sampleRow - row) * m_cellWorldSize;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist > evidenceRadius) continue;

					localActual += (float)m_actualCountGrid[sampleRow * m_gridWidth + sampleCol];
				}
			}

			int idx = row * m_gridWidth + col;
			localActualValues[idx] = localActual;
			if (localActual <= 0.f)
			{
				continue;
			}

			evidenceDiversityValues.push_back(m_diversityGrid[idx]);
			++evidenceCells;
		}
	}

	float lowDiversityTarget = 0.f;
	float strongLowDiversity = 0.f;
	if (!evidenceDiversityValues.empty())
	{
		std::sort(evidenceDiversityValues.begin(), evidenceDiversityValues.end());
		strongLowDiversity = GetPercentileFromSortedValues(evidenceDiversityValues, 0.10f);
		lowDiversityTarget = GetPercentileFromSortedValues(evidenceDiversityValues, 0.40f);
		if (lowDiversityTarget < strongLowDiversity + 0.001f)
		{
			lowDiversityTarget = strongLowDiversity + 0.001f;
		}
	}

	int lowDiversityCells = 0;
	int acceptedCells = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (localActualValues[idx] <= 0.f)
		{
			++acceptedCells;
			continue;
		}

		float diversity = m_diversityGrid[idx];
		if (diversity >= lowDiversityTarget)
		{
			++acceptedCells;
			continue;
		}

		float score = (lowDiversityTarget - diversity) / (lowDiversityTarget - strongLowDiversity);
		m_diversityAccessibilityDeviationGrid[idx] = Clamp01(score);
		++lowDiversityCells;
	}

	g_theDevConsole->AddLine(Stringf("LowDiversityDeviation: %d low-diversity, %d accepted/no evidence, %d evidence cells (%.0fm evidence, p10 %.3f, p40 %.3f)",
		lowDiversityCells, acceptedCells, evidenceCells, evidenceRadius, strongLowDiversity, lowDiversityTarget), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Shared low-diversity evaluator for generated type layouts.
// This rebuilds the local Shannon entropy field from optimized per-cell type histograms, then
// applies the same "only low diversity is a problem" rule used by the current distribution.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildLowDiversityDeviationGridForTypeCounts(std::vector<int> const& countGrid, std::vector<std::map<std::string, int>> const& typeCountGrid, std::vector<float>& outDeviationGrid, char const* debugLabel)
{
	int totalCells = m_gridWidth * m_gridHeight;
	outDeviationGrid.assign(totalCells, 0.f);

	std::vector<float> localDiversityGrid(totalCells, 0.f);

	float const diversitySigma = 80.f;
	float const diversitySampleRadius = 240.f;
	float const confidenceFullSample = 3.f;
	int const falloffCellRadius = (int)(diversitySampleRadius / m_cellWorldSize);

	for (int cellRow = 0; cellRow < m_gridHeight; ++cellRow)
	{
		for (int cellCol = 0; cellCol < m_gridWidth; ++cellCol)
		{
			std::map<std::string, float> weightedTypeCount;
			float totalWeightedCount = 0.f;

			int minRow = cellRow - falloffCellRadius;
			int maxRow = cellRow + falloffCellRadius;
			int minCol = cellCol - falloffCellRadius;
			int maxCol = cellCol + falloffCellRadius;
			if (minRow < 0) minRow = 0;
			if (minCol < 0) minCol = 0;
			if (maxRow >= m_gridHeight) maxRow = m_gridHeight - 1;
			if (maxCol >= m_gridWidth)  maxCol = m_gridWidth - 1;

			for (int sampleRow = minRow; sampleRow <= maxRow; ++sampleRow)
			{
				for (int sampleCol = minCol; sampleCol <= maxCol; ++sampleCol)
				{
					int sampleIdx = sampleRow * m_gridWidth + sampleCol;
					if (countGrid[sampleIdx] <= 0) continue;

					float dx = (float)(sampleCol - cellCol) * m_cellWorldSize;
					float dy = (float)(sampleRow - cellRow) * m_cellWorldSize;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist > diversitySampleRadius) continue;

					float sigmaRatio = dist / diversitySigma;
					float weight = expf(-0.5f * sigmaRatio * sigmaRatio);
					for (auto& typeAndCount : typeCountGrid[sampleIdx])
					{
						float weightedCount = (float)typeAndCount.second * weight;
						weightedTypeCount[typeAndCount.first] += weightedCount;
						totalWeightedCount += weightedCount;
					}
				}
			}

			int idx = cellRow * m_gridWidth + cellCol;
			if (totalWeightedCount <= 0.f)
			{
				continue;
			}

			float entropy = 0.f;
			for (auto& typeAndCount : weightedTypeCount)
			{
				float p = typeAndCount.second / totalWeightedCount;
				if (p > 0.f)
				{
					entropy -= p * log2f(p);
				}
			}

			float confidence = totalWeightedCount / confidenceFullSample;
			if (confidence > 1.f) confidence = 1.f;
			localDiversityGrid[idx] = entropy * confidence;
		}
	}

	float const evidenceRadius = 160.f;
	int const evidenceCellRadius = (int)(evidenceRadius / m_cellWorldSize);

	std::vector<float> localActualValues(totalCells, 0.f);
	std::vector<float> evidenceDiversityValues;
	evidenceDiversityValues.reserve(totalCells);

	int evidenceCells = 0;
	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			float localActual = 0.f;

			int minRow = row - evidenceCellRadius;
			int maxRow = row + evidenceCellRadius;
			int minCol = col - evidenceCellRadius;
			int maxCol = col + evidenceCellRadius;
			if (minRow < 0) minRow = 0;
			if (minCol < 0) minCol = 0;
			if (maxRow >= m_gridHeight) maxRow = m_gridHeight - 1;
			if (maxCol >= m_gridWidth)  maxCol = m_gridWidth - 1;

			for (int sampleRow = minRow; sampleRow <= maxRow; ++sampleRow)
			{
				for (int sampleCol = minCol; sampleCol <= maxCol; ++sampleCol)
				{
					float dx = (float)(sampleCol - col) * m_cellWorldSize;
					float dy = (float)(sampleRow - row) * m_cellWorldSize;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist > evidenceRadius) continue;

					localActual += (float)countGrid[sampleRow * m_gridWidth + sampleCol];
				}
			}

			int idx = row * m_gridWidth + col;
			localActualValues[idx] = localActual;
			if (localActual <= 0.f)
			{
				continue;
			}

			evidenceDiversityValues.push_back(localDiversityGrid[idx]);
			++evidenceCells;
		}
	}

	float lowDiversityTarget = 0.f;
	float strongLowDiversity = 0.f;
	if (!evidenceDiversityValues.empty())
	{
		std::sort(evidenceDiversityValues.begin(), evidenceDiversityValues.end());
		strongLowDiversity = GetPercentileFromSortedValues(evidenceDiversityValues, 0.10f);
		lowDiversityTarget = GetPercentileFromSortedValues(evidenceDiversityValues, 0.40f);
		if (lowDiversityTarget < strongLowDiversity + 0.001f)
		{
			lowDiversityTarget = strongLowDiversity + 0.001f;
		}
	}

	int lowDiversityCells = 0;
	int acceptedCells = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (localActualValues[idx] <= 0.f)
		{
			++acceptedCells;
			continue;
		}

		float diversity = localDiversityGrid[idx];
		if (diversity >= lowDiversityTarget)
		{
			++acceptedCells;
			continue;
		}

		float score = (lowDiversityTarget - diversity) / (lowDiversityTarget - strongLowDiversity);
		outDeviationGrid[idx] = Clamp01(score);
		++lowDiversityCells;
	}

	g_theDevConsole->AddLine(Stringf("%s: %d low-diversity, %d accepted/no evidence, %d evidence cells (%.0fm evidence, p10 %.3f, p40 %.3f)",
		debugLabel, lowDiversityCells, acceptedCells, evidenceCells, evidenceRadius, strongLowDiversity, lowDiversityTarget), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Compares nearby monster threat against the zone's design threat from zone_reference.csv.
// Red = actual monster threat is too high for the zone. Blue = too weak for the zone.
// Accessibility is intentionally not used here; encounter exposure is handled by EncounterRisk.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildThreatAlignmentDeviationGrid()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_threatAlignmentDeviationGrid.assign(totalCells, 0.f);

	float const localRadius = 100.f;
	int const localCellRadius = (int)(localRadius / m_cellWorldSize);

	std::vector<float> rawDeltas(totalCells, 0.f);
	std::vector<float> absRawDeltas;
	absRawDeltas.reserve(totalCells);

	int evidenceCells = 0;
	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			float localActual = 0.f;
			float weightedThreat = 0.f;

			int minRow = row - localCellRadius;
			int maxRow = row + localCellRadius;
			int minCol = col - localCellRadius;
			int maxCol = col + localCellRadius;
			if (minRow < 0) minRow = 0;
			if (minCol < 0) minCol = 0;
			if (maxRow >= m_gridHeight) maxRow = m_gridHeight - 1;
			if (maxCol >= m_gridWidth)  maxCol = m_gridWidth - 1;

			for (int sampleRow = minRow; sampleRow <= maxRow; ++sampleRow)
			{
				for (int sampleCol = minCol; sampleCol <= maxCol; ++sampleCol)
				{
					float dx = (float)(sampleCol - col) * m_cellWorldSize;
					float dy = (float)(sampleRow - row) * m_cellWorldSize;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist > localRadius) continue;

					int sampleIdx = sampleRow * m_gridWidth + sampleCol;
					int monsterCount = m_actualCountGrid[sampleIdx];
					if (monsterCount <= 0) continue;

					localActual += (float)monsterCount;
					weightedThreat += m_threatGrid[sampleIdx] * (float)monsterCount;
				}
			}

			int idx = row * m_gridWidth + col;
			if (localActual <= 0.f)
			{
				continue;
			}

			float actualThreat = weightedThreat / localActual;
			float expectedThreat = 2.5f;
			auto threatIter = m_zoneExpectedThreat.find(m_cellZoneIdGrid[idx]);
			if (threatIter != m_zoneExpectedThreat.end())
			{
				expectedThreat = threatIter->second;
			}

			float rawDelta = actualThreat - expectedThreat;

			rawDeltas[idx] = rawDelta;
			absRawDeltas.push_back(fabsf(rawDelta));
			++evidenceCells;
		}
	}

	float deadZone = 0.f;
	float strongDelta = 1.f;
	if (!absRawDeltas.empty())
	{
		std::sort(absRawDeltas.begin(), absRawDeltas.end());
		deadZone = GetPercentileFromSortedValues(absRawDeltas, 0.60f);
		strongDelta = GetPercentileFromSortedValues(absRawDeltas, 0.95f);
		if (strongDelta < deadZone + 0.001f) strongDelta = deadZone + 0.001f;
	}

	int tooWeakCells = 0;
	int tooHardCells = 0;
	int acceptedCells = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		float magnitude = fabsf(rawDeltas[idx]);
		if (magnitude <= deadZone)
		{
			++acceptedCells;
			continue;
		}

		float normalized = Clamp01((magnitude - deadZone) / (strongDelta - deadZone));
		m_threatAlignmentDeviationGrid[idx] = rawDeltas[idx] < 0.f ? -normalized : normalized;
		if (rawDeltas[idx] < 0.f) ++tooWeakCells;
		else ++tooHardCells;
	}

	g_theDevConsole->AddLine(Stringf("ThreatAlignmentDeviation: %d too weak, %d too hard, %d accepted/no evidence, %d evidence cells (zone base threat, %.0fm radius, p60 %.3f, p95 %.3f)",
		tooWeakCells, tooHardCells, acceptedCells, evidenceCells, localRadius, deadZone, strongDelta), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Shared threat-alignment evaluator for generated distributions.
// The optimizer works with aggregate type assignments per cell, so it provides countGrid plus
// threatSumGrid. The rest of the method mirrors the current-data threat deviation map: compare
// nearby average threat against the zone's base_threat_level and normalize by percentile.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildThreatAlignmentDeviationGridForThreatSums(std::vector<int> const& countGrid, std::vector<float> const& threatSumGrid, std::vector<float>& outDeviationGrid, char const* debugLabel)
{
	int totalCells = m_gridWidth * m_gridHeight;
	outDeviationGrid.assign(totalCells, 0.f);

	float const localRadius = 100.f;
	int const localCellRadius = (int)(localRadius / m_cellWorldSize);

	std::vector<float> rawDeltas(totalCells, 0.f);
	std::vector<float> absRawDeltas;
	absRawDeltas.reserve(totalCells);

	int evidenceCells = 0;
	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			float localActual = 0.f;
			float weightedThreat = 0.f;

			int minRow = row - localCellRadius;
			int maxRow = row + localCellRadius;
			int minCol = col - localCellRadius;
			int maxCol = col + localCellRadius;
			if (minRow < 0) minRow = 0;
			if (minCol < 0) minCol = 0;
			if (maxRow >= m_gridHeight) maxRow = m_gridHeight - 1;
			if (maxCol >= m_gridWidth)  maxCol = m_gridWidth - 1;

			for (int sampleRow = minRow; sampleRow <= maxRow; ++sampleRow)
			{
				for (int sampleCol = minCol; sampleCol <= maxCol; ++sampleCol)
				{
					float dx = (float)(sampleCol - col) * m_cellWorldSize;
					float dy = (float)(sampleRow - row) * m_cellWorldSize;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist > localRadius) continue;

					int sampleIdx = sampleRow * m_gridWidth + sampleCol;
					int monsterCount = countGrid[sampleIdx];
					if (monsterCount <= 0) continue;

					localActual += (float)monsterCount;
					weightedThreat += threatSumGrid[sampleIdx];
				}
			}

			int idx = row * m_gridWidth + col;
			if (localActual <= 0.f)
			{
				continue;
			}

			float actualThreat = weightedThreat / localActual;
			float expectedThreat = 2.5f;
			auto threatIter = m_zoneExpectedThreat.find(m_cellZoneIdGrid[idx]);
			if (threatIter != m_zoneExpectedThreat.end())
			{
				expectedThreat = threatIter->second;
			}

			float rawDelta = actualThreat - expectedThreat;
			rawDeltas[idx] = rawDelta;
			absRawDeltas.push_back(fabsf(rawDelta));
			++evidenceCells;
		}
	}

	float deadZone = 0.f;
	float strongDelta = 1.f;
	if (!absRawDeltas.empty())
	{
		std::sort(absRawDeltas.begin(), absRawDeltas.end());
		deadZone = GetPercentileFromSortedValues(absRawDeltas, 0.60f);
		strongDelta = GetPercentileFromSortedValues(absRawDeltas, 0.95f);
		if (strongDelta < deadZone + 0.001f) strongDelta = deadZone + 0.001f;
	}

	int tooWeakCells = 0;
	int tooHardCells = 0;
	int acceptedCells = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		float magnitude = fabsf(rawDeltas[idx]);
		if (magnitude <= deadZone)
		{
			++acceptedCells;
			continue;
		}

		float normalized = Clamp01((magnitude - deadZone) / (strongDelta - deadZone));
		outDeviationGrid[idx] = rawDeltas[idx] < 0.f ? -normalized : normalized;
		if (rawDeltas[idx] < 0.f) ++tooWeakCells;
		else ++tooHardCells;
	}

	g_theDevConsole->AddLine(Stringf("%s: %d too weak, %d too hard, %d accepted/no evidence, %d evidence cells (zone base threat, %.0fm radius, p60 %.3f, p95 %.3f)",
		debugLabel, tooWeakCells, tooHardCells, acceptedCells, evidenceCells, localRadius, deadZone, strongDelta), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Combines the problem maps into a per-cell quality score.
// score 1 = healthy / no evident problem, score 0 = severe combined problem.
// Water is not a hard error: road_map.png is a low-confidence reference layer, so water cells
// simply use the same composite ProblemScore terms as other cells.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildFinalScoreGrid()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_finalScoreGrid.assign(totalCells, 1.f);

	float const densityWeight = 0.40f;
	float const threatWeight = 0.30f;
	float const diversityWeight = 0.20f;
	float const encounterWeight = 0.10f;

	int severeCells = 0;
	int warningCells = 0;
	float minScore = 1.f;
	float maxProblem = 0.f;

	for (int idx = 0; idx < totalCells; ++idx)
	{
		float densityProblem = Clamp01(fabsf(m_densityDeviationGrid[idx]));
		float threatProblem = Clamp01(fabsf(m_threatAlignmentDeviationGrid[idx]));
		float diversityProblem = Clamp01(m_diversityAccessibilityDeviationGrid[idx]);
		float encounterProblem = Clamp01(m_encounterRiskGrid[idx]);

		float problem = densityWeight * densityProblem
			+ threatWeight * threatProblem
			+ diversityWeight * diversityProblem
			+ encounterWeight * encounterProblem;

		problem = Clamp01(problem);
		float score = 1.f - problem;
		m_finalScoreGrid[idx] = score;

		if (score < minScore) minScore = score;
		if (problem > maxProblem) maxProblem = problem;
		if (score < 0.35f) ++severeCells;
		else if (score < 0.65f) ++warningCells;
	}

	g_theDevConsole->AddLine(Stringf("FinalScore: min score %.3f, max ProblemScore %.3f, %d severe, %d warning",
		minScore, maxProblem, severeCells, warningCells), Rgba8::WHITE);
}

//-----------------------------------------------------------------------------------------------
// Builds an optimized virtual distribution and scores it with the same evaluation system.
//
// This does not overwrite monster_points.csv. It keeps the current distribution as the initial
// state, then performs a coarse-to-fine local search driven by ProblemScore:
// 1) Rank high-problem blocks from 320m down to 20m.
// 2) Generate candidate moves/type swaps based on the dominant problem component.
// 3) Pick the acceptance threshold from that round's actual candidate improvement distribution.
// 4) Accept candidates that improve the local patch estimate and do not worsen type-ratio rules.
// 5) Rebuild all optimized deviation maps after every round for honest before/after debug output.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::BuildOptimizedDistributionGrids()
{
	int totalCells = m_gridWidth * m_gridHeight;
	m_optimizedActualCountGrid = m_actualCountGrid;
	m_optimizedDensityDeviationGrid.assign(totalCells, 0.f);
	m_optimizedEncounterRiskGrid.assign(totalCells, 0.f);
	m_optimizedDiversityDeviationGrid.assign(totalCells, 0.f);
	m_optimizedThreatAlignmentDeviationGrid.assign(totalCells, 0.f);
	m_optimizedProblemScoreGrid.assign(totalCells, 0.f);

	struct MonsterTypeInfo
	{
		std::string type;
		std::string category;
		float threat = 1.f;
		bool isSpecial = false;
		float minRatio = 0.f;
		float maxRatio = 1.f;
	};

	int colType = m_monsterReference.GetColumnIndex("monster_type");
	int colCategory = m_monsterReference.GetColumnIndex("category");
	int colThreat = m_monsterReference.GetColumnIndex("threat_level");
	GUARANTEE_OR_DIE(colType >= 0 && colCategory >= 0 && colThreat >= 0,
		"monster_reference.csv missing monster_type, category, or threat_level");

	std::vector<MonsterTypeInfo> monsterTypes;
	monsterTypes.reserve(m_monsterReference.GetRowCount());
	for (int i = 0; i < m_monsterReference.GetRowCount(); ++i)
	{
		Strings const& row = m_monsterReference.m_rows[i];
		MonsterTypeInfo info;
		info.type = ParseCsvValue(row, colType, std::string(""));
		info.category = ParseCsvValue(row, colCategory, std::string(""));
		info.threat = ParseCsvValue(row, colThreat, 1.f);
		info.isSpecial = (info.category == "特殊感染者");
		if (info.type == "M01") { info.minRatio = 0.45f; info.maxRatio = 0.55f; }
		else if (info.type == "M04") { info.minRatio = 0.05f; info.maxRatio = 0.08f; }
		else if (info.type == "M07") { info.minRatio = 0.01f; info.maxRatio = 0.02f; }
		else if (info.type == "M08") { info.minRatio = 0.01f; info.maxRatio = 0.02f; }
		else if (info.type == "M02") { info.minRatio = 0.10f; info.maxRatio = 0.15f; }
		else if (info.type == "M03") { info.minRatio = 0.10f; info.maxRatio = 0.13f; }
		else if (info.type == "M05") { info.minRatio = 0.01f; info.maxRatio = 0.03f; }
		else if (info.type == "M06") { info.minRatio = 0.01f; info.maxRatio = 0.02f; }
		else if (info.type == "M09") { info.minRatio = 0.00f; info.maxRatio = 0.01f; }
		if (!info.type.empty())
		{
			monsterTypes.push_back(info);
		}
	}

	std::vector<std::map<std::string, int>> optimizedTypeCountGrid(totalCells);
	std::vector<float> optimizedThreatSumGrid(totalCells, 0.f);
	std::map<std::string, MonsterTypeInfo> typeInfoByName;
	std::map<std::string, int> globalTypeCounts;
	std::map<int, int> zoneTotalCounts;
	std::map<int, int> zoneSpecialCounts;
	int optimizedNormalCount = 0;
	int optimizedSpecialCount = 0;
	int totalCurrentMonsters = 0;

	for (MonsterTypeInfo const& info : monsterTypes)
	{
		typeInfoByName[info.type] = info;
		globalTypeCounts[info.type] = 0;
	}

	int colX = m_monsterPoints.GetColumnIndex("world_x");
	int colY = m_monsterPoints.GetColumnIndex("world_y");
	int colMT = m_monsterPoints.GetColumnIndex("monster_type");
	GUARANTEE_OR_DIE(colX >= 0 && colY >= 0 && colMT >= 0, "monster_points.csv missing world_x, world_y, or monster_type");
	for (int i = 0; i < m_monsterPoints.GetRowCount(); ++i)
	{
		Strings const& row = m_monsterPoints.m_rows[i];
		float wx = ParseCsvValue(row, colX, 0.f);
		float wy = ParseCsvValue(row, colY, 0.f);
		std::string type = ParseCsvValue(row, colMT, std::string(""));
		int col = (int)(wx / m_cellWorldSize);
		int r = (int)(wy / m_cellWorldSize);
		if (col < 0) col = 0;
		if (r < 0) r = 0;
		if (col >= m_gridWidth) col = m_gridWidth - 1;
		if (r >= m_gridHeight) r = m_gridHeight - 1;
		int idx = r * m_gridWidth + col;

		auto typeIter = typeInfoByName.find(type);
		if (typeIter == typeInfoByName.end())
		{
			continue;
		}

		MonsterTypeInfo const& info = typeIter->second;
		optimizedTypeCountGrid[idx][type] += 1;
		optimizedThreatSumGrid[idx] += info.threat;
		globalTypeCounts[type] += 1;
		zoneTotalCounts[m_cellZoneIdGrid[idx]] += 1;
		if (info.isSpecial)
		{
			zoneSpecialCounts[m_cellZoneIdGrid[idx]] += 1;
			++optimizedSpecialCount;
		}
		else
		{
			++optimizedNormalCount;
		}
		++totalCurrentMonsters;
	}

	if (totalCurrentMonsters <= 0 || monsterTypes.empty())
	{
		g_theDevConsole->AddLine("OptimizedDistribution: skipped; no monsters or no monster reference rows", Rgba8::YELLOW);
		return;
	}

	auto getExpectedThreatForCell = [&](int idx)->float
	{
		float expectedThreat = 2.5f;
		auto threatIter = m_zoneExpectedThreat.find(m_cellZoneIdGrid[idx]);
		if (threatIter != m_zoneExpectedThreat.end())
		{
			expectedThreat = threatIter->second;
		}
		return expectedThreat;
	};

	auto getZoneSpecialTarget = [&](int zoneId)->float
	{
		float target = 0.30f;
		auto categoryIter = m_zoneCategory.find(zoneId);
		if (categoryIter != m_zoneCategory.end())
		{
			if (categoryIter->second == "功能区") target = 0.50f;
			else if (categoryIter->second == "据点") target = 0.20f;
		}
		return target;
	};

	auto distanceToRange = [](float value, float minValue, float maxValue)->float
	{
		if (value < minValue) return minValue - value;
		if (value > maxValue) return value - maxValue;
		return 0.f;
	};

	auto isRatioTransitionAllowed = [&](float beforeValue, float afterValue, float minValue, float maxValue)->bool
	{
		float beforeDistance = distanceToRange(beforeValue, minValue, maxValue);
		float afterDistance = distanceToRange(afterValue, minValue, maxValue);
		if (beforeDistance <= 0.f)
		{
			return afterDistance <= 0.0001f;
		}
		return afterDistance <= beforeDistance + 0.0001f;
	};

	auto isGlobalTypeChangeAllowed = [&](std::string const& oldType, std::string const& newType, char const*& outRejectReason)->bool
	{
		if (oldType == newType)
		{
			return false;
		}
		std::map<std::string, int> newCounts = globalTypeCounts;
		newCounts[oldType] -= 1;
		newCounts[newType] += 1;

		for (MonsterTypeInfo const& info : monsterTypes)
		{
			int beforeCount = globalTypeCounts[info.type];
			int afterCount = newCounts[info.type];
			float beforeRatio = (float)beforeCount / (float)totalCurrentMonsters;
			float afterRatio = (float)afterCount / (float)totalCurrentMonsters;
			if (!isRatioTransitionAllowed(beforeRatio, afterRatio, info.minRatio, info.maxRatio))
			{
				outRejectReason = "global_type_ratio";
				return false;
			}
		}

		int beforeSpecial = optimizedSpecialCount;
		int afterSpecial = beforeSpecial;
		if (typeInfoByName[oldType].isSpecial) --afterSpecial;
		if (typeInfoByName[newType].isSpecial) ++afterSpecial;
		float beforeSpecialRatio = (float)beforeSpecial / (float)totalCurrentMonsters;
		float afterSpecialRatio = (float)afterSpecial / (float)totalCurrentMonsters;
		if (!isRatioTransitionAllowed(beforeSpecialRatio, afterSpecialRatio, 0.30f, 0.40f))
		{
			outRejectReason = "global_type_ratio";
			return false;
		}
		return true;
	};

	auto isZoneSpecialChangeAllowed = [&](int sourceZone, int destZone, std::string const& oldType, std::string const& newType, bool isMove, char const*& outRejectReason)->bool
	{
		std::map<int, int> newZoneTotal = zoneTotalCounts;
		std::map<int, int> newZoneSpecial = zoneSpecialCounts;
		if (isMove && sourceZone != destZone)
		{
			newZoneTotal[sourceZone] -= 1;
			newZoneTotal[destZone] += 1;
			if (typeInfoByName[oldType].isSpecial)
			{
				newZoneSpecial[sourceZone] -= 1;
				newZoneSpecial[destZone] += 1;
			}
		}
		else if (!isMove)
		{
			if (typeInfoByName[oldType].isSpecial) newZoneSpecial[sourceZone] -= 1;
			if (typeInfoByName[newType].isSpecial) newZoneSpecial[sourceZone] += 1;
		}

		int zonesToCheck[2] = { sourceZone, destZone };
		for (int zoneIndex = 0; zoneIndex < 2; ++zoneIndex)
		{
			int zoneId = zonesToCheck[zoneIndex];
			if (zoneId < 0 || zoneTotalCounts[zoneId] <= 0 || newZoneTotal[zoneId] <= 0)
			{
				continue;
			}
			float target = getZoneSpecialTarget(zoneId);
			float minRatio = std::max(0.f, target - 0.05f);
			float maxRatio = std::min(1.f, target + 0.05f);
			float beforeRatio = (float)zoneSpecialCounts[zoneId] / (float)zoneTotalCounts[zoneId];
			float afterRatio = (float)newZoneSpecial[zoneId] / (float)newZoneTotal[zoneId];
			if (!isRatioTransitionAllowed(beforeRatio, afterRatio, minRatio, maxRatio))
			{
				outRejectReason = "zone_ratio";
				return false;
			}
		}
		return true;
	};

	float const densityWeight = 0.40f;
	float const threatWeight = 0.30f;
	float const diversityWeight = 0.20f;
	float const encounterWeight = 0.10f;

	auto combineOptimizedProblemScore = [&]()
	{
		for (int idx = 0; idx < totalCells; ++idx)
		{
			float densityProblem = Clamp01(fabsf(m_optimizedDensityDeviationGrid[idx]));
			float threatProblem = Clamp01(fabsf(m_optimizedThreatAlignmentDeviationGrid[idx]));
			float diversityProblem = Clamp01(m_optimizedDiversityDeviationGrid[idx]);
			float encounterProblem = Clamp01(m_optimizedEncounterRiskGrid[idx]);
			float problemScore = densityWeight * densityProblem
				+ threatWeight * threatProblem
				+ diversityWeight * diversityProblem
				+ encounterWeight * encounterProblem;
			m_optimizedProblemScoreGrid[idx] = Clamp01(problemScore);
		}
	};

	struct ProblemStats
	{
		float avg = 0.f;
		float p95 = 0.f;
		int severe = 0;
	};

	auto getProblemStats = [&](std::vector<float> const& grid)->ProblemStats
	{
		ProblemStats stats;
		std::vector<float> values = grid;
		float sum = 0.f;
		for (float value : values)
		{
			sum += value;
			if (value > 0.65f) ++stats.severe;
		}
		stats.avg = values.empty() ? 0.f : sum / (float)values.size();
		std::sort(values.begin(), values.end());
		stats.p95 = GetPercentileFromSortedValues(values, 0.95f);
		return stats;
	};

	auto rebuildOptimizedEvaluation = [&](char const* densityCsvPath)
	{
		BuildDensityDeviationGridForCounts(
			m_optimizedActualCountGrid,
			m_optimizedDensityDeviationGrid,
			"OptimizedDensityDeviation",
			densityCsvPath);
		BuildEncounterRiskGridForCounts(m_optimizedActualCountGrid, m_optimizedEncounterRiskGrid, "OptimizedEncounterRisk");
		BuildLowDiversityDeviationGridForTypeCounts(m_optimizedActualCountGrid, optimizedTypeCountGrid, m_optimizedDiversityDeviationGrid, "OptimizedLowDiversityDeviation");
		BuildThreatAlignmentDeviationGridForThreatSums(m_optimizedActualCountGrid, optimizedThreatSumGrid, m_optimizedThreatAlignmentDeviationGrid, "OptimizedThreatAlignmentDeviation");
		combineOptimizedProblemScore();
	};

	auto getPatchProblem = [&](int centerIdx, int radiusCells)->float
	{
		int centerRow = centerIdx / m_gridWidth;
		int centerCol = centerIdx % m_gridWidth;
		int minRow = std::max(0, centerRow - radiusCells);
		int maxRow = std::min(m_gridHeight - 1, centerRow + radiusCells);
		int minCol = std::max(0, centerCol - radiusCells);
		int maxCol = std::min(m_gridWidth - 1, centerCol + radiusCells);
		float sum = 0.f;
		int count = 0;
		for (int row = minRow; row <= maxRow; ++row)
		{
			for (int col = minCol; col <= maxCol; ++col)
			{
				sum += m_optimizedProblemScoreGrid[row * m_gridWidth + col];
				++count;
			}
		}
		return count > 0 ? sum / (float)count : 0.f;
	};

	auto getCellDominantProblem = [&](int idx)->std::string
	{
		float densityProblem = densityWeight * Clamp01(fabsf(m_optimizedDensityDeviationGrid[idx]));
		float threatProblem = threatWeight * Clamp01(fabsf(m_optimizedThreatAlignmentDeviationGrid[idx]));
		float diversityProblem = diversityWeight * Clamp01(m_optimizedDiversityDeviationGrid[idx]);
		float encounterProblem = encounterWeight * Clamp01(m_optimizedEncounterRiskGrid[idx]);
		if (densityProblem >= threatProblem && densityProblem >= diversityProblem && densityProblem >= encounterProblem) return "density";
		if (threatProblem >= diversityProblem && threatProblem >= encounterProblem) return "threat";
		if (diversityProblem >= encounterProblem) return "diversity";
		return "encounter";
	};

	auto getMostCommonTypeInCell = [&](int idx)->std::string
	{
		std::string bestType;
		int bestCount = 0;
		for (auto const& typeAndCount : optimizedTypeCountGrid[idx])
		{
			if (typeAndCount.second > bestCount)
			{
				bestType = typeAndCount.first;
				bestCount = typeAndCount.second;
			}
		}
		return bestType;
	};

	auto getBestReplacementType = [&](int idx, std::string const& oldType, std::string const& dominantProblem)->std::string
	{
		float expectedThreat = getExpectedThreatForCell(idx);
		std::string bestType;
		float bestScore = 999999.f;
		for (MonsterTypeInfo const& info : monsterTypes)
		{
			if (info.type == oldType)
			{
				continue;
			}
			float score = fabsf(info.threat - expectedThreat);
			if (dominantProblem == "diversity")
			{
				score += (float)optimizedTypeCountGrid[idx][info.type] * 0.50f;
				score += (float)globalTypeCounts[info.type] / (float)totalCurrentMonsters;
			}
			char const* rejectReason = "";
			if (!isGlobalTypeChangeAllowed(oldType, info.type, rejectReason))
			{
				continue;
			}
			if (!isZoneSpecialChangeAllowed(m_cellZoneIdGrid[idx], m_cellZoneIdGrid[idx], oldType, info.type, false, rejectReason))
			{
				continue;
			}
			if (score < bestScore)
			{
				bestScore = score;
				bestType = info.type;
			}
		}
		return bestType;
	};

	struct OptimizationLevel
	{
		int blockCells = 16;
		int rounds = 2;
		float minCellProblem = 0.f;
		float topFractions[3] = { 0.10f, 0.10f, 0.10f };
		int candidateCaps[3] = { 100, 100, 100 };
	};

	struct BlockInfo
	{
		int minCol = 0;
		int minRow = 0;
		int maxCol = 0;
		int maxRow = 0;
		float score = 0.f;
		float avgProblem = 0.f;
		float maxProblem = 0.f;
		int monsterCount = 0;
		std::string dominantProblem;
		int clusterId = -1;
	};

	struct CandidateOperation
	{
		std::string operationType;
		std::string dominantProblem;
		int sourceIdx = -1;
		int destIdx = -1;
		std::string oldType;
		std::string newType;
		float oldPatchProblem = 0.f;
		float newPatchProblem = 0.f;
		float improvementRatio = 0.f;
		int clusterId = -1;
		std::string rejectReason;
	};

	auto buildProblemBlocks = [&](OptimizationLevel const& level)->std::vector<BlockInfo>
	{
		std::vector<BlockInfo> blocks;
		int blocksX = (m_gridWidth + level.blockCells - 1) / level.blockCells;
		int blocksY = (m_gridHeight + level.blockCells - 1) / level.blockCells;
		for (int row = 0; row < m_gridHeight; row += level.blockCells)
		{
			for (int col = 0; col < m_gridWidth; col += level.blockCells)
			{
				BlockInfo block;
				block.minCol = col;
				block.minRow = row;
				block.maxCol = std::min(m_gridWidth - 1, col + level.blockCells - 1);
				block.maxRow = std::min(m_gridHeight - 1, row + level.blockCells - 1);
				float sumProblem = 0.f;
				float maxProblem = 0.f;
				float sumDensity = 0.f;
				float sumThreat = 0.f;
				float sumDiversity = 0.f;
				float sumEncounter = 0.f;
				int count = 0;
				for (int r = block.minRow; r <= block.maxRow; ++r)
				{
					for (int c = block.minCol; c <= block.maxCol; ++c)
					{
						int idx = r * m_gridWidth + c;
						float problem = m_optimizedProblemScoreGrid[idx];
						sumProblem += problem;
						if (problem > maxProblem) maxProblem = problem;
						block.monsterCount += m_optimizedActualCountGrid[idx];
						sumDensity += densityWeight * Clamp01(fabsf(m_optimizedDensityDeviationGrid[idx]));
						sumThreat += threatWeight * Clamp01(fabsf(m_optimizedThreatAlignmentDeviationGrid[idx]));
						sumDiversity += diversityWeight * Clamp01(m_optimizedDiversityDeviationGrid[idx]);
						sumEncounter += encounterWeight * Clamp01(m_optimizedEncounterRiskGrid[idx]);
						++count;
					}
				}
				float avgProblem = count > 0 ? sumProblem / (float)count : 0.f;
				block.avgProblem = avgProblem;
				block.maxProblem = maxProblem;
				if (avgProblem < level.minCellProblem && maxProblem < level.minCellProblem)
				{
					continue;
				}
				block.score = 0.70f * avgProblem + 0.30f * maxProblem;
				if (sumDensity >= sumThreat && sumDensity >= sumDiversity && sumDensity >= sumEncounter) block.dominantProblem = "density";
				else if (sumThreat >= sumDiversity && sumThreat >= sumEncounter) block.dominantProblem = "threat";
				else if (sumDiversity >= sumEncounter) block.dominantProblem = "diversity";
				else block.dominantProblem = "encounter";
				blocks.push_back(block);
			}
		}

		if (level.blockCells >= 8 && !blocks.empty())
		{
			std::vector<float> scores;
			scores.reserve(blocks.size());
			for (BlockInfo const& block : blocks)
			{
				scores.push_back(block.score);
			}
			std::sort(scores.begin(), scores.end());
			float clusterThreshold = GetPercentileFromSortedValues(scores, 0.70f);
			std::vector<int> blockIndexByGrid((size_t)blocksX * (size_t)blocksY, -1);
			for (int i = 0; i < (int)blocks.size(); ++i)
			{
				int bx = blocks[i].minCol / level.blockCells;
				int by = blocks[i].minRow / level.blockCells;
				blockIndexByGrid[by * blocksX + bx] = i;
			}

			int nextClusterId = 0;
			for (int i = 0; i < (int)blocks.size(); ++i)
			{
				if (blocks[i].clusterId >= 0 || blocks[i].score < clusterThreshold)
				{
					continue;
				}

				std::queue<int> open;
				open.push(i);
				blocks[i].clusterId = nextClusterId;
				while (!open.empty())
				{
					int blockIndex = open.front();
					open.pop();
					int bx = blocks[blockIndex].minCol / level.blockCells;
					int by = blocks[blockIndex].minRow / level.blockCells;
					int neighbors[4][2] = { {bx - 1, by}, {bx + 1, by}, {bx, by - 1}, {bx, by + 1} };
					for (int n = 0; n < 4; ++n)
					{
						int nbx = neighbors[n][0];
						int nby = neighbors[n][1];
						if (nbx < 0 || nby < 0 || nbx >= blocksX || nby >= blocksY)
						{
							continue;
						}
						int neighborIndex = blockIndexByGrid[nby * blocksX + nbx];
						if (neighborIndex < 0 || blocks[neighborIndex].clusterId >= 0 || blocks[neighborIndex].score < clusterThreshold)
						{
							continue;
						}
						blocks[neighborIndex].clusterId = nextClusterId;
						open.push(neighborIndex);
					}
				}
				++nextClusterId;
			}
		}

		std::sort(blocks.begin(), blocks.end(), [](BlockInfo const& a, BlockInfo const& b) { return a.score > b.score; });
		return blocks;
	};

	auto findBestSourceInBlock = [&](BlockInfo const& block, std::string const& dominantProblem, std::map<int, int> const* plannedSourceUses)->int
	{
		int bestIdx = -1;
		float bestScore = -1.f;
		for (int r = block.minRow; r <= block.maxRow; ++r)
		{
			for (int c = block.minCol; c <= block.maxCol; ++c)
			{
				int idx = r * m_gridWidth + c;
				if (m_optimizedActualCountGrid[idx] <= 0)
				{
					continue;
				}
				int plannedUseCount = 0;
				if (plannedSourceUses != nullptr)
				{
					auto plannedIter = plannedSourceUses->find(idx);
					if (plannedIter != plannedSourceUses->end())
					{
						plannedUseCount = plannedIter->second;
					}
				}
				if (m_optimizedActualCountGrid[idx] <= plannedUseCount)
				{
					continue;
				}
				float score = m_optimizedProblemScoreGrid[idx];
				if (dominantProblem == "density") score += Clamp01(m_optimizedDensityDeviationGrid[idx]);
				if (dominantProblem == "encounter") score += Clamp01(m_optimizedEncounterRiskGrid[idx]);
				if (dominantProblem == "threat") score += Clamp01(fabsf(m_optimizedThreatAlignmentDeviationGrid[idx]));
				if (dominantProblem == "diversity") score += Clamp01(m_optimizedDiversityDeviationGrid[idx]);
				score -= (float)plannedUseCount * 0.35f;
				if (score > bestScore)
				{
					bestScore = score;
					bestIdx = idx;
				}
			}
		}
		return bestIdx;
	};

	auto findBestMoveTarget = [&](BlockInfo const& block, int sourceIdx, std::string const& dominantProblem)->int
	{
		int bestIdx = -1;
		float bestScore = 999999.f;
		int centerRow = sourceIdx / m_gridWidth;
		int centerCol = sourceIdx % m_gridWidth;
		int radius = std::max(2, block.maxRow - block.minRow + 1);
		int minRow = std::max(0, centerRow - radius);
		int maxRow = std::min(m_gridHeight - 1, centerRow + radius);
		int minCol = std::max(0, centerCol - radius);
		int maxCol = std::min(m_gridWidth - 1, centerCol + radius);
		for (int r = minRow; r <= maxRow; ++r)
		{
			for (int c = minCol; c <= maxCol; ++c)
			{
				int idx = r * m_gridWidth + c;
				if (idx == sourceIdx || !ShouldScoreDensityDeviationCell(m_cellTypeGrid[idx]))
				{
					continue;
				}
				float score = m_optimizedProblemScoreGrid[idx];
				if (dominantProblem == "density") score += Clamp01(m_optimizedDensityDeviationGrid[idx]);
				score += (float)m_optimizedActualCountGrid[idx] * 0.02f;
				if (score < bestScore)
				{
					bestScore = score;
					bestIdx = idx;
				}
			}
		}
		return bestIdx;
	};

	auto makeCandidateForBlock = [&](BlockInfo const& block, std::map<int, int> const& plannedSourceUses)->CandidateOperation
	{
		CandidateOperation candidate;
		candidate.dominantProblem = block.dominantProblem;
		candidate.clusterId = block.clusterId;
		int sourceIdx = findBestSourceInBlock(block, block.dominantProblem, &plannedSourceUses);
		if (sourceIdx < 0)
		{
			candidate.rejectReason = "empty_source";
			return candidate;
		}
		candidate.sourceIdx = sourceIdx;
		candidate.oldType = getMostCommonTypeInCell(sourceIdx);
		if (candidate.oldType.empty())
		{
			candidate.rejectReason = "empty_source";
			return candidate;
		}

		int patchRadiusCells = std::max(1, (int)(240.f / m_cellWorldSize));
		candidate.oldPatchProblem = getPatchProblem(sourceIdx, patchRadiusCells);

		if (block.dominantProblem == "density" || block.dominantProblem == "encounter")
		{
			candidate.operationType = "move";
			candidate.destIdx = findBestMoveTarget(block, sourceIdx, block.dominantProblem);
			if (candidate.destIdx < 0)
			{
				candidate.rejectReason = "no_candidate_target";
				return candidate;
			}
			candidate.newType = candidate.oldType;
			char const* rejectReason = "";
			if (!isZoneSpecialChangeAllowed(m_cellZoneIdGrid[sourceIdx], m_cellZoneIdGrid[candidate.destIdx], candidate.oldType, candidate.oldType, true, rejectReason))
			{
				candidate.rejectReason = rejectReason;
				return candidate;
			}
			float sourceProblem = m_optimizedProblemScoreGrid[sourceIdx];
			float destProblem = m_optimizedProblemScoreGrid[candidate.destIdx];
			float estimatedRatio = Clamp01((sourceProblem - destProblem) * 0.25f + Clamp01(m_optimizedDensityDeviationGrid[sourceIdx]) * 0.15f);
			if (estimatedRatio <= 0.f)
			{
				candidate.rejectReason = "no_improvement";
				return candidate;
			}
			candidate.oldPatchProblem = 0.5f * (candidate.oldPatchProblem + getPatchProblem(candidate.destIdx, patchRadiusCells));
			candidate.newPatchProblem = candidate.oldPatchProblem * (1.f - estimatedRatio);
		}
		else
		{
			candidate.operationType = "type_swap";
			candidate.newType = getBestReplacementType(sourceIdx, candidate.oldType, block.dominantProblem);
			if (candidate.newType.empty())
			{
				candidate.rejectReason = "global_type_ratio";
				return candidate;
			}
			char const* rejectReason = "";
			if (!isGlobalTypeChangeAllowed(candidate.oldType, candidate.newType, rejectReason)
				|| !isZoneSpecialChangeAllowed(m_cellZoneIdGrid[sourceIdx], m_cellZoneIdGrid[sourceIdx], candidate.oldType, candidate.newType, false, rejectReason))
			{
				candidate.rejectReason = rejectReason;
				return candidate;
			}
			float oldThreatDelta = fabsf(typeInfoByName[candidate.oldType].threat - getExpectedThreatForCell(sourceIdx));
			float newThreatDelta = fabsf(typeInfoByName[candidate.newType].threat - getExpectedThreatForCell(sourceIdx));
			float threatGain = std::max(0.f, oldThreatDelta - newThreatDelta) * 0.12f;
			float diversityGain = block.dominantProblem == "diversity" ? 0.08f : 0.02f;
			float estimatedRatio = Clamp01(threatGain + diversityGain + m_optimizedProblemScoreGrid[sourceIdx] * 0.10f);
			if (estimatedRatio <= 0.f)
			{
				candidate.rejectReason = "no_improvement";
				return candidate;
			}
			candidate.newPatchProblem = candidate.oldPatchProblem * (1.f - estimatedRatio);
		}

		float improvement = candidate.oldPatchProblem - candidate.newPatchProblem;
		candidate.improvementRatio = improvement / std::max(candidate.oldPatchProblem, 0.001f);
		if (improvement <= 0.f)
		{
			candidate.rejectReason = "no_improvement";
		}
		return candidate;
	};

	auto applyCandidate = [&](CandidateOperation const& candidate)->bool
	{
		if (candidate.sourceIdx < 0 || m_optimizedActualCountGrid[candidate.sourceIdx] <= 0)
		{
			return false;
		}
		if (optimizedTypeCountGrid[candidate.sourceIdx][candidate.oldType] <= 0)
		{
			return false;
		}
		if (candidate.operationType == "move")
		{
			if (candidate.destIdx < 0)
			{
				return false;
			}
			int sourceZone = m_cellZoneIdGrid[candidate.sourceIdx];
			int destZone = m_cellZoneIdGrid[candidate.destIdx];
			--m_optimizedActualCountGrid[candidate.sourceIdx];
			++m_optimizedActualCountGrid[candidate.destIdx];
			--optimizedTypeCountGrid[candidate.sourceIdx][candidate.oldType];
			++optimizedTypeCountGrid[candidate.destIdx][candidate.oldType];
			float threat = typeInfoByName[candidate.oldType].threat;
			optimizedThreatSumGrid[candidate.sourceIdx] -= threat;
			optimizedThreatSumGrid[candidate.destIdx] += threat;
			if (sourceZone != destZone)
			{
				zoneTotalCounts[sourceZone] -= 1;
				zoneTotalCounts[destZone] += 1;
				if (typeInfoByName[candidate.oldType].isSpecial)
				{
					zoneSpecialCounts[sourceZone] -= 1;
					zoneSpecialCounts[destZone] += 1;
				}
			}
			return true;
		}
		else if (candidate.operationType == "type_swap")
		{
			int zoneId = m_cellZoneIdGrid[candidate.sourceIdx];
			--optimizedTypeCountGrid[candidate.sourceIdx][candidate.oldType];
			++optimizedTypeCountGrid[candidate.sourceIdx][candidate.newType];
			optimizedThreatSumGrid[candidate.sourceIdx] -= typeInfoByName[candidate.oldType].threat;
			optimizedThreatSumGrid[candidate.sourceIdx] += typeInfoByName[candidate.newType].threat;
			globalTypeCounts[candidate.oldType] -= 1;
			globalTypeCounts[candidate.newType] += 1;
			if (typeInfoByName[candidate.oldType].isSpecial)
			{
				--optimizedSpecialCount;
				++optimizedNormalCount;
				zoneSpecialCounts[zoneId] -= 1;
			}
			if (typeInfoByName[candidate.newType].isSpecial)
			{
				++optimizedSpecialCount;
				--optimizedNormalCount;
				zoneSpecialCounts[zoneId] += 1;
			}
			return true;
		}
		return false;
	};

	std::vector<float> currentProblemGrid(totalCells, 0.f);
	for (int idx = 0; idx < totalCells; ++idx)
	{
		currentProblemGrid[idx] = 1.f - Clamp01(m_finalScoreGrid[idx]);
	}
	ProblemStats beforeStats = getProblemStats(currentProblemGrid);

	struct ProblemDeltaStats
	{
		int improvedCells = 0;
		int worsenedCells = 0;
		float highAvgBefore = 0.f;
		float highAvgAfter = 0.f;
	};

	auto getProblemDeltaStats = [&](std::vector<float> const& beforeGrid, std::vector<float> const& afterGrid)->ProblemDeltaStats
	{
		ProblemDeltaStats stats;
		float highBeforeSum = 0.f;
		float highAfterSum = 0.f;
		int highCount = 0;
		for (int idx = 0; idx < totalCells; ++idx)
		{
			float delta = afterGrid[idx] - beforeGrid[idx];
			if (delta < -0.000001f) ++stats.improvedCells;
			else if (delta > 0.000001f) ++stats.worsenedCells;

			if (beforeGrid[idx] >= 0.35f)
			{
				highBeforeSum += beforeGrid[idx];
				highAfterSum += afterGrid[idx];
				++highCount;
			}
		}
		if (highCount > 0)
		{
			stats.highAvgBefore = highBeforeSum / (float)highCount;
			stats.highAvgAfter = highAfterSum / (float)highCount;
		}
		return stats;
	};

	std::string summaryCsv = "current_avg_problem,current_p95_problem,current_severe_cells,initial_optimized_avg_problem,initial_optimized_p95_problem,final_avg_problem,final_p95_problem,final_severe_cells,avg_problem_delta,p95_problem_delta,severe_cell_delta,total_rounds,total_candidates,total_positive_candidates,total_accepted,high_problem_avg_before,high_problem_avg_after,improved_cell_count,worsened_cell_count\n";
	std::string candidateCsv = "level,round,operation_type,source_col,source_row,dest_col,dest_row,dominant_problem,cluster_id,old_type,new_type,old_patch_problem,new_patch_problem,improvement_ratio,reject_reason\n";

	rebuildOptimizedEvaluation(nullptr);
	ProblemStats initialOptimizedStats = getProblemStats(m_optimizedProblemScoreGrid);
	int totalOptimizationRounds = 0;
	int totalCandidateCount = 0;
	int totalPositiveCandidateCount = 0;
	int totalAcceptedCount = 0;

	OptimizationLevel levels[5];
	levels[0].blockCells = 16; levels[0].rounds = 3; levels[0].minCellProblem = 0.f; levels[0].topFractions[0] = 0.30f; levels[0].topFractions[1] = 0.22f; levels[0].topFractions[2] = 0.15f; levels[0].candidateCaps[0] = 180; levels[0].candidateCaps[1] = 140; levels[0].candidateCaps[2] = 100;
	levels[1].blockCells = 8;  levels[1].rounds = 3; levels[1].minCellProblem = 0.f; levels[1].topFractions[0] = 0.20f; levels[1].topFractions[1] = 0.14f; levels[1].topFractions[2] = 0.09f; levels[1].candidateCaps[0] = 420; levels[1].candidateCaps[1] = 300; levels[1].candidateCaps[2] = 220;
	levels[2].blockCells = 4;  levels[2].rounds = 2; levels[2].minCellProblem = 0.f; levels[2].topFractions[0] = 0.06f; levels[2].topFractions[1] = 0.04f; levels[2].topFractions[2] = 0.03f; levels[2].candidateCaps[0] = 240; levels[2].candidateCaps[1] = 180; levels[2].candidateCaps[2] = 140;
	levels[3].blockCells = 2;  levels[3].rounds = 1; levels[3].minCellProblem = 0.f; levels[3].topFractions[0] = 0.03f; levels[3].topFractions[1] = 0.02f; levels[3].topFractions[2] = 0.02f; levels[3].candidateCaps[0] = 180; levels[3].candidateCaps[1] = 120; levels[3].candidateCaps[2] = 120;
	levels[4].blockCells = 1;  levels[4].rounds = 1; levels[4].minCellProblem = 0.50f; levels[4].topFractions[0] = 1.00f; levels[4].topFractions[1] = 1.00f; levels[4].topFractions[2] = 1.00f; levels[4].candidateCaps[0] = 1000; levels[4].candidateCaps[1] = 600; levels[4].candidateCaps[2] = 600;

	auto getBlockOperationBudget = [&](BlockInfo const& block, OptimizationLevel const& level)->int
	{
		if (block.monsterCount <= 0)
		{
			return 1;
		}

		float severity = Clamp01((block.score - 0.15f) / 0.45f);
		float ratio = 0.05f + 0.10f * severity;
		int maxOps = 8;
		if (level.blockCells >= 16)
		{
			ratio = 0.04f + 0.08f * severity;
			maxOps = 12;
		}
		else if (level.blockCells >= 8)
		{
			ratio = 0.05f + 0.10f * severity;
			maxOps = 10;
		}
		else if (level.blockCells >= 4)
		{
			ratio = 0.06f + 0.12f * severity;
			maxOps = 8;
		}
		else if (level.blockCells >= 2)
		{
			ratio = 0.08f + 0.16f * severity;
			maxOps = 6;
		}
		else
		{
			ratio = 0.25f + 0.50f * severity;
			maxOps = 4;
		}

		int budget = (int)ceilf((float)block.monsterCount * ratio);
		if (block.score >= 0.50f) budget = std::max(budget, 2);
		if (block.score >= 0.65f) budget = std::max(budget, 3);
		budget = std::max(1, budget);
		budget = std::min(budget, block.monsterCount);
		budget = std::min(budget, maxOps);
		return budget;
	};

	for (int levelIndex = 0; levelIndex < (int)(sizeof(levels) / sizeof(levels[0])); ++levelIndex)
	{
		OptimizationLevel const& level = levels[levelIndex];
		for (int roundIndex = 0; roundIndex < level.rounds; ++roundIndex)
		{
			ProblemStats roundBefore = getProblemStats(m_optimizedProblemScoreGrid);
			std::vector<BlockInfo> blocks = buildProblemBlocks(level);
			int roundSlot = std::min(roundIndex, 2);
			float topFraction = level.topFractions[roundSlot];
			int candidateCap = level.candidateCaps[roundSlot];
			int visitedBlocks = (int)ceilf((float)blocks.size() * topFraction);
			if (visitedBlocks < 1 && !blocks.empty()) visitedBlocks = 1;
			if (visitedBlocks > (int)blocks.size()) visitedBlocks = (int)blocks.size();
			if (visitedBlocks > candidateCap) visitedBlocks = candidateCap;

			std::set<int> allClusters;
			for (BlockInfo const& block : blocks)
			{
				if (block.clusterId >= 0)
				{
					allClusters.insert(block.clusterId);
				}
			}

			std::vector<int> selectedBlockIndices;
			selectedBlockIndices.reserve(visitedBlocks);
			std::vector<bool> selectedFlags(blocks.size(), false);
			std::set<int> coveredClusters;
			if (level.blockCells >= 8)
			{
				for (int blockIndex = 0; blockIndex < (int)blocks.size() && (int)selectedBlockIndices.size() < visitedBlocks; ++blockIndex)
				{
					int clusterId = blocks[blockIndex].clusterId;
					if (clusterId < 0 || coveredClusters.find(clusterId) != coveredClusters.end())
					{
						continue;
					}
					selectedBlockIndices.push_back(blockIndex);
					selectedFlags[blockIndex] = true;
					coveredClusters.insert(clusterId);
				}
			}
			for (int blockIndex = 0; blockIndex < (int)blocks.size() && (int)selectedBlockIndices.size() < visitedBlocks; ++blockIndex)
			{
				if (selectedFlags[blockIndex])
				{
					continue;
				}
				selectedBlockIndices.push_back(blockIndex);
				selectedFlags[blockIndex] = true;
				if (blocks[blockIndex].clusterId >= 0)
				{
					coveredClusters.insert(blocks[blockIndex].clusterId);
				}
			}
			visitedBlocks = (int)selectedBlockIndices.size();

			std::vector<CandidateOperation> candidates;
			candidates.reserve(candidateCap);
			std::vector<float> positiveRatios;
			std::map<int, int> plannedSourceUses;
			for (int selectedIndex : selectedBlockIndices)
			{
				BlockInfo const& block = blocks[selectedIndex];
				int blockBudget = getBlockOperationBudget(block, level);
				for (int budgetIndex = 0; budgetIndex < blockBudget && (int)candidates.size() < candidateCap; ++budgetIndex)
				{
					CandidateOperation candidate = makeCandidateForBlock(block, plannedSourceUses);
					if (candidate.sourceIdx >= 0)
					{
						plannedSourceUses[candidate.sourceIdx] += 1;
					}
					bool shouldStopBlock = candidate.rejectReason == "empty_source";
					candidates.push_back(candidate);
					if (candidate.rejectReason.empty() && candidate.improvementRatio > 0.f)
					{
						positiveRatios.push_back(candidate.improvementRatio);
					}
					if (shouldStopBlock)
					{
						break;
					}
				}
				if ((int)candidates.size() >= candidateCap)
				{
					break;
				}
			}

			std::sort(positiveRatios.begin(), positiveRatios.end());
			float p60 = GetPercentileFromSortedValues(positiveRatios, 0.60f);
			float thresholdFloor = level.blockCells == 1 ? 0.005f : 0.01f;
			float acceptThreshold = positiveRatios.size() >= 20 ? std::max(thresholdFloor, p60) : 0.f;

			std::sort(candidates.begin(), candidates.end(),
				[](CandidateOperation const& a, CandidateOperation const& b)
				{
					return a.improvementRatio > b.improvementRatio;
				});

			auto applyCandidatePass = [&](float passThreshold, int maxAcceptCount)->int
			{
				int accepted = 0;
				for (CandidateOperation& candidate : candidates)
				{
					if (candidate.rejectReason == "accepted")
					{
						candidate.rejectReason.clear();
					}
					if (!candidate.rejectReason.empty())
					{
						continue;
					}
					if (candidate.improvementRatio < passThreshold)
					{
						candidate.rejectReason = "below_threshold";
						continue;
					}
					if (maxAcceptCount >= 0 && accepted >= maxAcceptCount)
					{
						candidate.rejectReason = "below_threshold";
						continue;
					}
					if (applyCandidate(candidate))
					{
						++accepted;
						candidate.rejectReason = "accepted";
					}
					else
					{
						candidate.rejectReason = "empty_source";
					}
				}
				return accepted;
			};

			int acceptedCount = applyCandidatePass(acceptThreshold, -1);
			rebuildOptimizedEvaluation(nullptr);
			ProblemStats roundAfter = getProblemStats(m_optimizedProblemScoreGrid);

			int sampleCount = 0;
			for (CandidateOperation const& candidate : candidates)
			{
				if (sampleCount < 500)
				{
					int sourceCol = candidate.sourceIdx >= 0 ? candidate.sourceIdx % m_gridWidth : -1;
					int sourceRow = candidate.sourceIdx >= 0 ? candidate.sourceIdx / m_gridWidth : -1;
					int destCol = candidate.destIdx >= 0 ? candidate.destIdx % m_gridWidth : -1;
					int destRow = candidate.destIdx >= 0 ? candidate.destIdx / m_gridWidth : -1;
					candidateCsv += Stringf("%d,%d,%s,%d,%d,%d,%d,%s,%d,%s,%s,%.6f,%.6f,%.6f,%s\n",
						levelIndex,
						roundIndex,
						candidate.operationType.c_str(),
						sourceCol,
						sourceRow,
						destCol,
						destRow,
						candidate.dominantProblem.c_str(),
						candidate.clusterId,
						candidate.oldType.c_str(),
						candidate.newType.c_str(),
						candidate.oldPatchProblem,
						candidate.newPatchProblem,
						candidate.improvementRatio,
						candidate.rejectReason.c_str());
					++sampleCount;
				}
			}

			++totalOptimizationRounds;
			totalCandidateCount += (int)candidates.size();
			totalPositiveCandidateCount += (int)positiveRatios.size();
			totalAcceptedCount += acceptedCount;

			g_theDevConsole->AddLine(Stringf("OptimizedProblemScore L%d R%d: top %.2f cap %d blocks %d, clusters %d/%d, candidates %d, positive %d, accepted %d, threshold %.4f, avg %.4f -> %.4f",
				levelIndex, roundIndex, topFraction, candidateCap, visitedBlocks, (int)coveredClusters.size(), (int)allClusters.size(),
				(int)candidates.size(), (int)positiveRatios.size(), acceptedCount, acceptThreshold,
				roundBefore.avg, roundAfter.avg), Rgba8::WHITE);
		}
	}

	rebuildOptimizedEvaluation("Data/Debug/HeatmapAssessment/optimized_density_deviation_debug.csv");
	ProblemStats afterStats = getProblemStats(m_optimizedProblemScoreGrid);
	float optimizedSpecialRatio = (optimizedNormalCount + optimizedSpecialCount) > 0
		? (float)optimizedSpecialCount / (float)(optimizedNormalCount + optimizedSpecialCount)
		: 0.f;
	int changedCells = 0;
	for (int idx = 0; idx < totalCells; ++idx)
	{
		if (m_optimizedActualCountGrid[idx] != m_actualCountGrid[idx])
		{
			++changedCells;
		}
	}

	g_theDevConsole->AddLine(Stringf("OptimizedDistribution: total %d, changed cells %d", totalCurrentMonsters, changedCells), Rgba8::WHITE);
	g_theDevConsole->AddLine(Stringf("OptimizedDistribution type mix: normal %d, special %d (special %.2f%%)",
		optimizedNormalCount, optimizedSpecialCount, optimizedSpecialRatio * 100.f), Rgba8::WHITE);
	g_theDevConsole->AddLine(Stringf("OptimizedProblemScore: current avg %.4f p95 %.4f severe %d; initial optimized avg %.4f p95 %.4f; final avg %.4f p95 %.4f severe %d",
		beforeStats.avg, beforeStats.p95, beforeStats.severe, initialOptimizedStats.avg, initialOptimizedStats.p95, afterStats.avg, afterStats.p95, afterStats.severe), Rgba8::WHITE);

	ProblemDeltaStats finalDeltaStats = getProblemDeltaStats(currentProblemGrid, m_optimizedProblemScoreGrid);
	summaryCsv += Stringf("%.6f,%.6f,%d,%.6f,%.6f,%.6f,%.6f,%d,%.6f,%.6f,%d,%d,%d,%d,%d,%.6f,%.6f,%d,%d\n",
		beforeStats.avg,
		beforeStats.p95,
		beforeStats.severe,
		initialOptimizedStats.avg,
		initialOptimizedStats.p95,
		afterStats.avg,
		afterStats.p95,
		afterStats.severe,
		afterStats.avg - beforeStats.avg,
		afterStats.p95 - beforeStats.p95,
		afterStats.severe - beforeStats.severe,
		totalOptimizationRounds,
		totalCandidateCount,
		totalPositiveCandidateCount,
		totalAcceptedCount,
		finalDeltaStats.highAvgBefore,
		finalDeltaStats.highAvgAfter,
		finalDeltaStats.improvedCells,
		finalDeltaStats.worsenedCells);

	std::vector<int> coarseClusterGrid(totalCells, -1);
	std::vector<BlockInfo> coarseDebugBlocks = buildProblemBlocks(levels[0]);
	for (BlockInfo const& block : coarseDebugBlocks)
	{
		for (int row = block.minRow; row <= block.maxRow; ++row)
		{
			for (int col = block.minCol; col <= block.maxCol; ++col)
			{
				coarseClusterGrid[row * m_gridWidth + col] = block.clusterId;
			}
		}
	}

	std::string debugCsv;
	debugCsv.reserve((size_t)totalCells * 128);
	debugCsv += "col,row,cell_type,zone_id,current_problem_score,optimized_problem_score,problem_delta,improved_flag,worsened_flag,coarse_cluster_id,dominant_problem,old_count,optimized_count,expected_weight,optimized_avg_threat,density_problem,threat_problem,diversity_problem,encounter_problem\n";
	for (int row = 0; row < m_gridHeight; ++row)
	{
		for (int col = 0; col < m_gridWidth; ++col)
		{
			int idx = row * m_gridWidth + col;
			float optimizedAvgThreat = m_optimizedActualCountGrid[idx] > 0
				? optimizedThreatSumGrid[idx] / (float)m_optimizedActualCountGrid[idx]
				: 0.f;
			float problemDelta = m_optimizedProblemScoreGrid[idx] - currentProblemGrid[idx];
			debugCsv += Stringf("%d,%d,%s,%d,%.6f,%.6f,%.6f,%d,%d,%d,%s,%d,%d,%.6f,%.3f,%.6f,%.6f,%.6f,%.6f\n",
				col,
				row,
				GetCellTypeName(m_cellTypeGrid[idx]),
				m_cellZoneIdGrid[idx],
				currentProblemGrid[idx],
				m_optimizedProblemScoreGrid[idx],
				problemDelta,
				problemDelta < -0.000001f ? 1 : 0,
				problemDelta > 0.000001f ? 1 : 0,
				coarseClusterGrid[idx],
				getCellDominantProblem(idx).c_str(),
				m_actualCountGrid[idx],
				m_optimizedActualCountGrid[idx],
				m_expectedDensityGrid[idx],
				optimizedAvgThreat,
				Clamp01(fabsf(m_optimizedDensityDeviationGrid[idx])),
				Clamp01(fabsf(m_optimizedThreatAlignmentDeviationGrid[idx])),
				Clamp01(m_optimizedDiversityDeviationGrid[idx]),
				Clamp01(m_optimizedEncounterRiskGrid[idx]));
		}
	}

	if (FileWriteFromString(debugCsv, "Data/Debug/HeatmapAssessment/optimized_problem_score_debug.csv"))
	{
		g_theDevConsole->AddLine("OptimizedProblemScore debug CSV: Data/Debug/HeatmapAssessment/optimized_problem_score_debug.csv", Rgba8::WHITE);
	}
	else
	{
		g_theDevConsole->AddLine("OptimizedProblemScore debug CSV write failed", Rgba8::YELLOW);
	}
	if (FileWriteFromString(summaryCsv, "Data/Debug/HeatmapAssessment/optimized_iteration_summary.csv"))
	{
		g_theDevConsole->AddLine("Optimized iteration summary CSV: Data/Debug/HeatmapAssessment/optimized_iteration_summary.csv", Rgba8::WHITE);
	}
	if (FileWriteFromString(candidateCsv, "Data/Debug/HeatmapAssessment/optimized_candidate_samples.csv"))
	{
		g_theDevConsole->AddLine("Optimized candidate samples CSV: Data/Debug/HeatmapAssessment/optimized_candidate_samples.csv", Rgba8::WHITE);
	}
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
void HeatmapAssessmentMode::ExportAllHeatmapCsvImageData() const
{
	IntVec2 const dimensions(m_gridWidth, m_gridHeight);
	if (dimensions.x <= 0 || dimensions.y <= 0)
	{
		return;
	}

	int exportedCount = 0;
	char const* outputFolder = "Data/Debug/HeatmapAssessment/CsvImageData/";

	auto exportFloatGrid = [&](char const* fileName, std::vector<float> const& grid, char const* valueHeader)
	{
		if ((int)grid.size() != dimensions.x * dimensions.y)
		{
			g_theDevConsole->AddLine(Stringf("Heatmap CSV skipped %s: grid size mismatch", fileName), Rgba8::YELLOW);
			return;
		}

		TileHeatMap heatMap = MakeTileHeatMapFromFloatGrid(dimensions, grid);
		std::string path = std::string(outputFolder) + fileName;
		if (WriteTileHeatMapToCsvImageData(path, heatMap, valueHeader))
		{
			++exportedCount;
		}
		else
		{
			g_theDevConsole->AddLine(Stringf("Heatmap CSV write failed: %s", path.c_str()), Rgba8::YELLOW);
		}
	};

	auto exportIntGrid = [&](char const* fileName, std::vector<int> const& grid, char const* valueHeader)
	{
		if ((int)grid.size() != dimensions.x * dimensions.y)
		{
			g_theDevConsole->AddLine(Stringf("Heatmap CSV skipped %s: grid size mismatch", fileName), Rgba8::YELLOW);
			return;
		}

		TileHeatMap heatMap = MakeTileHeatMapFromIntGrid(dimensions, grid);
		std::string path = std::string(outputFolder) + fileName;
		if (WriteTileHeatMapToCsvImageData(path, heatMap, valueHeader))
		{
			++exportedCount;
		}
		else
		{
			g_theDevConsole->AddLine(Stringf("Heatmap CSV write failed: %s", path.c_str()), Rgba8::YELLOW);
		}
	};

	exportIntGrid("01_zone_reference.csv", m_cellZoneIdGrid, "zone_id");
	exportFloatGrid("02_height_map.csv", m_heightGrid, "height");
	exportFloatGrid("03_expected_density.csv", m_expectedDensityGrid, "expected_density");
	exportIntGrid("04_actual_density.csv", m_actualCountGrid, "monster_count");
	exportFloatGrid("05_density_deviation.csv", m_densityDeviationGrid, "density_deviation");
	exportFloatGrid("06_road_distance.csv", m_roadDistGrid, "road_distance_meters");
	exportFloatGrid("07_settlement_distance.csv", m_nearestSettlementDist, "settlement_distance_meters");
	exportFloatGrid("08_accessibility.csv", m_accessibilityGrid, "accessibility");
	exportFloatGrid("09_encounter_risk.csv", m_encounterRiskGrid, "encounter_risk");
	exportIntGrid("10_monster_type_count.csv", m_monsterTypeCountGrid, "monster_type_count");
	exportFloatGrid("11_diversity.csv", m_diversityGrid, "diversity");
	exportFloatGrid("12_low_diversity_deviation.csv", m_diversityAccessibilityDeviationGrid, "low_diversity_deviation");
	exportFloatGrid("13_threat_level.csv", m_threatGrid, "threat_level");
	exportFloatGrid("14_threat_alignment_deviation.csv", m_threatAlignmentDeviationGrid, "threat_alignment_deviation");
	exportFloatGrid("15_final_score.csv", m_finalScoreGrid, "final_score");
	exportFloatGrid("16_optimized_problem_score.csv", m_optimizedProblemScoreGrid, "optimized_problem_score");

	g_theDevConsole->AddLine(Stringf("Exported %d TileHeatMap CSV image data files to %s", exportedCount, outputFolder), Rgba8::WHITE);
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
		std::vector<float> positiveDiversityValues;
		positiveDiversityValues.reserve(m_gridWidth * m_gridHeight);
		for (float d : m_diversityGrid)
		{
			if (d > 0.f)
			{
				positiveDiversityValues.push_back(d);
			}
		}

		float displayLow = 0.f;
		float displayHigh = 1.f;
		if (!positiveDiversityValues.empty())
		{
			std::sort(positiveDiversityValues.begin(), positiveDiversityValues.end());
			displayLow = GetPercentileFromSortedValues(positiveDiversityValues, 0.10f);
			displayHigh = GetPercentileFromSortedValues(positiveDiversityValues, 0.95f);
			if (displayHigh < displayLow + 0.001f)
			{
				displayHigh = displayLow + 0.001f;
			}
		}

		std::vector<float> displayValues(m_gridWidth * m_gridHeight, 0.f);
		float const displaySmoothRadius = 60.f;
		float const displaySmoothRadiusSq = displaySmoothRadius * displaySmoothRadius;
		float const displaySmoothSigma = 30.f;
		int const displaySmoothCellRadius = (int)ceilf(displaySmoothRadius / m_cellWorldSize);

		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				int idx = row * m_gridWidth + col;
				float d = m_diversityGrid[row * m_gridWidth + col];
				// Display uses a contrast stretch over the occupied local-diversity range.
				// The theoretical max, log2(9), is rarely reached, so direct normalization
				// makes most real map values collapse into the same middle gray.
				float t = (d - displayLow) / (displayHigh - displayLow);
				t = Clamp01(t);
				// Fade low-sample edge cells after contrast stretching. Without this, a cell
				// barely inside the 240m sampling tail can become white while its neighbor just
				// outside the tail is black, creating a false hard border.
				t *= Clamp01(m_diversityConfidenceGrid[row * m_gridWidth + col]);
				displayValues[idx] = t;
			}
		}

		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float weightedDisplay = 0.f;
				float totalWeight = 0.f;

				int minRow = row - displaySmoothCellRadius;
				int maxRow = row + displaySmoothCellRadius;
				int minCol = col - displaySmoothCellRadius;
				int maxCol = col + displaySmoothCellRadius;
				if (minRow < 0) minRow = 0;
				if (minCol < 0) minCol = 0;
				if (maxRow >= m_gridHeight) maxRow = m_gridHeight - 1;
				if (maxCol >= m_gridWidth)  maxCol = m_gridWidth - 1;

				for (int sampleRow = minRow; sampleRow <= maxRow; ++sampleRow)
				{
					for (int sampleCol = minCol; sampleCol <= maxCol; ++sampleCol)
					{
						float dx = (float)(sampleCol - col) * m_cellWorldSize;
						float dy = (float)(sampleRow - row) * m_cellWorldSize;
						float distSq = dx * dx + dy * dy;
						if (distSq > displaySmoothRadiusSq) continue;

						float sigmaRatioSq = distSq / (displaySmoothSigma * displaySmoothSigma);
						float weight = expf(-0.5f * sigmaRatioSq);
						weightedDisplay += displayValues[sampleRow * m_gridWidth + sampleCol] * weight;
						totalWeight += weight;
					}
				}

				float t = totalWeight > 0.f ? weightedDisplay / totalWeight : 0.f;

				Rgba8 color = InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 255, 255, 255), t);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::MONSTER_TYPE_COUNT)
	{
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				int typeCount = m_monsterTypeCountGrid[row * m_gridWidth + col];
				float t = (float)typeCount / 9.f;
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
		// Raw field map: white = near road, black = 1000m+ from road.
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float d = m_roadDistGrid[row * m_gridWidth + col];
				float t = 1.f - (d / 1000.f);
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

	if (m_currentHeatmap == HeatmapType::SETTLEMENT_DISTANCE)
	{
		// Raw field map: white = near settlement, black = 1000m+ from settlement.
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float d = m_nearestSettlementDist[row * m_gridWidth + col];
				float t = 1.f - (d / 1000.f);
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

	if (m_currentHeatmap == HeatmapType::DENSITY_DEVIATION)
	{
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				int idx = row * m_gridWidth + col;
				float d = m_densityDeviationGrid[idx];
				Rgba8 color = ShouldScoreDensityDeviationCell(m_cellTypeGrid[idx])
					? SignedDeviationToColor(d)
					: DENSITY_DEVIATION_NOT_SCORED_COLOR;

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::ENCOUNTER_RISK)
	{
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float t = Clamp01(m_encounterRiskGrid[row * m_gridWidth + col]);
				Rgba8 color = InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 0, 0, 255), t);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::DIVERSITY_ACCESSIBILITY_DEVIATION)
	{
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float t = Clamp01(m_diversityAccessibilityDeviationGrid[row * m_gridWidth + col]);
				Rgba8 color = InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 0, 0, 255), t);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::THREAT_ALIGNMENT_DEVIATION)
	{
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float d = m_threatAlignmentDeviationGrid[row * m_gridWidth + col];
				Rgba8 color = SignedDeviationToColor(d);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::FINAL_SCORE)
	{
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float score = Clamp01(m_finalScoreGrid[row * m_gridWidth + col]);
				float problem = 1.f - score;
				Rgba8 color = InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 0, 0, 255), problem);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::OPTIMIZED_PROBLEM_SCORE)
	{
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float problemScore = Clamp01(m_optimizedProblemScoreGrid[row * m_gridWidth + col]);
				Rgba8 color = InterpolateRGBA(Rgba8(0, 0, 0, 255), Rgba8(255, 0, 0, 255), problemScore);

				float screenX = m_mapOffsetX + (float)col;
				float screenY = (float)m_gridHeight - (float)row - 1.f;
				AABB2 cellBounds(Vec2(screenX, screenY), Vec2(screenX + 1.f, screenY + 1.f));
				AddVertsForAABB2D(m_heatmapVerts, cellBounds, color);
			}
		}
	}

	if (m_currentHeatmap == HeatmapType::ACCESSIBILITY)
	{
		m_heatmapVerts.reserve(m_gridWidth * m_gridHeight * 6);
		for (int row = 0; row < m_gridHeight; ++row)
		{
			for (int col = 0; col < m_gridWidth; ++col)
			{
				float t = m_accessibilityGrid[row * m_gridWidth + col];
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
	case HeatmapType::ROAD_DISTANCE:    return "RoadDistance";
	case HeatmapType::SETTLEMENT_DISTANCE: return "SettlementDistance";
	case HeatmapType::ACCESSIBILITY:    return "Accessibility";
	case HeatmapType::ENCOUNTER_RISK: return "EncounterRisk";
	case HeatmapType::EXPECTED_DENSITY: return "ExpectedDensity";
	case HeatmapType::ACTUAL_DENSITY:   return "ActualDensity";
	case HeatmapType::DENSITY_DEVIATION: return "DensityDeviation";
	case HeatmapType::HEIGHT:           return "HeightMap";
	case HeatmapType::MONSTER_TYPE_COUNT: return "MonsterTypeCount";
	case HeatmapType::DIVERSITY:        return "Diversity";
	case HeatmapType::DIVERSITY_ACCESSIBILITY_DEVIATION: return "LowDiversityDeviation";
	case HeatmapType::THREAT_LEVEL:     return "ThreatLevel";
	case HeatmapType::THREAT_ALIGNMENT_DEVIATION: return "ThreatAlignmentDeviation";
	case HeatmapType::FINAL_SCORE:      return "FinalScore";
	case HeatmapType::OPTIMIZED_PROBLEM_SCORE: return "OptimizedProblemScore";
	default:                            return "Unknown";
	}
}

//-----------------------------------------------------------------------------------------------
char const* HeatmapAssessmentMode::GetHeatmapCategory() const
{
	switch (m_currentHeatmap)
	{
	case HeatmapType::ROAD_DISTANCE:
	case HeatmapType::SETTLEMENT_DISTANCE:
	case HeatmapType::ACCESSIBILITY:
	case HeatmapType::ENCOUNTER_RISK:
		return "Player Reachability";
	case HeatmapType::ZONE_REFERENCE:
	case HeatmapType::HEIGHT:
	case HeatmapType::EXPECTED_DENSITY:
	case HeatmapType::ACTUAL_DENSITY:
	case HeatmapType::DENSITY_DEVIATION:
		return "Density Placement";
	case HeatmapType::MONSTER_TYPE_COUNT:
	case HeatmapType::DIVERSITY:
	case HeatmapType::DIVERSITY_ACCESSIBILITY_DEVIATION:
		return "Encounter Variety";
	case HeatmapType::THREAT_LEVEL:
	case HeatmapType::THREAT_ALIGNMENT_DEVIATION:
		return "Threat Pacing";
	case HeatmapType::FINAL_SCORE:
	case HeatmapType::OPTIMIZED_PROBLEM_SCORE:
		return "Composite";
	default:
		return "Unknown Category";
	}
}

//-----------------------------------------------------------------------------------------------
char const* HeatmapAssessmentMode::GetHeatmapDescription() const
{
	switch (m_currentHeatmap)
	{
	case HeatmapType::ROAD_DISTANCE:
		return "Show distance from each cell to the nearest road.";
	case HeatmapType::SETTLEMENT_DISTANCE:
		return "Show distance from each cell to the nearest survivor settlement.";
	case HeatmapType::ACCESSIBILITY:
		return "Estimate how easily players can reach each area from roads or settlements.";
	case HeatmapType::ENCOUNTER_RISK:
		return "Estimate likely player-monster encounter frequency from accessibility and local monster density.";
	case HeatmapType::ZONE_REFERENCE:
		return "Show the base terrain and zone type for each cell.";
	case HeatmapType::HEIGHT:
		return "Show terrain height estimated for each cell.";
	case HeatmapType::EXPECTED_DENSITY:
		return "Estimate where monster density should be high or low from zone, road, settlement, and height context.";
	case HeatmapType::ACTUAL_DENSITY:
		return "Show current monster count in each cell.";
	case HeatmapType::DENSITY_DEVIATION:
		return "Evaluate where current monster density is too high or too low compared with expectation.";
	case HeatmapType::MONSTER_TYPE_COUNT:
		return "Show how many distinct monster types appear in each cell.";
	case HeatmapType::DIVERSITY:
		return "Measure local monster variety and balance with Shannon entropy.";
	case HeatmapType::DIVERSITY_ACCESSIBILITY_DEVIATION:
		return "Highlight occupied areas where local monster variety is too low.";
	case HeatmapType::THREAT_LEVEL:
		return "Show current average monster threat level around each cell.";
	case HeatmapType::THREAT_ALIGNMENT_DEVIATION:
		return "Evaluate whether actual monster threat is too high or too low for the zone base threat.";
	case HeatmapType::FINAL_SCORE:
		return "Combine density, threat, diversity, and encounter risk into one quality score.";
	case HeatmapType::OPTIMIZED_PROBLEM_SCORE:
		return "Preview remaining ProblemScore after multi-scale distribution optimization.";
	default:
		return "";
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
		entries.push_back({Rgba8(255, 255, 255, 255), "0 cells / 0m"});
		entries.push_back({Rgba8(204, 204, 204, 255), "10 cells / 200m"});
		entries.push_back({Rgba8(128, 128, 128, 255), "25 cells / 500m"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "50+ cells / 1000m+"});
		break;
	case HeatmapType::SETTLEMENT_DISTANCE:
		entries.push_back({Rgba8(255, 255, 255, 255), "0 cells / 0m"});
		entries.push_back({Rgba8(204, 204, 204, 255), "10 cells / 200m"});
		entries.push_back({Rgba8(128, 128, 128, 255), "25 cells / 500m"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "50+ cells / 1000m+"});
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
	case HeatmapType::DENSITY_DEVIATION:
		entries.push_back({Rgba8(255,   0,   0, 255), "Too many monsters"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Near expected"});
		entries.push_back({Rgba8(  0,  80, 255, 255), "Too few monsters"});
		entries.push_back({DENSITY_DEVIATION_NOT_SCORED_COLOR, "Water / not scored"});
		break;
	case HeatmapType::ENCOUNTER_RISK:
		entries.push_back({Rgba8(255,   0,   0, 255), "High encounter risk"});
		entries.push_back({Rgba8(128,   0,   0, 255), "Medium encounter risk"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Low / no local monsters"});
		break;
	case HeatmapType::THREAT_LEVEL:
		entries.push_back({Rgba8(255, 255, 255, 255), "Threat 5 (M09 Boss)"});
		entries.push_back({Rgba8(190, 190, 190, 255), "Threat 4 (M05/M06 Elite)"});
		entries.push_back({Rgba8(128, 128, 128, 255), "Threat 3 (M02/M03 Special)"});
		entries.push_back({Rgba8( 64,  64,  64, 255), "Threat 2 (M04 Runner)"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Threat 1 (M01 Wanderer)"});
		break;
	case HeatmapType::DIVERSITY:
		entries.push_back({Rgba8(255, 255, 255, 255), "High local diversity"});
		entries.push_back({Rgba8(128, 128, 128, 255), "Medium local diversity"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Low / no local variety"});
		break;
	case HeatmapType::DIVERSITY_ACCESSIBILITY_DEVIATION:
		entries.push_back({Rgba8(255,   0,   0, 255), "Too little variety"});
		entries.push_back({Rgba8(128,   0,   0, 255), "Medium low-diversity issue"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Acceptable / no evidence"});
		break;
	case HeatmapType::MONSTER_TYPE_COUNT:
		entries.push_back({Rgba8(255, 255, 255, 255), "9 distinct types"});
		entries.push_back({Rgba8(128, 128, 128, 255), "4-5 distinct types"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "0 distinct types"});
		break;
	case HeatmapType::ACCESSIBILITY:
		entries.push_back({Rgba8(255, 255, 255, 255), "High access (road/settlement nearby)"});
		entries.push_back({Rgba8(128, 128, 128, 255), "Medium access"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Remote / low access"});
		break;
	case HeatmapType::THREAT_ALIGNMENT_DEVIATION:
		entries.push_back({Rgba8(255,   0,   0, 255), "Too hard for zone"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Fits zone threat"});
		entries.push_back({Rgba8(  0,  80, 255, 255), "Too weak for zone"});
		break;
	case HeatmapType::FINAL_SCORE:
		entries.push_back({Rgba8(255,   0,   0, 255), "Low score / many problems"});
		entries.push_back({Rgba8(128,   0,   0, 255), "Medium concern"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "High score / acceptable"});
		break;
	case HeatmapType::OPTIMIZED_PROBLEM_SCORE:
		entries.push_back({Rgba8(255,   0,   0, 255), "Remaining severe problem"});
		entries.push_back({Rgba8(128,   0,   0, 255), "Remaining medium problem"});
		entries.push_back({Rgba8(  0,   0,   0, 255), "Optimized / acceptable"});
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
// Renders category -> map hierarchy at the left-middle side of the screen.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::RenderCategoryPanel() const
{
	struct MapPanelEntry
	{
		char const* category;
		HeatmapType type;
		char const* name;
		char const* description;
	};

	static MapPanelEntry const entries[] = {
		{"Density Placement",   HeatmapType::ZONE_REFERENCE,       "ZoneReference",       "Show the base terrain and zone type for each cell."},
		{"Density Placement",   HeatmapType::HEIGHT,               "HeightMap",           "Show terrain height estimated for each cell."},
		{"Density Placement",   HeatmapType::EXPECTED_DENSITY,     "ExpectedDensity",     "Estimate where monster density should be high or low from zone, road, settlement, and height context."},
		{"Density Placement",   HeatmapType::ACTUAL_DENSITY,       "ActualDensity",       "Show current monster count in each cell."},
		{"Density Placement",   HeatmapType::DENSITY_DEVIATION,    "DensityDeviation",    "Evaluate where current monster density is too high or too low compared with expectation."},

		{"Player Reachability", HeatmapType::ROAD_DISTANCE,        "RoadDistance",        "Show distance from each cell to the nearest road."},
		{"Player Reachability", HeatmapType::SETTLEMENT_DISTANCE,  "SettlementDistance",  "Show distance from each cell to the nearest survivor settlement."},
		{"Player Reachability", HeatmapType::ACCESSIBILITY,        "Accessibility",       "Estimate how easily players can reach each area from roads or settlements."},
		{"Player Reachability", HeatmapType::ENCOUNTER_RISK, "EncounterRisk", "Estimate likely player-monster encounter frequency from accessibility and local monster density."},

		{"Encounter Variety",   HeatmapType::MONSTER_TYPE_COUNT,   "MonsterTypeCount",    "Show how many distinct monster types appear in each cell."},
		{"Encounter Variety",   HeatmapType::DIVERSITY,            "Diversity",           "Measure local monster variety and balance with Shannon entropy."},
		{"Encounter Variety",   HeatmapType::DIVERSITY_ACCESSIBILITY_DEVIATION, "LowDiversityDeviation", "Highlight occupied areas where local monster variety is too low."},

		{"Threat Pacing",       HeatmapType::THREAT_LEVEL,         "ThreatLevel",         "Show current average monster threat level around each cell."},
		{"Threat Pacing",       HeatmapType::THREAT_ALIGNMENT_DEVIATION, "ThreatAlignmentDeviation", "Evaluate whether actual monster threat matches the zone base threat level."},

		{"Composite",           HeatmapType::FINAL_SCORE,          "FinalScore",          "Combine all problem maps into one quality score; lower scores render redder."},
		{"Composite",           HeatmapType::OPTIMIZED_PROBLEM_SCORE, "OptimizedProblemScore", "Show the optimized composite problem score after the multi-scale distribution optimizer."},
	};

	AABB2 screenBounds = m_screenCamera.GetCameraBounds();
	float screenWidth = screenBounds.m_maxs.x - screenBounds.m_mins.x;
	float screenHeight = screenBounds.m_maxs.y - screenBounds.m_mins.y;
	float baseLineHeight = screenHeight / m_gameModeConfig.m_numMessageOnScreen;
	float mapFontHeight = m_gameModeConfig.m_lineHeightAndTextBoxRatio * baseLineHeight * 0.82f;
	float categoryFontHeight = mapFontHeight * 1.28f;
	float descFontHeight = mapFontHeight * 0.72f;

	float const left = 12.f;
	float const right = screenWidth * 0.30f;
	float const categoryGap = baseLineHeight * 0.28f;
	float const mapLineHeight = baseLineHeight * 0.72f;
	float const descLineHeight = baseLineHeight * 1.12f;
	float const categoryTextWidth = right - left;
	float const mapTextLeft = left + 18.f;
	float const descTextLeft = left + 38.f;
	float const mapTextWidth = right - mapTextLeft;
	float const descTextWidth = right - descTextLeft;

	float totalHeight = 0.f;
	char const* heightLastCategory = "";
	for (MapPanelEntry const& entry : entries)
	{
		if (strcmp(heightLastCategory, entry.category) != 0)
		{
			std::string wrappedCategory = WrapTextToWidth(m_gameModeConfig.m_font, entry.category, categoryTextWidth, categoryFontHeight, m_gameModeConfig.m_cellAspect);
			totalHeight += (float)CountTextLines(wrappedCategory) * baseLineHeight + categoryGap;
			heightLastCategory = entry.category;
		}

		std::string mapLineForHeight = std::string(entry.type == m_currentHeatmap ? "> " : "  ") + entry.name;
		std::string wrappedMapLine = WrapTextToWidth(m_gameModeConfig.m_font, mapLineForHeight, mapTextWidth, mapFontHeight, m_gameModeConfig.m_cellAspect);
		totalHeight += (float)CountTextLines(wrappedMapLine) * mapLineHeight;

		if (entry.type == m_currentHeatmap)
		{
			std::string wrappedDescription = WrapTextToWidth(m_gameModeConfig.m_font, entry.description, descTextWidth, descFontHeight, m_gameModeConfig.m_cellAspect);
			totalHeight += (float)CountTextLines(wrappedDescription) * descLineHeight;
		}
	}

	float y = screenHeight * 0.5f + totalHeight * 0.5f;

	std::vector<Vertex_PCU> textVerts;
	char const* lastCategory = "";
	for (MapPanelEntry const& entry : entries)
	{
		if (strcmp(lastCategory, entry.category) != 0)
		{
			std::string wrappedCategory = WrapTextToWidth(m_gameModeConfig.m_font, entry.category, categoryTextWidth, categoryFontHeight, m_gameModeConfig.m_cellAspect);
			float categoryBoxHeight = (float)CountTextLines(wrappedCategory) * baseLineHeight;
			y -= categoryBoxHeight;
			AABB2 categoryBox(Vec2(left, y), Vec2(right, y + categoryBoxHeight));
			m_gameModeConfig.m_font->AddVertsForTextInBox2D(
				textVerts, wrappedCategory, categoryBox, categoryFontHeight,
				Vec2(0.f, 0.5f), Rgba8(235, 220, 160, 255), 0.7f,
				m_gameModeConfig.m_cellAspect, TextDrawMode::OVERRUN
			);
			y -= categoryGap;
			lastCategory = entry.category;
		}

		bool isSelected = (entry.type == m_currentHeatmap);
		Rgba8 mapColor = isSelected ? Rgba8::WHITE : Rgba8(150, 150, 150, 255);
		char const* prefix = isSelected ? "> " : "  ";
		std::string mapLine = std::string(prefix) + entry.name;
		std::string wrappedMapLine = WrapTextToWidth(m_gameModeConfig.m_font, mapLine, mapTextWidth, mapFontHeight, m_gameModeConfig.m_cellAspect);

		float mapBoxHeight = (float)CountTextLines(wrappedMapLine) * mapLineHeight;
		y -= mapBoxHeight;
		AABB2 mapBox(Vec2(mapTextLeft, y), Vec2(right, y + mapBoxHeight));
		m_gameModeConfig.m_font->AddVertsForTextInBox2D(
			textVerts, wrappedMapLine, mapBox, mapFontHeight,
			Vec2(0.f, 0.5f), mapColor, 0.7f,
			m_gameModeConfig.m_cellAspect, TextDrawMode::OVERRUN
		);

		if (isSelected)
		{
			std::string wrappedDescription = WrapTextToWidth(m_gameModeConfig.m_font, entry.description, descTextWidth, descFontHeight, m_gameModeConfig.m_cellAspect);
			float descBoxHeight = (float)CountTextLines(wrappedDescription) * descLineHeight;
			y -= descBoxHeight;
			AABB2 descBox(Vec2(descTextLeft, y), Vec2(right, y + descBoxHeight));
			m_gameModeConfig.m_font->AddVertsForTextInBox2D(
				textVerts, wrappedDescription, descBox, descFontHeight,
				Vec2(0.f, 0.5f), Rgba8(210, 210, 210, 255), 0.6f,
				m_gameModeConfig.m_cellAspect, TextDrawMode::OVERRUN
			);
		}
	}

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(&m_gameModeConfig.m_font->GetTexture());
	g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
}

//-----------------------------------------------------------------------------------------------
// Handles N/M key input to cycle through heatmap types.
// M advances through the visible category order; N steps backward.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	bool changed = false;
	int count = 0;
	HeatmapType const* order = GetVisibleHeatmapOrder(count);
	int current = GetVisibleHeatmapIndex(m_currentHeatmap);

	if (g_theInput->WasKeyJustPressed('N'))
	{
		current = (current - 1 + count) % count;
		changed = true;
	}
	if (g_theInput->WasKeyJustPressed('M'))
	{
		current = (current + 1) % count;
		changed = true;
	}

	if (changed)
	{
		m_currentHeatmap = order[current];
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
	RenderCategoryPanel();
	g_theRenderer->EndCamera(m_screenCamera);
}

//-----------------------------------------------------------------------------------------------
// Renders only the heatmap cycling control hint in the upper-left corner.
//-----------------------------------------------------------------------------------------------
void HeatmapAssessmentMode::RenderScreenMessage() const
{
	AABB2 bounds = m_screenCamera.GetCameraBounds();
	float screenHeight = bounds.m_maxs.y - bounds.m_mins.y;
	float textLineHeight = screenHeight / m_gameModeConfig.m_numMessageOnScreen;
	float fontHeight = m_gameModeConfig.m_lineHeightAndTextBoxRatio * textLineHeight;

	std::vector<Vertex_PCU> messageVerts;
	AABB2 lineBox(Vec2(bounds.m_mins.x, bounds.m_maxs.y - textLineHeight), Vec2(bounds.m_maxs.x, bounds.m_maxs.y));
	m_gameModeConfig.m_font->AddVertsForTextInBox2D(messageVerts, m_controlInstruction, lineBox, fontHeight, Vec2(0.f, 0.5f),
		m_instructionLineColor, 0.25f, m_gameModeConfig.m_cellAspect, TextDrawMode::OVERRUN);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	m_gameModeConfig.m_renderer->BindTexture(&m_gameModeConfig.m_font->GetTexture());
	m_gameModeConfig.m_renderer->DrawVertexArray((int)messageVerts.size(), messageVerts.data());
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
	m_controlInstruction = "N: Previous Heatmap | M: Next Heatmap";
	int visibleCount = 0;
	GetVisibleHeatmapOrder(visibleCount);
	int visibleIndex = GetVisibleHeatmapIndex(m_currentHeatmap);
	m_testString = Stringf("[%d/%d] %s  |  Monsters: %d  |  Zones: %d  |  Settlements: %d",
		visibleIndex + 1, visibleCount, GetHeatmapName(),
		m_monsterPoints.GetRowCount(),
		m_zoneReference.GetRowCount(),
		m_settlements.GetRowCount());
}
