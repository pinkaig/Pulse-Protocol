/******************************************************************************/
/**
 * @file        collision.h
 * @project     Pulse Protocol
 * @author      Reginald Lew Yee Ren (primary) - 96%
 * @author      Goh Pin Kai (secondary) - 2%
 * @author      Ban Kai Wei Benjamin (secondary) - 2%
 * @brief       Collision management utilities handling AABB, OBB, and distance-based
 *              collision detection. Provides reusable helpers for mouse/GUI hit testing
 *              and Transform-to-bounding-box conversions. Used throughout gameplay logic
 *              for all physical interactions.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              the prior written consent of DigiPen Institute of Technology is
 *              prohibited.
 */
 /******************************************************************************/

#ifndef COLLISION_H
#define COLLISION_H

#include <unordered_map> // Uniform Grid
#include <unordered_set> // Uniform Grid
#include <vector> // Uniform Grid
#include <cstdint> // Uniform Grid
#include <cmath> // Uniform Grid
#include "CoreEngine/Assets.h"
#include "Graphics/Transform.h"

//for .NET
#include "CoreEngine/Core/ImportExport.h" // for DLL_API
#pragma warning(push)
#pragma warning(disable: 4251)

 // Forward declare
namespace Framework { class Transform; }

/**
 * @struct OBB
 * @brief Represents an Oriented Bounding Box for rotational collision detection.
 */
struct DLL_API OBB
{
    Vector2 center;        // Center of the box
    Vector2 halfExtents;   // Half width / height
    float rotation;        // Rotation in degrees (use Transform::angle_disp)
    Vector2 axis[2];       // Local axes for SAT
};


// Converts rotation to radians whether it's already an angle (float)
// or a 2D direction vector (Vector2) in your Transform.
inline float ToAngleRadians(float angleRadians) {
    return angleRadians; // already radians
}

inline float ToAngleRadians(const Vector2& dir) {
    // If Rotation is a direction vector, derive angle with atan2(y, x)
    // Assumes dir is normalized or at least non-zero.
    return atan2f(dir.y, dir.x);
}

/**
 * @class CollisionManager
 * @brief Handles all collision-related computations, separated from gameplay logic.
 */
class DLL_API CollisionManager : public Systems
{
public:
    void Init() override;
    void Update(float dt) override;

    // --- Broad Phase API (NEW) ---
    bool BroadPhaseAllows(unsigned int a, unsigned int b) const;

    // (Optional) debug counters for rubric proof
    size_t GetBroadPhasePairCount() const { return m_broadPairsLastFrame; }
    size_t GetBroadPhaseCellCount() const { return m_cells.size(); }

    // --- Core AABB Utilities ---
    AABB BuildAABBFromTransform(const Framework::Transform& transform, const AABB& baseBox);//carrie dunnid

    // --- Collision Detection ---
    bool CheckAABBCollision(const AABB& a, const AABB& b);
    bool CheckEdgeContactCollision(const AABB& playerBox, const AABB& enemyBox, float threshold);
    bool CheckDistanceCollision(const Vector2& a, float aradius, const Vector2& b, float bradius);

    // --- Rotational (OBB) Collision Support ---
    OBB BuildOBBFromTransform(const Framework::Transform& transform, const AABB& baseBox);//carrie dunnid
    bool CheckOBBvsOBB(const OBB& obbA, const OBB& obbB);//carrie dunnid
    void ResolveOBBCollision(Framework::Transform& a, Framework::Transform& b, float elasticity = 0.25f);//carrie dunnid

private:
    // --- Uniform Grid Broad Phase (NEW) ---
    struct CellCoord
    {
        int x{}, y{};
        bool operator==(const CellCoord& rhs) const { return x == rhs.x && y == rhs.y; }
    };

    struct CellHash
    {
        size_t operator()(const CellCoord& c) const noexcept
        {
            // simple hash combine
            return (static_cast<size_t>(c.x) * 73856093u) ^ (static_cast<size_t>(c.y) * 19349663u);
        }
    };

    struct PairKey
    {
        unsigned int a{}, b{}; // always stored ordered: a < b
        bool operator==(const PairKey& rhs) const { return a == rhs.a && b == rhs.b; }
    };

    struct PairHash
    {
        size_t operator()(const PairKey& p) const noexcept
        {
            return (static_cast<size_t>(p.a) * 2654435761u) ^ static_cast<size_t>(p.b);
        }
    };

    float m_cellSize = 200.0f; // tune later
    std::unordered_map<CellCoord, std::vector<unsigned int>, CellHash> m_cells;
    std::unordered_set<PairKey, PairHash> m_candidatePairs;

    size_t m_broadPairsLastFrame = 0;

private:
    void RebuildUniformGrid();
    static PairKey MakePair(unsigned int a, unsigned int b)
    {
        return (a < b) ? PairKey{ a, b } : PairKey{ b, a };
    }
};

#endif // COLLISION_H
