/******************************************************************************/
/**
 * @file        DebugConsole.h
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 * @brief		Implements an ImGui-based in-game debug console that supports
 *              scrolling log output, text filtering, command execution, message
 *              history navigation, and color-coded log levels. Provides helper
 *              functions for formatted logging as well as leveled and colored output.
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include <imgui.h>
#include <vector>
#include <string>

enum class ConsoleLevel { Trace, Debug, Info, Warn, Error, Critical };

class DebugConsole
{
public:
    DebugConsole();
    ~DebugConsole();

    void Clear();

    // Existing printf-style
    void AddLog(const char* fmt, ...);

    // New: explicit color / level helpers
    void AddLogColored(const ImVec4& col, const char* fmt, ...);
    void AddLogLevel(ConsoleLevel lvl, const char* fmt, ...);

    void Draw();

private:
    struct Item {
        ImVec4  color;
        char* text;
    };

    char                  InputBuf[256]{};
    ImVector<Item>        Items;      // colored items now
    ImVector<const char*> Commands;
    ImVector<char*>       History;
    int                   HistoryPos = -1;
    ImGuiTextFilter       Filter;
    bool                  AutoScroll = true;

    int   TextEditCallback(ImGuiInputTextCallbackData* data);
    void  ExecCommand(const char* command_line);

    static ImVec4 ColorForLevel(ConsoleLevel lvl);
    static const char* LevelToStr(ConsoleLevel lvl);
};
