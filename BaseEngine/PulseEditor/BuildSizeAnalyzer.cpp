/******************************************************************************/
/**
 * @file        BuildSizeAnalyzer.cpp
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 100%
 * @brief       Build Size Analyzer editor window for selecting and exporting assets.
 */
/******************************************************************************/

#include "pch/pch_temp.h"
#include "BuildSizeAnalyzer.h"

#define NOMINMAX
#include <Windows.h>
#include <shlobj.h>

#include <imgui.h>
#include "CoreEngine/Core/CoreEngine.h"

namespace PulseEditor
{
    namespace
    {
        struct AssetSizeEntry
        {
            std::string relativePath;
            std::string absolutePath;
            std::uintmax_t fileSizeBytes = 0;
        };

        static bool s_windowOpen = false;
        static bool s_requestOpenPopup = false;
        static bool s_initialized = false;
        static bool s_loadedSelectionFromDisk = false;
        static std::filesystem::path s_assetsRoot;
        static std::vector<AssetSizeEntry> s_assets;
        static std::unordered_map<std::string, bool> s_selected;
        static std::string s_statusMessage;

        std::filesystem::path GetAssetsRootPath()
        {
            namespace fs = std::filesystem;

            if (!FilePathToGame.empty())
            {
                fs::path pathFromConfig = FilePathToGame / "Assets";
                std::error_code ec;
                if (fs::exists(pathFromConfig, ec) && fs::is_directory(pathFromConfig, ec))
                {
                    return fs::weakly_canonical(pathFromConfig, ec);
                }
            }

            fs::path current = fs::current_path();
            for (int i = 0; i < 16; ++i)
            {
                fs::path candidate = current / "PulseProtocol" / "Assets";
                std::error_code ec;
                if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec))
                {
                    return fs::weakly_canonical(candidate, ec);
                }

                fs::path parent = current.parent_path();
                if (parent == current)
                {
                    break;
                }
                current = parent;
            }

