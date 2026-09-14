using System;
using ScriptAPI;

public enum EnemyState { Idle, Moving, Attacking, Hurt, Dead }
public enum RhythmType { OneBeat, TwoBeat, Syncopated, FourBeat }

public class EnemyRhythmController : Script
{
    // =========================
    // Global Access (Singleton)
    // =========================
    public static EnemyRhythmController? Instance { get; private set; }


    // === SPRITE SHEET SETTINGS ===
    public string IdleSpriteSheet   = "rat_idle";
    public string WalkSpriteSheet   = "rat_walk";
    public string AttackSpriteSheet = "rat_attack";
    public string HurtSpriteSheet   = "rat_hurt";
    public string DeathSpriteSheet  = "rat_death";

    public int IdleFrames = 15;
    public int IdleColumns = 4;
    public int IdleRows = 4;

    public int WalkFrames = 15;
    public int WalkColumns = 4;
    public int WalkRows = 4;

    public int AttackFrames = 20;
    public int AttackColumns = 5;
    public int AttackRows = 4;

    public int HurtFrames = 15;
    public int HurtColumns = 4;
    public int HurtRows = 4;

    public int DeathFrames = 15;
    public int DeathColumns = 4;
    public int DeathRows = 4;

    public float AnimationSpeed = 12f;
    public float SizeMultiplier = 2.0f;

    // === RHYTHM SETTINGS ===
    public RhythmType RhythmType = RhythmType.Syncopated;
    public float StepDistance = 50f;
    public float[] SlotOffsets = new float[] { 0.0f, 0.5f, 1.0f, 1.5f };
    // Which UI slot positions (0-3) each key occupies.
    // Rat uses all 4 slots — no gaps, just press everything on the beat.
    public int[] VisualSlotIndices = new int[] { 0, 1, 2, 3 };

    // If true, WeaknessCombo is re-randomized each turn (2 distinct keys from WASD)
    public bool RandomizeWeakness = true;
    // If true, VisualSlotIndices is re-randomized each turn (shifts where keys appear visually)
    public bool RandomizeVisualSlots = false;
    protected static readonly Random _rng = new Random();

    // === COMBAT SETTINGS ===
    public int AttackDamage = 5;
    public float Health = 15f;
    public int MaxHP { get; protected set; } = 1;

    // === HEALTH BAR SETTINGS ===
    public int HealthBarBlockCount = 4;
    public float HealthBarOffsetX = -102f;
    public float HealthBarOffsetY = -55f;
    public float HealthBarScaleOffsetXFactor = 0f;
    public float HealthBarScaleOffsetYFactor = 0.78f;

    // === COLLISION SETTINGS ===
    // "AABB"     = box collision (uses AABB components, may be affected by BroadPhase filtering)
    // "Edge"     = edge contact with threshold (uses AABB components)
    // "Distance" = radius-based via engine (uses transforms only, bypasses BroadPhase)
    // "Manual"   = uses engine CheckDistance with ManualAttackRange as radius (simplest, most reliable)
    public string CollisionMode = "AABB";

    // For Distance mode
    public float SelfCollisionRadius = 25f;
    public float PlayerCollisionRadius = 25f;

    // For Edge mode
    public float EdgeContactThreshold = 5f;

    public float ManualAttackRange = 350f;

    // === PLAYER REFERENCE ===
    private int playerEntityID = -1;
    private bool playerFound = false;

    // === WEAKNESS ===
    public string WeaknessCombo = "WASD";

    // === INTERNAL STATE ===
    private bool initialized = false;
    private int initializedForEntityID = -1; // Guards against C# object reuse across sessions
    private int lastBeat = -1;
    private bool animPaused = false;
    protected bool isDead = false;
    public bool IsDying => isDead;        // True from Die() until Instance is cleared after animation
    public bool IsPendingDeath => pendingDeath; // True from killing blow until Die() plays out
    private EnemyState currentState = EnemyState.Idle;

