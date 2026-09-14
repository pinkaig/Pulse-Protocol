/******************************************************************************/
/**
 * @file        AudioAPI.cpp
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 100%
 *
 * @brief       ScriptAPI audio functions to control audio sources (play/stop, volume, basic helpers) and connect scripts to the engine audio system.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#define generic generic_workaround

#include "AudioAPI.h"
#include "CoreEngine/Core/ScriptingBridge.h"
#include "CoreEngine/ECS/Coordinator.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioSource.h"
#include <msclr/marshal_cppstd.h>

#undef generic

using namespace System;
using namespace msclr::interop;

namespace ScriptAPI
{
    static AudioManager* GetAudioManager()
    {
        auto coord = Coordinator::GetInstance();
        if (!coord) return nullptr;

        auto sys = coord->GetSystem<AudioManager>();
        if (!sys) return nullptr;

        return sys.get();
    }

    static float Normalize01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    AudioComponent::AudioComponent(int id) : EntityID(id) {}

    void AudioComponent::Play()
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;

        AudioSource* src = ScriptBridge::GetAudioSourceComponent(EntityID);
        if (!src) return;

        mgr->PlayFromAudioSource(*src);
    }

    void AudioComponent::PlayBGM()
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;

        AudioSource* src = ScriptBridge::GetAudioSourceComponent(EntityID);
        if (!src) return;

        // Play whatever audio source is attached to this entity
        mgr->PlayFromAudioSource(*src);
    }

    void AudioComponent::PlayBGM(String^ name)
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;

        // If a valid name is given, use the explicit BGM path
        if (name != nullptr && name->Length > 0)
        {
            mgr->PlayBGM(marshal_as<std::string>(name));
            return;
        }

        // Otherwise fall back to the entity's attached audio source
        AudioSource* src = ScriptBridge::GetAudioSourceComponent(EntityID);
        if (!src) return;

        mgr->PlayFromAudioSource(*src);
    }

    void AudioComponent::StopBGM()
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;
        mgr->StopBGM();
    }

    void AudioComponent::StopAllSFX()
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;
        mgr->StopAllSFX();
    }

    void AudioComponent::PauseBGM()
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;
        mgr->PauseBGM();
    }

    void AudioComponent::ResumeBGM()
    {
        AudioManager* audio = GetAudioManager();
        if (!audio) return;

        audio->ResumeBGM();
    }

    void AudioComponent::SetBGMVolume(float volume)
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;
        mgr->SetBGMVolume(volume);
    }

    void AudioComponent::PlaySFX(String^ name)
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;
        mgr->PlaySound(marshal_as<std::string>(name));
    }

    void AudioComponent::SetMasterVolume(float volume01)
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;

        float v = Normalize01(volume01);
        mgr->mMasterVol = v;
        mgr->applyMasterVolume();
    }

    void AudioComponent::SetMusicVolume(float volume01)
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;

        float v = Normalize01(volume01);
        mgr->mMusicVol = v;
        mgr->applyMusicVolume();
    }

    void AudioComponent::SetSfxVolume(float volume01)
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;

        float v = Normalize01(volume01);
        mgr->mSfxVol = v;
        mgr->applySfxVolume();
    }

    float AudioComponent::GetMasterVolume()
    {
        auto mgr = GetAudioManager();
        if (!mgr) return 1.0f;
        return mgr->mMasterVol;
    }

    float AudioComponent::GetMusicVolume()
    {
        auto mgr = GetAudioManager();
        if (!mgr) return 1.0f;
        return mgr->mMusicVol;
    }

    float AudioComponent::GetSfxVolume()
    {
        auto mgr = GetAudioManager();
        if (!mgr) return 1.0f;
        return mgr->mSfxVol;
    }

    float AudioComponent::GetBGMPosition()
    {
        auto mgr = GetAudioManager();
        if (!mgr)
        {
            return 1.0f;
        }
        return mgr->GetBGMPositionSeconds();
    }

    void AudioComponent::FadeOutMusic(float duration)
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;

        mgr->FadeOutCurrentBGM(duration, true);
    }

    bool AudioComponent::IsBGMPlaying()
    {
        auto mgr = GetAudioManager();
        if (!mgr) return false;

        return mgr->IsBGMPlaying();
    }

    void AudioComponent::FadeInMusic(float duration)
    {
        AudioManager* audio = GetAudioManager();
        if (!audio) return;

        audio->FadeInCurrentBGM(duration);
    }

    void AudioComponent::SetListenerPosition(float x, float y)
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;
        mgr->SetListenerPosition(x, y);
    }

    void AudioComponent::PlaySFXAt(String^ name, float worldX, float worldY,
                                   float minDist, float maxDist, float minVolume)
    {
        auto mgr = GetAudioManager();
        if (!mgr) return;
        mgr->PlaySoundAt(marshal_as<std::string>(name), worldX, worldY, minDist, maxDist, minVolume);
    }

}
