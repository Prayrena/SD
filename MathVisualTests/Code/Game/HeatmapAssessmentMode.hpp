#pragma once
#include "Game/GameMode.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/core/CsvUtils.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include <vector>
#include <map>
#include <string>

class Texture;
class Image;

// ----- Heatmap constants -----
static constexpr int   HEATMAP_PIXELS_PER_CELL   = 5;    // how many image pixels make up one grid cell; lower = finer grid
static constexpr float HEATMAP_METERS_PER_PIXEL   = 4.f;  // world coordinate scale (1 pixel = 4 meters)

// ----- Cell type classification from road_map.png -----
enum class CellType : unsigned char
{
	TERRAIN = 0,
	WATER,
	SETTLEMENT,
	ROAD
};

// ----- Heatmap types -----
// First entry is the raw zone reference map (no overlay).
// Category A: grayscale state maps (show facts)
// Category B: white-to-red deviation maps (show problems)
// Category C: composite conclusion
enum class HeatmapType
{
	ZONE_REFERENCE = 0, // Raw zone_heatmap.png — starting view
	// Category A: Grayscale state maps
	ROAD_DISTANCE,
	HEIGHT,
	EXPECTED_DENSITY,
	ACTUAL_DENSITY,
	THREAT_LEVEL,
	DIVERSITY,
	// ACTUAL_DENSITY,
	// THREAT_LEVEL,
	// DIVERSITY,
	// ACCESSIBILITY,
	// Category B: White->Red deviation maps
	// DENSITY_DEVIATION,
	// THREAT_ALIGNMENT_DEVIATION,
	// TYPE_RATIO_DEVIATION,
	// DIVERSITY_ACCESSIBILITY_DEVIATION,
	// ELEVATION_SMOOTHNESS_DEVIATION,
	// Category C: Conclusion
	// FINAL_BADNESS_SCORE,
	COUNT
};

class HeatmapAssessmentMode : public GameMode
{
public:
	HeatmapAssessmentMode();
	~HeatmapAssessmentMode();

	void Startup() override;
	void Update(float deltaSeconds) override;
	void Render() const override;
	void Shutdown() override;

	void UpdateModeInfo() override;

private:
	// ----- Data loading -----
	void LoadCSVData();
	void LoadMapTextures();
	void LoadImageData();

	// ----- Preprocessing -----
	void BuildZoneColorLookup();
	void CalibrateZoneElevation();
	int  FindNearestZoneId(Rgba8 const& color) const;
	Rgba8 SampleZoneImageAtWorldPos(float worldX, float worldY) const;
	Rgba8 SampleRoadImageAtGridCell(int col, int row) const;

	// ----- Threat level computation -----
	void BuildThreatGrid();

	// ----- Diversity computation -----
	void BuildDiversityGrid();           // Shannon entropy of monster types per cell              // per-cell avg threat from monster data + relaxation fill

	// ----- Expected density computation -----
	void BuildRoadDistanceField();       // BFS from road cells -> meters to nearest road
	void ComputeSettlementDistances(); // Euclidean distance from settlements.csv positions
	void BuildExpectedDensityGrid();     // zone_base × road_mod × elev_mod × settlement_mod
	float GetZoneBaseDensity(int zoneId) const;       // from zone_category per spawn_design_notes.md
	float GetRoadModifier(float distMeters) const;    // step function from design doc
	float GetSettlementModifier(float distMeters) const; // step function from design doc

	// ----- Height grid building (multi-pass) -----
	void BuildCellTypeGrid();        // classify cells as terrain/water/settlement/road from road_map.png
	void BuildHeightGrid();          // pass 1: per-cell monster heights
	void FillTerrainHeights();       // pass 2: all empty non-road/non-water cells from zone average
	void FillRoadHeights();          // pass 3: road cells via Laplacian relaxation
	void FillWaterHeights();         // pass 4: water cells via Laplacian relaxation
	void SmoothTerrainHeights();     // pass 5: Laplacian relaxation on all cells except road and water

	// ----- Heatmap building -----
	void BuildHeatmapVerts();
	void BuildBackgroundVerts();

	// ----- Display info -----
	char const* GetHeatmapName() const;

	// ----- Legend -----
	struct LegendEntry
	{
		Rgba8       color;
		std::string label;
	};
	std::vector<LegendEntry> GetLegendEntries() const;
	void RenderLegend() const;

	// ----- Utility -----
	int CountMissingCells() const;

	// ----- CSV data -----
	CsvDocument m_monsterPoints;
	CsvDocument m_monsterReference;
	CsvDocument m_zoneReference;
	CsvDocument m_settlements;

	// ----- Map textures (for rendering) -----
	Texture* m_zoneHeatmapTexture = nullptr;
	Texture* m_roadMapTexture     = nullptr;

	// ----- Map images (for pixel sampling) -----
	Image* m_zoneHeatmapImage = nullptr;
	Image* m_roadMapImage     = nullptr;

	// ----- Zone lookup -----
	struct ZoneColorEntry
	{
		int zoneId   = -1;
		unsigned char r = 0;
		unsigned char g = 0;
		unsigned char b = 0;
	};
	std::vector<ZoneColorEntry> m_zoneColors;

	// ----- Zone metadata -----
	std::map<int, std::string> m_zoneCategory;  // zone_id -> zone_category (荒野/功能区/据点)

	// ----- Zone elevation calibration -----
	std::map<int, float> m_zoneAvgHeight;
	std::map<int, float> m_zoneElevationMod;

	// ----- Grid dimensions (derived from image size and sample rate) -----
	int   m_gridWidth  = 0;
	int   m_gridHeight = 0;
	float m_cellWorldSize = 0.f;

	// ----- Grid data -----
	std::vector<CellType> m_cellTypeGrid;
	std::vector<float>    m_heightGrid;          // -1 = missing
	std::vector<float>    m_roadDistGrid;        // distance in meters to nearest road per cell
	std::vector<float>    m_nearestSettlementDist;  // distance in meters to nearest settlement per cell
	std::vector<float>    m_expectedDensityGrid; // design formula result per cell
	std::vector<int>      m_actualCountGrid;     // monster count per cell
	std::vector<float>    m_threatGrid;          // avg threat_level per cell, -1 = missing
	std::vector<float>    m_diversityGrid;       // Shannon entropy per cell, -1 = no monsters
	std::vector<int>      m_cellZoneIdGrid;      // zone_id per cell (cached for reuse)

	// ----- Layout -----
	float m_mapOffsetX = 0.f;

	// ----- Rendering -----
	HeatmapType m_currentHeatmap = HeatmapType::ZONE_REFERENCE;
	std::vector<Vertex_PCU> m_backgroundVerts;
	std::vector<Vertex_PCU> m_heatmapVerts;
};