    private float attackCooldownTimer = 0f;
    private float hurtTimer = 0f;
    private float deathTimer = 0f;
    private bool waitingToDie = false;
    private bool deathNotified = false;  // true once spawner has been told to continue
    private bool pendingDeath = false;          // true when hp hit 0 but hurt animation hasn't played yet
    private float pendingDeathFallbackTimer = 0f; // safety: die after 3s if hurt never triggered

    // Track movement during enemy turn
    private int movementStepsTaken = 0;
    protected int maxMovementSteps = 4;

    // Once the enemy reaches the player, it STOPS moving entirely
    private bool hasReachedPlayer = false;

    // Track if we've tried to set weakness
    private bool weaknessSet = false;
    private int weaknessRetryCount = 0;
    private const int maxWeaknessRetries = 100;
    // Track which ComboSystem instance the weakness was set on.
    // If ComboSystem.Instance changes (new play session), we re-set so we never
    // leave the weakness pinned to a stale ComboSystem from a previous session.
    private ComboSystem? weaknessSetOnInstance = null;

    // Step SFX alternation
    private bool stepSFXToggle = false;

    // Debug
    private int collisionCheckCount = 0;

    public override void Update()
    {
        if (StageClearElement.IsCleared)
        {
            if (initialized && currentState != EnemyState.Dead && currentState != EnemyState.Hurt)
                SetState(EnemyState.Idle);
            return;
        }

        // Detect if this C# object has been reused for a new entity 
        if (initialized && entityID != initializedForEntityID)
        {
            Console.WriteLine($"[Enemy] Entity ID changed ({initializedForEntityID} -> {entityID}), re-initializing...");
            initialized = false;
        }

        if (!initialized)
        {
            Initialize();
            initialized = true;
            return;
        }

        if (!playerFound)
        {
            TryFindPlayer();
        }

        if (!weaknessSet && weaknessRetryCount < maxWeaknessRetries)
        {
            TrySetWeakness();
            weaknessRetryCount++;
        }

        // Freeze animation during countdown; resume when GO! fires
        if (!GameStart.Ready)
        {
            if (!animPaused)
            {
                animPaused = true;
                Animation anim = GetAnimation();
                anim.SetAnimation(IdleSpriteSheet, IdleRows, IdleColumns, IdleFrames, 0f);
            }
            return;
        }
        if (animPaused)
        {
            animPaused = false;
            Animation anim = GetAnimation();
            anim.SetAnimation(IdleSpriteSheet, IdleRows, IdleColumns, IdleFrames, AnimationSpeed);
        }

        if (isDead)
        {
            if (waitingToDie)
            {
                deathTimer += Time.DeltaTime;
                if (deathTimer >= DeathFrames / AnimationSpeed)
                {
                    TransformComponent tf = GetTransform();
                    tf.IsVisible = false;

                    // Only notify the spawner once, AFTER the death animation has played out.
                    if (!deathNotified)
                    {
                        deathNotified = true;
                        Instance = null;                           // clear singleton now that animation is done
                        EnemySpawner.Instance?.OnEnemyDefeated();  // tell spawner it's safe to continue
                    }
                }
            }
            return;
        }

        if (attackCooldownTimer > 0f)
        {
            attackCooldownTimer -= Time.DeltaTime;
            if (attackCooldownTimer <= 0f)
            {
                if (currentState == EnemyState.Attacking)
                    SetState(EnemyState.Idle);
            }
        }

        if (hurtTimer > 0f)
        {
            hurtTimer -= Time.DeltaTime;
            if (hurtTimer <= 0f)
            {
                if (currentState == EnemyState.Hurt)
                    FinishHurt();
            }
        }

        // Safety fallback: if hurt was never triggered (e.g. cheat kill), die after 3 seconds
        if (pendingDeath && !isDead && currentState != EnemyState.Hurt)
        {
            pendingDeathFallbackTimer += Time.DeltaTime;
            if (pendingDeathFallbackTimer >= 3f)
                Die();
        }

        if (Conductor.Instance != null)
        {
            int currentBeat = Conductor.Instance.GetCurrentBeat();
            if (currentBeat != lastBeat)
            {
                lastBeat = currentBeat;
                HandleBeat(currentBeat);
            }
        }
    }

