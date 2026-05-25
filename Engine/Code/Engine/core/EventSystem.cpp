#include "Engine/core/EventSystem.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"
#include "Engine/core/DevConsole.hpp"


EventSystem* g_theEventSystem = nullptr;
extern DevConsole* g_theDevConsole;

bool EventSubscriptionStandaloneFunction::Fire(EventArgs& eventArgs)
{
	return m_functionPtr(eventArgs);
}

EventSystem::EventSystem(EventSystemConfig const& config)
	:m_config(config)
{

}

void EventSystem::Startup()
{

}

void EventSystem::Shutdown()
{
	// todo: ??? what should I do with the event system shut down?
	// delete &m_config;
	// delete &m_subscriptionlistsByEventNames;
}

void EventSystem::BeginFrame()
{

}

void EventSystem::EndFrame()
{

}

// todo: add an bool option that will only allow subscript the function once
void EventSystem::SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFuncPtr)
{
	m_subscriptionlistsByEventNamesMutex.lock();
	std::string eventName_lower = ToLower(eventName);
	SubscriptionList& subscribersForThisEvent = m_subscriptionlistsByEventNames[eventName_lower];
	EventSubscriptionStandaloneFunction* standaloneFunction = new EventSubscriptionStandaloneFunction(callbackFuncPtr);
	subscribersForThisEvent.push_back(standaloneFunction);
	m_subscriptionlistsByEventNamesMutex.unlock();
}

template<typename T_ObjectType>
void EventSystem::SubscribeEventCallbackMemberFunction(std::string const& eventName, T_ObjectType* objectPtr,
	bool (T_ObjectType::* methodPtr)(EventArgs& eventArgs))
{
	EventSubscriptionBase* newObjMethodSubscription = new EventSubscriptionMemberFunction<T_ObjectType>(objectPtr, methodPtr);

	m_subscriptionlistsByEventNamesMutex.lock();
	std::string eventName_lower = ToLower(eventName);
	SubscriptionList& subscribersForThisEvent = m_subscriptionlistsByEventNames[eventName_lower];
	subscribersForThisEvent.push_back(newObjMethodSubscription);
	m_subscriptionlistsByEventNamesMutex.unlock();
}

void EventSystem::UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFuncPtr)
{
	m_subscriptionlistsByEventNamesMutex.lock();
	std::map< std::string, SubscriptionList>::iterator found = m_subscriptionlistsByEventNames.find(eventName);
	if (found != m_subscriptionlistsByEventNames.end())
	{
		SubscriptionList& subscribersToThisEvent = found->second;

		// go over the subscriber list
		for (int i = 0; i < (int)subscribersToThisEvent.size(); ++i)
		{
			EventSubscriptionBase* eventSubscriptionBase = subscribersToThisEvent[i];
			EventSubscriptionStandaloneFunction* eventSubscriptionFunc = dynamic_cast<EventSubscriptionStandaloneFunction*>(eventSubscriptionBase);
			// find the subscriber to unsubscribe
			if (callbackFuncPtr && eventSubscriptionFunc->m_functionPtr == callbackFuncPtr)
			{
				subscribersToThisEvent.erase( subscribersToThisEvent.begin() + i );
				// because we erase the subscriber from the list and all the other subscriber in the list move forward
				// e.g. we just deleted entry [4], so stay at [4] to check the new[4]
				// and for the next for loop the ++i, we --i to cancel ++ to check the original [5]
				--i;
			}
		}

		// if the subscriber list becomes empty, remove it from the std::map
		if (subscribersToThisEvent.empty())
		{
			m_subscriptionlistsByEventNames.erase(found);
		}
	}
	m_subscriptionlistsByEventNamesMutex.unlock();
}

template<typename T_ObjectType>
void EventSystem::UnsubscribeEventCallbackFunction(std::string const& eventName, T_ObjectType* objectPtr, bool (T_ObjectType::* methodPtr)(EventArgs& eventArgs))
{
	m_subscriptionlistsByEventNamesMutex.lock();
	std::map< std::string, SubscriptionList>::iterator found = m_subscriptionlistsByEventNames.find(eventName);
	if (found != m_subscriptionlistsByEventNames.end())
	{
		SubscriptionList& subscribersToThisEvent = found->second;

		// go over the subscriber list
		for (int i = 0; i < (int)subscribersToThisEvent.size(); ++i)
		{
			EventSubscriptionBase* eventSubscriptionBase = subscribersToThisEvent[i];
			EventSubscriptionMemberFunction<T_ObjectType>* eventSubscriptionMemberFunc = dynamic_cast< EventSubscriptionMemberFunction<T_ObjectType>* >(eventSubscriptionBase);
			// find the subscriber to unsubscribe
			if (eventSubscriptionMemberFunc && eventSubscriptionMemberFunc->IsForObject(objectPtr) && eventSubscriptionMemberFunc->IsForMemberFunction(methodPtr))
			{
				subscribersToThisEvent.erase(subscribersToThisEvent.begin() + i);
				// because we erase the subscriber from the list and all the other subscriber in the list move forward
				// e.g. we just deleted entry [4], so stay at [4] to check the new[4]
				// and for the next for loop the ++i, we --i to cancel ++ to check the original [5]
				--i;
			}
		}

		// if the subscriber list becomes empty, remove it from the std::map
		if (subscribersToThisEvent.empty())
		{
			m_subscriptionlistsByEventNames.erase(found);
		}
	}
	m_subscriptionlistsByEventNamesMutex.unlock();
}

