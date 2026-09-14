/******************************************************************************/
/**
 * @file        AudioSliderCore.cs
 * @project     Pulse Protocol
 * @author      Leu Jun Yong - 80%
 * @author      Goh Pin Kai - 20%
 *
 * @brief       main audio slider logic (mouse input, dragging, clamping, converting knob position to volume value).
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

using System;
using System.Collections.Generic;
using ScriptAPI;

// =========================================================
// Base (ABSTRACT) scripts: engine should NOT list abstract
// =========================================================

public abstract class AudioSliderKnobBase : Script
{
    // Must be provided by wrappers (Volume/Music/Sfx/Brightness)
    protected abstract string FixedTag { get; }

    // Empty bar width
    public float BarWidth = 1140.0f * 0.98f;

    // 2.0 => drag 2x farther, value changes 2x slower
    public float DragDistanceScale = 2.0f;

    public float TrackInset = 0.0f;
    public bool LockKnobY = true;

    // Start knob at MAX by default (100%)
    public bool StartAtMax = true;

    // Bigger clickable area (diamond -> bigger AABB)
    public float KnobHitboxScaleX = 2.2f;
    public float KnobHitboxScaleY = 2.2f;
    public float KnobHitboxPadX = 0.0f;
    public float KnobHitboxPadY = 0.0f;

    // ---- Published per tag ----
    public struct SliderState
    {
        public float Value01;

        public float TrackLeftX;
        public float TrackRightX;
        public float TrackY;

        public float FillLeftX;
        public float FillWidth;
    }

    public static Dictionary<string, SliderState> States = new Dictionary<string, SliderState>();
    private static bool s_persistedLoaded = false;
    private static bool s_appliedAudioOnce = false;

    // ---- runtime ----
    private bool initialized = false;
    private float lockedY;

    private float barLeftOuterX;
    private float knobMinX, knobMaxX;

    private bool dragging = false;
    private bool dragJustStarted = false;   // prevents instant cancel on click frame
    private float dragOffsetX = 0.0f;

    // ── Gamepad slider navigation (shared across Volume / Music / SFX instances) ──
    private static readonly List<AudioSliderKnobBase> s_sliders      = new List<AudioSliderKnobBase>();
    private static readonly List<AudioSliderKnobBase> s_prevSliders   = new List<AudioSliderKnobBase>();
    private static int   s_focusedIdx       = -1;
    private static int   s_sliderRegFrame   = -1;
    private static int   s_sliderNavFrame   = -1;
    private static bool  s_prevSettingsOpen = false;
    private static float s_pendingAdj       = 0f;
    private static int   s_pendingAdjFrame  = -1;

    private const int GP_DPAD_UP    = InputConstants.GP_DPAD_UP;
    private const int GP_DPAD_RIGHT = InputConstants.GP_DPAD_RIGHT;
    private const int GP_DPAD_DOWN  = InputConstants.GP_DPAD_DOWN;
    private const int GP_DPAD_LEFT  = InputConstants.GP_DPAD_LEFT;

    // How much each D-pad L/R press adjusts the value (0–1 scale).
    public float ControllerStep = 0.05f;
    // Scale multiplier applied to the knob sprite when it has gamepad focus.
    public float FocusScaleBoost = 1.15f;

    // Exposed so HoverableButton can suppress its D-pad nav while a slider is focused.
    public static bool IsAnySliderFocused => s_focusedIdx >= 0;

    // Called from HoverableButton when the settings panel closes.
    public static void ClearFocus()
    {
        s_focusedIdx = -1;
        s_prevSettingsOpen = false;  // force auto-focus to re-trigger when settings next opens
        DropdownState.IsDropdownFocused = false;
    }

    // Called from DropdownToggle when the user navigates down into sliders.
    // Records the frame so the nav handler suppresses the same D-pad press that caused the transfer.
    public static void FocusFirstSlider()
    {
        s_focusedIdx = 0;
        s_focusTransferFrame = Time.FrameCount;
    }

    private static int s_focusTransferFrame = -1;

    private bool  m_focused    = false;
    private float m_baseScaleX = 0f;
    private float m_baseScaleY = 0f;
    private bool  m_baseSaved  = false;

    // Robust held binding (click uses IsMouseButtonTriggered like your hover buttons)
    private object? cachedInputObj;
    private System.Reflection.MethodInfo? heldMI = null;
    private Type? heldParamType = null;

    public override void Update()
    {
        InputComponent input = GetInput();
        TransformComponent t = GetTransform();

        EnsurePersistedStateLoaded();
        ApplyPersistedAudioOnce();

        BindHeldIfNeeded(input);

        if (!initialized)
        {
            lockedY = t.Y;

            // SAVE the value BEFORE anything else
            float savedValue = -1f;
            if (States.ContainsKey(FixedTag))
            {
                savedValue = States[FixedTag].Value01;
            }

            float knobHalfW = t.ScaleX * 0.5f;

            if (StartAtMax)
            {
                // Assume knob starts at MAX
                float knobRightEdge = t.X + knobHalfW;
                float trackRightOuterX = knobRightEdge + TrackInset;
                barLeftOuterX = trackRightOuterX - (BarWidth * DragDistanceScale);
            }
            else
            {
                // Assume knob starts at MIN
                float knobLeftEdge = t.X - knobHalfW;
                barLeftOuterX = knobLeftEdge - TrackInset;
            }

            RecomputeTrack(t);

            // RESTORE knob position from saved value
            if (savedValue >= 0f)
            {
                t.X = knobMinX + (savedValue * (knobMaxX - knobMinX));
            }
            else
            {
                t.X = Clamp(t.X, knobMinX, knobMaxX);
            }

            if (LockKnobY) t.Y = lockedY;

            PublishState(ToValue01(t.X));
            initialized = true;
        }

        // Track settings open/close for auto-focus (runs even when slider is hidden).
        bool _settingsNowOpen = NavigationButtons.ShowSettingsOverlay || Scene.GetCurrentScene() == "Settings";
        if (_settingsNowOpen && !s_prevSettingsOpen)
            s_focusedIdx = 0;  // auto-focus first slider (Volume) when settings opens
        s_prevSettingsOpen = _settingsNowOpen;

        // =========================================================
        // IMPORTANT: When the settings UI is hidden, these slider
        // scripts must NOT respond to mouse input.
        //
        // We still initialize/sync above while hidden so the first
        // visible frame does not flash incorrect default values.
        // =========================================================
        if (!t.IsVisible || (Scene.GetCurrentScene() == "MainMenu" && !NavigationButtons.ShowSettingsOverlay))
        {
            // Cancel any in-progress drag so it can't keep moving while hidden.
            dragging = false;
            dragJustStarted = false;
            return;
        }
        else
        {
            RecomputeTrack(t);

            if (States.ContainsKey(FixedTag) && States[FixedTag].Value01 > 0.001f)
            {
                float savedValue = States[FixedTag].Value01;
                t.X = knobMinX + (savedValue * (knobMaxX - knobMinX));
            }
            else
            {
                t.X = Clamp(t.X, knobMinX, knobMaxX);
            }

            if (LockKnobY) t.Y = lockedY;

            PublishState(ToValue01(t.X));
            initialized = true;
        }

        // ─── Gamepad slider navigation ────────────────────────────────────────────
        int _frameNow = Time.FrameCount;

        // Save base scale once (before any focus boost is applied).
        if (!m_baseSaved)
        {
            m_baseScaleX = t.ScaleX;
            m_baseScaleY = t.ScaleY;
            m_baseSaved  = true;
        }

        // Rebuild per-frame candidate list.
        if (s_sliderRegFrame != _frameNow)
        {
            s_prevSliders.Clear();
            s_prevSliders.AddRange(s_sliders);
            s_sliders.Clear();
            s_sliderRegFrame = _frameNow;
        }
        s_sliders.Add(this);

        // Handle D-pad input once per frame (uses prev-frame list for stable ordering).
        // Skip when the screen mode dropdown has focus — DropdownToggle handles D-pad then.
        if (s_sliderNavFrame != _frameNow && s_prevSliders.Count > 0 && !DropdownState.IsDropdownFocused)
        {
            s_sliderNavFrame = _frameNow;

            bool _dUp    = input.IsGamepadButtonTriggered(GP_DPAD_UP);
            bool _dDown  = input.IsGamepadButtonTriggered(GP_DPAD_DOWN);
            bool _dLeft  = input.IsGamepadButtonTriggered(GP_DPAD_LEFT);
            bool _dRight = input.IsGamepadButtonTriggered(GP_DPAD_RIGHT);

            int _count = s_prevSliders.Count;

            // Up/Down cycles between sliders.
            // UP past the first slider → focus the screen mode dropdown.
            // DOWN past the last slider → release to NavigationButtons (back button).
            // Suppress on s_focusTransferFrame: that D-pad press came from DropdownToggle
            // and must not be re-consumed here.
            bool _suppressNav = (_frameNow == s_focusTransferFrame);
            if (s_focusedIdx >= 0 && (_dUp || _dDown) && !_suppressNav)
            {
                int _next = s_focusedIdx + (_dDown ? 1 : -1);
                if (_next < 0)
                {
                    s_focusedIdx = -1;
                    DropdownState.IsDropdownFocused = true;
                }
                else if (_next >= _count)
                {
                    // Already at the last slider — stay here, D-pad Down does nothing.
                }
                else
                {
                    s_focusedIdx = _next;
                }
            }

            // Left/Right adjusts the focused slider (auto-focuses first/last if none focused).
            if (_dLeft || _dRight)
            {
                if (s_focusedIdx < 0)
                    s_focusedIdx = _dLeft ? _count - 1 : 0;

                float _step = (s_focusedIdx >= 0 && s_focusedIdx < _count)
                              ? s_prevSliders[s_focusedIdx].ControllerStep
                              : ControllerStep;

                s_pendingAdj      = _dRight ? _step : -_step;
                s_pendingAdjFrame = _frameNow;
            }
        }

        // Apply the pending adjustment if this is the currently focused slider.
        int _myIdx = (s_prevSliders.Count > 0) ? s_prevSliders.IndexOf(this) : -1;
        if (_myIdx >= 0 && _myIdx == s_focusedIdx &&
            s_pendingAdjFrame == _frameNow && s_pendingAdj != 0f)
        {
            float _cur  = States.ContainsKey(FixedTag) ? States[FixedTag].Value01 : 0f;
            float _next = Clamp(_cur + s_pendingAdj, 0f, 1f);
            t.X = Clamp(knobMinX + _next * (knobMaxX - knobMinX), knobMinX, knobMaxX);
            if (LockKnobY) t.Y = lockedY;
            PublishState(_next);
            ApplyToAudioMixer(_next);
        }

        // Apply / remove focus scale boost on the knob sprite.
        bool _nowFocused = (_myIdx >= 0 && _myIdx == s_focusedIdx);
        if (_nowFocused && !m_focused)
        {
            t.ScaleX = m_baseScaleX * FocusScaleBoost;
            t.ScaleY = m_baseScaleY * FocusScaleBoost;
            m_focused = true;
        }
        else if (!_nowFocused && m_focused)
        {
            t.ScaleX = m_baseScaleX;
            t.ScaleY = m_baseScaleY;
            m_focused = false;
        }

        float mx = input.WorldMouseX;
        float my = input.WorldMouseY;

        // Enlarged knob hitbox (AABB)
        float hitW = (t.ScaleX * KnobHitboxScaleX) + (KnobHitboxPadX * 2.0f);
        float hitH = (t.ScaleY * KnobHitboxScaleY) + (KnobHitboxPadY * 2.0f);
        bool knobHovered = PointInRect(mx, my, t.X, t.Y, hitW, hitH);

        // Bar hover (for click-to-jump)
        var st = GetState();
        float hoverHalfH = Math.Max(20.0f, t.ScaleY * 1.5f);
        bool barHovered =
            (mx >= st.TrackLeftX && mx <= st.TrackRightX &&
             my >= lockedY - hoverHalfH && my <= lockedY + hoverHalfH);

        // Same "button style" click API you use everywhere
        bool click = input.IsMouseButtonTriggered(0);

        // For dragging we still need a "held" signal; bind it robustly
        bool held = MouseHeld(input, 0);

        // Start drag
        if (!dragging && click && (knobHovered || barHovered))
        {
            dragging = true;
            dragJustStarted = true;
            dragOffsetX = knobHovered ? (t.X - mx) : 0.0f;

            // Click bar (not knob) -> jump immediately
            if (barHovered && !knobHovered)
            {
                t.X = Clamp(mx, knobMinX, knobMaxX);
                if (LockKnobY) t.Y = lockedY;
                PublishState(ToValue01(t.X));
            }
        }

        // Stop drag after the grace frame when the button is no longer held
        if (dragging && !dragJustStarted && !held)
            dragging = false;

        // Drag movement (allow one grace frame so it doesn't instantly cancel)
        if (dragging && (held || dragJustStarted))
        {
            t.X = Clamp(mx + dragOffsetX, knobMinX, knobMaxX);
            if (LockKnobY) t.Y = lockedY;
            PublishState(ToValue01(t.X));
        }

        // End grace frame
        dragJustStarted = false;

        // Auto-apply to audio mixer (Brightness does nothing)
        ApplyToAudioMixer(GetState().Value01);
    }

    private void ApplyToAudioMixer(float v01)
    {
        string tag = FixedTag ?? "";

        if (tag.Equals("Volume", StringComparison.OrdinalIgnoreCase))
            GetAudio().SetMasterVolume(v01);
        else if (tag.Equals("Music", StringComparison.OrdinalIgnoreCase))
            GetAudio().SetMusicVolume(v01);
        else if (tag.Equals("SFX", StringComparison.OrdinalIgnoreCase) || tag.Equals("Sfx", StringComparison.OrdinalIgnoreCase))
            GetAudio().SetSfxVolume(v01);
    }

    private void EnsurePersistedStateLoaded()
    {
        if (s_persistedLoaded)
            return;

        s_persistedLoaded = true;

        LoadTag("Volume", 1.0f);
        LoadTag("Music", 1.0f);
        LoadTag("SFX", 0.25f);
        LoadTag("Brightness", 1.0f);
        LoadTag("Latency", 0.5f);
    }

    private void LoadTag(string tag, float fallback)
    {
        SliderState st;
        if (States.ContainsKey(tag))
            st = States[tag];
        else
            st = new SliderState();

        st.Value01 = UserSettingsStore.GetSliderValue(tag, fallback);
        States[tag] = st;
    }

    private void ApplyPersistedAudioOnce()
    {
        if (s_appliedAudioOnce)
            return;

        s_appliedAudioOnce = true;

        AudioComponent audio = GetAudio();
        audio.SetMasterVolume(UserSettingsStore.GetSliderValue("Volume", 1.0f));
        audio.SetMusicVolume(UserSettingsStore.GetSliderValue("Music", 1.0f));
        audio.SetSfxVolume(UserSettingsStore.GetSliderValue("SFX", 0.25f));
    }

    private void RecomputeTrack(TransformComponent knobT)
    {
        float knobHalfW = knobT.ScaleX * 0.5f;

        // Visual bar (fill range) stays BarWidth
        float visualRightOuterX = barLeftOuterX + BarWidth;

        float fillLeft = barLeftOuterX + TrackInset;
        float fillRight = visualRightOuterX - TrackInset;
        if (fillRight < fillLeft) fillRight = fillLeft;

        float fillWidth = fillRight - fillLeft;

        // Drag track is longer
        float effectiveBarWidth = BarWidth * DragDistanceScale;
        float trackRightOuterX = barLeftOuterX + effectiveBarWidth;

        float trackLeft = barLeftOuterX + TrackInset;
        float trackRight = trackRightOuterX - TrackInset;
        if (trackRight < trackLeft) trackRight = trackLeft;

        knobMinX = trackLeft + knobHalfW;
        knobMaxX = trackRight - knobHalfW;

        if (knobMaxX < knobMinX)
        {
            knobMinX = knobT.X;
            knobMaxX = knobT.X;
        }

        SliderState st = GetState();
        st.TrackLeftX = trackLeft;
        st.TrackRightX = trackRight;
        st.TrackY = lockedY;

        st.FillLeftX = fillLeft;
        st.FillWidth = fillWidth;

        States[FixedTag] = st;
    }

    private void PublishState(float value01)
    {
        SliderState st = GetState();
        st.Value01 = Clamp(value01, 0.0f, 1.0f);
        st.TrackY = lockedY;
        States[FixedTag] = st;
        UserSettingsStore.SetSliderValue(FixedTag, st.Value01);
    }

    private SliderState GetState()
    {
        if (!States.ContainsKey(FixedTag))
            States[FixedTag] = new SliderState();
        return States[FixedTag];
    }

    private float ToValue01(float knobX)
    {
        float denom = (knobMaxX - knobMinX);
        if (denom <= 0.0001f) return 0.0f;
        return Clamp((knobX - knobMinX) / denom, 0.0f, 1.0f);
    }

    private bool PointInRect(float px, float py, float cx, float cy, float w, float h)
    {
        float halfW = w * 0.5f;
        float halfH = h * 0.5f;
        return px >= cx - halfW && px <= cx + halfW &&
               py >= cy - halfH && py <= cy + halfH;
    }

    private float Clamp(float v, float lo, float hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    // =========================
    // Robust held binding (only)
    // =========================
    private void BindHeldIfNeeded(InputComponent input)
    {
        if (cachedInputObj == (object)input) return;
        cachedInputObj = input;

        heldMI = FindBool1ParamMethod(input,
            new string[] { "IsMouseButtonDown", "IsMouseButtonHeld", "IsMouseButtonPressed" },
            out heldParamType);
    }

    private System.Reflection.MethodInfo? FindBool1ParamMethod(object obj, string[] names, out Type? paramType)
    {
        paramType = null;
        Type tp = obj.GetType();

        foreach (string name in names)
        {
            System.Reflection.MethodInfo? best = null;
            Type? bestParam = null;

            foreach (var mi in tp.GetMethods(System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public))
            {
                if (mi.Name != name) continue;
                if (mi.ReturnType != typeof(bool)) continue;

                var ps = mi.GetParameters();
                if (ps == null || ps.Length != 1) continue;

                var pt = ps[0].ParameterType;

                if (best == null)
                {
                    best = mi; bestParam = pt;
                }
                else
                {
                    if (pt == typeof(int) && bestParam != typeof(int)) { best = mi; bestParam = pt; }
                    else if (pt == typeof(uint) && bestParam != typeof(int) && bestParam != typeof(uint)) { best = mi; bestParam = pt; }
                    else if (pt.IsEnum && !(bestParam == typeof(int) || bestParam == typeof(uint) || bestParam!.IsEnum)) { best = mi; bestParam = pt; }
                }
            }

            if (best != null)
            {
                paramType = bestParam;
                return best;
            }
        }

        return null;
    }

    private bool MouseHeld(InputComponent input, int button)
    {
        var mi = heldMI;
        var pt = heldParamType;
        if (mi == null || pt == null)
            return dragging; // fallback: keep dragging

        object arg = ConvertButtonArg(pt, button);

        try { return (bool?)mi.Invoke(input, new object[] { arg }) ?? dragging; }
        catch { return dragging; }
    }

    private object ConvertButtonArg(Type pt, int button)
    {
        if (pt == typeof(int)) return button;
        if (pt == typeof(uint)) return (uint)button;
        if (pt.IsEnum) return Enum.ToObject(pt, button);
        return button;
    }
}

public abstract class AudioSliderFillBase : Script
{
    protected abstract string FixedTag { get; }

    // 0=center, 1=left, 2=right
    public int PivotMode = 2;

    public bool SnapToKnobY = true;
    public float YOffset = 0.0f;

    // Uniform thickness
    public bool ForceUniformHeight = true;
    public float UniformHeight = 20.0f;

    private bool initialized = false;
    private float baseY;
    private float baseH;

    public override void Update()
    {
        TransformComponent t = GetTransform();

        if (!initialized)
        {
            baseY = t.Y;
            baseH = t.ScaleY;
            initialized = true;
        }

        if (!AudioSliderKnobBase.States.ContainsKey(FixedTag))
        {
            t.ScaleX = 0.0f;
            t.ScaleY = ForceUniformHeight ? UniformHeight : baseH;
            t.Y = baseY;
            return;
        }

        var st = AudioSliderKnobBase.States[FixedTag];

        float maxW = st.FillWidth;
        float v = st.Value01;

        float newW = (maxW <= 0.0001f) ? 0.0f : (maxW * v);

        t.ScaleX = newW;
        t.ScaleY = ForceUniformHeight ? UniformHeight : baseH;

        float left = st.FillLeftX;

        if (PivotMode == 0)      t.X = left + (newW * 0.5f);
        else if (PivotMode == 1) t.X = left;
        else                     t.X = left + newW;

        t.Y = SnapToKnobY ? (st.TrackY + YOffset) : baseY;
    }
}