    private void Initialize()
    {
        Instance = this;
        initializedForEntityID = entityID; // Stamp the entity this init belongs to

        // Full state reset
        isDead = false;
        waitingToDie = false;
        deathTimer = 0f;
        deathNotified = false;
        hasReachedPlayer = false;
        movementStepsTaken = 0;
        attackCooldownTimer = 0f;
        hurtTimer = 0f;
        pendingDeath = false;
        pendingDeathFallbackTimer = 0f;
        collisionCheckCount = 0;
        playerFound = false;
        playerEntityID = -1;
        lastBeat = -1;
        animPaused = false;
        stepSFXToggle = false;
        SetupSprites();

        // Reset weakness retry state so each new enemy gets a fresh attempt
        weaknessSet = false;
        weaknessRetryCount = 0;
        weaknessSetOnInstance = null;

        Console.WriteLine("[Enemy] Initializing...");

        TransformComponent tf = GetTransform();

        // Force the idle spritesheet to load regardless of the previous currentState.
        currentState = EnemyState.Dead;
        SetState(EnemyState.Idle);

        if (Conductor.Instance != null)
        {
            lastBeat = Conductor.Instance.GetCurrentBeat();
            Console.WriteLine("[Enemy] Conductor connected");
        }

        TryFindPlayer();
        TrySetWeakness();

        HealthComponent eh = GetHealth();
        MaxHP = Math.Max(1, eh.hp);
        Health = eh.hp;
        MaxHP = Math.Max(1, eh.hp);
        if (HealthBarBlockCount < 2) HealthBarBlockCount = 2;
        if (HealthBarBlockCount > 4) HealthBarBlockCount = 4;
        eh.isAlive = true;
        if (HealthBarBlockCount < 2) HealthBarBlockCount = 2;
        if (HealthBarBlockCount > 4) HealthBarBlockCount = 4;
        Console.WriteLine($"[Enemy] Initialized at ({tf.X:F1}, {tf.Y:F1}) | Health: {eh.hp}");
        Console.WriteLine($"[Enemy] Collision mode: {CollisionMode} | ManualAttackRange: {ManualAttackRange}");
        Console.WriteLine($"[Enemy] AttackDamage: {AttackDamage}");
        Console.WriteLine($"[Enemy] Own entity ID: {entityID}");
    }

    // =========================
    // Player Discovery
    // =========================
    private void TryFindPlayer()
    {
        if (playerFound) return;

        if (PlayerHealth.Instance != null)
        {
            playerEntityID = PlayerHealth.Instance.PlayerEntityID;

            if (playerEntityID >= 0)
            {
                playerFound = true;
                Console.WriteLine($"[Enemy] Found player entity ID: {playerEntityID}");
                RunCollisionDiagnostic();
            }
            else
            {
                Console.WriteLine("[Enemy] PlayerHealth exists but entity ID not set yet");
            }
        }
    }

    // =========================
    // Diagnostic: test collision modes once at startup
    // =========================
    private void RunCollisionDiagnostic()
    {
        Console.WriteLine("[Enemy] === COLLISION DIAGNOSTIC ===");
        TransformComponent myTf = GetTransform();
        Console.WriteLine($"[Enemy] Enemy pos: ({myTf.X:F1}, {myTf.Y:F1}), scale: ({myTf.ScaleX:F1}, {myTf.ScaleY:F1})");
        Console.WriteLine($"[Enemy] Player entity ID: {playerEntityID}");

        CollisionComponent col = GetCollision();

        bool bigRadius = col.CheckDistance(50000f, playerEntityID, 50000f);
        Console.WriteLine($"[Enemy] Test CheckDistance(50000, 50000): {bigRadius}");

        bool manualTest = col.CheckDistance(ManualAttackRange, playerEntityID, 0f);
        Console.WriteLine($"[Enemy] Test CheckDistance({ManualAttackRange}, 0): {manualTest}");

        bool aabbTest = col.CheckAABB(playerEntityID);
        Console.WriteLine($"[Enemy] Test CheckAABB: {aabbTest}");

        Console.WriteLine("[Enemy] === END DIAGNOSTIC ===");
    }

