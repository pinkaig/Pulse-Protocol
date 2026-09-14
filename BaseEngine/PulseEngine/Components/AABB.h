/******************************************************************************/
/**
 * @file        AABB.h
 * @project     Pulse Protocol
 * @author		Reginald Lew Yee Ren
 * @brief		Axis-Aligned Bounding Box (AABB) component used for collision.
 *				This component defines the bounding region of an entity by its
 *				minimum and maximum corners, along with its width and height.
 *				It is purely geometric and contains no gameplay logic.
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#pragma once


 /**
  * @struct AABB
  * @brief  Represents a rectangular collision box aligned with the world axes.
  */
//struct AABB
//{
//    Vector2 min;    ///< Bottom-left corner of the box
//    Vector2 max;    ///< Top-right corner of the box
//    float width;    ///< Box width along X-axis
//    float height;   ///< Box height along Y-axis
//
//    /**
//     * @brief Default constructor initializing all values to zero.
//     */
//     /* AABB()
//          : min{ 0.0f, 0.0f }, max{ 0.0f, 0.0f }, width{ 0.0f }, height{ 0.0f } {}*/
//
//
//    void Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const
//    {
//        using namespace rapidjson;
//        out.SetObject();
//
//        Value minVal;
//        Value maxVal;
//        min.Serialize(minVal, alloc);
//        max.Serialize(maxVal, alloc);
//
//        out.AddMember("min", minVal, alloc);
//        out.AddMember("max", maxVal, alloc);
//        out.AddMember("width", width, alloc);
//        out.AddMember("height", height, alloc);
//    }
//
//    void Deserialize(const rapidjson::Value& in)
//    {
//        if (!in.IsObject()) throw std::runtime_error("Invalid JSON for LogicComponent");
//        if (in.HasMember("min")) min.Deserialize(in["min"]);
//        if (in.HasMember("max")) max.Deserialize(in["max"]);
//        if (in.HasMember("width")) width = in["width"].GetFloat();
//        if (in.HasMember("height")) height = in["height"].GetFloat();
//    }
//};