            return {};
        }

        std::filesystem::path GetSelectionStateFilePath()
        {
            namespace fs = std::filesystem;

            if (!FilePathToGame.empty())
            {
                return FilePathToGame / "JSON" / "BuildSizeAnalyzerSelection.txt";
            }

            if (!s_assetsRoot.empty())
            {
                return s_assetsRoot.parent_path() / "JSON" / "BuildSizeAnalyzerSelection.txt";
            }

            return fs::current_path() / "BuildSizeAnalyzerSelection.txt";
        }

        std::string FormatBytes(std::uintmax_t bytes)
        {
            static const char* units[] = { "B", "KB", "MB", "GB", "TB" };
            double value = static_cast<double>(bytes);
            int unitIndex = 0;
            while (value >= 1024.0 && unitIndex < 4)
            {
                value /= 1024.0;
                ++unitIndex;
            }

            std::ostringstream out;
            out << std::fixed << std::setprecision((unitIndex == 0) ? 0 : 2) << value << " " << units[unitIndex];
            return out.str();
        }

        void SaveSelectionToDisk()
        {
            namespace fs = std::filesystem;
            std::error_code ec;

            const fs::path stateFile = GetSelectionStateFilePath();
            fs::create_directories(stateFile.parent_path(), ec);

            std::ofstream out(stateFile);
            if (!out.is_open())
            {
                return;
            }

            for (const auto& [path, isSelected] : s_selected)
            {
                if (isSelected)
                {
                    out << path << "\n";
                }
            }
        }

        void LoadSelectionFromDisk()
        {
            namespace fs = std::filesystem;
            s_selected.clear();
            s_loadedSelectionFromDisk = false;

            const fs::path stateFile = GetSelectionStateFilePath();
            std::ifstream in(stateFile);
            if (!in.is_open())
            {
                return;
            }

            s_loadedSelectionFromDisk = true;
            std::string line;
            while (std::getline(in, line))
            {
                if (!line.empty())
                {
                    s_selected[line] = true;
                }
            }
        }

        void RefreshAssetList()
        {
            namespace fs = std::filesystem;
            s_assets.clear();

            if (s_assetsRoot.empty())
            {
                return;
            }

            std::unordered_map<std::string, bool> previousSelection = s_selected;
            std::error_code ec;
            for (fs::recursive_directory_iterator it(s_assetsRoot, ec), end; it != end; it.increment(ec))
            {
                if (ec)
                {
                    continue;
                }

                if (!it->is_regular_file(ec))
                {
                    continue;
                }

                const fs::path absPath = it->path();
                const fs::path relPath = fs::relative(absPath, s_assetsRoot, ec);
                if (ec || relPath.empty())
                {
                    continue;
                }

                std::uintmax_t size = it->file_size(ec);
                if (ec)
                {
                    size = 0;
                }

                AssetSizeEntry entry;
                entry.relativePath = relPath.generic_string();
                entry.absolutePath = absPath.string();
                entry.fileSizeBytes = size;
                s_assets.push_back(std::move(entry));
            }

            std::sort(s_assets.begin(), s_assets.end(),
                [](const AssetSizeEntry& a, const AssetSizeEntry& b)
                {
                    return a.relativePath < b.relativePath;
                });

            s_selected.clear();
            for (const auto& asset : s_assets)
            {
                auto it = previousSelection.find(asset.relativePath);
                if (it != previousSelection.end())
                {
                    s_selected[asset.relativePath] = it->second;
                }
                else
                {
                    s_selected[asset.relativePath] = !s_loadedSelectionFromDisk;
                }
            }
        }

        bool EnsureInitialized()
        {
            if (s_initialized)
            {
                return !s_assetsRoot.empty();
            }

            s_assetsRoot = GetAssetsRootPath();
            if (s_assetsRoot.empty())
            {
                s_statusMessage = "Could not find Assets folder.";
                s_initialized = true;
                return false;
            }

            LoadSelectionFromDisk();
            RefreshAssetList();
            s_initialized = true;
            return true;
        }

        std::uintmax_t GetSelectedSizeBytes(int* outSelectedCount = nullptr)
        {
            std::uintmax_t total = 0;
            int selectedCount = 0;
            for (const auto& asset : s_assets)
            {
                auto it = s_selected.find(asset.relativePath);
                if (it != s_selected.end() && it->second)
                {
                    total += asset.fileSizeBytes;
                    ++selectedCount;
                }
            }

            if (outSelectedCount)
            {
                *outSelectedCount = selectedCount;
            }
            return total;
        }

        void SetAllSelections(bool value)
        {
            for (const auto& asset : s_assets)
            {
                s_selected[asset.relativePath] = value;
            }
            SaveSelectionToDisk();
        }

        std::string OpenFolderPickerDialog()
        {
            BROWSEINFOA bi{};
            bi.lpszTitle = "Select export destination folder";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

            LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
            if (!pidl)
            {
                return {};
            }

            char folderPath[MAX_PATH] = {};
            const bool ok = SHGetPathFromIDListA(pidl, folderPath) == TRUE;
            CoTaskMemFree(pidl);

            if (!ok)
            {
                return {};
            }

            return std::string(folderPath);
        }

        void ExportSelectedAssets()
        {
            namespace fs = std::filesystem;
            const std::string destinationFolder = OpenFolderPickerDialog();
            if (destinationFolder.empty())
            {
                s_statusMessage = "Export canceled.";
                return;
            }

            std::error_code ec;
            fs::path destinationRoot = fs::path(destinationFolder);
            fs::create_directories(destinationRoot, ec);

            int exportedCount = 0;
            int failedCount = 0;
            std::uintmax_t exportedBytes = 0;

            for (const auto& asset : s_assets)
            {
                auto it = s_selected.find(asset.relativePath);
                if (it == s_selected.end() || !it->second)
                {
                    continue;
                }

                const fs::path src = fs::path(asset.absolutePath);
                const fs::path dst = destinationRoot / fs::path(asset.relativePath);

                fs::create_directories(dst.parent_path(), ec);
                fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
                if (ec)
                {
                    ++failedCount;
                    ec.clear();
                }
                else
                {
                    ++exportedCount;
                    exportedBytes += asset.fileSizeBytes;
                }
            }

            std::ostringstream msg;
            msg << "Exported " << exportedCount << " file(s), "
                << FormatBytes(exportedBytes) << " total";
            if (failedCount > 0)
            {
                msg << ". Failed: " << failedCount;
            }
            s_statusMessage = msg.str();
        }
    }

    void OpenBuildSizeAnalyzerWindow()
    {
        s_windowOpen = true;
        s_requestOpenPopup = true;
        if (!EnsureInitialized())
        {
            return;
        }

        RefreshAssetList();
    }

    void DrawBuildSizeAnalyzerMenu()
    {
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Build Size Analyzer"))
            {
                OpenBuildSizeAnalyzerWindow();
            }
            ImGui::EndMenu();
        }
    }

    void DrawBuildSizeAnalyzerWindow()
    {
        if (!s_windowOpen)
        {
            return;
        }

        if (s_requestOpenPopup)
        {
            ImGui::OpenPopup("Build Size Analyzer");
            s_requestOpenPopup = false;
        }

        const bool wasOpen = s_windowOpen;
        ImGui::SetNextWindowSize(ImVec2(900.0f, 520.0f), ImGuiCond_Appearing);
        const ImGuiWindowFlags modalFlags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoCollapse;

        if (!ImGui::BeginPopupModal("Build Size Analyzer", &s_windowOpen, modalFlags))
        {
            if (wasOpen && !s_windowOpen)
            {
                SaveSelectionToDisk();
            }
            return;
        }

        if (!EnsureInitialized())
        {
            ImGui::TextUnformatted(s_statusMessage.c_str());
            ImGui::EndPopup();
            return;
        }

        if (ImGui::Button("Refresh"))
        {
            RefreshAssetList();
        }

        ImGui::SameLine();
        if (ImGui::Button("Select All"))
        {
            SetAllSelections(true);
        }

        ImGui::SameLine();
        if (ImGui::Button("Deselect All"))
        {
            SetAllSelections(false);
        }

        ImGui::SameLine();
        if (ImGui::Button("Export Selected"))
        {
            ExportSelectedAssets();
        }

        ImGui::Separator();
        ImGui::Text("Assets Root: %s", s_assetsRoot.string().c_str());
        ImGui::Separator();

        if (ImGui::BeginChild("BuildSizeAnalyzerList", ImVec2(0.0f, -70.0f), true))
        {
            if (ImGui::BeginTable("BuildSizeAnalyzerTable", 3,
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Borders |
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Select", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                ImGui::TableSetupColumn("Asset Path", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("File Size", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableHeadersRow();

                bool selectionChanged = false;
                for (std::size_t i = 0; i < s_assets.size(); ++i)
                {
                    const AssetSizeEntry& asset = s_assets[i];
                    bool selected = s_selected[asset.relativePath];

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    std::string checkboxId = "##asset_select_" + std::to_string(i);
                    if (ImGui::Checkbox(checkboxId.c_str(), &selected))
                    {
                        s_selected[asset.relativePath] = selected;
                        selectionChanged = true;
                    }

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(asset.relativePath.c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(FormatBytes(asset.fileSizeBytes).c_str());
                }

                if (selectionChanged)
                {
                    SaveSelectionToDisk();
                }

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();

        int selectedCount = 0;
        const std::uintmax_t selectedBytes = GetSelectedSizeBytes(&selectedCount);
        ImGui::Text("Selected: %d / %d files", selectedCount, static_cast<int>(s_assets.size()));
        ImGui::Text("Total Selected Size: %s", FormatBytes(selectedBytes).c_str());

        if (!s_statusMessage.empty())
        {
            ImGui::TextWrapped("%s", s_statusMessage.c_str());
        }

        ImGui::EndPopup();

        if (wasOpen && !s_windowOpen)
        {
            SaveSelectionToDisk();
        }
    }
}

