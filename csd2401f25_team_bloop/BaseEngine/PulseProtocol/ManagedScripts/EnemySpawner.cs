using System;
using System.Collections.Generic;
using ScriptAPI;

public class EnemySpawner : Script
{
    // =========================
    // Singleton
    // =========================
    public static EnemySpawner? Instance { get; private set; }

    // =========================
    // Spawn Settings
    // =========================
    public string EnemyPrefabName = "Rat_Enemy";
    public float SpawnX = 800f;
    public float SpawnY = 350f;

    public int EnemiesToSpawn = 3;
    public float SpawnInterval = 0.5f; // keep small for testing

    public bool SpawnImmediatelyOnStart = true;

    // =========================
    // UI / Queue
    // =========================
    // I made this public static so i can use them in playeranimationcontroller.cs for different hero attack animations
    public static Queue<EnemyIconType> enemyQueue = new Queue<EnemyIconType>(); // public enum EnemyIconType { None, Rat, Bat, Gorilla }
    public static int enemyQueueID = 0;
    private bool uiInitialized = false;
    private int framesSinceStart = 0;
    private const int WAIT_FRAMES = 5;

    // =========================
    // State
    // =========================
    private bool initialized = false;
    private int enemiesSpawned = 0;
    private int enemiesDefeated = 0;
    private float timeSinceLastSpawn = 0f;
    private bool waveComplete = false;
    private bool pendingSpawn = false;
    private bool EnemySpawnedAlr = false; // Stop spawning as soon as u spawn an enemy

    //Container of enemy 2 spawn. 
    // private List<EnemyIconType> EnemyContainer = new List<EnemyIconType>
    // {
    //     EnemyIconType.Rat,
    //     EnemyIconType.Bat,
    //     EnemyIconType.Gorilla,
    //     EnemyIconType.Rat
    // };

    public string CurrentScene = "";
    // Level 1: Teach randomised 4-beat combos (rat), then introduce the beat-gap pattern (gorilla).
    // 4 enemies keeps the level focused without outstaying its welcome.
    private List<EnemyIconType> EnemyContainer1 = new List<EnemyIconType>
    {
        EnemyIconType.Rat,           // learn the randomised 4-beat rhythm
        EnemyIconType.Rat,           // reinforce it
        EnemyIconType.BlueGorilla,   // introduce the gap (3-key, fixed pattern)
        EnemyIconType.Rat,           // familiar closer
    };

    // Level 2: Reintroduce familiar enemies, then layer in the bat's syncopated quaver rhythm.
    // 5 enemies — gradual build, two bat encounters so the new pattern feels earned not cheap.
    private List<EnemyIconType> EnemyContainer2 = new List<EnemyIconType>
    {
        EnemyIconType.Rat,           // warm-up
        EnemyIconType.BlueGorilla,   // gap pattern refresher
        EnemyIconType.Bat,           // introduce quaver syncopation
        EnemyIconType.Rat,           // breather after hard timing
        EnemyIconType.Bat,           // end strong on the new mechanic
    };

    private List<EnemyIconType> EnemyContainer3 = new List<EnemyIconType>
    {
        EnemyIconType.Rat,
        EnemyIconType.Bat,
        EnemyIconType.BlueGorilla,
        EnemyIconType.Bat,
        EnemyIconType.BlueGorilla,
        EnemyIconType.Rat,
        EnemyIconType.Boss
    };

    public override void Update()
    {
        if (!initialized)
        {
            Initialize();
            initialized = true;
            return;
        }

        if (StageClearElement.IsCleared)
            return;

        // UI init after a few frames
        framesSinceStart++;
        if (!uiInitialized && framesSinceStart >= WAIT_FRAMES)
        {
            if (OrderSlipUIController.Instance != null)
            {
                if (OrderSlipUIController.Instance.SlotsReady()) // WAIT UR TURN
                {
                    InitializeOrderSlipUI();
                    uiInitialized = true;
                }
            }
        }

        if (waveComplete) return;

        // Stop when we’re done
        if (enemiesSpawned >= EnemiesToSpawn)
        {
            waveComplete = true;
            Console.WriteLine($"[EnemySpawner] Wave complete! Spawned {enemiesSpawned} enemies");
            return;
        }

        // This ensures that it will spawn in only one enemy at a time
        if (EnemySpawnedAlr)
        {
            // Wait until the new enemy finish setup of its EnemyRhythmController script
            if (EnemyRhythmController.Instance != null)
            {
                EnemySpawnedAlr = false;
                Console.WriteLine("New enemy spawned!!!\n\n\n");
            }
            return;
        }

        if (EnemyRhythmController.Instance != null)
        {
            return;
        }

        timeSinceLastSpawn += Time.DeltaTime;

        bool intervalReady = (timeSinceLastSpawn >= SpawnInterval);
        if (pendingSpawn || intervalReady)
        {
            SpawnEnemy();
        
            pendingSpawn = false;
            timeSinceLastSpawn = 0f;
        }
    }

