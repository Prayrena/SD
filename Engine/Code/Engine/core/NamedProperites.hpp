#pragma once
#include <map>
#include <string>

// example of constructing a class using template
//template<typename T, typename S>
//class stdPair
//{
//	T m_first;
//	S m_second;
//};
//
//template<typename T>
//class stdPair
//{
//	T m_first;
//	template<typename S> S GetValueAsAnotherType()
//	{
//		return (S)m_first;
//	}
//};
//
//class NamedProperties
//{
//public:
//	std::string m_data;
//	template<typename T> void GetAsType(T& out_value)
//	{
//		return static_cast<T>(m_data);
//	}
//};

//int main()
//{
//	//NamedProperties np;
//	//np.m_data = "hello";
//	//int i = 0;
//	//np.GetAsType<int>(i);
//}

class TypedPropertyBase
{

};

template<typename T>
class TypedProperty : public TypedPropertyBase
{
	friend class NamedProperties;

private:
	TypedProperty(T const& initialValue)
		: m_data(initialValue)
	{
	}
	T m_data;
};

class NamedProperties
{
private:
	// you cannot use TypedProperty* because TypedProperty are different class using different m_data type
	std::map<std::string, TypedPropertyBase*> m_properties;

public:
	template<typename T>
	void SetValue(std::string const& keyName, T const& value);

	template<typename T>
	T GetValue(std::string keyName, T defaultValue) const;
};
