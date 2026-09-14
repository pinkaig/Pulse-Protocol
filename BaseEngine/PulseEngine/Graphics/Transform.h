/******************************************************************************/
/**
 * @file        Transform.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai (primary) - 90%
 * @author 		Ban Kai Wei Benjamin (secondary) - 10%
 * @brief       Declares the Transform component for the ECS framework,
 *              storing position, scale, rotation, animation state,
 *              and sprite data. Provides interfaces for updating,
 *              serializing, and deserializing transform properties.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
// #include "../ComponentJson/Component_System.h"
// #include "../Math/Matrix_3x3.h"
// #include "../Math/Vector_2D.h"

#include "pch/pch_temp.h"
#include "Math/math.h"
#include "CoreEngine/ECS/Coordinator.h"
#include "CoreEngine/Asset/AssetsManager.h"
#include <glm/gtc/type_ptr.hpp>
#include "glhelper.h"
#include "glapp.h"
#include <iostream>

//for .NET
#include "CoreEngine/Core/ImportExport.h" // for DLL_API
#pragma warning(push)
#pragma warning(disable: 4251) // stupid warning just saying to make sure we are using same compilier type and runtime MDd. WHICH WE ARE SO WHY WARNING STILL??????????

#ifndef FRAMEWORK_TRANSFORM_H
#define FRAMEWORK_TRANSFORM_H

namespace Framework
{
	struct TestComponent
	{
		Vector2 Pos;
		Vector2 Scale;
	};

	class DLL_API  Transform
	{
	public:
		// ComponentTypeID typeID = Com_Trans;
		Vector2 Pos;
		Vector2 Scale;
		Vector2 Rotation;
		//float angle_disp = 0.f;
		//float angle_speed = 0.f;
		Matrix3x3 mdl_to_ndc_xform; // Initialize as identity matrix
		Transform();
		~Transform();

		virtual void Initialize();
		void update(GLdouble delta_time, const std::set<Entity> entity);
		void draw() const;



		bool isVisible = true;		  // Used for input detect purpose
		float visibilityTimer = 0.0f; // Used for input detect purpose
		bool autoHide = false;		  // Used for input detect purpose

		//operator overloads
		//for Undo comparison function
		bool operator==(Transform const& other) const;
		bool operator!=(Transform const& other) const;

		// Register these components in your initialization code:
		// void Serialize(Value& data, Document::AllocatorType& alloc) override;
		// void Deserialize(Value& data);
		void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const;
		void Deserialize(const rapidjson::Value& in);
	};

	DLL_API std::ostream& operator<<(std::ostream& os, Transform const& t);

	DLL_API bool SaveTransform(Transform const& t, std::string const& path);
	DLL_API bool LoadTransform(Transform& t, std::string const& path);
}
#pragma warning(pop)
#endif // FRAMEWORK_TRANSFORM_H

//namespace Framework {
//	inline std::ostream& operator<<(std::ostream& os, const Transform& t) {
//			os << "[Transform Component] contains these values\n";
//	os << "Transform pos:" << t.Pos.x << " " << t.Pos.y << "\n";
//	os << "Transform scale:" << t.Scale.x << " " << t.Scale.y << "\n";
//	os << "Transform rot:" << t.Rotation.x << " " << t.Rotation.y << "\n";
//	return os;
//	}
//}