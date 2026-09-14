/*****************************************************************************/
/**
 * @file        GameLogic.h
 * @project     Pulse Protocol
 * @author		Chia Wei Xuan Rachael (primary) - 80%
 * @author		Reginald Lew Yee Ren (secondary) - 10%
 * @author		Goh Pin Kai (secondary) - 5%
 * @author		Ban Kai Wei Benjamin (secondary) - 5%
 * 
 * @brief       Defines the scripting system for game entities.
 *              Provides the interface `Iscript` and derived `PlayerScript` and
 *              `EnemyScript` classes to handle entity-specific logic (movement,
 *              collision mode toggles, and interactions).
 *              Managed by `LogicSystem` which handles script registration,
 *              updating, and serialization of script names to JSON.
 *
 * @copyright  Copyright (C) 2026 DigiPen Institute of Technology.
 *             Reproduction or disclosure of this file or its contents without the
 *             prior written consent of DigiPen Institute of Technology is prohibited.
 ******************************************************************************/
#pragma once
#include "pch/pch_temp.h"
#include <GLFW/glfw3.h>
#include "CoreEngine/ECS/Types.h"
#include "CoreEngine/ECS/System.h"
#include "CoreEngine/ECS/Coordinator.h"
//#include "../PulseEngine/Components/ScriptName.h"
// #include "Serialization/Serialization.h"
#include "Input/RhythmGameplayInput.h"
#include "VFX/VFXManager.h"

//components:
#include "Graphics/Transform.h"
#include "Components/DisplayName.h"
#include "Graphics/Renderable.h"


//Serialize
#pragma warning(push, 0)
#include <rapidjson/document.h>     // for Document, Value, AllocatorType
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h> // for StringBuffer
#pragma warning(pop)

 //for .NET
#include "CoreEngine/Core/ImportExport.h" // for DLL_API
#pragma warning(push)
#pragma warning(disable: 4251)

extern bool g_countdownFinished;

// base class for scripts
class Iscript
{
public:

	// ALL SCRIPTS MUST HAVE THESE 3 FUNCTIONS
	virtual void Init(Entity ID) = 0;
	virtual void Update(Entity ID) = 0;
	virtual void Exit(Entity ID) = 0;

	// if needed, for collision msg
	//virtual void OnCollisionEnter(Entity ID) {};
	//virtual void OnCollisionUpdate(Entity ID) {};
	//virtual void OnCollisionExit(Entity ID) {};


	//ser
	virtual void Serialize() {};


	// add clone function and = 0, so all kid will have to have it.
	virtual Iscript* Clone() = 0;
	//virtual Iscript* Clone(); // i remove the = 0 for testing(past rae)
};

//class PlayerScript : public Iscript
//{
//
//public:
//	void Init(Entity ID) override;
//	void Update(Entity ID) override;
//	void Exit(Entity ID) override;
//
//	void Serialize() override;
//
//	//std::map<std::string, std::any(bool,float,int,std::string)>;
//	// add in game variables here.
//	int health = 0;
//
//	PlayerScript* Clone() override;
//
//private:
//	bool  isAttacking = false;
//	float attackTimer = 0.0f;
//};

class EnemyScript : public Iscript
{
public:
	void Init(Entity ID) override;
	void Update(Entity ID) override;
	void Exit(Entity ID) override;

	EnemyScript* Clone() override;

	// --- Toggles for testing modes ---
	static bool testAABBCollision;
	static bool testOBBCollision;

	float enemyDmg;
};

//class CountdownScript : public Iscript
//{
//public:
//	void Init(Entity ID) override;
//	void Update(Entity ID) override;
//	void Exit(Entity ID) override;
//	CountdownScript* Clone() override;
//
//private:
//	float countdownTimer = 0.0f;
//};
//
//class FeedbackScript : public Iscript
//{
//public:
//	void Init(Entity ID) override;
//	void Update(Entity ID) override;
//	void Exit(Entity ID) override;
//	FeedbackScript* Clone() override;
//
//private:
//	float displayTimer = 0.0f;
//	bool isShowing = false;
//	float startTime = 0.0f;
//};

class DLL_API LogicSystem: public Systems
{
public:
	LogicSystem();  //constructor
	~LogicSystem(); //destructor

	void Init()           override;
	void Update(float dt) override;
	void Exit()           override;

	// Register script and update script function
	void AddScript(std::string const& ScriptName, std::shared_ptr<Iscript> script);

	// We give script name as param and get the update function of each script(child of Iscript)
	std::shared_ptr<Iscript> GrabScript(std::string const& ScriptName);
	VFXManager& GetVfx() { return mVfx; }
	const VFXManager& GetVfx() const { return mVfx; }
private:
	// key is the name of script which is a component, then reward is the class parent but can also grab kid functions and data
	std::unordered_map<std::string, std::shared_ptr<Iscript>> map_of_scripts; // template
	std::unordered_map<Entity, std::unordered_map<std::string, std::shared_ptr<Iscript>>> map_of_scriptInstances; // copying from template (wrap raw ptr into smart ptr(shared) to prevent mem leak ltr )
	VFXManager mVfx;
};

struct DLL_API LogicComponent
{
//    std::string ScriptName;
	std::vector<std::string> ScriptName;

	//undo
	bool operator==(LogicComponent const& other) const;
	bool operator!=(LogicComponent const& other) const;

	// Find out how to make it a script instance.
	//std::unordered_map<std::string, std::unique_ptr<Iscript>> scriptInstances;

	void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const ;

	void Deserialize(const rapidjson::Value& in);
};

bool SaveScriptName(LogicComponent const& t, std::string const& path);
bool LoadScriptName(LogicComponent& t, std::string const& path);
