/******************************************************************************/
/**
 * @file        GameLogic.cpp
 * @project     Pulse Protocol
 * @author		Chia Wei Xuan Rachael (primary) - 50%
 * @author		Reginald Lew Yee Ren (secondary) - 15%
 * @author		Chloe Lau Rey En (secondary) - 20%
 * @author		Goh Pin Kai (secondary) - 10%
 * @author		Ban Kai Wei Benjamin (secondary) - 5%
 * @brief		Core Components:
 *              - LogicSystem: Manages script registration, lifecycle (Init/Update/Exit),
 *                and execution for all entities with LogicComponent. Only updates during
 *                gameplay (respects Engine->IsPlaying() state).
 *              - Iscript Interface: Base class for all game scripts with Init, Update, Exit.
 *              - LogicComponent: Stores script name per entity, supports JSON serialization.
 *
 *
 *              Implements game entity logic for both Player and Enemy.
 *              Handles entity movement, rotation toggling, and two collision test modes:
 *              AABB (Axis-Aligned Bounding Box) via 'M' key and
 *              OBB (Oriented Bounding Box) via 'N' key.
 *              Includes the LogicSystem that registers, updates, and executes scripts.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "GameLogic.h"
#include "pch/pch_temp.h"
#include "CoreEngine/Factory/Factory.h"
#include "CoreEngine/Core/CoreEngine.h"

//bool EnemyScript::testAABBCollision = false;
//bool EnemyScript::testOBBCollision = false;

// ===== Global enemy order variables (declared in GameLogic.cpp) =====
extern Entity g_firstEnemy;
extern Entity g_secondEnemy;
extern bool   g_lockDetection;

// ===== Global countdown freeze flag =====
extern bool g_countdownFinished;

// ===== GLOBAL COUNTDOWN STATE =====
bool g_countdownFinished = false;

// ===== GLOBAL STATIC TRACKING FOR ENEMY ORDER =====
Entity g_firstEnemy = (Entity)-1;
Entity g_secondEnemy = (Entity)-1;  
bool   g_lockDetection = false;

LogicSystem::LogicSystem()
{

}

LogicSystem::~LogicSystem()
{

}

void LogicSystem::Init()
{
    //auto* coord = Coordinator::GetInstance();

    ////function pointer to C++/CLI function AddScriptViaName
    //auto addScriptFunc = Engine->GetFunctionPtr<bool(*)(int, const char*)>
    //    (
    //        "ScriptAPI",
    //        "ScriptAPI.EngineInterface",
    //        "AddScriptViaName"
    //    );
       
    //auto* coord = Coordinator::GetInstance();
    //auto rhythm = coord->GetSystem<RhythmGameplayInput>();
    //if (rhythm)
    //{
    //    rhythm->registerInputListener("VFX_Judgement",
    //        [this](const InputEvent& evt)
    //        {
    //            if (evt.type != InputEvent::Type::COMBO_EXECUTED)
    //                return;

    //            // Choose a spawn position (replace with your hit-line position if you have one)
    //            glm::vec2 spawnPos{ 0.0f, 0.0f };

    //            // If you can access something like evt.hitPos, use that instead:
    //            // glm::vec2 spawnPos = evt.hitPos;

    //            mVfx.OnJudgement(evt.feedback, spawnPos);
    //        });
    //}

    mVfx.Init(/*maxParticles=*/2000);

    /*AddScript("PlayerScript", std::make_shared<PlayerScript>());*/
    AddScript("EnemyScript", std::make_shared<EnemyScript>());
    //AddScript("CountdownScript", std::make_shared<CountdownScript>());
    //AddScript("FeedbackScript", std::make_shared<FeedbackScript>());
}

void LogicSystem::Update(float dt)
{
    mVfx.Update(dt);
    // don't update scripts, if not playing the game
    if (!Engine->IsPlaying()) {
        return;
    }

    // run thru all the c# scripts (Inside ScriptAPI) 
    auto ExecuteUpdate = Engine->GetFunctionPtr<void(*)()>(
        "ScriptAPI",
        "ScriptAPI.EngineInterface",
        "ExecuteUpdate"
    );

    if (ExecuteUpdate)
    {
        
            //std::cout << "[LogicSystem] ExecuteUpdate function found, calling...\n";

        ExecuteUpdate();

           // std::cout << "[LogicSystem] ExecuteUpdate finished\n";
    }
    else
    {
        std::cerr << "[LogicSystem] ERROR: ExecuteUpdate is NULL!\n";
    }

    // make copy first
    //auto entities = EntityMember;

    // only update all entity that has c++ scripts
    //for (auto const& entity : entities)
    //{
    //    // get script name from each entity and feed them to its respective scripts
    //    auto& scriptname = g_coordinator->GetComponent<LogicComponent>(entity);

    //    // find reward (script update function) using script name
    //    auto func = map_of_scripts.find(scriptname.ScriptName);

    //    // if we can find it then good, we did it right XD. and now we call said function
    //    if (func != map_of_scripts.end())
    //    {
    //        func->second->Update(entity);
    //    }

    //    else
    //    {
    //        // THIS IS WHERE U LEFT OFF DUMBASS: CheeseBuger
    //        //std::cerr << "CAN'T FIND SCRIPTS UPDATE FUNCTION!" << std::endl;
    //    }
    //}
}

void LogicSystem::Exit()
{
    //auto* g_coordinator = Coordinator::GetInstance();

    //// clear all scripts 
    //auto clearFunc = Engine->GetFunctionPtr<void(*)()>(
    //    "ScriptAPI",
    //    "ScriptAPI.EngineInterface",
    //    "ClearAllScripts"
    //);
    //clearFunc();


   /* for (auto const& entity : EntityMember)
    {
        auto& scriptname = g_coordinator->GetComponent<LogicComponent>(entity);
        auto func = map_of_scripts.find(scriptname.ScriptName);

        if (func != map_of_scripts.end())
        {
            func->second->Exit(entity);
        }
    }*/

    // Clear the script map
    map_of_scripts.clear();
}

// Might change std::function to a class of the init(), update(), and exit(). Smth like Iscript, can i sub in Iscript for std::function
void LogicSystem::AddScript(std::string const& ScriptName, std::shared_ptr<Iscript> script)
{
    // add this type of script to unlock this kind of update function. 
    // exp: name = monsterscript. Unlocks => monsters script class functions  
    map_of_scripts[ScriptName] = script;
}

std::shared_ptr<Iscript> LogicSystem::GrabScript(std::string const& ScriptName)
{
    // grab reward(script update function)
    auto func = map_of_scripts.find(ScriptName);
    if (func != map_of_scripts.end())
    {
        return func->second;
    }

    else
    {
        //std::cerr << "CAN'T FIND SCRIPTS UPDATE FUNCTION!" << std::endl;
        return nullptr;
    }
}

void LogicComponent::Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const
{
    //using namespace rapidjson;
    out.SetObject();

    rapidjson::Value scriptsArray(rapidjson::kArrayType);

    for (const auto& scriptName : ScriptName)
    {
        rapidjson::Value nameVal(scriptName.c_str(), alloc);
        scriptsArray.PushBack(nameVal, alloc);
    }

    out.AddMember("ScriptName", scriptsArray, alloc);

    //out.AddMember("ScriptName", rapidjson::Value(ScriptName.c_str(), alloc), alloc);
}

