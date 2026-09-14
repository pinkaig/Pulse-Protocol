/******************************************************************************/
/**
 * @file        MessageManager.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief		Defines the messaging system and central manager for
                subscribing, publishing, and broadcasting engine events.

 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include "pch/pch_temp.h"
#include "Math/math.h"

// Base Message class
class Message {
public:
    enum class Type {
        INPUT_RHYTHM,
        INPUT_COMBO,
        COLLISION_HIT,
        AUDIO_BEAT
    };

    //enum class Priority {
    //    LOW,        // Background events, logging
    //    NORMAL,     // Standard gameplay events
    //    HIGH,       // Critical gameplay events
    //    CRITICAL    // System-level events (pause, game over)
    //};

    Type type;              // Message category
    //Priority priority;      // Message importance level
    double timestamp;       // When the message was created
    double delay;           // Delay before processing (seconds)
    bool processed;         // Has this queued message been processed?

    Message(Type t, double time, double d = 0.0) : type(t), timestamp(time), delay(d), processed(false) {}
    virtual ~Message() = default;
};

// Input combo message
class InputComboMessage : public Message {
public:
    std::string comboName;

    InputComboMessage(const std::string& combo, double time)
        : Message(Type::INPUT_COMBO, time), comboName(combo) {
    }
};

// Collision message
class CollisionMessage : public Message {
public:
    Entity entityA;
    Entity entityB;
    Vector2 collisionPoint;

    CollisionMessage(Entity a, Entity b, Vector2 point, double time)
        : Message(Type::COLLISION_HIT, time), entityA(a), entityB(b), collisionPoint(point) {
    }
};

// Audio beat message
class AudioBeatMessage : public Message {
public:
    int beatNumber;
    double bpm;

    AudioBeatMessage(int beat, double beatsPerMinute, double time)
        : Message(Type::AUDIO_BEAT, time), beatNumber(beat), bpm(beatsPerMinute) {
    }
};

/**
 * @brief MessageManager - Observer/Observable messaging system
 *
 * OBSERVER PATTERN IMPLEMENTATION (Lecture Slides 18-22):
 * - Observable Interface: Subscribe(), Unsubscribe(), Publish()
 * - Observer Interface: Callback registration via Subscribe()
 * - Loose coupling: Publishers and subscribers don't know each other
 *
 * ADVANCED FEATURE (Lecture Slide 24):
 * - Message queuing: Messages can be queued for delayed delivery
 *
 * USAGE:
 * // Subscribe (Observer):
 * msgMgr->Subscribe(Message::Type::INPUT_COMBO, "GraphicsSystem",
 *     [](const Message& msg) { HandleMessage(msg); });
 *
 * // Publish (Observable):
 * InputComboMessage msg("COMBO", time);
 * msgMgr->Publish(msg);
 *
 * // Queue delayed message:
 * InputComboMessage* delayed = new InputComboMessage("DELAYED", time);
 * delayed->delay = 2.0;
 * msgMgr->QueueMessage(delayed);
 */
class MessageManager {
public:
    using MessageCallback = std::function<void(const Message&)>;

    static MessageManager* GetInstance();
    static void DestroyInstance();

    // Observer Pattern: Subscribe/Unsubscribe (RegisterObserver interface from lecture)
    void Subscribe(Message::Type type, const std::string& listenerName, MessageCallback callback);
    void Unsubscribe(Message::Type type, const std::string& listenerName);

    // Observer Pattern: Publish (SendToObservers interface from lecture)
    void Publish(const Message& message);
    void Broadcast(const Message& message);

    // Advanced Feature: Message Queuing (temporal messaging)
    void QueueMessage(Message* message);
    void ProcessQueue(double currentTime);
    void ClearQueue();

private:
    MessageManager() = default;
    ~MessageManager();

    static MessageManager* instance;

    // Observer Pattern: List of observers per message type
    std::unordered_map<Message::Type, std::unordered_map<std::string, MessageCallback>> subscribers;

    // Advanced Feature: Message queue for delayed delivery
    std::vector<Message*> messageQueue;
};