using System;
using ScriptAPI;

public enum EnemyIconType { None, Rat, Bat, BlueGorilla, Boss }

public class OrderSlipUIController : Script
{
    public static OrderSlipUIController? Instance;

    // Tunables
    //public float currentScale = 1.0f; //NO SHIT IT DISAPEAR ITS 1
    //public float nextScale = 0.75f;   //NO SHIT IT DISAPEAR ITS 0.75
    public float currentScale = 200f;
    public float nextScale = 120f;
    public float fadeDuration = 0.5f;

    // State
    // State
    public EnemyIconType CurrentType { get; private set; } = EnemyIconType.None;
    public EnemyIconType NextType { get; private set; } = EnemyIconType.None;

    public bool Transitioning { get; private set; } = false;
    public float Fade01 { get; private set; } = 0.0f;

    public float CurrentFadeOut => 1.0f - Fade01;  // Current icon fades out
    public float NextFadeIn => Fade01;             // Next icon fades in

    public float CurrentSlotX { get; private set; }
    public float CurrentSlotY { get; private set; }
    public float NextSlotX { get; private set; }
    public float NextSlotY { get; private set; }

    private bool hasCurrentSlot = false;
    private bool hasNextSlot = false;

    private bool initialized = false;

    public override void Update()
    {
        // First-frame setup - establish singleton immediately
        if (!initialized)
        {
            Instance = this;
            initialized = true;
            return;
        }

        if (!Transitioning) return;

        float dt = Time.DeltaTime;

        // Update fade progress
        Fade01 = (fadeDuration <= 0.0001f) ? 1.0f : (Fade01 + dt / fadeDuration);
        if (Fade01 > 1.0f) Fade01 = 1.0f;

        // End transition when fade completes
        if (Fade01 >= 1.0f)
        {
            Transitioning = false;
            Console.WriteLine("[OrderSlipUI] Transition complete");
        }
    }

    // Call once externally after scripts exist (or from a bootstrap script)
    public void Init(EnemyIconType current, EnemyIconType upcoming)
    {
        if (Transitioning)
        {
            Console.WriteLine("[OrderSlipUI] WARNING: Init called during transition - ignoring");
            return;
        }

        CurrentType = current;
        NextType = upcoming;

        Transitioning = false;
        Fade01 = 0.0f;

        Console.WriteLine($"[OrderSlipUI] Init Current={CurrentType}, Next={NextType}");
    }

    public void AdvanceQueue(EnemyIconType newUpcoming)
    {
        if (Transitioning)
        {
            Console.WriteLine("[OrderSlipUI] WARNING: AdvanceQueue called during transition - ignoring");
            return;
        }

        if (!SlotsReady())
        {
            Console.WriteLine("[OrderSlipUI] WARNING: Slots not registered yet - cannot advance");
            return;
        }

        // Shift queue forward
        CurrentType = NextType;
        NextType = newUpcoming;

        // Start transition
        Transitioning = true;
        Fade01 = 0.0f;

        Console.WriteLine($"[OrderSlipUI] Advance -> Current={CurrentType}, Next={NextType}");
    }

    public void RegisterCurrentSlot(float x, float y)
    {
        CurrentSlotX = x;
        CurrentSlotY = y;
        hasCurrentSlot = true;
        Console.WriteLine($"[OrderSlipUI] Current slot registered at ({x}, {y})");
    }

    public void RegisterNextSlot(float x, float y)
    {
        NextSlotX = x;
        NextSlotY = y;
        hasNextSlot = true;
        Console.WriteLine($"[OrderSlipUI] Next slot registered at ({x}, {y})");
    }

    public bool SlotsReady()
    {
        return hasCurrentSlot && hasNextSlot;
    }

    public string GetVibrantSprite(EnemyIconType type)
    {
        switch (type)
        {
            case EnemyIconType.Rat: return "enemyicon_rat";
            case EnemyIconType.Bat: return "enemyicon_bat";
            case EnemyIconType.BlueGorilla: return "enemyicon_gorilla";
            case EnemyIconType.Boss: return "enemyicon_boss";
            default: return "";
        }
    }

    public string GetDimSprite(EnemyIconType type)
    {
        switch (type)
        {
            case EnemyIconType.Rat: return "enemyicon_rat_grey";
            case EnemyIconType.Bat: return "enemyicon_bat_grey";
            case EnemyIconType.BlueGorilla: return "enemyicon_gorilla_grey";
            case EnemyIconType.Boss: return "enemyicon_boss_grey";
            default: return "";
        }
    }
}