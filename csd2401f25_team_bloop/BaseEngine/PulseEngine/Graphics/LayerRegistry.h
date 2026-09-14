/******************************************************************************/
/**
 * @file        LayerRegistry.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En (primary) - 90%
 * @author      Ban Kai Wei Benjamin (secondary) - 10%
 * 
 * @brief       Centralized registry for layer names and configuration.
 *              This allows layer names to be edited via the editor while
 *              keeping the underlying bitmask system unchanged.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "CoreEngine/Core/ImportExport.h"  // For DLL_API

using LayerMask = uint32_t;

// Layer mask constants (keep these for compatibility)
enum : LayerMask {
    LAYER_BACKGROUND = 1u << 0,
    LAYER_WORLD = 1u << 1,
    LAYER_UI = 1u << 2,
    LAYER_TEXT = 1u << 3,
    LAYER_ALL = 0xFFFFFFFFu
};

/**
 * @struct LayerInfo
 * @brief  Stores all data for a single layer
 */
struct DLL_API LayerInfo
{
    std::string  name;              // Editable display name
    LayerMask    mask = 0;          // Bitmask (LAYER_BACKGROUND, etc.)
    bool         visible = true;    // Editor visibility
    bool         locked = false;    // Lock from selection in editor
    bool         isDefault = true;  // Cannot be deleted if true

    // Display color (stored as floats for ImGui compatibility)
    float colorR = 1.0f;
    float colorG = 1.0f;
    float colorB = 1.0f;
    float colorA = 1.0f;

    bool operator==(LayerInfo const& other) const
    {
        return name == other.name &&
            mask == other.mask &&
            visible == other.visible &&
            locked == other.locked &&
            isDefault == other.isDefault &&
            colorR == other.colorR &&
            colorG == other.colorG &&
            colorB == other.colorB &&
            colorA == other.colorA;
    }

    bool operator!=(LayerInfo const& other) const
    {
        return !(*this == other);
    }
};
/**
 * @struct LayerRegistrySnapshot
 * @brief  takes a snapshot of the layer registry
 */
struct DLL_API LayerRegistrySnapshot
{
    std::vector<LayerInfo> layers;

    bool operator==(LayerRegistrySnapshot const& other) const
    {
        return layers == other.layers;
    }

    bool operator!=(LayerRegistrySnapshot const& other) const
    {
        return !(*this == other);
    }
};

/**
 * @class LayerRegistry
 * @brief Singleton class managing all layer definitions
 *        Provides name<->mask mapping and serialization support
 */
class DLL_API LayerRegistry
{
public:
    // Singleton access
    static LayerRegistry& Get();

    // Initialize with default layers (call once at startup)
    void Initialize();

    // -------------------------------------------------------------------------
    // Layer Query API
    // -------------------------------------------------------------------------

    // Get all layers (for UI iteration)
    const std::vector<LayerInfo>& GetAllLayers() const;
    std::vector<LayerInfo>& GetAllLayersMutable();

    // Get layer info by mask
    const LayerInfo* GetLayerByMask(LayerMask mask) const;
    LayerInfo* GetLayerByMaskMutable(LayerMask mask);

    // Get layer info by name
    const LayerInfo* GetLayerByName(const std::string& name) const;

    // Get layer name from mask (for display/serialization)
    std::string GetLayerName(LayerMask mask) const;

    // Get layer mask from name (for deserialization)
    LayerMask GetLayerMask(const std::string& name) const;

    // Get index in layer list from mask
    int GetLayerIndex(LayerMask mask) const;

    // Get mask from index in layer list
    LayerMask GetLayerMaskFromIndex(int index) const;

    // Get total layer count
    size_t GetLayerCount() const;

    // -------------------------------------------------------------------------
    // Layer Modification API
    // -------------------------------------------------------------------------

    // Rename a layer
    void SetLayerName(LayerMask mask, const std::string& newName);

    // Set layer visibility (for editor filtering)
    void SetLayerVisibility(LayerMask mask, bool visible);

    // Set layer locked state
    void SetLayerLocked(LayerMask mask, bool locked);

    // Set layer color
    void SetLayerColor(LayerMask mask, float r, float g, float b, float a = 1.0f);

    // Add a custom layer (returns new mask, or 0 if failed)
    LayerMask AddCustomLayer(const std::string& name);

    // Remove a custom layer (cannot remove default layers)
    bool RemoveCustomLayer(LayerMask mask);

    // Check if layer can be removed
    bool CanRemoveLayer(LayerMask mask) const;

    // -------------------------------------------------------------------------
    // Visibility API (for renderer filtering)
    // -------------------------------------------------------------------------

    // Check if a specific layer is visible
    bool IsLayerVisible(LayerMask mask) const;

    // Check if a specific layer is locked
    bool IsLayerLocked(LayerMask mask) const;

    // Compute combined visible layer mask (for SetEditorLayerFilter)
    LayerMask ComputeVisibleLayerMask() const;

    // Quick visibility controls
    void SetAllLayersVisible(bool visible);
    void ShowAllLayers();
    void HideAllLayers();
    void UnlockAllLayers();

    // -------------------------------------------------------------------------
    // Serialization
    // -------------------------------------------------------------------------

    // Save layer configuration to JSON file
    void SaveConfig(const std::string& filepath) const;

    // Load layer configuration from JSON file
    bool LoadConfig(const std::string& filepath);

    // Get layer names array for ImGui combo (returns vector of c_str pointers)
    // Note: Pointers are valid only until layers are modified
    std::vector<const char*> GetLayerNamesForCombo() const;

    //undo...
    LayerRegistrySnapshot MakeSnapshot() const;
    void RestoreSnapshot(LayerRegistrySnapshot const& snap);
private:
    LayerRegistry() = default;
    ~LayerRegistry() = default;
    LayerRegistry(const LayerRegistry&) = delete;
    LayerRegistry& operator=(const LayerRegistry&) = delete;

    std::vector<LayerInfo> m_layers;
    bool m_initialized = false;

    // Find next available bitmask for custom layers
    LayerMask FindNextAvailableMask() const;
};

// -------------------------------------------------------------------------
// Convenience functions (inline, call exported class methods)
// -------------------------------------------------------------------------

// Get layer name (shorthand)
inline std::string GetLayerName(LayerMask mask) {
    return LayerRegistry::Get().GetLayerName(mask);
}

// Get layer mask from name (shorthand)
inline LayerMask GetLayerMask(const std::string& name) {
    return LayerRegistry::Get().GetLayerMask(name);
}