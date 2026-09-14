#pragma once
#define generic generic_workaround
#include "ScriptName.h"
#include "CoreEngine/Core/ScriptingBridge.h"
#include "GameLogic/GameLogic.h"
#include <msclr/marshal_cppstd.h>   // for converting C# to C++ string
#undef generic


namespace ScriptAPI
{
	// constrcutro for each instance, Param: ECS entity ID. We make both ECS and script system use same ID.
	//LogicComponent::LogicComponent(unsigned int ID) : entityID(ID) {}

	//System::String^ LogicComponent::ScriptName::get()
	//{
	//	auto* SN = ScriptBridge::GetLogicComponent(entityID);
	//	
	//	// Convert std::string to System::String^
	//	return gcnew System::String(SN->ScriptName.c_str());	
	//}	

	//void LogicComponent::ScriptName::set(System::String^ value)
	//{
	//	auto* SN = ScriptBridge::GetLogicComponent(entityID);
	//	
	//	// Convert System::String^ to std::string
	//	msclr::interop::marshal_context context;
	//	SN->ScriptName = context.marshal_as<std::string>(value);
	//}
}
