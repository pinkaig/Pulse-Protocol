/******************************************************************************/
/**
 * @file        UserSettingsStore.cs
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 100%
 * @brief       Persists and retrieves user settings for audio, display, and latency.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Text.Json;

internal static class UserSettingsStore
{
    private const string SaveFileName = "user_settings.json";

    private sealed class Data
    {
        public float Volume { get; set; } = 1.0f;
        public float Music { get; set; } = 1.0f;
        public float SFX { get; set; } = 0.25f;
        public float Brightness { get; set; } = 1.0f;
        public float Latency { get; set; } = 0.5f;
        public bool Fullscreen { get; set; } = false;
    }

    private static bool loaded = false;
    private static Data cached = new Data();
    private static string? canonicalPath;

    public static float GetSliderValue(string tag, float fallback)
    {
        EnsureLoaded();
        string key = NormalizeTag(tag);
        if (key == "volume") return Clamp01(cached.Volume);
        if (key == "music") return Clamp01(cached.Music);
        if (key == "sfx") return Clamp01(cached.SFX);
        if (key == "brightness") return Clamp01(cached.Brightness);
        if (key == "latency") return Clamp01(cached.Latency);
        return Clamp01(fallback);
    }

    public static void SetSliderValue(string tag, float value01)
    {
        EnsureLoaded();

        float v = Clamp01(value01);
        bool changed = false;
        string key = NormalizeTag(tag);

        if (key == "volume" && Math.Abs(cached.Volume - v) > 0.0005f) { cached.Volume = v; changed = true; }
        else if (key == "music" && Math.Abs(cached.Music - v) > 0.0005f) { cached.Music = v; changed = true; }
        else if (key == "sfx" && Math.Abs(cached.SFX - v) > 0.0005f) { cached.SFX = v; changed = true; }
        else if (key == "brightness" && Math.Abs(cached.Brightness - v) > 0.0005f) { cached.Brightness = v; changed = true; }
        else if (key == "latency" && Math.Abs(cached.Latency - v) > 0.0005f) { cached.Latency = v; changed = true; }

        if (changed)
            Save();
    }

    public static bool GetFullscreen()
    {
        EnsureLoaded();
        return cached.Fullscreen;
    }

    public static void SetFullscreen(bool fullscreen)
    {
        EnsureLoaded();
        if (cached.Fullscreen == fullscreen)
            return;

        cached.Fullscreen = fullscreen;
        Save();
    }

    private static string NormalizeTag(string tag)
    {
        if (string.IsNullOrEmpty(tag))
            return "";
        return tag.Trim().ToLowerInvariant();
    }

    private static float Clamp01(float v)
    {
        if (v < 0f) return 0f;
        if (v > 1f) return 1f;
        return v;
    }

    private static void EnsureLoaded()
    {
        if (loaded)
            return;

        loaded = true;
        string targetPath = GetCanonicalPath();

        // Canonical location first.
        if (TryLoadFromPath(targetPath))
            return;

        // One-time migration read from legacy locations.
        foreach (string path in GetLegacyReadPaths())
        {
            if (TryLoadFromPath(path))
            {
                Save(); // migrate to canonical path
                return;
            }
        }
    }

    private static void Save()
    {
        string json = JsonSerializer.Serialize(cached, new JsonSerializerOptions { WriteIndented = true });
        string path = GetCanonicalPath();
        try
        {
            string? dir = Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(dir))
                Directory.CreateDirectory(dir);

            File.WriteAllText(path, json);
        }
        catch
        {
            Console.WriteLine("[UserSettingsStore] WARNING: Failed to save user_settings.json to canonical path: " + path);
        }
    }

    private static bool TryLoadFromPath(string path)
    {
        try
        {
            if (!File.Exists(path))
                return false;

            string json = File.ReadAllText(path);
            Data? data = JsonSerializer.Deserialize<Data>(json);
            if (data == null)
                return false;

            cached = data;
            return true;
        }
        catch
        {
            return false;
        }
    }

    private static string GetCanonicalPath()
    {
        if (!string.IsNullOrEmpty(canonicalPath))
            return canonicalPath!;

        string baseDir = AppContext.BaseDirectory;
        string cwd = Directory.GetCurrentDirectory();
        string exeDir = "";
        try
        {
            string? exePath = Process.GetCurrentProcess().MainModule?.FileName;
            if (!string.IsNullOrEmpty(exePath))
                exeDir = Path.GetDirectoryName(exePath) ?? "";
        }
        catch
        {
            // fallback below
        }

        // Single shared runtime location in build tree:
        //   BaseEngine/build/PulseProtocol/JSON/user_settings.json
        string? buildSharedPath = FindBuildPulseProtocolSettingsPath(exeDir)
                               ?? FindBuildPulseProtocolSettingsPath(cwd)
                               ?? FindBuildPulseProtocolSettingsPath(baseDir);

        if (!string.IsNullOrEmpty(buildSharedPath))
            canonicalPath = buildSharedPath;
        else if (!string.IsNullOrEmpty(exeDir))
            canonicalPath = Path.GetFullPath(Path.Combine(exeDir, "..", "JSON", SaveFileName));
        else
            canonicalPath = Path.GetFullPath(Path.Combine(cwd, "JSON", SaveFileName));

        // Extra safety fallback if path resolution somehow fails.
        if (string.IsNullOrWhiteSpace(canonicalPath))
            canonicalPath = Path.GetFullPath(Path.Combine(baseDir, "JSON", SaveFileName));

        return canonicalPath!;
    }

    private static string? FindBuildPulseProtocolSettingsPath(string startDir)
    {
        if (string.IsNullOrWhiteSpace(startDir))
            return null;

        try
        {
            string current = Path.GetFullPath(startDir);
            for (int i = 0; i < 12; i++)
            {
                // Case 1: current is .../build
                string candidateFromBuildRoot = Path.Combine(current, "PulseProtocol", "JSON");
                if (Directory.Exists(candidateFromBuildRoot))
                    return Path.GetFullPath(Path.Combine(candidateFromBuildRoot, SaveFileName));

                // Case 2: current is .../build/PulseProtocol/<config>
                string candidateFromConfigDir = Path.Combine(current, "..", "JSON");
                if (Directory.Exists(candidateFromConfigDir))
                    return Path.GetFullPath(Path.Combine(candidateFromConfigDir, SaveFileName));

                string? parent = Path.GetDirectoryName(current);
                if (string.IsNullOrEmpty(parent) || string.Equals(parent, current, StringComparison.OrdinalIgnoreCase))
                    break;

                current = parent;
            }
        }
        catch
        {
            // ignore and fallback
        }

        return null;
    }

    private static List<string> GetLegacyReadPaths()
    {
        string baseDir = AppContext.BaseDirectory;
        string cwd = Directory.GetCurrentDirectory();
        string canonical = GetCanonicalPath();
        var unique = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var ordered = new List<string>();

        void Add(string p)
        {
            try
            {
                string full = Path.GetFullPath(p);
                if (!full.Equals(canonical, StringComparison.OrdinalIgnoreCase) && unique.Add(full))
                    ordered.Add(full);
            }
            catch
            {
                // ignore
            }
        }

        Add(Path.Combine("PulseProtocol", "JSON", SaveFileName));
        Add(Path.Combine("..", "..", "PulseProtocol", "JSON", SaveFileName));
        Add(Path.Combine("..", "..", "..", "PulseProtocol", "JSON", SaveFileName));
        Add(Path.Combine("..", "..", "..", "..", "PulseProtocol", "JSON", SaveFileName));
        Add(Path.Combine(cwd, "PulseProtocol", "JSON", SaveFileName));
        Add(Path.Combine(baseDir, "..", "..", "..", "..", "PulseProtocol", "JSON", SaveFileName));
        Add(Path.Combine(baseDir, "JSON", SaveFileName));
        Add(SaveFileName);

        return ordered;
    }
}