    // =========================
    // Collision Detection
    // =========================
    private bool IsCollidingWithPlayer()
    {
        if (!playerFound || playerEntityID < 0)
            return false;

        collisionCheckCount++;
        CollisionComponent col = GetCollision();

        switch (CollisionMode)
        {
            case "AABB":
                {
                    bool result = col.CheckAABB(playerEntityID);
                    if (collisionCheckCount % 4 == 1)
                        Console.WriteLine($"[Enemy] AABB check #{collisionCheckCount}: {result}");
                    return result;
                }

            case "Edge":
                {
                    bool result = col.CheckEdgeContact(playerEntityID, EdgeContactThreshold);
                    if (result)
                        Console.WriteLine("[Enemy] Edge contact detected!");
                    return result;
                }

            case "Distance":
                {
                    bool result = col.CheckDistance(SelfCollisionRadius, playerEntityID, PlayerCollisionRadius);
                    if (collisionCheckCount % 4 == 1)
                        Console.WriteLine($"[Enemy] Distance check #{collisionCheckCount}: {result} (radii: {SelfCollisionRadius}+{PlayerCollisionRadius})");
                    return result;
                }

            case "Manual":
                {
                    TransformComponent myTf = GetTransform();
                    bool result = col.CheckDistance(ManualAttackRange, playerEntityID, 0f);

                    if (collisionCheckCount % 4 == 1)
                        Console.WriteLine($"[Enemy] Manual check #{collisionCheckCount} at X={myTf.X:F1}: {result} (range: {ManualAttackRange})");
                    if (result)
                        Console.WriteLine($"[Enemy] IN RANGE at X={myTf.X:F1}!");
                    return result;
                }

            default:
                Console.WriteLine($"[Enemy] Unknown collision mode: {CollisionMode}");
                return false;
        }
    }

    private bool IsPlayerInAttackRange()
    {
        if (playerFound && playerEntityID >= 0)
            return IsCollidingWithPlayer();

        Console.WriteLine("[Enemy] Cannot check attack range - player entity not found!");
        return false;
    }

    private void TrySetWeakness()
    {
        if (weaknessSet && ComboSystem.Instance != weaknessSetOnInstance)
        {
            weaknessSet = false;
            weaknessSetOnInstance = null;
            weaknessRetryCount = 0;
        }

        if (weaknessSet) return;

        if (ComboSystem.Instance != null)
        {
            ComboSystem.Instance.SetEnemyWeakness(WeaknessCombo);
            weaknessSet = true;
            weaknessSetOnInstance = ComboSystem.Instance;
            Console.WriteLine($"[Enemy] Weakness successfully set to: {WeaknessCombo}");
        }
        else if (weaknessRetryCount == 0)
        {
            Console.WriteLine("[Enemy] ComboSystem.Instance is null - will retry...");
        }
        else if (weaknessRetryCount == maxWeaknessRetries - 1)
        {
            Console.WriteLine("[Enemy] ERROR: ComboSystem never initialized!");
        }
    }

    private void SetState(EnemyState newState)
    {
        if (currentState == newState) return;
        Console.WriteLine($"[Enemy] State: {currentState} -> {newState}");
        currentState = newState;
        UpdateSpriteSheet(newState);
    }

    private void UpdateSpriteSheet(EnemyState state)
    {
        Animation anim = GetAnimation();

        switch (state)
        {
            case EnemyState.Idle:
                anim.SetAnimation(IdleSpriteSheet, IdleRows, IdleColumns, IdleFrames, AnimationSpeed);
                break;
            case EnemyState.Moving:
                anim.SetAnimation(WalkSpriteSheet, WalkRows, WalkColumns, WalkFrames, AnimationSpeed);
                break;
            case EnemyState.Attacking:
                anim.SetAnimation(AttackSpriteSheet, AttackRows, AttackColumns, AttackFrames, AnimationSpeed);
                break;
            case EnemyState.Hurt:
                anim.SetAnimation(HurtSpriteSheet, HurtRows, HurtColumns, HurtFrames, AnimationSpeed);
                break;
            case EnemyState.Dead:
                anim.SetAnimation(DeathSpriteSheet, DeathRows, DeathColumns, DeathFrames, AnimationSpeed);
                break;
        }
    }

