/******************************************************************************/
/**
 * @file        UndoManager.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Undo Manager contains a "stack" of actions or steps that can be
 *				undo/redo up to a maximum of 40 steps will be stored in
 *				MAX_HISTORY
 * 
 *				MAX_HISTORY is the variable for how many "steps"
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#pragma once
#include <vector>
#include <functional>
class UndoManager {
public:
	//std::function -> general purpose polymorphic function wrapper.
	//A better function ptor that can store ANY callable object
	//To be called when we need to undo
	
	using Command = std::function<void()>;
	struct Action {
        Command undo;
        Command redo;
    };
	void Push(Action action);
	void Undo();
	void Redo();
	//checks whether the undo stack is empty
	//returns true if we can undo
   	bool CanUndo() const;
    bool CanRedo() const;
	//void Clear(); //clear the stack when changing scenes
	void ClearUndo();
    void ClearRedo();
	operator bool() const noexcept;
	size_t Capacity() const noexcept;
private:
	bool m_applying = false;  // blocks re-entrant Push() calls during undo/redo execution
	//keep track of history for undo
	static constexpr size_t MAX_HISTORY = 40;
	// stack of undo commands (LIFO)
	// Last-In-First-Out, store the previous step
	std::vector<Action> undo_stack;
	std::vector<Action> redo_stack;
	// Trims history by erasing oldest action
	void TrimUndoHistory();
	void TrimRedoHistory();
};