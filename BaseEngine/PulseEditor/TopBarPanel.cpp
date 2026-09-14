/******************************************************************************/
/**
 * @file        TopBarPanel.cpp
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (90%)
 * @co-author   Chia Wei Xuan Rachael(10%).
 * @brief       Implements the Top Panel for the editor.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "pch/pch_temp.h"   // must stay first (PCH)

#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>

// editor headers
#include "TopBarPanel.h"
#include "Editor.h"
#include "LayerManagerPanel.h"  // For syncing layer visibility
#include "BuildSizeAnalyzer.h"
#include <imgui.h>

// engine headers
#include "CoreEngine/ECS/Coordinator.h"
#include "CoreEngine/Scene/SceneManager.h"

namespace PulseEditor
{

    struct SavedEntityState
    {
        Entity id;

        bool hasTransform;
        Framework::Transform transform;

        bool hasRenderable;
        Framework::Renderable renderable;

        bool hasName;
        Name name;

        SavedEntityState()
            : id(INVALID_ENTITY),
            hasTransform(false),
            hasRenderable(false),
            hasName(false)
        {
        }
    };
            
    static bool s_hasEditorBaseline = false;     
    static Entity s_selectedBeforePlay = INVALID_ENTITY;
    static std::vector<SavedEntityState> s_editorBaseline;
    static std::string s_sceneBeforePlay;

    static bool s_wasManuallPaused = false;  // for checking if you pressed pause button

    void CaptureEditorBaseline()
    {
        s_editorBaseline.clear();

        auto* coord = Coordinator::GetInstance();
        if (!coord)
            return;

        for (auto e : coord->GetAllEntities())
        {
            SavedEntityState st;
            st.id = e;

            if (coord->HasComponent<Framework::Transform>(e))
            {
                st.hasTransform = true;
                st.transform = coord->GetComponent<Framework::Transform>(e);
            }

            if (coord->HasComponent<Framework::Renderable>(e))
            {
                st.hasRenderable = true;
                st.renderable = coord->GetComponent<Framework::Renderable>(e);
            }

            if (coord->HasComponent<Name>(e))
            {
                st.hasName = true;
                st.name = coord->GetComponent<Name>(e);
            }

            s_editorBaseline.push_back(std::move(st));
        }

        s_hasEditorBaseline = true;
        //std::cout << "[Editor] Captured baseline with "
        //    << s_editorBaseline.size() << " entities.\n";
    }

    void RestoreEditorBaseline()
    {
        if (!s_hasEditorBaseline)
            return;

        auto* coord = Coordinator::GetInstance();
        if (!coord)
            return;

        for (auto const& st : s_editorBaseline)
        {
            if (coord->HasComponent<Framework::Transform>(st.id) && st.hasTransform)
                coord->GetComponent<Framework::Transform>(st.id) = st.transform;

            if (coord->HasComponent<Framework::Renderable>(st.id) && st.hasRenderable)
                coord->GetComponent<Framework::Renderable>(st.id) = st.renderable;

            if (coord->HasComponent<Name>(st.id) && st.hasName)
                coord->GetComponent<Name>(st.id) = st.name;
        }

        //std::cout << "[Editor] Restored baseline.\n";
    }

    std::string PulseEditor::OpenJsonFileDialog()
    {
        char fileBuf[MAX_PATH] = { 0 };

        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(OPENFILENAMEA);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
        ofn.lpstrFile = fileBuf;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        ofn.lpstrTitle = "Select scene JSON";

        namespace fs = std::filesystem;
        std::string dir = (FilePathToGame / "JSON").string();
        ofn.lpstrInitialDir = dir.c_str();


        if (GetOpenFileNameA(&ofn)) {
            return std::string(fileBuf);
        }
        return {};
    }

    void DrawTopBarPanel(float W, float top_y, float top_h)
    {
        // Static variable to defer scene loading after dialog closes
        static std::string s_pendingScenePath;

        // New Game popup state
        static bool s_showNewGamePopup = false;
        static char s_newGameNameBuf[128] = {};
        static std::string s_newGameResult;
        static bool s_openGameSwitchedAfterCreate = false;
        static std::string s_newGameCreatedPath;

        ImGuiWindowFlags top_flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::SetNextWindowPos(ImVec2(0.0f, top_y));
        ImGui::SetNextWindowSize(ImVec2(W, top_h));

        if (!ImGui::Begin("TopBar", nullptr, top_flags))
        {
            ImGui::End();
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

        if (ImGui::BeginMenuBar())
        {
            DrawBuildSizeAnalyzerMenu();
            ImGui::EndMenuBar();
        }

        ImVec2 framePadding = ImGui::GetStyle().FramePadding;
        float itemSpacing = ImGui::GetStyle().ItemSpacing.x;

        // ------------------------------------------------------------
        // Left side: Save Scene / Load Scene / New Game / Load Other Games
        // ------------------------------------------------------------
        ImVec2 newTextSize = ImGui::CalcTextSize("New Scene");
        ImVec2 saveTextSize = ImGui::CalcTextSize("Save Scene");
        ImVec2 loadTextSize = ImGui::CalcTextSize("Load Scene");
        ImVec2 newButtonSize = ImVec2(newTextSize.x + framePadding.x * 2, 0);
        ImVec2 saveButtonSize = ImVec2(saveTextSize.x + framePadding.x * 2, 0);
        ImVec2 loadButtonSize = ImVec2(loadTextSize.x + framePadding.x * 2, 0);

        if (IsPlaying())
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        }

        if (ImGui::Button("Save Scene", saveButtonSize))
        {
            if (!IsPlaying()) // block the action manually
            {
                if (auto sceneMgr = Coordinator::GetInstance()->GetSystem<SceneManager>())
                    sceneMgr->SaveCurrentScene();
            }
        }

        if (IsPlaying())
        {
            ImGui::PopStyleColor(3);
            if (ImGui::IsItemHovered())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f)); // red-ish warning
                ImGui::SetTooltip("Stop the game first before saving!!!");
                ImGui::PopStyleColor();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("New Scene", newButtonSize))
        {
            //add name 
            s_showNewGamePopup = true;
            s_newGameNameBuf[0] = '\0';
            s_newGameResult.clear();
            ImGui::OpenPopup("New Scene Name");
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Scene", loadButtonSize))
        {
            s_pendingScenePath = OpenJsonFileDialog();
        }

        ImGui::SameLine();

        if (ImGui::Button("New Game"))
        {
            s_showNewGamePopup = true;
            s_newGameNameBuf[0] = '\0';
            s_newGameResult.clear();
            ImGui::OpenPopup("New Game Project");
        }

        ImGui::SameLine(); // umm i think this puts it on the same Y axis as those above???(idfk tbh, me is just follow)

        if (ImGui::Button("Load Other Games"))
        {
            // Set up the Windows file-open finder. Find config.json that will be inside the Games Json Folder
            char fileBuf[MAX_PATH] = { 0 };
            OPENFILENAMEA ofn{};
            ofn.lStructSize  = sizeof(OPENFILENAMEA);   // Windows API say need do.....
            ofn.hwndOwner    = nullptr;                 // no parent window

            // format for the config.json
            ofn.lpstrFilter  = "Game Config\0config.json\0All Files\0*.*\0";
            ofn.lpstrFile    = fileBuf;
            ofn.nMaxFile     = MAX_PATH;
            ofn.Flags        = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            ofn.lpstrTitle   = "Open Game Project (select config.json)";        

            // Only continue if the user picks a file
            if (GetOpenFileNameA(&ofn))
            {
                namespace fs = std::filesystem;

                // by right this will be in BaseEngine/GameName/Json/config.json if u pick correctly ah :>
                fs::path selectedFile(fileBuf);

                // Now we go up 2 levels to BaseEngine/GameName. We then update the 
                // file path to the game and the Game Name  
                fs::path newGameRoot = selectedFile.parent_path().parent_path();
                std::string newGameName = newGameRoot.filename().string();

                // We then find BaseEngine to update ActiveGame.json with the new game name(so that it knows which game 2 load nxt time)
                fs::path cur = fs::current_path();
                while (cur.filename() != RootFolderName && cur.has_parent_path())
                {
                    cur = cur.parent_path();
                }

                // Write into ActiveGame.json
                std::ofstream ConfigOut(cur / "ActiveGame.json");
                if (ConfigOut.is_open())
                {
                    ConfigOut << "{\n  \"ActiveGame\": \"" << newGameName << "\"\n}\n";
                }

                // Auto delete CMakeCache.txt when u pick a new game so that ran.bat can reconfigures the cmake for the new game.
                fs::path cacheFile = cur / "build" / "CMakeCache.txt";
                if (fs::exists(cacheFile))
                {
                    std::error_code ecCache;
                    fs::remove(cacheFile, ecCache);
                }
                

                // Show a message that tell user need to restart the entire editor as FilePathToGame takes effect only on next time it launches
                ImGui::OpenPopup("Game Switched");
            }
        }

        ImGui::SetNextWindowSize(ImVec2(520, 0), ImGuiCond_Always);
        if (ImGui::BeginPopupModal("Game Switched", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Game project switched!");
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "CMake cache auto-cleared.");
            if (!s_newGameCreatedPath.empty())
            {
                ImGui::Separator();
                ImGui::Text("Project created at:");
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.4f, 1.0f));
                ImGui::TextWrapped("%s", s_newGameCreatedPath.c_str());
                ImGui::PopStyleColor();
            }
            ImGui::Separator();
            ImGui::Text("Next steps:");
            ImGui::BulletText("Close Visual Studio");
            ImGui::BulletText("Run ran.bat");
            ImGui::BulletText("Reopen PulseEngine.sln");
            if (ImGui::Button("OK"))
            {
                s_newGameCreatedPath.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // For new scene
        if (ImGui::BeginPopupModal("New Scene Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter a name for the new scene:");
            ImGui::InputText("##gamename", s_newGameNameBuf, sizeof(s_newGameNameBuf));

            if (!s_newGameResult.empty())
            {
                ImGui::TextWrapped("%s", s_newGameResult.c_str());
            }

            if (ImGui::Button("Create") && s_newGameNameBuf[0] != '\0')
            {
                namespace fs = std::filesystem;

                std::string gameName(s_newGameNameBuf);

                // Reject gamename with spaces 
                if (gameName.find(' ') != std::string::npos)
                {
                    s_newGameResult = "Error: Game name cannot contain spaces!\n";
                    ImGui::EndPopup();
                    return;
                }


                // Write starter JSON files
                auto writeFile = [](const fs::path& p, const std::string& content) {
                    std::ofstream f(p);
                    if (f.is_open()) f << content;
                    };


                // create json file with new game name
                writeFile(FilePathToGame / "JSON" / (gameName + ".json"),("{\n    \"scene\": \"" + gameName + "\",\n    \"entities\": []\n}\n").c_str());

                // load that scene 
                auto g_coordinator = Coordinator::GetInstance();
                auto g_Scene = g_coordinator->GetSystem<SceneManager>();
                g_Scene->ChangeCurrentScene(gameName);
                ImGui::CloseCurrentPopup();
            }


            ImGui::SameLine();
            if (ImGui::Button("Close"))
            {
                s_showNewGamePopup = false;
                s_newGameResult.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ── New Game popup ─────────────────────────────────────────
        if (ImGui::BeginPopupModal("New Game Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter a name for the new game project:");
            ImGui::InputText("##gamename", s_newGameNameBuf, sizeof(s_newGameNameBuf));

            if (!s_newGameResult.empty())
            {
                ImGui::TextWrapped("%s", s_newGameResult.c_str());
            }

            if (ImGui::Button("Create") && s_newGameNameBuf[0] != '\0')
            {
                namespace fs = std::filesystem;

                std::string gameName(s_newGameNameBuf);

                // Reject gamename with spaces 
                if (gameName.find(' ') != std::string::npos)
                {
                    s_newGameResult = "Error: Game name cannot contain spaces!\n";
                    ImGui::EndPopup();
                    return;
                }

                // Find BaseEngine folder by walking up from current path
                fs::path cur = fs::current_path();
                while (cur.filename() != RootFolderName && cur.has_parent_path())
                    cur = cur.parent_path();

                fs::path gameRoot = cur / gameName;

                // Create folder structure
                std::error_code ec;
                fs::create_directories(gameRoot / "Assets" / "Textures", ec);
                fs::create_directories(gameRoot / "Assets" / "Sound",    ec);
                fs::create_directories(gameRoot / "Assets" / "Font",     ec);
                fs::create_directories(gameRoot / "Assets" / "Prefabs",  ec);
                fs::create_directories(gameRoot / "JSON",                 ec);

                // Write starter JSON files
                auto writeFile = [](const fs::path& p, const char* content) {
                    std::ofstream f(p);
                    if (f.is_open()) f << content;
                };

                writeFile(gameRoot / "JSON" / "config.json",
                    ("{\n    \"GameName\": \"" + gameName + "\",\n    \"config\": {\n        \"WindowWidth\": 1600,\n        \"WindowHeight\": 900,\n        \"ConfigPath\": \"\",\n        \"FirstScene\": \"FirstScene\"\n    }\n}\n").c_str());

                // Default empty scene — engine loads this if no other FirstScene is set in config.json
                writeFile(gameRoot / "JSON" / "FirstScene.json",
                    ("{\n    \"scene\": \"FirstScene\",\n    \"entities\": []\n}\n"));
                writeFile(gameRoot / "JSON" / "texture.json",
                    "{\n    \"textureFilePaths\": []\n}\n");
                writeFile(gameRoot / "JSON" / "audio.json",
                    "{\n    \"audioFilePaths\": []\n}\n");
                writeFile(gameRoot / "JSON" / "animation.json",
                    "{\n    \"animationFilePaths\": []\n}\n");
                writeFile(gameRoot / "JSON" / "font.json",
                    "{\n    \"fontFilePaths\": []\n}\n");
                writeFile(gameRoot / "JSON" / "scene.json",
                    "{\n    \"sceneFilePaths\": []\n}\n");

                // Create ManagedScripts folder + project files so the new game has its own C# script project(AND MUST BE IN THE GAME AH)
                fs::create_directories(gameRoot / "ManagedScripts", ec);

                // .csproj use the path: ..\..\build\.bin\ which works at the same folder depth as PulseProtocol.
                // The rest are just to tell dotnet how to build your C# script(used to be in the other cmake but do it here instead cos easier.)
                writeFile(gameRoot / "ManagedScripts" / "ManagedScripts.csproj",
                    "<Project Sdk=\"Microsoft.NET.Sdk\">\n"
                    "  <PropertyGroup>\n"
                    "    <OutputType>Library</OutputType>\n"
                    "    <TargetFramework>net8.0</TargetFramework>\n"
                    "    <ImplicitUsings>enable</ImplicitUsings>\n"
                    "    <Nullable>enable</Nullable>\n"
                    "    <Platforms>x64</Platforms>\n"
                    "    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>\n"
                    "  </PropertyGroup>\n"
                    "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">\n"
                    "    <OutputPath>..\\..\\build\\.bin\\$(Configuration)-$(Platform)\\</OutputPath>\n"
                    "    <PlatformTarget>x64</PlatformTarget>\n"
                    "  </PropertyGroup>\n"
                    "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">\n"
                    "    <OutputPath>..\\..\\build\\.bin\\$(Configuration)-$(Platform)\\</OutputPath>\n"
                    "    <PlatformTarget>x64</PlatformTarget>\n"
                    "  </PropertyGroup>\n"
                    "  <ItemGroup>\n"
                    "    <Reference Include=\"ScriptAPI\">\n"
                    "      <HintPath>..\\..\\build\\.bin\\$(Configuration)-$(Platform)\\ScriptAPI.dll</HintPath>\n"
                    "    </Reference>\n"
                    "  </ItemGroup>\n"
                    "</Project>\n");

                // .sln file: THIS IS WHAT U OPEN TO EDIT SCRIPTS!!!!!!!!!
                writeFile(gameRoot / "ManagedScripts" / "ManagedScripts.sln",
                    "Microsoft Visual Studio Solution File, Format Version 12.00\n"
                    "# Visual Studio Version 17\n"
                    " VisualStudioVersion = 17.5.2.0\n"
                    "MinimumVisualStudioVersion = 10.0.40219.1\n"
                    "Project(\"{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}\") = \"ManagedScripts\", \"ManagedScripts.csproj\", \"{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}\"\n"
                    "EndProject\n"
                    "Global\n"
                    "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n"
                    "\t\tDebug|Any CPU = Debug|Any CPU\n"
                    "\t\tRelease|Any CPU = Release|Any CPU\n"
                    "\tEndGlobalSection\n"
                    "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n"
                    "\t\t{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Debug|Any CPU.ActiveCfg = Debug|Any CPU\n"
                    "\t\t{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Debug|Any CPU.Build.0 = Debug|Any CPU\n"
                    "\t\t{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Release|Any CPU.ActiveCfg = Release|Any CPU\n"
                    "\t\t{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Release|Any CPU.Build.0 = Release|Any CPU\n"
                    "\tEndGlobalSection\n"
                    "\tGlobalSection(SolutionProperties) = preSolution\n"
                    "\t\tHideSolutionNode = FALSE\n"
                    "\tEndGlobalSection\n"
                    "\tGlobalSection(ExtensibilityGlobals) = postSolution\n"
                    "\t\tSolutionGuid = {B2C3D4E5-F6A7-8901-BCDE-F12345678901}\n"
                    "\tEndGlobalSection\n"
                    "EndGlobal\n");

                // CMakeLists.txt: so cmake builds it as part of the main build
                writeFile(gameRoot / "ManagedScripts" / "CMakeLists.txt",
                    "# Build ManagedScripts C# assembly using dotnet CLI\n"
                    "# The .csproj OutputPath already points to build/.bin/{Config}-x64/\n"
                    "# so we don't pass -o here — dotnet respects the .csproj settings.\n"
                    "add_custom_target(ManagedScripts ALL\n"
                    "    COMMAND dotnet build \"${CMAKE_CURRENT_SOURCE_DIR}/ManagedScripts.csproj\"\n"
                    "            -c $<CONFIG>\n"
                    "            -p:Platform=x64\n"
                    "    BYPRODUCTS \"${CMAKE_BINARY_DIR}/.bin/$<CONFIG>-x64/ManagedScripts.dll\"\n"
                    "    COMMENT \"Building ManagedScripts C# assembly\"\n"
                    "    WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\"\n"
                    ")\n"
                    "\n"
                    "# ManagedScripts references ScriptAPI.dll. Must build after it!!!!\n"
                    "add_dependencies(ManagedScripts ScriptAPI)\n");

                // Entry point for game & its exe
                // Entry point name: GameMain.cpp creates CoreEngine and runs the game loop(Copied from PulseEngine ah so if change pulse need change here)
                writeFile(gameRoot / "GameMain.cpp",
                    "#include \"pch/pch.h\"\n"
                    "#include <windows.h>\n"
                    "\n"
                    "int main()\n"
                    "{\n"
                    "    CoreEngine engine;\n"
                    "    Engine = &engine;\n"
                    "\n"
                    "    engine.Setup();\n"
                    "    engine.SetStartupFullscreen(true);\n"
                    "    engine.Init();\n"
                    "    engine.PlayGame();\n"
                    "\n"
                    "    engine.GameLoop();\n"
                    "    engine.Exit();\n"
                    "\n"
                    "    Engine = nullptr;\n"
                    "    return 0;\n"
                    "}\n");

                // WinMain.cpp: so release builds don't open a console window.(If PulseProtocol change, this also change)
                writeFile(gameRoot / "WinMain.cpp",
                    "#include <windows.h>\n"
                    "\n"
                    "int main(int argc, char** argv);\n"
                    "\n"
                    "int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)\n"
                    "{\n"
                    "    return main(__argc, __argv);\n"
                    "}\n");

                // CMakeLists.txt: tells cmake how to build this game as an exe.
                writeFile(gameRoot / "CMakeLists.txt",
                    "cmake_minimum_required(VERSION 3.20)\n"
                    "\n"
                    "# Grab the game name from the current folder name(it should be same name as when u add a new game in editor)\n"
                    "get_filename_component(GAME_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)\n"
                    "set(GAME_ROOT ${CMAKE_CURRENT_LIST_DIR})\n"
                    "\n"
                    "# Grab all cpp/h in this folder\n"
                    "file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS\n"
                    "    \"${GAME_ROOT}/*.cpp\"\n"
                    "    \"${GAME_ROOT}/*.h\"\n"
                    ")\n"
                    "\n"
                    "add_executable(${GAME_NAME} ${GAME_SRC})\n"
                    "\n"
                    "# Hide console in Release (GUI subsystem), keep console in Debug\n"
                    "if (WIN32)\n"
                    "  set_target_properties(${GAME_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)\n"
                    "  target_link_options(${GAME_NAME} PRIVATE\n"
                    "    $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>\n"
                    "    $<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>\n"
                    "    $<$<NOT:$<CONFIG:Debug>>:/ENTRY:mainCRTStartup>\n"
                    "  )\n"
                    "endif()\n"
                    "\n"
                    "add_dependencies(${GAME_NAME} PulseEngine)\n"
                    "\n"
                    "set_target_properties(${GAME_NAME} PROPERTIES\n"
                    "    VS_GLOBAL_DisableFastUpToDateCheck true\n"
                    ")\n"
                    "\n"
                    "target_link_libraries(${GAME_NAME} PRIVATE PulseEngine)\n"
                    "\n"
                    "target_include_directories(${GAME_NAME} PRIVATE\n"
                    "    ${CMAKE_SOURCE_DIR}/PulseEngine\n"
                    ")\n"
                    "\n"
                    "add_dependencies(${GAME_NAME} ScriptAPI)\n"
                    "\n"
                    "# Copy PulseEngine.dll next to the game exe\n"
                    "add_custom_command(TARGET ${GAME_NAME} POST_BUILD\n"
                    "  COMMAND ${CMAKE_COMMAND} -E copy_if_different\n"
                    "    \"$<TARGET_FILE:PulseEngine>\" \"$<TARGET_FILE_DIR:${GAME_NAME}>\"\n"
                    "  VERBATIM\n"
                    ")\n"
                    "\n"
                    "# Copy runtime DLLs (glfw3.dll etc.)\n"
                    "add_custom_command(TARGET ${GAME_NAME} POST_BUILD\n"
                    "    COMMAND ${CMAKE_COMMAND} -E copy_if_different\n"
                    "        $<TARGET_RUNTIME_DLLS:${GAME_NAME}> \"$<TARGET_FILE_DIR:${GAME_NAME}>\"\n"
                    "    COMMAND_EXPAND_LISTS\n"
                    "    VERBATIM\n"
                    ")\n"
                    "\n"
                    "# Copy ScriptAPI.dll next to the game exe\n"
                    "add_custom_command(TARGET ${GAME_NAME} POST_BUILD\n"
                    "    COMMAND ${CMAKE_COMMAND} -E copy_if_different\n"
                    "        \"$<TARGET_FILE:ScriptAPI>\"\n"
                    "        \"$<TARGET_FILE_DIR:${GAME_NAME}>\"\n"
                    ")\n"
                    "\n"
                    "# Copy .NET Runtime DLLs for C# scripting\n"
                    "add_custom_command(TARGET ${GAME_NAME} POST_BUILD\n"
                    "    COMMAND ${CMAKE_COMMAND} -E copy_directory\n"
                    "        \"${CMAKE_SOURCE_DIR}/extern/dotnet/bin\"\n"
                    "        \"$<TARGET_FILE_DIR:${GAME_NAME}>\"\n"
                    ")\n"
                    "\n"
                    "# Copy this game's JSON folder next to the exe\n"
                    "add_custom_command(TARGET ${GAME_NAME} POST_BUILD\n"
                    "    COMMAND ${CMAKE_COMMAND} -E copy_directory\n"
                    "        ${CMAKE_CURRENT_LIST_DIR}/JSON\n"
                    "        $<TARGET_FILE_DIR:${GAME_NAME}>/JSON\n"
                    ")\n"
                    "\n"
                    "# Copy this game's Assets folder next to the exe\n"
                    "add_custom_command(TARGET ${GAME_NAME} POST_BUILD\n"
                    "    COMMAND ${CMAKE_COMMAND} -E copy_directory\n"
                    "        ${CMAKE_CURRENT_LIST_DIR}/Assets\n"
                    "        $<TARGET_FILE_DIR:${GAME_NAME}>/Assets\n"
                    ")\n"
                    "\n"
                    "# # Copy engine shaders to the release layout PulseProtocol expects\n"
                    "add_custom_command(TARGET ${GAME_NAME} POST_BUILD\n"
                    "    COMMAND ${CMAKE_COMMAND} -E copy_directory\n"
                    "        \"${CMAKE_SOURCE_DIR}/PulseEngine/Graphics/shader\"\n"
                    "        \"$<TARGET_FILE_DIR:${GAME_NAME}>/PulseEngine/Graphics/shader\"\n"
                    ")\n"
                    "\n"
                    "# Copy ManagedScripts.dll next to the game exe\n"
                    "add_custom_command(TARGET ${GAME_NAME} POST_BUILD\n"
                    "    COMMAND ${CMAKE_COMMAND} -E copy_if_different\n"
                    "        \"${CMAKE_BINARY_DIR}/.bin/$<CONFIG>-x64/ManagedScripts.dll\"\n"
                    "        \"$<TARGET_FILE_DIR:${GAME_NAME}>\"\n"
                    ")\n"
                    "\n"
                    "add_dependencies(${GAME_NAME} ManagedScripts)\n"
                    "\n"
                    "# Bundle MSVC runtime DLLs (VCRUNTIME140.dll, MSVCP140.dll, etc.) for self-contained distribution\n"
                    "set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)\n"
                    "include(InstallRequiredSystemLibraries)\n"
                    "if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)\n"
                    "    add_custom_command(TARGET ${GAME_NAME} POST_BUILD\n"
                    "        COMMAND ${CMAKE_COMMAND} -E copy_if_different\n"
                    "            ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}\n"
                    "            \"$<TARGET_FILE_DIR:${GAME_NAME}>\"\n"
                    "    )\n"
                    "endif()\n"
                    "\n"
                    "if(MSVC)\n"
                    "    set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} /MP\")\n"
                    "    if (FMOD_API_DIR_FOUND)\n"
                    "        set(FMOD_CORE_DEBUG   \"${FMOD_API_DIR_FOUND}/api/core/lib/x64/fmodL.dll\")\n"
                    "        set(FMOD_CORE_RELEASE \"${FMOD_API_DIR_FOUND}/api/core/lib/x64/fmod.dll\")\n"
                    "        set(FMOD_STUDIO_DEBUG   \"${FMOD_API_DIR_FOUND}/api/studio/lib/x64/fmodstudioL.dll\")\n"
                    "        set(FMOD_STUDIO_RELEASE \"${FMOD_API_DIR_FOUND}/api/studio/lib/x64/fmodstudio.dll\")\n"
                    "        add_custom_command(TARGET ${GAME_NAME} POST_BUILD\n"
                    "            COMMAND ${CMAKE_COMMAND} -E copy_if_different\n"
                    "                \"$<IF:$<CONFIG:Debug>,${FMOD_CORE_DEBUG},${FMOD_CORE_RELEASE}>\"\n"
                    "                $<TARGET_FILE_DIR:${GAME_NAME}>\n"
                    "            COMMAND ${CMAKE_COMMAND} -E copy_if_different\n"
                    "                \"$<IF:$<CONFIG:Debug>,${FMOD_STUDIO_DEBUG},${FMOD_STUDIO_RELEASE}>\"\n"
                    "                $<TARGET_FILE_DIR:${GAME_NAME}>\n"
                    "        )\n"
                    "    endif()\n"
                    "endif()\n");

                // Fianlly, da starter C# script with instructions so the project isn't empty when first opened
                writeFile(gameRoot / "ManagedScripts" / "HelloWorld.cs",
                    ("using ScriptAPI;\n\n"
                    "// Starter script for " + gameName + ".\n"
                    "public class HelloWorld : Script\n"
                    "{\n"
                    "    public override void Update()\n"
                    "    {\n"
                    "    }\n"
                    "}\n").c_str());

                // After the Json folder has been made, open this ones config.json instead of the old one
                if (fs::exists(gameRoot / "JSON"))
                {
                    // Switch ActiveGame.json so next launch opens this game
                    std::ofstream ConfigOut(cur / "ActiveGame.json");
                    if (ConfigOut.is_open())
                    {
                        ConfigOut << "{\n  \"ActiveGame\": \"" << gameName << "\"\n}\n";
                    }

                    // Auto delete CMakeCache.txt so run.bat can make a new add_subdirectory
                    {
                        fs::path cacheFile = cur / "build" / "CMakeCache.txt";
                        if (fs::exists(cacheFile))
                        {
                            std::error_code ecCache;
                            fs::remove(cacheFile, ecCache);
                        }
                    }

                    s_newGameCreatedPath = gameRoot.string();
                    s_openGameSwitchedAfterCreate = true;
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    s_newGameResult = "ERROR: Could not create project folder. Check permissions.";
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Close"))
            {
                s_showNewGamePopup = false;
                s_newGameResult.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Re-open the GameSwitched info popup after a successful New Game creation
        if (s_openGameSwitchedAfterCreate)
        {
            ImGui::OpenPopup("Game Switched");
            s_openGameSwitchedAfterCreate = false;
        }

        // ------------------------------------------------------------
        // Center: Play / Pause buttons
        // ------------------------------------------------------------
        const char* playStopLabel = IsPlaying() ? "Stop" : "Play";
        ImVec2 playStopTextSize = ImGui::CalcTextSize(playStopLabel);

        static bool isPaused = false;
        const char* pauseLabel = isPaused ? "Resume" : "Pause";
        ImVec2 pauseTextSize = ImGui::CalcTextSize(pauseLabel);

        ImVec2 playStopButtonSize = ImVec2(playStopTextSize.x + framePadding.x * 2 + 20.0f, 0);
        ImVec2 pauseButtonSize = ImVec2(pauseTextSize.x + framePadding.x * 2 + 20.0f, 0);

        float windowWidth = ImGui::GetWindowWidth();
        float totalButtonWidth = playStopButtonSize.x + pauseButtonSize.x + itemSpacing;
        float offset = (windowWidth - totalButtonWidth) * 0.5f;

        if (offset > 0.0f)
            ImGui::SameLine(offset);
        else
            ImGui::SameLine();

        // ═══════════════════════════════════════════════════════════════════════
        // Play / Stop Button
        // ═══════════════════════════════════════════════════════════════════════
        if (ImGui::Button(playStopLabel, playStopButtonSize))
        {
            if (!IsPlaying())
            {
                // PLAY PRESSED

                // Save selected entity
                s_selectedBeforePlay = selectedEntity;

                // Deselect to avoid issues
                selectedEntity = INVALID_ENTITY;

                // Save scene state
                if (auto sceneMgr = Coordinator::GetInstance()->GetSystem<SceneManager>())
                {
                    s_sceneBeforePlay = sceneMgr->SerializeToString();
                }

                Engine->PlayGame();
                SetPlaying(true);
                isPaused = false;
            }
            else
            {
                // STOP PRESSED

                Engine->StopGame();

                // Restore scene
                if (auto sceneMgr = Coordinator::GetInstance()->GetSystem<SceneManager>())
                {
                    if (!s_sceneBeforePlay.empty())
                    {
                        sceneMgr->LoadFromString(s_sceneBeforePlay);
                    }
                }

                // DON'T restore selected - just clear it
                selectedEntity = INVALID_ENTITY;

                s_sceneBeforePlay.clear();
                SetPlaying(false);
                isPaused = false;
            }
        }

        ImGui::SameLine();

        // --- Pause / Resume ---
        if (!IsPlaying())
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button(pauseLabel, pauseButtonSize))
        {
            if (IsPlaying())
            {
                isPaused = !isPaused;
                s_wasManuallPaused = isPaused; // track that the pause button HAS been pressed

                if (isPaused)
                {
                    Engine->PauseGame();
                    //std::cout << "[Editor] Game Paused\n";
                }
                else
                {
                    Engine->ResumeGame();
                    //std::cout << "[Editor] Game Resumed\n";
                }
            }
        }

        // if the pause button is manually paused, enforce it every frame
        if (IsPlaying() && s_wasManuallPaused && !Engine->IsPaused())
        {
            Engine->PauseGame();
        }

        if (!IsPlaying())
        {
            ImGui::EndDisabled();
        }

        if (!IsPlaying() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Start playing to enable Pause");
        }

        // ============================================================
        // Layer Visibility Dropdown (Synced with Layer Manager)
        // This dropdown controls the same visibility as Layer Manager
        // Both systems stay in sync
        // ============================================================

        auto& registry = LayerRegistry::Get();
        const auto& allLayers = registry.GetAllLayers();

        // Count how many layers are visible
        int visibleCount = 0;
        int singleVisibleIndex = -1;
        for (size_t i = 0; i < allLayers.size(); ++i)
        {
            if (allLayers[i].visible)
            {
                visibleCount++;
                singleVisibleIndex = static_cast<int>(i);
            }
        }

        // Determine current selection for dropdown
        // Options: [Layer0, Layer1, Layer2, ..., LayerN, "Custom", "All"]
        static int s_layerFilter = -1; // -1 means "All"

        if (visibleCount == static_cast<int>(allLayers.size()))
        {
            s_layerFilter = static_cast<int>(allLayers.size()) + 1; // "All" index
        }
        else if (visibleCount == 1)
        {
            s_layerFilter = singleVisibleIndex; // Single layer visible
        }
        else
        {
            s_layerFilter = static_cast<int>(allLayers.size()); // "Custom" index
        }

        // Build options: [Layer names..., "Custom", "All"]
        static std::vector<std::string> s_layerNames;
        static std::vector<const char*> s_layerOptions;

        s_layerNames.clear();
        s_layerOptions.clear();

        for (const auto& layer : allLayers)
        {
            s_layerNames.push_back(layer.name);
        }
        s_layerNames.push_back("Custom");
        s_layerNames.push_back("All");

        for (const auto& name : s_layerNames)
        {
            s_layerOptions.push_back(name.c_str());
        }

        // Calculate positions for right-aligned placement
        ImVec2 layerTextSize = ImGui::CalcTextSize("Layers:");
        float comboWidth = 130.0f;
        float totalLayerWidth = layerTextSize.x + itemSpacing + comboWidth;

        float rightPadding = 20.0f;
        float layerX = windowWidth - totalLayerWidth - rightPadding;

        ImGui::SameLine(layerX);

        ImGui::Text("Layers:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(comboWidth);

        int previousFilter = s_layerFilter;
        int customIndex = static_cast<int>(allLayers.size());
        int allIndex = static_cast<int>(allLayers.size()) + 1;

        if (ImGui::Combo("##LayerFilter", &s_layerFilter, s_layerOptions.data(), static_cast<int>(s_layerOptions.size())))
        {
            if (s_layerFilter == customIndex)
            {
                // Can't select "Custom" directly - revert
                s_layerFilter = previousFilter;
            }
            else if (s_layerFilter == allIndex)
            {
                // Show all layers
                registry.ShowAllLayers();
                UpdateGraphicsLayerFilter();
            }
            else if (s_layerFilter >= 0 && s_layerFilter < static_cast<int>(allLayers.size()))
            {
                // Show only selected layer
                for (size_t i = 0; i < allLayers.size(); ++i)
                {
                    registry.SetLayerVisibility(allLayers[i].mask, (i == static_cast<size_t>(s_layerFilter)));
                }
                UpdateGraphicsLayerFilter();
            }
        }

        if (s_layerFilter == customIndex && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Multiple layers visible - use Layer Manager to adjust");
        }
        // ============================================================

        ImGui::PopStyleVar();
        ImGui::End();

        if (!s_pendingScenePath.empty())
        {
            // Editor "Load Scene" button should stop the game
            // (This is an editor action, not in-game navigation)
            if (IsPlaying())
            {
                //std::cout << "[Editor] Stopping game before loading scene (editor action)\n";
                
                // reset manual pause since we r force-stopping for scene load
                s_wasManuallPaused = false;
                Engine->StopGame();
                SetPlaying(false);
                isPaused = false;
            }

            if (auto sceneMgr = Coordinator::GetInstance()->GetSystem<SceneManager>())
            {
                sceneMgr->LoadSceneFromJson(s_pendingScenePath);
                //std::cout << "[Editor] Loaded scene: " << s_pendingScenePath << "\n";
                s_hasEditorBaseline = false;
                selectedEntity = INVALID_ENTITY;
            }

            s_pendingScenePath.clear();
        }
    }

} // namespace PulseEditor
