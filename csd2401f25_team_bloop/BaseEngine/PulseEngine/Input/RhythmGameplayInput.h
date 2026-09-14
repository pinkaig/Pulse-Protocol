/******************************************************************************/
/**
 * @file        RhythmGameplayInput.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Declares the InputManager system, responsible for managing
 *              GLFW keyboard inputs with rhythm-based timing. Provides:
 *                - Real-time input state tracking (per-frame key states).
 *                - Rhythm gameplay support with beat synchronization.
 *                - Sword combo detection (multi-key sequences).
 *                - Debug draw toggles via F1–F6 keys.
 *                - Integration with ECS (Transform + AudioManager).
 *                - Event broadcasting to subscribed systems (InputEvent).
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

//#pragma once
//#include "pch/pch_temp.h"
//#include "Input/InputManager.h"
//#include "CoreEngine/ECS/Coordinator.h"
//
//struct InputEvent {
//	enum class Type {
//		RHYTHM_INPUT,
//		COMBO_EXECUTED,
//	};
//
//	Type type;
//	int key;
//	double timestamp;
//	std::string feedback;
//	std::string comboName;
//
//	InputEvent(Type t, int k, double time, const std::string& fb = "", const std::string& combo = "")
//		: type(t), key(k), timestamp(time), feedback(fb), comboName(combo) {}
//};
//
//using InputCallback = std::function<void(const InputEvent&)>;
//
//class RhythmGameplayInput : public Systems {
//public:
//	RhythmGameplayInput();
//	~RhythmGameplayInput();
//
//	void Initialize();
//	void Update(float dt);
//	void Shutdown();
//
//	void setFontEntity(Entity entity) { fontEntity = entity; }
//
//    float songBPM = 120.0f;
//    float beatInterval = 60.0f / songBPM;
//    float beatTolerance = 0.3f;
//    int playerChances = 3;
//    bool canInput = true;
//    int maxComboBeats = 4;
//
//    bool canInputCheck() const { return canInput; }
//
//    void registerInputListener(const std::string& systemName, InputCallback callback);
//    void unregisterInputListener(const std::string& systemName);
//    void broadcastInputEvent(const InputEvent& event);
//
//    int getSuccessfulCombos() const { return successfulCombos; }
//
//private:
//    Entity fontEntity;
//
//    int currentBeatIndex = 0;
//    int comboStartBeat = -1;
//    double lastBeatTime = 0.0;
//    bool inputReceivedOnThisBeat = false;
//    int currentCyclePosition = 1;
//    double lastInputTime = 0.0;
//
//    std::unordered_map<std::string, InputCallback> inputListeners;
//
//    struct SwordCombo {
//        std::vector<int> keySequence;
//        std::string name;
//        SwordCombo(const std::vector<int>& keys, const std::string& n)
//            : keySequence(keys), name(n) {
//        }
//    };
//
//    std::vector<SwordCombo> swordCombos;
//    std::vector<int> currentComboSequence;
//    int successfulCombos = 0;
//    int successfulCombosThisWindow = 0;
//
//    void processRhythmInputs();
//    void processComboInputs();
//    void initializeSwordCombos();
//    void processRhythmTiming(int key);
//    std::string getTimingFeedback();
//    void checkForMissedBeat();
//};