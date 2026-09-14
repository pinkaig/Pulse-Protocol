/******************************************************************************/
/**
* @file        BGEnemyScroll.cs
* @project     Pulse Protocol
* @brief       Scrolls background enemy entities across the screen. Hidden by
*              default; becomes visible when each pass starts (always from frame 0)
*              and hides again once it exits. All instances share the same
*              travel direction, randomised once every enemy finishes its pass.
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

using System;
using ScriptAPI;

public class BGEnemyScroll : Script
{
    public float  offScreenLeft   = -1600f;
    public float  offScreenRight  =  1600f;
    public float  minSpeed        =   300f;  // Min world units per second
    public float  maxSpeed        =   600f;  // Max world units per second
    public float  startDelay      =     0f;  // Stagger offset between enemies (seconds)
    public float  minGapDelay     =   0.2f;  // Min pause between passes (seconds)
    public float  maxGapDelay     =   0.8f;  // Max pause between passes (seconds)
    public float  minY            =  -154f;
    public float  maxY            =   445f;

    // Animation fields
    public string animName        = "enemy_BG"; // Sprite/animation name
    public int    animRow         =     5;      // Rows in spritesheet
    public int    animCol         =     6;      // Columns in spritesheet
    public int    animTotalFrames =    30;      // Total frames (5 * 6)
    public float  animSpeed       =     1f;     // Playback speed

    // Shared across all instances.
    // Direction randomises once per wave — locked for directionLockTime seconds
    // so all enemies starting a new pass close together use the same direction.
    public  float         directionLockTime = 3f;   // seconds to lock direction after a change
    private static bool   globalFromLeft    = true;
    private static float  directionTimer    = 999f;  // start high so first pass always picks
    private static Random sharedRng         = new Random();

    private enum State { StartDelay, Scrolling, Gap }

    private State state           = State.StartDelay;
    private float elapsed         = 0f;
    private float currentX        = 0f;
    private float savedY          = 0f;
    private float currentEndX     = 0f;
    private float currentGapDelay = 0f;
    private float moveDir         = 1f;
    private float currentSpeed    = 300f;
    private bool  registered      = false;
    private static int totalInstances = 0;
    private bool  lastFromLeft    = true;
    private bool  scaleInitialized = false;

    public override void Update()
    {
        if (!registered)
        {
            totalInstances++;
            registered = true;
        }

        // Advance direction timer — divided by instance count so it runs at real speed
        directionTimer += Time.DeltaTime / totalInstances;

        if (state == State.StartDelay)
        {
            elapsed += Time.DeltaTime;

            // Keep hidden and parked while waiting
            TransformComponent tf = GetTransform();
            tf.IsVisible = false;
            tf.X         = offScreenLeft;

            if (elapsed >= startDelay)
            {
                elapsed = 0f;
                BeginScroll();
            }
        }
        else if (state == State.Scrolling)
        {
            currentX += moveDir * currentSpeed * Time.DeltaTime;

            TransformComponent transform = GetTransform();
            transform.X         = currentX;
            transform.Y         = savedY;
            transform.IsVisible = true;

            bool reachedEnd = moveDir > 0f
                ? currentX >= currentEndX
                : currentX <= currentEndX;

            if (reachedEnd)
            {
                currentX            = currentEndX;
                transform.X         = currentEndX;
                transform.IsVisible = false;

                // Stop animation so it waits on current frame while hidden
                GetAnimation().SetAnimationSpeed(0f);

                currentGapDelay = minGapDelay + (float)(sharedRng.NextDouble() * (maxGapDelay - minGapDelay));
                elapsed = 0f;
                state   = State.Gap;
            }
        }
        else if (state == State.Gap)
        {
            elapsed += Time.DeltaTime;

            // Stay hidden and parked off-screen
            TransformComponent tf = GetTransform();
            tf.IsVisible = false;
            tf.X         = currentEndX;

            if (elapsed >= currentGapDelay)
            {
                elapsed = 0f;
                BeginScroll();
            }
        }
    }

    private void BeginScroll()
    {
        // Randomise direction once per wave — all enemies finishing close together agree
        if (directionTimer >= directionLockTime)
        {
            globalFromLeft = sharedRng.Next(0, 2) == 0;
            directionTimer = 0f;
        }

        bool fromLeft = globalFromLeft;

        moveDir     = fromLeft ? 1f   : -1f;
        currentX    = fromLeft ? offScreenLeft  : offScreenRight;
        currentEndX = fromLeft ? offScreenRight : offScreenLeft;

        savedY       = minY + (float)(sharedRng.NextDouble() * (maxY - minY));
        currentSpeed = minSpeed + (float)(sharedRng.NextDouble() * (maxSpeed - minSpeed));

        // Reset animation to frame 0 before making it visible
        GetAnimation().SetAnimation(animName, animRow, animCol, animTotalFrames, animSpeed);

        TransformComponent tf = GetTransform();
        tf.X         = currentX;
        tf.Y         = savedY;
        tf.IsVisible = false; // Scrolling state will make it visible next frame

        // Only flip ScaleX when direction actually changes
        if (!scaleInitialized || fromLeft != lastFromLeft)
        {
            tf.ScaleX        = fromLeft ? -Math.Abs(tf.ScaleX) : Math.Abs(tf.ScaleX);
            lastFromLeft     = fromLeft;
            scaleInitialized = true;
        }

        state = State.Scrolling;
    }
}
