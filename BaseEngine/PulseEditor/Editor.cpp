/******************************************************************************/
/**
 * @file        Editor.cpp
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 70%
 * @author      Goh Pin Kai (secondary) - 15%
 *              Chloe Lau Rey En (secondary) - 15%
 * @brief       Manages the full ImGui-based editor interface, including initialization, frame updates, rendering, and runtime toggling of editor features.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "Editor.h"
#include "ProjectPanel.h"
#include "Graphics/glhelper.h"
#include "CoreEngine/Asset/AssetsManager.h"
#include "Input/InputManager.h"
#include "EditorTypes.h"
//Vector2 v2{};
//    OAConfig cfg(
//        false,                     // UseCPPMemManager_
//        64,            // ObjectsPerPage_
//        0,                  // MaxPages_ (0 = unlimited)
//        false,                   // DebugOn_
//        0,                         // PadBytes_
//        0,                         // HeaderBlocks_
//        alignof(std::max_align_t)  // Alignment_
//    );
//
//   ObjectAllocator oa(sizeof(Benchmark::Student), cfg);


// ==========================================================================
//Benchmark::OABenchmarkResult test;

namespace PulseEditor
{


    // --------------------------------------------------------------------
    // CONSTANTS & BASIC STATE
    // --------------------------------------------------------------------


   // selectedEntity 
    // The key idea:
    // - The entity ID changes when you undo/redo a deletion. (since new obj)
    // - So every action must refer to a *shared handle* that always stores
    //   the CURRENT live entity id for that "conceptual object".
    UndoManager mUndo;
   // const Entity INVALID_ENTITY = static_cast<Entity>(-1);
    Entity selectedEntity = INVALID_ENTITY;

    std::shared_ptr<EntityHandle> selectedHandle = nullptr;
    std::vector<Entity> entityDisplayOrder{};
    std::unordered_map<Entity, std::shared_ptr<EntityHandle>> handleOf{};
    Factory Editorfactory;

// keep synced every frame / on selection change

    // this vector maintains the order entities appear in the hierarchy
    static GLFWwindow* s_Window = nullptr;
    static bool s_restoreNextFrame = false;
    static bool s_prevKeys[GLFW_KEY_LAST + 1] = { false };

    static bool s_frameStarted = false;
    static bool s_playing = false;


    // --------------------------------------------------------------------
    // PANEL RATIOS
    // --------------------------------------------------------------------
    static float s_top_ratio = 0.05f;     // ~5% of screen height for Play/Pause bar
    static float s_bottom_ratio = 0.30f;  // bottom assets/debug/profiler
    static float s_left_ratio = 0.20f;    // scene/entity panel
    static float s_right_ratio = 0.15f;   // inspector panel

    static constexpr float SPLIT_THICK = 6.0f;
    static constexpr float MIN_RATIO_W = 0.12f;   // min 12% for left/right widths
    static constexpr float MIN_RATIO_H = 0.05f;   // min 5% for top/bottom heights
    static constexpr float MAX_TOTAL_SIDE = 0.85f;
    static constexpr float MAX_TOTAL_VERT = 0.85f;


    bool PulseEditor::IsPlaying() { return s_playing; }
    void PulseEditor::SetPlaying(bool p)
    {
        s_playing = p;

        // When switching modes, tell GLApp to reset the cameras if needed.
        if (p == true)
        {
            // Entering Play Mode
            GLApp::gEditorCamera = GLApp::Camera2D{}; // freeze editor camera where it was
        }
    }


#ifdef IMGUI_HAS_DOCKING
    static ImGuiID   g_DockspaceID = 0;
    static ImGuiID   g_NodeLeft = 0, g_NodeRight = 0, g_NodeBottom = 0, g_NodeCenter = 0;
    static bool      g_DockLayoutBuilt = false;

    enum class DockTarget { Left, Right, Bottom, Center };
#endif


    // --------------------------------------------------------------------
    // UTILITY
    // --------------------------------------------------------------------
    template<typename T>
    static T clamp_val(T v, T lo, T hi)
    {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }


    // helper to draw a 1px line
    static void DrawGuideLine(ImVec2 a, ImVec2 b) {
        ImGui::GetForegroundDrawList()->AddLine(
            a, b,
            IM_COL32(180, 180, 180, 160),
            1.0f
        );
    }

    // Draw Gizmo
    void Gizmo_DrawAndHandle(const GameViewport& vp);



#ifdef IMGUI_HAS_DOCKING
    static void BuildMainDockspace()
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("DockSpaceHost", nullptr, flags);
        ImGui::PopStyleVar(2);

        g_DockspaceID = ImGui::GetID("PulseEditorDockSpace");
        ImGui::DockSpace(g_DockspaceID, ImVec2(0, 0), ImGuiDockNodeFlags_None);

        ImGui::End();
    }

    static void EnsureDefaultDockLayout()
    {
        if (g_DockLayoutBuilt || g_DockspaceID == 0) return;

        ImGui::DockBuilderRemoveNode(g_DockspaceID);
        ImGui::DockBuilderAddNode(g_DockspaceID, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(g_DockspaceID, ImGui::GetMainViewport()->WorkSize);

        g_NodeCenter = g_DockspaceID;
        g_NodeLeft = ImGui::DockBuilderSplitNode(g_NodeCenter, ImGuiDir_Left, 0.20f, nullptr, &g_NodeCenter);
        g_NodeRight = ImGui::DockBuilderSplitNode(g_NodeCenter, ImGuiDir_Right, 0.20f, nullptr, &g_NodeCenter);
        g_NodeBottom = ImGui::DockBuilderSplitNode(g_NodeCenter, ImGuiDir_Down, 0.25f, nullptr, &g_NodeCenter);

        // Place your known windows by name (must match ImGui::Begin titles)
        ImGui::DockBuilderDockWindow("Scene Hierarchy", g_NodeLeft);
        ImGui::DockBuilderDockWindow("Inspector", g_NodeRight);
        ImGui::DockBuilderDockWindow("Bottom", g_NodeBottom);
        ImGui::DockBuilderDockWindow("TopBar", g_NodeCenter);

        ImGui::DockBuilderFinish(g_DockspaceID);
        g_DockLayoutBuilt = true;
    }

    static ImGuiID GetTargetNodeID(DockTarget t)
    {
        switch (t)
        {
        case DockTarget::Left:   return g_NodeLeft;
        case DockTarget::Right:  return g_NodeRight;
        case DockTarget::Bottom: return g_NodeBottom;
        default:                 return g_NodeCenter;
        }
    }

    static void Dock_MoveWindowTo(const char* window_name, DockTarget target)
    {
        if (g_DockspaceID == 0) return;
        EnsureDefaultDockLayout();
        ImGuiID node = GetTargetNodeID(target);
        if (node == 0) node = g_DockspaceID;
        ImGui::DockBuilderDockWindow(window_name, node);
        ImGui::DockBuilderFinish(g_DockspaceID);
    }

    static void Dock_FloatWindow(const char* window_name, ImVec2 size = ImVec2(640, 360))
    {
        ImGuiID float_node = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_None);
        ImGui::DockBuilderSetNodeSize(float_node, size);
        ImVec2 mouse = ImGui::GetMousePos();
        ImGui::DockBuilderSetNodePos(float_node, mouse);
        ImGui::DockBuilderDockWindow(window_name, float_node);
        ImGui::DockBuilderFinish(float_node);
    }
#endif

    static ImGuiKey TranslateGLFWKeyToImGuiKey(int key)
    {
        if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
            return (ImGuiKey)(ImGuiKey_A + (key - GLFW_KEY_A));

        if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
            return (ImGuiKey)(ImGuiKey_0 + (key - GLFW_KEY_0));

        switch (key)
        {
        case GLFW_KEY_SPACE:        return ImGuiKey_Space;
        case GLFW_KEY_ESCAPE:       return ImGuiKey_Escape;
        case GLFW_KEY_ENTER:        return ImGuiKey_Enter;
        case GLFW_KEY_TAB:          return ImGuiKey_Tab;
        case GLFW_KEY_BACKSPACE:    return ImGuiKey_Backspace;

        case GLFW_KEY_LEFT:         return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT:        return ImGuiKey_RightArrow;
        case GLFW_KEY_UP:           return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN:         return ImGuiKey_DownArrow;

        case GLFW_KEY_LEFT_CONTROL:
        case GLFW_KEY_RIGHT_CONTROL:
            return ImGuiKey_LeftCtrl;  

        case GLFW_KEY_LEFT_SHIFT:
        case GLFW_KEY_RIGHT_SHIFT:
            return ImGuiKey_LeftShift;

        case GLFW_KEY_LEFT_ALT:
        case GLFW_KEY_RIGHT_ALT:
            return ImGuiKey_LeftAlt;

        case GLFW_KEY_LEFT_SUPER:
        case GLFW_KEY_RIGHT_SUPER:
            return ImGuiKey_LeftSuper;


        default:
            return ImGuiKey_None;
        }
    }




    void Init(GLFWwindow* w)
    {
        s_Window = w;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        Engine->m_editorFrameCallback = Editor_FrameCallback;

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImGui_ImplOpenGL3_Init("#version 130");

        InitProjectPanel();

        SetupDropCallback(w);
    }


    void NewFrame() {
        // IMPORTANT! If previous frame was started but not completed, force complete it
        if (s_frameStarted) {
            /*std::cerr << "[Editor] WARNING: Previous frame not completed! Force completing..." << std::endl;*/
            // This can happen during scene loading when framebuffer becomes invalid mid-frame
            ImGui::Render(); // Complete the previous frame
            ImGui::EndFrame(); // Ensure frame is fully ended
        }

        s_frameStarted = false;

        // Skip if window is minimized
        int display_w, display_h;
        glfwGetFramebufferSize(s_Window, &display_w, &display_h);
        if (display_w <= 0 || display_h <= 0)
            return;

        // --- Backend: OpenGL only ---
        ImGui_ImplOpenGL3_NewFrame();

        ImGuiIO& io = ImGui::GetIO();

        // ============================================================
        // 1. Proper DeltaTime (fix cursor blink, animation speed, etc)
        // ============================================================
        {
            static double lastTime = glfwGetTime();
            double now = glfwGetTime();
            io.DeltaTime = (float)(now - lastTime);

            // fallback (avoid zero or negative values)
            if (io.DeltaTime <= 0.0f)
                io.DeltaTime = 1.0f / 60.0f;

            lastTime = now;
        }

        // ============================================================
        // 2. Display size
        // ============================================================
        float w = (float)WindowWidth;
        float h = (float)WindowHeight;
        if (w <= 0) w = 1;
        if (h <= 0) h = 1;

        io.DisplaySize = ImVec2(w, h);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

        // ============================================================
        // 3. Manual input injection (NO glfw backend)
        // ============================================================
        auto g_coordinator = Coordinator::GetInstance();
        if (auto input = g_coordinator->GetSystem<InputManager>())
        {
            // --- Mouse position ---
            glm::vec2 mp = input->getMousePosition();
            io.AddMousePosEvent(mp.x, mp.y);

            // --- Mouse buttons ---
            for (int i = 0; i < 3; i++)
            {
                io.AddMouseButtonEvent(i, input->isMouseButtonPressed(i));
            }

            // --- Mouse scroll (your engine probably uses scrollY) ---
            io.AddMouseWheelEvent(0.0f, (float)g_scrollY);
            g_scrollY = 0.0;

            // --- Keyboard events (ONLY on state change!!!) ---
            for (int key = 32; key <= GLFW_KEY_LAST; key++)
            {
                bool down = input->isKeyPressed(key);

                // Only send a key event when state changes
                if (down != s_prevKeys[key])
                {
                    ImGuiKey imKey = TranslateGLFWKeyToImGuiKey(key);
                    if (imKey != ImGuiKey_None)
                    {
                        io.AddKeyEvent(imKey, down);
                    }

                    s_prevKeys[key] = down;
                }
            }

            // --- Text input (typing real characters) ---
            unsigned int c = input->getLastCharTyped();
            if (c != 0 && c != '\b')
            {
                io.AddInputCharacter(c);
            }
            input->clearLastChar();
        }

        // ============================================================
        // 4. ImGui internal start
        // ============================================================
        ImGui::NewFrame();

        s_frameStarted = true;
    }

    void Draw() {

        int display_w, display_h;
        glfwGetFramebufferSize(s_Window, &display_w, &display_h);
        if (display_w <= 0 || display_h <= 0)
            return;

        // Don't draw if NewFrame() wasn't called
        if (!s_frameStarted)
            return;

        if (s_restoreNextFrame) {
            RestoreEditorBaseline();
            s_restoreNextFrame = false;
        }

        ImGuiIO& io = ImGui::GetIO();
        const float W = io.DisplaySize.x;
        const float H = io.DisplaySize.y;

        //UNDO Manager
        // ================================
        // GLOBAL SHORTCUT (Undo)
        // ================================
        bool ctrlDown =
            glfwGetKey(s_Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(s_Window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        bool shiftDown =
            glfwGetKey(s_Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(s_Window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

        static bool prevZ = false;
        bool currZ = (glfwGetKey(s_Window, GLFW_KEY_Z) == GLFW_PRESS);
        if (ctrlDown && currZ && !prevZ)
        {
            std::cout << "[Editor] CTRL+Z pressed in editor\n";
            if (shiftDown)
            {
                std::cout << "[Editor] CTRL+Z+SHIFT pressed in editor\n";
                // Ctrl+Shift+Z = Redo
                if (mUndo.CanRedo()) {
                    std::cout << "[Editor] CanRedo? :" << mUndo.CanRedo() << "\n";
                    mUndo.Redo();
                }
            }
            else
            {
                std::cout << "[Editor] ELSE UNDO?\n";
                // Ctrl+Z = Undo
                if (mUndo.CanUndo()) {
                    std::cout << "[Editor] CanUndo? :" << mUndo.CanUndo() << "\n";
                    mUndo.Undo();
                }  
            }
        }
        prevZ = currZ;

        // --- Clamp layout ratios
        s_top_ratio = clamp_val(s_top_ratio, MIN_RATIO_H, MAX_TOTAL_VERT);
        s_bottom_ratio = clamp_val(s_bottom_ratio, MIN_RATIO_H, MAX_TOTAL_VERT);
        if (s_top_ratio + s_bottom_ratio > MAX_TOTAL_VERT) s_bottom_ratio = MAX_TOTAL_VERT - s_top_ratio;

        s_left_ratio = clamp_val(s_left_ratio, MIN_RATIO_W, MAX_TOTAL_SIDE);
        s_right_ratio = clamp_val(s_right_ratio, MIN_RATIO_W, MAX_TOTAL_SIDE);
        if (s_left_ratio + s_right_ratio > MAX_TOTAL_SIDE) s_right_ratio = MAX_TOTAL_SIDE - s_left_ratio;

        const float top_h = H * s_top_ratio;
        const float bottom_h = H * s_bottom_ratio;
        const float middle_h = H - top_h - bottom_h;
        const float safe_middle_h = (middle_h < 1.0f ? 1.0f : middle_h);

        const float left_w = W * s_left_ratio;
        const float right_w = W * s_right_ratio;
        const float mid_w = W - left_w - right_w;
        const float safe_mid_w = (mid_w < 1.0f ? 1.0f : mid_w);

        const float top_y = 0.0f;
        const float middle_y = top_h;
        const float bottom_y = top_h + safe_middle_h;
        const float left_x = 0.0f;
        const float right_x = left_w + safe_mid_w;

        ImGuiWindowFlags panel_flags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings;


        DrawTopBarPanel(W, top_y, top_h);


        // ======================================================
        // FIXED HOST PANELS WITH DOCKSPACES: Left / Right / Bottom
        // (no DockBuilder: we only create DockSpace IDs)
        // ======================================================

        // --- LEFT HOST ---
        ImGui::SetNextWindowPos(ImVec2(left_x, middle_y));
        ImGui::SetNextWindowSize(ImVec2(left_w, safe_middle_h));
        ImGuiID leftDockID = 0;
        if (ImGui::Begin("Left Panel", nullptr, panel_flags)) {
            leftDockID = ImGui::GetID("LeftDockSpace");
            ImGui::DockSpace(leftDockID, ImVec2(0, 0));
        }
        ImGui::End();

        // --- RIGHT HOST ---
        ImGui::SetNextWindowPos(ImVec2(right_x, middle_y));
        ImGui::SetNextWindowSize(ImVec2(right_w, safe_middle_h));

        ImGuiID rightDockID = 0;
        if (ImGui::Begin("Right Panel", nullptr, panel_flags)) {
            rightDockID = ImGui::GetID("RightDockSpace");
            ImGui::DockSpace(rightDockID, ImVec2(0, 0));
        }
        ImGui::End();

        // --- BOTTOM HOST ---
        ImGui::SetNextWindowPos(ImVec2(0.0f, bottom_y));
        ImGui::SetNextWindowSize(ImVec2(W, bottom_h));
        ImGuiID bottomDockID = 0;
        if (ImGui::Begin("Bottom", nullptr, panel_flags)) {
            bottomDockID = ImGui::GetID("BottomDockSpace");
            ImGui::DockSpace(bottomDockID, ImVec2(0, 0));
        }
        ImGui::End();

        // =========================================
        // DOCKABLE TOOL WINDOWS (become tabs)
        // We "suggest" a default dock only on first run using ImGuiCond_FirstUseEver
        // =========================================

        // Scene Hierarchy
        if (leftDockID) ImGui::SetNextWindowDockID(leftDockID, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Hierarchy"))
        {
            DrawHierarchyWindow(leftDockID);
        }
        ImGui::End();

        // Inspector
        if (rightDockID) ImGui::SetNextWindowDockID(rightDockID, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Inspector")) {
            DrawInspectorWindow(rightDockID);
        }
        ImGui::End();

        // Layer Manager
        if (rightDockID) ImGui::SetNextWindowDockID(rightDockID, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Layer Manager")) {
            DrawLayerManagerWindow();
        }
        ImGui::End();

        // Config Editor
        if (rightDockID) ImGui::SetNextWindowDockID(rightDockID, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Config")) {
            DrawConfigPanel();
        }
        ImGui::End();

        // Project (Assets & Prefabs)
        if (bottomDockID) ImGui::SetNextWindowDockID(bottomDockID, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Project")) {
            // folder browser
            ProjectPanel();
        }
        ImGui::End();

        // Debug Console
        if (bottomDockID) ImGui::SetNextWindowDockID(bottomDockID, ImGuiCond_FirstUseEver);
        static DebugConsole debugConsole;
        if (ImGui::Begin("Debug Console")) {
            debugConsole.Draw();
            // put your debug UI here

            // Example on logging usage
            // Plain text
            //debugConsole.AddLog("Hello plain text");

            // OA::BenchMark
            if (ImGui::IsKeyPressed(ImGuiKey_M))
            {
               //Student
               auto r = Benchmark::RunOABenchmark(1000);
               debugConsole.AddLog("%s", Benchmark::PrintOABenchmark(r).c_str());
            }

            //auto r = Benchmark::RunOABenchmark(1000);
            //debugConsole.AddLog("%s", Benchmark::PrintOABenchmark(r).c_str());

            //// With level (colored automatically)
            //debugConsole.AddLogLevel(ConsoleLevel::Info, "Loaded %d textures", &Systems::EntityMember);
            //debugConsole.AddLogLevel(ConsoleLevel::Warn, "FPS is low: %.1f", GLHelper::fps);
            //debugConsole.AddLogLevel(ConsoleLevel::Error, "Failed to open '%s'", "[filename]");

            ////// With explicit color (custom)
            //debugConsole.AddLogColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "Green success message");
        }
        ImGui::End();

        // Profiler
        if (bottomDockID) ImGui::SetNextWindowDockID(bottomDockID, ImGuiCond_FirstUseEver);
        static ProfilingPanel profilingPanel;
        if (ImGui::Begin("Profiler")) {
            profilingPanel.Draw();
        }
        ImGui::End();

        DrawBuildSizeAnalyzerWindow();

        // ===== In-scene picking/dragging (gizmo) on the middle viewport =====
        PulseEditor::GameViewport vp;
        vp.x = (int)left_w;
        vp.y = (int)middle_y;
        vp.w = (int)safe_mid_w;
        vp.h = (int)safe_middle_h;


        PulseEditor::Gizmo_DrawAndHandle(vp);



        // =============================
        // SPLITTER OVERLAY (unchanged)
        // =============================
        ImGuiWindowFlags overlay_flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(W, H));
        ImGui::Begin("LayoutOverlay", nullptr, overlay_flags);

        // Top/Middle splitter
        {
            float bar_y = top_h;
            ImGui::SetCursorScreenPos(ImVec2(0.0f, bar_y - SPLIT_THICK * 0.5f));
            ImGui::InvisibleButton("##split_top_mid", ImVec2(W, SPLIT_THICK));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            if (ImGui::IsItemActive()) {
                float new_edge = bar_y + ImGui::GetIO().MouseDelta.y;
                s_top_ratio = clamp_val(new_edge / H, MIN_RATIO_H, MAX_TOTAL_VERT);
                if (s_top_ratio + s_bottom_ratio > MAX_TOTAL_VERT) s_top_ratio = MAX_TOTAL_VERT - s_bottom_ratio;
            }
            DrawGuideLine(ImVec2(0.0f, bar_y), ImVec2(W, bar_y));
        }

        // Middle/Bottom splitter
        {
            float bar_y = bottom_y;
            ImGui::SetCursorScreenPos(ImVec2(0.0f, bar_y - SPLIT_THICK * 0.5f));
            ImGui::InvisibleButton("##split_mid_bottom", ImVec2(W, SPLIT_THICK));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            if (ImGui::IsItemActive()) {
                float new_edge = bar_y + ImGui::GetIO().MouseDelta.y;
                float new_bottom_h = H - new_edge;
                s_bottom_ratio = clamp_val(new_bottom_h / H, MIN_RATIO_H, MAX_TOTAL_VERT);
                if (s_top_ratio + s_bottom_ratio > MAX_TOTAL_VERT) s_bottom_ratio = MAX_TOTAL_VERT - s_top_ratio;
            }
            DrawGuideLine(ImVec2(0.0f, bar_y), ImVec2(W, bar_y));
        }

        // Left/Middle splitter
        {
            float bar_x = left_w;
            ImGui::SetCursorScreenPos(ImVec2(bar_x - SPLIT_THICK * 0.5f, middle_y));
            ImGui::InvisibleButton("##split_left_mid", ImVec2(SPLIT_THICK, safe_middle_h));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            if (ImGui::IsItemActive()) {
                float new_edge = bar_x + ImGui::GetIO().MouseDelta.x;
                s_left_ratio = clamp_val(new_edge / W, MIN_RATIO_W, MAX_TOTAL_SIDE);
                if (s_left_ratio + s_right_ratio > MAX_TOTAL_SIDE) s_left_ratio = MAX_TOTAL_SIDE - s_right_ratio;
            }
            DrawGuideLine(ImVec2(bar_x, middle_y), ImVec2(bar_x, middle_y + safe_middle_h));
        }

        // Middle/Right splitter
        {
            float bar_x = right_x;
            ImGui::SetCursorScreenPos(ImVec2(bar_x - SPLIT_THICK * 0.5f, middle_y));
            ImGui::InvisibleButton("##split_mid_right", ImVec2(SPLIT_THICK, safe_middle_h));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            if (ImGui::IsItemActive()) {
                float new_edge = bar_x + ImGui::GetIO().MouseDelta.x;
                float new_right_w = (W - new_edge);
                s_right_ratio = clamp_val(new_right_w / W, MIN_RATIO_W, MAX_TOTAL_SIDE);
                if (s_left_ratio + s_right_ratio > MAX_TOTAL_SIDE) s_right_ratio = MAX_TOTAL_SIDE - s_left_ratio;
            }
            DrawGuideLine(ImVec2(bar_x, middle_y), ImVec2(bar_x, middle_y + safe_middle_h));
        }

        ImGui::End(); // LayoutOverlay
        UpdateImGuiCursor(s_Window);

    }


    void Render()
    {
        // --- Apply viewport ---
        auto glvp = ComputeMiddleGameViewportPixels(s_Window, true);
        GLHelper::GameOffsetX = glvp.x;
        GLHelper::GameOffsetY = glvp.y;
        GLHelper::GameWidth = glvp.w;
        GLHelper::GameHeight = glvp.h;
        glViewport(glvp.x, glvp.y, glvp.w, glvp.h);

        // --- Render ImGui ---
        // Don't render if NewFrame() wasn't called
        if (!s_frameStarted)
            return;

        int display_w, display_h;
        glfwGetFramebufferSize(s_Window, &display_w, &display_h);
        if (display_w <= 0 || display_h <= 0)
            return;

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }



    void Shutdown()
    {

        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }


    extern float s_left_ratio;
    extern float s_right_ratio;
    extern float s_bottom_ratio;

    GameViewport ComputeMiddleGameViewportPixels(GLFWwindow* window, bool keep_16x9)
    {
        ImGuiIO& io = ImGui::GetIO();

        // 1. Get current ImGui logical size (in "UI pixels")
        float w = io.DisplaySize.x;
        float h = io.DisplaySize.y;

        if (w <= 0.f || h <= 0.f)
        {
            int winW = 0, winH = 0;
            glfwGetWindowSize(window, &winW, &winH);
            w = (float)std::max(1, winW);
            h = (float)std::max(1, winH);
        }

        // 2. Clamp ratios, but SWAP meaning:
        //    - s_bottom_ratio now drives the TOP bar height
        //    - s_top_ratio    now drives the BOTTOM panel height
        float clamped_top = clamp_val(s_bottom_ratio, MIN_RATIO_H, MAX_TOTAL_VERT);
        float clamped_bottom = clamp_val(s_top_ratio, MIN_RATIO_H, MAX_TOTAL_VERT);

        if (clamped_top + clamped_bottom > MAX_TOTAL_VERT)
        {
            // prevent overlap: shrink bottom if the combined is too big
            clamped_bottom = MAX_TOTAL_VERT - clamped_top;
        }

        float clamped_left = clamp_val(s_left_ratio, MIN_RATIO_W, MAX_TOTAL_SIDE);
        float clamped_right = clamp_val(s_right_ratio, MIN_RATIO_W, MAX_TOTAL_SIDE);
        if (clamped_left + clamped_right > MAX_TOTAL_SIDE)
        {
            // prevent overlap: shrink right if combined is too big
            clamped_right = MAX_TOTAL_SIDE - clamped_left;
        }

        // 3. Convert ratios -> absolute logical sizes
        const float topbar_h = h * clamped_top;        // NOW driven by s_bottom_ratio
        const float bottombar_h = h * clamped_bottom;     // NOW driven by s_top_ratio
        const float middle_h = h - topbar_h - bottombar_h;
        const float safe_mid_h = (middle_h < 1.0f ? 1.0f : middle_h);

        const float left_w = w * clamped_left;
        const float right_w = w * clamped_right;
        const float mid_w = w - left_w - right_w;
        const float safe_mid_w = (mid_w < 1.0f ? 1.0f : mid_w);

        // 4. Game area in logical ImGui coords
        //    It still starts after the "top bar" and ends before the "bottom bar".
        const float game_x = left_w;
        const float game_y = topbar_h;        // below (swapped) top bar
        const float game_w = safe_mid_w;
        const float game_h = safe_mid_h;      // above (swapped) bottom panel

        // 5. Convert to framebuffer coords
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbW <= 0 || fbH <= 0) {
            // window minimized / no drawable area
            return { 0, 0, 0, 0 };
        }


        const float sx = (w > 0.f) ? (float)fbW / w : 1.f;
        const float sy = (h > 0.f) ? (float)fbH / h : 1.f;

        int vx = (int)std::round(game_x * sx);
        int vy = (int)std::round(game_y * sy);
        int vw = (int)std::round(game_w * sx);
        int vh = (int)std::round(game_h * sy);

        // 6. Optional 16:9 fit inside the middle band
        if (keep_16x9 && vw > 0 && vh > 0)
        {
            const float target = 16.0f / 9.0f;
            if ((float)vh > (float)vw / target)
            {
                // area is too tall -> clamp height
                vh = (int)std::round(vw / target);
            }
            else
            {
                // area is too wide -> clamp width
                vw = (int)std::round(vh * target);
            }

            // recenter that (vw,vh) inside the mid band
            const int bandWpx = (int)std::round(game_w * sx);
            const int bandHpx = (int)std::round(game_h * sy);
            vx += (bandWpx - vw) / 2;
            vy += (bandHpx - vh) / 2;
        }

        // 7. Clamp to framebuffer bounds
        vw = std::clamp(vw, 1, fbW);
        vh = std::clamp(vh, 1, fbH);
        vx = std::clamp(vx, 0, std::max(0, fbW - vw));
        vy = std::clamp(vy, 0, std::max(0, fbH - vh));

        return { vx, vy, vw, vh };
    }

    static void UpdateImGuiCursor(GLFWwindow* window)
    {
        ImGuiIO& io = ImGui::GetIO();

        // If ImGui draws its own cursor (software cursor), hide OS cursor
        if (io.MouseDrawCursor)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            return;
        }

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        // Ask ImGui what cursor it wants
        ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
        GLFWcursor* glfwCursor = nullptr;

        switch (cursor)
        {
        case ImGuiMouseCursor_ResizeNS:
            glfwCursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
            break;

        case ImGuiMouseCursor_ResizeEW:
            glfwCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
            break;

        case ImGuiMouseCursor_Hand:
            glfwCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
            break;

            // GLFW does NOT support diagonal resize → use arrow instead
        case ImGuiMouseCursor_ResizeNWSE:
        case ImGuiMouseCursor_ResizeNESW:
            glfwCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
            break;

        default:
            glfwCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
            break;
        }

        if (glfwCursor)
            glfwSetCursor(window, glfwCursor);
    }


    void Editor_FrameCallback()
    {

        // 1. Start a new ImGui frame
        PulseEditor::NewFrame();

        // 3. Draw the editor
        PulseEditor::Draw();
        PulseEditor::Render();
    }
   
}

