/******************************************************************************/
/**
 * @file        RhythmGameplayInput.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Implements the InputManager system, handling GLFW-based
 *              keyboard input for rhythm gameplay. Provides real-time input
 *              state tracking, combo detection, rhythm timing feedback,
 *              event broadcasting to subscribed systems, debug toggles.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
/*
#include "Input/RhythmGameplayInput.h"
#include "CoreEngine/Core/CoreEngine.h"
#include "Components/DisplayName.h"

static std::string keyToString(int key)
{
    switch (key)
    {
    case GLFW_KEY_W:
        return "W";
    case GLFW_KEY_A:
        return "A";
    case GLFW_KEY_S:
        return "S";
    case GLFW_KEY_D:
        return "D";
    case GLFW_KEY_Z:
        return "Z";
    default:
        return std::to_string(key);
    }
}

RhythmGameplayInput::RhythmGameplayInput()
{
    successfulCombos = 0;
    successfulCombosThisWindow = 0;
    currentCyclePosition = 1;
    initializeSwordCombos();
}

RhythmGameplayInput::~RhythmGameplayInput()
{
    Shutdown();
}

void RhythmGameplayInput::Initialize()
{
    // std::cout << "[RhythmGameplayInput] Successfully initialized." << std::endl;
    // std::cout << "[COMBOS READY] Available combos: Basic Strike (W->A), Swift Slash (S->D), Power Combo (A->S->D)" << std::endl;

    currentBeatIndex = 0;
    comboStartBeat = -1;
    lastBeatTime = 0.0;
    currentCyclePosition = 1;

    fontEntity = 0;

    // Initialize combos
    initializeSwordCombos();
}

void RhythmGameplayInput::Update(float deltaTime)
{
    (void)deltaTime;

    if (!Engine->IsPlaying()) {
        return;
    }

    // Don't process rhythm inputs on menus
    auto* coord = Coordinator::GetInstance();
    auto sceneManager = coord->GetSystem<SceneManager>();
    if (sceneManager)
    {
        std::string currentScene = sceneManager->GrabCurrentScene();
        if (currentScene.find("MainMenu") != std::string::npos)
            //currentScene.find("Settings") != std::string::npos ||
            //currentScene.find("Credits") != std::string::npos)
        {
            return;
        }
    }

    processRhythmInputs();
}

void RhythmGameplayInput::Shutdown()
{
    inputListeners.clear();
    std::cout << "[RhythmGameplayInput] Shutdown completed." << std::endl;
}

void RhythmGameplayInput::registerInputListener(const std::string &systemName, InputCallback callback)
{
    inputListeners[systemName] = callback;
}

void RhythmGameplayInput::unregisterInputListener(const std::string &systemName)
{
    inputListeners.erase(systemName);
}

void RhythmGameplayInput::broadcastInputEvent(const InputEvent &event)
{
    auto *g_coordinator = Coordinator::GetInstance();
    if (event.type == InputEvent::Type::RHYTHM_INPUT)
    {
        auto &transform1 = g_coordinator->GetComponent<Framework::Transform>(fontEntity);
        transform1.isVisible = true;
        transform1.visibilityTimer = 2.0f;
        transform1.autoHide = true;
    }

    for (const auto &[systemName, callback] : inputListeners)
    {
        callback(event);
    }
}

void RhythmGameplayInput::processRhythmInputs()
{
    auto *g_coordinator = Coordinator::GetInstance();
    auto inputMgr = g_coordinator->GetSystem<InputManager>();

     if (!Engine->IsPlaying()) {
         return;
     }

    const int rhythmKeys[] = {GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D};
    const char *keyNames[] = {"W", "A", "S", "D"};

    for (int i = 0; i < 4; ++i)
    {
        if (inputMgr->isKeyTriggered(rhythmKeys[i]))
        {

            std::cout << "[KEY DETECTED] " << keyToString(rhythmKeys[i]) << " key pressed!" << std::endl;

            // Show feedback bar by setting alpha to 1.0
            //if (g_coordinator->HasComponent<GUIButton>(fontEntity))
            //{
            //    auto &button = g_coordinator->GetComponent<GUIButton>(fontEntity);
            //    button.normalColor.a = 1.0f; // Make visible
            //    button.hoverColor.a = 1.0f;

            //    auto &transform = g_coordinator->GetComponent<Framework::Transform>(fontEntity);
            //    transform.visibilityTimer = 2.0f;
            //    transform.autoHide = true;
            //}

            processRhythmTiming(rhythmKeys[i]);
            broadcastInputEvent(InputEvent(InputEvent::Type::RHYTHM_INPUT, rhythmKeys[i], glfwGetTime()));
        }
    }
}

void RhythmGameplayInput::processComboInputs()
{
    if (currentComboSequence.empty())
        return;

    for (const auto &combo : swordCombos)
    {
        if (currentComboSequence.size() == combo.keySequence.size())
        {
            if (currentComboSequence == combo.keySequence)
            {
                successfulCombos++;
                successfulCombosThisWindow++;

                bool isPartOfLongerCombo = false;
                for (const auto &longerCombo : swordCombos)
                {
                    if (longerCombo.keySequence.size() > combo.keySequence.size())
                    {
                        bool matches = true;
                        for (size_t i = 0; i < combo.keySequence.size(); ++i)
                        {
                            if (longerCombo.keySequence[i] != combo.keySequence[i])
                            {
                                matches = false;
                                break;
                            }
                        }
                        if (matches)
                        {
                            isPartOfLongerCombo = true;
                            break;
                        }
                    }
                }

                if (!isPartOfLongerCombo)
                {
                    InputEvent comboEvent(InputEvent::Type::COMBO_EXECUTED, 0, glfwGetTime(), "COMBO", combo.name);
                    broadcastInputEvent(comboEvent);
                    currentComboSequence.clear();
                }
                return;
            }
        }
    }

    bool isValidPrefix = false;
    for (const auto &combo : swordCombos)
    {
        if (currentComboSequence.size() < combo.keySequence.size())
        {
            bool matches = true;
            for (size_t i = 0; i < currentComboSequence.size(); ++i)
            {
                if (currentComboSequence[i] != combo.keySequence[i])
                {
                    matches = false;
                    break;
                }
            }
            if (matches)
            {
                isValidPrefix = true;
                break;
            }
        }
    }

    if (!isValidPrefix)
    {
        currentComboSequence.clear();
    }
}

void RhythmGameplayInput::initializeSwordCombos()
{
    swordCombos.clear();
    swordCombos.emplace_back(std::vector<int>{GLFW_KEY_W, GLFW_KEY_A}, "Basic Strike");
    swordCombos.emplace_back(std::vector<int>{GLFW_KEY_S, GLFW_KEY_D}, "Swift Slash"); // not yet implemented
    swordCombos.emplace_back(std::vector<int>{GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D}, "Power Combo"); // not yet implemented
}

std::string RhythmGameplayInput::getTimingFeedback()
{
    auto* g_coordinator = Coordinator::GetInstance();
    auto audioSystem = g_coordinator->GetSystem<AudioManager>();

    if (!audioSystem)
        return "MISS";

    double musicPosition = audioSystem->GetBGMPositionSeconds();
    if (musicPosition <= 0.0)
        return "MISS";

    double beatPosition = fmod(musicPosition + 0.1, beatInterval);
    double distance = std::min(beatPosition, beatInterval - beatPosition);

    if (distance <= 0.075)
        return "PERFECT";
    if (distance <= 0.10)
        return "GOOD";
    return "MISS";
}

void RhythmGameplayInput::processRhythmTiming(int key)
{
    if (!canInput)
        return;

    auto* g_coordinator = Coordinator::GetInstance();

    currentComboSequence.push_back(key);

    if (currentComboSequence.size() == 2)
    {
        if (currentComboSequence[0] == GLFW_KEY_W && currentComboSequence[1] == GLFW_KEY_A)
        {
            // COMBO SUCCESS - no key sound, PlayerScript plays sword swing
            std::string feedback = getTimingFeedback();
            if (feedback == "MISS") feedback = "GOOD";

            std::cout << "[" << feedback << "] COMBO: Basic Strike!" << std::endl;

            broadcastInputEvent(InputEvent(InputEvent::Type::COMBO_EXECUTED, key, glfwGetTime(), feedback, "Basic Strike"));

            currentComboSequence.clear();
        }
        else
        {
            // WRONG COMBO - play fail sound
            std::cout << "[MISS] Wrong combo sequence!" << std::endl;

            if (auto audio = g_coordinator->GetSystem<AudioManager>())
            {
                audio->PlaySound("SFX_WrongInput");
            }

            broadcastInputEvent(InputEvent(InputEvent::Type::COMBO_EXECUTED, key, glfwGetTime(), "MISS", ""));

            currentComboSequence.clear();
        }
    }
    else if (currentComboSequence.size() == 1)
    {
        if (currentComboSequence[0] == GLFW_KEY_W)
        {
            // FIRST KEY CORRECT - play W sound
            if (auto audio = g_coordinator->GetSystem<AudioManager>())
            {
                audio->PlaySound("SFX_W_Key");
            }
        }
        else
        {
            // WRONG START KEY - play fail sound
            std::cout << "[MISS] Wrong start key!" << std::endl;

            if (auto audio = g_coordinator->GetSystem<AudioManager>())
            {
                audio->PlaySound("SFX_WrongInput");
            }

            broadcastInputEvent(InputEvent(InputEvent::Type::COMBO_EXECUTED, key, glfwGetTime(), "MISS", ""));

            currentComboSequence.clear();
        }
    }
}

void RhythmGameplayInput::checkForMissedBeat()
{
    if (!Engine->IsPlaying()) {
        return;
    }

    if (!canInput)
        return;
    auto *g_coordinator = Coordinator::GetInstance();
    auto audioSystem = g_coordinator->GetSystem<AudioManager>();
    if (!audioSystem)
    {
        return;
    }

    double currentTime = audioSystem->GetBGMPositionSeconds();

    if (currentTime - lastBeatTime < beatInterval)
    {
        return;
    }

    lastBeatTime = currentTime - fmod(currentTime, beatInterval);
    currentBeatIndex++;

    currentCyclePosition = ((currentBeatIndex - 1) % 8) + 1;

    if (currentCyclePosition >= 1 && currentCyclePosition <= 4)
    {
        std::cout << "[INCOMING] GET READY.." << std::endl;
        inputReceivedOnThisBeat = false;
    }
    else if (currentCyclePosition >= 5 && currentCyclePosition <= 8)
    {
        if (currentCyclePosition == 5)
        {
            inputReceivedOnThisBeat = false;
        }

        if (currentCyclePosition > 5 && !inputReceivedOnThisBeat)
        {
            std::cout << "[MISS] NO INPUT DETECTED" << std::endl;
        }

        if (currentCyclePosition == 8)
        {
            if (successfulCombosThisWindow == 0)
            {
                playerChances--;
                std::cout << "[MISS] No valid combo completed | Chances left: " << playerChances << std::endl;
            }

            currentComboSequence.clear();
            successfulCombosThisWindow = 0;
            comboStartBeat = currentBeatIndex + 1;
        }
    }

    if (playerChances <= 0)
    {
        canInput = false;
        std::cout << "[YOU DIED!] No more chances." << std::endl;
        if (audioSystem)
            audioSystem->StopBGM();
    }

    if (currentCyclePosition <= 4)
    {
        inputReceivedOnThisBeat = false;
    }
}
*/