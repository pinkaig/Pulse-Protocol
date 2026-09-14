#include "GameState.h"

namespace Framework
{
    static bool gPlaying = false;

    bool GameState::IsPlaying() { return gPlaying; }
    void GameState::SetPlaying(bool playing) { gPlaying = playing; }
}