/******************************************************************************/
/**
 * @file        LayerManagerPanel.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * 
 * @brief       Layer management window for controlling layer visibility,
 *              locking, and filtering in the editor.
 *              Uses LayerRegistry for editable layer names.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once

#include "Graphics/LayerRegistry.h"  // For LayerMask type and LayerRegistry

namespace PulseEditor
{
    // Draw the Layer Manager window
    void DrawLayerManagerWindow();

    // Notify renderer of layer visibility changes
    void UpdateGraphicsLayerFilter();

    // -------------------------------------------------------------------------
    // Convenience wrappers (forward to LayerRegistry)
    // These maintain API compatibility with existing code
    // -------------------------------------------------------------------------

    // Check if a layer is currently 

    inline bool IsLayerVisible(LayerMask layer) {
        return LayerRegistry::Get().IsLayerVisible(layer);
    }

    // Check if a layer is currently locked (for editor selection)
    inline bool IsLayerLocked(LayerMask layer) {
        return LayerRegistry::Get().IsLayerLocked(layer);
    }

    // Set visibility for a specific layer
    inline void SetLayerVisibility(LayerMask layer, bool visible) {
        LayerRegistry::Get().SetLayerVisibility(layer, visible);
        UpdateGraphicsLayerFilter();
    }

    // Set visibility for all layers at once (for quick filters like TopBar)
    inline void SetAllLayersVisibility(bool bg, bool world, bool ui, bool text) {
        auto& reg = LayerRegistry::Get();
        reg.SetLayerVisibility(LAYER_BACKGROUND, bg);
        reg.SetLayerVisibility(LAYER_WORLD, world);
        reg.SetLayerVisibility(LAYER_UI, ui);
        reg.SetLayerVisibility(LAYER_TEXT, text);
        UpdateGraphicsLayerFilter();
    }

}