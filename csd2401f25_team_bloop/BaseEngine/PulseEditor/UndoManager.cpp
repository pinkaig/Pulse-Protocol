/******************************************************************************/
/**
 * @file        UndoManager.cpp
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Implementation of class UndoManager member functions
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "UndoManager.h"
#include <iostream>
#include <utility>

namespace {
	/*
	RAII guard for undo/redo
	prevents double undo/redo on a single action
	*/
	struct ApplyingGuard {
		bool& flag;
		explicit ApplyingGuard(bool& f) : flag(f) { flag = true; }
		~ApplyingGuard() { flag = false; }
	};
}

//erases only from undostack, erases OLDEST action
void UndoManager::TrimUndoHistory() 
{
	if (undo_stack.size() > MAX_HISTORY) {
		undo_stack.erase(undo_stack.begin(),
			undo_stack.begin() + (undo_stack.size() - MAX_HISTORY));
	}
}
void UndoManager::TrimRedoHistory()
{
	if (redo_stack.size() > MAX_HISTORY) {
		redo_stack.erase(redo_stack.begin(),
			redo_stack.begin() + (redo_stack.size() - MAX_HISTORY));
	}
}
void UndoManager::Push(Action a) {
	if (m_applying) return;  // block re-entrant pushes during undo/redo
	// undo, redo return if no callables
	if (!a.undo)
		return;
	if (!a.redo)
		return;

	// new action invalidates redo history
	// if you do something in between UNDOs and Redos
	redo_stack.clear();
	//add most recent "action" or "step"
	//to the back of the undo_stack
	//std::cout << "[UndoManager] Push, size before = " << undo_stack.size() << "\n";
	undo_stack.push_back(std::move(a));
	TrimUndoHistory();
	//std::cout << "[UndoManager] size after = " << undo_stack.size() << "\n";
}
void UndoManager::Undo()
{
	if (undo_stack.empty()) {
		return;
	}
	Action a = std::move(undo_stack.back());
	undo_stack.pop_back();
    // undo the action
	{
		ApplyingGuard g(m_applying);
		if (a.undo) { //if the function exist then call it
			a.undo();
			std::cout << "[UndoManager] Sucessfully Undo-ed an Action\n";
		}
		else {
			std::cout << "[UndoManager] ERROR: undo is null\n";
			return; // Don't push a broken "action" to Redo
		}
	}//in a scope, applying guard dtor called at end of this

	if (a.redo)  // only push if redo exists
	{
		redo_stack.push_back(std::move(a));
		TrimRedoHistory();
	}
	//std::cout << "[UndoManager] size after undo = " << undo_stack.size() << "\n";
}

void UndoManager::Redo()
{
	if (redo_stack.empty()) {
		return;
	}
	//add the action happened to the stack
    Action a = std::move(redo_stack.back());
	redo_stack.pop_back();

	//guard
	{ //in a scope, applying guard dtor called at end of this
		ApplyingGuard g(m_applying);
		if (a.redo) {
			a.redo();
			std::cout << "[UndoManager] Sucessfully Redo-ed an Action\n";
		}
		else {
			std::cout << "[UndoManager] ERROR: redo is null\n";
			return;
		}			
	}// g should be done here

	//Move back to undo_stack and trim THE UNDO stack

	if (a.undo) {
		undo_stack.push_back(std::move(a)); // now undo-able again
		TrimUndoHistory();
	}
}

//checks whether the undo stack is empty
//returns true if we can undo
bool  UndoManager::CanUndo() const
{
	return !undo_stack.empty();
}
bool UndoManager::CanRedo() const
{
    return !redo_stack.empty();
}
void  UndoManager::ClearUndo() //clear the Undo stack when changing scenes
{
	undo_stack.clear();
}
void UndoManager::ClearRedo()
{
    redo_stack.clear();
}
UndoManager::operator bool() const noexcept {
	//return !undo_stack.empty();
	return CanUndo() || CanRedo();
}
size_t UndoManager::Capacity() const noexcept 
{ 
	return MAX_HISTORY;
}

/*
* void MoveObject(GameObject& obj, Vector3 newPos) {
    Vector3 oldPos = obj.getPosition();

    Action moveAction;
    
    // The "Forward" logic
    moveAction.redo = [&obj, newPos]() { 
        obj.setPosition(newPos); 
    };

    // The "Reverse" logic
    moveAction.undo = [&obj, oldPos]() { 
        obj.setPosition(oldPos); 
    };

    // Execute it once immediately
    moveAction.redo();

    // Push it to UndoManager
    myUndoManager.Push(std::move(moveAction));
}
*/