/******************************************************************************/
/**
 * @file        FolderValidation.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * 
 * @brief       Implementation of folder type validation system.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "FolderValidation.h"
#include "../pch/pch_temp.h"

namespace PulseEditor
{
    // Helper function to check a single folder name
    static FolderType GetFolderTypeFromName(const std::string& folderName)
    {
        std::string lower = folderName;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Audio folders
        if (lower == "sound" || lower == "sounds" || lower == "audio" || lower == "music")
            return FolderType::Audio;

        // Texture folders
        if (lower == "texture" || lower == "textures" || lower == "sprites" || lower == "images")
            return FolderType::Texture;

        // Font folders
        if (lower == "font" || lower == "fonts")
            return FolderType::Font;

        // Prefab folders
        if (lower == "prefab" || lower == "prefabs")
            return FolderType::Prefab;

        return FolderType::Generic;
    }

    FolderType GetFolderType(const std::string& folderPath)
    {
        namespace fs = std::filesystem;

        // Check the full path - walk up from current folder to find any special folder
        fs::path path(folderPath);

        // Check each component of the path (from deepest to root)
        while (!path.empty() && path.has_filename())
        {
            std::string folderName = path.filename().string();
            FolderType type = GetFolderTypeFromName(folderName);

            if (type != FolderType::Generic)
            {
                return type;  // Found a special folder in the hierarchy
            }

            // Move to parent
            fs::path parent = path.parent_path();
            if (parent == path)
                break;  // Reached root
            path = parent;
        }

        return FolderType::Generic;
    }

    bool IsValidExtensionForFolder(FolderType type, const std::string& filepath)
    {
        std::string ext = std::filesystem::path(filepath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        switch (type)
        {
        case FolderType::Audio:
            return ext == ".mp3" || ext == ".wav";

        case FolderType::Texture:
            return ext == ".png" || ext == ".jpg" || ext == ".jpeg";

        case FolderType::Font:
            return ext == ".ttf" || ext == ".otf";

        case FolderType::Prefab:
            return ext == ".json";

        case FolderType::Generic:
        default:
            // ===============================================================
            // ALLOWLIST: only allow certain extensions, so user can't put ANY files with weird extensions inside
            // ===============================================================

            if (ext == ".mp3" || ext == ".wav")
                return true;

            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                return true;

            if (ext == ".ttf" || ext == ".otf")
                return true;

            if (ext == ".json")
                return true;

            // everything else is blocked/rejected
            return false;
        }
    }

    const char* GetFolderTypeName(FolderType type)
    {
        switch (type)
        {
        case FolderType::Audio:   return "Audio";
        case FolderType::Texture: return "Texture";
        case FolderType::Font:    return "Font";
        case FolderType::Prefab:  return "Prefab";
        default:                  return "Assets";
        }
    }

    const char* GetAllowedExtensions(FolderType type)
    {
        switch (type)
        {
        case FolderType::Audio:   return ".mp3, .wav";
        case FolderType::Texture: return ".png, .jpg, .jpeg";
        case FolderType::Font:    return ".ttf, .otf";
        case FolderType::Prefab:  return ".json";
        default:                  return ".png, .jpg, .jpeg, .mp3, .wav, .ttf, .otf, .json";
        }
    }

}  // eof namespace