    private void Initialize()
    {
        if (Instance != null && Instance != this)
            Console.WriteLine("[EnemySpawner] WARNING: Multiple spawners detected - replacing Instance");

        Instance = this;
        StageClearElement.ResetStageClear();

        // WHY IS ALL THIS HARD CODED WHEN U HAVE A CONTAINER ALR??????
        Console.WriteLine("[EnemySpawner] Initialized");
        Console.WriteLine($"  Will spawn {EnemiesToSpawn} enemies");
        Console.WriteLine($"  Prefab: {EnemyPrefabName}");
        Console.WriteLine($"  Spawn position: ({SpawnX}, {SpawnY})");

        CurrentScene = Scene.GetCurrentScene();
        SetupEnemyQueue();

        pendingSpawn = SpawnImmediatelyOnStart;
    }

    private void SetupEnemyQueue()
    {
        enemyQueue.Clear();

        //// Ensure Current + Next always exist for UI
        //int totalIcons = Math.Max(EnemiesToSpawn + 2, 2);

        ////HARD CODED AGAIN????????????????????????
        //for (int i = 0; i < totalIcons; i++)
        //    enemyQueue.Enqueue(EnemyIconType.Rat);

        // Add enemy container for each lvl here VVV
        if (CurrentScene == "Level1")
        {
            // use my container for the enemyQueue UI instead,cos the queue is a fking enum of int when factory takes in string
            foreach (EnemyIconType type in EnemyContainer1)
            {
                enemyQueue.Enqueue(type);
            }
        }

        else if (CurrentScene == "Level2")
        {
            foreach (EnemyIconType type in EnemyContainer2)
            {
                enemyQueue.Enqueue(type);
            }
        }

        else if (CurrentScene == "Level3")
        {
            foreach (EnemyIconType type in EnemyContainer3)
            {
                enemyQueue.Enqueue(type);
            }
        }

        EnemiesToSpawn = enemyQueue.Count();

        Console.WriteLine($"[EnemySpawner] Enemy queue setup with {enemyQueue.Count} enemies");
    }

    private void InitializeOrderSlipUI()
    {
        var ui = OrderSlipUIController.Instance;
        if (ui == null) return;

        if (enemyQueue.Count < 2) return;

        EnemyIconType current = PeekAtIndex(0);
        EnemyIconType next = PeekAtIndex(1);

        try
        {
            ui.Init(current, next);
            Console.WriteLine($"[EnemySpawner] UI init -> Current={current}, Next={next}");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[EnemySpawner] ERROR during UI Init: {ex.Message}");
        }
    }

    private void SpawnEnemy()
    {
        // all enemies are dead. Yippeee
        if (enemyQueue.Count == 0)
        {
            //Scene.LoadScene("GameWin");
            StageClearElement.StageClear(); // bring up winner overlay
            return;
        }

        string FactoryString = EnumtoString(enemyQueue.Peek());

        // if it returns ""
        if (string.IsNullOrEmpty(FactoryString))
        {
            return;
        }

        int enemyID = Factory.Instantiate(FactoryString);

        if (enemyID < 0)
        {
            Console.WriteLine($"[EnemySpawner] Failed to spawn enemy! Prefab '{FactoryString}' not found");
            return;
        }

        EnemySpawnedAlr = true;
        enemiesSpawned++;
        Console.WriteLine($"[EnemySpawner] Spawned enemy {enemiesSpawned}/{EnemiesToSpawn} (ID: {enemyID})");
    }

    public string EnumtoString(EnemyIconType type)
    {
        switch (type)
        {
            case EnemyIconType.Rat: return "Rat_Enemy";
            case EnemyIconType.Bat: return "Bat_Enemy";
            case EnemyIconType.BlueGorilla: return "BlueGorilla_Enemy";
            case EnemyIconType.Boss: return "Boss_Enemy";
            default: return "";
        }
    }

    public void OnEnemyDefeated()
    {
        enemiesDefeated++;
        Console.WriteLine($"[EnemySpawner] Enemy defeated! ({enemiesDefeated} total)");

        // Shift queue (remove current)
        if (enemyQueue.Count > 0)
            enemyQueue.Dequeue();

        // The newUpcoming is what should appear as "Next" after shifting
        EnemyIconType newUpcoming = EnemyIconType.None;
        if (enemyQueue.Count > 1)
            newUpcoming = PeekAtIndex(1);

        var ui = OrderSlipUIController.Instance;
        if (ui != null)
        {
            try
            {
                ui.AdvanceQueue(newUpcoming);
                Console.WriteLine($"[EnemySpawner] UI advanced -> Upcoming={newUpcoming}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[EnemySpawner] ERROR during AdvanceQueue: {ex.Message}");
            }
        }

        // Request the next enemy spawn (next Update, safe)
        pendingSpawn = true;
    }

    private EnemyIconType PeekAtIndex(int index)
    {
        if (index >= enemyQueue.Count)
            return EnemyIconType.None;

        EnemyIconType[] temp = enemyQueue.ToArray();
        return temp[index];
    }
}
