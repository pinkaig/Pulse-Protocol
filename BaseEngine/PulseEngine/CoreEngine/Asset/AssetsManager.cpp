/******************************************************************************/
/**
 * @file        AssetsManager.cpp
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin (primary) - 90%
 * @author      Leu Jun Yong (secondary) - 10%
 * 
 * @brief       Handles loading, retrieving, and unloading of audio (FMOD)
 *              and texture (OpenGL) assets.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#include "AssetsManager.h"
#include "Graphics/GraphicsManager.h"
#include "Audio/AudioManager.h"
// namespace Framework {
#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <Serialization/Serialization.h>

AssetsManager::~AssetsManager()
{
    Shutdown();
}

void AssetsManager::Initialize()
{
    // 1) Load manifests first (registry has metadata)
    mRegistry.LoadAudioManifest();
    mRegistry.LoadTextureManifest();
    mRegistry.LoadFontManifest();
    mRegistry.LoadSceneManifest();
    mRegistry.LoadAnimationManifest();

    //mGraphics->Initialize(); // already called in constructor

    std::cout << "[AssetsManager] Initialized complete.\n";
}

void AssetsManager::Shutdown()
{
    // Registry is POD metadata; no special teardown needed
    // Registry no external dependences, constructed
    // and destroyed automatically by compiler(RAII)
    std::cout << "[AssetsManager] Shutdown complete.\n";
}

AssetRegistry &AssetsManager::getRegistry() { return mRegistry; }
AudioManager &AssetsManager::getAudio() { return *mAudio; }
AudioManager const &AssetsManager::getAudio() const { return *mAudio; }

GraphicsManager &AssetsManager::getGraphics() { return *mGraphics; }
GraphicsManager const &AssetsManager::getGraphics() const { return *mGraphics; }

unsigned int AssetsManager::GetOrLoadTexture(const std::string &name)
{
    return mGraphics ? mGraphics->GetOrLoad(name) : 0u;
}

bool AssetsManager::EvictTexture(std::string const& idOrPath)
{
    return mGraphics ? mGraphics->EvictTexture(idOrPath) : false;
}

std::string AssetsManager::GetAudioPath(std::string const& id)  const {
    return mRegistry.GetAudioItemPath(id);
}
std::string AssetsManager::GetTexturePath(std::string const& id) const {
    return mRegistry.GetTextureItemPath(id);
}
std::string AssetsManager::GetFontPath(std::string const& id)   const {
    return mRegistry.GetFontItemPath(id);
}
std::string AssetsManager::GetScenePath(std::string const& id)   const {
    return mRegistry.GetSceneItemPath(id);
}
std::string AssetsManager::GetAnimationPath(std::string const& id)   const {
    return mRegistry.GetAnimationItemPath(id);
}

std::unordered_map<std::string, AudioResource> const& AssetsManager::getAudioRegContainer() const {
    return mRegistry.getAudioContainer();
}

std::unordered_map<std::string, TextureResource> const& AssetsManager::getTexRegContainer() const {
    return mRegistry.getTexContainer();
}
std::unordered_map<std::string, FontResource>const& AssetsManager::getFontRegContainer() const {
    return mRegistry.getFontContainer();
}
std::unordered_map<std::string, SceneResource> const& AssetsManager::getSceneRegContainer() const {
    return mRegistry.getSceneContainer();
}
std::unordered_map<std::string, AnimationResource>const& AssetsManager::getAnimRegContainer() const {
    return mRegistry.getAnimContainer();
}

bool AssetsManager::HasAudio(std::string const& id)const {
    return getAudioRegContainer().contains(id);
}
bool AssetsManager::HasTexture(std::string const& id)const {
    return getTexRegContainer().contains(id);
}
bool AssetsManager::HasFont(std::string const& id)const {
    return getFontRegContainer().contains(id);
}
bool AssetsManager::HasScene(std::string const& id) const {
    return getSceneRegContainer().contains(id);
}
bool AssetsManager::HasAnimation(std::string const& id)const {
    return getAnimRegContainer().contains(id);
}

bool AssetsManager::PlaySFX(std::string const& id) {
    if (mAudio) {
        mAudio->PlaySound(id);  
        return true;                 
    }
    return false;                 
}

bool AssetsManager::PlayMusic(std::string const& id) {
    if (mAudio) {
        mAudio->PlayBGM(id);
        return true;
    }
    return false;
}

void AssetsManager::SetAudioManager(AudioManager* audio)
{
    mAudio = audio;
}

void AssetsManager::SetGraphicsManager(GraphicsManager* graphics)
{
    mGraphics = graphics;
}


bool AssetsManager::PreloadSceneAssets(SceneManifest const& manifest)
{
    bool allOk = true;

    std::cout << "\n[AssetsManager] ===== Preloading Scene Assets =====\n";
    std::cout << "[AssetsManager] Scene: " << manifest.sceneName << "\n";

    // =========================================================
    // Textures
    // =========================================================
    if (!mGraphics)
    {
        std::cout << "[AssetsManager] [FAIL] GraphicsManager is null. Cannot preload textures.\n";
        allOk = false;
    }
    else
    {
        std::cout << "[AssetsManager] Textures:\n";
        for (auto const& texId : manifest.textures)
        {
            std::string texPath = GetTexturePath(texId);

            if (texPath.empty())
            {
                std::cout << "  [FAIL] Texture ID not found: " << texId << "\n";
                allOk = false;
                continue;
            }

            unsigned int handle = mGraphics->GetOrLoad(texId);
            if (handle == 0u)
            {
                std::cout << "  [FAIL] Texture load failed"
                    << " | id: " << texId
                    << " | path: " << texPath << "\n";
                allOk = false;
            }
            else
            {
                std::cout << "  [OK] Loaded Texture"
                    << " | id: " << texId
                    << " | path: " << texPath
                    << " | handle: " << handle << "\n";
            }
        }
    }

    // =========================================================
    // Audio
    // =========================================================
    if (!mAudio)
    {
        std::cout << "[AssetsManager] [FAIL] AudioManager is null. Cannot preload audio.\n";
        allOk = false;
    }
    else
    {
        std::cout << "[AssetsManager] Audio:\n";
        for (auto const& audioId : manifest.audio)
        {
            if (!HasAudio(audioId))
            {
                std::cout << "  [FAIL] Audio ID not found: " << audioId << "\n";
                allOk = false;
                continue;
            }

            auto const& table = getAudioRegContainer();
            auto it = table.find(audioId);
            if (it == table.end())
            {
                std::cout << "  [FAIL] Audio registry lookup failed: " << audioId << "\n";
                allOk = false;
                continue;
            }

            std::string const& audioType = it->second.audiotype;
            std::string audioPath = GetAudioPath(audioId);

            bool ok = false;
            if (audioType == "BGM")
                ok = mAudio->LoadBGM(audioId);
            else if (audioType == "SFX")
                ok = mAudio->LoadSFX(audioId);
            else
            {
                std::cout << "  [FAIL] Unknown audio type"
                    << " | id: " << audioId
                    << " | type: " << audioType << "\n";
                allOk = false;
                continue;
            }

            if (!ok)
            {
                std::cout << "  [FAIL] Audio load failed"
                    << " | id: " << audioId
                    << " | type: " << audioType
                    << " | path: " << audioPath << "\n";
                allOk = false;
            }
            else
            {
                std::cout << "  [OK] Loaded Audio"
                    << " | id: " << audioId
                    << " | type: " << audioType
                    << " | path: " << audioPath << "\n";
            }
        }
    }

    // =========================================================
    // Fonts
    // =========================================================
    if (!mGraphics)
    {
        std::cout << "[AssetsManager] [FAIL] GraphicsManager is null. Cannot validate fonts.\n";
        allOk = false;
    }
    else
    {
        std::cout << "[AssetsManager] Fonts:\n";
        for (auto const& fontId : manifest.fonts)
        {
            std::string fontPath = GetFontPath(fontId);

            if (fontPath.empty())
            {
                std::cout << "  [FAIL] Font ID not found: " << fontId << "\n";
                allOk = false;
                continue;
            }

            if (!mGraphics->GM_FindFont(fontId))
            {
                std::cout << "  [FAIL] Font not loaded/found"
                    << " | id: " << fontId
                    << " | path: " << fontPath << "\n";
                allOk = false;
            }
            else
            {
                std::cout << "  [OK] Ready Font"
                    << " | id: " << fontId
                    << " | path: " << fontPath << "\n";
            }
        }
    }

    std::cout << "[AssetsManager] ===== Preload Complete ===== "
        << (allOk ? "[SUCCESS]" : "[PARTIAL/FAILED]") << "\n\n";

    return allOk;
}
bool AssetsManager::UnloadSceneAssets(SceneManifest const& manifest)
{
    bool allOk = true;

    std::cout << "\n[AssetsManager] ===== Unloading Scene Assets =====\n";
    std::cout << "[AssetsManager] Scene: " << manifest.sceneName << "\n";

    // =========================================================
    // Textures
    // =========================================================
    if (!mGraphics)
    {
        std::cout << "[AssetsManager] [FAIL] GraphicsManager is null. Cannot unload textures.\n";
        allOk = false;
    }
    else
    {
        std::cout << "[AssetsManager] Textures:\n";
        for (auto const& texId : manifest.textures)
        {
            std::string texPath = GetTexturePath(texId);

            if (IsTextureInAnySharedGroup(texId))
            {
                std::cout << "  [SKIP] Shared texture remains loaded"
                    << " | id: " << texId
                    << " | path: " << texPath << "\n";
                continue;
            }

            if (!mGraphics->EvictTexture(texId))
            {
                std::cout << "  [FAIL] Failed to unload texture"
                    << " | id: " << texId
                    << " | path: " << texPath << "\n";
                allOk = false;
            }
            else
            {
                std::cout << "  [OK] Unloaded Texture"
                    << " | id: " << texId
                    << " | path: " << texPath << "\n";
            }
        }
    }

    // =========================================================
    // Audio
    // =========================================================
    if (!mAudio)
    {
        std::cout << "[AssetsManager] [FAIL] AudioManager is null. Cannot unload audio.\n";
        allOk = false;
    }
    else
    {
        std::cout << "[AssetsManager] Audio:\n";
        for (auto const& audioId : manifest.audio)
        {
            if (IsAudioSFX(audioId))
            {
                std::cout << "  [SKIP] Global SFX loaded on engine start"
                    << " | id: " << audioId
                    << " | path: " << GetAudioPath(audioId) << "\n";
                continue;
            }

            if (!IsAudioBGM(audioId))
            {
                std::cout << "  [SKIP] Unknown audio type"
                    << " | id: " << audioId
                    << " | path: " << GetAudioPath(audioId) << "\n";
                continue;
            }

            if (!mAudio->UnloadAudio(audioId))
            {
                std::cout << "  [FAIL] Failed to unload BGM"
                    << " | id: " << audioId
                    << " | path: " << GetAudioPath(audioId) << "\n";
                allOk = false;
            }
            else
            {
                std::cout << "  [OK] Unloaded BGM"
                    << " | id: " << audioId
                    << " | path: " << GetAudioPath(audioId) << "\n";
            }
        }
    }

    // =========================================================
    // Fonts
    // =========================================================
    std::cout << "[AssetsManager] Fonts:\n";
    for (auto const& fontId : manifest.fonts)
    {
        std::cout << "  [SKIP] Font unload not implemented"
            << " | id: " << fontId
            << " | path: " << GetFontPath(fontId) << "\n";
    }

    // =========================================================
    // Animations
    // =========================================================
    std::cout << "[AssetsManager] Animations:\n";
    for (auto const& animId : manifest.animations)
    {
        std::cout << "  [SKIP] Animation unload not implemented"
            << " | id: " << animId
            << " | path: " << GetAnimationPath(animId) << "\n";
    }

    std::cout << "[AssetsManager] ===== Unload Complete ===== "
        << (allOk ? "[SUCCESS]" : "[PARTIAL/FAILED]") << "\n\n";

    return allOk;
}


bool AssetsManager::GetSharedManifestLoaded() const {
    return mSharedManifestLoaded;
}

bool AssetsManager::LoadSharedManifestFromFile(std::string const& path) {
    namespace fs = std::filesystem;

    rapidjson::Document d;
    if (!JSONUtils::ReadJSONFromFile(d, path))
    {
        std::cerr << "[AssetsManager] Failed to load shared manifest file.\n";
        return false;
    }

    if (!d.IsObject() || !d.HasMember("sharedmanifest") || !d["sharedmanifest"].IsObject())
    {
        std::cerr << "[AssetsManager] Missing 'sharedmanifest' root object in: "
            << path << "\n";
        return false;
    }

    auto const& root = d["sharedmanifest"];

    if (!root.HasMember("groups") || !root["groups"].IsObject())
    {
        std::cerr << "[AssetsManager] Missing 'groups' object in shared manifest: "
            << path << "\n";
        return false;
    }

    auto const& groupsObj = root["groups"];

    mSharedManifest.groups.clear();

    for (auto it = groupsObj.MemberBegin(); it != groupsObj.MemberEnd(); ++it)
    {
        if (!it->name.IsString() || !it->value.IsObject())
            continue;

        std::string groupName = it->name.GetString();
        auto const& groupJson = it->value;

        AssetGroup group;

        auto ReadStringArray = [](rapidjson::Value const& obj,
            char const* key,
            std::vector<std::string>& outVec)
            {
                outVec.clear();

                if (!obj.HasMember(key) || !obj[key].IsArray())
                    return;

                auto const& arr = obj[key];
                for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
                {
                    if (arr[i].IsString())
                        outVec.emplace_back(arr[i].GetString());
                }
            };

        ReadStringArray(groupJson, "textures", group.textures);
        ReadStringArray(groupJson, "audio", group.audio);
        ReadStringArray(groupJson, "fonts", group.fonts);
        ReadStringArray(groupJson, "animations", group.animations);

        mSharedManifest.groups[groupName] = std::move(group);
    }

    mSharedManifestLoaded = true;

    std::cout << "[AssetsManager] Loaded shared manifest: " << path
        << " | groups: " << mSharedManifest.groups.size() << "\n";

    return true;
}

//group name is like 
//bool AssetsManager::LoadSharedAssets(std::string const& groupName) {
//    namespace fs = std::filesystem;
//
//    if (!mSharedManifestLoaded)
//    {
//        fs::path sharedManifestPath = FilePathToGame / "JSON" / "SharedAssets.manifest.json";
//
//        if (!LoadSharedManifestFromFile(sharedManifestPath.string()))
//        {
//            std::cerr << "[AssetsManager] Failed to load shared manifest file.\n";
//            return false;
//        }
//    }
//
//    auto it = mSharedManifest.groups.find(groupName);
//    if (it == mSharedManifest.groups.end())
//    {
//        std::cerr << "[AssetsManager] Shared group not found: "
//            << groupName << "\n";
//        return false;
//    }
//
//    SceneManifest tempManifest;
//    tempManifest.sceneName = "shared:" + groupName;
//    tempManifest.textures = it->second.textures;
//    tempManifest.audio = it->second.audio;
//    tempManifest.fonts = it->second.fonts;
//    tempManifest.animations = it->second.animations;
//
//    std::cout << "[AssetsManager] Loading shared group: " << groupName << "\n";
//    return PreloadSceneAssets(tempManifest);
//}

bool AssetsManager::LoadSharedAssets(std::string const& groupName) {
    namespace fs = std::filesystem;

    if (IsSharedGroupLoaded(groupName))
    {
        std::cout << "[AssetsManager] Shared group already loaded: "
            << groupName << "\n";
        return true;
    }

    if (!mSharedManifestLoaded)
    {
        fs::path sharedManifestPath = FilePathToGame / "JSON" / "SharedAssets.manifest.json";

        if (!LoadSharedManifestFromFile(sharedManifestPath.string()))
        {
            std::cerr << "[AssetsManager] Failed to load shared manifest file.\n";
            return false;
        }
    }

    auto it = mSharedManifest.groups.find(groupName);
    if (it == mSharedManifest.groups.end())
    {
        std::cerr << "[AssetsManager] Shared group not found: "
            << groupName << "\n";
        return false;
    }

    SceneManifest tempManifest;
    tempManifest.sceneName = "shared:" + groupName;
    tempManifest.textures = it->second.textures;
    tempManifest.audio = it->second.audio;
    tempManifest.fonts = it->second.fonts;
    tempManifest.animations = it->second.animations;

    std::cout << "[AssetsManager] Loading shared group: " << groupName << "\n";

    if (!PreloadSceneAssets(tempManifest))
    {
        return false;
    }

    MarkSharedGroupLoaded(groupName);
    return true;
}

//

bool AssetsManager::IsAudioBGM(std::string const& id) const
{
    auto const& audioTable = getAudioRegContainer();
    auto it = audioTable.find(id);
    if (it == audioTable.end())
    {
        return false;
    }

    return it->second.audiotype == "BGM";
}

bool AssetsManager::IsAudioSFX(std::string const& id) const
{
    auto const& audioTable = getAudioRegContainer();
    auto it = audioTable.find(id);
    if (it == audioTable.end())
    {
        return false;
    }

    return it->second.audiotype == "SFX";
}

bool AssetsManager::UnloadSceneBGMOnly(SceneManifest const& manifest)
{
    bool allOk = true;

    if (!mAudio)
    {
        std::cout << "[AssetsManager] [FAIL] AudioManager is null. Cannot unload scene BGM.\n";
        return false;
    }

    std::cout << "\n[AssetsManager] ===== Unloading Scene BGM Only =====\n";
    std::cout << "[AssetsManager] Scene: " << manifest.sceneName << "\n";

    for (auto const& audioId : manifest.audio)
    {
        if (!IsAudioBGM(audioId))
        {
            continue; // skip SFX, keep global
        }

        if (!mAudio->UnloadAudio(audioId))
        {
            std::cout << "  [FAIL] Failed to unload BGM | id: " << audioId
                << " | path: " << GetAudioPath(audioId) << "\n";
            allOk = false;
        }
        else
        {
            std::cout << "  [OK] Unloaded BGM | id: " << audioId
                << " | path: " << GetAudioPath(audioId) << "\n";
        }
    }

    return allOk;
}

bool AssetsManager::IsTextureInAnySharedGroup(std::string const& texId) const
{
    for (auto const& [groupName, group] : mSharedManifest.groups)
    {
        auto it = std::find(group.textures.begin(), group.textures.end(), texId);
        if (it != group.textures.end())
        {
            return true;
        }
    }
    return false;
}

bool AssetsManager::IsSharedGroupLoaded(std::string const& groupName) const
{
    return mLoadedSharedGroups.find(groupName) != mLoadedSharedGroups.end();
}

void AssetsManager::MarkSharedGroupLoaded(std::string const& groupName)
{
    mLoadedSharedGroups.insert(groupName);
}

void AssetsManager::MarkSharedGroupUnloaded(std::string const& groupName)
{
    mLoadedSharedGroups.erase(groupName);
}