/******************************************************************************/
/**
 * @file        ComponentManager.h
 * @project     Pulse Protocol
 * @author      Chia Wei Xuan Rachael
 * @brief		Manages registration and access to all component types in the ECS
 *				architecture. Maintains mappings between component type names and
 *				their unique IDs, handles component arrays for each type, and
 *				provides type-safe add/remove/get operations while coordinating
 *				entity-wide component cleanup on entity destruction
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
#include "ComponentArray.h"

class ComponentManager
{
public:
	template<typename T>
	void RegisterComponents()
	{
		// get name of component T. Exp: transform, scale
		std::string ComponentTypeName = typeid(T).name();

		assert(ComponentTypes.find(ComponentTypeName) == ComponentTypes.end() && "Registering component type more than once.");

		ComponentTypes[ComponentTypeName] = NextComponentType;
		ComponentDataArray[ComponentTypeName] = std::make_shared<ComponentList<T>>();

		//ComponentTypes.insert({ ComponentTypeName, NextComponentType });
		//ComponentDataArray.insert({ ComponentTypeName, std::make_shared<ComponentList<T>>() });

		++NextComponentType;

	}

	template<typename T>
	ComponentType GetComponentType()
	{
		const char* ComponentTypeName = typeid(T).name();

		assert(ComponentTypes.find(ComponentTypeName) != ComponentTypes.end() && "Component has not been registered!");

		return ComponentTypes[ComponentTypeName];
	}


	template<typename T>
	void AddComponent(Entity entity, T component)
	{
		GetComponentArray<T>()->AddData(entity, component);
	}

	template<typename T>
	void RemoveComponent(Entity entity)
	{
		GetComponentArray<T>()->RemoveData(entity);
	}

	template<typename T>
	T& GetComponent(Entity entity)
	{
		return GetComponentArray<T>()->GrabData(entity);
	}

	void RemoveEntity(Entity entity)
	{
		// remove all the component the entity is apart of 
		for (auto const& pair : ComponentDataArray)
		{
			auto const& component = pair.second;

			component->RemoveEntity(entity);
		}
	}
	


private:
	// Map where ame of component is key and Component Type(ID) as "reward" (Max 32 for now ah)
	std::unordered_map<std::string, ComponentType> ComponentTypes;

	// Map where name of componentis key and the component data as "reward"
	std::unordered_map<std::string, std::shared_ptr<ComponentTypeList>> ComponentDataArray;
	
	//std::shared_pt: A smart pointer that retains shared ownership of an object through a pointer and 
	//				  only dies when all the object owning the object is destroyed

	// The component type to be assigned to the next registered component. Starts at 0.
	ComponentType NextComponentType{};

	// Helper function thats gets T Component array
	template<typename T>
	std::shared_ptr<ComponentList<T>> GetComponentArray()
	{
		// Grab da name of component T
		const char* ComponentTypeName = typeid(T).name();

		// Check if component has been registered
		assert(ComponentTypes.find(ComponentTypeName) != ComponentTypes.end() && "Component has not been registered!");

		// We change the parent to child. As ComponentDataArray is Parent(ComponentTypeList) & ComponentList is child
		// So we use ComponenTypeName that was set in above and we will get the pointer to the ComponentTypeList that has the data.
		return std::static_pointer_cast<ComponentList<T>>(ComponentDataArray[ComponentTypeName]);
	}

};