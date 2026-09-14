/******************************************************************************/
/**
 * @file        SceneManifest.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin 
 * 
 * @brief       Scene manifest storing asset registry IDs required by a scene, 
 *              with JSON serialization and file load/save 
 *              
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <rapidjson/document.h>
struct SceneManifest
{
    std::string sceneName;
    std::vector<std::string> textures;
    std::vector<std::string> audio;
    std::vector<std::string> fonts;
    std::vector<std::string> animations;
    std::vector<std::string> sharedGroups;
     //std::vector<std::string> prefabs; 

    void Serialize(rapidjson::Value& out,
        rapidjson::Document::AllocatorType& alloc) const;

    void Deserialize(rapidjson::Value const& in);


};
bool SaveSceneManifestFile(SceneManifest const& sm, std::string const& path);
bool LoadSceneManifestFile(SceneManifest& sm, std::string const& path);


//bool LoadSceneManifestFile(std::string const& path, SceneManifest& outManifest);
//bool SaveSceneManifestFile(std::string const& path, SceneManifest const& manifest);