    private void HandleBeat(int beat)
    {
        if (TurnManager.Instance == null) return;

        // Don't interrupt hurt animation
        if (currentState == EnemyState.Hurt) return;

        TurnPhase currentPhase = TurnManager.Instance!.GetCurrentPhase();

        if (currentPhase == TurnPhase.Countdown)
        {
            SetState(EnemyState.Idle);
            return;
        }

        if (currentPhase == TurnPhase.EnemyDisplay)
        {
            // If enemy has already reached the player, do NOT move anymore
            if (hasReachedPlayer)
            {
                Console.WriteLine("[Enemy] Already at player - skipping movement");
                return;
            }

            if (movementStepsTaken < maxMovementSteps)
            {
                // Check collision BEFORE stepping - if already in range, don't step further
                if (IsPlayerInAttackRange())
                {
                    Console.WriteLine("[Enemy] Already in range before step - stopping and attacking!");
                    hasReachedPlayer = true;
                    AttackPlayer();
                    return;
                }

                Step();
                movementStepsTaken++;
                Console.WriteLine($"[Enemy] Enemy Display - moved step {movementStepsTaken}/{maxMovementSteps}");

                // Check collision AFTER stepping
                if (IsPlayerInAttackRange())
                {
                    Console.WriteLine("[Enemy] Reached player after step - stopping and attacking!");
                    hasReachedPlayer = true;
                    AttackPlayer();
                    return;
                }
            }
            return;
        }

        if (currentPhase == TurnPhase.PlayerInput)
        {
            SetState(EnemyState.Idle);
            return;
        }
    }

    private void Step()
    {
        SetState(EnemyState.Moving);
        TransformComponent myTf = GetTransform();
        myTf.X -= StepDistance;

        stepSFXToggle = !stepSFXToggle;
        string stepSfx = stepSFXToggle ? "SFX_Rat_Steps_1" : "SFX_Rat_Steps_2";
        GetAudio().PlaySFXAt(stepSfx, myTf.X, myTf.Y, 0f, 800f, 0.25f);

        Console.WriteLine($"[Enemy] Stepped to X={myTf.X:F1}");
    }

    protected virtual void OnAttackStart() { }

    private void AttackPlayer()
    {
        if (isDead || Cheats.invincible)
        {
            return;
        }

        int attackDamage = GetHealth().damage;
        Console.WriteLine($"[Enemy] ATTACKING PLAYER for {attackDamage} damage!");
        SetState(EnemyState.Attacking);
        OnAttackStart();

        attackCooldownTimer = AttackFrames / AnimationSpeed;

        if (PlayerHealth.Instance != null)
        {
            PlayerHealth.Instance.TakeDamage(attackDamage);
            Console.WriteLine("[Enemy] Successfully damaged player");
        }
        else
        {
            Console.WriteLine("[Enemy] WARNING: PlayerHealth.Instance is null!");
        }
    }

    public virtual void TakeDamage(float dmg)
    {
        if (isDead || pendingDeath) return;

        HealthComponent h = GetHealth();
        int prev = h.hp;
        h.hp -= (int)dmg;

        if (h.hp < 0) h.hp = 0;
        Console.WriteLine($"[Enemy] Took {dmg} damage! HP: {h.hp}/{prev}");

        // Capture position for VFX (must be done from this entity's context for correct coords)
        var tf = GetTransform();
        HitVFX.SpawnX = tf.X;
        HitVFX.SpawnY = tf.Y;

        // Spawn damage indicator
        EnemyDamageIndicator.SpawnX = tf.X;
        EnemyDamageIndicator.SpawnY = tf.Y;
        EnemyDamageIndicator.DamageText = "-" + ((int)dmg).ToString();
        // Console.WriteLine($"[DamageIndicator] Spawning at X:{tf.X} Y:{tf.Y}");
        Factory.Instantiate("EnemyDamageIndicator");

        if (h.hp <= 0)
        {
            pendingDeath = true;
            pendingDeathFallbackTimer = 0f;
            // DimSlotsForDeath is intentionally NOT called here — calling it immediately in
            // TakeDamage would wipe the last key's feedback sprite in the same frame it was set.
            // Die() already calls DimSlotsForDeath once the death animation actually starts.
        }
    }

