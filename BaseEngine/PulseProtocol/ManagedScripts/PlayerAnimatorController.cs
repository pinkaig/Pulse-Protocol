using System;
using ScriptAPI;

public enum PlayerAnimState { /*Idle1,*/ Idle2, Attack, Hurt }

public class PlayerAnimationController : Script
{
    // =========================
    // Singleton
    // =========================
    public static PlayerAnimationController? Instance { get; private set; }

    // =========================
    // Sprite Sheet Paths
    // =========================
    // public string Idle1SpriteSheet = "../Characters/Dorian_Idle1_579x615";
    public string Idle2SpriteSheet = "dorian_idle2";
    public string AttackSpriteSheet = "dorian_sword";
    public string HurtSpriteSheet = "dorian_hurt";

    // =========================
    // Sprite Sheet Dimensions
    // =========================
    // public int Idle1Rows = 4;
    // public int Idle1Columns = 5;
    // public int Idle1Frames = 18;

    public int Idle2Rows = 4;
    public int Idle2Columns = 4;
    public int Idle2Frames = 15;

    public int AttackRows = 4;
    public int AttackColumns = 5;
    public int AttackFrames = 18;

    public int HurtRows = 4;
    public int HurtColumns = 4;
    public int HurtFrames = 15;

    public float AnimationSpeed = 70f;

    // =========================
    // Internal State
    // =========================
    private bool initialized = false;
    private PlayerAnimState currentState = PlayerAnimState.Idle2;
    private bool swingToggle = false;

    // Boss-specific attack cycling: 0=sword, 1=gun, 2=spear
    private static int  _bossAttackCycle = 0;
    private static int  _prevBossPhase   = 0;
    private static int  _spearSFXIndex   = 0;
    private bool hurtToggle = false;

    // Determines which idle to return to after Attack/Hurt finishes
    // false = Idle2 (default, unarmed stance)
    // true  = Idle1 (armed stance, after attacking)
    // private bool hasAttacked = false;

    // Timer for non-idle states to auto-return to idle
    private float stateTimer = 0f;
    private float stateDuration = 0f;
    private bool waitingForStateEnd = false;
    private bool animPaused = false;

    // Delayed hit VFX
    private bool  pendingHitVFX   = false;
    private float hitVFXTimer     = 0f;

    public override void Update()
    {
        if (!initialized)
        {
            Initialize();
            initialized = true;
            return;
        }

        if (Application.IsPaused())
            return;

        // Freeze animations during countdown; resume when GO! fires
        if (!GameStart.Ready)
        {
            if (!animPaused)
            {
                animPaused = true;
                Animation anim = GetAnimation();
                // if (currentState == PlayerAnimState.Idle1)
                //     anim.SetAnimation(Idle1SpriteSheet, Idle1Rows, Idle1Columns, Idle1Frames, 0f);
                // else
                    anim.SetAnimation(Idle2SpriteSheet, Idle2Rows, Idle2Columns, Idle2Frames, 0f);
            }
            return;
        }
        if (animPaused)
        {
            animPaused = false;
            currentState = PlayerAnimState.Attack; // bypass same-state guard in SetAnimState
            GoToIdle();
        }

        // Handle timed states (Attack, Hurt) returning to Idle
        if (waitingForStateEnd)
        {
            stateTimer += Time.DeltaTime;
            if (stateTimer >= stateDuration)
            {
                waitingForStateEnd = false;
                Console.WriteLine($"[PlayerAnim] {currentState} finished -> returning to idle");
                GoToIdle();
            }
        }

        // Spawn hit VFX on the enemy after attack animation finishes
        if (pendingHitVFX)
        {
            hitVFXTimer -= Time.DeltaTime;
            if (hitVFXTimer <= 0f)
            {
                pendingHitVFX = false;
                if (EnemyRhythmController.Instance != null)
                {
                    Factory.Instantiate("HitVFX");
                    EnemyRhythmController.Instance.TriggerHurtAnimation();
                }
            }
        }
    }

