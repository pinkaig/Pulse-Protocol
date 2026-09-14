/******************************************************************************/
/**
 * @file        AudioSource.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Audio Component
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once

#include <string>
#include <rapidjson/document.h>
#include <iostream>

// Forward-declare FMOD types to avoid heavy includes here
namespace FMOD
{
    class System;
    class Sound;
    class Channel;
    class ChannelGroup;
}

struct AudioSource {
    std::string audio_id = ""; //which sound this entity plays
	float volume = 1.0f;
	bool loop = false;
	bool playOnAwake = false;

    //runtime
    FMOD::Channel* channel = nullptr;

    bool operator==(AudioSource const& other) const {
        return audio_id == other.audio_id &&
            volume == other.volume &&
            loop == other.loop &&
            playOnAwake == other.playOnAwake;
    }
    bool operator!=(AudioSource const& other) const
    {
        return !(*this == other);
    }


    // ---------------------------------------------------
    // Serialize
    // ---------------------------------------------------
    void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const {
        using namespace rapidjson;
        out.SetObject();

        //string
        Value audioidVal;
        audioidVal.SetString(audio_id.c_str(),
            static_cast<SizeType>(audio_id.length()),
            alloc);
        out.AddMember("audio_id", audioidVal, alloc);
        //float
        out.AddMember("volume", volume, alloc);
        //bool
        out.AddMember("loop", loop, alloc);
        out.AddMember("playOnAwake", playOnAwake, alloc);

    }
    // ---------------------------------------------------
    // Deserialize
    // ---------------------------------------------------
    void Deserialize(const rapidjson::Value& in) {

        if (!in.IsObject())
        {
            std::cerr << "[AudioSource] Invalid JSON format for AudioSource\n";
            return;
        }
        //string
        if (in.HasMember("audio_id") && in["audio_id"].IsString())
        {
            audio_id = (in["audio_id"].GetString());
        }
        //float
        if (in.HasMember("volume") && in["volume"].IsNumber())
        {
            volume = (in["volume"].GetFloat());
        }
        //bool
        if (in.HasMember("loop") && in["loop"].IsBool())
        {
            loop = (in["loop"].GetBool());
        }
        if (in.HasMember("playOnAwake") && in["playOnAwake"].IsBool())
        {
            playOnAwake = (in["playOnAwake"].GetBool());
        }
        //MUST reset runtime-only ptr
        channel = nullptr;
    }
};


