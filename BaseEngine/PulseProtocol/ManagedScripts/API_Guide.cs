using System;
using System.Runtime.CompilerServices;
using ScriptAPI;

//How to make a script
public class ScriptName : Script
{
    // An update function & if you want, helper functions 
    public override void Update()
    {
        //Firstly, to get get a component:
        Animation anim = GetAnimation();
        SpriteComponent s = GetSprite();
        InputComponent input = GetInput();
        AudioComponent audio = GetAudio();
        HealthComponent health = GetHealth();
        CollisionComponent c = GetCollision();
        TransformComponent t = GetTransform();
        var text = GetTextComponent();


        // 1. TransformComponent:
        t.X = 0f;                   // world position X
        t.Y = 0f;                   // world position Y
        t.RotX = 0f;                // rotation X
        t.RotY = 0f;                // rotation Y
        t.ScaleX = 0f;              // width
        t.ScaleY = 0f;              // height
        t.IsVisible = true;         // show/hide entity


        health.hp = 5;              // returns int, get/set current HP
        health.damage += 1;         // returns int, get/set damage value
        health.isAlive = false;     // returns bool, get/set alive state

        // 2. Scene API
        Scene.LoadScene("SceneName");                       // load scene,name must match JSON!!!
        string SceneName = Scene.GetCurrentScene();         // string of current scene name

        // 3. Factory API
        int id = Factory.Instantiate("PrefabName");         // spawn from prefab JSON, returns entity ID
        Factory.Destroy(id);                                // destroy an entity, KILL OTHER ID, not urself. That one call health API

        // 4. Collision API
        // Note: currently there is no check all entity with collision component,but since 
        // we only have 1 enemy, should be quite easy to just check with 1 enemy.
        int otherEntityID = Factory.Instantiate("PrefabName");
        c.CheckAABB(otherEntityID);                                              // returns bool, check CURRENT entity with another entity
        float leeway =5.0f;
        c.CheckEdgeContact(otherEntityID, leeway);                               // return bool, edge contact with some leeway
        float CurrentEntityRadius = 5, OtherEntityRadius = 5;
        c.CheckDistance(CurrentEntityRadius, otherEntityID, OtherEntityRadius);  // bool - circle vs circle

        // 5. Delta Time
        float dt = Time.DeltaTime;     // seconds since last frame (use for movement OR timer!!!!!!!)
        t.X += 5.0f * Time.DeltaTime;  // Movement
        float spawnCooldown = 5.0f;    // Timer example :p
        if (spawnCooldown > 0)
        {
            spawnCooldown -= dt;
        }
        //Time.FrameCount;             // total frames since start
        //Time.GameTime;               // total time elapsed in seconds, Used for game Time Counter


        // 6. Game State API
        Application.Quit();               // close the game
        Application.IsPlaying();          // returns bool, check if game is currently playing
        Application.SetPaused(false);     // paused means set DeltaTime to = 0, so it no move if u apply dt 2 movement
        Application.IsPaused();           // returns bool, check if game is currently paused

        // 7. Sprite API
        s.Texture = "texturePath";          // set texture path
        string texPath = s.Texture;         // get current texture path

        s.TintR = 1.0f;                     // set red tint (0.0 - 1.0)
        s.TintG = 1.0f;                     // set green tint (0.0 - 1.0)
        s.TintB = 1.0f;                     // set blue tint (0.0 - 1.0)
        s.TintA = 1.0f;                     // set alpha/opacity (0.0 - 1.0)
        s.SetTint(1.0f, 1.0f, 1.0f, 1.0f); // set all tint values at once (r, g, b, a)

        s.UseTexture = true;                // true = use texture, false = use tint color only
        bool hasSprite = s.HasSprite();     // returns bool, check if entity has a sprite component

        // 8. Audio API
        audio.Play();                       // play this entity's own audiosource component
        audio.PlaySFX("SFX_Button_Hover");  // play sound by audio bank ID
        audio.PlayBGM("BGM_GENERAL");       // play BGM by ID
        audio.StopBGM();
        audio.PauseBGM();
        audio.ResumeBGM();

        //Setter for audio
        audio.SetBGMVolume(0.5f);           // 0.0 - 1.0
        audio.SetMasterVolume(0.5f);
        audio.SetMusicVolume(0.5f);
        audio.SetSfxVolume(0.5f);
        
        //Getter for audio
        audio.GetMasterVolume();            // returns float
        audio.GetMusicVolume();
        audio.GetSfxVolume();


        // 9. Vector2 for audio I think? TLDR, vec2 is now able to use in C#
        float x = 5, y = 5;
        Vector2 v = new Vector2(x,y);

        // 11. Mouse/Keyboard API 
        float PosX, PosY;
        PosX = input.MouseX;                    // screen space X
        PosY = input.MouseY;                    // screen space Y
        PosX = input.WorldMouseX;               // world space X (use for button hit detection?)
        PosY = input.WorldMouseY;               // world space Y
        bool OnScreen = input.InGameViewport;   // check if mouse is inside game window

        // Mouse buttons 0=left, 1=right, 2=middle
        input.IsMouseButtonPressed(0);    // held down
        input.IsMouseButtonTriggered(0);  // just clicked this frame
        input.IsMouseButtonReleased(0);  // just released this frame

        // Keyboard GLFW key codes (NOT ASCII)
        input.IsKeyPressed(32);    // held down
        input.IsKeyTriggered(32);  // just pressed this frame
        input.IsKeyReleased(32);   // just released this frame


        // 12. Animation API
        int row = 5, col = 5, totalFrames = 5; 
        float speed = 5.0f;
        anim.SetAnimation("SpriteName", row, col, totalFrames, speed);  // Set current entity animation 2 smth else
        string current = anim.GetCurrentAnimation();                    // Get current sprite name (returns full path string)
        anim.SetAnimationSpeed(speed);                                  // Change speed of animation

        // 13. Text API
        text.SetText("Hello World");        // set the text string
        string currentText = text.GetText(); // get the current text string

        text.SetColor(1.0f, 1.0f, 1.0f);   // set text color (r, g, b) 0.0 - 1.0
        text.SetFontSize(24.0f);            // set font size
        text.SetVisible(true);              // show/hide text
        bool isVisible = text.IsVisible();  // returns bool, check if text is visible

        // Alignment: 0 = left, 1 = center, 2 = right (check TextAlignment enum)
        text.SetAlignment(0);

        // 14. VFX API - all effects are explicitly opt-in, nothing fires automatically.

        // --- Camera shake only ---
        // BeatPulse: camera zoom-in punch (call from Conductor.cs on each beat)
        VFXAPI.BeatPulse();

        // ShakeCamera: camera shake
        VFXAPI.ShakeCamera(0.08f, 10.0f); // MISS-grade  - strong
        VFXAPI.ShakeCamera(0.05f, 8.0f);  // GOOD-grade  - moderate
        VFXAPI.ShakeCamera(0.02f, 6.0f);  // PERFECT-grade - subtle

        // --- Particles only ---
        // Colored circles at a world position (no texture):
        VFXAPI.SpawnParticles(0f, 0f, 10);   // MISS-grade   burst (10)
        VFXAPI.SpawnParticles(0f, 0f, 18);   // GOOD-grade   burst (18)
        VFXAPI.SpawnParticles(0f, 0f, 26);   // PERFECT-grade burst (26)

        // Textured quads at a world position:
        //   textureName = asset ID registered in texture.json
        //   size        = world-pixel size of each particle
        //   textureName(3rd parameter) of SpawnParticles must match the id inside texture.json 
        //   id is created in this manner inside of texture.json:
        //   { 
        //     "id": "particle", "path": "../../PulseProtocol/Assets/Textures/VFX/star.png",
        //     "vert_shader_path": "shader/sprite_vs.vert", "frag_shader_path": "shader/sprite_fs.frag",
        //     "mdl_ref": 1, "shd_ref": 2 
        //   }
        VFXAPI.SpawnParticles(0f, 0f, 12, "particle", 64f);


        // 15. FadeAPI - SCREEN-WIDE fullscreen overlay fade.
        //     Renders a colored overlay on top of ALL game content.
        //     Use this for scene transitions (e.g. fade to black between scenes).
        //     This is a STATIC class - no entity or component needed.
        //
        //   FadeOut: overlay alpha 0 - 1 (screen fills with color, game hidden)
        //   FadeIn:  overlay alpha 1 - 0 (color clears, game becomes visible)
        //
        // Black fade (default color):
        FadeAPI.FadeOut(1.0f);              // fade screen to black over 1 second
        FadeAPI.FadeIn(1.0f);               // fade screen in from black over 1 second

        // Colored fade (r, g, b in 0.0-1.0):
        FadeAPI.FadeOut(0.5f, 1f, 0f, 0f); // fade to red over 0.5 s
        FadeAPI.FadeIn(0.5f, 1f, 0f, 0f);  // fade in from red over 0.5 s

        // Control & query:
        FadeAPI.Stop();                     // immediately kill overlay (alpha to 0)
        bool screenDone  = FadeAPI.IsFadeDone(); // true once interpolation finishes
        float screenAlpha = FadeAPI.GetAlpha(); // current overlay alpha (0 = clear, 1 = opaque)
        FadeAPI.SetAlpha(0.5f);             // jump to alpha instantly (no interpolation)

        // example of a scene-transition:
        //   FadeAPI.FadeOut(0.5f);
        //   ... wait for IsFadeDone() ...
        //   Scene.LoadScene("NextScene");
        //   FadeAPI.FadeIn(0.5f);


        // 16. FadeComponent - PER-ENTITY alpha fade.
        //     Tweens the alpha (opacity) of THIS entity's sprite only.
        //     Use this to fade individual sprites in or out independently.
        //     Accessed via GetFade() - bound to the script's own entity.
        //
        FadeComponent entityFade = GetFade();

        entityFade.FadeOut(1.0f);           // fade THIS entity to 0 alpha over 1 second
        entityFade.FadeIn(1.0f);            // fade THIS entity to 1 alpha over 1 second
        entityFade.FadeTo(0.5f, 1.0f);      // tween alpha to any target (0.0-1.0) over duration
        entityFade.SetAlpha(1.0f);          // set alpha instantly (no tween)

        float entityAlpha = entityFade.GetAlpha();   // current sprite alpha
        bool  entityDone  = entityFade.IsDone();     // true when no tween is running

        // example of a blink/flash an entity:
        //   GetFade().FadeOut(0.1f);
        //   ... wait for IsDone() ...
        //   GetFade().FadeIn(0.1f);


        // 17. PostProcessAPI - SCREEN-WIDE bloom post-processing.
        //     Adds a glow halo around bright pixels (sprites, particles, VFX).
        //     Bloom is OFF by default - you must enable it explicitly.
        //     This is a STATIC class - no entity or component needed.
        //
        //     Pipeline (when enabled, per frame):
        //       1. Scene renders into an off-screen FBO at virtual resolution
        //       2. Bright-pass filter extracts pixels above the threshold
        //       3. 10-pass Gaussian blur creates the soft glow
        //       4. Composited back: final = scene + bloom * intensity
        //     Text and screen-overlay entities are NOT bloomed (rendered after).

        // Enable / disable the effect:
        PostProcessAPI.SetBloomEnabled(true);   // turn bloom on
        PostProcessAPI.SetBloomEnabled(false);  // turn bloom off (scene renders normally)
        bool bloomOn = PostProcessAPI.IsBloomEnabled();

        // Intensity - how bright the glow halo is (0.0 = none, 1.2 = default, 3.0 = max):
        PostProcessAPI.SetBloomIntensity(1.2f);          // default - subtle glow
        PostProcessAPI.SetBloomIntensity(2.5f);          // beat-flash - strong burst
        PostProcessAPI.SetBloomIntensity(0.0f);          // effectively off (same as disabled)
        float intensity = PostProcessAPI.GetBloomIntensity();

        // Threshold - luminance cutoff above which pixels contribute to bloom (0.0-1.0):
        //   low  threshold (0.2) = almost everything glows (background, sprites, etc.)
        //   high threshold (0.8) = only the very brightest pixels glow (near-white highlights)
        PostProcessAPI.SetBloomThreshold(0.5f);          // default - balanced
        PostProcessAPI.SetBloomThreshold(0.2f);          // low - whole scene glows
        PostProcessAPI.SetBloomThreshold(0.8f);          // high - only highlights glow
        float threshold = PostProcessAPI.GetBloomThreshold();

        // Example - beat-flash: spike intensity on beat, ease back over the interval:
        //   PostProcessAPI.SetBloomEnabled(true);
        //   PostProcessAPI.SetBloomIntensity(2.5f);   // call from Conductor on beat
        //   ... lerp back to 1.2 over the beat interval each Update() ...

        // Example - boss-entry dramatic effect:
        //   PostProcessAPI.SetBloomEnabled(true);
        //   PostProcessAPI.SetBloomThreshold(0.1f);   // whole scene glows
        //   PostProcessAPI.SetBloomIntensity(3.0f);   // max intensity
        //   ... after a few seconds, restore defaults ...
        //   PostProcessAPI.SetBloomThreshold(0.5f);
        //   PostProcessAPI.SetBloomIntensity(1.2f);


        // 18. GlowComponent - PER-ENTITY blurred glow halo.
        //     Adds a soft blurred halo around an entity.
        //     Independent of bloom - both can be active simultaneously.
        //     Accessed via GetGlow() - bound to the script's own entity.
        //
        //     The glow color is rendered as the entity silhouette in glow color,
        //     then blurred and composited additively on top of the scene.
        //
        GlowComponent glow = GetGlow();

        glow.SetEnabled(true);              // turn glow on for this entity
        glow.SetEnabled(false);             // turn glow off

        glow.SetColor(1f, 0.8f, 0f);       // golden glow (r, g, b in 0.0-1.0)
        glow.SetColor(0f, 1f, 1f);         // cyan glow
        glow.SetColor(1f, 1f, 1f);         // white glow (default)

        glow.SetIntensity(1.5f);            // set intensity instantly (0 = dim, 1 = normal, 2+ = intense)
        float currentIntensity = glow.GetIntensity();

        glow.GlowTo(2.0f, 0.5f);           // tween intensity to 2.0 over 0.5 seconds
        glow.GlowTo(0.0f, 1.0f);           // fade glow out over 1 second
        bool done = glow.IsDone();          // true when no tween is running

        bool glowOn = glow.IsEnabled();     // check if glow is currently enabled

        // Example - pulse glow on a pickup item:
        //   GlowComponent glow = GetGlow();
        //   glow.SetEnabled(true);
        //   glow.SetColor(1f, 1f, 0f);        // yellow glow
        //   glow.GlowTo(2.5f, 0.3f);          // flash bright
        //   ... wait for IsDone() ...
        //   glow.GlowTo(0.8f, 0.5f);          // ease back to subtle
        //
        // Example - boss entry dramatic glow:
        //   GetGlow().SetEnabled(true);
        //   GetGlow().SetColor(1f, 0f, 0f);   // red glow
        //   GetGlow().GlowTo(3.0f, 1.0f);     // ramp up over 1 second


        // 19. Gamepad/Controller API
        //     Accessed through the same InputComponent as keyboard/mouse.
        //     Supports PS4 (Bluetooth) and Xbox controllers via GLFW gamepad layout.
        //
        //     Use InputConstants for readable button names instead of raw indices:
        //       InputConstants.GP_CROSS      = 0   (PS4: ×   / Xbox: A)
        //       InputConstants.GP_CIRCLE     = 1   (PS4: ○   / Xbox: B)
        //       InputConstants.GP_SQUARE     = 2   (PS4: □   / Xbox: X)
        //       InputConstants.GP_TRIANGLE   = 3   (PS4: △   / Xbox: Y)
        //       InputConstants.GP_L1         = 4   (PS4: L1  / Xbox: LB)
        //       InputConstants.GP_R1         = 5   (PS4: R1  / Xbox: RB)
        //       InputConstants.GP_START      = 7   (PS4: Options / Xbox: Menu)
        //       InputConstants.GP_DPAD_UP    = 11
        //       InputConstants.GP_DPAD_RIGHT = 12
        //       InputConstants.GP_DPAD_DOWN  = 13
        //       InputConstants.GP_DPAD_LEFT  = 14
        //
        //     Axes (GetGamepadAxis):
        //       0 = Left stick X,  1 = Left stick Y
        //       2 = Right stick X, 3 = Right stick Y
        //       4 = L2 trigger,    5 = R2 trigger  (range: -1.0 to 1.0)

        bool connected = input.IsGamepadConnected;            // true if a gamepad is plugged in

        // Button states — same 3-state model as keyboard/mouse
        input.IsGamepadButtonPressed(InputConstants.GP_CROSS);    // held down
        input.IsGamepadButtonTriggered(InputConstants.GP_CROSS);  // just pressed this frame
        input.IsGamepadButtonReleased(InputConstants.GP_CROSS);   // just released this frame

        // Analog axes (-1.0 to 1.0)
        float leftStickX  = input.GetGamepadAxis(0);
        float leftStickY  = input.GetGamepadAxis(1);
        float rightStickX = input.GetGamepadAxis(2);
        float rightStickY = input.GetGamepadAxis(3);
        float l2Trigger   = input.GetGamepadAxis(4);
        float r2Trigger   = input.GetGamepadAxis(5);

        // Rumble: leftMotor and rightMotor are 0.0-1.0 intensity; durationSec is how long
        input.RumbleGamepad(0.5f, 0.5f, 0.2f);   // light rumble for 0.2 s (e.g. button confirm)
        input.RumbleGamepad(1.0f, 1.0f, 0.5f);   // strong rumble for 0.5 s (e.g. hit taken)
        input.RumbleGamepad(0.0f, 0.0f, 0.0f);   // stop rumble immediately

        // Example - detect any face button press and rumble:
        //   if (input.IsGamepadConnected &&
        //       input.IsGamepadButtonTriggered(InputConstants.GP_CROSS))
        //   {
        //       input.RumbleGamepad(0.4f, 0.4f, 0.1f);
        //   }


        // 20. Screen Mode API (ToggleScreenAPI)
        //     Static class for switching between fullscreen and windowed mode at runtime.
        //     Typically called from settings UI scripts.

        ToggleScreenAPI.ToggleFullscreen();               // switch between fullscreen and windowed
        ToggleScreenAPI.SetFullscreen(true);              // force fullscreen
        ToggleScreenAPI.SetFullscreen(false);             // force windowed
        bool isFullscreen = ToggleScreenAPI.IsFullscreen(); // returns bool — true if currently fullscreen

        // Example - settings button that persists the choice:
        //   ToggleScreenAPI.ToggleFullscreen();
        //   UserSettingsStore.SetFullscreen(ToggleScreenAPI.IsFullscreen());
    }
}