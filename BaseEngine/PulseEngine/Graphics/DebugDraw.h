/******************************************************************************/
/**
 * @file        DebugDraw.h
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 * @brief		Declares OpenGL-based debug drawing utilities for rendering
				points, lines, shapes, and primitives to aid development visualization.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once

#ifndef DEBUG_DRAW_H
#define DEBUG_DRAW_H

#include "glslshader.h"
#include "pch/pch_temp.h"

class DebugDraw {
public:
	static void Init();
	static void Shutdown();

	static void Begin(const glm::mat4& mvp);
	static void End();

	static void DrawPoint(const glm::vec2& pos, float size, const glm::vec3& color);
	static void DrawLine(const glm::vec2& a, const glm::vec2& b, const glm::vec3& color);
	static void DrawRect(const glm::vec2& minCorner, const glm::vec2& maxCorner, const glm::vec3& color);
	static void DrawRectFilled(const glm::vec2& minCorner, const glm::vec2& maxCorner, const glm::vec3& color);
	static void DrawCircle(const glm::vec2& center, float radius, const glm::vec3& color, int segments = 32);
	static void DrawCircleFilled(const glm::vec2& center, float radius, const glm::vec3& color, int segments = 32);

private:
	struct Vertex { glm::vec2 position; glm::vec3 color; };

	static void Flush();

	static GLSLShader sShader;
	static GLuint sVao;
	static GLuint sVbo;
	static GLint sLocMvp;
	static GLint sLocPos;
	static GLint sLocCol;

	static std::vector<Vertex> sLineVertices;
	static std::vector<Vertex> sPointVertices;
	static std::vector<Vertex> sTriangleVertices;

	static glm::mat4 sMvp;
	static bool sBegun;
};

#endif // DEBUG_DRAW_H



