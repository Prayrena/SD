#pragma once
#include <vector>
#include <string>
typedef std::vector<std::string> Strings;

//-----------------------------------------------------------------------------------------------
const std::string Stringf( char const* format, ... );
const std::string Stringf( int maxLength, char const* format, ... );

Strings SplitStringOnDelimiter(std::string const& originalString, char const& delimiterToSplitOn, bool delimiterBySpace = false);
int		SplitStringOnDelimiter(Strings& outSplitStrings, std::string const& originalString, std::string const&  delimiterToSplitOn, bool removeEmpty = false);
char*	TrimCharByDelimiter(char const* valueAsText, char delimeterToTrim = '"');
void	GetTokensOfAllLines(Strings const& lines, std::vector<Strings>& tokensOfAllLines);

int			GetCountOfSymbolInString(std::string const& str, char symbol);
std::string	TrimAPairOfDoubleQuotesInString(std::string const& str);
std::string	GetPartOfStringAfterTheSymbol(std::string const& str, char symbol);

std::wstring	ConvertStringToWstring(const std::string& str);
std::string		ToLower(std::string const& string);
char*			ToLower(char const* str);
