/******************************************************************************/
/**
 * @file        DebugConsole.cpp
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 * @brief		ImGui-powered in-game debug console: renders a filterable, auto-scrolling
 *              log with color-coded levels; supports simple commands (HELP, CLEAR, HISTORY),
 *              input history, and helpers for plain/colored/leveled logging.
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "DebugConsole.h"
#include <cctype>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#ifdef _MSC_VER
#define STRDUP _strdup   // silence POSIX deprecation on MSVC
#else
#define STRDUP strdup
#endif

int DebugConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
{
    IM_UNUSED(data);
    return 0;
}

static int Stricmp(const char* s1, const char* s2)
{
    int d;
    while ((d = std::toupper(*s2) - std::toupper(*s1)) == 0 && *s1) { s1++; s2++; }
    return d;
}

static std::string vformat(const char* fmt, va_list args)
{
    char buf[1024];
    va_list cp; va_copy(cp, args);
    int n = vsnprintf(buf, sizeof(buf), fmt, cp);
    va_end(cp);
    if (n < 0) return {};
    if (n < (int)sizeof(buf)) return std::string(buf, n);
    std::string out; out.resize(n + 1);
    vsnprintf(out.data(), out.size(), fmt, args);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

ImVec4 DebugConsole::ColorForLevel(ConsoleLevel lvl)
{
    switch (lvl) {
    case ConsoleLevel::Trace:    return ImVec4(0.60f, 0.60f, 0.60f, 1.0f); // grey
    case ConsoleLevel::Debug:    return ImVec4(0.70f, 0.85f, 1.00f, 1.0f); // light blue
    case ConsoleLevel::Info:     return ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // near white
    case ConsoleLevel::Warn:     return ImVec4(1.00f, 0.80f, 0.25f, 1.0f); // amber
    case ConsoleLevel::Error:    return ImVec4(1.00f, 0.40f, 0.40f, 1.0f); // red
    case ConsoleLevel::Critical: return ImVec4(1.00f, 0.20f, 0.20f, 1.0f); // deeper red
    }
    return ImVec4(1, 1, 1, 1);
}

const char* DebugConsole::LevelToStr(ConsoleLevel lvl)
{
    switch (lvl) {
    case ConsoleLevel::Trace:    return "TRACE";
    case ConsoleLevel::Debug:    return "DEBUG";
    case ConsoleLevel::Info:     return "INFO";
    case ConsoleLevel::Warn:     return "WARN";
    case ConsoleLevel::Error:    return "ERROR";
    case ConsoleLevel::Critical: return "CRIT";
    }
    return "";
}

DebugConsole::DebugConsole()
{
    Clear();
    Commands.push_back("HELP");
    Commands.push_back("CLEAR");
    Commands.push_back("HISTORY");
}

DebugConsole::~DebugConsole() { Clear(); }

void DebugConsole::Clear()
{
    for (int i = 0; i < Items.Size; ++i)
        free(Items[i].text);
    Items.clear();
    History.clear();
}

void DebugConsole::AddLog(const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    std::string s = vformat(fmt, args);
    va_end(args);

    Item it;
    it.color = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // default = Info color-ish
    it.text = STRDUP(s.c_str());
    Items.push_back(it);
}

void DebugConsole::AddLogColored(const ImVec4& col, const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    std::string s = vformat(fmt, args);
    va_end(args);

    Item it;
    it.color = col;
    it.text = STRDUP(s.c_str());
    Items.push_back(it);
}

void DebugConsole::AddLogLevel(ConsoleLevel lvl, const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    std::string s = vformat(fmt, args);
    va_end(args);

    // Prefix with level tag for readability + filtering
    std::string line = "[" + std::string(LevelToStr(lvl)) + "] " + s;

    Item it;
    it.color = ColorForLevel(lvl);
    it.text = STRDUP(line.c_str());
    Items.push_back(it);
}

void DebugConsole::ExecCommand(const char* command_line)
{
    AddLogLevel(ConsoleLevel::Info, "> %s", command_line);

    if (Stricmp(command_line, "CLEAR") == 0)
        Clear();
    else if (Stricmp(command_line, "HELP") == 0) {
        AddLogLevel(ConsoleLevel::Info, "Commands: CLEAR, HELP, HISTORY");
    }
    else
        AddLogLevel(ConsoleLevel::Warn, "Unknown command: '%s'", command_line);

    History.push_back(STRDUP(command_line));
}

void DebugConsole::Draw()
{
    // Filter + controls row (optional)
    ImGui::AlignTextToFramePadding();
    Filter.Draw("Filter", 180);
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &AutoScroll);
    ImGui::Separator();

    // Log window
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false))
    {
        for (int i = 0; i < Items.Size; ++i)
        {
            const Item& it = Items[i];
            if (!Filter.PassFilter(it.text))
                continue;

            ImGui::PushStyleColor(ImGuiCol_Text, it.color);
            ImGui::TextUnformatted(it.text);
            ImGui::PopStyleColor();
        }

        if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    // Input field
    if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf),
        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        ExecCommand(InputBuf);
        strcpy(InputBuf, "");
    }
}
