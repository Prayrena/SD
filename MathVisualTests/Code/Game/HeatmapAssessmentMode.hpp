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
	// Density Placement
	ZONE_REFERENCE = 0,
	HEIGHT,
	EXPECTED_DENSITY,
	ACTUAL_DENSITY,
	DENSITY_DEVIATION,

	// Player Reachability
	ROAD_DISTANCE,
	SETTLEMENT_DISTANCE,
	ACCESSIBILITY,
	ENCOUNTER_RISK,

	// Encounter Variety
	MONSTER_TYPE_COUNT,
	DIVERSITY,
	DIVERSITY_ACCESSIBILITY_DEVIATION,

	// Threat Pacing
	THREAT_LEVEL,
	THREAT_ALIGNMENT_DEVIATION,

	// Composite conclusion
	FINAL_SCORE,
	OPTIMIZED_PROBLEM_SCORE,
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
	void RenderScreenMessage() const override;

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
	void BuildDiversityGrid();           // Raw per-cell entropy + smoothed local encounter diversity

	// ----- Expected density computation -----
	void BuildRoadDistanceField();       // BFS from road cells -> meters to nearest road
	void ComputeSettlementDistances(); // Euclidean distance from settlements.csv positions
	void BuildExpectedDensityGrid();     // zone_base × road_mod × elev_mod × settlement_mod
	void BuildDensityDeviationGrid();    // signed density mismatch: blue=too few, red=too many
	void BuildAccessibilityGrid();       // player reachability from roads and settlements
	void BuildEncounterRiskGrid();       // exposure risk: accessible areas with local monster pressure
	void BuildDiversityAccessibilityDeviationGrid(); // low diversity problem score
	void BuildThreatAlignmentDeviationGrid(); // signed actual threat vs zone base threat mismatch
	void BuildFinalScoreGrid();          // [0,1], lower score = more severe combined problem
	void BuildOptimizedDistributionGrids(); // virtual optimized distribution + optimized problem score
	void BuildDensityDeviationGridForCounts(std::vector<int> const& countGrid, std::vector<float>& outDeviationGrid, char const* debugLabel, char const* debugCsvPath);
	void BuildEncounterRiskGridForCounts(std::vector<int> const& countGrid, std::vector<float>& outRiskGrid, char const* debugLabel);
	void BuildThreatAlignmentDeviationGridForThreatSums(std::vector<int> const& countGrid, std::vector<float> const& threatSumGrid, std::vector<float>& outDeviationGrid, char const* debugLabel);
	void BuildLowDiversityDeviationGridForTypeCounts(std::vector<int> const& countGrid, std::vector<std::map<std::string, int>> const& typeCountGrid, std::vector<float>& outDeviationGrid, char const* debugLabel);
	float GetZoneBaseDensity(int zoneId) const;       // from zone_category per spawn_design_notes.md
	float GetRoadModifier(float distMeters) const;    // step function from design doc
	float GetSettlementModifier(float distMeters) const; // step function from design doc
	float GetZoneRiskJustification(int zoneId) const;

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
	char const* GetHeatmapCategory() const;
	char const* GetHeatmapDescription() const;

	// ----- Legend -----
	struct LegendEntry
	{
		Rgba8       color;
		std::string label;
	};
	std::vector<LegendEntry> GetLegendEntries() const;
	void RenderLegend() const;
	void RenderCategoryPanel() const;

	// ----- Utility -----
	int CountMissingCells() const;
	void ExportAllHeatmapCsvImageData() const;

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
	std::map<int, float>       m_zoneExpectedThreat; // zone_id -> expected threat center from base_threat_level

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
	std::vector<float>    m_densityDeviationGrid; // signed [-1,1], blue=too few, red=too many
	std::vector<float>    m_encounterRiskGrid; // [0,1], high = accessible local monster pressure
	std::vector<float>    m_diversityAccessibilityDeviationGrid; // [0,1], red=too little local variety
	std::vector<float>    m_threatAlignmentDeviationGrid; // signed [-1,1], red=too hard, blue=too weak for zone threat
	std::vector<float>    m_finalScoreGrid; // [0,1], 1=healthy, 0=worst combined problem
	std::vector<int>      m_optimizedActualCountGrid; // optimized monster count per cell after applying design weights
	std::vector<float>    m_optimizedDensityDeviationGrid; // optimized signed density mismatch
	std::vector<float>    m_optimizedEncounterRiskGrid; // optimized accessibility x local density pressure
	std::vector<float>    m_optimizedDiversityDeviationGrid; // optimized low-diversity problem
	std::vector<float>    m_optimizedThreatAlignmentDeviationGrid; // optimized threat-vs-zone mismatch
	std::vector<float>    m_optimizedProblemScoreGrid; // [0,1], 1=remaining problem after optimization
	std::vector<int>      m_actualCountGrid;     // monster count per cell
	std::vector<float>    m_threatGrid;          // avg threat_level per cell, -1 = missing
	std::vector<int>      m_monsterTypeCountGrid; // distinct monster types per cell
	std::vector<float>    m_rawDiversityGrid;    // Shannon entropy per cell, -1 = not enough local samples
	std::vector<float>    m_diversityGrid;       // smoothed Shannon entropy from nearby weighted type counts
	std::vector<float>    m_diversityConfidenceGrid; // [0,1], fades low-sample diversity display edges
	std::vector<float>    m_accessibilityGrid;   // normalized [0,1], high near roads/settlements
	std::vector<int>      m_cellZoneIdGrid;      // zone_id per cell (cached for reuse)

	// ----- Layout -----
	float m_mapOffsetX = 0.f;

	// ----- Rendering -----
	HeatmapType m_currentHeatmap = HeatmapType::ZONE_REFERENCE;
	std::vector<Vertex_PCU> m_backgroundVerts;
	std::vector<Vertex_PCU> m_heatmapVerts;
};
