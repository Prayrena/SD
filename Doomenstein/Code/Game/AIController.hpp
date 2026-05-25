#pragma once
#include "Game/Controller.hpp"

class Map;

class AIController: public Controller
{
public:
	AIController(Map* map, ActorUID uid);
	~AIController() = default;

	virtual void Update() override;
	virtual bool IsPlayer() override;

	// the actor the controller is attacking
	ActorUID m_targetUID = ActorUID::INVALID; // for current target actor
};