// stop the all the subscriptions for a specific callback function
void EventSystem::UnscribeFromAllEvents(EventCallbackFuncPtr callbackFunc)
{
	// std::map< std::string, std::vector<EventCallbackFuncPtr> >::iterator eventIter;
	// the line above could also be written as:
	std::map< std::string, SubscriptionList>::iterator eventIter;
	m_subscriptionlistsByEventNamesMutex.lock();
	for (eventIter = m_subscriptionlistsByEventNames.begin(); eventIter != m_subscriptionlistsByEventNames.end(); ++ eventIter)
	{
		std::string const& eventName = eventIter->first; // this is the way to get the key
		// SubscriptionList& subscribersForThisEvent = eventIter->second; // this is the way to get the value
		UnsubscribeEventCallbackFunction(eventName, callbackFunc);
	}
	m_subscriptionlistsByEventNamesMutex.unlock();
}

void EventSystem::FireEvent(std::string const& eventName, EventArgs& args)
{
	std::string eventName_lowerStr = ToLower(eventName);

	// find the subscriber list
	m_subscriptionlistsByEventNamesMutex.lock();
	std::map< std::string, SubscriptionList>::iterator found = m_subscriptionlistsByEventNames.find(eventName_lowerStr);
	if (found != m_subscriptionlistsByEventNames.end())
	{
		SubscriptionList& subscribersToThisEvent = found->second;

		// go over the subscriber list
		for (int i = 0; i < (int)subscribersToThisEvent.size(); ++i)
		{
			EventSubscriptionBase* callbackFuncBase = subscribersToThisEvent[i];
			bool wasConsumed = callbackFuncBase->Fire(args);// call the subscriber's callback function
			if (wasConsumed)
			{
				break; 
				// Event was consumed by this subscriber, tell no remaining subscribers about the event is firing
				// e.g. the phone is ringing and if one person picks it up, the rest of them don't need to answer the phone
				// but it might be the ring is ringing and everyone knows that so it is not consumed
			}
		}

		// print the line on screen if the devConsole successful execute a subscriber function
		g_theDevConsole->m_executeFoundSubscriber = true;
	}
	else 
	{
		// no subscriber is no founded
		g_theDevConsole->m_executeFoundSubscriber = false;
	}
	m_subscriptionlistsByEventNamesMutex.unlock();
}

void EventSystem::FireEvent(std::string const& eventName)
{
	// EventArgs emptyArgs;
	// SubscriptionList& subscriberForThisEvent = m_subscriptionlistsByEventNames[eventName];
	// for (int i = 0; i < (int)subscriberForThisEvent.size(); ++i)
	// {
	// 	EventCallbackFuncPtr callbackFuncPtr = subscriberForThisEvent[i];
	// 	if (!callbackFuncPtr)
	// 	{
	// 		callbackFuncPtr(emptyArgs);// call the subscriber's callback function
	// 	}
	// }

	std::string lowerString = ToLower(eventName);
	EventArgs args; // crate a fake and empty args
	FireEvent(eventName, args);
}

Strings EventSystem::GetAllSubscriptionEventNames()
{
	Strings eventNames;

	m_subscriptionlistsByEventNamesMutex.lock();
	for (std::map<std::string, SubscriptionList>::iterator it = m_subscriptionlistsByEventNames.begin();
		it != m_subscriptionlistsByEventNames.end(); ++it)
	{
		eventNames.push_back(it->first);
	}
	m_subscriptionlistsByEventNamesMutex.unlock();

	return eventNames;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFunc)
{
	// register the event name to the dev console
	// All key lookups for event names must be case-insensitive
	std::string lowerString = ToLower(eventName);
	g_theEventSystem->SubscribeEventCallbackFunction(lowerString, callbackFunc);
}

void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFunc)
{
	g_theEventSystem->UnsubscribeEventCallbackFunction(eventName, callbackFunc);
}

void UnscribeFromAllEvents(EventCallbackFuncPtr callbackFunc)
{
	if (g_theEventSystem)
	{
		g_theEventSystem->UnscribeFromAllEvents(callbackFunc);
	}
	else
	{
		ERROR_AND_DIE("The event system does not exist!!!");
	}
}

void FireEvent(std::string const& eventName, EventArgs& args)
{
	if (g_theEventSystem)
	{
		g_theEventSystem->FireEvent(eventName, args);
	}
	else
	{
		ERROR_AND_DIE("The event system does not exist!!!");
	}
}

void FireEvent(std::string const& eventName)
{
	if (g_theEventSystem)
	{
		g_theEventSystem->FireEvent(eventName);
	}
	else
	{
		ERROR_AND_DIE("The event system does not exist!!!");
	}
}
