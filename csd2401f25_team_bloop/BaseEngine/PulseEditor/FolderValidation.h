/******************************************************************************/
/**
 * @file        FolderValidation.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * 
 * @brief       Folder type validation system for asset imports.
 *              Validates file extensions based on target folder type.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include "pch/pch_temp.h"

namespace PulseEditor
{
    // ======================================================================
    // ACCEPTABLE FOLDER TYPES
    // ======================================================================
    enum class FolderType
    {
        Generic,    // any file allowed
        Audio,      // .mp3, .wav
        Texture,    // .png, .jpg, .jpeg
        Font,       // .ttf, .otf
        Prefab      // .json
    };

    // ======================================================================
    // VALIDATION FUNCTIONS
    // ======================================================================

    // determines folder type from folder name
    FolderType GetFolderType(const std::string& folderName);

    // checks if a file extension is valid for the given folder type
    bool IsValidExtensionForFolder(FolderType type, const std::string& filepath);

    // gets human-readable folder type name
    const char* GetFolderTypeName(FolderType type);

    // gets comma-separated list of allowed extensions for display
    const char* GetAllowedExtensions(FolderType type);

}  // eof namespace