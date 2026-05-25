#include "Engine/Core/StringUtils.hpp"
//#include <algorithm> // For std::transform
//#include <cctype>    // For std::tolower
#include <stdarg.h>

//-----------------------------------------------------------------------------------------------
constexpr int STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;


//-----------------------------------------------------------------------------------------------
const std::string Stringf( char const* format, ... )
{
	char textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	return std::string( textLiteral );
}


//-----------------------------------------------------------------------------------------------
const std::string Stringf( int maxLength, char const* format, ... )
{
	char textLiteralSmall[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	char* textLiteral = textLiteralSmall;
	if( maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH )
		textLiteral = new char[ maxLength ];

	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, maxLength, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ maxLength - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	std::string returnValue( textLiteral );
	if( maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH )
		delete[] textLiteral;

	return returnValue;
}

Strings SplitStringOnDelimiter(std::string const& originalString, char const& delimiterToSplitOn, bool delimiterBySpace /*= false*/)
{
	std::string tempString;								// temp store for single string
	std::vector<std::string> StringsCollection;			// list of string for output

	for (int charIndex = 0; charIndex < (int)(originalString.size()); ++charIndex)
	{
		char currentChar = originalString[charIndex];

		if (currentChar != delimiterToSplitOn && !delimiterBySpace)// if it is not the same as the split char and it is not ' '
		{
			tempString.push_back(currentChar);// push the char into the temp char array
		}
		else
		{
			
			if (delimiterBySpace) // remove space as well
			{
				if (currentChar == delimiterToSplitOn || originalString[charIndex] == ' ')
				{
					if (!tempString.empty())
					{
						StringsCollection.push_back(tempString);	// put the temp string into the list of string
						tempString.clear();				// clear up the temp string for the next loop
					}
					else
					{
						continue;
					}
				}
				else
				{
					tempString.push_back(currentChar);// push the char into the temp char array
				}
			}
			else // remove only delimiter
			{
				if (currentChar == delimiterToSplitOn) // need to remove the char
				{
					if (!tempString.empty()) //temp string is not empty
					{
						StringsCollection.push_back(tempString);	// put the temp string into the list of string
						tempString.clear();				// clear up the temp string for the next loop
					}
					else // temp string is empty, check the next char
					{
						continue;
					}
				}
				else // no need to remove the char, add the char to the temp string
				{
					tempString.push_back(currentChar);// push the char into the temp char array
				}
			}
		}
	}

	if (!tempString.empty())
	{
		StringsCollection.push_back(tempString);// put the rest of the temp string into the strings
	}
	Strings result = StringsCollection; // ???????????????????????????????????????????????????????????????????????????????????
	return result;
}

int SplitStringOnDelimiter(Strings& outSplitStrings, std::string const& originalString, std::string const& delimiterToSplitOn, bool removeEmpty/* = false*/)
{
	unsigned int start = 0;
	unsigned int end = (unsigned int)originalString.find(delimiterToSplitOn);

	// Clear the output vector to ensure it's empty before adding new elements
	outSplitStrings.clear();

	// Loop until no more delimiter is found in the original string
	while (end != static_cast<unsigned int>(std::string::npos)) 
	{
		// Extract the substring and add it to the output vector
		if (start != end)
		{
			if (!removeEmpty)
			{
				outSplitStrings.push_back(originalString.substr(start, end - start));
			}
			else
			{
				std::string tempString;
				for (unsigned int i = start; i < (start + (end - start)); ++i)
				{
					char singleChar = originalString[i];
					if (singleChar != ' ')
					{
						tempString.push_back(singleChar);
					}
				}
				outSplitStrings.push_back(tempString);
			}
		}

		// Move the starting index forward to search for the next part of the string
		start = end + (unsigned int)delimiterToSplitOn.length();
		end = (unsigned int)originalString.find(delimiterToSplitOn, start);
	}

	// Add the remaining part of the string after the last delimiter
	// only if the left is not empty
	std::string stringLeft = originalString.substr(start);
	if (!stringLeft.empty())
	{
		outSplitStrings.push_back(originalString.substr(start));
	}

	// Return the number of split strings
	return static_cast<unsigned int>(outSplitStrings.size());
}

char* TrimCharByDelimiter(char const* valueAsText, char delimeterToTrim /*= '"'*/)
{
	int len = (int)strlen(valueAsText);
	char* result = new char[len + 1];

	int index = 0;
	for (int i = 0; i < len; i++)
	{
		if (valueAsText[i] != delimeterToTrim) 
		{
			result[index++] = valueAsText[i];
		}
	}
	result[index] = '\0'; // Null-terminate the result

	return result;
}

void GetTokensOfAllLines(Strings const& lines, std::vector<Strings>& tokensOfAllLines)
{
	for (auto line : lines)
	{
		Strings tokensOfEachLine = SplitStringOnDelimiter(line, ' ');
		tokensOfAllLines.push_back(tokensOfEachLine);
	}
}

int GetCountOfSymbolInString(std::string const& str, char symbol)
{
	int count = 0;
	for (char c : str)
	{
		if (c == symbol)
		{
			++count;
		}
	}
	return count;
}

std::string TrimAPairOfDoubleQuotesInString(std::string const& str)
{
	// if there are not even 2 char or no 2 quote, then do nothing
	if (str.length() < 2 || GetCountOfSymbolInString(str, '"') < 2)
	{
		return str;
	}

	// we'll remove the first and last double quotes in the str
	size_t firstQuote = str.find_first_of("\"");
	size_t lastQuote = str.find_last_of("\"");
	if (firstQuote != std::string::npos && 
		lastQuote != std::string::npos && 
		firstQuote != lastQuote)
	{
		std::string removedStr;
		for (int i = 0; i < (int)str.length(); ++ i)
		{
			if (i != firstQuote && i != lastQuote)
			{
				removedStr.push_back(str[i]);
			}
		}
		return removedStr;
	}
	else
	{
		return str;
	}
}

std::string GetPartOfStringAfterTheSymbol(std::string const& str, char symbol)
{
	size_t pos = str.find(symbol);

	if (pos != std::string::npos ) 
	{
		return str.substr(pos + 1);
	}
	else
	{
		return str;
	}
}

// The "A" suffix indicates this is the ANSI version, which expects an 8-bit string (char*).
// The "W" suffix indicates this is the Unicode version, which expects a wide-character string (wchar_t*).
std::wstring ConvertStringToWstring(const std::string& str)
{
    return std::wstring(str.begin(), str.end());
}

std::string ToLower(std::string const& string)
{
	std::string result = string; // Create a copy of the input string

	// Transform each character to lowercase
	for (char& c : result) 
	{
		c = (char)std::tolower(c); 
	}

	return result; // Return the lowercase string
}

char* ToLower(char const* str)
{
	size_t len = std::strlen(str); 
	char* lowerStr = new char[len + 1];  // Allocate new memory, +1 for the null terminator

	for (size_t i = 0; i < len; ++i) 
	{
		lowerStr[i] = (char)std::tolower(str[i]);
	}
	lowerStr[len] = '\0';  // Null-terminate the new string

	return lowerStr;
}