    public int GetCurrentHP() => GetHealth().hp;

    public float GetHealthPercent()
    {
        int hp = GetHealth().hp;
        if (hp < 0) hp = 0;
        return (float)hp / MaxHP;
    }

    public int GetHealthBarBlockCount() => HealthBarBlockCount;
    public float GetHealthBarOffsetX() => HealthBarOffsetX;
    public float GetHealthBarOffsetY() => HealthBarOffsetY;
    public float GetHealthBarScaleOffsetXFactor() => HealthBarScaleOffsetXFactor;
    public float GetHealthBarScaleOffsetYFactor() => HealthBarScaleOffsetYFactor;

    public void TriggerHurtAnimation()
    {
        if (isDead) return;
        // If this hit was the killing blow, skip the hurt animation and go straight to death
        if (pendingDeath)
        {
            Die();
            return;
        }
        SetState(EnemyState.Hurt);
        hurtTimer = HurtFrames / AnimationSpeed;
    }

    private void FinishHurt()
    {
        if (pendingDeath)
            Die();
        else
            SetState(EnemyState.Idle);
    }

    protected virtual void Die()
    {
        if (isDead) return;

        isDead = true;
        HealthComponent hDie = GetHealth();
        hDie.isAlive = false;
        waitingToDie = true;
        deathTimer = 0f;
        deathNotified = false;
        SetState(EnemyState.Dead);

        // Dim player slots instead of hiding — input is locked by Resolution phase,
        // but the slots should stay visible (dimmed) while the death animation plays.
        ComboDisplayManager.Instance?.DimSlotsForDeath();


        Console.WriteLine("[Enemy] DEFEATED!");
        Console.WriteLine("==============================================");
        Console.WriteLine("VICTORY! Enemy defeated!");
        Console.WriteLine("==============================================");
    }

    public void ExecuteFailurePunishment()
    {
        Console.WriteLine("[Enemy] ExecuteFailurePunishment() called");

        // If already at the player, just attack (don't move further)
        if (hasReachedPlayer)
        {
            Console.WriteLine("[Enemy] Already at player - attacking immediately!");
            AttackPlayer();
            return;
        }

        int remainingSteps = maxMovementSteps - movementStepsTaken;
        Console.WriteLine($"[Enemy] Player failed! Moving {remainingSteps} remaining steps");

        for (int i = 0; i < remainingSteps; i++)
        {
            // Check BEFORE stepping
            if (IsPlayerInAttackRange())
            {
                Console.WriteLine($"[Enemy] In range before punishment step {i + 1} - stopping!");
                hasReachedPlayer = true;
                AttackPlayer();
                return;
            }

            Step();

            // Check AFTER stepping
            if (IsPlayerInAttackRange())
            {
                Console.WriteLine($"[Enemy] Reached player during punishment step {i + 1}!");
                hasReachedPlayer = true;
                AttackPlayer();
                return;
            }
        }

        // Final check after all steps
        if (IsPlayerInAttackRange())
        {
            Console.WriteLine("[Enemy] In range after all punishment steps - attacking!");
            hasReachedPlayer = true;
            AttackPlayer();
        }
        else
        {
            Console.WriteLine("[Enemy] Not in range yet after punishment steps - waiting...");
        }
    }

