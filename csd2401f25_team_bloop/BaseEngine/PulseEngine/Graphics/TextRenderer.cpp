/******************************************************************************/
/**
 * @file        TextRenderer.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En
 * @brief       Text rendering implementation with line break and alignment support
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "GraphicsManager.h"
#include "TextRenderer.h"

void TextRenderer::Initialize(GLuint vao, GLuint vbo, const glm::mat4& projection)
{
    m_vao = vao;
    m_vbo = vbo;
    m_projection = projection;
}

void TextRenderer::SetProjection(const glm::mat4& projection)
{
    m_projection = projection;
}

std::vector<std::string> TextRenderer::SplitLines(const std::string& text)
{
    std::vector<std::string> lines;
    std::string currentLine;

    for (char c : text)
    {
        if (c == '\n')
        {
            lines.push_back(currentLine);
            currentLine.clear();
        }
        else
        {
            currentLine += c;
        }
    }

    // Don't forget the last line (no trailing \n)
    lines.push_back(currentLine);

    return lines;
}

float TextRenderer::CalculateTextWidth(
    const std::string& text,
    float scale,
    const FontData* fontData)
{
    if (!fontData) return 0.0f;

    float width = 0.0f;

    for (unsigned char c : text)
    {
        // Stop at newline for single line width
        if (c == '\n') break;

        auto it = fontData->glyphs.find(c);
        if (it == fontData->glyphs.end()) continue;

        // Advance is in 1/64 pixels
        width += (it->second.Advance >> 6) * scale;
    }

    return width;
}

float TextRenderer::CalculateTextHeight(
    const std::string& text,
    float scale,
    const FontData* fontData)
{
    if (!fontData) return 0.0f;

    std::vector<std::string> lines = SplitLines(text);
    float lineHeight = fontData->pixelHeight * scale * 1.2f;

    return lines.size() * lineHeight;
}

void TextRenderer::RenderLine(
    GLSLShader& /*shader*/,
    const std::string& line,
    float x, float y,
    float scale,
    const FontData* fontData)
{
    float currentX = x;

    for (unsigned char c : line)
    {
        auto it = fontData->glyphs.find(c);
        if (it == fontData->glyphs.end())
            continue;

        const auto& ch = it->second;

        // Screen quad placement
        float xpos = currentX + ch.Bearing.x * scale;
        float ypos = y - ch.Bearing.y * scale + ch.Size.y * scale;
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // 6 verts, each vert = {pos.x, pos.y, uv.x, uv.y}
        float verts[6][4] = {
            {xpos,     ypos,     0.0f, 1.0f},
            {xpos,     ypos - h, 0.0f, 0.0f},
            {xpos + w, ypos - h, 1.0f, 0.0f},

            {xpos,     ypos,     0.0f, 1.0f},
            {xpos + w, ypos - h, 1.0f, 0.0f},
            {xpos + w, ypos,     1.0f, 1.0f}
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursor (FreeType gives advance in 1/64 pixels)
        currentX += (ch.Advance >> 6) * scale;
    }
}

void TextRenderer::RenderText(
    GLSLShader& shader,
    const std::string& text,
    float x, float y,
    float scale,
    const glm::vec4& color,
    const FontData* fontData,
    TextAlignment alignment)
{
    if (!fontData)
    {
        std::cerr << "[TextRenderer] fontData not found!\n";
        return;
    }

    if (text.empty())
        return;

    // Setup shader
    shader.Use();

    glUniformMatrix4fv(
        glGetUniformLocation(shader.GetHandle(), "projection"),
        1, GL_FALSE,
        glm::value_ptr(m_projection));

    glUniform4f(
        glGetUniformLocation(shader.GetHandle(), "textColor"),
        color.x, color.y, color.z, color.w);

    glUniform1i(
        glGetUniformLocation(shader.GetHandle(), "text"),
        0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_vao);

    // Split text into lines
    std::vector<std::string> lines = SplitLines(text);

    // Calculate line height
    float lineHeight = fontData->pixelHeight * scale * 1.2f;

    float currentY = y;

    // Render each line
    for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
    {
        const std::string& line = lines[lineIndex];

        if (line.empty())
        {
            // Empty line, just move down
            currentY += lineHeight;
            continue;
        }

        // Calculate the width of this line for horizontal alignment
        float lineWidth = CalculateTextWidth(line, scale, fontData);
        float lineX = x;

        // Apply horizontal alignment
        switch (alignment)
        {   
        case TextAlignment::Left:
            // Left aligned: x is the left edge of the text
            lineX = x;
            break;

        case TextAlignment::Center:
            // Center aligned: x is the center point, subtract half width to get left edge
            lineX = x - (lineWidth / 2.0f);
            break;

        case TextAlignment::Right:
            // Right aligned: x is the right edge, subtract full width to get left edge
            lineX = x - lineWidth;
            break;

        default:
            lineX = x;
            break;
        }

        // Render the line at calculated position
        RenderLine(shader, line, lineX, currentY, scale, fontData);

        // Move to next line
        currentY += lineHeight;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}