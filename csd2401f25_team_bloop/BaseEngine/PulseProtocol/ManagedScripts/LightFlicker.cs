/******************************************************************************/
/**
* @file        LightFlicker.cs
* @project     Pulse Protocol
* @brief       Simulates a light flicker effect by randomly flickering the
*              entity's sprite alpha between on and off at short random intervals.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using ScriptAPI;

public class LightFlicker : Script
{
    public float minOnTime  = 0.2f;   // Min seconds the light stays on
    public float maxOnTime  = 1.0f;   // Max seconds the light stays on
    public float minOffTime = 0.08f;  // Min seconds the light stays off
    public float maxOffTime = 0.3f;   // Max seconds the light stays off
    public float flickerChance = 0.3f; // 0-1: how often it flickers (lower = more stable)
    public float dimAlpha   = 0.15f;  // Alpha when "off" (0 = fully off, >0 = dim glow)

    private Random rng     = new Random();
    private float  timer   = 0f;
    private bool   isOn    = true;

    public override void Update()
    {
        timer -= Time.DeltaTime;

        if (timer <= 0f)
        {
            if (isOn)
            {
                // Decide whether to flicker off or stay on
                if (rng.NextDouble() < flickerChance)
                {
                    isOn  = false;
                    timer = minOffTime + (float)(rng.NextDouble() * (maxOffTime - minOffTime));
                }
                else
                {
                    // Stay on for a bit longer before next check
                    timer = minOnTime + (float)(rng.NextDouble() * (maxOnTime - minOnTime));
                }
            }
            else
            {
                isOn  = true;
                timer = minOnTime + (float)(rng.NextDouble() * (maxOnTime - minOnTime));
            }

            SpriteComponent sprite = GetSprite();
            sprite.TintA = isOn ? 1.0f : dimAlpha;
        }
    }
}
