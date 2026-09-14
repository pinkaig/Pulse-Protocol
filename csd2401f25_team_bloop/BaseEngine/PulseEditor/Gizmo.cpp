/******************************************************************************/
/**
 * @file        Gizmo.cpp
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 60%
 * @author      Chloe Lau Rey En (secondary) - 20%
 * @author      Ban Kai Wei Benjamin (secondary) - 20%
 * @brief       Implements a Unity-style transform gizmo system with move, scale,
 *              and rotate modes for entity manipulation in the editor viewport
 *              - Undo feature for ImGui::IsKeyPressed(ImGuiKey_Delete) key for entities and
 *              - Undo for Gizmo dragging
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "pch/pch_temp.h"
#include "../../imgui/imgui_internal.h"  // for GImGui->HoveredWindow (viewport blocking check)

// Core editor / engine headers
//#include "Editor.h" // for GameViewport, GizmoState, selectedEntity, INVALID_ENTITY
#include "Gizmo.h"
#include "LayerManagerPanel.h"  // for IsLayerVisible, IsLayerLocked
#include "HierarchyHelpers.h"

#include "CoreEngine/ECS/Coordinator.h"
#include "CoreEngine/ECS/Types.h"
#include "Graphics/Transform.h"
#include "Graphics/glapp.h"
#include "Graphics/Layer.h"
#include "CoreEngine/Scene/SceneManager.h"  // For scene change detection
#include "Components/ParentChild.h"

// #include "../CoreEngine/ECS/Types.h"
// #include "../Graphics/Transform.h"
// #include "../Graphics/glapp.h"
// #include "../Graphics/Layer.h"

#include "SnapShot.h"
#include "EditorTypes.h"
#include "UndoHelper.h"

namespace PulseEditor
{
    // Gizmo-undo state (separate from Inspector)
    static bool s_gizmoEditing = false;
    static Framework::Transform s_gizmoBeforeTransform{};
    static std::shared_ptr<EntityHandle> s_gizmoHandle = nullptr; //will assign when dragging
    GLFWwindow* window;
    // -------------------------------------------------------------------------
    // Internal state & constants
    // -------------------------------------------------------------------------

    namespace GizmoConfig
    {
        // =====================================================================
        // GIZMO VISUAL SETTINGS (in screen pixels at 1x zoom)
        // These scale automatically with camera zoom to stay consistent size
        // =====================================================================

        // Move tool
        constexpr float ARROW_LENGTH_BASE = 80.0f;    // Length of move arrows
        constexpr float ARROW_HEAD_SIZE_BASE = 12.0f; // Size of arrow heads
        constexpr float ARROW_THICKNESS = 3.0f;       // Line thickness

        // Scale tool
        constexpr float SCALE_BOX_SIZE_BASE = 10.0f; // Scale handle boxes

        // Rotate tool
        constexpr float ROTATE_RADIUS_BASE = 70.0f; // Rotation ring radius
        constexpr float ROTATE_THICKNESS = 4.0f;    // Ring thickness

        // Shared
        constexpr float CENTER_BOX_SIZE_BASE = 12.0f; // Center square
        constexpr float HIT_PADDING = 8.0f;           // Extra hit detection padding

        // Size limits (prevents gizmo from becoming too small/large)
        constexpr float MIN_SCALE_FACTOR = 0.5f;
        constexpr float MAX_SCALE_FACTOR = 3.0f;

        // =====================================================================
        // COLORS (Unity-style)
        // =====================================================================
        constexpr ImU32 COLOR_X = IM_COL32(230, 75, 75, 255);       // Red - X axis
        constexpr ImU32 COLOR_Y = IM_COL32(130, 200, 80, 255);      // Green - Y axis
        constexpr ImU32 COLOR_CENTER = IM_COL32(255, 220, 80, 255); // Yellow - center/uniform
        constexpr ImU32 COLOR_ROTATE = IM_COL32(80, 180, 255, 255); // Blue - rotation
        constexpr ImU32 COLOR_HOVER = IM_COL32(255, 255, 100, 255); // Bright yellow - hovered
        constexpr ImU32 COLOR_BOUNDS = IM_COL32(255, 150, 50, 120); // Orange - selection bounds
    }

    // =========================================================================
    // DYNAMIC ACCESSORS (from GLApp)
    // =========================================================================

    // Get world dimensions from GLApp
    static float GetWorldWidth() { return static_cast<float>(GLApp::VIRTUAL_W); }
    static float GetWorldHeight() { return static_cast<float>(GLApp::VIRTUAL_H); }

    // Get camera zoom for scaling gizmo handles
    // Zoom > 0.5 = zoomed in, < 0.5 = zoomed out (based on your Camera2D)
    static float GetCameraZoom() { return GLApp::gEditorCamera.zoom; }

    // Calculate gizmo scale factor based on zoom
    // When zoomed out, gizmo should be larger; when zoomed in, smaller
    static float GetGizmoScaleFactor()
    {
        using namespace GizmoConfig;

        // Normalize zoom: 0.5 is "normal" in your camera
        float normalizedZoom = GetCameraZoom() / 0.5f;

        // Inverse relationship: zoom out = larger gizmo, zoom in = smaller gizmo
        float scaleFactor = 1.0f / normalizedZoom;

        // Clamp to reasonable range
        return std::clamp(scaleFactor, MIN_SCALE_FACTOR, MAX_SCALE_FACTOR);
    }

    // =========================================================================
    // GIZMO TYPES
    // =========================================================================

    enum class GizmoMode
    {
        Move,  // 1 key - Translate
        Scale, // 2 key - Scale
        Rotate // 3 key - Rotate
    };

    enum class GizmoHandle
    {
        None,
        // Move handles
        MoveX,
        MoveY,
        MoveCenter,
        // Scale handles
        ScaleX,
        ScaleY,
        ScaleUniform,
        // Rotate handle
        Rotate
    };

    // =========================================================================
    // GIZMO STATE
    // =========================================================================

    struct GizmoStateInternal
    {
        // Current mode
        GizmoMode mode = GizmoMode::Move;

        // Drag state
        bool dragging = false;
        Entity active = INVALID_ENTITY;
        GizmoHandle activeHandle = GizmoHandle::None;

        // Starting values when drag began
        glm::vec2 dragStartScreen{ 0, 0 };
        Vector2 dragStartWorld{ 0, 0 };
        Vector2 entityStartPos{ 0, 0 };
        Vector2 entityStartScale{ 0, 0 };
        float entityStartRotation = 0.0f;
        float dragStartAngle = 0.0f;

        // Scene tracking for auto-deselect
        std::string lastSceneDuringPlay = "";
    };

    static GizmoStateInternal g_gizmo;

    // =========================================================================
    // HELPER FUNCTIONS
    // =========================================================================

    static inline glm::vec2 ToGlm(const Vector2& v) { return { v.x, v.y }; }
    static inline Vector2 ToV2(const glm::vec2& v) { return { v.x, v.y }; }

    // Compute letterboxed game draw rectangle (uses GLApp values)
    static void ComputeGameDrawRect(const GameViewport& vp, float& drawX, float& drawY,
        float& drawW, float& drawH, float& scale)
    {
        const float worldW = GetWorldWidth();
        const float worldH = GetWorldHeight();

        const float sx = vp.w / worldW;
        const float sy = vp.h / worldH;
        scale = (sx < sy) ? sx : sy;
        drawW = worldW * scale;
        drawH = worldH * scale;
        drawX = vp.x + (vp.w - drawW) * 0.5f;
        drawY = vp.y + (vp.h - drawH) * 0.5f;
    }

    // Point in rectangle check
    static bool PointInRect(const glm::vec2& pt, const glm::vec2& min, const glm::vec2& max)
    {
        return pt.x >= min.x && pt.x <= max.x && pt.y >= min.y && pt.y <= max.y;
    }

    // Point near line segment check
    static bool PointNearLine(const glm::vec2& pt, const glm::vec2& a, const glm::vec2& b, float threshold)
    {
        glm::vec2 ab = b - a;
        glm::vec2 ap = pt - a;
        float len2 = glm::dot(ab, ab);
        if (len2 < 0.0001f)
            return glm::length(pt - a) <= threshold;
        float t = glm::clamp(glm::dot(ap, ab) / len2, 0.0f, 1.0f);
        glm::vec2 closest = a + t * ab;
        return glm::length(pt - closest) <= threshold;
    }

    // Point near circle check
    static bool PointNearCircle(const glm::vec2& pt, const glm::vec2& center, float radius, float thickness)
    {
        float dist = glm::length(pt - center);
        return dist >= (radius - thickness) && dist <= (radius + thickness);
    }

    // Get angle from center to point
    static float GetAngle(const glm::vec2& center, const glm::vec2& pt)
    {
        return std::atan2(pt.y - center.y, pt.x - center.x);
    }

    // =========================================================================
    // SCALED SIZE GETTERS (adjust based on camera zoom)
    // =========================================================================

    static float GetArrowLength() { return GizmoConfig::ARROW_LENGTH_BASE * GetGizmoScaleFactor(); }
    static float GetArrowHeadSize() { return GizmoConfig::ARROW_HEAD_SIZE_BASE * GetGizmoScaleFactor(); }
    static float GetCenterBoxSize() { return GizmoConfig::CENTER_BOX_SIZE_BASE * GetGizmoScaleFactor(); }
    static float GetScaleBoxSize() { return GizmoConfig::SCALE_BOX_SIZE_BASE * GetGizmoScaleFactor(); }
    static float GetRotateRadius() { return GizmoConfig::ROTATE_RADIUS_BASE * GetGizmoScaleFactor(); }

    // =========================================================================
    // WORLD <-> SCREEN CONVERSION
    // =========================================================================

    glm::vec2 WorldToScreen(const Vector2& wpos, const GameViewport& vp)
    {
        const float worldW = GetWorldWidth();
        const float worldH = GetWorldHeight();

        const float sx = 2.0f / worldW, sy = 2.0f / worldH;
        const glm::mat3 N(sx, 0, 0, 0, sy, 0, 0, 0, 1);
        const glm::mat3 Cam = GLApp::BuildPanZoomNDC(GLApp::gEditorCamera);
        const glm::vec3 ndc = Cam * (N * glm::vec3(wpos.x, wpos.y, 1.0f));

        float drawX, drawY, drawW, drawH, scale;
        ComputeGameDrawRect(vp, drawX, drawY, drawW, drawH, scale);

        const float x = drawX + (ndc.x * 0.5f + 0.5f) * drawW;
        const float y = drawY + (1.0f - (ndc.y * 0.5f + 0.5f)) * drawH;
        return { x, y };
    }

    Vector2 ScreenToWorld(glm::vec2 screen, const GameViewport& vp)
    {
        const float worldW = GetWorldWidth();
        const float worldH = GetWorldHeight();

        float drawX, drawY, drawW, drawH, scale;
        ComputeGameDrawRect(vp, drawX, drawY, drawW, drawH, scale);

        const float ndcX = ((screen.x - drawX) / drawW) * 2.0f - 1.0f;
        const float ndcY = 1.0f - ((screen.y - drawY) / drawH) * 2.0f;
        const glm::vec3 ndc(ndcX, ndcY, 1.0f);

        const float sx = 2.0f / worldW, sy = 2.0f / worldH;
        const glm::mat3 N(sx, 0, 0, 0, sy, 0, 0, 0, 1);
        const glm::mat3 Cam = GLApp::BuildPanZoomNDC(GLApp::gEditorCamera);
        const glm::mat3 invAll = glm::inverse(Cam * N);

        const glm::vec3 w = invAll * ndc;
        return Vector2{ w.x, w.y };
    }

    // =========================================================================
    // GIZMO DRAWING FUNCTIONS
    // =========================================================================

    // Draw arrow (for move gizmo)
    static void DrawArrow(ImDrawList* dl, const glm::vec2& from, const glm::vec2& to,
        ImU32 color, float thickness, float headSize, float uiScale)
    {
        ImVec2 f(from.x / uiScale, from.y / uiScale);
        ImVec2 t(to.x / uiScale, to.y / uiScale);

        // Line
        dl->AddLine(f, t, color, thickness);

        // Arrow head
        glm::vec2 dir = glm::normalize(to - from);
        glm::vec2 perp(-dir.y, dir.x);
        glm::vec2 headBase = to - dir * headSize;
        glm::vec2 h1 = headBase + perp * (headSize * 0.5f);
        glm::vec2 h2 = headBase - perp * (headSize * 0.5f);

        dl->AddTriangleFilled(
            t,
            ImVec2(h1.x / uiScale, h1.y / uiScale),
            ImVec2(h2.x / uiScale, h2.y / uiScale),
            color);
    }

    // Draw move gizmo (Unity-style arrows)
    static GizmoHandle DrawMoveGizmo(ImDrawList* dl, const glm::vec2& center,
        const glm::vec2& mouse, float uiScale)
    {
        using namespace GizmoConfig;
        GizmoHandle hovered = GizmoHandle::None;

        // Get scaled sizes
        const float arrowLength = GetArrowLength();
        const float arrowHeadSize = GetArrowHeadSize();
        const float centerBoxSize = GetCenterBoxSize();

        // Arrow endpoints
        glm::vec2 xEnd = center + glm::vec2(arrowLength, 0);
        glm::vec2 yEnd = center + glm::vec2(0, -arrowLength); // Up in screen space

        // Check hover states
        bool hoverX = PointNearLine(mouse, center, xEnd, HIT_PADDING + ARROW_THICKNESS);
        bool hoverY = PointNearLine(mouse, center, yEnd, HIT_PADDING + ARROW_THICKNESS);

        glm::vec2 centerMin = center - glm::vec2(centerBoxSize);
        glm::vec2 centerMax = center + glm::vec2(centerBoxSize);
        bool hoverCenter = PointInRect(mouse, centerMin, centerMax);

        // Determine which handle is hovered (priority: center > Y > X)
        if (hoverCenter)
            hovered = GizmoHandle::MoveCenter;
        else if (hoverY)
            hovered = GizmoHandle::MoveY;
        else if (hoverX)
            hovered = GizmoHandle::MoveX;

        // Draw X arrow (red)
        ImU32 colorX = (hovered == GizmoHandle::MoveX) ? COLOR_HOVER : COLOR_X;
        DrawArrow(dl, center, xEnd, colorX, ARROW_THICKNESS, arrowHeadSize, uiScale);

        // Draw Y arrow (green)
        ImU32 colorY = (hovered == GizmoHandle::MoveY) ? COLOR_HOVER : COLOR_Y;
        DrawArrow(dl, center, yEnd, colorY, ARROW_THICKNESS, arrowHeadSize, uiScale);

        // Draw center box (yellow)
        ImU32 colorCenter = (hovered == GizmoHandle::MoveCenter) ? COLOR_HOVER : COLOR_CENTER;
        dl->AddRectFilled(
            ImVec2(centerMin.x / uiScale, centerMin.y / uiScale),
            ImVec2(centerMax.x / uiScale, centerMax.y / uiScale),
            colorCenter);

        return hovered;
    }

    // Draw scale gizmo (Unity-style boxes)
    static GizmoHandle DrawScaleGizmo(ImDrawList* dl, const glm::vec2& center,
        const glm::vec2& mouse, float uiScale)
    {
        using namespace GizmoConfig;
        GizmoHandle hovered = GizmoHandle::None;

        // Get scaled sizes
        const float arrowLength = GetArrowLength();
        const float scaleBoxSize = GetScaleBoxSize();
        const float centerBoxSize = GetCenterBoxSize();

        // Scale handle positions
        glm::vec2 xEnd = center + glm::vec2(arrowLength, 0);
        glm::vec2 yEnd = center + glm::vec2(0, -arrowLength);

        // Box extents
        glm::vec2 xBoxMin = xEnd - glm::vec2(scaleBoxSize);
        glm::vec2 xBoxMax = xEnd + glm::vec2(scaleBoxSize);
        glm::vec2 yBoxMin = yEnd - glm::vec2(scaleBoxSize);
        glm::vec2 yBoxMax = yEnd + glm::vec2(scaleBoxSize);
        glm::vec2 centerMin = center - glm::vec2(centerBoxSize);
        glm::vec2 centerMax = center + glm::vec2(centerBoxSize);

        // Check hover
        bool hoverX = PointInRect(mouse, xBoxMin, xBoxMax) ||
            PointNearLine(mouse, center, xEnd, HIT_PADDING);
        bool hoverY = PointInRect(mouse, yBoxMin, yBoxMax) ||
            PointNearLine(mouse, center, yEnd, HIT_PADDING);
        bool hoverCenter = PointInRect(mouse, centerMin, centerMax);

        if (hoverCenter)
            hovered = GizmoHandle::ScaleUniform;
        else if (hoverY)
            hovered = GizmoHandle::ScaleY;
        else if (hoverX)
            hovered = GizmoHandle::ScaleX;

        // Draw X line and box
        ImU32 colorX = (hovered == GizmoHandle::ScaleX) ? COLOR_HOVER : COLOR_X;
        dl->AddLine(
            ImVec2(center.x / uiScale, center.y / uiScale),
            ImVec2(xEnd.x / uiScale, xEnd.y / uiScale),
            colorX, ARROW_THICKNESS);
        dl->AddRectFilled(
            ImVec2(xBoxMin.x / uiScale, xBoxMin.y / uiScale),
            ImVec2(xBoxMax.x / uiScale, xBoxMax.y / uiScale),
            colorX);

        // Draw Y line and box
        ImU32 colorY = (hovered == GizmoHandle::ScaleY) ? COLOR_HOVER : COLOR_Y;
        dl->AddLine(
            ImVec2(center.x / uiScale, center.y / uiScale),
            ImVec2(yEnd.x / uiScale, yEnd.y / uiScale),
            colorY, ARROW_THICKNESS);
        dl->AddRectFilled(
            ImVec2(yBoxMin.x / uiScale, yBoxMin.y / uiScale),
            ImVec2(yBoxMax.x / uiScale, yBoxMax.y / uiScale),
            colorY);

        // Draw center box (uniform scale)
        ImU32 colorCenter = (hovered == GizmoHandle::ScaleUniform) ? COLOR_HOVER : COLOR_CENTER;
        dl->AddRectFilled(
            ImVec2(centerMin.x / uiScale, centerMin.y / uiScale),
            ImVec2(centerMax.x / uiScale, centerMax.y / uiScale),
            colorCenter);

        return hovered;
    }

    // Draw rotate gizmo (Unity-style ring)
    static GizmoHandle DrawRotateGizmo(ImDrawList* dl, const glm::vec2& center,
        const glm::vec2& mouse, float uiScale)
    {
        using namespace GizmoConfig;
        GizmoHandle hovered = GizmoHandle::None;

        // Get scaled size
        const float rotateRadius = GetRotateRadius();

        // Check if mouse is near the ring
        if (PointNearCircle(mouse, center, rotateRadius, ROTATE_THICKNESS + HIT_PADDING))
        {
            hovered = GizmoHandle::Rotate;
        }

        ImU32 color = (hovered == GizmoHandle::Rotate) ? COLOR_HOVER : COLOR_ROTATE;

        // Draw rotation ring
        dl->AddCircle(
            ImVec2(center.x / uiScale, center.y / uiScale),
            rotateRadius / uiScale,
            color,
            48,
            ROTATE_THICKNESS);

        // Draw small handle at top of ring
        glm::vec2 handlePos = center + glm::vec2(0, -rotateRadius);
        dl->AddCircleFilled(
            ImVec2(handlePos.x / uiScale, handlePos.y / uiScale),
            6.0f,
            color);

        return hovered;
    }

    // [CHANGED] Replaced the floating tool panel with a tiny non-interactive
    // mode label. The user switches modes with 1/2/3 keys. The label just
    // confirms which mode is currently active without cluttering the viewport.
    static void DrawModeIndicator(const GameViewport& vp)
    {
        // vp.x/y are in framebuffer pixels — divide by DPI scale to get ImGui logical pixels
        ImGuiIO& io = ImGui::GetIO();
        const float sx_ui = io.DisplayFramebufferScale.x > 0.f ? io.DisplayFramebufferScale.x : 1.f;

        // Position in the top-left corner of the viewport, not the screen
        ImGui::SetNextWindowPos(ImVec2(vp.x / sx_ui + 8.0f, vp.y / sx_ui + 8.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.82f);

        // [CHANGED] Removed NoDecoration and NoMove so the panel has a title bar
        // with a collapse arrow (user can minimize it) and can be dragged anywhere
        // in the viewport. NoInputs also removed so buttons remain clickable.
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav;

        if (ImGui::Begin("Gizmo Controls##GizmoModeIndicator", nullptr, flags))
        {
            struct ToolDef {
                GizmoMode mode;
                const char* icon;
                const char* label;
                const char* key;
                ImVec4 activeColor;
            };

            ToolDef tools[] = {
                { GizmoMode::Move,   "->", "Move",   "[1]", ImVec4(1.0f, 0.85f, 0.25f, 1.0f) },
                { GizmoMode::Scale,  "<>", "Scale",  "[2]", ImVec4(0.4f, 0.85f, 1.0f,  1.0f) },
                { GizmoMode::Rotate, "()", "Rotate", "[3]", ImVec4(0.5f, 1.0f,  0.5f,  1.0f) },
            };

            for (auto& t : tools)
            {
                bool isActive = (g_gizmo.mode == t.mode);

                if (isActive)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.42f, 0.72f, 1.0f));
                else
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));

                std::string btnLabel = std::string(t.icon) + "  " + t.label + "  " + t.key
                    + "##tool" + t.key;
                if (ImGui::Button(btnLabel.c_str(), ImVec2(130, 26)))
                    g_gizmo.mode = t.mode;

                ImGui::PopStyleColor();

                if (isActive)
                {
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    ImGui::GetWindowDrawList()->AddLine(
                        ImVec2(min.x, max.y - 2),
                        ImVec2(max.x, max.y - 2),
                        ImGui::ColorConvertFloat4ToU32(t.activeColor), 3.0f);
                }
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "Active: ");
            ImGui::SameLine();
            int modeIdx = static_cast<int>(g_gizmo.mode);
            ImGui::TextColored(tools[modeIdx].activeColor, "%s", tools[modeIdx].label);
        }
        ImGui::End();
    }

    // =========================================================================
    // MAIN GIZMO FUNCTION
    // =========================================================================

    void Gizmo_DrawAndHandle(const GameViewport& vp)
    {
        using namespace GizmoConfig;

        auto* coord = Coordinator::GetInstance();
        if (!coord)
            return;

        // -----------------------------------------------------------------------------
        // VIEWPORT BLOCKING CHECK
        // Prevent any viewport interaction when a panel or modal covers the viewport.
        // Two cases:
        //   1. A modal/popup is open (e.g. "New Scene" dialog).
        //   2. A floating panel window has been dragged on top of the viewport.
        //      "LayoutOverlay" is the expected background window over the viewport;
        //      any other hovered window means something is blocking it.
        // -----------------------------------------------------------------------------
        bool viewportBlocked = false;
        {
            if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
            {
                viewportBlocked = true;
            }
            else
            {
                ImGuiWindow* hovWin = GImGui->HoveredWindow;
                if (hovWin && strcmp(hovWin->Name, "LayoutOverlay") != 0)
                    viewportBlocked = true;
            }
        }

        // -----------------------------------------------------------------------------
       // CAMERA PAN
       // -----------------------------------------------------------------------------
        {
            window = glfwGetCurrentContext();
            ImGuiIO& io = ImGui::GetIO();

            //-----------------------------------------
            // Convert mouse → framebuffer coordinates
            //-----------------------------------------
            int winW, winH, fbW, fbH;
            glfwGetWindowSize(window, &winW, &winH);
            glfwGetFramebufferSize(window, &fbW, &fbH);

            double curX = io.MousePos.x;
            double curY = io.MousePos.y;

            double fbX = 0.0, fbY = 0.0;
            if (winW > 0 && winH > 0)
            {
                fbX = curX * (double)fbW / (double)winW;

                // ImGui Y=0 at top → framebuffer Y=0 at bottom
                double normY = curY / (double)winH;
                fbY = (1.0 - normY) * (double)fbH;
            }

            //-----------------------------------------
            // Convert viewport rect → framebuffer coords
            // (Y must be flipped because vp.y is from TOP)
            //-----------------------------------------
            double vpFbX0 = vp.x * (double)fbW / (double)winW;
            double vpFbX1 = (vp.x + vp.w) * (double)fbW / (double)winW;

            // Flip Y:
            double vpFbY1 = (double)fbH - (vp.y * (double)fbH / (double)winH);
            double vpFbY0 = (double)fbH - ((vp.y + vp.h) * (double)fbH / (double)winH);

            //-----------------------------------------
            // Final hit test (framebuffer space)
            //-----------------------------------------
            bool allowCamera =
                (fbX >= vpFbX0 && fbX <= vpFbX1 &&
                    fbY >= vpFbY0 && fbY <= vpFbY1);


            //-----------------------------------------
            // Middle mouse dragging → pan camera
            //-----------------------------------------
            static double lastX = curX, lastY = curY;

            if (!PulseEditor::IsPlaying())
            {
                if (allowCamera && !viewportBlocked && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
                {
                    double dx = curX - lastX;
                    double dy = curY - lastY;

                    GLApp::gEditorCamera.center.x += float(2.0 * dx / double(vp.w));
                    GLApp::gEditorCamera.center.y -= float(2.0 * dy / double(vp.h));

                }
            }

            lastX = curX;
            lastY = curY;

            // -------------------- Scroll wheel zoom (editor camera) -------------
            float wheel = ImGui::GetIO().MouseWheel;
            auto& activeCam = GLApp::gEditorCamera;

            // Only zoom in editor mode, and only if mouse is over the game viewport
            if (!PulseEditor::IsPlaying() && allowCamera && !viewportBlocked && wheel != 0.0f)
            {
                const float oldZ = activeCam.zoom;
                const float step = 1.12f;

                float newZ = glm::clamp(
                    oldZ * std::pow(step, wheel),
                    activeCam.minZoom,
                    activeCam.maxZoom
                );

                // Simple zoom around the current camera center
                activeCam.zoom = newZ;
            }


        }

        // ALWAYS check if selected entity still exists (even during play)
        // This handles scene changes during gameplay
        // ---------------------------------------------------------------------
        // KEYBOARD SHORTCUTS (1/2/3 for mode switching)
        // ---------------------------------------------------------------------
        if (!ImGui::GetIO().WantTextInput)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_1))
                g_gizmo.mode = GizmoMode::Move;
            if (ImGui::IsKeyPressed(ImGuiKey_2))
                g_gizmo.mode = GizmoMode::Scale;
            if (ImGui::IsKeyPressed(ImGuiKey_3))
                g_gizmo.mode = GizmoMode::Rotate;
        }

        // Draw mode indicator UI
        DrawModeIndicator(vp);

        // ---------------------------------------------------------------------
        // VALIDATE SELECTED ENTITY
        // ---------------------------------------------------------------------
        if (selectedEntity != INVALID_ENTITY && g_gizmo.dragging)
        {
            if (!coord->HasComponent<Framework::Transform>(selectedEntity))
            {
                g_gizmo.dragging = false;
                g_gizmo.active = INVALID_ENTITY;
                g_gizmo.activeHandle = GizmoHandle::None;
            }
        }

        // ---------------------------------------------------------------------
        // SCENE CHANGE DETECTION (auto-deselect on ANY scene change)
        // ---------------------------------------------------------------------
        {
            static std::string s_lastKnownScene = "";
            auto sceneManager = coord->GetSystem<SceneManager>();
            std::string currentScene = sceneManager ? sceneManager->GrabCurrentScene() : "";

            if (!s_lastKnownScene.empty() && currentScene != s_lastKnownScene)
            {
                // Scene changed - deselect everything
                selectedEntity = INVALID_ENTITY;
                g_gizmo.dragging = false;
                g_gizmo.active = INVALID_ENTITY;
                g_gizmo.activeHandle = GizmoHandle::None;
            }
            s_lastKnownScene = currentScene;
        }

        // ---------------------------------------------------------------------
        // PANEL DRAG DESELECT
        // Clear selection whenever the user grabs a panel title bar to move it.
        // GImGui->MovingWindow is non-null for every frame a window is being dragged.
        // ---------------------------------------------------------------------
        if (GImGui->MovingWindow != nullptr)
        {
            selectedEntity = INVALID_ENTITY;
            selectedHandle = nullptr;
            g_gizmo.dragging = false;
            g_gizmo.active = INVALID_ENTITY;
            g_gizmo.activeHandle = GizmoHandle::None;
        }

        // ---------------------------------------------------------------------
        // TOPBAR / BUILD SIZE ANALYZER CLICK DESELECT
        // Clear selection when the user clicks the TopBar (any button or the
        // Tools menu and its popup children) or the Build Size Analyzer panel
        // (catches docked-tab drag starts as well as regular clicks).
        // We walk the ParentWindow chain so that menu popups, which have their
        // own window name, are still recognised as belonging to TopBar.
        // ---------------------------------------------------------------------
        {
            ImGuiWindow* hovWin = GImGui->HoveredWindow;
            if (hovWin && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                bool shouldDeselect = false;
                for (ImGuiWindow* w = hovWin; w != nullptr; w = w->ParentWindow)
                {
                    if (strcmp(w->Name, "TopBar") == 0 ||
                        strcmp(w->Name, "Build Size Analyzer") == 0)
                    {
                        shouldDeselect = true;
                        break;
                    }
                }

                if (shouldDeselect)
                {
                    selectedEntity = INVALID_ENTITY;
                    selectedHandle = nullptr;
                    g_gizmo.dragging = false;
                    g_gizmo.active = INVALID_ENTITY;
                    g_gizmo.activeHandle = GizmoHandle::None;
                }
            }
        }

        // Don't allow gizmo interaction during play mode
        if (IsPlaying())
        {
            return;
        }

        // ---------------------------------------------------------------------
        // LAYER VISIBILITY/LOCK CHECK
        // ---------------------------------------------------------------------
        if (selectedEntity != INVALID_ENTITY)
        {
            if (coord->HasComponent<LayerTag>(selectedEntity))
            {
                auto& tag = coord->GetComponent<LayerTag>(selectedEntity);
                if (!IsLayerVisible(tag.mask) || IsLayerLocked(tag.mask))
                {
                    selectedEntity = INVALID_ENTITY;
                    g_gizmo.dragging = false;
                    g_gizmo.active = INVALID_ENTITY;
                    g_gizmo.activeHandle = GizmoHandle::None;
                }
            }
        }

        // ---------------------------------------------------------------------
        // SETUP COORDINATE TRANSFORMS
        // ---------------------------------------------------------------------
        ImGuiIO& io = ImGui::GetIO();
        const float sx_ui = io.DisplayFramebufferScale.x > 0.f ? io.DisplayFramebufferScale.x : 1.f;
        const float sy_ui = io.DisplayFramebufferScale.y > 0.f ? io.DisplayFramebufferScale.y : 1.f;

        float drawX, drawY, drawW, drawH, scale;
        ComputeGameDrawRect(vp, drawX, drawY, drawW, drawH, scale);

        const glm::vec2 mouseFB(io.MousePos.x * sx_ui, io.MousePos.y * sy_ui);

        const float ndcX = ((mouseFB.x - drawX) / drawW) * 2.0f - 1.0f;
        const float ndcY = 1.0f - ((mouseFB.y - drawY) / drawH) * 2.0f;
        const glm::vec3 mouseNDC(ndcX, ndcY, 1.0f);

        // Camera pan/zoom used by the renderer
        const glm::mat3 PanZoom = GLApp::BuildPanZoomNDC(GLApp::gEditorCamera);

        const float worldW = GetWorldWidth();
        const float worldH = GetWorldHeight();
        const float nsx = 2.0f / worldW, nsy = 2.0f / worldH;
        const glm::mat3 N(nsx, 0, 0, 0, nsy, 0, 0, 0, 1);
        const glm::mat3 invAll = glm::inverse(PanZoom * N);

        auto ScreenToWorld_DrawRect = [&](glm::vec2 fb) -> Vector2
            {
                const float x = ((fb.x - drawX) / drawW) * 2.0f - 1.0f;
                const float y = 1.0f - ((fb.y - drawY) / drawH) * 2.0f;
                glm::vec3 w = invAll * glm::vec3(x, y, 1.0f);
                return Vector2{ w.x, w.y };
            };

        // ---------------------------------------------------------------------
        // COLLECT ENTITIES FOR HIT-TESTING
        // ---------------------------------------------------------------------
        Entity hovered = INVALID_ENTITY;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->PushClipRect(
            ImVec2(drawX / sx_ui, drawY / sy_ui),
            ImVec2((drawX + drawW) / sx_ui, (drawY + drawH) / sy_ui),
            true);

        const bool inGameRect =
            (mouseFB.x >= drawX && mouseFB.x <= drawX + drawW &&
                mouseFB.y >= drawY && mouseFB.y <= drawY + drawH);

        std::vector<std::pair<Entity, int>> entitiesWithOrder;

        for (Entity e : coord->GetAllEntities())
        {
            if (!coord->HasComponent<Framework::Transform>(e) ||
                !coord->HasComponent<Framework::Renderable>(e))
                continue;

            auto& tr = coord->GetComponent<Framework::Transform>(e);
            if (!tr.isVisible)
                continue;

            if (coord->HasComponent<LayerTag>(e))
            {
                auto& tag = coord->GetComponent<LayerTag>(e);
                if (!IsLayerVisible(tag.mask))
                    continue;
            }

            int order = 0;
            if (coord->HasComponent<LayerTag>(e))
            {
                order = coord->GetComponent<LayerTag>(e).orderInLayer;
            }

            entitiesWithOrder.push_back({ e, order });
        }

        std::sort(entitiesWithOrder.begin(), entitiesWithOrder.end(),
            [](const auto& a, const auto& b)
            { return a.second < b.second; });

        // ---------------------------------------------------------------------
        // HIT-TEST AND DRAW
        // ---------------------------------------------------------------------
        GizmoHandle hoveredHandle = GizmoHandle::None;

        for (auto it = entitiesWithOrder.rbegin(); it != entitiesWithOrder.rend(); ++it)
        {
            Entity e = it->first;
            auto& tr = coord->GetComponent<Framework::Transform>(e);
            auto& re = coord->GetComponent<Framework::Renderable>(e);

            glm::mat3 M = glm::make_mat3(tr.mdl_to_ndc_xform.Begin());
            glm::mat3 modelToNDC = PanZoom * M;
            glm::mat3 invModelToNDC = glm::inverse(modelToNDC);

            glm::vec2 qmin, qmax;
            if (re.mdl_ref == 0)
            {
                qmin = { -1.f, -1.f };
                qmax = { 1.f, 1.f };
            }
            else
            {
                qmin = { -0.5f, -0.5f };
                qmax = { 0.5f, 0.5f };
            }

            // Hit test for entity selection (only if not dragging gizmo)
            if (inGameRect && hovered == INVALID_ENTITY && !g_gizmo.dragging)
            {
                bool isLocked = false;
                if (coord->HasComponent<LayerTag>(e))
                {
                    auto& tag = coord->GetComponent<LayerTag>(e);
                    isLocked = IsLayerLocked(tag.mask);
                }

                if (!isLocked)
                {
                    glm::vec3 m = invModelToNDC * mouseNDC;
                    if (m.x >= qmin.x && m.x <= qmax.x && m.y >= qmin.y && m.y <= qmax.y)
                    {
                        hovered = e;
                    }
                }
            }

            // Draw gizmo for selected entity
            if (e == selectedEntity)
            {
                auto toUI = [&](glm::vec2 modelP) -> ImVec2
                    {
                        glm::vec3 n = modelToNDC * glm::vec3(modelP, 1.0f);
                        const float x = drawX + (n.x * 0.5f + 0.5f) * drawW;
                        const float y = drawY + (1.0f - (n.y * 0.5f + 0.5f)) * drawH;
                        return ImVec2(x / sx_ui, y / sy_ui);
                    };

                auto toFB = [&](glm::vec2 modelP) -> glm::vec2
                    {
                        glm::vec3 n = modelToNDC * glm::vec3(modelP, 1.0f);
                        const float x = drawX + (n.x * 0.5f + 0.5f) * drawW;
                        const float y = drawY + (1.0f - (n.y * 0.5f + 0.5f)) * drawH;
                        return { x, y };
                    };

                // Draw selection bounds
                ImVec2 a = toUI({ qmin.x, qmin.y });
                ImVec2 b = toUI({ qmax.x, qmin.y });
                ImVec2 c = toUI({ qmax.x, qmax.y });
                ImVec2 d = toUI({ qmin.x, qmax.y });

                dl->AddLine(a, b, COLOR_BOUNDS, 2.0f);
                dl->AddLine(b, c, COLOR_BOUNDS, 2.0f);
                dl->AddLine(c, d, COLOR_BOUNDS, 2.0f);
                dl->AddLine(d, a, COLOR_BOUNDS, 2.0f);

                // Get center in screen space
                glm::vec2 centerFB = toFB({ 0, 0 });

                // Draw appropriate gizmo based on current mode
                switch (g_gizmo.mode)
                {
                case GizmoMode::Move:
                    hoveredHandle = DrawMoveGizmo(dl, centerFB, mouseFB, sx_ui);
                    break;
                case GizmoMode::Scale:
                    hoveredHandle = DrawScaleGizmo(dl, centerFB, mouseFB, sx_ui);
                    break;
                case GizmoMode::Rotate:
                    hoveredHandle = DrawRotateGizmo(dl, centerFB, mouseFB, sx_ui);
                    break;
                }
            }
        }

        // ---------------------------------------------------------------------
        // MOUSE INPUT HANDLING
        // ---------------------------------------------------------------------
        if (inGameRect && !viewportBlocked && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            // Priority: Gizmo handle > Entity selection
            if (selectedEntity != INVALID_ENTITY && hoveredHandle != GizmoHandle::None)
            {
                auto& tr = coord->GetComponent<Framework::Transform>(selectedEntity);

                g_gizmo.dragging = true;
                g_gizmo.active = selectedEntity;
                g_gizmo.activeHandle = hoveredHandle;
                g_gizmo.dragStartScreen = mouseFB;
                g_gizmo.dragStartWorld = ScreenToWorld_DrawRect(mouseFB);
                g_gizmo.entityStartPos = tr.Pos;
                g_gizmo.entityStartScale = tr.Scale;
                g_gizmo.entityStartRotation = tr.Rotation.x;
                g_gizmo.dragStartAngle = GetAngle(WorldToScreen(tr.Pos, vp), mouseFB); // <-- And this!

                // UNDO: snapshot transform at start of gizmo drag
                s_gizmoEditing = true;
                s_gizmoBeforeTransform = tr;
                s_gizmoHandle = selectedHandle;
            }
            else if (hovered != INVALID_ENTITY)
            {
                selectedEntity = hovered;
                selectedHandle = (selectedEntity != INVALID_ENTITY) ? handleOf[selectedEntity] : nullptr;
            }
            else
            {
                selectedEntity = INVALID_ENTITY;
            }
        }

        // Handle dragging
        if (g_gizmo.dragging && g_gizmo.active != INVALID_ENTITY && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (coord->HasComponent<Framework::Transform>(g_gizmo.active))
            {
                auto& tr = coord->GetComponent<Framework::Transform>(g_gizmo.active);
                Vector2 mouseWorld = ScreenToWorld_DrawRect(mouseFB);
                glm::vec2 delta = mouseFB - g_gizmo.dragStartScreen;

                switch (g_gizmo.activeHandle)
                {
                    // ----- MOVE HANDLES -----
                case GizmoHandle::MoveX:
                    tr.Pos.x = g_gizmo.entityStartPos.x + (mouseWorld.x - g_gizmo.dragStartWorld.x);
                    break;

                case GizmoHandle::MoveY:
                    tr.Pos.y = g_gizmo.entityStartPos.y + (mouseWorld.y - g_gizmo.dragStartWorld.y);
                    break;

                case GizmoHandle::MoveCenter:
                {
                    float deltaX = mouseWorld.x - g_gizmo.dragStartWorld.x;
                    float deltaY = mouseWorld.y - g_gizmo.dragStartWorld.y;

                    // Reset to start position
                    tr.Pos.x = g_gizmo.entityStartPos.x;
                    tr.Pos.y = g_gizmo.entityStartPos.y;

                    // Move entity and all children
                    HierarchyHelpers::MoveEntityAndChildren(g_gizmo.active, deltaX, deltaY);

                    // Update start for smooth dragging
                    g_gizmo.dragStartWorld = mouseWorld;
                    g_gizmo.entityStartPos = tr.Pos;
                    break;
                }

                // ----- SCALE HANDLES -----
                case GizmoHandle::ScaleX:
                {
                    float scaleFactor = 1.0f + delta.x * 0.01f;
                    tr.Scale.x = std::max(10.0f, g_gizmo.entityStartScale.x * scaleFactor);
                    break;
                }

                case GizmoHandle::ScaleY:
                {
                    float scaleFactor = 1.0f - delta.y * 0.01f; // Inverted Y
                    tr.Scale.y = std::max(10.0f, g_gizmo.entityStartScale.y * scaleFactor);
                    break;
                }

                case GizmoHandle::ScaleUniform:
                {
                    glm::vec2 centerScreen = WorldToScreen(g_gizmo.entityStartPos, vp);
                    float startDist = glm::length(g_gizmo.dragStartScreen - centerScreen);
                    float currentDist = glm::length(mouseFB - centerScreen);

                    if (startDist > 1.0f)
                    {
                        float scaleFactor = currentDist / startDist;
                        tr.Scale.x = std::max(10.0f, g_gizmo.entityStartScale.x * scaleFactor);
                        tr.Scale.y = std::max(10.0f, g_gizmo.entityStartScale.y * scaleFactor);
                    }
                    break;
                }

                // ----- ROTATE HANDLE -----
                case GizmoHandle::Rotate:
                {
                    glm::vec2 centerScreen = WorldToScreen(tr.Pos, vp);
                    float currentAngle = GetAngle(centerScreen, mouseFB);
                    float deltaAngle = currentAngle - g_gizmo.dragStartAngle;
                    float deltaDegrees = deltaAngle * (180.0f / 3.14159265f);
                    tr.Rotation.x = g_gizmo.entityStartRotation - deltaDegrees;
                    break;
                }

                default:
                    break;
                }
            }
        }

        // Release drag
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            g_gizmo.dragging = false;
            g_gizmo.active = INVALID_ENTITY;
            g_gizmo.activeHandle = GizmoHandle::None;

            // Replace the entire if block with:
            if (s_gizmoEditing && s_gizmoHandle->id != INVALID_ENTITY)
            {
                auto* g = Coordinator::GetInstance();
                if (g && g->HasComponent<Framework::Transform>(s_gizmoHandle->id))
                {
                    Framework::Transform after =
                        g->GetComponent<Framework::Transform>(s_gizmoHandle->id);

                    if (s_gizmoBeforeTransform != after)
                    {
                        PushTransformEdit(mUndo, s_gizmoHandle, s_gizmoBeforeTransform, after);
                    }
                }

                s_gizmoEditing = false;
                s_gizmoHandle.reset();
            }
        }

        //// ---------------------------------------------------------------------
        //// DELETE KEY (with children support)
        //// ---------------------------------------------------------------------
        if (selectedEntity != INVALID_ENTITY &&
            glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS)
        {
            if (!coord)
                return;

            Entity toDelete = selectedEntity;

            // If gizmo was dragging this entity, stop it
            if (g_gizmo.dragging && g_gizmo.active == toDelete)
            {
                g_gizmo.dragging = false;
                g_gizmo.active = INVALID_ENTITY;
                g_gizmo.activeHandle = GizmoHandle::None;
            }

            if (selectedHandle && selectedHandle->id != INVALID_ENTITY)
            {
                PushDeleteSubtree(
                    mUndo,
                    Editorfactory,
                    entityDisplayOrder,
                    handleOf,
                    selectedEntity,   // root to delete
                    selectedEntity,   // selection ref
                    selectedHandle    // selection handle ref
                );
            }
        }

        dl->PopClipRect();
    }

    void CancelGizmoIfEntity(Entity e)
    {
        if (g_gizmo.active == e)
        {
            g_gizmo.dragging = false;
            g_gizmo.active = INVALID_ENTITY;
            g_gizmo.activeHandle = GizmoHandle::None;
        }
    }
} // namespace PulseEditor