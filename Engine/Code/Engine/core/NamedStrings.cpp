#include "Engine/core/NamedStrings.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Engine/core/EngineCommon.hpp"

extern NamedStrings g_gameConfigBlackboard;

void NamedStrings::PopulateFromXmlElementAttributes(XmlElement const& element)
{
	XmlAttribute const* setting = element.FirstAttribute();
	while (setting)
	{
		std::string name = setting->Name();
		std::string value = setting->Value();
		g_gameConfigBlackboard.SetValue(name, value);
		setting = setting->Next();
	}
}

void NamedStrings::SetValue(std::string const& keyName, std::string const& newValue)
{
	m_keyValuePairs[keyName] = newValue;
}

Rgba8 NamedStrings::GetValue(std::string const& keyName, Rgba8 const& defaultValue) const
{
	Rgba8 returnValue = defaultValue;
	std::map<std::string, std::string>::const_iterator found = m_keyValuePairs.find(keyName);
	if ( found != m_keyValuePairs.end() )
	{
		returnValue.SetFromText(found->second.c_str());
	}
	return returnValue;
}

Vec2 NamedStrings::GetValue(std::string const& keyName, Vec2 const& defaultValue) const
{
	Vec2 returnValue = defaultValue;
	auto found = m_keyValuePairs.find(keyName);
	if (found != m_keyValuePairs.end())
	{
		returnValue.SetFromText(found->second.c_str());
	}
	return returnValue;
}

Vec3 NamedStrings::GetValue(std::string const& keyName, Vec3 const& defaultValue) const
{
	Vec3 returnValue = defaultValue;
	auto found = m_keyValuePairs.find(keyName);
	if (found != m_keyValuePairs.end())
	{
		returnValue.SetFromText(found->second.c_str());
	}
	return returnValue;
}

IntVec2 NamedStrings::GetValue(std::string const& keyName, IntVec2 const& defaultValue) const
{
	IntVec2 returnValue = defaultValue;
	auto found = m_keyValuePairs.find(keyName);
	if (found != m_keyValuePairs.end())
	{
		returnValue.SetFromText(found->second.c_str());
	}
	return returnValue;
}

std::string NamedStrings::GetValue(std::string const& keyName, char const* defaultValue) const
{
	std::string returnValue = defaultValue;
	auto found = m_keyValuePairs.find(keyName);
	if (found != m_keyValuePairs.end())
	{
		return found->second;
	}
	return returnValue;
}

float NamedStrings::GetValue(std::string const& keyName, float defaultValue) const
{
	float returnValue = defaultValue;
	auto found = m_keyValuePairs.find(keyName);
	if (found != m_keyValuePairs.end())
	{
		// return atof((found->second).c_str());
		return std::stof(found->second);
	}
	return returnValue;
}

int NamedStrings::GetValue(std::string const& keyName, int defaultValue) const
{
	int returnValue = defaultValue;
	auto found = m_keyValuePairs.find(keyName);
	if (found != m_keyValuePairs.end())
	{
		//return atoi((found->second).c_str());
		return stoi(found->second);
	}
	return returnValue;
}

bool NamedStrings::GetValue(std::string const& keyName, bool defaultValue) const
{
	char returnValue = defaultValue;
	auto found = m_keyValuePairs.find(keyName);
	if (found != m_keyValuePairs.end())
	{
		// return atof((found->second).c_str());
		if (found->second == "true")
		{
			return true;
		}
		else return false;
	}
	return returnValue;
}

std::string NamedStrings::GetValue(std::string const& keyName, std::string const& defaultValue) const
{
	auto found = m_keyValuePairs.find(keyName);
	if (found != m_keyValuePairs.end())
	{
		return found->second;
	}
	return defaultValue;
}