/******************************************************************************/
/**
 * @file        AssetsManager.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin (primary) - 90%
 * @author      Leu Jun Yong (secondary) - 10%
 * 
 * @brief       Central manager for loading, accessing, and releasing audio (FMOD)
 *              and texture (OpenGL) assets.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#pragma once
#include "pch/pch_temp.h"
#include "Resources/AssetRegistry.h"
#include <unordered_set>
// #include "../Graphics/GraphicsManager.h"
// #include "../Audio/AudioManager.h"
#include "../Scene/SceneManifest.h"
// for .net
#include "CoreEngine/Core/ImportExport.h"
#pragma warning(push)
#pragma warning(disable: 4251) // stupid warning just saying to make sure we are using same compilier type and runtime MDd. WHICH WE ARE SO WHY WARNING STILL??????????

struct AssetGroup
{
    std::vector<std::string> textures;
    std::vector<std::string> audio;
    std::vector<std::string> fonts;
    std::vector<std::string> animations;
};

struct SharedAssetsManifest
{
    std::unordered_map<std::string, AssetGroup> groups;
};

class GraphicsManager; // fwd
class AudioManager;    // fwd
// class AssetRegistry; //cant fwd because take by value
class DLL_API AssetsManager
{
public:
    AssetsManager() = default;
    ~AssetsManager();

    void Initialize();
    void Shutdown();

    // Accessors (refs avoid copies)
    AssetRegistry &getRegistry();

    AudioManager &getAudio();
    AudioManager const &getAudio() const;

    GraphicsManager &getGraphics();
    GraphicsManager const &getGraphics() const;

    //for editor
    unsigned int GetOrLoadTexture(const std::string &name);
    bool   EvictTexture(std::string const& idOrPath);

    //get path from registry
    std::string GetAudioPath(std::string const& id)  const;
    std::string GetTexturePath(std::string const& id) const;
    std::string GetFontPath(std::string const& id)   const;
    std::string GetScenePath(std::string const& id)   const;
    std::string GetAnimationPath(std::string const& id)   const;

    //Set Audio and GraphicsManager
    void SetAudioManager(AudioManager* audio);
    void SetGraphicsManager(GraphicsManager* graphics);
    //Call AudioManager Function
    bool PlaySFX(std::string const& id);
    bool PlayMusic(std::string const& id);

    //get container
    std::unordered_map<std::string, AudioResource> const& getAudioRegContainer()const;
    std::unordered_map<std::string, TextureResource>const& getTexRegContainer()const;
    std::unordered_map<std::string, FontResource> const& getFontRegContainer()const;
    std::unordered_map<std::string, SceneResource> const& getSceneRegContainer()const;
    std::unordered_map<std::string, AnimationResource> const& getAnimRegContainer()const;
    //boolean check has XXX id in container
    bool HasAudio(std::string const& id)const;
    bool HasTexture(std::string const& id)const;
    bool HasFont(std::string const& id)const;
    bool HasScene(std::string const& id)const;
    bool HasAnimation(std::string const& id)const;

    //Scene manifest file
    bool PreloadSceneAssets(SceneManifest const& manifest);
    bool UnloadSceneAssets(SceneManifest const& manifest);

    bool GetSharedManifestLoaded() const;
    bool LoadSharedManifestFromFile(std::string const& path);
    bool LoadSharedAssets(std::string const& groupName);

    bool UnloadSceneBGMOnly(SceneManifest const& manifest);
    bool IsAudioBGM(std::string const& id) const;
    bool IsAudioSFX(std::string const& id) const;

    bool IsTextureInAnySharedGroup(std::string const& texId) const;

    bool IsSharedGroupLoaded(std::string const& groupName) const;
    void MarkSharedGroupLoaded(std::string const& groupName);
    void MarkSharedGroupUnloaded(std::string const& groupName);
private:
    AssetRegistry mRegistry; // owned
    SharedAssetsManifest mSharedManifest; //owned
    bool mSharedManifestLoaded = false;
    AudioManager* mAudio = nullptr;    // non-owning
    GraphicsManager* mGraphics = nullptr;   // non-owning

    std::unordered_set<std::string> mLoadedSharedGroups;
};
#pragma warning(pop)