#include "Engine/core/NamedProperites.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"

template<typename T>
void NamedProperties::SetValue(std::string const& keyName, T const& value)
{
	auto found = m_properties.find(keyName);
	if (found == m_properties.end())	// this key name property does not exist
	{
		m_properties[keyName] = new TypedProperty<T>(value);
	}
	else  // this key name property already exist
	{
		TypedProperty<T>* asSameType = dynamic_cast<TypedProperty<T>*>(found->second);
		if (asSameType)
		{
			m_properties[keyName] = value;
		}
		else
		{
			delete found->second;
			m_properties[keyName] = new TypedProperty<T>(value);
		}
	}
}

template<typename T>
T NamedProperties::GetValue(std::string keyName, T defaultValue) const
{
	auto found = m_properties.find(keyName);
	if (found == m_properties.end())
	{
		return defaultValue;
	}
	else
	{
		TypedProperty<T>* asTypedProperty = dynamic_cast<TypedProperty<T>*>(found->second);
		if (asTypedProperty)
		{
			return asTypedProperty->m_data;
		}
		else
		{
			ERROR_AND_DIE("As for type is incorrect");
		}
	}
}