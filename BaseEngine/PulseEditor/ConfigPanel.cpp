/******************************************************************************/
/**
 * @file        ConfigPanel.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 *
 * @brief       Implementation of the game configuration editor panel,
 *              allowing runtime editing and saving of config.json values
 *              such as window width and height via ImGui interface.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "pch/pch_temp.h"
#include "ConfigPanel.h"
#include "Editor.h"
#include "CoreEngine/Core/CoreEngine.h"
#include "Serialization/Serialization.h"

#include <Windows.h>

namespace PulseEditor
{
    auto getSourceConfigPath = []() -> std::filesystem::path {
        char exePath[MAX_PATH];
        GetModuleFileNameA(GetModuleHandleA("PulseEngine.dll"), exePath, MAX_PATH);
        std::filesystem::path currentDir = std::filesystem::path(exePath).parent_path();
        auto baseEngineDir = currentDir;
        while (baseEngineDir.filename() != RootFolderName && baseEngineDir.has_parent_path())
            baseEngineDir = baseEngineDir.parent_path();
        return FilePathToGame / "JSON" / "config.json";
    };

    void DrawConfigPanel()
    {
        bool isPlaying = IsPlaying();
        if (isPlaying)
        {
            ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Stop game to edit config.");
            ImGui::BeginDisabled();
        }

        ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Game Configuration");
        ImGui::Separator();
        ImGui::Spacing();

        static int s_width = 0;
        static int s_height = 0;
        static bool s_loaded = false;

        if (!s_loaded)
        {
            s_width = config.GetWindowWidth();
            s_height = config.GetWindowHeight();
            s_loaded = true;
        }

        ImGui::Text("Window Resolution");
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputInt("Width##cfg", &s_width);
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputInt("Height##cfg", &s_height);

        if (s_width < 640)  s_width = 640;
        if (s_height < 360) s_height = 360;
        if (s_width > 3840) s_width = 3840;
        if (s_height > 2160) s_height = 2160;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled("Config path:");
        ImGui::TextWrapped("%s", config.GetConfigPath().c_str());

        ImGui::Spacing();

        if (ImGui::Button("Save Config"))
        {
            config.SetWindowWidth(s_width);
            config.SetWindowHeight(s_height);

            char exePath[MAX_PATH];
            GetModuleFileNameA(GetModuleHandleA("PulseEngine.dll"), exePath, MAX_PATH);
            std::filesystem::path currentDir = std::filesystem::path(exePath).parent_path();

            auto baseEngineDir = currentDir;
            while (baseEngineDir.filename() != RootFolderName && baseEngineDir.has_parent_path())
                baseEngineDir = baseEngineDir.parent_path();

            std::filesystem::path savePath = FilePathToGame / "JSON" / "config.json";

            std::cout << "[ConfigPanel] Saving to source: " << savePath << std::endl;

            if (SaveConfig(config, savePath.string()))
                ImGui::OpenPopup("SavedPopup");
        }

        ImGui::SameLine();

        if (ImGui::Button("Reload from File"))
        {
            char exePath[MAX_PATH];
            GetModuleFileNameA(GetModuleHandleA("PulseEngine.dll"), exePath, MAX_PATH);
            std::filesystem::path currentDir = std::filesystem::path(exePath).parent_path();

            auto baseEngineDir = currentDir;
            while (baseEngineDir.filename() != RootFolderName && baseEngineDir.has_parent_path())
                baseEngineDir = baseEngineDir.parent_path();

            std::filesystem::path sourcePath = FilePathToGame / "JSON" / "config.json";

            LoadConfig(config, sourcePath.string());
            s_width = config.GetWindowWidth();
            s_height = config.GetWindowHeight();
        }

        if (ImGui::BeginPopup("SavedPopup"))
        {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Config saved!");
            ImGui::EndPopup();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Changes apply on next launch.");

        if (isPlaying)
            ImGui::EndDisabled();
    }
}