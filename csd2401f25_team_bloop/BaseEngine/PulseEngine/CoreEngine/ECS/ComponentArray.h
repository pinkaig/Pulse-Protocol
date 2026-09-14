/******************************************************************************/
/**
 * @file        ComponentArray.h
 * @project     Pulse Protocol
 * @author      Chia Wei Xuan Rachael
 * @brief		Implements a packed array storage system for ECS components with
 *				efficient add/remove operations. Maintains bidirectional mappings
 *				between entities and component indices, using swap-and-pop for
 *				O(1) removal while keeping the array tightly packed for cache
 *				efficiency and optimal iteration performance
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
#include "EntityManager.h"

// parent
class ComponentTypeList	
{
public:
	// if inherited
	virtual ~ComponentTypeList() = default;
	virtual void RemoveEntity(Entity entity) = 0;
};

//child
template<typename T>
class ComponentList: public ComponentTypeList
{
public:
	void AddData(Entity entity, T component)
	{
		assert(Map_For_EntityToIndex.find(entity) == Map_For_EntityToIndex.end() && "Component added to same entity more than once.");

		// Assigned ID based on size of component array ID
		size_t NewID = Size;
		Map_For_EntityToIndex[entity] = NewID;

		// Bi_directional map linking entity and component using ID as the key.
		Map_For_IndexToEntity[NewID] = entity;
		ComponentArray[NewID] = component;

		//update size of component array for T type component
		++Size;
	}

	void RemoveData(Entity entity)
	{
		// First check if entity has this component before removing
		assert(Map_For_EntityToIndex.find(entity) != Map_For_EntityToIndex.end() && "Removing non-existent component.");

		// Swapping. First we get the index of the entity being removed
		size_t Index_of_RemovedEntity = Map_For_EntityToIndex[entity];

		// Get the index of the last component in used. We gonna use this to swap with deleted
		size_t IndexOfLastElement = Size - 1;

		// Swap the component of the last index with the one deleted. We gotta make it a tiedly packed array
		// Before: [0][1][REMOVED][3][4]  After: [1][2][4][3][EMPTY]
		ComponentArray[Index_of_RemovedEntity] = ComponentArray[IndexOfLastElement];

		// Now we do it for the entity side. 
		// Find which entity ID owns the component we moved [E]
		Entity Entity_of_LastElement = Map_For_IndexToEntity[IndexOfLastElement];

		// Mkae it so that the entity know where component is shifted.
		Map_For_EntityToIndex[Entity_of_LastElement] = Index_of_RemovedEntity;
		
		// Allocated it so that it points to that slot that was gonna be deleted
		Map_For_IndexToEntity[Index_of_RemovedEntity] = Entity_of_LastElement;

		// now we can delete the entity and its component as its tied tgt.
		Map_For_EntityToIndex.erase(entity);
		Map_For_IndexToEntity.erase(IndexOfLastElement);

		// Update size of the component array for T type component
		--Size;
	}

	void RemoveEntity(Entity entity) override
	{
		// Only remove if this component is being used by this entity 
		if (Map_For_EntityToIndex.find(entity) != Map_For_EntityToIndex.end())
		{
			RemoveData(entity);
		}
	}

	T& GrabData(Entity entity)
	{
		assert(Map_For_EntityToIndex.find(entity) != Map_For_EntityToIndex.end() && "Retrieving non-existent component.");

		// retun where the component is stored.
		return ComponentArray[Map_For_EntityToIndex[entity]];

		// easier to see I hope :p
		// [T][G][P][]: Component Type Array
		//	|
		//	v
		// [T1] -> [E1] owns this. Map_For_EntityToID[entity] = 0, Entity 1  owns ComponentArray[0]
		// [T2] -> [E1] owns this. Map_For_EntityToID[entity] = 1, Entity 23 owns ComponentArray[1]
		// [T3]	-> [E5] owns this. Map_For_EntityToID[entity] = 2, Entity 5  owns ComponentArray[2]
		// Component Array
	}


private:
	size_t Size{};												// Current number of components stored
	std::array<T, MaxEntity> ComponentArray{};					// Array holding all data of type T. T = Component Type Array
	std::unordered_map<Entity, size_t> Map_For_EntityToIndex{}; // Entity ID to find component Index
	std::unordered_map<size_t, Entity> Map_For_IndexToEntity{}; // component Index to find Entity ID
};
#pragma once
