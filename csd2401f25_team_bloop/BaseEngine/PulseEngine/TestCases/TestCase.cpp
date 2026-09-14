///*****************************************************************************/
///**
// * @file        TestCase.cpp
// * @project     Pulse Protocol
// * @author      Reginald Lew Yee Ren
// * @brief       Implements game logic and test case behavior using CollisionManager.
// *              Includes Hero–Monster collision (with offsets) and mouse hover/click.
// */
// /*****************************************************************************/
//
//#include "pch/pch.h"
//#include "TestCase.h"
//
//// --- Toggle for console debugging ---
//bool EnableTestCases = true;
//
//// ==================================================================================
//// Initialization
//// ==================================================================================
//void TestCase::Init()
//{
//    auto* g_coordinator = Coordinator::GetInstance();
//    auto inputMgr = g_coordinator->GetSystem<InputManager>();
//    auto rhythmSystem = g_coordinator->GetSystem<RhythmGameplayInput>();
//
//    if (!inputMgr)
//    {
//        std::cout << "[ERROR] InputSystem is NULL!\n";
//        return;
//    }
//
//    // --- Background ---
//    Entity Background = g_coordinator->CreateEntity();
//    Framework::Transform bgTransform{};
//    Framework::Renderable bgRenderable{};
//    Framework::Animation bgAnimation{};
//    bgTransform.Pos = { 0, 0 };
//    bgTransform.Scale = { 1600.0f, 900.0f };
//    bgRenderable.mdl_ref = 0.0f;
//    bgRenderable.shd_ref = 1.0f;
//    bgTransform.isVisible = true;
//    bgAnimation.totalFrames = 1;
//    bgAnimation.columns = 1;
//    bgAnimation.rows = 1;
//    bgRenderable.spriteName = "../../PulseEngine/Assets/Textures/Backgrounds/Background1.jpg";
//    g_coordinator->AddComponent(Background, bgTransform);
//    g_coordinator->AddComponent(Background, bgRenderable);
//    g_coordinator->AddComponent(Background, bgAnimation);
//
//    // --- Hero ---
//    Entity Hero = g_coordinator->CreateEntity();
//    Framework::Transform heroTransform{};
//    Framework::Renderable heroRenderable{};
//    Framework::Animation heroAnimation{};
//    heroTransform.Pos = { 0, 0 };
//    heroTransform.Scale = { 200.0f, 200.0f };
//    heroRenderable.mdl_ref = 1.0f;
//    heroRenderable.shd_ref = 2.0f;
//    heroTransform.isVisible = true;
//    heroAnimation.totalFrames = 3;
//    heroAnimation.columns = 3;
//    heroAnimation.rows = 1;
//    heroRenderable.spriteName = "../../PulseEngine/Assets/Textures/Characters/hero_spritesheet.png";
//    g_coordinator->AddComponent(Hero, heroTransform);
//    g_coordinator->AddComponent(Hero, heroRenderable);
//    g_coordinator->AddComponent(Hero, heroAnimation);
//
//    g_coordinator->AddComponent(Hero, NPC{
//        100.0f, 100.0f,
//        200.0f, 100.0f,
//        0.0f, false, 10, true
//        });
//
//    g_coordinator->AddComponent(Hero, collision.BuildHeroHitbox(heroTransform, g_coordinator->GetComponent<NPC>(Hero)));
//    EntityNPC.push_back(Hero);
//    std::cout << "Hero spawned at center.\n";
//
//    if (inputMgr)
//        inputMgr->DebugEntities.insert(Hero);
//
//    // --- Monsters ---
//    float startPositions[2] = { -500.0f, 500.0f };
//    for (int i = 0; i < 2; ++i)
//    {
//        Entity Monster = g_coordinator->CreateEntity();
//        Framework::Transform MonsterTransform{};
//        Framework::Renderable MonsterRenderable{};
//        Framework::Animation MonsterAnimation{};
//        MonsterTransform.Pos = { startPositions[i], 0 };
//        MonsterTransform.Scale = { 200.0f, 200.0f };
//        MonsterRenderable.mdl_ref = 1.0f;
//        MonsterRenderable.shd_ref = 2.0f;
//        MonsterTransform.isVisible = true;
//        MonsterAnimation.totalFrames = 1;
//        MonsterAnimation.columns = 1;
//        MonsterAnimation.rows = 1;
//        MonsterRenderable.spriteName = "../../PulseEngine/Assets/Textures/Enemies/BlueGorillaEnemy.png";
//        g_coordinator->AddComponent(Monster, MonsterTransform);
//        g_coordinator->AddComponent(Monster, MonsterRenderable);
//        g_coordinator->AddComponent(Monster, MonsterAnimation);
//
//        g_coordinator->AddComponent(Monster, NPC{
//            100.0f, 100.0f,
//            200.0f, 100.0f,
//            0.0f, false, 10, true
//            });
//
//        g_coordinator->AddComponent(Monster, collision.BuildMonsterHitbox(MonsterTransform, g_coordinator->GetComponent<NPC>(Monster)));
//        EntityNPC.push_back(Monster);
//
//        std::cout << "[Init] Monster spawned at x=" << startPositions[i] << "\n";
//    }
//
//    std::cout << "============================================\n\n";
//
//    LoadGUIButtons();
//
//    // --- UI entity for rhythm system (hidden bar) ---
//    Entity inputEntity = g_coordinator->CreateEntity();
//    Framework::Transform inputObj{};
//    Framework::Renderable inputR{};
//    Framework::Animation inputA{};
//    inputObj.Pos = { 0.0f, -500.0f };
//    inputObj.Scale = { 800.0f, 200.0f };
//    inputR.mdl_ref = 1.0f;
//    inputR.shd_ref = 2.0f;
//    inputObj.isVisible = false;
//    inputR.spriteName = "../../PulseEngine/Assets/Textures/UI/bar.png";
//    g_coordinator->AddComponent(inputEntity, inputObj);
//    g_coordinator->AddComponent(inputEntity, inputR);
//    g_coordinator->AddComponent(inputEntity, inputA);
//
//    if (rhythmSystem)
//        rhythmSystem->setFontEntity(inputEntity);
//
//    std::cout << "[DEBUG] Entity count = " << inputMgr->EntityMember.size() << "\n";
//}
//
//// ==================================================================================
//// Update
//// ==================================================================================
//void TestCase::Update(float dt)
//{
//    auto* g_coordinator = Coordinator::GetInstance();
//
//    // --- Movement toggle ---
//    if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_M) == GLFW_PRESS)
//        monsterCanMove = true;
//
//    // --- Hero Scale Controls ---
//    static bool kPressed = false, lPressed = false;
//    auto& hero_transform = g_coordinator->GetComponent<Framework::Transform>(EntityNPC[0]);
//    if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_K) == GLFW_PRESS && !kPressed)
//    {
//        hero_transform.Scale *= 1.1f;
//        std::cout << "[Hero] Scaled Up to " << hero_transform.Scale.x << "\n";
//        kPressed = true;
//    }
//    else if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_K) == GLFW_RELEASE)
//        kPressed = false;
//
//    if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_L) == GLFW_PRESS && !lPressed)
//    {
//        hero_transform.Scale *= 0.9f;
//        std::cout << "[Hero] Scaled Down to " << hero_transform.Scale.x << "\n";
//        lPressed = true;
//    }
//    else if (glfwGetKey(Engine->GetWindow(), GLFW_KEY_L) == GLFW_RELEASE)
//        lPressed = false;
//
//    // --- Rotation (both monsters simultaneously) ---
//    if (EntityNPC.size() > 1)
//    {
//        bool rotateLeft = glfwGetKey(Engine->GetWindow(), GLFW_KEY_J) == GLFW_PRESS;
//        bool rotateRight = glfwGetKey(Engine->GetWindow(), GLFW_KEY_H) == GLFW_PRESS;
//
//        if (rotateLeft || rotateRight)
//        {
//            float rotationAmount = 20.0f * dt * (rotateRight ? 1.0f : -1.0f);
//
//            for (size_t i = 1; i < EntityNPC.size(); ++i)
//            {
//                auto& monster_transform = g_coordinator->GetComponent<Framework::Transform>(EntityNPC[i]);
//                monster_transform.angle_disp += rotationAmount;
//            }
//
//            if (EnableTestCases)
//            {
//                if (rotateRight)
//                    std::cout << "[Rotate] Monsters rotated clockwise.\n";
//                else
//                    std::cout << "[Rotate] Monsters rotated counter-clockwise.\n";
//            }
//        }
//    }
//
//    // --- Hero–Monster Collision ---
//    auto& hero_npc = g_coordinator->GetComponent<NPC>(EntityNPC[0]);
//    auto& hero_pos = g_coordinator->GetComponent<Framework::Transform>(EntityNPC[0]);
//
//    for (size_t i = 1; i < EntityNPC.size(); ++i)
//    {
//        auto& monster_npc = g_coordinator->GetComponent<NPC>(EntityNPC[i]);
//        auto& monster_pos = g_coordinator->GetComponent<Framework::Transform>(EntityNPC[i]);
//        if (!monster_npc.isAlive || !hero_npc.isAlive) continue;
//
//        AABB heroBox = collision.BuildHeroHitbox(hero_pos, hero_npc);
//        AABB monsterBox = collision.BuildMonsterHitbox(monster_pos, monster_npc);
//
//        if (monsterCanMove)
//        {
//            float speed = (monster_pos.Pos.x < 0) ? 100.0f : -100.0f;
//            monster_pos.Pos.x += speed * dt;
//        }
//
//        float HeroOffset = hero_npc.width / 2.0f;
//        float MonsterOffset = monster_npc.width / 2.0f;
//
//        if ((heroBox.max.x + HeroOffset >= monsterBox.min.x - MonsterOffset &&
//            heroBox.min.x - HeroOffset <= monsterBox.max.x + MonsterOffset) &&
//            (heroBox.max.y >= monsterBox.min.y && heroBox.min.y <= monsterBox.max.y))
//        {
//            hero_npc.hp -= 1;
//
//            if (EnableTestCases)
//                std::cout << std::setw(12) << "[Collision] "
//                << "Monster collided with Hero! Hero HP: " << hero_npc.hp << "\n";
//
//            g_coordinator->DestroyEntity(EntityNPC[i]);
//            EntityNPC.erase(EntityNPC.begin() + i);
//            --i;
//
//            if (hero_npc.hp <= 0)
//            {
//                hero_npc.isAlive = false;
//                if (EnableTestCases)
//                    std::cout << std::setw(12) << "[Hero] Hero has died!\n";
//            }
//        }
//    }
//
//    // --- Mouse Click + Hero Collision Test ---
//    static bool leftPressed = false;
//    static bool rightPressed = false;
//
//    int leftState = glfwGetMouseButton(Engine->GetWindow(), GLFW_MOUSE_BUTTON_LEFT);
//    int rightState = glfwGetMouseButton(Engine->GetWindow(), GLFW_MOUSE_BUTTON_RIGHT);
//
//    // Always read mouse and hero position
//    glm::vec2 mousePosVirtual = g_coordinator->GetSystem<InputManager>()->getMousePositionVirtual();
//    int winW, winH;
//    glfwGetWindowSize(Engine->GetWindow(), &winW, &winH);
//
//    // Convert to world-space (-800..800, -450..450)
//    float worldX = ((mousePosVirtual.x / winW) - 0.5f) * 1600.0f;
//    float worldY = ((1.0f - (mousePosVirtual.y / winH)) - 0.5f) * 900.0f;
//    Vector2 mousePos(worldX, worldY);
//
//    // Ensure hero exists
//    if (!EntityNPC.empty())
//    {
//        auto& heroPos = g_coordinator->GetComponent<Framework::Transform>(EntityNPC[0]);
//        auto& heroNPC = g_coordinator->GetComponent<NPC>(EntityNPC[0]);
//        AABB heroBox = collision.BuildHeroHitbox(heroPos, heroNPC);
//
//        // --- Left Click ---
//        if (leftState == GLFW_PRESS && !leftPressed)
//        {
//            std::cout << "[Mouse] Left Click at World (" << worldX << ", " << worldY << ")\n";
//            std::cout << "[Hero] Pos(" << heroPos.Pos.x << ", " << heroPos.Pos.y << ")\n";
//            std::cout << "[Hero Box] Min(" << heroBox.min.x << ", " << heroBox.min.y
//                << ")  Max(" << heroBox.max.x << ", " << heroBox.max.y << ")\n";
//
//            if (collision.CheckMouseCollision(mousePos, heroBox))
//                std::cout << "[Collision] Hero clicked!\n";
//            else
//                std::cout << "[Miss] Clicked outside hero box.\n";
//
//            leftPressed = true;
//        }
//        else if (leftState == GLFW_RELEASE)
//        {
//            leftPressed = false;
//        }
//
//        // --- Right Click ---
//        if (rightState == GLFW_PRESS && !rightPressed)
//        {
//            std::cout << "[Mouse] Right Click at World (" << worldX << ", " << worldY << ")\n";
//            std::cout << "[Hero] Pos(" << heroPos.Pos.x << ", " << heroPos.Pos.y << ")\n";
//            std::cout << "[Hero Box] Min(" << heroBox.min.x << ", " << heroBox.min.y
//                << ")  Max(" << heroBox.max.x << ", " << heroBox.max.y << ")\n";
//
//            if (collision.CheckMouseCollision(mousePos, heroBox))
//                std::cout << "[Collision] Hero right-clicked!\n";
//            else
//                std::cout << "[Miss] Right-clicked outside hero box.\n";
//
//            rightPressed = true;
//        }
//        else if (rightState == GLFW_RELEASE)
//        {
//            rightPressed = false;
//        }
//    }
//
//}
//
//// ==================================================================================
//// Load GUI Buttons from JSON
//// ==================================================================================
//void TestCase::LoadGUIButtons() {
//    auto* g_coordinator = Coordinator::GetInstance();
//    auto inputMgr = g_coordinator->GetSystem<InputManager>();
//
//    std::ifstream ifs("../../PulseEngine/JSON/GUIButtons.json");
//    if (!ifs.is_open()) {
//        std::cerr << "[TestCase] Cannot open GUIButtons.json - trying alternate path\n";
//        ifs.open("../../PulseEngine/JSON/GuiButtons.json");
//        if (!ifs.is_open()) {
//            std::cerr << "[TestCase] Failed to open GUI buttons JSON file\n";
//            return;
//        }
//    }
//
//    rapidjson::IStreamWrapper isw(ifs);
//    rapidjson::Document d;
//    d.ParseStream(isw);
//
//    if (d.HasParseError()) {
//        std::cerr << "[TestCase] JSON parse error\n";
//        return;
//    }
//
//    if (!d.HasMember("buttons") || !d["buttons"].IsArray()) {
//        std::cerr << "[TestCase] No buttons array\n";
//        return;
//    }
//
//    const auto& buttonsArray = d["buttons"];
//    float verticalSpacing = 100.0f;
//    float startY = 350.0f;
//
//    for (rapidjson::SizeType i = 0; i < buttonsArray.Size(); ++i) {
//        const auto& buttonJson = buttonsArray[i];
//
//        Entity buttonEntity = g_coordinator->CreateEntity();
//
//        // Transform Component
//        Framework::Transform transform;
//        transform.Pos.y = startY - (i * verticalSpacing);
//        transform.Pos.x = -600.0f;  // Left side of screen (world coords)
//        transform.Scale = { 200.0f, 50.0f };
//        transform.isVisible = true;
//        g_coordinator->AddComponent(buttonEntity, transform);
//
//        //// Renderable Component
//        //Framework::Renderable renderable;
//        //renderable.mdl_ref = 1.0f;
//        //renderable.shd_ref = 2.0f;  // test shader (solid color)
//        //renderable.spriteName = "";
//        //g_coordinator->AddComponent(buttonEntity, renderable);
//
//        // Animation Component
//        Framework::Animation animation;
//        animation.totalFrames = 1;
//        animation.columns = 1;
//        animation.rows = 1;
//        g_coordinator->AddComponent(buttonEntity, animation);
//
//        // GUIButton Component
//        GUIButton button;
//        button.Deserialize(buttonJson);
//        g_coordinator->AddComponent(buttonEntity, button);
//
//        // Register button with InputManager for input handling
//        if (inputMgr)
//            inputMgr->GUIButtonEntities.insert(buttonEntity);
//
//        guiButtonEntities.push_back(buttonEntity);
//
//        std::cout << "[TestCase] Created GUI button: " << button.buttonID
//            << " at (" << transform.Pos.x << ", " << transform.Pos.y << ")\n";
//    }
//
//    std::cout << "[TestCase] Loaded " << guiButtonEntities.size() << " GUI buttons\n";
//}