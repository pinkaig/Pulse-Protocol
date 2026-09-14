/******************************************************************************/
/**
 * @file        ProjectPanelHelpers.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 *
 * @brief       Helper functions for ProjectPanel to improve code readability.
 *              Contains file validation, directory operations, drag-drop handlers,
 *              and UI drawing utilities.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once

#include "Editor.h"           // Includes everything needed
#include "FolderValidation.h"

namespace PulseEditor
{
    // =============================================================
    // FILE VALIDATION HELPERS
    // =============================================================

    // Check if file has image extension (.png, .jpg, .jpeg)
    // Used specifically for texture drag-drop and thumbnail loading
    bool IsImageExt(const std::string& path);

    // Check if folder is a prefab folder (convenience wrapper)
    bool IsPrefabFolder(const std::string& folderName);

    // =============================================================
    // DIRECTORY OPERATIONS
    // =============================================================

    // Scan directory and populate entries list
    void RefreshDirectory(const std::string& assetRoot, const std::string& currentDir, std::vector<AssetItem>& entries);

    // Log warnings for invalid files in folder (console output)
    void ValidateFolderContents(const std::string& currentDir, const std::vector<AssetItem>& entries);

    // =============================================================
    // TEXTURE OPERATIONS
    // =============================================================

    // Convert absolute file path to the format needed by GetOrLoadTexture
    // Returns absolute path with forward slashes normalized
    std::string GetRelativeAssetPath(const std::string& absolutePath);

    // Load thumbnail texture if not already loaded
    void EnsureThumbnailLoaded(AssetItem& item, const std::string& currentDir);

    // Updates file on disk, texture cache, project thumbnails, live scene entities
    bool ReplaceTextureFile(const std::string& dstPath, const std::string& srcPath, std::vector<AssetItem>& dirEntries);

    // =============================================================
    // DRAG & DROP HANDLERS
    // =============================================================

    // Handle files dropped from OS file explorer into editor
    void HandleOSFileDrop(const std::vector<std::string>& droppedPaths, const std::string& currentDir, bool& showImportError, std::string& invalidFileName, std::string& targetFolderName, FolderType& targetFolderType);

    // Handle entity dropped onto prefab folder
    void HandleEntityDropToPrefab(Entity droppedEntity, bool& showWarning, std::string& blockedName);

    // =============================================================
    // UI DRAWING HELPERS
    // =============================================================

    // Draw file/folder label with validation coloring (red for invalid files)
    void DrawFileLabel(const AssetItem& item, float thumbSize, const std::string& currentDir);

    // =============================================================
    // POPUP MODALS
    // =============================================================

    // Draw warning popup for engine-managed entities
    void DrawEngineManagedWarningPopup(bool& showWarning, const std::string& blockedName);

    // Draw error popup for invalid file imports
    void DrawImportErrorPopup(bool& showError, const std::string& invalidFile, const std::string& targetFolder, FolderType targetType);


    std::string FindTextureRegistryIdByPath(std::string const& fullPath);
    std::string ResolveTextureKeyForAsset(std::string const& fullPath);
    void UpdateRenderablesUsingTextureKey(std::string const& oldKey, std::string const& newKey, unsigned int newTex);

}  // namespace PulseEditor