#include "Game/Controller.hpp"
#include "Game/App.hpp"
#include "Game/Player.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Actor.hpp"

extern App* g_theApp;
extern Game* g_theGame;

Controller::Controller(Map* map, ActorUID uid)
	: m_map(map)
	, m_actorUID(uid)
{

}

// Unpossess any currently possessed actor and possess a new one
// Notify each actor so it can check for restoring AI controllers or handle other change of possession logic
void Controller::Possess(Actor* actorPtr)
{
	Actor* actor = GetActor();
	if (actor != nullptr)
	{
		actor->OnUnpossessed(actor->m_controller);
	}

	m_actorUID = actorPtr->m_actorUID;

	// set the controller's camera according to actor
	Player * playerController = dynamic_cast<Player*>(this);
	if (playerController) // if it is the player controller, we are going to set its camera according to the actor setting
	{
		playerController->m_position = actorPtr->m_position + Vec3(0.f, 0.f, actorPtr->m_actorDef->m_eyeHeight);
		playerController->m_orientation = actorPtr->m_orientation;
		playerController->m_freeFlyCamera = false;
	}
}

Actor* Controller::GetActor() const
{
	return m_map->GetActorByUID(m_actorUID);
}

Vec3 Controller::GetAimingDirection()
{
	Actor* controlledActor = m_map->GetActorByUID(m_actorUID);

	if (controlledActor)
	{
		if (this == g_theGame->m_playersList[0])
		{
			return g_theGame->m_playersList[0]->GetCameraOrientation();
		}
		else if (this == g_theGame->m_playersList[1])
		{
			return g_theGame->m_playersList[1]->GetCameraOrientation();
		}
		else
		{
			return Vec3(1.f, 0.f, 0.f);
		}	
	}
	else return Vec3(1.f, 0.f, 0.f);
}
