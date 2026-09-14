/******************************************************************************/
/**
 * @file        Layer.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai (primary) - 90%
 * @author      Ban Kai Wei Benjamin (secondary) - 10%
 * 
 * @brief       Defines the rendering layer system used to control draw order.
 *              Updated to use LayerRegistry for editable layer names.
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include"pch/pch_temp.h"
#include "LayerRegistry.h"  // For GetLayerName/GetLayerMask functions

struct LayerTag {
    LayerMask mask;
    int sortingLayer = 1; // 0=Background, 1=World, 2=UI (for render order)
    int orderInLayer = 0;  // Higher = draws later = appears in front
    float z = 0.f;       // Depth offset for fine-grained ordering

    bool operator==(LayerTag const& other) const {
        return
            mask == other.mask &&
            sortingLayer == other.sortingLayer &&
            orderInLayer == other.orderInLayer &&
            z == other.z;
    }
    bool operator!=(LayerTag const& other) const
    {
        return !(*this == other);
    }

    void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const {
        using namespace rapidjson;
        out.SetObject();

        // Get layer name from registry (supports custom/renamed layers)
        std::string layerName = LayerRegistry::Get().GetLayerName(mask);

        Value nameVal;
        nameVal.SetString(layerName.c_str(), static_cast<SizeType>(layerName.length()), alloc);
        out.AddMember("layerName", nameVal, alloc);

        out.AddMember("mask", mask, alloc);
        out.AddMember("sortingLayer", sortingLayer, alloc);
        out.AddMember("orderInLayer", orderInLayer, alloc);
        out.AddMember("z", z, alloc);
    }

    void Deserialize(const rapidjson::Value& in) {
        if (!in.IsObject()) return;

        // First try to load by mask (most reliable)
        if (in.HasMember("mask") && in["mask"].IsUint()) {
            mask = in["mask"].GetUint();
        }
        // Fall back to name lookup (for human-edited files)
        else if (in.HasMember("layerName") && in["layerName"].IsString()) {
            std::string name = in["layerName"].GetString();
            mask = LayerRegistry::Get().GetLayerMask(name);
        }
        else {
            // Default to World layer if nothing specified
            mask = LAYER_WORLD;
        }

        if (in.HasMember("sortingLayer") && in["sortingLayer"].IsInt()) {
            sortingLayer = in["sortingLayer"].GetInt();
        }
        else {
            // Backwards compatibility: derive from mask
            if (mask == LAYER_BACKGROUND) sortingLayer = 0;
            else if (mask == LAYER_UI) sortingLayer = 2;
            else if (mask == LAYER_TEXT) sortingLayer = 3;
            else sortingLayer = 1;  // Default to World
        }

        if (in.HasMember("orderInLayer") && in["orderInLayer"].IsInt())
            orderInLayer = in["orderInLayer"].GetInt();

        if (in.HasMember("z") && in["z"].IsNumber())
            z = in["z"].GetFloat();
    }
};