/******************************************************************************/
/**
 * @file        Renderable.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai (primary) - 90%
 * @author 		Ban Kai Wei Benjamin (secondary) - 10%
 * @brief       Declares the Renderable component used by the ECS framework.
 *              This component stores references to the model, shader, and
 *              texture used during rendering, along with sprite name for
 *              texture atlas / asset lookup. Provides helper functions for
 *              assigning textures from asset paths, and supports
 *              serialization and deserialization for scene saving/loading.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include "pch/pch_temp.h" // FIRST
#include "CoreEngine/ECS/Coordinator.h"
#include "CoreEngine/Asset/AssetsManager.h"
#include <glm/gtc/type_ptr.hpp>
#include "glhelper.h"

#ifndef FRAMEWORK_RENDERABLE_H
#define FRAMEWORK_RENDERABLE_H

namespace Framework
{
	// for solid color support
	struct Color {
		float r = 1.0f;
		float g = 1.0f;
		float b = 1.0f;
		float a = 1.0f;

		Color() = default;
		Color(float red, float green, float blue, float alpha = 1.0f)
			: r(red), g(green), b(blue), a(alpha) {
		}

		bool operator!=(Color const& other) const {
			return
				r != other.r ||
				g != other.g ||
				b != other.b ||
				a != other.a;
		}

		bool operator==(Color const& other) const {
			return !(*this != other);
		}
	};


	class Renderable
	{
	public:
		int texW = 0;
		int texH = 0;
		float mdl_ref = 0.f;		// Model no
		float shd_ref = 0.f;		// Shader no
		GLuint textureID = 0;		// Texture ID
		std::string spriteName;

		Color tintColor = Color(1.0f, 1.0f, 1.0f, 1.0f); // Default white
		bool useTexture = true;     // Flag: true = use texture, false = use solid color

		struct GlowState {
			bool  enabled   = false;
			float intensity = 1.0f;   // multiplier applied to glow color when rendering
			float r = 1.f, g = 1.f, b = 1.f;  // glow color
		} glow;

		//void SetSprite(std::string ref);
		//std::string GetSprite() const;
		void AssignTextureFromPath(Framework::Renderable& t);
		void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const;
		void Deserialize(const rapidjson::Value& in);

		bool        needsTextureReload = false;   // true after user edits the name
		std::string loadedSpriteName;             // the name we actually loaded

		//operator overloads
		//for Undo comparison function
		bool operator==(Renderable const& other) const {
				return
		texW == other.texW &&
		texH == other.texH &&
		mdl_ref == other.mdl_ref &&
		shd_ref == other.shd_ref &&
		textureID == other.textureID &&
		spriteName == other.spriteName &&
		tintColor == other.tintColor &&
		useTexture == other.useTexture &&
		loadedSpriteName == other.loadedSpriteName;
		}
		bool operator!=(Renderable const& other) const
		{
			return !(*this == other);
		}
	};

	////std::ostream& operator<<(std::ostream& os, Renderable const& t);
	//	std::ostream& operator<<(std::ostream& os, Renderable const& r) {
	//	os << "[Renderable Component] contains these values\n";
	//	os << "Renderable texW:" << r.texW << "\n";
	//	os << "Renderable texH:" << r.texH << "\n";
	//	os << "Renderable spriteName:" << r.spriteName << "\n";
	//	os << "Renderable mdl_ref:" << r.mdl_ref << "\n";
	//	os << "Renderable shd_ref:" << r.shd_ref << "\n";
	//	os << "Renderable textureID:" << r.textureID << "\n";
	//	return os;
	//}
}
#endif // FRAMEWORK_RENDERABLE_H