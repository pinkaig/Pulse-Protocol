/******************************************************************************/
/**
 * @file        Serialization.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Declares serialization and deserialization interfaces for engine
 *              configuration (Config) and entities (e.g., NPC). Provides
 *              save/load utility function prototypes for writing and reading
 *              JSON-based game data files.
 *	            
 *			    c++ obj -> json text -> file
 * 
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              the prior written consent of DigiPen Institute of Technology is
 *              prohibited.
 */
 /******************************************************************************/

#pragma once
#//include <pch/pch.h>
//#include "../PulseEngine/CoreEngine/ECS/Types.h" // ensures Entity/Signature are known  <-- (from Types.h)
#include "CoreEngine/Core/ImportExport.h"
#include <rapidjson/ostreamwrapper.h>

//#include <rapidjson/document.h>     // for Document, Value, AllocatorType in pch tmp
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h> // for StringBuffer
#include <rapidjson/error/en.h> // GetParseError_En
#include "pch/pch_temp.h"


class DLL_API Config
{
private:
	int WindowWidth = 0;
	int WindowHeight = 0;
	//std::string ConfigPath = "../../PulseEngine/JSON/config.json";
    std::string ConfigPath = "";
    std::string FirstScene = "";
public:
	int GetWindowWidth() const;
	int GetWindowHeight() const;
	std::string GetConfigPath() const;
    std::string GetFirstScenePath() const;
	void SetWindowWidth(int w);
	void SetWindowHeight(int h);
	void SetConfigPath(std::string path);
    void SetFirstScenePath(std::string path);
	void Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const;
	void Deserialize(const rapidjson::Value &in);
};

DLL_API bool SaveConfig(Config const &cfg, std::string const &path, bool pretty = true);

//bool SaveConfig(Config const& cfg) {
//	return SaveConfig(cfg, cfg.GetConfigPath());
//}
//bool SaveConfig(Config const& cfg, std::string const& path);
DLL_API bool LoadConfig(Config &cfg, std::string const &path);

namespace JSONUtils {
	using namespace rapidjson;
    //using rapidjson::Document;
    //using rapidjson::Value;
    //using rapidjson::Writer;
    //using rapidjson::PrettyWriter;
    //using rapidjson::StringBuffer;
    //using rapidjson::SizeType;
    //using rapidjson::kObjectType;

    //bool pretty means use PrettyWriter or just Writer
	//PrettyWriter takes more file space but more readable
	//Writer makes everything one liner
	std::string JSONToString(Document const& d, bool pretty);

    template <typename T>
	Document MakeObjDocument(T const& object, std::string const& memberName) {
		Document d(kObjectType);
		auto& alloc = d.GetAllocator();

		Value Val(kObjectType);
		object.Serialize(Val, alloc);
		//void AddMember(Value& name, Value& value, Allocator& allocator);
		//instead of d.AddMember("transform", tVal, alloc);
		//we use like this

		//same thing more constructor call
	   //Value key(memberName.c_str(), static_cast<SizeType>(memberName.size()), alloc);
	   // d.AddMember(key,Val, alloc);

		d.AddMember(
			Value(memberName.c_str(), static_cast<SizeType>(memberName.size()), alloc),
			Val,
			alloc);

		return d; // NRVO (construct in caller) or move if optimization skipped
	}

    //bool pretty means use PrettyWriter or just Writer
    //PrettyWriter takes more file space but more readable
    //Writer makes everything one liner
    bool WriteJSONToFile(Document const& d, std::filesystem::path const& path, bool pretty);

    //load
    //inline bool ReadJSONFromFile(Document& outDoc,
    //    std::filesystem::path const& path)
    //{
    //    std::ifstream ifs(path, std::ios::binary);
    //    if (!ifs)
    //    {
    //        std::cerr << "Error: Cannot open file for reading: " << path << "\n";
    //        return false;
    //    }

    //    IStreamWrapper isw(ifs);
    //    outDoc.ParseStream(isw);

    //    if (outDoc.HasParseError())
    //    {
    //        std::cerr << "Error: JSON parse error in " << path
    //            << " : " << GetParseError_En(outDoc.GetParseError())
    //            << " (offset " << outDoc.GetErrorOffset() << ")\n";
    //        return false;
    //    }
    //    return true;
    //}
    bool ReadJSONFromFile(Document& outDoc,std::filesystem::path const& path);

    // ---------------------------
    // optimised SAVE / LOAD
    // ---------------------------
    template <typename T>
    bool SaveObject(T const& obj,
        std::filesystem::path const& path,
        std::string const& rootName,
        bool pretty = true)
    {
        Document d = MakeObjDocument(obj, rootName);
        return WriteJSONToFile(d, path, pretty);
    }

    template <typename T>
    bool LoadObject(T& obj,
        std::filesystem::path const& path,
        std::string const& rootName)
    {
        Document d;
        if (!ReadJSONFromFile(d, path))
            return false;
        if (!d.IsObject())
        {
            std::cerr << "Error: Top-level JSON is not an object: " << path << "\n";
            return false;
        }

        auto it = d.FindMember(rootName.c_str());
        if (it == d.MemberEnd())
        {
            std::cerr << "Error: Missing root '" << rootName
                << "' in file: " << path << "\n";
            return false;
        }

        obj.Deserialize(d[rootName.c_str()]);
        return true;
    }


}//end of namespace JSONUtils