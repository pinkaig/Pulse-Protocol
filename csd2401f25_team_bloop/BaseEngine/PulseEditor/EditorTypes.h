#pragma once

#include <memory>
#include "CoreEngine/ECS/Types.h"

class UndoManager;

namespace PulseEditor{
    struct GameViewport { int x, y, w, h; };
    // extern const Entity INVALID_ENTITY;
    inline constexpr Entity INVALID_ENTITY = static_cast<Entity>(-1);

    struct EntityHandle
    {
        uint32_t id = INVALID_ENTITY;  // current live entity id (changes over time)
    };

    //UNDO Manager
    extern UndoManager mUndo;
    extern Entity selectedEntity;
    extern Factory Editorfactory;
    // this vector maintains the order entities appear in the hierarchy
    extern std::shared_ptr<EntityHandle> selectedHandle;
    extern std::vector<Entity> entityDisplayOrder;
    extern std::unordered_map<Entity, std::shared_ptr<EntityHandle>> handleOf;
}
