/******************************************************************************/
/**
 * @file        Coordinator.h
 * @project     Pulse Protocol
 * @author      Chia Wei Xuan Rachael (primary) - 97%
 * @author		Ban Kai Wei Benjamin (secondary) - 3%
 * 
 * @brief      Central coordinator class implementing the Singleton pattern that
 *             acts as a unified interface for the ECS architecture. Manages
 *             entity creation/destruction, component registration/access, and
 *             system management by coordinating between EntityManager,
 *             ComponentManager, and SystemManager
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#pragma once
#include "pch/pch_temp.h"
#include "Types.h"
#include "System.h"
#include "SystemManager.h"
#include "EntityManager.h"
#include "ComponentArray.h"
#include "ComponentManager.h"
#include "CoreEngine/ECS/Singleton.h"
#include "CoreEngine/Core/ImportExport.h"
#include "../PulseEngine/ObjectAllocator/ObjectAllocator.h"

//  Acts as a mediator that allows us to have a single instance of the coordinator to use for all 3 manager :D
class DLL_API Coordinator : public Singleton<Coordinator>
{
public:	
	Coordinator()
		: cfg_(/*UseCPPMemManager*/ false,
			/*ObjectsPerPage*/  4,
			/*MaxPages*/        2,
			/*DebugOn*/         false,
			/*PadBytes*/        0,
			/*HeaderBlocks*/    0,
			/*Alignment*/       alignof(std::max_align_t)),
		entityOA_(sizeof(EntityManager), cfg_),
		compOA_(sizeof(ComponentManager), cfg_),
		sysOA_(sizeof(SystemManager), cfg_)
	{
	}

	//static Coordinator* GetInstance();
	void init()
	{
		//OAConfig cfg;

		//cfg.ObjectsPerPage_ = 4;
		//cfg.MaxPages_ = 2;
		//cfg.UseCPPMemManager_ = false; // use your pool

		//ObjectAllocator studentObjMgr(sizeof(Student), cfg);
		//std::cout << "B4 Alloc:\n";
		//PrintCounts2(&studentObjMgr);
		//// 2) Allocate a few students
		//// Student* s1 = reinterpret_cast<Student*>(studentObjMgr.Allocate());
		//// Student* s2 = reinterpret_cast<Student*>(studentObjMgr.Allocate());
		//// Student* s3 = reinterpret_cast<Student*>(studentObjMgr.Allocate());
		//// Student* s4 = reinterpret_cast<Student*>(studentObjMgr.Allocate());
		//std::unique_ptr<Student, OADeleter> s1 = oa_make_unique<Student>(studentObjMgr);
		//std::unique_ptr<Student, OADeleter> s2 = oa_make_unique<Student>(studentObjMgr);
		//std::unique_ptr<Student, OADeleter> s3 = oa_make_unique<Student>(studentObjMgr);
		//std::unique_ptr<Student, OADeleter> s4 = oa_make_unique<Student>(studentObjMgr);

		//std::unique_ptr<EntityManager> CoordEnitiyManager;
		//std::unique_ptr<ComponentManager> CoordComponentManager;
		//std::unique_ptr<SystemManager> CoordSystemManager;

		// Create pointers to each manager
		//CoordEnitiyManager = std::make_unique<EntityManager>();
		//CoordComponentManager = std::make_unique<ComponentManager>();
		//CoordSystemManager = std::make_unique<SystemManager>();

		

		CoordComponentManager = oa_make_unique<ComponentManager>(compOA_);
		CoordEnitiyManager = oa_make_unique<EntityManager>(entityOA_);
		CoordSystemManager = oa_make_unique<SystemManager>(sysOA_);
	}

	// Shortcuts for create/kill entity 
	Entity CreateEntity()
	{
		return CoordEnitiyManager -> CreateEntity();	
	}

	Entity CreateEntityWithID(Entity id)
	{
		return CoordEnitiyManager->CreateEntityWithID(id);
	}

	void DestroyEntity(Entity entity)
	{
		CoordEnitiyManager->RemoveEntity(entity);

		CoordComponentManager->RemoveEntity(entity);

		CoordSystemManager->RemoveEntity(entity);
	}

	// Shortcuts for Component methods
	template<typename T>
	void RegisterComponent()
	{
		CoordComponentManager->RegisterComponents<T>();
	}

	template<typename T>
	void AddComponent(Entity entity, T component)
	{
		CoordComponentManager->AddComponent<T>(entity, component);

		auto signature = CoordEnitiyManager->GetSignature(entity);
		signature.set(CoordComponentManager->GetComponentType<T>(), true);

		CoordEnitiyManager->SetSignature(entity, signature);
		CoordSystemManager->EntitySignatureChanged(entity, signature);
	}

	template<typename T>
	void RemoveComponent(Entity entity)
	{
		CoordComponentManager->RemoveComponent<T>(entity);

		auto signature = CoordEnitiyManager->GetSignature(entity);
		signature.set(CoordComponentManager->GetComponentType<T>(), false);

		CoordEnitiyManager->SetSignature(entity, signature);
		CoordSystemManager->EntitySignatureChanged(entity, signature);
	}

	template<typename T>
	T& GetComponent(Entity entity)
	{
		return CoordComponentManager->GetComponent<T>(entity);
	}

	template<typename T>
	ComponentType GetComponentType()
	{
		return CoordComponentManager->GetComponentType<T>();
	}

	template<typename T>
	std::shared_ptr<T> GetSystem()
	{
		return CoordSystemManager->GetSystem<T>();
	}

	template<typename T>
	std::shared_ptr<T> RegisterSystem()
	{
		return CoordSystemManager->RegisteringSystem<T>();
	}

	template<typename T>
	void SetSystemSignature(Signature signature)
	{
		CoordSystemManager->SetSignature<T>(signature);
	}

	void InitializeAllSystems()
	{
		CoordSystemManager->InitializeAllSystems();
	}

	void UpdateAllSystems(float dt)
	{
		CoordSystemManager->UpdateAllSystems(dt);
	}

	void ExitAllSystems()
	{
		CoordSystemManager->ExitAllSystems();
	}

	std::deque<Entity> GetAllEntities() const
	{
		return CoordEnitiyManager->GetActiveEntities();
	}


	template<typename T>
	bool HasComponent(Entity entity) const {
		
		// get entities component bit
		auto sig = CoordEnitiyManager->GetSignature(entity);
		
		// get bit position of this component
		auto type = CoordComponentManager->GetComponentType<T>();
		
		// check if bit is 1(true)
		return sig.test(type);
	}

	// check if this entity has this component, if not the nullptr, if have, give component
	template<typename T>
	T* TryGetComponent(Entity entity) {
		if (!HasComponent<T>(entity)) return nullptr;
		return &CoordComponentManager->GetComponent<T>(entity);
	}

	//aaa
	bool IsAlive(Entity e) const
	{
		return CoordEnitiyManager->IsAlive(e);
	}

private:
	//std::unique_ptr<EntityManager> CoordEnitiyManager;
	//std::unique_ptr<ComponentManager> CoordComponentManager;
	//std::unique_ptr<SystemManager> CoordSystemManager;
	OAConfig cfg_;
	ObjectAllocator entityOA_;
	ObjectAllocator compOA_;
	ObjectAllocator sysOA_;

	std::unique_ptr<EntityManager, OADeleter> CoordEnitiyManager;
	std::unique_ptr<ComponentManager, OADeleter> CoordComponentManager;
	std::unique_ptr<SystemManager, OADeleter> CoordSystemManager;
	


};

