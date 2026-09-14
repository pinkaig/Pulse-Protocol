/******************************************************************************/
/**
 * @file        Animation.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 * @brief       Declares the Animation component for the ECS framework.
 *              This component stores sprite sheet animation playback state,
 *              including frame indices, timing, and atlas layout information.
 *              Provides serialization and deserialization support for
 *              saving and loading animation settings in scene files.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once
// #include "../ComponentJson/Component_System.h"
// #include "../Math/Matrix_3x3.h"
// #include "../Math/Vector_2D.h"
#include "pch/pch_temp.h" // FIRST
#include "CoreEngine/ECS/Coordinator.h"
// #include "CoreEngine/Asset/AssetsManager.h"
#include <glm/gtc/type_ptr.hpp>
#include "glhelper.h"
#include <string>
#include "GameState.h"
#ifndef FRAMEWORK_ANIMATION_H
#define FRAMEWORK_ANIMATION_H

namespace Framework
{
	class Animation
	{
	public:
		std::string id = "";
		std::string path = "";
		int currentFrame = 0;
		float elapsedTime = 0.0f; // Time accumulator for animation
		int totalFrames = 1;	  // Total frames in the sprite sheet
		int columns = 1;		  // Columns in the sprite sheet
		int rows = 1;			  // Rows in the sprite sheet
		float animationSpeed = 2.f;
		void Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const;
		void Deserialize(const rapidjson::Value &in);
		static bool IsSingleImage(const Animation& a);
		void Tick(float dt);

		//bool operator==(Animation const& other) const;
		//bool operator!=(Animation const& other) const;
		bool operator==(Animation const& other) const {
			return id == other.id &&
				path == other.path &&
				currentFrame == other.currentFrame &&
				elapsedTime == other.elapsedTime &&
				totalFrames == other.totalFrames &&
				columns == other.columns &&
				rows == other.rows &&
				animationSpeed == other.animationSpeed;
		}
		bool operator!=(Animation const& other) const
		{
			return !(*this == other);
		}
	};
	//std::ostream& operator<<(std::ostream& os, Animation const& t);
}
#endif // FRAMEWORK_ANIMATION_H