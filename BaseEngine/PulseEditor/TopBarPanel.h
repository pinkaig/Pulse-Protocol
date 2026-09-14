/******************************************************************************/
/**
 * @file        TopBarPanel.h
 * @project     Pulse Protocol
 * @author      Leu Jun Yong
 * @brief       Top bar panel UI and play-mode state management.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#pragma once

namespace PulseEditor
{
    std::string OpenJsonFileDialog();

    // Play-mode baseline helpers
    void CaptureEditorBaseline();
    void RestoreEditorBaseline();

    // UI draw function
    void DrawTopBarPanel(float windowWidth, float top_y, float top_h);

    bool IsPlaying();
}
