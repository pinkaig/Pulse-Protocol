/******************************************************************************/
/**
* @file        Factory.h
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief	   Static utility class providing C# scripts with entity instantiation 
*              from prefabs and entity destruction through factory pattern.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
#pragma once

namespace ScriptAPI
{
	// abstract sealed make it global I think? 
	public ref class Factory abstract sealed 
	{
	public:
		static int Instantiate(System::String^ scriptname);

		static void Destroy(int entityID);

	};
}