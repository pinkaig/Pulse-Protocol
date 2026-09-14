/******************************************************************************/
/**
 * @file        AssetRegistry.cpp
 * @project     Pulse Protocol
 * @author		Ban Kai Wei Benjamin - 99%
 * @author      Goh Pin Kai (secondary) - 1%
 * 
 * @brief       Implementation of a basic asset registry
 *              An asset registry in a game is a centralized system that tracks and 
 *				provides access to all the digital resources (assets) metadata
 *				in a project,  such as font, textures, anim and audio files
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              the prior written consent of DigiPen Institute of Technology
 *              is prohibited.
 */
 /******************************************************************************/

#include "AssetRegistry.h"
#include "pch/pch_temp.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

// namespace PathUtils {
// 	std::filesystem::path GetExeDir() {
// #ifdef _WIN32
// 		wchar_t buf[MAX_PATH]{};
// 		GetModuleFileNameW(nullptr, buf, MAX_PATH);
// 		return std::filesystem::path(buf).parent_path();
// #elif defined(__linux__)
// 		// Linux: /proc/self/exe points to the running executable
// 		std::error_code ec;
// 		auto p = std::filesystem::read_symlink("/proc/self/exe", ec);
// 		return ec ? std::filesystem::current_path() : p.parent_path();
// #else
// 		return std::filesystem::current_path(); // fallback if you build non-windows
// #endif
// 	}
// }

	namespace {
		void printSeparator(std::ostream& os) {
			os << "=============================\n";
		}
	}

	/*Audio Resource*/

	void AudioResource::Deserialize(const rapidjson::Value& in_obj) {
		if (!in_obj.IsObject()) {
			std::cerr << "[AudioResource] Invalid JSON format\n";
			return;
		}

		//***Essential metadata, must exist and validated 
		if (in_obj.HasMember("id") && in_obj["id"].IsString()) { audio_id = in_obj["id"].GetString(); }
		if (in_obj.HasMember("path") && in_obj["path"].IsString()) { path = in_obj["path"].GetString(); }

		//***optional metadata, 
		if (in_obj.HasMember("loop") && in_obj["loop"].IsBool()) {
			loop = in_obj["loop"].GetBool();
		}
		else {
			std::cerr << "[AudioResource] Missing 'loop' field, using default (false) for " << audio_id << "\n";
		}
		if (in_obj.HasMember("volume") && in_obj["volume"].IsNumber()) {
			volume = static_cast<float>(in_obj["volume"].GetDouble());
		}
		else {
			std::cerr << "[AudioResource] Missing 'volume' field, using default value (1.0) for "
				<< audio_id << "\n";
		}

		if (in_obj.HasMember("audiotype") && in_obj["audiotype"].IsString()) { audiotype = in_obj["audiotype"].GetString(); }
	}
	/*Texture Resource*/
	void TextureResource::Deserialize(const rapidjson::Value& in_obj) {
		if (!in_obj.IsObject()) {
			std::cerr << "[TextureResource] Invalid JSON format\n";
			return;
		}

		//***Essential metadata, must exist and validated  
		if (in_obj.HasMember("id") && in_obj["id"].IsString()) { tex_name = in_obj["id"].GetString(); }
		if (in_obj.HasMember("path") && in_obj["path"].IsString()) { path = in_obj["path"].GetString(); }

		if (in_obj.HasMember("vert_shader_path") && in_obj["vert_shader_path"].IsString()) {
			vert_shader_path = in_obj["vert_shader_path"].GetString();
		}
		if (in_obj.HasMember("frag_shader_path") && in_obj["frag_shader_path"].IsString()) {
			frag_shader_path = in_obj["frag_shader_path"].GetString();
		}

		if (in_obj.HasMember("mdl_ref") && in_obj["mdl_ref"].IsNumber()) {
			mdl_ref = in_obj["mdl_ref"].GetInt64();
		}
		if (in_obj.HasMember("shd_ref") && in_obj["shd_ref"].IsNumber()) {
			shd_ref = in_obj["shd_ref"].GetInt64();
		}
	}
	/*Font Resource*/
	void FontResource::Deserialize(const rapidjson::Value& in_obj) {
		if (!in_obj.IsObject()) {
			std::cerr << "[FontResource] Invalid JSON format\n";
			return;
		}

		//***Essential metadata, must exist and validated 
		if (in_obj.HasMember("id") && in_obj["id"].IsString()) { font_id = in_obj["id"].GetString(); }
		if (in_obj.HasMember("path") && in_obj["path"].IsString()) { path = in_obj["path"].GetString(); }
		if (in_obj.HasMember("pixelHeight") && in_obj["pixelHeight"].IsUint()) {
			pixelHeight = in_obj["pixelHeight"].GetUint();
		}
		else if (in_obj.HasMember("pixelHeight") && in_obj["pixelHeight"].IsNumber()) {
			pixelHeight = static_cast<unsigned int>(in_obj["pixelHeight"].GetDouble());
		}
	}

	/*Scene Resource*/
	void SceneResource::Deserialize(const rapidjson::Value& in_obj) {
		if (!in_obj.IsObject()) {
			std::cerr << "[SceneResource] Invalid JSON format\n";
			return;
		}

		//***Essential metadata, must exist and validated 
		if (in_obj.HasMember("id") && in_obj["id"].IsString()) { scene_id = in_obj["id"].GetString(); }
		if (in_obj.HasMember("path") && in_obj["path"].IsString()) { path = in_obj["path"].GetString(); }
	}

	/*Animation Resource*/
	void AnimationResource::Deserialize(const rapidjson::Value& in_obj) {
		if (!in_obj.IsObject()) {
			std::cerr << "[AnimationResource] Invalid JSON format\n";
			return;
		}

		//***Essential metadata, must exist and validated 
		if (in_obj.HasMember("id") && in_obj["id"].IsString()) { anim_id = in_obj["id"].GetString(); }
		if (in_obj.HasMember("path") && in_obj["path"].IsString()) { path = in_obj["path"].GetString(); }

		if (in_obj.HasMember("totalFrames") && in_obj["totalFrames"].IsNumber()) { totalFrames = in_obj["totalFrames"].GetUint(); }
		if (in_obj.HasMember("columns") && in_obj["columns"].IsNumber()) { columns = in_obj["columns"].GetUint(); }
		if (in_obj.HasMember("rows") && in_obj["rows"].IsNumber()) { rows = in_obj["rows"].GetUint(); }
		if (in_obj.HasMember("animationSpeed") && in_obj["animationSpeed"].IsNumber()) { animationSpeed = in_obj["animationSpeed"].GetFloat(); }
	}


	/*Asset Registry*/

	/*accessor*/
	std::unordered_map<std::string, FontResource> const& AssetRegistry::getFontContainer() const {
		return fontResources;
	}
	std::unordered_map<std::string, AudioResource> const& AssetRegistry::getAudioContainer() const {
		return audioResources;
	}
	std::unordered_map<std::string, TextureResource> const& AssetRegistry::getTexContainer() const {
		return textureResources;
	}

	std::unordered_map<std::string, SceneResource> const& AssetRegistry::getSceneContainer() const {
		return sceneResources;
	}

	std::unordered_map<std::string, AnimationResource> const& AssetRegistry::getAnimContainer() const {
		return animationResources;
	}

	std::string const& AssetRegistry::GetAudioJSONfilepath() const {
		return audiojsonfilepath;
	}

	std::string const& AssetRegistry::GetTextureJSONfilepath() const {
		return texturejsonfilepath;
	}

	std::string const& AssetRegistry::GetFontJSONfilepath() const {
		return fontjsonfilepath;
	}

	std::string const& AssetRegistry::GetSceneJSONfilepath() const {
		return scenejsonfilepath;
	}

	std::string const& AssetRegistry::GetAnimationJSONfilepath() const {
		return animationjsonfilepath;
	}

	void AssetRegistry::SetAssetPaths(std::filesystem::path const& pulseEnginePath)
	{
		basePath = pulseEnginePath;
		std::cout << "[AssetRegistry] basePath=" << basePath.string() << "\n";
		audiojsonfilepath = (pulseEnginePath / "JSON" / "audio.json").string();
		texturejsonfilepath = (pulseEnginePath / "JSON" / "texture.json").string();
		fontjsonfilepath = (pulseEnginePath / "JSON" / "font.json").string();
		scenejsonfilepath = (pulseEnginePath / "JSON" / "scene.json").string();
		animationjsonfilepath = (pulseEnginePath / "JSON" / "animation.json").string();
	}

	AudioResource  const* AssetRegistry::FindAudio(std::string const& id)  const {
		if (auto it = audioResources.find(id); it != audioResources.end()) {
			return &it->second;
		}
		return nullptr;
	}
	TextureResource const* AssetRegistry::FindTexture(std::string const& id) const {
		if (auto it = textureResources.find(id); it != textureResources.end()) {
			return &it->second;
		}
		return nullptr;
	}
	FontResource   const* AssetRegistry::FindFont(std::string const& id)   const {
		if (auto it = fontResources.find(id); it != fontResources.end()) {
			return &it->second;
		}
		return nullptr;
	}

	SceneResource   const* AssetRegistry::FindScene(std::string const& id)   const {
		if (auto it = sceneResources.find(id); it != sceneResources.end()) {
			return &it->second;
		}
		return nullptr;
	}

	AnimationResource   const* AssetRegistry::FindAnimation(std::string const& id)   const {
		if (auto it = animationResources.find(id); it != animationResources.end()) {
			return &it->second;
		}
		return nullptr;
	}

	/*default cons for debug*/
	//AssetRegistry::AssetRegistry() {
	//	std::cout << "AssetRegistry Initialised" << "\n";
	//}

	bool AssetRegistry::LoadAudioManifest(std::string const& jsonPath) {
		std::ifstream ifs(jsonPath);
		if (!ifs.is_open()) {
			std::cerr << "Error: Cannot open file for reading: " << jsonPath << "\n";
			return false;
		}
		rapidjson::IStreamWrapper isw(ifs);
		rapidjson::Document d;
		//d.ParseStream(isw);
		//this means parse data from JSON file ignore comments 
		//in areas where, whitespaces. e.g. before or after tokens
		d.ParseStream<rapidjson::kParseCommentsFlag>(isw);
		if (d.HasParseError()) {
		std::cerr << "Error parsing " << jsonPath << ": "
					<< rapidjson::GetParseError_En(d.GetParseError())
					<< " at offset " << d.GetErrorOffset() << "\n";
		return false;
		}
		if (!d.HasMember("audioFilePaths")) {
			std::cerr << "Error: No 'audioFilePaths' field in file: " << jsonPath << "\n";
			return false;
		}

		//get the array of json file
		auto const& arr = d["audioFilePaths"].GetArray();
		//variables to count successful parsing and failed parsing
		size_t added = 0, skipped = 0;

		for (const auto& entry : arr) {

			AudioResource aud_res;
			aud_res.Deserialize(entry);

			/*	if (!aud_res.audio_id.empty()) {
					audioResources.emplace(aud_res.audio_id, std::move(aud_res));
				}*/

			if (aud_res.audio_id.empty() || aud_res.path.empty()) {
				++skipped;
				std::cerr << "[Audio] Skipping entry with missing id/path\n";
				continue;
			}
			//insert into map assign into variable type
			//std::pair<std::unordered_map<std::string, AudioResource>::iterator, bool>
			//ok refers to successful flag

			std::cout << "audio id : " << aud_res.audio_id << "\n";

			auto [it, ok] = audioResources.emplace(aud_res.audio_id, std::move(aud_res));
	
			if (!ok) {
				++skipped;
				std::cerr << "[Audio] Duplicate id =" << it->first << "= error skipping\n";
			}
			else {
				++added;
			}
		}// end of for-range to load data into container


		std::cout << "No. of [Audio] item(s) Loaded :" << added << "\n"
			<< "No. item(s), skipped " << skipped << "\n"
			<< "Loaded from :" << jsonPath << "\n";

		return true;
	}

	bool AssetRegistry::LoadTextureManifest(std::string const& jsonPath) {
		std::ifstream ifs(jsonPath);
		if (!ifs.is_open()) {
			std::cerr << "Error: Cannot open file for reading: " << jsonPath << "\n";
			return false;
		}
		rapidjson::IStreamWrapper isw(ifs);
		rapidjson::Document d;
		//d.ParseStream(isw);
		//this means parse data from JSON file ignore comments 
		//in areas where, whitespaces. e.g. before or after tokens
		d.ParseStream<rapidjson::kParseCommentsFlag>(isw);
		if (d.HasParseError()) {
		std::cerr << "Error parsing " << jsonPath << ": "
					<< rapidjson::GetParseError_En(d.GetParseError())
					<< " at offset " << d.GetErrorOffset() << "\n";
		return false;
		}

		if (!d.HasMember("textureFilePaths")) {
			std::cerr << "Error: No 'textureFilePaths' field in file: " << jsonPath << "\n";
			return false;
		}

		//get the array of json file
		auto const& arr = d["textureFilePaths"].GetArray();
		//variables to count successful parsing and failed parsing
		size_t added = 0, skipped = 0;

		for (const auto& entry : arr) {

			TextureResource tex_res;
			tex_res.Deserialize(entry);

			if (tex_res.tex_name.empty() || tex_res.path.empty()) {
				++skipped;
				std::cerr << "[Texture] Skipping entry with missing id/path\n";
				continue;
			}
			auto [it, ok] = textureResources.emplace(tex_res.tex_name, std::move(tex_res));
			if (!ok) {
				++skipped;
				std::cerr << "[Texture] Duplicate id =" << it->first << "= error skipping\n";
			}
			else {
				++added;
				std::cout << "[Texture] Added: id='" << it->second.tex_name
					<< "' path='" << it->second.path << "'\n";
			}
		}// end of for-range to load data into container

		std::cout << "No. of [Texture] item(s) Loaded :" << added << "\n"
			<< "No. item(s), skipped " << skipped << "\n"
			<< "Loaded from :" << jsonPath << "\n";

		return true;
	}



	bool AssetRegistry::LoadFontManifest(std::string const& jsonPath) {
		std::ifstream ifs(jsonPath);
		if (!ifs.is_open()) {
			std::cerr << "Error: Cannot open file for reading: " << jsonPath << "\n";
			return false;
		}
		rapidjson::IStreamWrapper isw(ifs);
		rapidjson::Document d;
		//d.ParseStream(isw);
		//this means parse data from JSON file ignore comments 
		//in areas where, whitespaces. e.g. before or after tokens
		d.ParseStream<rapidjson::kParseCommentsFlag>(isw);
		if (d.HasParseError()) {
		std::cerr << "Error parsing " << jsonPath << ": "
					<< rapidjson::GetParseError_En(d.GetParseError())
					<< " at offset " << d.GetErrorOffset() << "\n";
		return false;
		}

		if (!d.HasMember("fontFilePaths")) {
			std::cerr << "Error: No 'fontFilePaths' field in file: " << jsonPath << "\n";
			return false;
		}

		//get the array of json file
		auto const& arr = d["fontFilePaths"].GetArray();
		//variables to count successful parsing and failed parsing
		size_t added = 0, skipped = 0;

		for (const auto& entry : arr) {

			FontResource font_res;
			font_res.Deserialize(entry);

			if (font_res.font_id.empty() || font_res.path.empty()) {
				++skipped;
				std::cerr << "[Font] Skipping entry with missing id/path\n";
				continue;
			}
			auto [it, ok] = fontResources.emplace(font_res.font_id, std::move(font_res));
			if (!ok) {
				++skipped;
				std::cerr << "[Font] Duplicate id =" << it->first << "= error skipping\n";
			}
			else {
				++added;
				std::cout << "[Font] Added: id='" << it->second.font_id
					<< "' path='" << it->second.path << "'\n";
			}

		}// end of for-range to load data into container

		std::cout << "No. of [Font] item(s) Loaded :" << added << "\n"
			<< "No. item(s), skipped " << skipped << "\n"
			<< "Loaded from :" << jsonPath << "\n";

		


		return true;
	}


	bool AssetRegistry::LoadSceneManifest(std::string const& jsonPath) {
		std::ifstream ifs(jsonPath);
		if (!ifs.is_open()) {
			std::cerr << "Error: Cannot open file for reading: " << jsonPath << "\n";
			return false;
		}
		rapidjson::IStreamWrapper isw(ifs);
		rapidjson::Document d;
		//d.ParseStream(isw);
		//this means parse data from JSON file ignore comments 
		//in areas where, whitespaces. e.g. before or after tokens
		d.ParseStream<rapidjson::kParseCommentsFlag>(isw);
		if (d.HasParseError()) {
		std::cerr << "Error parsing " << jsonPath << ": "
					<< rapidjson::GetParseError_En(d.GetParseError())
					<< " at offset " << d.GetErrorOffset() << "\n";
		return false;
		}

		if (!d.HasMember("sceneFilePaths")) {
			std::cerr << "Error: No 'sceneFilePaths' field in file: " << jsonPath << "\n";
			return false;
		}

		//get the array of json file
		auto const& arr = d["sceneFilePaths"].GetArray();
		//variables to count successful parsing and failed parsing
		size_t added = 0, skipped = 0;

		for (const auto& entry : arr) {

			SceneResource scene_res;
			scene_res.Deserialize(entry);

			if (scene_res.scene_id.empty() || scene_res.path.empty()) {
				++skipped;
				std::cerr << "[Scene] Skipping entry with missing id/path\n";
				continue;
			}
			auto [it, ok] = sceneResources.emplace(scene_res.scene_id, std::move(scene_res));
			if (!ok) {
				++skipped;
				std::cerr << "[Scene] Duplicate id =" << it->first << "= error skipping\n";
			}
			else {
				++added;
			}
		}// end of for-range to load data into container

		std::cout << "No. of [Scene] item(s) Loaded :" << added << "\n"
			<< "No. item(s), skipped " << skipped << "\n"
			<< "Loaded from :" << jsonPath << "\n";

		return true;
	}

	bool AssetRegistry::LoadAnimationManifest(std::string const& jsonPath) {
		std::ifstream ifs(jsonPath);
		if (!ifs.is_open()) {
			std::cerr << "Error: Cannot open file for reading: " << jsonPath << "\n";
			return false;
		}
		rapidjson::IStreamWrapper isw(ifs);
		rapidjson::Document d;
		//d.ParseStream(isw);
		//this means parse data from JSON file ignore comments 
		//in areas where, whitespaces. e.g. before or after tokens
		d.ParseStream<rapidjson::kParseCommentsFlag>(isw);
		if (d.HasParseError()) {
		std::cerr << "Error parsing " << jsonPath << ": "
					<< rapidjson::GetParseError_En(d.GetParseError())
					<< " at offset " << d.GetErrorOffset() << "\n";
		return false;
		}

		if (!d.HasMember("animationFilePaths")) {
			std::cerr << "Error: No 'animationFilePaths' field in file: " << jsonPath << "\n";
			return false;
		}

		//get the array of json file
		auto const& arr = d["animationFilePaths"].GetArray();
		//variables to count successful parsing and failed parsing
		size_t added = 0, skipped = 0;

		for (const auto& entry : arr) {

			AnimationResource anim_res;
			anim_res.Deserialize(entry);

			if (anim_res.anim_id.empty() || anim_res.path.empty()) {
				++skipped;
				std::cerr << "[Animation] Skipping entry with missing id/path\n";
				continue;
			}
			auto [it, ok] = animationResources.emplace(anim_res.anim_id, std::move(anim_res));
			if (!ok) {
				++skipped;
				std::cerr << "[Animation] Duplicate id =" << it->first << "= error skipping\n";
			}
			else {
				++added;
			}
		}// end of for-range to load data into container

		std::cout << "No. of [Animation] item(s) Loaded :" << added << "\n"
			<< "No. item(s), skipped " << skipped << "\n"
			<< "Loaded from :" << jsonPath << "\n";

		return true;
	}

	bool AssetRegistry::LoadAudioManifest() {
		audioResources.clear();
		return LoadAudioManifest(audiojsonfilepath);
	}
	bool AssetRegistry::LoadTextureManifest() {
		textureResources.clear();
		return LoadTextureManifest(texturejsonfilepath);
	}
	bool AssetRegistry::LoadFontManifest() {
		fontResources.clear();
		return LoadFontManifest(fontjsonfilepath);
	}

	bool AssetRegistry::LoadSceneManifest() {
		sceneResources.clear();
		return LoadSceneManifest(scenejsonfilepath);
	}

	bool AssetRegistry::LoadAnimationManifest() {
		animationResources.clear();
		return LoadAnimationManifest(animationjsonfilepath);
	}


	std::string AssetRegistry::GetAudioItemPath(std::string const& id)  const {
		auto* res = FindAudio(id);
		if (!res) return "";

		//  If path starts with ../.. use basePath, otherwise use as-is
		std::filesystem::path p(res->path);
		if (p.is_absolute()) return res->path;
		return (basePath / p).lexically_normal().string();
	}
	std::string AssetRegistry::GetTextureItemPath(std::string const& id) const {
		auto* res = FindTexture(id);
		if (!res) {
			//std::cout << "[DEBUG] GetTextureItemPath('" << id << "'): NOT FOUND in registry!\n";
			return "";
		}

		//std::cout << "[DEBUG] GetTextureItemPath('" << id << "'):\n";
		//std::cout << "  basePath: " << basePath << "\n";
		//std::cout << "  res->path: " << res->path << "\n";

		std::filesystem::path p(res->path);
		if (p.is_absolute()) return res->path;
		return (basePath / p).lexically_normal().string();

	}
	std::string AssetRegistry::GetFontItemPath(std::string const& id)   const {
		auto* res = FindFont(id);
		if (!res) {
			//std::cout << "[DEBUG] GetFontItemPath('" << id << "'): NOT FOUND in registry!\n";
			return "";
		}

		//std::cout << "[DEBUG] GetFontItemPath('" << id << "'):\n";
		//std::cout << "  basePath: " << basePath << "\n";
		//std::cout << "  res->path: " << res->path << "\n";	

		std::filesystem::path p(res->path);
		if (p.is_absolute()) return res->path;
		return (basePath / p).lexically_normal().string();
	}
	std::string AssetRegistry::GetSceneItemPath(std::string const& id)   const {
		auto* res = FindScene(id);
		if (!res) return "";

		std::filesystem::path p(res->path);
		if (p.is_absolute()) return res->path;
		return (basePath / p).lexically_normal().string();
	}
	std::string AssetRegistry::GetAnimationItemPath(std::string const& id)   const {
		auto* res = FindAnimation(id);
		if (!res) return "";

		std::filesystem::path p(res->path);
		if (p.is_absolute()) return res->path;
		return (basePath / p).lexically_normal().string();
	}


	/*For Debug and Console Init print all container element*/

	std::ostream& operator<<(std::ostream& os, AudioResource const& aud_r) {
		os << "[AudioResource]"
			<< " id='" << aud_r.audio_id << "'"
			<< " path='" << aud_r.path << "'"
			<< " loop=" << std::boolalpha << aud_r.loop
			<< " volume=" << aud_r.volume
			<< " type='" << aud_r.audiotype << "'";
		return os;
	}
	std::ostream& operator<<(std::ostream& os, TextureResource const& tr) {
		os << "[TextureResource]"
			<< " id='" << tr.tex_name << "'"
			<< " path='" << tr.path << "'"
			<< " vert_shader_path='" << tr.vert_shader_path << "'"
			<< " frag_shader_path='" << tr.frag_shader_path << "'"
			<< " mdl_ref='" << tr.mdl_ref << "'"
			<< " shd_ref='" << tr.shd_ref << "'";
		return os;
	}
	std::ostream& operator<<(std::ostream& os, FontResource const& fr) {
		os << "[FontResource]"
			<< " id='" << fr.font_id << "'"
			<< " path='" << fr.path << "'"
			<< " pixelHeight=" << fr.pixelHeight;
		return os;
	}
	std::ostream& operator<<(std::ostream& os, SceneResource const& sr) {
		os << "[SceneResource]"
			<< " id='" << sr.scene_id << "'"
			<< " path='" << sr.path << "'";
		return os;
	}
	std::ostream& operator<<(std::ostream& os, AnimationResource const& anim_r) {
		os << "[AnimationResource]"
			<< " id='" << anim_r.anim_id
			<< "' path='" << anim_r.path
			<< "' frames=" << anim_r.totalFrames
			<< " cols=" << anim_r.columns
			<< " rows=" << anim_r.rows
			<< " speed=" << anim_r.animationSpeed;
		return os;
	}


	std::ostream& operator<<(std::ostream& os, AssetRegistry const& ar) {

		os << "===Asset Registry Loaded: ===\n";
		os << "Audio:  \n";
		printSeparator(os);
		for (auto const& [id, aud] : ar.getAudioContainer()) {
			os << id << "->" << aud.path << "\n";
			os << "loop: " << aud.loop << "\n";
			os << "volume: " << aud.volume << "\n";
		}
		printSeparator(os);
		os << "Texture:  \n";
		printSeparator(os);
		for (auto const& [id, tex] : ar.getTexContainer()) {
			os << id << "->" << tex.path << "\n";
		}
		printSeparator(os);
		os << "Fonts: \n";
		printSeparator(os);
		for (auto const& [id, font] : ar.getFontContainer()) {
			os << id << "->" << font.path << "\n";
		}
		printSeparator(os);

		os << "Scene: \n";
		printSeparator(os);
		for (auto const& [id, scene] : ar.getSceneContainer()) {
			os << id << "->" << scene.path << "\n";
		}
		printSeparator(os);

		os << "Animation: \n";
		printSeparator(os);
		for (auto const& [id, scene] : ar.getAnimContainer()) {
			os << id << "->" << scene.path << "\n";
		}
		printSeparator(os);
		return os;
	}


	//
	std::filesystem::path const& AssetRegistry::GetBasePath() const {
		return basePath;
	}
