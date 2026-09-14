using System;
using ScriptAPI;

public enum GameState
{
    Tutorial,
    Countdown,
    Playing,
    Win,
    Lose,
    Paused
}

public class GameStateManager : Script
{
    //create the instance
    public static GameStateManager? Instance { get; private set; }
    private bool _init = false;
    private void EnsureInit()
    {
        if (_init) return;
        _init = true;
        if (Instance == null) Instance = this;
    }

    // =========================
    // State
    // =========================
    private GameState currentState = GameState.Paused;

    private float countdownTimer = 3.0f;
    private bool countingDown = true;

    // =========================
    // Public API
    // =========================
    public GameState GetState()
    {
        return currentState;
    }

    public void SetState(GameState newState)
    {
        currentState = newState;

        switch (newState)
        {
            case GameState.Win:
                Console.WriteLine("🏆 Win!");
                break;

            case GameState.Lose:
                Console.WriteLine("💀 Lose!");
                break;

            case GameState.Paused:
                Console.WriteLine("⏸ Paused!");
                break;

            case GameState.Playing:
                Console.WriteLine("▶ Playing");
                break;
        }
    }

    // =========================
    // Countdown Logic
    // =========================
    public void StartResumeCountdown(float seconds = 3.0f)
    {
        countdownTimer = seconds;
        countingDown = true;
        currentState = GameState.Countdown;
    }

    // =========================
    // Update Loop
    // =========================

    public override void Update()
    {


        float time = Time.GameTime;
        Console.WriteLine("Time is : " + time);

        EnsureInit();
        float dt = Time.DeltaTime;

        if (countingDown)
        {
            countdownTimer -= dt;
            Console.WriteLine("GET STATE " + GetState());
            Console.WriteLine("COUNTING DOWN" + countdownTimer);
            if (countdownTimer <= 0.0f)
            {
                countingDown = false;
                SetState(GameState.Playing);
            }
        }
    }

    // =========================
    // Scene Control
    // =========================
    public void RestartLevel(string sceneName)
    {
        Scene.LoadScene(sceneName);
    }

    public void QuitToMenu()
    {
        Scene.LoadScene("MainMenu");
    }
}
