/******************************************************************************/
/**
 * @file        MessageManager.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief		Implements a singleton message manager that allows subscribing,
                unsubscribing, publishing, and broadcasting engine messages.

 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "MessageManager.h"

// ============================================================================
// SINGLETON IMPLEMENTATION - Only one MessageManager exists in the whole engine
// ============================================================================

MessageManager* MessageManager::instance = nullptr;

MessageManager* MessageManager::GetInstance() {
    if (!instance) {
        instance = new MessageManager();
    }
    return instance;
}

void MessageManager::DestroyInstance() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

// Necessary for clearing up any leftover messages
MessageManager::~MessageManager() {
    ClearQueue();
}

// ============================================================================
// OBSERVER PATTERN: Subscribe/Unsubscribe
// RegisterObserver() interface; It's like subscribing to a Utube channel
// ============================================================================

/**
 * @brief Subscribe to receive specific types of messages
 *
 * SIMPLE EXPLANATION:
 * Like clicking "Subscribe" on a YouTube channel:
 * - You say "I want to see videos about cats" (message type)
 * - You give your name (listenerName)
 * - You say what to do when you get a cat video (callback function)
 *
 * EXAMPLES:
 * RhythmGameplayInput says: "Tell me every time a beat hits so I can
 * check if player pressed the combo at the right time!"
 *
 * GraphicsManager says: "Tell me when player presses keys so I can
 * show visual feedback!"
 *
 * @param type - What kind of messages you want (INPUT_COMBO, COLLISION, etc.)
 * @param listenerName - Your system's name (so you can unsubscribe later)
 * @param callback - Function to call when message arrives
 */
void MessageManager::Subscribe(Message::Type type, const std::string& listenerName, MessageCallback callback) {
    subscribers[type][listenerName] = callback;
}

/**
 * @brief Unsubscribe from messages
 *
 * SIMPLE EXPLANATION:
 * Like clicking "Unsubscribe" on YouTube:
 * - "I don't want cat videos anymore"
 * - Remove your name from the list
 *
 * WHEN TO USE:
 * When a system shuts down (in Shutdown() function)
 * When changing scenes/levels
 *
 * @param type - What message type to stop listening to
 * @param listenerName - Your system's name
 */
void MessageManager::Unsubscribe(Message::Type type, const std::string& listenerName) {
    if (subscribers.find(type) != subscribers.end()) {
        subscribers[type].erase(listenerName);
    }
}

// ============================================================================
// OBSERVER PATTERN: Publish/Broadcast
// SendToObservers() interface; Think of this like posting on social media
// ============================================================================

/**
 * @brief Publish a message to all subscribers of that type
 *
 * SIMPLE EXPLANATION:
 * Like posting on YouTube:
 * - You upload a cat video
 * - ONLY people subscribed to your cat channel see it
 * - Everyone else doesn't get notified
 *
 * HOW IT WORKS:
 * 1. Find everyone subscribed to this message type
 * 2. Call their callback function (tell them the news)
 * 3. They handle it however they want
 *
 * Example 1 - Beat Message:
 * AudioManager: "Beat 4 just hit at 120 BPM!"
 * -> RhythmGameplay hears it: Checks if player's combo timing is correct
 * -> Graphics hears it: Flashes the beat indicator on screen
 *
 * Example 2 - Combo Message:
 * InputManager: "Player pressed W-A-W-A combo!"
 * -> RhythmGameplay hears it: Checks if it's the right weapon for enemy
 * -> Graphics hears it: Shows spear weapon activation animation
 * -> AudioManager hears it: Plays weapon sound effect
 *
 * @param message - The message to send (contains type and data)
 */
void MessageManager::Publish(const Message& message) {
    auto it = subscribers.find(message.type);
    if (it != subscribers.end()) {
        for (const auto& [name, callback] : it->second) {
            callback(message);
        }
    }
}

/**
 * @brief Broadcast message to EVERYONE (regardless of type)
 *
 * SIMPLE EXPLANATION:
 * Like a fire alarm:
 * - EVERYONE hears it, no matter what they subscribed to
 * - Use for emergency situations only
 *
 * WHEN TO USE:
 * - Game pause (all systems need to freeze)
 * - Boss defeated (everyone needs to know to trigger end sequence)
 * - Perfect combo streak (everyone celebrates with effects)
 *
 * @param message - The urgent message to broadcast
 */
