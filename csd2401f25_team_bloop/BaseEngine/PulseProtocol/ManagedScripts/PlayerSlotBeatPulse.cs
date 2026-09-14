/******************************************************************************/
/**
* @file        PlayerSlotBeatPulse.cs
* @project     Pulse Protocol
* @author      Chloe Lau Rey En
* @brief       Handles per-slot gold colour flash and scale pulse on the player
*              input key slot outlines, triggered externally by beat events in TurnManager.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/
using System;
using System.Collections.Generic;
using ScriptAPI;

public class PlayerSlotBeatPulse : Script
{
    // Default scale amplitude for per-slot beats (moderate)
    public float scaleAmp = 0.3f;

    // Gold colour
    public float targetR = 1.0f;
    public float targetG = 0.75f;
    public float targetB = 0.0f;

    // How long the individual slot flash lasts (enemy display)
    public float flashDuration = 0.35f;

    // How fast to lerp toward gold / white
    public float colourLerpSpeed = 6.0f;


    // Sorted by X position once all 4 instances have registered.
    private static List<PlayerSlotBeatPulse> _pending   = new List<PlayerSlotBeatPulse>();
    private static Dictionary<int, PlayerSlotBeatPulse> _instances = new Dictionary<int, PlayerSlotBeatPulse>();
    private static bool _assigned = false;

    private int  SlotIndex    = -1;
    private bool _registered  = false;

    public static void TriggerFlash(int slotIndex, float duration = -1f)
    {
        if (_instances.ContainsKey(slotIndex))
        {
            float d = duration > 0f ? duration : _instances[slotIndex].flashDuration;
            _instances[slotIndex]._flashTimer         = d;
            _instances[slotIndex]._flashTotalDuration = d;
        }
    }

    // amplitude < 0 uses the instance's default scaleAmp
    public static void TriggerScale(int slotIndex, float duration, float amplitude = -1f)
    {
        if (_instances.ContainsKey(slotIndex))
        {
            var inst = _instances[slotIndex];
            inst._scaleTimer     = duration;
            inst._scaleTotalDur  = duration;
            inst._scaleAmpActive = amplitude >= 0f ? amplitude : inst.scaleAmp;
        }
    }

    // Call this when a new play session starts to re-detect slots
    public static void ResetDetection()
    {
        // Reset instance-level registration state so reused C# objects re-register in the new session.
        foreach (var kvp in _instances)
        {
            kvp.Value._registered = false;
            kvp.Value.SlotIndex   = -1;
        }
        _pending.Clear();
        _instances.Clear();
        _assigned = false;
    }

    // Per-instance
    private bool  _scaleCached = false;
    private float _baseScaleX  = 1f;
    private float _baseScaleY  = 1f;

    private float _currentR = 1f;
    private float _currentG = 1f;
    private float _currentB = 1f;

    private float _flashTimer         = 0f;
    private float _flashTotalDuration = 0.35f;

    // Separate scale timer (independent from color flash)
    private float _scaleTimer     = 0f;
    private float _scaleTotalDur  = 0.35f;
    private float _scaleAmpActive = 0f;
    
    public override void Update()
    {
        // Auto-detect slot index by X position once all 4 instances are present
        if (!_registered)
        {
            if (!_pending.Contains(this))
                _pending.Add(this);

            if (_pending.Count >= 4 && !_assigned)
            {
                _pending.Sort((a, b) =>
                    a.GetTransform().X.CompareTo(b.GetTransform().X));

                _instances.Clear();
                for (int i = 0; i < _pending.Count && i < 4; i++)
                {
                    _pending[i].SlotIndex   = i;
                    _pending[i]._registered = true;
                    _instances[i]           = _pending[i];
                    Console.WriteLine($"[PlayerSlotBeatPulse] Outline slot {i} assigned at X={_pending[i].GetTransform().X:F1}");
                }
                _pending.Clear();
                _assigned = true;
            }
            return; // wait until assigned before doing anything else
        }

        if (Conductor.Instance == null)   return;
        if (TurnManager.Instance == null) return;

        TransformComponent t = GetTransform();
        SpriteComponent    s = GetSprite();

        if (!_scaleCached)
        {
            _baseScaleX  = t.ScaleX;
            _baseScaleY  = t.ScaleY;
            _scaleCached = true;
        }

        float dt = Time.DeltaTime;

        // Flash gold for this slot's player-turn beat; otherwise stay white
        if (_flashTimer > 0f)
        {
            _flashTimer -= dt;
            float ft     = Clamp01(_flashTimer / _flashTotalDuration);
            _currentR    = Lerp(1f, targetR, ft);
            _currentG    = Lerp(1f, targetG, ft);
            _currentB    = Lerp(1f, targetB, ft);
        }
        else
        {
            // Always lerp back to white when not flashing
            float speed = colourLerpSpeed * dt;
            _currentR   = Lerp(_currentR, 1f, speed);
            _currentG   = Lerp(_currentG, 1f, speed);
            _currentB   = Lerp(_currentB, 1f, speed);
        }

        s.TintR = _currentR;
        s.TintG = _currentG;
        s.TintB = _currentB;

        // Scale pulse: driven by separate _scaleTimer (independent from colour flash)
        if (_scaleTimer > 0f)
        {
            _scaleTimer -= dt;
            if (_scaleTimer < 0f) _scaleTimer = 0f;
            float ft    = _scaleTimer / _scaleTotalDur;
            float scale = 1f + _scaleAmpActive * ft;
            t.ScaleX = _baseScaleX * scale;
            t.ScaleY = _baseScaleY * scale;
        }
        else
        {
            t.ScaleX = _baseScaleX;
            t.ScaleY = _baseScaleY;
        }
    }

    private float Lerp(float a, float b, float t)
    {
        if (t > 1f) t = 1f;
        return a + (b - a) * t;
    }

    private float Clamp01(float v)
    {
        if (v < 0f) return 0f;
        if (v > 1f) return 1f;
        return v;
    }
}
