## ACTIVE PRODUCTION SCRIPTS:

|Script|What It Does|
|-|-|
|AudioSliderCore.cs|Abstract base classes for draggable audio slider knobs and fill bars|
|AudioSliderWrappers.cs|Implemented slider types: VolumeKnob/Fill, MusicKnob/Fill, SFXKnob/Fill, BrightnessKnob/Fill|
|BGMMgr.cs|LevelBGM starts level and audio and resets the Conductor clock. GeneralBGM plays menu music. LevelSelectBGM fades in/out level preview BGM while the level-select overlay is open|
|BossEnemyController.cs|Two-phase boss: Phase 1 picks random combos from the Rat/Gorilla/BlueGorilla/Bat pool; Phase 2 generates random 2/3/4/6/8-key combos with crotchet or quaver timing and a scrolling 4-slot window. Phase transition seamlessly restores HP instead of dying.|
|ComboDisplayManager.cs|Central hub for combo visual feedback: coordinates ComboKeySlot.cs and ComboJudgmentDisplay.cs.|
|ComboJudgementDisplay.cs|Shows PERFECT/GREAT/MISS sprites for a short duration|
|ComboKeySlot.cs|Individual WASD key sprite slot: Shows/Hides with timer on each input|
|ComboSystem.cs|Registers W/A/S/D input, judges PERFECT/GREAT/MISS timing, evaluates combo against enemy weakness|
|Conductor.cs|FOUNDATIONAL BEAT CLOCK: Accumulates songTime, fires beat/half-beat events and exposes GetCurrentBeat() function for ALL SCRIPTS that require syncing.|
|CountdownUI.cs|Shows 3,2,1,GO! tint pulses during TurnPhase.Countdown by reacting to beats from Conductor.cs|
|CutscenePanel.cs|Staggered pair slide in cutscene: Each panel currently handles its own animation, with a shared timer via statics|
|DebugDisplayScript.cs|In-game FPS + top profiling system overlay text|
|EnemyRhythmController.cs|Enemy AI - steps toward player on beats, attacks on contact, holds WeaknessCombo \& handles death|
|EnemySpawner.cs|Spawns enemies from prefab one at a time, manages enemy queue and notifies OrderSlipUIController.cs|
|HealthBarController.cs|Drives health bar sprite frames (tile001-tile012) based on PlayerHealth.GetHealthPercent()|
|HealthBGController.cs|Background UI deco behind health bar; flashes red on damage.|
|HoverableButton.cs|ALL UI buttons: Hover effects, scene navigation, pause/quit/settings overlays (NavigationButtons, QuitOverlay, PauseOverlay, HowToPlayOverlay,SettingsOverlay)|
|InputBridge.cs|Polls GLFW input each frame, listens to TurnManager.cs function:  TurnManager.IsInputLocked, which locks the player input when it's enemy's turn.  Forwards keys to ComboSystem.cs and plays key SFX|
|OrderSlipCurrentIcon.cs|Renders the current enemy icon slot, fades out during transition|
|OrderSlipNextIcon.cs|Renders the next enemy icon slot, animates sliding up with arc to become current|
|OrderSlipUIController.cs|"Order Slip" enemy queue display: Manages Current/Next icon types and animated transitions|
|PauseAnimatorBackground.cs|Ping-pong animates the pause screen background with freeze/flicker|
|PauseLabels.cs|Single-liner that shows/hides pause text labels by mirroring ShowPauseOverlay.|
|PlayerAnimatorController.cs|Player animation FSM: Idle1 (armed) / Idle2 (unarmed) / Attack / Hurt|
|PlayerHealth.cs|Player HP. Damage checker function is here too: TakeDamage(), death (pauses game), triggers hurt animation and glow flash|
|ScreenModeDropdown.cs|Fullscreen/Windowed toggle dropdown (DropdownToggle, DropdownOpenFollower, DropdownOptionFullscreen, DropdownOptionWindowed, DropdownClosedLabelFullscreen/Windowed)|
|StageClear.cs|StageClearElement: Shows stage clear UI when StageClear() is called globally|
|TurnManager.cs|Drives turn phases: Countdown -> EnemyDisplay -> PlayerInput -> Resolution|
|VisualBeatSystem.cs|Pulses border's scale and opacity on each beat using Conductor.cs song time|

## 

## SUPERSEDED SCRIPTS (kept in repo but no longer wired up — safe to delete):

|Script|What It Does|Superseded By|
|-|-|-|
|GameStateManager.cs|Defines GameState enum (Tutorial/Countdown/Playing/Win/Lose/Paused) and a countdown timer — not called by any other script|TurnManager.cs handles phases; NavigationButtons handles win/lose scene transitions|
|TextSample.cs|Standalone 3-2-1-GO! countdown demo|CountdownUI.cs|

## 

## DEBUG SCRIPTS:

|Script|What It Does|
|-|-|
|API\_Guide.cs|Reference for ScriptAPI|
|ConductorBeatTester.cs|Tests beat timing with console output|
|DamageTestScript.cs|Press T to deal damage to the player (tests PlayerHealth)|
|DebugEnemyCheats.cs|Press K to instantly kill the current enemy (9999 DMG)|
|FadeTestScript.cs|Tests fade in/out effects|
|FSM.cs|TestFSM: Tests FSM API (Space = attack, W = move)|
|InitializationDiagnostic.cs|Checks all singletons (Conductor, TurnManager, ComboSystem, Enemy) exists during first 10 frames|
|MasterDiagnostic.cs|Prints a full system health report to console every 60 frames|
|PlayerAnimTest.cs|Keys 1/2/3/4 to manually trigger player animation states and test auto-cycle|
|TestScript.cs|Original engine integration test (transform, factory, collision, health, FSM, VFX): TestScript, TestScript2, TestScript3, TestPause classes|
|TestStageClear.cs|Press P for stage clear, 0 to load GameLose scene|