void MessageManager::Broadcast(const Message& message) {
    for (auto& [type, listeners] : subscribers) {
        for (const auto& [name, callback] : listeners) {
            callback(message);
        }
    }
}

// ============================================================================
// ADVANCED FEATURE: Message Queuing (Temporal Messaging)
// Think of this as setting a reminder on your phone
// ============================================================================

/**
 * @brief Queue a message to be sent later
 *
 * SIMPLE EXPLANATION:
 * Like setting a phone reminder:
 * - "Remind me to take cookies out of oven in 10 minutes"
 * - The reminder waits, then goes off at the right time
 *
 * HOW IT WORKS:
 * - You create a message with a delay (e.g., 2 seconds)
 * - Put it in the queue (the waiting list)
 * - Later, ProcessQueue checks if it's time and sends it
 *
 * EXAMPLE:
 * Example 1 - Delayed Enemy Attack:
 * Enemy charges up for 2 beats -> Queue attack message with delay
 * After 2 beats (or 4 beats, however much we want) -> Attack automatically triggers
 *
 * Example 2 - Combo Window Timeout:
 * Player starts combo -> Queue "combo expired" message with 1 second delay
 * If player doesn't finish in 1 second -> Combo resets
 * 
 * Example 3 - Order Slip Fade:
 * Show enemy weakness UI -> Queue "fade out" message with 4-beat delay
 * After 4 beats -> UI automatically fades away
 *
 * @param message - The message to queue (must have delay set)
 */
void MessageManager::QueueMessage(Message* message) {
    if (message) {
        messageQueue.push_back(message);
        // Add to the waiting list
    }
}

/**
 * @brief Check queued messages and send ones whose time is up
 *
 * SIMPLE EXPLANATION:
 * Like checking your phone reminders every second:
 * - Look at each reminder
 * - If enough time passed: Send the reminder
 * - If not enough time: Keep waiting
 *
 * CALL THIS EVERY FRAME in your game loop!
 *
 * HOW IT WORKS:
 * 1. Look at each queued message
 * 2. Check: (currentTime - message.timestamp) >= message.delay ?
 * 3. If yes: Publish it now
 * 4. If no: Leave it in queue for next frame
 *
 * EXAMPLE:
 * Beat 0: Player sees Order Slip for enemy weakness
 * Beat 0: Queue "fade out Order Slip" message with 4-beat delay
 *         (delay = 4 beats * (60.0/120 BPM) = 2.0 seconds)
 * Beat 1: ProcessQueue checks... only 0.5s passed, not ready
 * Beat 2: ProcessQueue checks... only 1.0s passed, not ready
 * Beat 3: ProcessQueue checks... only 1.5s passed, not ready
 * Beat 4: ProcessQueue checks... 2.0s passed! Fade out now!
 *
 * @param currentTime - Current game time in seconds
 */
void MessageManager::ProcessQueue(double currentTime) {
    auto it = messageQueue.begin();
    while (it != messageQueue.end()) {
        Message* msg = *it;

        // Calculate: Has enough time passed
        // If message created at 0.0 with delay 2.0, 
        // and current time is 2.5, then yes
        if ((currentTime - msg->timestamp) >= msg->delay && !msg->processed) {

            // Publish the message to subscribers since time is up
            Publish(*msg);
            msg->processed = true;

            // Delete the message and remove from queue
            delete msg;
            it = messageQueue.erase(it);
        }
        else {
            ++it;
        }
    }
}

/**
 * @brief Clear all queued messages (cancel all reminders)
 *
 * SIMPLE EXPLANATION:
 * Like clearing all phone reminders:
 * - Cancel everything in the waiting list
 * - Delete all scheduled messages
 *
 * WHEN TO USE:
 * - When player dies: Clear all pending enemy attacks
 * - When tansitioning between levels: Clear old level's queued UI messages
 * - When pausing: Clear any timing-sensitive messages
 *
 * WHY:
 * we don't want old level's messages appearing in a new level
 */
void MessageManager::ClearQueue() {
    for (Message* msg : messageQueue) {
        delete msg; // Free memory for each message
    }
    messageQueue.clear(); // Empty the list
}