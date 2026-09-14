/******************************************************************************/
/**
 * @file        SceneManifest.cpp
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin 
 * 
 * @brief       Parses and writes scene manifest JSON, storing scene asset lists 
 *              and shared asset groups for loading and saving
 *              
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/


#include "pch/pch_temp.h"
#include "SceneManifest.h"
#include "Serialization/Serialization.h"

namespace
{
    //Write to JSON file, vector to array(json)
    void WriteStringVector(rapidjson::Value& out,
        char const* key,
        std::vector<std::string> const& data,
        rapidjson::Document::AllocatorType& alloc)
    {
        rapidjson::Value arr(rapidjson::kArrayType);

        for (auto const& item : data)
        {
            rapidjson::Value strVal; 
            //assign data from vector directly into JSON obj
            strVal.SetString(item.c_str(),
                static_cast<rapidjson::SizeType>(item.size()),
                alloc);
            arr.PushBack(strVal, alloc);
        }
        //keyname e.g texture , audio
        rapidjson::Value keyName;
        keyName.SetString(key,
            static_cast<rapidjson::SizeType>(std::strlen(key)),
            alloc);

        out.AddMember(keyName, arr, alloc);
    }

    //Read from JSON file, array(json) to vector 
    void ReadStringVector(rapidjson::Value const& in,
        char const* key,
        std::vector<std::string>& outVec)
    {
        outVec.clear();

        if (!in.HasMember(key) || !in[key].IsArray()) {
            std::cout << "Does not contain any valid key" << key << "\n";
            return; //early return
        }
        //store in the container    
        auto const& arr = in[key];
        for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
        {
            if (arr[i].IsString()) {
                outVec.emplace_back(arr[i].GetString());
            }
                
        }
    }
}

//serialize/deserialize
void SceneManifest::Serialize(rapidjson::Value& out,
    rapidjson::Document::AllocatorType& alloc) const {

    out.SetObject();

    rapidjson::Value sceneIdVal;
    sceneIdVal.SetString(sceneName.c_str(),
        static_cast<rapidjson::SizeType>(sceneName.size()),
        alloc);
    out.AddMember("sceneName", sceneIdVal, alloc);

    WriteStringVector(out, "textures", textures, alloc);
    WriteStringVector(out, "audio", audio, alloc);
    WriteStringVector(out, "fonts", fonts, alloc);
    WriteStringVector(out, "animations", animations, alloc);
    WriteStringVector(out, "sharedGroups", sharedGroups, alloc);
    //WriteStringVector(out, "prefabs", prefabs, alloc);
}

void SceneManifest::Deserialize(rapidjson::Value const& in) {
    if (!in.IsObject())
        return;

    if (in.HasMember("sceneName") && in["sceneName"].IsString())
        sceneName = in["sceneName"].GetString();
    else
        sceneName.clear();

    ReadStringVector(in, "textures", textures);
    ReadStringVector(in, "audio", audio);
    ReadStringVector(in, "fonts", fonts);
    ReadStringVector(in, "animations", animations);
    ReadStringVector(in, "sharedGroups", sharedGroups);
    //ReadStringVector(in, "prefabs", prefabs);
}
//load/save
bool LoadSceneManifestFile(SceneManifest& sm,std::string const& path) {
    return JSONUtils::LoadObject(sm, path, "scenemanifest");
}
bool SaveSceneManifestFile(SceneManifest const& sm, std::string const& path) {
    return JSONUtils::SaveObject(sm, path, "scenemanifest");
}

