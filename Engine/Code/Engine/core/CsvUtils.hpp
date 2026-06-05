#pragma once
#include "Engine/core/StringUtils.hpp"
#include <string>
#include <vector>

class TileHeatMap;

// CsvDocument: loads a CSV file and provides row/column access.
// Row 0 is treated as a header row; m_rows contains only data rows.
struct CsvDocument
{
	Strings				 m_headers;
	std::vector<Strings> m_rows;

	bool LoadFile(std::string const& filePath);
	int  GetColumnIndex(std::string const& header) const;  // returns -1 if not found
	int  GetRowCount() const { return (int)m_rows.size(); }
};

// ParseCsvValue: type-safe column extraction with default fallback.
// colIndex < 0 or out of range returns the default value.
int			ParseCsvValue(Strings const& row, int colIndex, int defaultValue);
float		ParseCsvValue(Strings const& row, int colIndex, float defaultValue);
bool		ParseCsvValue(Strings const& row, int colIndex, bool defaultValue);
std::string	ParseCsvValue(Strings const& row, int colIndex, std::string const& defaultValue);
std::string	ParseCsvValue(Strings const& row, int colIndex, char const* defaultValue);

bool WriteTileHeatMapToCsvImageData(std::string const& filePath, TileHeatMap const& heatMap, char const* valueHeader = "value");
