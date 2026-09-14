/******************************************************************************/
/**
* @file        AnimatedBackground.cs
* @project     Pulse Protocol
* @author      Chloe Lau Rey En
* @brief       Animates the pause and settings screen background with randomised frame timing,
*              including occasional freezes and flickers for an organic feel.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using ScriptAPI;

public class AnimatedBackground : Script
{
    private int currentFrame = 1;
    private float frameTimer = 0f;
    private float frameDelay = 0.12f;
    private bool reverse = false;
    private Random rand = new Random();

    // DEFAULT VALUES; WILL GET OVERRIDEN
    protected virtual bool ShouldShow() => false;
    protected virtual string FramePrefix() => "";
    protected virtual int TotalFrames() => 14;

    public override void Update()
    {
        TransformComponent t = GetTransform();
        SpriteComponent sprite = GetSprite();
        bool isShowing = ShouldShow();
        
        t.IsVisible = isShowing;
        
        if (!isShowing) return;
        
        float deltaTime = Time.DeltaTime;
        if (deltaTime == 0f)
            deltaTime = 0.016f;
        
        frameTimer += deltaTime;
        
        if (frameTimer >= frameDelay)
        {
            // 10% chance to freeze on current frame for a bit
            if (rand.NextDouble() < 0.10)
            {
                frameDelay = 0.5f + (float)(rand.NextDouble() * 0.5f); // hold 0.5 - 1.0s
                frameTimer = 0f;
                return;
            }

            // 10% chance to flicker quickly
            if (rand.NextDouble() < 0.10)
            {
                frameDelay = 0.03f; // very fast flicker
            }
            else
            {
                // Normal range but much wider: 0.06 to 0.24s
                frameDelay = 0.06f + (float)(rand.NextDouble() * 0.18f);
            }

            // Ping-pong animation
            if (reverse)
            {
                currentFrame--;
                if (currentFrame < 1)
                {
                    currentFrame = 2;
                    reverse = false;
                }
            }
            else
            {
                currentFrame++;
                if (currentFrame > TotalFrames())
                {
                    currentFrame = TotalFrames() - 1;
                    reverse = true;
                }
            }
            
            sprite.Texture = $"{FramePrefix()}{currentFrame}";
            frameTimer = 0f;
        }
    }
}

// pause menu animation overlay
public class PauseAnimatedBackground : AnimatedBackground
{
    protected override bool ShouldShow() =>
        NavigationButtons.ShowPauseOverlay ||
        (NavigationButtons.ShowSettingsOverlay && NavigationButtons.IsSettingsOpenedFromPause);
    protected override string FramePrefix() => "pause";
        protected override int TotalFrames() => 19;
}

// settings menu animation overlay
public class SettingsAnimatedBackground : AnimatedBackground
{
    protected override bool ShouldShow() => NavigationButtons.ShowSettingsOverlay;
    protected override string FramePrefix() => "settings";
    protected override int TotalFrames() => 19;
}
