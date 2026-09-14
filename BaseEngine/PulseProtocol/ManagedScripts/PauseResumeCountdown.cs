/******************************************************************************/
/**
* @file        PauseResumeCountdown.cs
* @project     Pulse Protocol
* @author      Ban Kai Wei Benjamin
* @brief       adds countdown after resuming gameplay from pause state
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ScriptAPI;
using System.Text.Json;
using System.IO;

public class PauseResumeCountdown : Script
{
    public static PauseResumeCountdown? Instance { get; private set; }
    public static bool IsRunning { get; private set; } = false;


    public float TextScale = 200f;
    public float StepDuration = 0.75f;
    public bool AnimateScale = true;
    public float PulseAmount = 1.25f;

    private bool initialized = false;
    private float timer = 0f;
    private int step = 0; // 0=3, 1=2, 2=1, 3=GO
    private float currentScale = 1.0f;
    private float targetScale = 1.0f;
    private float scaleVelocity = 0.0f;

    public override void Update()
    {
        if (!initialized)
        {
            Instance = this;   // always replace with current scene's instance
            initialized = true;
            Hide();
        }

        if (!IsRunning)
            return;

        timer += SafeDeltaTime();

        if (AnimateScale)
            AnimatePulse();

        if (timer >= StepDuration)
        {
            timer = 0f;
            step++;

            if (step > 3)
            {
                FinishResume();
                return;
            }

            UpdateDisplay();
        }
    }

    private void Initialize()
    {
        if (Instance == null)
            Instance = this;

        Hide();
        initialized = true;
    }

    public void StartCountdown()
    {
        IsRunning = true;
        timer = 0f;
        step = 0;

        Show();
        UpdateDisplay();
    }

    private void FinishResume()
    {
        Hide();
        IsRunning = false;

        NavigationButtons.NotifyResumeCountdownFinished();
    }

    private void UpdateDisplay()
    {
        string displayText = "";
        switch (step)
        {
            case 0: displayText = "3"; break;
            case 1: displayText = "2"; break;
            case 2: displayText = "1"; break;
            case 3: displayText = "GO!"; break;
        }

        TextComponentAPI text = GetTextComponent();
        text.SetText(displayText);
        text.SetVisible(true);

        TransformComponent tf = GetTransform();
        tf.IsVisible = true;

        if (AnimateScale)
        {
            currentScale = 1.0f;
            targetScale = PulseAmount;
            scaleVelocity = 0.0f;
        }

        tf.ScaleX = TextScale * currentScale;
        tf.ScaleY = TextScale * currentScale;
    }

    private void Show()
    {
        TextComponentAPI text = GetTextComponent();

        text.SetVisible(true);

        TransformComponent tf = GetTransform();
        tf.IsVisible = true;

        currentScale = 1.0f;
        targetScale = PulseAmount;
        scaleVelocity = 0.0f;
    }

    private void Hide()
    {
        TextComponentAPI text = GetTextComponent();
        text.SetVisible(false);

        TransformComponent tf = GetTransform();
        tf.IsVisible = false;
    }

    private void AnimatePulse()
    {
        float dt = SafeDeltaTime();
        float smoothTime = 0.15f;

        currentScale = SmoothDamp(currentScale, targetScale, ref scaleVelocity, smoothTime, dt);

        TransformComponent tf = GetTransform();
        tf.ScaleX = TextScale * currentScale;
        tf.ScaleY = TextScale * currentScale;

        if (Math.Abs(currentScale - targetScale) < 0.01f)
            targetScale = 1.0f;
    }

    private float SafeDeltaTime()
    {
        float dt = Time.DeltaTime;
        return dt > 0f ? dt : 0.016f;
    }

    private float SmoothDamp(float current, float target, ref float velocity, float smoothTime, float deltaTime)
    {
        smoothTime = Math.Max(0.0001f, smoothTime);

        float omega = 2.0f / smoothTime;
        float x = omega * deltaTime;
        float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

        float change = current - target;
        float originalTarget = target;

        float maxChange = float.MaxValue * smoothTime;
        change = Math.Max(-maxChange, Math.Min(change, maxChange));
        target = current - change;

        float temp = (velocity + omega * change) * deltaTime;
        velocity = (velocity - omega * temp) * exp;
        float output = target + (change + temp) * exp;

        if ((originalTarget - current > 0.0f) == (output > originalTarget))
        {
            output = originalTarget;
            velocity = (output - originalTarget) / deltaTime;
        }

        return output;
    }
}