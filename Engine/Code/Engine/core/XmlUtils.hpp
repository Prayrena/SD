#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/core/Rgba8.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"

typedef tinyxml2::XMLDocument    XmlDocument;
typedef tinyxml2::XMLElement     XmlElement;
typedef tinyxml2::XMLAttribute   XmlAttribute;
typedef tinyxml2::XMLError       XmlResult;

int			ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue, bool trimDoubleQuotes = true);
char		ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue, bool trimDoubleQuotes = true);
bool		ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue, bool trimDoubleQuotes = true);
float		ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue, bool trimDoubleQuotes = true);
FloatRange	ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange range, bool trimDoubleQuotes = true);
Rgba8		ParseXmlAttribute(XmlElement const& element, char const* attributeName, Rgba8 const& defaultValue, bool trimDoubleQuotes = true);
Vec2		ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue, bool trimDoubleQuotes = true);
Vec3		ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue, bool trimDoubleQuotes = true);
EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue, bool trimDoubleQuotes = true);
IntVec2		ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue, bool trimDoubleQuotes = true);
std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue, bool trimDoubleQuotes = true);
Strings		ParseXmlAttribute(XmlElement const& element, char const* attributeName, Strings const& defaultValues, bool trimDoubleQuotes = true);
std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue, bool trimDoubleQuotes = true);

// VertexType  ParseXmlAttribute(XmlElement const& element, char const* attributeName, VertexType defaulType); // todo: maybe this file is used before the vertex type is declared???