    private void Initialize()
    {

        Instance = this;

        Console.WriteLine("[PlayerAnim] Initializing...");
        Console.WriteLine($"[PlayerAnim] Entity ID: {entityID}");

        // Start in Idle2 (default unarmed stance)
        // hasAttacked = false;
        animPaused = false;
        SetAnimState(PlayerAnimState.Idle2);

        Console.WriteLine("[PlayerAnim] Initialized - Idle2 (unarmed)");
    }

    // =========================
    // Idle Selection Logic
    // =========================
    // Idle1 = armed stance (sword drawn after first attack)
    // Idle2 = default / unarmed stance
    // Taking damage reverts to Idle2 (dropped guard)
    private void GoToIdle()
    {
        SetAnimState(PlayerAnimState.Idle2);
        Console.WriteLine("[PlayerAnim] -> Idle2 (unarmed stance)");
    }

    // =========================
    // Public API
    // =========================

    public void PlayAttack(string combo) // param is the enemies weakness combo
    {
        // ── Boss-fight path: cycle sword -> gun -> spear ──────────────────────
        int currentBossPhase = BossEnemyController.BossPhase;

        // Reset the cycle whenever a new boss fight starts (phase goes 0 → 1)
        if (currentBossPhase > 0 && _prevBossPhase == 0)
            _bossAttackCycle = 0;
        _prevBossPhase = currentBossPhase;

        if (currentBossPhase > 0)
        {
            switch (_bossAttackCycle % 3)
            {
                case 0: // sword
                    AttackSpriteSheet = "dorian_sword";
                    AttackRows = 4; AttackColumns = 5; AttackFrames = 18;
                    break;
                case 1: // gun
                    AttackSpriteSheet = "dorian_gun";
                    AttackRows = 5; AttackColumns = 6; AttackFrames = 29;
                    break;
                case 2: // spear
                    AttackSpriteSheet = "dorian_spear";
                    AttackRows = 4; AttackColumns = 5; AttackFrames = 18;
                    break;
            }
        }
        else
        {
            // ── Normal enemies ────────────────────────────────────────────────
            switch (combo.Length)
            {
                case 3:
                    AttackSpriteSheet = "dorian_spear"; //gorilla
                    AttackRows = 4; AttackColumns = 5; AttackFrames = 18;
                    break;
                case 4:
                    if (EnemySpawner.enemyQueue.Peek() == EnemyIconType.Rat)
                    {
                        AttackSpriteSheet = "dorian_sword"; //rat
                        AttackRows = 4; AttackColumns = 5; AttackFrames = 18;
                    }
                    else if (EnemySpawner.enemyQueue.Peek() == EnemyIconType.Bat)
                    {
                        AttackSpriteSheet = "dorian_gun";   //bat
                        AttackRows = 5; AttackColumns = 6; AttackFrames = 29;
                    }
                    break;
                default:
                    AttackSpriteSheet = "dorian_sword";
                    AttackRows = 4; AttackColumns = 5; AttackFrames = 18;
                    break;
            }
        }

        // Don't interrupt an ongoing attack
        if (currentState == PlayerAnimState.Attack && waitingForStateEnd)
        {
            Console.WriteLine("[PlayerAnim] Already attacking - ignoring");
            return;
        }

        Console.WriteLine("[PlayerAnim] Playing Attack");
        // hasAttacked = true; // From now on, idle = Idle2
        if (AttackSpriteSheet == "dorian_gun")
        {
            GetAudio().PlaySFX("SFX_Gun_1");
        }
        else if (AttackSpriteSheet == "dorian_spear")
        {
            string spearSFX = "SFX_Spear_" + (_spearSFXIndex + 1);
            _spearSFXIndex = (_spearSFXIndex + 1) % 4;
            GetAudio().PlaySFX(spearSFX);
        }
        else
        {
            string swingSFX = swingToggle ? "SFX_SwordSwing_2" : "SFX_SwordSwing_1";
            swingToggle = !swingToggle;
            GetAudio().PlaySFX(swingSFX);
        }
        SetAnimState(PlayerAnimState.Attack);

        waitingForStateEnd = true;
        stateTimer = 0f;

        if (AttackSpriteSheet == "dorian_gun")
        {
            float gunSpeed = GetBeatSyncedSpeed(18);
            GetAnimation().SetAnimation(AttackSpriteSheet, AttackRows, AttackColumns, AttackFrames, gunSpeed);
            stateDuration = AttackFrames / gunSpeed;
        }
        else
        {
            stateDuration = AttackFrames / GetBeatSyncedSpeed(AttackFrames);
        }

        // Queue hit VFX to spawn on enemy after this attack animation finishes
        pendingHitVFX = true;
        hitVFXTimer   = stateDuration;

        // Spawn attack VFX
        SwordSlashVFX.SpawnX = GetTransform().X + 300f;
        SwordSlashVFX.SpawnY = GetTransform().Y;
        SwordSlashVFX.SpawnDuration = stateDuration;

        if (currentBossPhase > 0)
        {
            // VFX matches the current cycle step, then advance the counter
            switch (_bossAttackCycle % 3)
            {
                case 0:
                    SwordSlashVFX.BossVFXOverride = "sword";
                    Factory.Instantiate("SwordSlashVFX");
                    break;
                case 1:
                    Factory.Instantiate("GunAttackVFX");
                    break;
                case 2:
                    SwordSlashVFX.BossVFXOverride = "spear";
                    Factory.Instantiate("SpearAttackVFX");
                    break;
            }
            _bossAttackCycle++;
        }
        else
        {
            switch (EnemySpawner.enemyQueue.Count > 0 ? EnemySpawner.enemyQueue.Peek() : EnemyIconType.None)
            {
                case EnemyIconType.BlueGorilla:
                    Factory.Instantiate("SpearAttackVFX");
                    break;
                case EnemyIconType.Bat:
                    Factory.Instantiate("GunAttackVFX");
                    break;
                default: // Rat and anything else
                    Factory.Instantiate("SwordSlashVFX");
                    break;
            }
        }
    }

