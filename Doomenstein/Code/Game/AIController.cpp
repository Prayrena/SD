#include "Game/AIController.hpp"
#include "Game/Map.hpp"
#include "Game/Weapon.hpp"

AIController::AIController(Map* map, ActorUID uid)
	:Controller(map, uid)
{

}

void AIController::Update()
{
	Actor* controlledActor = m_map->GetActorByUID(m_actorUID);
	// Stops attempting to control their actor if it is dead or deleted
	if (!controlledActor)
	{
		return;
	}
	if (controlledActor->m_isDead || controlledActor->m_isDestroyed)
	{
		return;
	}

	// if an AI controller is controlling an actor, it is going to look for the Marine see if it is visible and close enough
	Actor* targetActor = nullptr;
	if (controlledActor)
	{
		targetActor = m_map->GetClosestVisibleEnemy(controlledActor);
	}

	// if the target actor exist, update and save the target
	if (targetActor)
	{
		m_targetUID = targetActor->m_actorUID;
	}
	else // if there is no closest visible enemy, we will check for the target we saw before
	{
		targetActor = m_map->GetActorByUID(m_targetUID);
	}
	
	// if there is a closest visible enemy
	if (m_map->CheckIfActorExistAndIsAlive(targetActor))
	{
		// turn towards and move towards the target
		Vec3 disp = targetActor->m_position - controlledActor->m_position;
		float dist = disp.GetLength();
		disp = disp.GetNormalized();
		float goalAngle = disp.GetYawDegrees();
		controlledActor->TurnInDirection(goalAngle);


		// check if controlled actor has a weapon and get the attack range
		Weapon* currentWeapon = controlledActor->GetEquippedWeapon();
		float attackRange = 0.f;

		if (currentWeapon)
		{
			switch (currentWeapon->m_weaponDef->m_weaponType)
			{
			case WeaponType::RAYCAST: {attackRange = currentWeapon->m_weaponDef->m_rayRange; } break;
			case WeaponType::PROJECTILE: {attackRange = currentWeapon->m_weaponDef->m_flyDist; } break;
			case WeaponType::MELEE: {attackRange = currentWeapon->m_weaponDef->m_meleeRange; } break;
			}
		}

		float slowDownDist = 2.f;
		// Vec3 direction = targetActor->m_position - controlledActor->m_position; // just move the actor towards iBasis of the actor because we don't want the actor go backwards
		Vec3 direction = Vec3(1.f, 0.f, 0.f);
		direction = controlledActor->GetModelMatrix().TransformVectorQuantity3D(direction);
		
		// if the attack range is too small, smaller than 2 unit
		if (attackRange < slowDownDist)
		{
			// chasing the target from run speed
			if (dist >= slowDownDist)
			{
				controlledActor->MoveInDirection(direction, controlledActor->m_actorDef->m_runSpeed);
				// controlledActor->m_currentState = ActorAnimState::WALK;
			}

			// if the controlling actor get close enough to the target, switch to walk speed
			// this is trying to avoid the enemy is running too fast and circling around the target actor
			if (dist < slowDownDist && dist > attackRange)
			{
				controlledActor->MoveInDirection(direction, controlledActor->m_actorDef->m_walkSpeed);
				// controlledActor->m_currentState = ActorAnimState::WALK;
			}
		}
		else // we will just run to the attack range
		{
			// chasing the target from run speed
			if (dist >= (attackRange * 0.9f))
			{
				controlledActor->MoveInDirection(direction, controlledActor->m_actorDef->m_runSpeed);
			}
		}
		// we also want the enemy stop moving when the player is in its weapon range

		// if the controlled actor has a weapon, it will try to attack when the opponent is in range
		if (dist <= attackRange)
		{
			controlledActor->Attack();
		}
	}
}

bool AIController::IsPlayer()
{
	return false;
}
