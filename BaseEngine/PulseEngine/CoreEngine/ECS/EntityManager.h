/******************************************************************************/
/**
 * @file       EntityManager.h
 * @project     Pulse Protocol
 * @author      Chia Wei Xuan Rachael (primary) - 97%
 * @author		Ban Kai Wei Benjamin (secondary) - 3%
 * @brief      Manages entity lifecycle and component signatures in the ECS
 *             architecture. Maintains a pool of reusable entity IDs using a
 *             queue-based allocation system, tracks live entity count, and
 *             stores component signature bitsets for each entity to enable
 *             efficient system-entity matching
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#pragma once
#include "Types.h"
#include "System.h"
#include "pch/pch_temp.h"


class EntityManager
{
public:
	EntityManager()
	{
		for (Entity entity = 0; entity < MaxEntity; ++entity)
		{
			// Just an queue of Entity ID(int)
			EntityList.push_back(entity);
		}
	}

	Entity CreateEntity()
	{
		// IN CASE AMOUNT OF ENTITY IS ABOVE WHAT IS SET(iirc its 32, in types.h)
		assert(LiveEntityCount < MaxEntity && "Too many entities in existence.");

		// Take an ID from the front of the queue
		Entity ID = EntityList.front();

		// remove it from queue
		EntityList.pop_front();
		++LiveEntityCount;
		//aaa
		Alive[ID] = true;
		return ID;
	}

	Entity CreateEntityWithID(Entity id)
	{
		assert(id < MaxEntity && "Entity out of range.");
		// Make sure this ID is currently free (i.e., in the queue)
		auto it = std::find(EntityList.begin(), EntityList.end(), id);
		if (it == EntityList.end())
		{
			//ID not in free list->already alive
				return static_cast<Entity>(-1);
		}
		 //Take this ID out of the free list
		EntityList.erase(it);
		++LiveEntityCount;
		 //Reset signature (should already be reset by RemoveEntity, but this is safe)
		SignatureID[id].reset();

		Alive[id] = true;
		return id;
	}

	void RemoveEntity(Entity entity)
	{
		// make sure Entity ID is withint range
		assert(entity < MaxEntity && "Entity out of range.");

		// clear it 
		SignatureID[entity].reset();

		// make it so the Entity ID is now free for use 
		EntityList.push_back(entity);
		--LiveEntityCount;
		Alive[entity] = false;
	}

	void SetSignature(Entity entity, Signature signature)
	{
		assert(entity < MaxEntity && "Entity out of range.");

		// set entity to the signature
		SignatureID[entity] = signature;
	}

	Signature GetSignature(Entity entity)
	{
		assert(entity < MaxEntity && "Entity out of range.");

		// give entity systems bit signature
		return SignatureID[entity];
	}

	std::deque<Entity> GetActiveEntities() const
	{
		std::deque<Entity> active;
		for (Entity e = 0; e < MaxEntity; ++e)
		{
			//Signature sig = SignatureID[e];
			//if (sig.any())
			//	active.push_back(e);

			if (Alive[e])
				active.push_back(e);
		}
		return active;
	}

	bool IsAlive(Entity e) const
	{
		return (e < MaxEntity) && Alive[e];
	}

	// make a function for ben that returns EntityList, link with coordinator cos its not in the ecs. aka not inherite ECS

private:
	std::deque<Entity> EntityList{}; //EntityList[i]
	std::array<Signature, MaxEntity> SignatureID{};
	unsigned int LiveEntityCount{};
	std::array<bool, MaxEntity> Alive{}; // false by default
};