void LogicComponent::Deserialize(const rapidjson::Value& in)
{
    if (!in.IsObject()) throw std::runtime_error("Invalid JSON for LogicComponent");

    ScriptName.clear();

    if (in.HasMember("ScriptName") && in["ScriptName"].IsArray()) 
    {
        const auto& arr = in["ScriptName"];
        for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
        {
            if (arr[i].IsString())
            {
                ScriptName.push_back(arr[i].GetString());
            }
        }
    }
    
    //if (in.HasMember("ScriptName")) ScriptName = in["ScriptName"].GetString(); 

}

bool SaveScriptName(LogicComponent const& t, std::string const& path)
{
    rapidjson::Document d;
    d.SetObject();
    auto& alloc = d.GetAllocator();

    rapidjson::Value ScriptComponents;
    t.Serialize(ScriptComponents, alloc);
    // d.AddMember("ScriptName", ScriptComponents, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);

    // MUST HAVE TO RETURN BACK TO MYENGINE INSTEAD OF BUILD(AKA GETS DELETE WHEN CLEANED)
    namespace fs = std::filesystem;
    fs::create_directories(FilePathToGame / "JSON"); // use active game path, not hardcoded PulseProtocol

    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        std::cerr << "Error: Cannot open file for writing: " << path << "\n";
        return false;
    }
    ofs << buffer.GetString();
    if (!ofs.good()) {
        std::cerr << "Error: Failed to write JSON to file: " << path << "\n";
        return false;
    }
    return true;
}

bool LoadScriptName(LogicComponent& t, std::string const& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open())
    {
        std::cerr << "Error: Cannot open file for reading: " << path << "\n";
        return false;
    }
    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document d;
    d.ParseStream(isw);

    if (d.HasParseError()) {
        std::cerr << "Error: Failed to parse JSON in file: " << path << "\n";
        return false;
    }

    if (!d.HasMember("ScriptName")) {
        std::cerr << "Error: No 'transform' field in file: " << path << "\n";
        return false;
    }

    t.Deserialize(d["ScriptName"]);
    return true;
}

bool LogicComponent::operator==(LogicComponent const& other) const
{
    return ScriptName == other.ScriptName;
}

bool LogicComponent::operator!=(LogicComponent const& other) const
{
    return !(*this == other);
}

// ---------------------------------------------------------------------
// FOR SCRIPTS ONLY ONWARDS
// ---------------------------------------------------------------------

//// =====================================================================
//// PLAYER SCRIPT
//// =====================================================================
//void PlayerScript::Init(Entity ID)
//{
//    //autoMoveTimer = 2.0f; // reset timer
//    auto* coord = Coordinator::GetInstance();
//    auto  rhythm = coord->GetSystem<RhythmGameplayInput>();
//
//    if (!rhythm) return;
//
//    rhythm->registerInputListener("PlayerCombo",
//        [this, ID](const InputEvent& evt)
//        {
//            if (evt.type != InputEvent::Type::COMBO_EXECUTED)
//                return;
//
//            if (evt.comboName.empty() || evt.comboName != "Basic Strike")
//                return;
//
//            auto* c = Coordinator::GetInstance();
//
//            if (!c->HasComponent<Framework::Renderable>(ID) ||
//                !c->HasComponent<Framework::Animation>(ID))
//                return;
//
//            auto& rend = c->GetComponent<Framework::Renderable>(ID);
//            auto& anim = c->GetComponent<Framework::Animation>(ID);
//
//            // PLAY SUCCESS SFX FROM PLAYER'S AUDIOSOURCE
//            if (c->HasComponent<AudioSource>(ID))
//            {
//                auto& audioSrc = c->GetComponent<AudioSource>(ID);
//                if (auto audio = c->GetSystem<AudioManager>())
//                {
//                    audio->PlayFromAudioSource(audioSrc);
//                }
//            }
//
//            // SWITCH TO ATTACK SPRITE
//            rend.spriteName = "../../PulseEngine/Assets/Textures/Characters/hero_spritesheet.png";
//            rend.needsTextureReload = true;
//
//            anim.totalFrames = 3;
//            anim.columns = 3;
//            anim.rows = 1;
//            anim.animationSpeed = 8.0f;
//            anim.currentFrame = 0;
//            anim.elapsedTime = 0.0f;
//
//            isAttacking = true;
//            attackTimer = 0.5f;
//
//            // === KILL THE ACTIVE ENEMY ===
//            extern Entity g_firstEnemy;
//            extern Entity g_secondEnemy;
//
//            Entity targetEnemy = (Entity)-1;
//
//            // Kill first enemy if alive, otherwise kill second
//            if (g_firstEnemy != (Entity)-1 && c->HasComponent<Health>(g_firstEnemy))
//            {
//                auto& hp = c->GetComponent<Health>(g_firstEnemy);
//                if (hp.isAlive)
//                {
//                    targetEnemy = g_firstEnemy;
//                }
//            }
//
//            if (targetEnemy == (Entity)-1 && g_secondEnemy != (Entity)-1 && c->HasComponent<Health>(g_secondEnemy))
//            {
//                auto& hp = c->GetComponent<Health>(g_secondEnemy);
//                if (hp.isAlive)
//                {
//                    targetEnemy = g_secondEnemy;
//                }
//            }
//
//            // Delete the enemy
//            if (targetEnemy != (Entity)-1)
//            {
//                if (c->HasComponent<Health>(targetEnemy))
//                {
//                    c->GetComponent<Health>(targetEnemy).isAlive = false;
//                }
//
//                Factory factory;
//                factory.DeleteEntity(targetEnemy);
//
//                // Check if ALL enemies are now dead -> GameWin
//                bool firstDead = (g_firstEnemy == (Entity)-1) ||
//                    !c->HasComponent<Health>(g_firstEnemy) ||
//                    !c->GetComponent<Health>(g_firstEnemy).isAlive;
//                bool secondDead = (g_secondEnemy == (Entity)-1) ||
//                    !c->HasComponent<Health>(g_secondEnemy) ||
//                    !c->GetComponent<Health>(g_secondEnemy).isAlive;
//
//                if (firstDead && secondDead)
//                {
//                    auto sceneMgr = c->GetSystem<SceneManager>();
//                    if (sceneMgr)
//                    {
//                        sceneMgr->CurrentScene = "GameWin";
//                    }
//                }
//            }
//        });
//}
//
//void PlayerScript::Update(Entity ID)
//{
//    auto g_coordinator = Coordinator::GetInstance();
//
//    static double lastTime = glfwGetTime();
//    float dt = Engine->GetDeltaTime();
//
//    // switch back to idle animation
//    if (isAttacking)
//    {
//        attackTimer -= dt;
//        if (attackTimer <= 0.0f)
//        {
//            isAttacking = false;
//
//            if (g_coordinator->HasComponent<Framework::Renderable>(ID) &&
//                g_coordinator->HasComponent<Framework::Animation>(ID))
//            {
//                auto& rend = g_coordinator->GetComponent<Framework::Renderable>(ID);
//                auto& anim = g_coordinator->GetComponent<Framework::Animation>(ID);
//
//                // idle sheet (your 2-frame image)
//                rend.spriteName = "../../PulseEngine/Assets/Textures/Characters/hero_idle.png";
//                rend.needsTextureReload = true;
//
//                anim.totalFrames = 2;
//                anim.columns = 2;
//                anim.rows = 1;
//                anim.animationSpeed = 2.0f;
//                anim.currentFrame = 0;
//                anim.elapsedTime = 0.0f;
//            }
//        }
//    }
//}
//
//void PlayerScript::Exit(Entity ID)
//{
//    (void)ID;
//}
//
//void PlayerScript::Serialize() 
//{
//    health++;
//    //enemyDmg; can't use since its enemy.
//}
//
//PlayerScript* PlayerScript::Clone() {
//    return new PlayerScript{};
//}
//// Static variables
//Entity EnemySpawnerScript::Enemy1ID = (Entity)-1;
//Entity EnemySpawnerScript::Enemy2ID = (Entity)-1;
//bool  EnemySpawnerScript::Enemy1Dead = false;
//
//std::unordered_map<Entity, float> EnemySpawnerScript::HopTimer;
//
//// -------------------------------------------------------
//// Init: Assign first enemy as Enemy1, second as Enemy2
//// -------------------------------------------------------
//void EnemySpawnerScript::Init(Entity ID)
//{
//    if (Enemy1ID == (Entity)-1)
//    {
//        Enemy1ID = ID;
//        std::cout << "[Spawner] Enemy 1 = " << ID << "\n";
//    }
//    else
//    {
//        Enemy2ID = ID;
//        std::cout << "[Spawner] Enemy 2 = " << ID << "\n";
//    }
//
//    HopTimer[ID] = 0.0f; // initialize hop timer per enemy
//}
//
//// -------------------------------------------------------
//// Update: Check if Enemy1 is dead
//// -------------------------------------------------------
//void EnemySpawnerScript::Update(Entity ID)
//{
//    auto g = Coordinator::GetInstance();
//    float dt = (float)GLHelper::delta_time;
//
//    // --- Check if enemy 1 has died ---
//    if (Enemy1ID != (Entity)-1)
//    {
//        auto& hp = g->GetComponent<Health>(Enemy1ID);
//        if (!hp.isAlive)
//            Enemy1Dead = true;
//    }
//
//    auto& transform = g->GetComponent<Framework::Transform>(ID);
//
//    // --- Enemy movement gating ---
//    bool canMove = false;
//
//    if (ID == Enemy1ID)
//        canMove = true;            // enemy1 moves immediately
//    else if (ID == Enemy2ID)
//        canMove = Enemy1Dead;      // enemy2 waits until enemy1 dies
//
//    if (!canMove)
//        return;
//
//    // =============================
//    // HOPPING MOVEMENT LOGIC
//    // =============================
//    float& timer = HopTimer[ID];
//    timer += dt;
//
//    const float hopInterval = 0.50f;     // time between hops
//    const float hopDistance = 80.0f;     // hop length
//
//    if (timer >= hopInterval)
//    {
//        transform.Pos.x -= hopDistance;  // instant left hop
//        timer = 0.0f;
//    }
//}
//
//// -------------------------------------------------------
//// Exit: Mark Enemy1 as dead if it leaves scene
//// -------------------------------------------------------
//void EnemySpawnerScript::Exit(Entity ID)
//{
//    if (ID == Enemy1ID)
//        Enemy1Dead = true;
//
//    HopTimer.erase(ID);
//}

