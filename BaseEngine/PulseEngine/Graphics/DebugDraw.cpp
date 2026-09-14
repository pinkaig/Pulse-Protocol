/******************************************************************************/
/**
 * @file        DebugDraw.cpp
 * @project     Pulse Protocol
 * @author		Goh Pin Kai
 * @brief		Implements OpenGL-based debug drawing utilities for rendering
				points, lines, shapes, and primitives to aid development visualization.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "DebugDraw.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Minimal solid color shader sources (embedded)
static const char* kDebugVert = R"GLSL(
#version 450 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec3 aCol;
uniform mat4 uMVP;
out vec3 vCol;
void main(){
    vCol = aCol;
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
}
)GLSL";

static const char* kDebugFrag = R"GLSL(
#version 450 core
in vec3 vCol;
out vec4 FragColor;
void main(){
    FragColor = vec4(vCol, 1.0);
}
)GLSL";

GLSLShader DebugDraw::sShader;
GLuint DebugDraw::sVao = 0;
GLuint DebugDraw::sVbo = 0;
GLint DebugDraw::sLocMvp = -1;
GLint DebugDraw::sLocPos = 0;
GLint DebugDraw::sLocCol = 1;
std::vector<DebugDraw::Vertex> DebugDraw::sLineVertices;
std::vector<DebugDraw::Vertex> DebugDraw::sPointVertices;
std::vector<DebugDraw::Vertex> DebugDraw::sTriangleVertices;
glm::mat4 DebugDraw::sMvp(1.0f);
bool DebugDraw::sBegun = false;

void DebugDraw::Init() {
	if (sVao != 0) return;

	if (!sShader.CompileShaderFromString(GL_VERTEX_SHADER, kDebugVert)) {
		std::exit(EXIT_FAILURE);
	}
	if (!sShader.CompileShaderFromString(GL_FRAGMENT_SHADER, kDebugFrag)) {
		std::exit(EXIT_FAILURE);
	}
	if (!sShader.Link()) {
		std::exit(EXIT_FAILURE);
	}
	if (!sShader.Validate()) {
		std::exit(EXIT_FAILURE);
	}

	glCreateVertexArrays(1, &sVao);
	glCreateBuffers(1, &sVbo);
	glNamedBufferStorage(sVbo, sizeof(Vertex) * 8192, nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
	glVertexArrayVertexBuffer(sVao, 0, sVbo, 0, sizeof(Vertex));
	glEnableVertexArrayAttrib(sVao, 0);
	glVertexArrayAttribFormat(sVao, 0, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
	glVertexArrayAttribBinding(sVao, 0, 0);
	glEnableVertexArrayAttrib(sVao, 1);
	glVertexArrayAttribFormat(sVao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, color));
	glVertexArrayAttribBinding(sVao, 1, 0);

	sLocMvp = glGetUniformLocation(sShader.GetHandle(), "uMVP");
}

void DebugDraw::Shutdown() {
	// Clear vectors to free their memory
	sLineVertices.clear();
	sLineVertices.shrink_to_fit();
	sPointVertices.clear();
	sPointVertices.shrink_to_fit();
	sTriangleVertices.clear();
	sTriangleVertices.shrink_to_fit();

	if (sVao) {
		glDeleteVertexArrays(1, &sVao);
		sVao = 0;
	}
	if (sVbo) {
		glDeleteBuffers(1, &sVbo);
		sVbo = 0;
	}

	// Clean up shader program - THIS IS WHAT'S MISSING
	GLuint handle = sShader.GetHandle();
	if (handle != 0) {
		glDeleteProgram(handle);
	}
}

void DebugDraw::Begin(const glm::mat4& mvp) {
	sBegun = true;
	sMvp = mvp;
	sLineVertices.clear();
	sPointVertices.clear();
	sTriangleVertices.clear();
}

void DebugDraw::End() {
	Flush();
	sBegun = false;
}

void DebugDraw::DrawPoint(const glm::vec2& pos, float size, const glm::vec3& color) {
	(void)size; // using GL_POINTS; size can be set via glPointSize if desired per batch
	sPointVertices.push_back({ pos, color });
}

void DebugDraw::DrawLine(const glm::vec2& a, const glm::vec2& b, const glm::vec3& color) {
	sLineVertices.push_back({ a, color });
	sLineVertices.push_back({ b, color });
}

void DebugDraw::DrawRect(const glm::vec2& minCorner, const glm::vec2& maxCorner, const glm::vec3& color) {
	glm::vec2 a = { minCorner.x, minCorner.y };
	glm::vec2 b = { maxCorner.x,  minCorner.y };
	glm::vec2 c = { maxCorner.x,  maxCorner.y };
	glm::vec2 d = { minCorner.x, maxCorner.y };
	DrawLine(a, b, color);
	DrawLine(b, c, color);
	DrawLine(c, d, color);
	DrawLine(d, a, color);
}

void DebugDraw::DrawRectFilled(const glm::vec2& minCorner, const glm::vec2& maxCorner, const glm::vec3& color) {
	glm::vec2 a = { minCorner.x, minCorner.y };
	glm::vec2 b = { maxCorner.x,  minCorner.y };
	glm::vec2 c = { maxCorner.x,  maxCorner.y };
	glm::vec2 d = { minCorner.x, maxCorner.y };
	sTriangleVertices.push_back({ a, color });
	sTriangleVertices.push_back({ b, color });
	sTriangleVertices.push_back({ c, color });
	sTriangleVertices.push_back({ a, color });
	sTriangleVertices.push_back({ c, color });
	sTriangleVertices.push_back({ d, color });
}

void DebugDraw::DrawCircle(const glm::vec2& center, float radius, const glm::vec3& color, int segments) {
	if (segments < 3) segments = 3;
	float step = 6.28318530718f / static_cast<float>(segments);
	glm::vec2 prev = center + glm::vec2(radius, 0.0f);
	for (int i = 1; i <= segments; ++i) {
		float ang = step * static_cast<float>(i);
		glm::vec2 curr = center + glm::vec2(std::cos(ang) * radius, std::sin(ang) * radius);
		DrawLine(prev, curr, color);
		prev = curr;
	}
}

void DebugDraw::DrawCircleFilled(const glm::vec2& center, float radius, const glm::vec3& color, int segments) {
	if (segments < 3) segments = 3;
	float step = 6.28318530718f / static_cast<float>(segments);
	glm::vec2 prev = center + glm::vec2(radius, 0.0f);
	for (int i = 1; i <= segments; ++i) {
		float ang = step * static_cast<float>(i);
		glm::vec2 curr = center + glm::vec2(std::cos(ang) * radius, std::sin(ang) * radius);
		sTriangleVertices.push_back({ center, color });
		sTriangleVertices.push_back({ prev, color });
		sTriangleVertices.push_back({ curr, color });
		prev = curr;
	}
}

void DebugDraw::Flush() {
	if (sLineVertices.empty() && sPointVertices.empty() && sTriangleVertices.empty()) return;

	sShader.Use();
	if (sLocMvp >= 0) {
		glUniformMatrix4fv(sLocMvp, 1, GL_FALSE, glm::value_ptr(sMvp));
	}
	glBindVertexArray(sVao);

	if (!sTriangleVertices.empty()) {
		glNamedBufferSubData(sVbo, 0, sizeof(Vertex) * sTriangleVertices.size(), sTriangleVertices.data());
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(sTriangleVertices.size()));
	}

	if (!sLineVertices.empty()) {
		glNamedBufferSubData(sVbo, 0, sizeof(Vertex) * sLineVertices.size(), sLineVertices.data());
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(sLineVertices.size()));
	}

	if (!sPointVertices.empty()) {
		glNamedBufferSubData(sVbo, 0, sizeof(Vertex) * sPointVertices.size(), sPointVertices.data());
		glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(sPointVertices.size()));
	}

	glBindVertexArray(0);
	sShader.UnUse();
}


