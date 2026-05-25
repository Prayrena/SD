#include "Engine/core/XmlUtils.hpp"
#include "StringUtils.hpp"
#include <string>

int ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName);
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		int result = atoi(valueAsText);
		delete valueAsText;
		return result;
	}
	else return defaultValue;
}

char ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName);
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		return *valueAsText;
	}
	else return defaultValue;
}

bool ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName);
	char char_true[] = "true";
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		if (*valueAsText == *char_true)
		{
			delete valueAsText;
			return true;
		}
		delete valueAsText;
		return false;
	}
	else return defaultValue;
}

float ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName);
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		float result = (float)atof(valueAsText);
		delete valueAsText;
		return result;
	}
	else return defaultValue;
}

FloatRange ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange range, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName); // attribute translate element into char
	FloatRange floatRange = range;
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		floatRange.SetFromText(valueAsText);
		delete valueAsText;
		return floatRange;
	}
	else return floatRange;
}

Rgba8 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Rgba8 const& defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName);
	Rgba8 rgb8 = defaultValue;
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		rgb8.SetFromText(valueAsText);
		delete valueAsText;
		return rgb8;
	}
	else return defaultValue;
}

Vec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName);
	Vec2 vec2 = defaultValue;
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		vec2.SetFromText(valueAsText);
		delete valueAsText;
		return vec2;
	}
	else return defaultValue;
}

Vec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName);
	Vec3 vec3 = defaultValue;
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		vec3.SetFromText(valueAsText);
		delete valueAsText;
		return vec3;
	}
	else return defaultValue;
}


EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName);
	EulerAngles angle = defaultValue;
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		angle.SetFromText(valueAsText);
		delete valueAsText;
		return angle;
	}
	else return defaultValue;
}

IntVec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName); // attribute translate element into char
	IntVec2 intVec2 = defaultValue;
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		intVec2.SetFromText(valueAsText);
		delete valueAsText;
		return intVec2;
	}
	else return defaultValue;
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName);
	std::string stringValue = defaultValue;
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
			stringValue = valueAsText;
			delete valueAsText;
		}
		else
		{
			stringValue = valueAsText;
		}
		return stringValue;
	}
	else return defaultValue;
}

Strings ParseXmlAttribute(XmlElement const& element, char const* attributeName, Strings const& defaultValues, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName); // attribute translate element into char
	Strings stringsValues = defaultValues;
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
		}
		std::string originalString = valueAsText;
		delete valueAsText;
		stringsValues = SplitStringOnDelimiter(originalString, ',');
		return stringsValues;
	}
	else return defaultValues;
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue, bool trimDoubleQuotes /*= true*/)
{
	char const* valueAsText = element.Attribute(attributeName);
	std::string stringValue = defaultValue;
	if (valueAsText)
	{
		if (trimDoubleQuotes)
		{
			valueAsText = TrimCharByDelimiter(valueAsText);
			stringValue = valueAsText;
			delete valueAsText;
		}
		else
		{
			stringValue = valueAsText;
		}
		return stringValue;
	}
	else return defaultValue;
}

char* TrimCharByDelimiter(char const* valueAsText)
{
	int len = (int)strlen(valueAsText);
	char* result = new char[len + 1]; 

	int index = 0;
	for (int i = 0; i < len; i++) 
	{
		if (valueAsText[i] != '"') {
			result[index++] = valueAsText[i];
		}
	}
	result[index] = '\0'; // Null-terminate the result

	return result;
}

//VertexType ParseXmlAttribute(XmlElement const& element, char const* attributeName, VertexType defaulType)
//{
//	char const* valueAsText = element.Attribute(attributeName);
//	if (valueAsText == "Vertex_PCUTBN")
//	{
//		return VertexType::Vertex_PCUTBN;
//	}
//	else
//	{
//		return VertexType::Vertex_PCU;
//	}
//}