// =====================================================================
// ENEMY SCRIPT
// =====================================================================
void EnemyScript::Init(Entity ID)
{
    std::cout << "[EnemyScript] Init() called for entity " << ID << "\n";

    // =========================================================================
    // IMPORTANT FIX #1:
    // Always reset enemy order detection upon scene reload.
    // Previously, this was guarded by static 'firstRun' which only ran once.
    // This caused enemies to share stale data between gameplay sessions.
    // =========================================================================
    g_firstEnemy = (Entity)-1;
    g_secondEnemy = (Entity)-1;
    g_lockDetection = false;

    // =========================================================================
    // IMPORTANT FIX #2:
    // Clear beat-tracking state for ALL enemies.
    // Static maps store data across scenes if not cleared.
    // =========================================================================
    {
        static std::unordered_map<Entity, bool> wasOnBeat;
        wasOnBeat.clear();
    }

    std::cout << "[EnemyScript] Enemy state fully reset.\n";
}

void EnemyScript::Update(Entity ID)
{
    // =========================================================================
    // FREEZE UNTIL COUNTDOWN COMPLETES
    // =========================================================================
    if (!g_countdownFinished)
        return;

    auto g = Coordinator::GetInstance();

    // Validate all required components
    if (!g->HasComponent<Framework::Transform>(ID) ||
        !g->HasComponent<AABB>(ID) ||
        !g->HasComponent<Health>(ID))
        return;

    auto& enemy_transform = g->GetComponent<Framework::Transform>(ID);
    auto& enemy_box = g->GetComponent<AABB>(ID);
    auto& enemy_health = g->GetComponent<Health>(ID);

    float dt = static_cast<float>(GLHelper::delta_time);

    if (!enemy_health.isAlive)
        return;

    // =========================================================================
    // DETERMINE FIRST & SECOND ENEMY
    // This is done ONCE per gameplay session.
    // Second enemy must remain frozen until the first dies.
    // =========================================================================
    if (!g_lockDetection)
    {
        if (g_firstEnemy == (Entity)-1)
        {
            g_firstEnemy = ID;
            // std::cout << "[EnemyScript] REG: First enemy = " << ID << "\n";
        }
        else if (g_secondEnemy == (Entity)-1 && ID != g_firstEnemy)
        {
            g_secondEnemy = ID;
            g_lockDetection = true;
            // std::cout << "[EnemyScript] REG: Second enemy = " << ID << "\n";
        }
    }

    // =========================================================================
    // FREEZE SECOND ENEMY UNTIL FIRST ENEMY IS DEAD
    // =========================================================================
    if (ID == g_secondEnemy)
    {
        if (g_firstEnemy != (Entity)-1 &&
            g->HasComponent<Health>(g_firstEnemy))
        {
            auto& hp = g->GetComponent<Health>(g_firstEnemy);

            // First enemy still alive? -> freeze second enemy completely.
            if (hp.isAlive)
                return;
        }
    }

    // =========================================================================
    // BEAT-SYNCED HOP MOVEMENT
    // =========================================================================
    static std::unordered_map<Entity, bool> wasOnBeat;

    auto audioSystem = g->GetSystem<AudioManager>();
    if (!audioSystem)
        return;

    double musicPos = audioSystem->GetBGMPositionSeconds();
    if (musicPos <= 0.0)
        return;

    float beatInterval = 0.5f;    // how long per beat
    float beatWindow = 0.15f;   // beat detection window

    float normalizedTime = fmodf((float)musicPos, beatInterval) / beatInterval;

    bool onBeat =
        (normalizedTime < beatWindow ||
            normalizedTime >(1.0f - beatWindow));

    // Enemy hops EXACTLY when entering the beat window
    if (onBeat && !wasOnBeat[ID])
    {
        enemy_transform.Pos.x -= 100.0f;   // hop left
    }

    wasOnBeat[ID] = onBeat;

    // =========================================================================
    // MANUAL DEBUG MOVEMENT — unchanged (arrow keys)
    // =========================================================================
    float move = 300.0f;

    if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_UP) == GLFW_PRESS)
        enemy_transform.Pos.y += move * dt;

    if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_LEFT) == GLFW_PRESS)
        enemy_transform.Pos.x -= move * dt;

    if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_DOWN) == GLFW_PRESS)
        enemy_transform.Pos.y -= move * dt;

    if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_RIGHT) == GLFW_PRESS)
        enemy_transform.Pos.x += move * dt;

    // =========================================================================
    // COLLISION WITH PLAYER
    // =========================================================================
    std::vector<Entity> players;
    auto logicSys = g->GetSystem<LogicSystem>();

    for (auto const& e : logicSys->EntityMember)
    {
        if (!g->HasComponent<LogicComponent>(e)) continue;
        //if (g->GetComponent<LogicComponent>(e).ScriptName == "PlayerScript")
          //  players.push_back(e);
    }

    if (players.empty())
        return;

    CollisionManager collision;

    AABB enemyAABB = collision.BuildAABBFromTransform(enemy_transform, enemy_box);

    for (Entity p : players)
    {
        if (!g->HasComponent<Framework::Transform>(p) ||
            !g->HasComponent<AABB>(p) ||
            !g->HasComponent<Health>(p))
            continue;

        auto& pt = g->GetComponent<Framework::Transform>(p);
        auto& pb = g->GetComponent<AABB>(p);
        auto& ph = g->GetComponent<Health>(p);

        AABB playerAABB = collision.BuildAABBFromTransform(pt, pb);

        if (collision.CheckAABBCollision(enemyAABB, playerAABB))
        {
            // Play enemy SFX
            if (g->HasComponent<AudioSource>(ID))
            {
                auto& audioSrc = g->GetComponent<AudioSource>(ID);
                if (!audioSrc.audio_id.empty())
                {
                    if (auto audioManager = g->GetSystem<AudioManager>())
                        audioManager->PlayFromAudioSource(audioSrc);
                }
            }

            // Apply damage
            enemy_health.isAlive = false;
            ph.hp -= enemy_health.damage;

            // Destroy enemy
            Factory factory;
            factory.DeleteEntity(ID);

            // Game Over
            auto sceneMgr = g->GetSystem<SceneManager>();
            if (sceneMgr)
            {
                sceneMgr->CurrentScene = "GameLose";
            }

            return;
        }
    }
    //auto g_coordinator = Coordinator::GetInstance();
    //if (!g_coordinator->HasComponent<Framework::Transform>(ID) ||
    //    !g_coordinator->HasComponent<AABB>(ID) ||
    //    !g_coordinator->HasComponent<Health>(ID))
    //    return;

    //auto& enemy_transform = g_coordinator->GetComponent<Framework::Transform>(ID);
    //auto& enemy_box = g_coordinator->GetComponent<AABB>(ID);
    //auto& enemy_health = g_coordinator->GetComponent<Health>(ID);

    //// ====== Time (for time-based movement) ======
    //float dt = static_cast<float>(GLHelper::delta_time);

    //if (!enemy_health.isAlive)
    //    return;

    //// ============================================================
    //// AUTO-DETECT FIRST & SECOND ENEMY SAFELY
    //// ============================================================
    //static Entity firstEnemy = (Entity)-1;
    //static Entity secondEnemy = (Entity)-1;

    //// Register the first enemy that enters update()
    //if (firstEnemy == (Entity)-1)
    //    firstEnemy = ID;
    //else if (secondEnemy == (Entity)-1 && ID != firstEnemy)
    //    secondEnemy = ID;

    //// ============================================================
    //// FREEZE SECOND ENEMY UNTIL FIRST ENEMY DIES
    //// ============================================================
    //if (ID == secondEnemy)
    //{
    //    // Check if first is still alive
    //    if (firstEnemy != (Entity)-1)
    //    {
    //        auto& firstHP = g->GetComponent<Health>(firstEnemy);
    //        if (firstHP.isAlive)
    //            return;   // ❗ Freeze enemy 2 completely
    //    }
    //}

    //// ============================================================
    //// HOPPER MOVEMENT (Teleport style)
    //// ============================================================
    //// Per-enemy hop timer
    //static std::unordered_map<Entity, float> hopTimer;
    //hopTimer[ID] += dt;

    //const float hopInterval = 0.20f;     // time between hops (adjust)
    //const float hopDistance = 40.0f;     // size of hop left (adjust)

    //if (hopTimer[ID] >= hopInterval)
    //{
    //    enemy_transform.Pos.x -= hopDistance;
    //    hopTimer[ID] = 0.0f;
    //}

    //// ====== Manual arrow-key movement (kept for debug) ======
    //float enemyMoveSpeed = 300.0f;

    //if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_UP) == GLFW_PRESS)
    //{
    //    enemy_transform.Pos.y += enemyMoveSpeed * dt;
    //}

    //if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_LEFT) == GLFW_PRESS)
    //{
    //    enemy_transform.Pos.x -= enemyMoveSpeed * dt;
    //}

    //if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_DOWN) == GLFW_PRESS)
    //{
    //    enemy_transform.Pos.y -= enemyMoveSpeed * dt;
    //}

    //if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_RIGHT) == GLFW_PRESS)
    //{
    //    enemy_transform.Pos.x += enemyMoveSpeed * dt;
    //}

    //// // ====== Mode toggles (your original AABB / OBB debug modes) ======
    //// static bool aabbMode = false;  // AABB slide + edge collision
    //// static bool obbMode = false;  // OBB spin + chase test
    //// static bool wasMPressed = false;
    //// static bool wasNPressed = false;

    //// // --- Handle M toggle (AABB) ---
    //// if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_M) == GLFW_PRESS)
    //// {
    ////     if (!wasMPressed)
    ////     {
    ////         aabbMode = !aabbMode;
    ////         obbMode = false; // disable OBB when AABB is on
    ////         wasMPressed = true;
    ////         std::cout << (aabbMode ? "[ENEMY] AABB Mode ON\n" : "[ENEMY] AABB Mode OFF\n");
    ////     }
    //// }
    //// else
    ////     wasMPressed = false;

    //// // --- Handle N toggle (OBB) ---
    //// if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_N) == GLFW_PRESS)
    //// {
    ////     if (!wasNPressed)
    ////     {
    ////         obbMode = !obbMode;
    ////         aabbMode = false; // disable AABB when OBB is on
    ////         wasNPressed = true;
    ////         std::cout << (obbMode ? "[ENEMY] OBB Mode ON\n" : "[ENEMY] OBB Mode OFF\n");
    ////     }
    //// }
    //// else
    ////     wasNPressed = false;

    //// ====== Find ALL Player Entities (unchanged) ======
    //std::vector<Entity> playerEntities;
    //auto logicSys = g_coordinator->GetSystem<LogicSystem>();
    //for (auto const& e : logicSys->EntityMember)
    //{
    //    if (!g_coordinator->HasComponent<LogicComponent>(e))
    //        continue;

    //    auto& logic = g_coordinator->GetComponent<LogicComponent>(e);
    //    if (logic.ScriptName == "PlayerScript")
    //    {
    //        playerEntities.push_back(e);
    //    }
    //}

    //if (playerEntities.empty())
    //    return;

    //CollisionManager collision;

    //// // ====================================================================
    //// // === MODE 1: AABB SLIDE & EDGE COLLISION (M) ========================
    //// // ====================================================================
    //// if (aabbMode)
    //// {
    ////     float slideSpeed = 300.0f;   // units per second
    ////     enemy_transform.Pos.x -= slideSpeed * dt;

    ////     AABB enemyAABB = collision.BuildAABBFromTransform(enemy_transform, enemy_box);

    ////     // Loop through ALL players to check collision
    ////     for (Entity playerEntity : playerEntities)
    ////     {
    ////         if (!g_coordinator->HasComponent<Framework::Transform>(playerEntity) ||
    ////             !g_coordinator->HasComponent<AABB>(playerEntity) ||
    ////             !g_coordinator->HasComponent<Health>(playerEntity))
    ////             continue;

    ////         auto& player_transform = g_coordinator->GetComponent<Framework::Transform>(playerEntity);
    ////         auto& player_box = g_coordinator->GetComponent<AABB>(playerEntity);
    ////         auto& player_health = g_coordinator->GetComponent<Health>(playerEntity);

    ////         AABB playerAABB = collision.BuildAABBFromTransform(player_transform, player_box);

    ////         float edgeThreshold = 2.0f; // sensitivity adjustment
    ////         bool edgeHit = collision.CheckEdgeContactCollision(playerAABB, enemyAABB, edgeThreshold);

    ////         if (edgeHit && enemy_health.isAlive && player_health.isAlive)
    ////         {
    ////             // Play sound from enemy's AudioSource component
    ////             if (g_coordinator->HasComponent<AudioSource>(ID)) {
    ////                 auto& audioSrc = g_coordinator->GetComponent<AudioSource>(ID);
    ////                 if (!audioSrc.audio_id.empty()) {
    ////                     if (auto audioManager = g_coordinator->GetSystem<AudioManager>()) {
    ////                         audioManager->PlaySound(audioSrc.audio_id);
    ////                     }
    ////                 }
    ////             }

    ////             enemy_health.isAlive = false;
    ////             player_health.hp -= enemy_health.damage;

    ////             std::cout << ">>> EDGE COLLISION DETECTED (AABB) <<<\n";
    ////             std::cout << "Player HP: " << player_health.hp << "\n";

    ////             if (player_health.hp <= 0)
    ////             {
    ////                 player_health.hp = 0;
    ////                 player_health.isAlive = false;
    ////                 std::cout << ">>> PLAYER DEFEATED\n";
    ////             }

    ////             Factory factory;
    ////             factory.DeleteEntity(ID);
    ////             return;  // Exit after collision
    ////         }
    ////     }

    ////     // If in AABB debug mode, don't run rhythm logic
    ////     return;
    //// }

    //// // ====================================================================
    //// // === MODE 2: OBB ROTATION CHASE & EDGE COLLISION (N) ===============
    //// // ====================================================================
    //// if (obbMode)
    //// {
    ////     static double last = glfwGetTime();
    ////     double now = glfwGetTime();
    ////     float dtOBB = static_cast<float>(now - last);
    ////     last = now;

    ////     // Spin enemy clockwise
    ////     enemy_transform.Rotation.x += 180.0f * dtOBB;
    ////     if (enemy_transform.Rotation.x >= 360.0f)
    ////         enemy_transform.Rotation.x -= 360.0f;

    ////     Entity firstPlayer = playerEntities[0];
    ////     auto& player_transform = g_coordinator->GetComponent<Framework::Transform>(firstPlayer);

    ////     // Move toward player
    ////     Vector2 dir = player_transform.Pos - enemy_transform.Pos;
    ////     float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    ////     if (len > 0.001f)
    ////     {
    ////         dir.x /= len;
    ////         dir.y /= len;
    ////         enemy_transform.Pos.x += dir.x * 100.0f * dtOBB;
    ////         enemy_transform.Pos.y += dir.y * 100.0f * dtOBB;
    ////     }

    ////     // loop through ALL players to check collision
    ////     for (Entity playerEntity : playerEntities)
    ////     {
    ////         if (!g_coordinator->HasComponent<Framework::Transform>(playerEntity) ||
    ////             !g_coordinator->HasComponent<AABB>(playerEntity) ||
    ////             !g_coordinator->HasComponent<Health>(playerEntity))
    ////             continue;

    ////         auto& current_player_transform = g_coordinator->GetComponent<Framework::Transform>(playerEntity);
    ////         auto& current_player_box = g_coordinator->GetComponent<AABB>(playerEntity);
    ////         auto& current_player_health = g_coordinator->GetComponent<Health>(playerEntity);

    ////         // --- Build OBBs & Check ---
    ////         OBB enemyOBB = collision.BuildOBBFromTransform(enemy_transform, enemy_box);
    ////         OBB playerOBB = collision.BuildOBBFromTransform(current_player_transform, current_player_box);

    ////         bool obbHit = collision.CheckOBBvsOBB(enemyOBB, playerOBB);

    ////         if (obbHit && enemy_health.isAlive && current_player_health.isAlive)
    ////         {
    ////             // Play sound from enemy's AudioSource component
    ////             if (g_coordinator->HasComponent<AudioSource>(ID)) {
    ////                 auto& audioSrc = g_coordinator->GetComponent<AudioSource>(ID);
    ////                 if (!audioSrc.audio_id.empty()) {
    ////                     if (auto audioManager = g_coordinator->GetSystem<AudioManager>()) {
    ////                         audioManager->PlaySound(audioSrc.audio_id);
    ////                     }
    ////                 }
    ////             }

    ////             enemy_health.isAlive = false;
    ////             current_player_health.hp -= enemy_health.damage;

    ////             std::cout << ">>> EDGE COLLISION DETECTED via OBB <<<\n";
    ////             std::cout << "Player HP: " << current_player_health.hp << "\n";

    ////             if (current_player_health.hp <= 0)
    ////             {
    ////                 current_player_health.hp = 0;
    ////                 current_player_health.isAlive = false;
    ////                 std::cout << ">>> PLAYER DEFEATED\n";
    ////             }

    ////             Factory factory;
    ////             factory.DeleteEntity(ID);
    ////             return;  // Exit after collision
    ////         }
    ////     }

    ////     // If in OBB debug mode, don't run rhythm logic
    ////     return;
    ////}

    //// // ====================================================================
    //// // === RHYTHM MODE (DEFAULT) ==========================================
    //// // ====================================================================
    //// // Only runs when BOTH aabbMode and obbMode are OFF.
    //// // There are 2 enemies using this script. Second enemy starts AFTER first dies.

    //// // ---- Grab first player (for direction) ----
    //// Entity mainPlayer = playerEntities[0];
    //// auto& main_player_transform = g_coordinator->GetComponent<Framework::Transform>(mainPlayer);
    //// auto& main_player_box = g_coordinator->GetComponent<AABB>(mainPlayer);
    //// auto& main_player_health = g_coordinator->GetComponent<Health>(mainPlayer);

    //// // ---- Per-enemy rhythm state (stored in a static map) ----
    //// struct RhythmState
    //// {
    ////     int   phase = 0;     // 0 = EnterFromRight, 1 = Combo_MoveLeft, 2 = PreAttack, 3 = FinalRush
    ////     float phaseTimer = 0.0f;
    ////     bool  started = false;
    //// };

    //// static std::unordered_map<Entity, RhythmState> s_rhythmStates;
    //// static Entity s_firstEnemyId = static_cast<Entity>(-1);
    //// static bool   s_firstEnemyCompleted = false;

    //// if (g_enemyRhythmNeedsReset) {
    ////     std::cout << "[EnemyScript] Resetting rhythm state after scene reload\n";
    ////     s_rhythmStates.clear();
    ////     s_firstEnemyId = static_cast<Entity>(-1);
    ////     g_enemyRhythmNeedsReset = false;
    //// }

    //// RhythmState& rs = s_rhythmStates[ID];

    //// // ------------------------------------------------------------
    //// // DETERMINE FIRST AND SECOND ENEMY BEHAVIOR
    //// // ------------------------------------------------------------
    //// if (!rs.started)
    //// {
    ////     // FIRST enemy logic
    ////     if (s_firstEnemyId == static_cast<Entity>(-1))
    ////     {
    ////         // This enemy becomes the FIRST enemy
    ////         s_firstEnemyId = ID;
    ////         rs.started = true;         // allow phase updates
    ////         rs.phase = 0;
    ////         rs.phaseTimer = 0.0f;
    ////     }
    ////     else
    ////     {
    ////         // This enemy is SECOND enemy.

    ////         // If first enemy NOT YET completed → second enemy MUST WAIT.
    ////         if (!s_firstEnemyCompleted)
    ////         {
    ////             // IMPORTANT FIX:
    ////             // Do NOT allow phaseTimer to increase, do NOT allow rs.started to become true.
    ////             // Enemy is completely frozen until first enemy dies.
    ////             return;
    ////         }

    ////         // First enemy is completed → now second enemy may begin
    ////         rs.started = true;
    ////         rs.phase = 0;
    ////         rs.phaseTimer = 0.0f;
    ////     }
    //// }


    //// // ---- Speeds and settings for rhythm behavior ----
    //// float slowSpeed = 330.0f;    // entering + during combo
    //// float mediumSpeed = 200.0f;   // pre-attack approach
    //// float fastSpeed = 300.0f;   // final rush
    //// /*float stopDistance = 150.0f;*/  // leave space before player during pre-attack

    //// // ---- Rhythm Phase Machine ----
    //// rs.phaseTimer += dt;

    //// switch (rs.phase)
    //// {
    ////     // =====================================================
    ////     // PHASE 1:
    ////     // Enter from right → move left for 2 seconds
    ////     // Then FREEZE for 4 seconds
    ////     // =====================================================
    //// case 0:
    //// {
    ////     if (rs.phaseTimer < 2.0f)
    ////     {
    ////         // Move left for the first 2 seconds
    ////         enemy_transform.Pos.x -= slowSpeed * dt;
    ////     }
    ////     else if (rs.phaseTimer < 6.0f)
    ////     {
    ////         // Freeze enemy for 4 seconds (no movement)
    ////         // do nothing
    ////     }
    ////     else
    ////     {
    ////         // After 6 seconds total → move to phase 1
    ////         rs.phase = 1;
    ////         rs.phaseTimer = 0.0f;
    ////     }
    ////     break;
    //// }

    //// // =====================================================
    //// // PHASE 2:
    //// // Combo window → move left for 4 seconds
    //// // Then FREEZE for 4 seconds
    //// // =====================================================
    //// case 1:
    //// {
    ////     if (rs.phaseTimer < 4.0f)
    ////     {
    ////         // Move left slowly during combo duration
    ////         enemy_transform.Pos.x -= mediumSpeed * dt;
    ////     }
    ////     else if (rs.phaseTimer < 8.0f)
    ////     {
    ////         // Freeze enemy for 4 seconds
    ////         // do nothing
    ////     }
    ////     else
    ////     {
    ////         // After 8 seconds → move to phase 2
    ////         rs.phase = 2;
    ////         rs.phaseTimer = 0.0f;
    ////     }
    ////     break;
    //// }

    //// // =====================================================
    //// // PHASE 3:
    //// // Final rush → full-speed toward player
    //// // AABB collision kills enemy
    //// // =====================================================
    //// case 2:
    //// {
    ////     Vector2 diff = main_player_transform.Pos - enemy_transform.Pos;
    ////     float dist = sqrtf(diff.x * diff.x + diff.y * diff.y);

    ////     if (dist > 0.001f)
    ////     {
    ////         diff.x /= dist;
    ////         diff.y /= dist;
    ////     }

    ////     enemy_transform.Pos.x += diff.x * fastSpeed * dt;
    ////     enemy_transform.Pos.y += diff.y * fastSpeed * dt;

    ////     // AABB collision check
    ////     AABB enemyAABB = collision.BuildAABBFromTransform(enemy_transform, enemy_box);
    ////     AABB playerAABB = collision.BuildAABBFromTransform(main_player_transform, main_player_box);

    ////     if (collision.CheckAABBCollision(enemyAABB, playerAABB))
    ////     {
    ////         // Play sound from enemy's AudioSource component
    ////         if (g_coordinator->HasComponent<AudioSource>(ID)) {
    ////             auto& audioSrc = g_coordinator->GetComponent<AudioSource>(ID);
    ////             if (!audioSrc.audio_id.empty()) {
    ////                 if (auto audioManager = g_coordinator->GetSystem<AudioManager>()) {
    ////                     audioManager->PlaySound(audioSrc.audio_id);
    ////                 }
    ////             }
    ////         }

    ////         enemy_health.isAlive = false;
    ////         main_player_health.hp -= enemy_health.damage;

    ////         if (main_player_health.hp <= 0)
    ////         {
    ////             main_player_health.hp = 0;
    ////             main_player_health.isAlive = false;
    ////         }

    ////         // Mark first enemy as completed
    ////         if (ID == s_firstEnemyId)
    ////             s_firstEnemyCompleted = true;

    ////         Factory factory;
    ////         factory.DeleteEntity(ID);
    ////         return;
    ////     }
    ////     break;
    //// }

    //// default:
    ////     break;
    //// }

    //// ======================================================================
    //// ===== ACTIVE CODE: ALWAYS-ON AABB COLLISION ==========================
    //// ======================================================================

    //// Build enemy AABB once
    //AABB enemyAABB = collision.BuildAABBFromTransform(enemy_transform, enemy_box);

    //// Check collision against ALL players
    //for (Entity playerEntity : playerEntities)
    //{
    //    if (!g_coordinator->HasComponent<Framework::Transform>(playerEntity) ||
    //        !g_coordinator->HasComponent<AABB>(playerEntity) ||
    //        !g_coordinator->HasComponent<Health>(playerEntity))
    //        continue;

    //    auto& player_transform = g_coordinator->GetComponent<Framework::Transform>(playerEntity);
    //    auto& player_box = g_coordinator->GetComponent<AABB>(playerEntity);
    //    auto& player_health = g_coordinator->GetComponent<Health>(playerEntity);

    //    AABB playerAABB = collision.BuildAABBFromTransform(player_transform, player_box);

    //    if (collision.CheckAABBCollision(enemyAABB, playerAABB))
    //    {
    //        // Play sound from enemy's AudioSource component (if any)
    //        if (g_coordinator->HasComponent<AudioSource>(ID)) {
    //            auto& audioSrc = g_coordinator->GetComponent<AudioSource>(ID);
    //            if (!audioSrc.audio_id.empty()) {
    //                if (auto audioManager = g_coordinator->GetSystem<AudioManager>()) {
    //                    audioManager->PlaySound(audioSrc.audio_id);
    //                }
    //            }
    //        }

    //        enemy_health.isAlive = false;
    //        player_health.hp -= enemy_health.damage;

    //        std::cout << ">>> AABB COLLISION: Enemy hit player\n";
    //        std::cout << "Player HP: " << player_health.hp << "\n";

    //        if (player_health.hp <= 0)
    //        {
    //            player_health.hp = 0;
    //            player_health.isAlive = false;
    //            std::cout << ">>> PLAYER DEFEATED\n";
    //        }

    //        Factory factory;
    //        factory.DeleteEntity(ID);
    //        return;
    //    }
    //}
}

