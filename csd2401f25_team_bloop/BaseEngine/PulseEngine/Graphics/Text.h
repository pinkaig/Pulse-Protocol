/******************************************************************************/
/**
 * @file        Text.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Text Component
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once
#include "pch/pch_temp.h"
#include "Math/math.h"

enum class TextAlignment
{
    Left = 0,
    Center = 1,
    Right = 2
};

struct TextComponent
{
    std::string text{};    // text to display
    std::string font_id{}; // key to font registry
    // float x{}, y{}; //might change to offset
    Vector2 offset{0, 0};
    float font_size{};           // pixel height
    glm::vec3 color = {1, 1, 1}; // RGB
    float alpha = 1.0f;          // opacity (0.0 = transparent, 1.0 = opaque)
    bool visible = true;         // toggle on/off
    TextAlignment alignment = TextAlignment::Left; // text alignment
    // To be added, probably typed class enum
    // std::string font_style;


    //operator overloads
    //for Undo comparison function
    bool operator==(TextComponent const& other) const {
        return
            text == other.text &&
            font_id == other.font_id &&
            offset == other.offset &&
            font_size == other.font_size &&
            color == other.color &&
            visible == other.visible &&
            alignment == other.alignment;
    }
    bool operator!=(TextComponent const& other) const
    {
        return !(*this == other);
    }


    // ---------------------------------------------------
    // Serialize
    // ---------------------------------------------------
    void Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const
    {
        using namespace rapidjson;
        out.SetObject();

        // string
        Value TextVal;
        TextVal.SetString(text.c_str(),
                          static_cast<SizeType>(text.length()),
                          alloc);
        out.AddMember("text", TextVal, alloc);

        Value fond_idVal;
        fond_idVal.SetString(font_id.c_str(),
                             static_cast<SizeType>(font_id.length()),
                             alloc);
        out.AddMember("font_id", fond_idVal, alloc);
        // out.AddMember("x", x, alloc);
        // out.AddMember("y", y, alloc);
        //  Vector2
        Value offsetVal;
        offset.Serialize(offsetVal, alloc);
        out.AddMember("offset", offsetVal, alloc);
        // font size
        out.AddMember("font_size", font_size, alloc);
        // vec3

        rapidjson::Value colorArr(rapidjson::kArrayType);
        colorArr.PushBack(color.r, alloc);
        colorArr.PushBack(color.g, alloc);
        colorArr.PushBack(color.b, alloc);

        out.AddMember("color", colorArr, alloc);

        // bool
        out.AddMember("visible", visible, alloc);

        out.AddMember("alignment", static_cast<int>(alignment), alloc);
    }
    // ---------------------------------------------------
    // Deserialize
    // ---------------------------------------------------
    void Deserialize(const rapidjson::Value &in)
    {

        if (!in.IsObject())
        {
            std::cerr << "[TextComponent] Invalid JSON format for TextComponent\n";
            return;
        }
        // string
        if (in.HasMember("text") && in["text"].IsString())
        {
            text = (in["text"].GetString());
        }
        if (in.HasMember("font_id") && in["font_id"].IsString())
        {
            font_id = (in["font_id"].GetString());
        }
        // float
        // if (in.HasMember("x") && in["x"].IsNumber())
        //{
        //     x = (in["x"].GetFloat());
        // }
        // if (in.HasMember("y") && in["y"].IsNumber())
        //{
        //     y = (in["y"].GetFloat());
        // }

        if (in.HasMember("offset"))
        {
            offset.Deserialize(in["offset"]);
        }

        if (in.HasMember("font_size") && in["font_size"].IsNumber())
        {
            font_size = (in["font_size"].GetFloat());
        }

        // glm::vec4 color
        // check if contain color member and if its an array
        if (in.HasMember("color") && in["color"].IsArray())
        {
            auto const &arr = in["color"].GetArray();
            if (arr.Size() == 3) // fixed size 3, vec3
            {
                color.r = arr[0].GetFloat();
                color.g = arr[1].GetFloat();
                color.b = arr[2].GetFloat();
            }
            else
            {
                std::cerr << "[TextComponent] 'color' array must have 3 elements (r,g,b)\n";
            }
        }

        // bool
        if (in.HasMember("visible") && in["visible"].IsBool())
        {
            visible = (in["visible"].GetBool());
        }
        
        // alignment
        if (in.HasMember("alignment") && in["alignment"].IsInt())
        {
            alignment = static_cast<TextAlignment>(in["alignment"].GetInt());
        }
    }
};