    public virtual void ResetMovementForNewTurn()
    {
        // Don't clobber the death animation state if this enemy is already dying
        if (isDead) return;

        Console.WriteLine("[Enemy] ResetMovementForNewTurn() - resetting step counter");
        movementStepsTaken = 0;

        if (RandomizeWeakness)
        {
            char[] keys = { 'W', 'A', 'S', 'D' };
            for (int i = keys.Length - 1; i > 0; i--)
            {
                int j = _rng.Next(i + 1);
                char tmp = keys[i]; keys[i] = keys[j]; keys[j] = tmp;
            }
            int count = Math.Min(SlotOffsets.Length, keys.Length);
            WeaknessCombo = new string(keys, 0, count);
            Console.WriteLine($"[Enemy] Randomized combo: {WeaknessCombo}");
        }

        if (RandomizeVisualSlots)
        {
            // Two consecutive slots — no gap between keys
            int[][] slotPool = new int[][] { new int[] { 0, 1 }, new int[] { 1, 2 }, new int[] { 2, 3 } };
            VisualSlotIndices = slotPool[_rng.Next(slotPool.Length)];

            // Derive SlotOffsets from visual positions so timing matches what the player sees.
            // Each slot index represents 1 beat (SecondsPerBeat each).
            float slotDur = Conductor.Instance?.SecondsPerBeat ?? 0.5f;
            SlotOffsets = new float[VisualSlotIndices.Length];
            for (int i = 0; i < VisualSlotIndices.Length; i++)
                SlotOffsets[i] = VisualSlotIndices[i] * slotDur;

            Console.WriteLine($"[Enemy] Randomized visual slots: [{string.Join(",", VisualSlotIndices)}], offsets: [{string.Join(",", SlotOffsets)}]");
        }

        if (!weaknessSet)
        {
            weaknessRetryCount = 0;
            TrySetWeakness();
        }

        // wait til the attack animation done then reset the state :P
        if (attackCooldownTimer <= 0f && currentState != EnemyState.Hurt)
        {
            SetState(EnemyState.Idle);
        }
            
    }

    protected virtual void SetupSprites()
    {
        //nth cos rat took the parent spot!??!!??! wtv
    }
}


public class Gorilla: EnemyRhythmController
{
    // Two gap patterns, randomised each turn.
    // Pattern A: [1][X][3][4]  SlotOffsets={0.0, 1.0, 1.5}
    // Pattern B: [1][2][X][4]  SlotOffsets={0.0, 0.5, 1.5}
    public override void ResetMovementForNewTurn()
    {
        base.ResetMovementForNewTurn();

        if (_rng.Next(2) == 0)
        {
            SlotOffsets = new float[] { 0.0f, 1.0f, 1.5f };
            VisualSlotIndices = new int[] { 0, 2, 3 };
            Console.WriteLine("[Gorilla] Pattern A: [1][X][3][4]");
        }
        else
        {
            SlotOffsets = new float[] { 0.0f, 0.5f, 1.5f };
            VisualSlotIndices = new int[] { 0, 1, 3 };
            Console.WriteLine("[Gorilla] Pattern B: [1][2][X][4]");
        }

        // Re-apply weakness so ComboSystem recalculates minTimeBetweenPresses from new SlotOffsets
        ComboSystem.Instance?.SetEnemyWeakness(WeaknessCombo);
    }

    protected override void SetupSprites()
    {
        IdleSpriteSheet = "gorilla_idle";
        WalkSpriteSheet = "gorilla_walk";
        AttackSpriteSheet = "gorilla_attack";
        HurtSpriteSheet = "gorilla_hurt";
        WeaknessCombo = "WAS";

        IdleFrames = 15;
        IdleColumns = 4;
        IdleRows = 4;
        
        WalkFrames = 30;
        WalkColumns = 6;
        WalkRows = 5;
        
        AttackFrames = 30;
        AttackColumns = 6;
        AttackRows = 5;
        
        HurtFrames = 15;
        HurtColumns = 4;
        HurtRows = 4;

        DeathSpriteSheet = "gorilla_death";
        DeathFrames = 15;
        DeathColumns = 4;
        DeathRows = 4;

        AnimationSpeed = 12f;
        SizeMultiplier = 5.0f;

        // first key, empty beat, then two close keys
        SlotOffsets = new float[] { 0.0f, 1.0f, 1.5f };
        VisualSlotIndices = new int[] { 0, 2, 3 }; // slot 1 intentionally empty to show the beat gap
        RandomizeWeakness = false;
        RandomizeVisualSlots = false;

        HealthBarOffsetX = 0f;
        HealthBarScaleOffsetXFactor = -0.24f;
        HealthBarOffsetY = -110f;
        HealthBarScaleOffsetYFactor = 0.76f;
    }
}

