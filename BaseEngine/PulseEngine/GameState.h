#pragma once

namespace Framework
{
    struct GameState
    {
        static bool IsPlaying();
        static void SetPlaying(bool playing);
    };
}