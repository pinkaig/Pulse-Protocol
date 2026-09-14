/******************************************************************************/
/**
 * @file        ScreenModeDropdown.cs
 * @project     Pulse Protocol
 * @author      Leu Jun Yong - 70%
 * @author      Goh Pin Kai - 30%
 *
 * @brief       Screen mode dropdown UI logic (open/close behavior, click handling, and selecting fullscreen/windowed).
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
using ScriptAPI;

// ======================================================
// Shared dropdown state (engine-style)
// ======================================================
public static class DropdownState
{
    public static bool ScreenDropdownOpen = false;

    // Bounds of the OPEN dropdown (world space)
    public static float OpenX;
    public static float OpenY;
    public static float OpenHalfW;
    public static float OpenHalfH;

    // Bounds of the CLOSED header box (world space)
    public static float HeaderX;
    public static float HeaderY;
    public static float HeaderHalfW;
    public static float HeaderHalfH;

    // Debounce: block option selection for a couple frames after opening
    public static int SuppressOptionClickFrames = 0;

    // Controller: whether the dropdown header currently has gamepad focus.
    public static bool IsDropdownFocused = false;

    // Which row is highlighted when open via controller:
    //   0 = header/current-selection row  →  Cross just closes (no change)
    //   1 = alternate-option row          →  Cross toggles screen mode
    public static int HoveredOptionIndex = 1;

    public static bool IsInsideOpenDropdown(float x, float y)
    {
        return x >= OpenX - OpenHalfW && x <= OpenX + OpenHalfW &&
               y >= OpenY - OpenHalfH && y <= OpenY + OpenHalfH;
    }

    public static bool IsInsideHeader(float x, float y)
    {
        return x >= HeaderX - HeaderHalfW && x <= HeaderX + HeaderHalfW &&
               y >= HeaderY - HeaderHalfH && y <= HeaderY + HeaderHalfH;
    }
}

// ======================================================
// Controls click + open/close state
// Attach to: DropdownBox (Closed)
// ======================================================
public class DropdownToggle : Script
{
    private bool initialized = false;
    private float baseScaleX;
    private float baseScaleY;

    private const int GP_CROSS      = InputConstants.GP_CROSS;
    private const int GP_CIRCLE     = InputConstants.GP_CIRCLE;
    private const int GP_DPAD_UP    = InputConstants.GP_DPAD_UP;
    private const int GP_DPAD_RIGHT = InputConstants.GP_DPAD_RIGHT;
    private const int GP_DPAD_DOWN  = InputConstants.GP_DPAD_DOWN;
    private const int GP_DPAD_LEFT  = InputConstants.GP_DPAD_LEFT;

    // Scale multiplier applied to the header sprite when it has gamepad focus.
    public float FocusScaleBoost = 1.15f;
    private bool m_focused = false;
    // Frame on which gamepad focus was just gained — D-pad input is suppressed that frame
    // so the same press that caused the focus transfer doesn't immediately release it.
    private int m_focusGainFrame = -1;

    public override void Update()
    {
        InputComponent input = GetInput();
        TransformComponent t = GetTransform();

        if (!initialized)
        {
            baseScaleX = t.ScaleX;
            baseScaleY = t.ScaleY;
            initialized = true;
        }

        // Publish header bounds every frame
        DropdownState.HeaderX = t.X;
        DropdownState.HeaderY = t.Y;
        DropdownState.HeaderHalfW = baseScaleX;
        DropdownState.HeaderHalfH = baseScaleY;

        // Controller: apply/remove focus scale boost
        if (DropdownState.IsDropdownFocused)
        {
            if (!m_focused)
            {
                Console.WriteLine("[Dropdown] Gamepad focus GAINED");
                m_focused = true;
                m_focusGainFrame = Time.FrameCount;
            }
            // Scale boost only when the dropdown is closed (shows the header has gamepad focus).
            // When open, no scale boost on the header — the FULL SCREEN row uses its hover
            // texture instead, keeping the visual clean.
            bool headerHighlighted = !DropdownState.ScreenDropdownOpen;
            if (headerHighlighted)
            {
                t.ScaleX = baseScaleX * FocusScaleBoost;
                t.ScaleY = baseScaleY * FocusScaleBoost;
            }
            else
            {
                t.ScaleX = baseScaleX;
                t.ScaleY = baseScaleY;
            }
        }
        else if (m_focused)
        {
            Console.WriteLine("[Dropdown] Gamepad focus LOST");
            t.ScaleX = baseScaleX;
            t.ScaleY = baseScaleY;
            m_focused = false;
        }

        // Controller: handle input when this header has gamepad focus
        if (DropdownState.IsDropdownFocused)
        {
            bool _cross  = input.IsGamepadButtonTriggered(GP_CROSS);
            bool _circle = input.IsGamepadButtonTriggered(GP_CIRCLE);
            // Suppress D-pad on the same frame focus was gained — that press is what caused
            // the transfer and must not be re-consumed here (would immediately release focus).
            bool _suppressDPad = (Time.FrameCount == m_focusGainFrame);
            bool _dLeft  = !_suppressDPad && input.IsGamepadButtonTriggered(GP_DPAD_LEFT);
            bool _dRight = !_suppressDPad && input.IsGamepadButtonTriggered(GP_DPAD_RIGHT);
            bool _dUp    = !_suppressDPad && input.IsGamepadButtonTriggered(GP_DPAD_UP);
            bool _dDown  = !_suppressDPad && input.IsGamepadButtonTriggered(GP_DPAD_DOWN);

            if (_cross || _dLeft || _dRight || _dUp || _dDown || _circle)
                Console.WriteLine($"[Dropdown] focused input: cross={_cross} dL={_dLeft} dR={_dRight} dU={_dUp} dD={_dDown} open={DropdownState.ScreenDropdownOpen}");

            if (DropdownState.ScreenDropdownOpen)
            {
                // D-pad Up: move highlight toward the header row; clamp at top (no exiting)
                if (_dUp)
                {
                    if (DropdownState.HoveredOptionIndex > 0)
                        DropdownState.HoveredOptionIndex--;
                    return;
                }
                // D-pad Down: move highlight toward the alternate row; clamp at bottom (no exiting)
                if (_dDown)
                {
                    if (DropdownState.HoveredOptionIndex < 1)
                        DropdownState.HoveredOptionIndex++;
                    return;
                }
                // Cross confirms the highlighted row:
                //   index 1 (alternate) → toggle screen mode, close, move to slider
                //   index 0 (current)   → just close, keep focus on header (no change)
                if (_cross)
                {
                    if (DropdownState.HoveredOptionIndex == 1)
                    {
                        ToggleScreenAPI.ToggleFullscreen();
                        UserSettingsStore.SetFullscreen(ToggleScreenAPI.IsFullscreen());
                        DropdownState.ScreenDropdownOpen = false;
                        DropdownState.IsDropdownFocused  = false;
                        AudioSliderKnobBase.FocusFirstSlider();
                    }
                    else
                    {
                        // Confirmed current selection — just close the dropdown, stay on header
                        DropdownState.ScreenDropdownOpen = false;
                    }
                    return;
                }
                // Circle: close the dropdown, keep focus on the header (no navigation away)
                if (_circle)
                {
                    DropdownState.ScreenDropdownOpen = false;
                    return;
                }
            }
            else
            {
                // Dropdown is closed — Cross opens it; start highlight on the current row (top)
                // so D-pad Down is required to reach the alternate option before confirming.
                if (_cross)
                {
                    Console.WriteLine("[Dropdown] Opening dropdown via Cross");
                    DropdownState.HoveredOptionIndex = 0;
                    DropdownState.ScreenDropdownOpen = true;
                    DropdownState.SuppressOptionClickFrames = 2;
                    return;
                }
                // D-pad Down: move focus to Volume slider
                if (_dDown)
                {
                    DropdownState.IsDropdownFocused = false;
                    AudioSliderKnobBase.FocusFirstSlider();
                    return;
                }
                // D-pad Up: release focus back to NavigationButtons (back button)
                if (_dUp)
                {
                    DropdownState.IsDropdownFocused = false;
                    return;
                }
            }
        }

        float mx = input.WorldMouseX;
        float my = input.WorldMouseY;

        bool isHovered =
            mx >= t.X - baseScaleX && mx <= t.X + baseScaleX &&
            my >= t.Y - baseScaleY && my <= t.Y + baseScaleY;

        // Click header toggles dropdown open/close
        if (isHovered && input.IsMouseButtonTriggered(0))
        {
            bool wasOpen = DropdownState.ScreenDropdownOpen;
            DropdownState.ScreenDropdownOpen = !wasOpen;

            // If we JUST opened, suppress option click for 2 frames
            // (2 makes it robust even if script update order is weird)
            if (!wasOpen && DropdownState.ScreenDropdownOpen)
                DropdownState.SuppressOptionClickFrames = 2;

            return;
        }

        // Click outside closes dropdown (but NOT if click is inside open dropdown)
        if (DropdownState.ScreenDropdownOpen &&
            input.IsMouseButtonTriggered(0) &&
            !DropdownState.IsInsideOpenDropdown(mx, my) &&
            !DropdownState.IsInsideHeader(mx, my))
        {
            DropdownState.ScreenDropdownOpen = false;
        }
    }
}

// ======================================================
// Shows / hides the open dropdown graphic
// Attach to: DropdownBox (Open)
// ======================================================
public class DropdownOpenFollower : Script
{
    private bool initialized = false;
    private float originalScaleX;
    private float originalScaleY;

    public override void Update()
    {
        TransformComponent t = GetTransform();

        if (!initialized)
        {
            originalScaleX = t.ScaleX;
            originalScaleY = t.ScaleY;
            initialized = true;
        }

        // Decrement debounce here (runs every frame)
        if (DropdownState.SuppressOptionClickFrames > 0)
            DropdownState.SuppressOptionClickFrames--;

        // Publish open bounds using original size (even if hidden)
        DropdownState.OpenX = t.X;
        DropdownState.OpenY = t.Y;
        DropdownState.OpenHalfW = originalScaleX;
        DropdownState.OpenHalfH = originalScaleY;

        if (DropdownState.ScreenDropdownOpen)
        {
            t.ScaleX = originalScaleX;
            t.ScaleY = originalScaleY;
        }
        else
        {
            t.ScaleX = 0.0f;
            t.ScaleY = 0.0f;
        }
    }
}

// ======================================================
// OPTION: Fullscreen
// Attach to: the "FULL SCREEN" row entity (text OR sprite)
// ======================================================
public class DropdownOptionFullscreen : Script
{
    public float RowHitboxScaleY = 2.8f;
    public float RowPadY = 0.0f;

    private bool initialized = false;
    private float originalScaleX;
    private float originalScaleY;

    private bool   m_texInit    = false;
    private string m_defaultTex = "";
    private string m_hoverTex   = "";

    public override void Update()
    {
        // Show only when currently WINDOWED and dropdown is open
        if (ToggleScreenAPI.IsFullscreen() || !DropdownState.ScreenDropdownOpen)
        {
            HideSelf();
            if (m_texInit) { SpriteComponent _spr = GetSprite(); _spr.Texture = m_defaultTex; }
            return;
        }

        ShowSelf();

        InputComponent input = GetInput();
        TransformComponent t = GetTransform();

        if (!initialized)
        {
            originalScaleX = t.ScaleX;
            originalScaleY = t.ScaleY;
            initialized = true;
        }

        // Cache hover texture once
        if (!m_texInit)
        {
            SpriteComponent spr = GetSprite();
            m_defaultTex = spr.Texture ?? "";
            int dot = m_defaultTex.LastIndexOf('.');
            m_hoverTex = dot >= 0
                ? m_defaultTex.Substring(0, dot) + "_hover" + m_defaultTex.Substring(dot)
                : m_defaultTex + "_hover";
            m_texInit = true;
        }

        float mx = input.WorldMouseX;
        float my = input.WorldMouseY;

        float cx    = DropdownState.OpenX;
        float cy    = t.Y;
        float halfW = DropdownState.OpenHalfW;
        float halfH = (originalScaleY * RowHitboxScaleY) + RowPadY;

        bool hovered = mx >= cx - halfW && mx <= cx + halfW &&
                       my >= cy - halfH && my <= cy + halfH;

        // Hover texture: mouse hover OR controller navigated to this row (index 1)
        bool controllerOnThisRow = DropdownState.IsDropdownFocused && DropdownState.HoveredOptionIndex == 1;
        { SpriteComponent _spr = GetSprite(); _spr.Texture = (hovered || controllerOnThisRow) ? m_hoverTex : m_defaultTex; }

        if (DropdownState.SuppressOptionClickFrames > 0)
            return;

        if (hovered && input.IsMouseButtonTriggered(0))
        {
            ToggleScreenAPI.ToggleFullscreen();
            UserSettingsStore.SetFullscreen(ToggleScreenAPI.IsFullscreen());
            DropdownState.ScreenDropdownOpen = false;
        }
    }

    private void HideSelf()
    {
        TransformComponent t = GetTransform();
        if (!initialized)
        {
            originalScaleX = t.ScaleX;
            originalScaleY = t.ScaleY;
            initialized = true;
        }
        t.ScaleX = 0.0f;
        t.ScaleY = 0.0f;
    }

    private void ShowSelf()
    {
        TransformComponent t = GetTransform();
        if (!initialized)
        {
            originalScaleX = t.ScaleX;
            originalScaleY = t.ScaleY;
            initialized = true;
        }
        t.ScaleX = originalScaleX;
        t.ScaleY = originalScaleY;
    }
}

// ======================================================
// OPTION: Windowed
// Attach to: the "WINDOWED" row entity (for fullscreen state)
// ======================================================
public class DropdownOptionWindowed : Script
{
    public float RowHitboxScaleY = 2.8f;
    public float RowPadY = 0.0f;

    private bool initialized = false;
    private float originalScaleX;
    private float originalScaleY;

    private bool   m_texInit    = false;
    private string m_defaultTex = "";
    private string m_hoverTex   = "";

    public override void Update()
    {
        // Show only when currently FULLSCREEN and dropdown is open
        if (!ToggleScreenAPI.IsFullscreen() || !DropdownState.ScreenDropdownOpen)
        {
            HideSelf();
            if (m_texInit) { SpriteComponent _spr = GetSprite(); _spr.Texture = m_defaultTex; }
            return;
        }

        ShowSelf();

        InputComponent input = GetInput();
        TransformComponent t = GetTransform();

        if (!initialized)
        {
            originalScaleX = t.ScaleX;
            originalScaleY = t.ScaleY;
            initialized = true;
        }

        // Cache hover texture once
        if (!m_texInit)
        {
            SpriteComponent spr = GetSprite();
            m_defaultTex = spr.Texture ?? "";
            int dot = m_defaultTex.LastIndexOf('.');
            m_hoverTex = dot >= 0
                ? m_defaultTex.Substring(0, dot) + "_hover" + m_defaultTex.Substring(dot)
                : m_defaultTex + "_hover";
            m_texInit = true;
        }

        float mx = input.WorldMouseX;
        float my = input.WorldMouseY;

        float cx    = DropdownState.OpenX;
        float cy    = t.Y;
        float halfW = DropdownState.OpenHalfW;
        float halfH = (originalScaleY * RowHitboxScaleY) + RowPadY;

        bool hovered = mx >= cx - halfW && mx <= cx + halfW &&
                       my >= cy - halfH && my <= cy + halfH;

        // Hover texture: mouse hover OR controller navigated to this row (index 1)
        bool controllerOnThisRow = DropdownState.IsDropdownFocused && DropdownState.HoveredOptionIndex == 1;
        { SpriteComponent _spr = GetSprite(); _spr.Texture = (hovered || controllerOnThisRow) ? m_hoverTex : m_defaultTex; }

        if (DropdownState.SuppressOptionClickFrames > 0)
            return;

        if (hovered && input.IsMouseButtonTriggered(0))
        {
            ToggleScreenAPI.ToggleFullscreen();
            UserSettingsStore.SetFullscreen(ToggleScreenAPI.IsFullscreen());
            DropdownState.ScreenDropdownOpen = false;
        }
    }

    private void HideSelf()
    {
        TransformComponent t = GetTransform();
        if (!initialized)
        {
            originalScaleX = t.ScaleX;
            originalScaleY = t.ScaleY;
            initialized = true;
        }
        t.ScaleX = 0.0f;
        t.ScaleY = 0.0f;
    }

    private void ShowSelf()
    {
        TransformComponent t = GetTransform();
        if (!initialized)
        {
            originalScaleX = t.ScaleX;
            originalScaleY = t.ScaleY;
            initialized = true;
        }
        t.ScaleX = originalScaleX;
        t.ScaleY = originalScaleY;
    }

// ======================================================
// CLOSED LABEL: WINDOWED
// Attach to: ScreenLabel_Windowed
// Shows only when NOT fullscreen
// ======================================================
public class DropdownClosedLabelWindowed : Script
{
    private bool initialized = false;
    private float originalScaleX;
    private float originalScaleY;

    public override void Update()
    {
        TransformComponent t = GetTransform();

        // Cache original visible scale (make sure label is NOT saved at 0 in editor)
        if (!initialized)
        {
            originalScaleX = t.ScaleX;
            originalScaleY = t.ScaleY;
            initialized = true;
        }

        bool isFs = ToggleScreenAPI.IsFullscreen();

        if (!isFs)
        {
            // show
            t.ScaleX = originalScaleX;
            t.ScaleY = originalScaleY;
        }
        else
        {
            // hide
            t.ScaleX = 0.0f;
            t.ScaleY = 0.0f;
        }
    }
}

// ======================================================
// CLOSED LABEL: FULL SCREEN
// Attach to: ScreenLabel_Fullscreen
// Shows only when fullscreen
// ======================================================
public class DropdownClosedLabelFullscreen : Script
{
    private bool initialized = false;
    private float originalScaleX;
    private float originalScaleY;

    public override void Update()
    {
        TransformComponent t = GetTransform();

        // Cache original visible scale (make sure label is NOT saved at 0 in editor)
        if (!initialized)
        {
            originalScaleX = t.ScaleX;
            originalScaleY = t.ScaleY;
            initialized = true;
        }

        bool isFs = ToggleScreenAPI.IsFullscreen();

        if (isFs)
        {
            // show
            t.ScaleX = originalScaleX;
            t.ScaleY = originalScaleY;
        }
        else
        {
            // hide
            t.ScaleX = 0.0f;
            t.ScaleY = 0.0f;
        }
    }
}



}