public class BlueGorilla : EnemyRhythmController
{
    // Same two gap patterns as Gorilla but with different fixed keys (DWS).
    public override void ResetMovementForNewTurn()
    {
        base.ResetMovementForNewTurn();

        if (_rng.Next(2) == 0)
        {
            SlotOffsets = new float[] { 0.0f, 1.0f, 1.5f };
            VisualSlotIndices = new int[] { 0, 2, 3 };
            Console.WriteLine("[BlueGorilla] Pattern A: [1][X][3][4]");
        }
        else
        {
            SlotOffsets = new float[] { 0.0f, 0.5f, 1.5f };
            VisualSlotIndices = new int[] { 0, 1, 3 };
            Console.WriteLine("[BlueGorilla] Pattern B: [1][2][X][4]");
        }

        // Re-apply weakness so ComboSystem recalculates minTimeBetweenPresses from new SlotOffsets
        ComboSystem.Instance?.SetEnemyWeakness(WeaknessCombo);
    }

    protected override void SetupSprites()
    {
        // Uses gorilla sprites until artist delivers blue gorilla sheets
        IdleSpriteSheet = "gorilla_idle";
        WalkSpriteSheet = "gorilla_walk";
        AttackSpriteSheet = "gorilla_attack";
        HurtSpriteSheet = "gorilla_hurt";
        WeaknessCombo = "DWS";  // same 3-key length, different keys from Gorilla's WAS

        IdleFrames = 15;
        IdleColumns = 4;
        IdleRows = 4;

        WalkFrames = 30;
        WalkColumns = 6;
        WalkRows = 5;

        AttackFrames = 30;
        AttackColumns = 6;
        AttackRows = 5;

        HurtFrames = 15;
        HurtColumns = 4;
        HurtRows = 4;

        DeathSpriteSheet = "gorilla_death";
        DeathFrames = 15;
        DeathColumns = 4;
        DeathRows = 4;

        AnimationSpeed = 12f;
        SizeMultiplier = 5.0f;

        // same rhythm pattern as Gorilla: first key, empty beat, then two close keys
        SlotOffsets = new float[] { 0.0f, 1.0f, 1.5f };
        VisualSlotIndices = new int[] { 0, 2, 3 }; // slot 1 intentionally empty to show the beat gap
        RandomizeWeakness = false;
        RandomizeVisualSlots = false;

        HealthBarOffsetX = 0f;
        HealthBarScaleOffsetXFactor = -0.24f;
        HealthBarOffsetY = -110f;
        HealthBarScaleOffsetYFactor = 0.76f;
    }
}

public class Bat: EnemyRhythmController
{
    protected override void SetupSprites()
    {
        IdleSpriteSheet = "bat_idle";
        WalkSpriteSheet = "bat_walk";
        AttackSpriteSheet = "bat_attack";
        HurtSpriteSheet = "bat_hurt";
        WeaknessCombo = "SAWD";

        IdleFrames = 15;
        IdleColumns = 4;
        IdleRows = 4;

        WalkFrames = 15;
        WalkColumns = 4;
        WalkRows = 4;

        AttackFrames = 15;
        AttackColumns = 4;
        AttackRows = 4;

        HurtFrames = 15;
        HurtColumns = 4;
        HurtRows = 4;

        DeathSpriteSheet = "bat_death";
        DeathFrames = 15;
        DeathColumns = 4;
        DeathRows = 4;

        AnimationSpeed = 12f;
        SizeMultiplier = 2.0f;

        // syncopated: quick pair, pause, quick pair
        SlotOffsets = new float[] { 0.0f, 0.25f, 1.0f, 1.25f };
        RandomizeWeakness = false;
        RandomizeVisualSlots = false;

        HealthBarOffsetX = 40f;
    }

    protected override void OnAttackStart()
    {
        var tf = GetTransform();
        SwordSlashVFX.SpawnX = tf.X - 600f;  // in front of bat (toward player)
        SwordSlashVFX.SpawnY = tf.Y;
        SwordSlashVFX.SpawnDuration = AttackFrames / AnimationSpeed;
        Factory.Instantiate("BatAttackVFX");
    }
}
