================================================================================

              ____  _   _ _     ____  _____ 
             |  _ \| | | | |   / ___|| ____|
             | |_) | | | | |   \___ \|  _|  
             |  __/| |_| | |___ ___) | |___ 
             |_|    \___/|_____|____/|_____|
                                                   
          ____  ____   ___ _____ ___   ____ ___  _     
         |  _ \|  _ \ / _ \_   _/ _ \ / ___/ _ \| |    
         | |_) | |_) | | | || || | | | |  | | | | |    
         |  __/|  _ <| |_| || || |_| | |__| |_| | |___ 
         |_|   |_| \_\\___/ |_| \___/ \____\___/|_____|

                 ~ Rhythm-Action Combat ~

================================================================================


[ 1. GAME CONCEPT ]

Pulse Protocol is a rhythm-action combat game set in a cyberworld.

Players execute attacks by pressing keys in time with the beat. A visual border
pulses on screen every 0.5 seconds (120 BPM), accompanied by a sound effect,
to help players stay on rhythm.

The system evaluates timing accuracy and provides feedback:
- PERFECT : Within 0.5s of the beat
- GOOD    : Within 1.0s of the beat
- MISS    : More than 1.0s off the beat


[ 2. CUSTOM DEMO INPUT & USAGE ]

Gameplay:
---------
- A countdown sequence plays: "Ready" → "3" → "2" → "1" → "GO!"
- Once the countdown finishes, the enemy automatically begins
- Press W then A in rhythm with the pulsing border to attack
- Watch for on-screen feedback showing timing accuracy (PERFECT/GOOD/MISS)

Attack Combo:
W → A → W → A (press in rhythm with the beat)

Editor Controls:
----------------
- Play Button    : Starts the game and switches to Game Camera
- Stop Button    : Stops the game and switches back to Editor Camera
- Pause Button   : Pauses the game while in Game Camera
- Project Panel  : Displays all project assets
- Inspector      : Shows all components of selected entity
- Layer Panel    : Located beside the Inspector tab, allows locking/hiding layers

Developer Controls:
-------------------
F1    - Toggle Debug Rectangles
F2    - Toggle Debug Lines
F3    - Toggle Debug Circles
F4    - Toggle Debug Circles Filled
F5    - Toggle Debug Points
F6    - Toggle Debug Grid
F8    - Toggle Debug Overlay (FPS counter, performance stats)
F11   - Toggle Fullscreen
Space - Queue delayed combo message (test feature)


[ 3. HOW TO PLAY ] 

As a cyborg with the power to harness music and utilise it as a weapon against enemies, you can use the designated keys to create different combos to attack and defeat enemies.

Using a combination of the four designated keys, 'W, A, S, D', you will face again waves of enemies you must defeat to reach your goal as humanity's last hope.

You will see a slip with the enemy type and weakness on briefly, make sure to memorise it before it fades! 

Once the enemy appears, make sure to watch closely before acting, pressing the keys before you know the enemy's movement pattern means you are one step closer to getting hurt!

Match each enemy’s rhythm and perform their weapon combo at the right beats to defeat them.


[ 4. ALL CONTROLS (Keys) ]
 
- 'W' Key : SFX_W_Key.mp3
- 'A' Key : SFX_A_Key.mp3
- 'S' Key : SFX_S_Key.mp3 
- 'D' Key : SFX_D_Key.mp3 


[ 5. FEATURES ]

ALL CURRENT PLAYABLE/FUNCTIONING FEATURES:

Combo System: You are able to activate 1 type of weapons with the following combos:
1. 'WAWA' (Spear) 

Enemy Combat: You are able to input the combo of the enemy's weakness and defeat the enemy successfully


INCOMPLETE FEATURES: 

Slip System: Fading in and out of the slips is not fully functional at the moment, hence all slips are fully display throughout for now, will be functional by M3. 

Full Level Progression: Currently you are only able to defeat one enemy before you win the game. 

HP Progression: If you fail to input the combo successfully before the enemy reaches you, you die instantly at the moment.


[ 6. CHEAT CODES ]

Level Skip Cheat Code:
----------------------
Shift + 1  : Skip to Level 1
Shift + 2  : Skip to Level 2
Shift + 3  : Skip to Level 3

In-Game Cheat Code:
-------------------
I          : Toggle player invincibility (player takes no damage)
K          : Instantly kill the current enemy (deals 99999 damage)
O          : Clear the current wave (triggers the stage clear overlay)


In-Game Controller Cheat Code: 
------------------------------
L1 + R1 + Triangle : Instantly kill the current enemy (deals 99999 damage)

Note: These cheat codes above only work when you are in the levels.

Skip Cut Scene Cheat Code:
--------------------------
Space Bar  : Skip the opening cutscene when its being shown.


[ 7. TEAM ROSTER ]

Chloe Lau Rey En (IMGD)
- Role: Product Manager
- Champion: Input & Debugging

Reginald Lew Yee Ren (IMGD)
- Role: Programmer
- Champion: Physics & Collision

Leu Jun Yong (IMGD)
- Role: Programmer
- Champion: Gameplay/AI & Level Design

Chia Wei Xuan, Rachael (RTIS)
- Role: Tech Lead
- Champion: Production

Goh Pin Kai (RTIS)
- Role: Programmer
- Champion: Graphics

Ban Kai Wei Benjamin (RTIS)
- Role: Programmer
- Champion: Engine

Carrie Lam Tze Ying (UXGD)
- Role: Design Lead
- Champion: Audio

Chong Sze Ting Fenny (BFA)
- Role: Art Lead
- Champion: Art
================================================================================