    public void PlayHurt()
    {
        Console.WriteLine("[PlayerAnim] Playing Hurt");
        // hasAttacked = false; // Revert to Idle1 after hurt
        string hurtSFX = hurtToggle ? "SFX_Player_Hurt_2" : "SFX_Player_Hurt_1";
        hurtToggle = !hurtToggle;
        GetAudio().PlaySFX(hurtSFX);
        SetAnimState(PlayerAnimState.Hurt);

        waitingForStateEnd = true;
        stateTimer = 0f;
        stateDuration = HurtFrames / GetBeatSyncedSpeed(HurtFrames);
    }


    public void ForceIdle()
    {
        waitingForStateEnd = false;
        GoToIdle();
    }

    public PlayerAnimState GetCurrentState()
    {
        return currentState;
    }


    private float GetBeatSyncedSpeed(int frameCount)
    {
        if (Conductor.Instance == null || Conductor.Instance.SecondsPerBeat <= 0f)
            return AnimationSpeed;
        return frameCount / Conductor.Instance.SecondsPerBeat * 1.1065f;
    }

    // =========================
    // Internal: Set Animation
    // =========================
    private void SetAnimState(PlayerAnimState newState)
    {
        if (currentState == newState && !waitingForStateEnd)
            return;

        Console.WriteLine($"[PlayerAnim] State: {currentState} -> {newState}");
        currentState = newState;

        Animation anim = GetAnimation();

        switch (newState)
        {
            // case PlayerAnimState.Idle1:
            //     anim.SetAnimation(Idle1SpriteSheet, Idle1Rows, Idle1Columns, Idle1Frames, AnimationSpeed);
            //     break;

            case PlayerAnimState.Idle2:
                anim.SetAnimation(Idle2SpriteSheet, Idle2Rows, Idle2Columns, Idle2Frames, GetBeatSyncedSpeed(Idle2Frames));
                break;

            case PlayerAnimState.Attack:
                anim.SetAnimation(AttackSpriteSheet, AttackRows, AttackColumns, AttackFrames, GetBeatSyncedSpeed(AttackFrames));
                break;

            case PlayerAnimState.Hurt:
                anim.SetAnimation(HurtSpriteSheet, HurtRows, HurtColumns, HurtFrames, GetBeatSyncedSpeed(HurtFrames));
                break;
        }
    }
}
