/******************************************************************************/
/**
 * @file        Renderable.cpp
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 * @brief       Implements the Renderable component defined in Renderable.h.
 *              Provides texture assignment from asset paths and handles
 *              serialization and deserialization of Renderable data for
 *              scene saving and loading. This includes model reference,
 *              shader reference, texture ID, and sprite/texture lookup name.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#include "Renderable.h"
#include "Math/math.h"

namespace Framework
{
	void AssignTextureFromPath(Framework::Renderable &t)
	{
		if (t.textureID == 0 && !t.spriteName.empty())
		{
			// t.textureID = Framework::GraphicsManager::GetOrLoad(t.spriteName);
			//  t.textureID = mAssets.getGraphics().GetOrLoad(t.spriteName);
			t.textureID = mAssets.GetOrLoadTexture(t.spriteName);
		}
	}

	/*Serialization && Deserialization */
	void Renderable::Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const
	{
		using namespace rapidjson;
		out.SetObject();

		out.AddMember("mdl_ref", mdl_ref, alloc);
		out.AddMember("shd_ref", shd_ref, alloc);
		out.AddMember("useTexture", useTexture, alloc);

		Value spriteNameVal;
		spriteNameVal.SetString(spriteName.c_str(), static_cast<SizeType>(spriteName.length()), alloc);
		out.AddMember("spriteName", spriteNameVal, alloc);

		/* Color Serialization part */
		Value colorVal(kObjectType);
		colorVal.AddMember("r", tintColor.r, alloc);
		colorVal.AddMember("g", tintColor.g, alloc);
		colorVal.AddMember("b", tintColor.b, alloc);
		colorVal.AddMember("a", tintColor.a, alloc);
		out.AddMember("tintColor", colorVal, alloc);

		/* Glow Serialization */
		Value glowVal(kObjectType);
		glowVal.AddMember("enabled",   glow.enabled,   alloc);
		glowVal.AddMember("intensity", glow.intensity, alloc);
		glowVal.AddMember("r",         glow.r,         alloc);
		glowVal.AddMember("g",         glow.g,         alloc);
		glowVal.AddMember("b",         glow.b,         alloc);
		out.AddMember("glow", glowVal, alloc);
	}

	void Renderable::Deserialize(const rapidjson::Value &in)
	{
		if (!in.IsObject())
		{
			throw std::runtime_error("Invalid JSON for Renderable");
		}

		if (in.HasMember("mdl_ref"))
			mdl_ref = in["mdl_ref"].GetFloat();
		if (in.HasMember("shd_ref"))
			shd_ref = in["shd_ref"].GetFloat();

		/*if (in.HasMember("textureID"))
			textureID = static_cast<GLuint>(in["textureID"].GetUint64());*/

		if (in.HasMember("spriteName"))
			spriteName = in["spriteName"].GetString();

		if (in.HasMember("useTexture"))
			useTexture = in["useTexture"].GetBool();

		/* color deserialization part */
		if (in.HasMember("tintColor") && in["tintColor"].IsObject())
		{
			auto &col = in["tintColor"];
			if (col.HasMember("r"))
				tintColor.r = col["r"].GetFloat();
			if (col.HasMember("g"))
				tintColor.g = col["g"].GetFloat();
			if (col.HasMember("b"))
				tintColor.b = col["b"].GetFloat();
			if (col.HasMember("a"))
				tintColor.a = col["a"].GetFloat();
		}

		/* Glow deserialization */
		if (in.HasMember("glow") && in["glow"].IsObject())
		{
			auto& gl = in["glow"];
			if (gl.HasMember("enabled"))   glow.enabled   = gl["enabled"].GetBool();
			if (gl.HasMember("intensity")) glow.intensity = gl["intensity"].GetFloat();
			if (gl.HasMember("r"))         glow.r         = gl["r"].GetFloat();
			if (gl.HasMember("g"))         glow.g         = gl["g"].GetFloat();
			if (gl.HasMember("b"))         glow.b         = gl["b"].GetFloat();
		}
	}


	/*
	* Something wrong if define outside the class have LINKR error wtf?
	*/

	//bool Renderable::operator==(Renderable const& other) const
	//{
	//	return
	//		texW == other.texW &&
	//		texH == other.texH &&
	//		mdl_ref == other.mdl_ref &&
	//		shd_ref == other.shd_ref &&
	//		textureID == other.textureID &&
	//		spriteName == other.spriteName &&
	//		useTexture == other.useTexture &&
	//		loadedSpriteName == other.loadedSpriteName;
	//}
	//bool Renderable::operator!=(Renderable const& other) const
	//{
	//	return !(*this == other);
	//}

	//std::ostream& operator<<(std::ostream& os, Renderable const& r) {
	//	os << "[Renderable Component] contains these values\n";
	//	os << "Renderable texW:" << r.texW << "\n";
	//	os << "Renderable texH:" << r.texH << "\n";
	//	os << "Renderable spriteName:" << r.spriteName << "\n";
	//	os << "Renderable mdl_ref:" << r.mdl_ref << "\n";
	//	os << "Renderable shd_ref:" << r.shd_ref << "\n";
	//	os << "Renderable textureID:" << r.textureID << "\n";
	//	return os;
	//}

} // end of namespace
