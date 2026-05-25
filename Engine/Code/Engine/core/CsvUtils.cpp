#include "Engine/core/CsvUtils.hpp"
#include "Engine/core/FileUtils.hpp"
#include "Engine/core/StringUtils.hpp"
#include <string>

//-----------------------------------------------------------------------------------------------
// Splits one CSV line into fields, respecting double-quoted fields that may contain commas.
// Quotes are stripped from the resulting tokens. Empty fields are preserved.
//   e.g.  a,,c          -> ["a", "", "c"]
//   e.g.  "hello, world",42  -> ["hello, world", "42"]
static Strings SplitCsvLine(std::string const& line)
{
	Strings tokens;
	std::string current;
	bool inQuotes = false;

	for (int i = 0; i < (int)line.size(); ++i)
	{
		char c = line[i];
		if (c == '"')
		{
			inQuotes = !inQuotes;
		}
		else if (c == ',' && !inQuotes)
		{
			tokens.push_back(current);
			current.clear();
		}
		else
		{
			current.push_back(c);
		}
	}
	tokens.push_back(current);
	return tokens;
}

//-----------------------------------------------------------------------------------------------
bool CsvDocument::LoadFile(std::string const& filePath)
{
	std::string fileContent;
	int bytesRead = FileReadToString(fileContent, filePath);
	if (bytesRead == 0)
		return false;

	// Strip UTF-8 BOM if present
	if (fileContent.size() >= 3 &&
		(unsigned char)fileContent[0] == 0xEF &&
		(unsigned char)fileContent[1] == 0xBB &&
		(unsigned char)fileContent[2] == 0xBF)
	{
		fileContent = fileContent.substr(3);
	}

	Strings lines = SplitStringOnDelimiter(fileContent, '\n');
	if (lines.empty())
		return false;

	// Strip trailing \r from each line (Windows CRLF line endings)
	for (std::string& line : lines)
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
	}

	// First non-empty line is the header row
	m_headers = SplitCsvLine(lines[0]);

	// Remaining lines are data rows; skip blank lines
	m_rows.reserve(lines.size() - 1);
	for (int i = 1; i < (int)lines.size(); ++i)
	{
		if (lines[i].empty())
			continue;
		m_rows.push_back(SplitCsvLine(lines[i]));
	}

	return true;
}

//-----------------------------------------------------------------------------------------------
int CsvDocument::GetColumnIndex(std::string const& header) const
{
	for (int i = 0; i < (int)m_headers.size(); ++i)
	{
		if (m_headers[i] == header)
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------------------------
int ParseCsvValue(Strings const& row, int colIndex, int defaultValue)
{
	if (colIndex < 0 || colIndex >= (int)row.size() || row[colIndex].empty())
		return defaultValue;
	return atoi(row[colIndex].c_str());
}

float ParseCsvValue(Strings const& row, int colIndex, float defaultValue)
{
	if (colIndex < 0 || colIndex >= (int)row.size() || row[colIndex].empty())
		return defaultValue;
	return (float)atof(row[colIndex].c_str());
}

bool ParseCsvValue(Strings const& row, int colIndex, bool defaultValue)
{
	if (colIndex < 0 || colIndex >= (int)row.size() || row[colIndex].empty())
		return defaultValue;
	return row[colIndex] == "true" || row[colIndex] == "1";
}

std::string ParseCsvValue(Strings const& row, int colIndex, std::string const& defaultValue)
{
	if (colIndex < 0 || colIndex >= (int)row.size() || row[colIndex].empty())
		return defaultValue;
	return row[colIndex];
}

std::string ParseCsvValue(Strings const& row, int colIndex, char const* defaultValue)
{
	if (colIndex < 0 || colIndex >= (int)row.size() || row[colIndex].empty())
		return defaultValue;
	return row[colIndex];
}
