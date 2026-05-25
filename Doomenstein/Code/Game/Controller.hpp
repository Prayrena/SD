#pragma once
#include "Game/ActorUID.hpp"
#include "Engine/Math/Vec3.hpp"

class Actor;
class Map;

class Controller
{
public:
	Controller(Map* map, ActorUID uid = ActorUID::INVALID);
	virtual ~Controller() = default;

	virtual void Update() = 0;

	void Possess(Actor* actorPtr);

	// Returns the currently possessed actor or null if no actor is possessed
	Actor* GetActor() const;

	// void SetnormalizedViewport(const AABB2& viewport);
	// AABB2 GetNormalizedViewport

	virtual bool IsPlayer() = 0;
	// virtual void DamagedBy(Actor* actor);
	// virtual void KilledBy(Actor* actor);
	// virtual void Killed(Actor* actor);

	Vec3 GetAimingDirection();

	// Actor* m_actor; // not store an actor but an UID
	ActorUID m_actorUID = ActorUID::INVALID; // UID of our currently possessed actor or INVALID if no actor is possessed
	Map* m_map = nullptr; // the map that contains the actor that this controller is controlling
};

// map -> update actors -> AI controller
// map -> update player controller(), because the player controller could live on any actor