void EnemyScript::Exit(Entity ID)
{
    (void)ID;
}

EnemyScript* EnemyScript::Clone()
{
    return new EnemyScript{};
}

//// =====================================================================
//// BUTTON SCRIPT
//// =====================================================================
//void ButtonScript::Init(Entity ID)
//{
//    auto* coord = Coordinator::GetInstance();
//
//    // Create button data for this entity
//    ButtonData& data = buttonDataMap[ID];
//
//    if (!coord->HasComponent<Name>(ID)) return;
//    auto& name = coord->GetComponent<Name>(ID);
//    data.buttonName = name.name;
//
//    // Get textures from Renderable
//    if (coord->HasComponent<Framework::Renderable>(ID)) {
//        auto& renderable = coord->GetComponent<Framework::Renderable>(ID);
//        data.normalTexture = renderable.spriteName;
//
//        // Construct hover texture name
//        size_t extPos = data.normalTexture.find_last_of('.');
//        if (extPos != std::string::npos) {
//            data.hoverTexture = data.normalTexture.substr(0, extPos) + "_hover" + data.normalTexture.substr(extPos);
//        }
//    }
//
//    /*std::cout << "[ButtonScript] Init: " << data.buttonName << " Normal: " << data.normalTexture  << " Hover: " << data.hoverTexture << std::endl;*/
//}
//
//void ButtonScript::Update(Entity ID)
//{
//    auto* coord = Coordinator::GetInstance();
//    auto inputMgr = coord->GetSystem<InputManager>();
//    if (!inputMgr) return;
//
//    // Get this button's data - if missing, initialize it
//    auto it = buttonDataMap.find(ID);
//    if (it == buttonDataMap.end()) {
//        /*std::cout << "[ButtonScript] Re-initializing button entity " << ID << std::endl;*/
//        Init(ID);  // Re-initialize if missing
//        it = buttonDataMap.find(ID);
//        if (it == buttonDataMap.end()) return; // Still missing, bail out
//    }
//    ButtonData& data = it->second;
//
//    if (!coord->HasComponent<Framework::Transform>(ID)) return;
//    if (!coord->HasComponent<Framework::Renderable>(ID)) return;
//
//    auto& transform = coord->GetComponent<Framework::Transform>(ID);
//    auto& renderable = coord->GetComponent<Framework::Renderable>(ID);
//    if (!transform.isVisible) return;
//
//    // Get mouse position
//    glm::vec2 mouseVirtual = inputMgr->getMousePositionVirtual();
//    Vector2 mousePos{ mouseVirtual.x, mouseVirtual.y };
//
//    // Calculate AABB directly (DON'T use BuildAABBFromTransform)
//    AABB buttonAABB;
//    buttonAABB.width = transform.Scale.x;
//    buttonAABB.height = transform.Scale.y;
//    buttonAABB.min.x = transform.Pos.x - (buttonAABB.width * 0.5f);
//    buttonAABB.max.x = transform.Pos.x + (buttonAABB.width * 0.5f);
//    buttonAABB.min.y = transform.Pos.y - (buttonAABB.height * 0.5f);
//    buttonAABB.max.y = transform.Pos.y + (buttonAABB.height * 0.5f);
//
//    // DEBUG - print every second for NEW GAME button
//    //static int debugFrameCount = 0;
//    //debugFrameCount++;
//    //if (debugFrameCount % 60 == 0) {
//    //    std::cout << "\n===== " << data.buttonName << " =====" << std::endl;
//    //    std::cout << "Transform.Pos: (" << transform.Pos.x << ", " << transform.Pos.y << ")" << std::endl;
//    //    std::cout << "Transform.Scale: (" << transform.Scale.x << ", " << transform.Scale.y << ")" << std::endl;
//    //    std::cout << "AABB: [" << buttonAABB.min.x << " to " << buttonAABB.max.x
//    //        << ", " << buttonAABB.min.y << " to " << buttonAABB.max.y << "]" << std::endl;
//    //    std::cout << "Mouse: (" << mousePos.x << ", " << mousePos.y << ")" << std::endl;
//    //    std::cout << "========================\n" << std::endl;
//    //}
//
//    // Check if mouse is in bounds
//    bool wasHovering = data.isHovering;
//    bool inX = (mousePos.x >= buttonAABB.min.x && mousePos.x <= buttonAABB.max.x);
//    bool inY = (mousePos.y >= buttonAABB.min.y && mousePos.y <= buttonAABB.max.y);
//    // UI buttons work without viewport check (fixes Level1 camera issue)
//    data.isHovering = inX && inY;
//
//    // Handle texture swap
//    if (data.isHovering && !wasHovering) {
//        /*std::cout << "[ButtonScript] Trying to load hover texture: " << data.hoverTexture << std::endl;*/
//        renderable.spriteName = data.hoverTexture;
//        renderable.needsTextureReload = true;
//        /*std::cout << "[ButtonScript] Hover: " << data.buttonName << std::endl;*/
//    }
//    else if (!data.isHovering && wasHovering) {
//        renderable.spriteName = data.normalTexture;
//        renderable.needsTextureReload = true;
//    }
//
//    // Handle click - track button press state per entity
//    static std::unordered_map<Entity, bool> wasClickedLastFrame;
//    bool isClickedThisFrame = data.isHovering && inputMgr->isMouseButtonPressed(0);
//    bool wasClickedPreviously = wasClickedLastFrame[ID];
//
//    // Trigger action on button RELEASE while still hovering (prevents double-triggers)
//    if (data.isHovering && wasClickedPreviously && !isClickedThisFrame) {
//        /*std::cout << "[ButtonScript] Clicked: " << data.buttonName << std::endl;*/
//
//        auto sceneMgr = coord->GetSystem<SceneManager>();
//        if (!sceneMgr) return;
//
//        if (data.buttonName == "NewGameBtn") {
//            sceneMgr->CurrentScene = "Level1";
//        }
//        else if (data.buttonName == "ContinueBtn") {
//            std::cout << "[ButtonScript] Continue not implemented yet" << std::endl;
//        }
//        else if (data.buttonName == "SettingsBtn") {
//            std::cout << "[ButtonScript] Settings not implemented yet" << std::endl;
//        }
//        else if (data.buttonName == "BackBtn") {
//            sceneMgr->CurrentScene = "MainMenu";
//        }
//        else if (data.buttonName == "RestartBtn") {
//            sceneMgr->CurrentScene = "MainMenu";
//        }
//        else if (data.buttonName == "QuitBtn") {
//            if (Engine) {
//                glfwSetWindowShouldClose(Engine->GetWindow(), GLFW_TRUE);
//            }
//        }
//    }
//
//    wasClickedLastFrame[ID] = isClickedThisFrame;
//}
//
//void ButtonScript::Exit(Entity ID)
//{
//    // Clean up button data for this entity
//    buttonDataMap.erase(ID);
//}
//
//ButtonScript* ButtonScript::Clone()
//{
//    return new ButtonScript{};
//}
//
//// =====================================================================
//// PULSING BORDER SCRIPT
//// =====================================================================
//void PulsingBorderScript::Init(Entity ID)
//{
//    auto* g_coordinator = Coordinator::GetInstance();
//
//    pulseTimer = static_cast<float>(glfwGetTime());  // Store START time
//
//    if (g_coordinator->HasComponent<Framework::Transform>(ID))
//    {
//        auto& transform = g_coordinator->GetComponent<Framework::Transform>(ID);
//        transform.isVisible = false;
//    }
//}
//
//void PulsingBorderScript::Update(Entity ID)
//{
//    auto* g_coordinator = Coordinator::GetInstance();
//
//    float beatInterval = 0.5f;
//    double timeSource = 0.0;
//
//    if (!g_countdownFinished)
//    {
//        timeSource = glfwGetTime() - pulseTimer;
//    }
//    else
//    {
//        auto audioSystem = g_coordinator->GetSystem<AudioManager>();
//        if (!audioSystem)
//            return;
//
//        timeSource = audioSystem->GetBGMPositionSeconds();
//
//        // Only skip if BGM not started yet (negative), 0.0 is valid first beat
//        if (timeSource < 0.0)
//            return;
//    }
//
//    float normalizedTime = fmodf((float)timeSource, beatInterval) / beatInterval;
//    float beatWindow = 0.15f;
//
//    bool shouldBeVisible = (normalizedTime < beatWindow || normalizedTime >(1.0f - beatWindow));
//
//    if (g_coordinator->HasComponent<Framework::Transform>(ID))
//    {
//        auto& transform = g_coordinator->GetComponent<Framework::Transform>(ID);
//
//        bool wasVisible = transform.isVisible;
//        if (shouldBeVisible && !wasVisible)
//        {
//            if (g_coordinator->HasComponent<AudioSource>(ID))
//            {
//                auto audioSystem = g_coordinator->GetSystem<AudioManager>();
//                auto& audioSrc = g_coordinator->GetComponent<AudioSource>(ID);
//                if (audioSystem && !audioSrc.audio_id.empty())
//                {
//                    audioSystem->PlayFromAudioSource(audioSrc);
//                }
//            }
//        }
//
//        transform.isVisible = shouldBeVisible;
//    }
//}
//
//void PulsingBorderScript::Exit(Entity ID)
//{
//    (void)ID;
//    //std::cout << "[PulsingBorderScript] Shutdown for entity " << ID << std::endl;
//}
//
//
//PulsingBorderScript* PulsingBorderScript::Clone()
//{
//    return new PulsingBorderScript{};
//}


