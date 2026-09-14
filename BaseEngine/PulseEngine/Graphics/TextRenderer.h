/******************************************************************************/
/**
 * @file        TextRenderer.h
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Text rendering with line break (\n) and alignment support
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 *
 /******************************************************************************/
#pragma once

#include "pch/pch_temp.h"
#include "Graphics/Text.h"
#include "glslshader.h"

struct FontData;

class TextRenderer
{
public:
    TextRenderer() = default;
    ~TextRenderer() = default;

    /**
     * @brief Initialize the text renderer with VAO/VBO and projection
     * @param vao Vertex Array Object for text quads
     * @param vbo Vertex Buffer Object for text quads
     * @param projection Orthographic projection matrix
     */
    void Initialize(GLuint vao, GLuint vbo, const glm::mat4& projection);

    /**
     * @brief Update the projection matrix (call on window resize)
     */
    void SetProjection(const glm::mat4& projection);

    /**
     * @brief Render text with line break and alignment support
     * @param shader Text shader program
     * @param text Text to render (supports \n for line breaks)
     * @param x X position
     * @param y Y position
     * @param scale Scale factor
     * @param color Text color (RGB)
     * @param fontData Font glyph data
     * @param alignment Text alignment (Left/Center/Right)
     */
    void RenderText(
        GLSLShader& shader,
        const std::string& text,
        float x, float y,
        float scale,
        const glm::vec4& color,
        const FontData* fontData,
        TextAlignment alignment = TextAlignment::Left
    );

    /**
     * @brief Calculate the width of a single line of text
     * @param text Text to measure (stops at \n)
     * @param scale Scale factor
     * @param fontData Font glyph data
     * @return Width in pixels
     */
    float CalculateTextWidth(
        const std::string& text,
        float scale,
        const FontData* fontData
    );

    /**
     * @brief Calculate the total height of text (including all lines)
     * @param text Text to measure
     * @param scale Scale factor
     * @param fontData Font glyph data
     * @return Height in pixels
     */
    float CalculateTextHeight(
        const std::string& text,
        float scale,
        const FontData* fontData
    );

private:
    /**
     * @brief Split text into lines by \n
     */
    std::vector<std::string> SplitLines(const std::string& text);

    /**
     * @brief Render a single line of text
     */
    void RenderLine(
        GLSLShader& shader,
        const std::string& line,
        float x, float y,
        float scale,
        const FontData* fontData
    );

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    glm::mat4 m_projection{ 1.0f };
};