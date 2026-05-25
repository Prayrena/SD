#pragma once
#include "Engine/core/NamedStrings.hpp"
#include "Engine/core/StringUtils.hpp"
#include <string>
#include <vector>
#include <Mutex>

// "callback" in this system literally means the function ptr that is prepared to be trigger in the future
// "event" means the std::map which bounds the string and function
typedef NamedStrings EventArgs;
typedef bool (*EventCallbackFuncPtr)(EventArgs& eventArgs); // call back funcs could take the args modified in the func // todo: why I must have const for EventArgs const& eventArgs?
// typedef std::vector<EventCallbackFuncPtr> SubscriptionList;

struct EventSubscriptionBase
{
	EventSubscriptionBase() = default;
	virtual ~EventSubscriptionBase() = default;

	virtual bool Fire(EventArgs& args) = 0;
};

// for static function
struct EventSubscriptionStandaloneFunction : public EventSubscriptionBase
{
	EventSubscriptionStandaloneFunction(EventCallbackFuncPtr methodPtr)
		: m_functionPtr(methodPtr)
	{
	}

	virtual bool Fire(EventArgs& eventArgs) override;

	EventCallbackFuncPtr	m_functionPtr = nullptr;
};

// for class instance and its member functions
template<typename T_ObjectType>
struct EventSubscriptionMemberFunction : public EventSubscriptionBase
{
	typedef bool (T_ObjectType::*EventCallbackFuncPtr)(EventArgs& eventArgs);

	EventSubscriptionMemberFunction(T_ObjectType* objectPtr, EventCallbackFuncPtr methodPtr)
		: m_functionPtr(methodPtr)
		, m_object(objectPtr)
	{
	}

	virtual bool Fire(EventArgs& eventArgs) override;
	virtual bool IsForObject(T_ObjectType* objPtr) override;
	virtual bool IsForMemberFunction(EventCallbackFuncPtr funcPtr);

	EventCallbackFuncPtr	m_functionPtr = nullptr;
	T_ObjectType*			m_object = nullptr;
};

template<typename T_ObjectType>
bool EventSubscriptionMemberFunction<T_ObjectType>::Fire(EventArgs& eventArgs)
{
	return (m_object->*m_functionPtr)(eventArgs);
}

template<typename T_ObjectType>
bool EventSubscriptionMemberFunction<T_ObjectType>::IsForObject(T_ObjectType* objPtr)
{
	return objPtr == m_object;
}

template<typename T_ObjectType>
bool EventSubscriptionMemberFunction<T_ObjectType>::IsForMemberFunction(EventCallbackFuncPtr funcPtr)
{
	return m_functionPtr == funcPtr;
}

struct EventSystemConfig
{

};

typedef std::vector<EventSubscriptionBase*> SubscriptionList;

class EventSystem
{
friend class DevConsole;

public:
	EventSystem (EventSystemConfig const& config );
	EventSystem() {};
	~EventSystem() {};
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFuncPtr);
	void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFuncPtr);

	template<typename T_ObjectType>
	void SubscribeEventCallbackMemberFunction(std::string const& eventName, T_ObjectType* objectPtr, 
												bool (T_ObjectType::* methodPtr)(EventArgs& eventArgs));
	template<typename T_ObjectType>
	void UnsubscribeEventCallbackFunction(std::string const& eventName, T_ObjectType* objectPtr,
												bool (T_ObjectType::* methodPtr)(EventArgs& eventArgs));

	void UnscribeFromAllEvents(EventCallbackFuncPtr callbackFunc);
	void FireEvent(std::string const& eventName, EventArgs& args);
	void FireEvent(std::string const& eventName); // used when the event argument is set up within this function or it does not need any argument

	Strings GetAllSubscriptionEventNames();

protected:
	EventSystemConfig							m_config;

	std::recursive_mutex						m_subscriptionlistsByEventNamesMutex;
	std:: map<std::string, SubscriptionList>	m_subscriptionlistsByEventNames;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
// standalone global-namespace helper functions: these forward to "the" event system, which should exist for every game
void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFunc);
void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFunc);
void UnscribeFromAllEvents(EventCallbackFuncPtr callbackFunc);
void FireEvent(std::string const& eventName, EventArgs& args);
void FireEvent(std::string const& eventName);

//class Thing
//{
//public:
//	int a = 0;
//	int b = 0;
//
//	int AddTwoInts(int a, int b)
//	{
//		return a + b;
//	}
//};
//
//typedef int (*PtrToMemberFunctionOnThingType)(int, int);
//
//void TestMain()
//{
//	Thing t;
//	Thing* tPtr = &t;
//	PtrToMemberFunctionOnThingType functionPtr = nullptr;
//	int x = 
//}