//// =====================================================================
//// COUNTDOWN SCRIPT
//// =====================================================================
//void CountdownScript::Init(Entity ID)
//{
//    (void)ID;
//    countdownTimer = static_cast<float>(glfwGetTime());  // Store START time
//    g_countdownFinished = false;
//}
//
//void CountdownScript::Update(Entity ID)
//{
//    auto* coord = Coordinator::GetInstance();
//
//    if (!coord->HasComponent<TextComponent>(ID))
//        return;
//
//    auto& text = coord->GetComponent<TextComponent>(ID);
//
//    if (g_countdownFinished)
//    {
//        text.visible = false;
//        return;
//    }
//
//    float elapsed = static_cast<float>(glfwGetTime()) - countdownTimer;
//
//    if (elapsed < 1.0f)
//    {
//        text.text = "Ready?";
//    }
//    else if (elapsed < 2.0f)
//    {
//        text.text = "3";
//    }
//    else if (elapsed < 3.0f)
//    {
//        text.text = "2";
//    }
//    else if (elapsed < 4.0f)
//    {
//        text.text = "1";
//    }
//    else if (elapsed < 5.0f)
//    {
//        text.text = "GO!";
//    }
//    else
//    {
//        g_countdownFinished = true;
//        text.visible = false;
//
//        if (coord->HasComponent<AudioSource>(ID))
//        {
//            auto& audioSrc = coord->GetComponent<AudioSource>(ID);
//            auto audioManager = coord->GetSystem<AudioManager>();
//            if (audioManager && !audioSrc.audio_id.empty())
//            {
//                audioManager->PlayFromAudioSource(audioSrc);
//            }
//        }
//    }
//}
//
//void CountdownScript::Exit(Entity ID)
//{
//    (void)ID;
//    g_countdownFinished = false;
//}
//
//CountdownScript* CountdownScript::Clone() { return new CountdownScript{}; }
//
//// =====================================================================
//// FEEDBACK SCRIPT (PERFECT/GOOD/MISS images)
//// =====================================================================
//void FeedbackScript::Init(Entity ID)
//{
//    auto* coord = Coordinator::GetInstance();
//
//    // Hide initially
//    if (coord->HasComponent<Framework::Transform>(ID))
//    {
//        auto& transform = coord->GetComponent<Framework::Transform>(ID);
//        transform.isVisible = false;
//    }
//
//    // Register listener for combo events
//    auto rhythm = coord->GetSystem<RhythmGameplayInput>();
//    if (!rhythm) return;
//
//    rhythm->registerInputListener("FeedbackDisplay",
//        [this, ID](const InputEvent& evt)
//        {
//            if (evt.type != InputEvent::Type::COMBO_EXECUTED)
//                return;
//
//            auto* c = Coordinator::GetInstance();
//
//            if (!c->HasComponent<Framework::Renderable>(ID) ||
//                !c->HasComponent<Framework::Transform>(ID))
//                return;
//
//            auto& rend = c->GetComponent<Framework::Renderable>(ID);
//            auto& transform = c->GetComponent<Framework::Transform>(ID);
//
//            // Change texture based on feedback
//            if (evt.feedback == "PERFECT")
//            {
//                rend.spriteName = "../../PulseEngine/Assets/Textures/UI/perfect.png";
//            }
//            else if (evt.feedback == "GOOD")
//            {
//                rend.spriteName = "../../PulseEngine/Assets/Textures/UI/great.png";
//            }
//            else // MISS
//            {
//                rend.spriteName = "../../PulseEngine/Assets/Textures/UI/miss.png";
//            }
//
//            rend.needsTextureReload = true;
//            transform.isVisible = true;
//            isShowing = true;
//            startTime = static_cast<float>(glfwGetTime());
//            displayTimer = 0.5f;  // Show for 0.5 seconds
//        });
//}
//
//void FeedbackScript::Update(Entity ID)
//{
//    if (!isShowing)
//        return;
//
//    auto* coord = Coordinator::GetInstance();
//
//    float elapsed = static_cast<float>(glfwGetTime()) - startTime;
//
//    if (elapsed >= displayTimer)
//    {
//        if (coord->HasComponent<Framework::Transform>(ID))
//        {
//            auto& transform = coord->GetComponent<Framework::Transform>(ID);
//            transform.isVisible = false;
//        }
//        isShowing = false;
//    }
//}
//
//void FeedbackScript::Exit(Entity ID)
//{
//    (void)ID;
//}
//
//FeedbackScript* FeedbackScript::Clone() { return new FeedbackScript{}; }