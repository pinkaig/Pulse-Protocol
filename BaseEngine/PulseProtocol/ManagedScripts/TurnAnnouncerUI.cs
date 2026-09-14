/******************************************************************************/
/**
* @file        TurnAnnouncer.cs
* @project     Pulse Protocol
* @author      Carrie Lam (70%)
*              Chloe Lau (30%)
* @brief       Displays phase announcer text with beat-synced animations to cue
*              the player on the current turn state: static "MEMORIZE!" during
*              enemy turn, and a scale-pop "INPUT NOW!" when player input opens.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
******************************************************************************/
using System;
using ScriptAPI;

public class TurnAnnouncerUI : Script
{
    public static TurnAnnouncerUI? Instance { get; private set; }

    // =========================
    // Config
    // =========================
    public string GoMessage    = "INPUT NOW!";
    public string EnemyMessage = "MEMORIZE!";

    // Pop + settle (INPUT NOW — stays visible, no fade)
    public float PopScale    = 1.6f;
    public float PopDuration = 0.12f;
    public float SettleScale = 1.1f;

    // =========================
    // State
    // =========================
    private bool initialized = false;
    private TurnPhase lastPhase = TurnPhase.Countdown;

    private enum GoState { Idle, Popping, Settled }
    private GoState _goState = GoState.Idle;
    private float   _goTimer = 0f;

    private bool  _scaleCached = false;
    private float _baseScaleX  = 1f;
    private float _baseScaleY  = 1f;

    public override void Update()
    {
        if (!initialized)
        {
            Initialize();
            return;
        }

        if (StageClearElement.IsCleared)
        {
            ResetScale();
            Hide();
            return;
        }

        if (NavigationButtons.ShowPauseOverlay)
            return;

        if (TurnManager.Instance == null)
            return;

        TurnPhase phase = TurnManager.Instance!.GetCurrentPhase();

        if (phase != lastPhase)
        {
            lastPhase = phase;
            OnPhaseChanged(phase);
        }

        // Pop + settle animation (INPUT NOW!)
        if (_goState == GoState.Popping)
        {
            _goTimer += Time.DeltaTime;
            float t01   = Math.Min(_goTimer / PopDuration, 1f);
            float eased = 1f - (1f - t01) * (1f - t01);
            float scale = PopScale + (SettleScale - PopScale) * eased;
            var tr = GetTransform();
            tr.ScaleX = _baseScaleX * scale;
            tr.ScaleY = _baseScaleY * scale;
            if (t01 >= 1f) _goState = GoState.Settled;
        }
    }

    // =========================
    // Initialization
    // =========================
    private void Initialize()
    {
        Instance  = this;
        lastPhase = TurnPhase.Countdown;
        _goState  = GoState.Idle;

        if (!_scaleCached)
        {
            var tr      = GetTransform();
            _baseScaleX = tr.ScaleX;
            _baseScaleY = tr.ScaleY;
            _scaleCached = true;
        }

        var text = GetTextComponent();
        text.SetAlignment(1);
        text.SetColor(1f, 1f, 1f);

        Hide();

        initialized = true;
        Console.WriteLine("[TurnAnnouncerUI] Initialized");
    }

    // =========================
    // Phase Change Handler
    // =========================
    private void OnPhaseChanged(TurnPhase phase)
    {
        switch (phase)
        {
            case TurnPhase.EnemyDisplay:
                ResetScale();
                var et = GetTextComponent();
                et.SetText(EnemyMessage);
                et.SetColor(1f, 1f, 1f);
                et.SetVisible(true);
                Console.WriteLine($"[TurnAnnouncerUI] >> {EnemyMessage}");
                break;

            default:
                ResetScale();
                Hide();
                break;
        }
    }

    // Called from TurnManager when player input opens
    public void ShowGo()
    {
        ResetScale();
        var text = GetTextComponent();
        text.SetText(GoMessage);
        text.SetColor(1f, 1f, 1f);
        text.SetVisible(true);

        var tr = GetTransform();
        tr.ScaleX = _baseScaleX * PopScale;
        tr.ScaleY = _baseScaleY * PopScale;

        _goState = GoState.Popping;
        _goTimer = 0f;
        Console.WriteLine($"[TurnAnnouncerUI] >> {GoMessage} (pop)");
    }

    // =========================
    // Helpers
    // =========================
    private void ResetScale()
    {
        _goState = GoState.Idle;
        _goTimer = 0f;
        if (_scaleCached)
        {
            var tr  = GetTransform();
            tr.ScaleX = _baseScaleX;
            tr.ScaleY = _baseScaleY;
        }
        GetTextComponent().SetColor(1f, 1f, 1f);
    }

    private void Hide()
    {
        GetTextComponent().SetVisible(false);
    }
}
