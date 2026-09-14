/******************************************************************************/
/**
 * @file        PostProcessing.cpp
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 *
 * @brief       Implements the bloom post-processing pipeline.
 *              See PostProcessing.h for usage.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              the prior written consent of DigiPen Institute of Technology
 *              is prohibited.
 */
/******************************************************************************/
#include "PostProcessing.h"

#include <iostream>
#include <filesystem>

// ============================================================
//  Init / Shutdown
// ============================================================

void PostProcessing::Init(int w, int h, const std::string& shaderDir)
{
    m_w  = w;
    m_h  = h;
    m_bw = w / 2;
    m_bh = h / 2;

    CreateFBOs();
    CreateQuad();
    LoadShaders(shaderDir);

    std::cout << "[PostProcessing] Initialized (" << w << "x" << h
              << ", blur " << m_bw << "x" << m_bh << ")\n";
}

void PostProcessing::Shutdown()
{
    DeleteFBOs();

    if (m_quadVAO) { glDeleteVertexArrays(1, &m_quadVAO); m_quadVAO = 0; }
    if (m_quadVBO) { glDeleteBuffers(1,     &m_quadVBO);  m_quadVBO = 0; }

    m_extractShader.DeleteShaderProgram();
    m_blurShader.DeleteShaderProgram();
    m_compositeShader.DeleteShaderProgram();
    m_glowCompositeShader.DeleteShaderProgram();

    std::cout << "[PostProcessing] Shutdown complete\n";
}

// ============================================================
//  Frame capture
// ============================================================

void PostProcessing::BeginCapture()
{
    // Save the current viewport — in the editor this is the game panel rect
    // (set by Editor::Render the previous frame via glViewport). In a standalone
    // game it's the full letterboxed area. The composite will restore this so
    // the scene is displayed at exactly the right position and scale.
    glGetIntegerv(GL_VIEWPORT, m_savedVP);

    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
    glViewport(0, 0, m_w, m_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

// ============================================================
//  Bloom pipeline
// ============================================================

void PostProcessing::ApplyBloom()
{
    // Post-process passes don't need alpha blending
    glDisable(GL_BLEND);

    // ---- 1. Bright-pass extract (full res -> half res) ----
    glBindFramebuffer(GL_FRAMEBUFFER, m_extractFBO);
    glViewport(0, 0, m_bw, m_bh);
    glClear(GL_COLOR_BUFFER_BIT);

    m_extractShader.Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
    m_extractShader.SetUniform("uScene",     0);
    m_extractShader.SetUniform("uThreshold", m_bloomThreshold);
    RenderQuad();
    m_extractShader.UnUse();

    // ---- 2. Ping-pong Gaussian blur (5 iterations) ----
    // Start with the extract result in the first blur FBO, then alternate.
    // bool horizontal = true;
    // First pass: extract texture -> blurFBO[0]
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO[0]);
        glViewport(0, 0, m_bw, m_bh);
        glClear(GL_COLOR_BUFFER_BIT);

        m_blurShader.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_extractTexture);
        m_blurShader.SetUniform("uTex",        0);
        m_blurShader.SetUniform("uHorizontal", (GLboolean)GL_TRUE);
        RenderQuad();
        m_blurShader.UnUse();
    }

    // Remaining 9 passes (alternating V/H)
    for (int i = 1; i < 10; ++i)
    {
        int dst = i & 1;   // alternates 1,0,1,0,...
        int src = 1 - dst;

        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO[dst]);
        glViewport(0, 0, m_bw, m_bh);
        glClear(GL_COLOR_BUFFER_BIT);

        m_blurShader.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_blurTexture[src]);
        m_blurShader.SetUniform("uTex",        0);
        // Alternate: pass 1 = vertical, 2 = horizontal, ...
        GLboolean isHoriz = (i % 2 == 0) ? GL_TRUE : GL_FALSE;
        m_blurShader.SetUniform("uHorizontal", isHoriz);
        RenderQuad();
        m_blurShader.UnUse();
    }
    // After 10 passes (0..9), last dst = 9&1 = 1, so final bloom is in m_blurTexture[1]
    // Pass 0 -> dst=0, Pass 1 -> dst=1, ..., Pass 9 -> dst=1
    GLuint finalBloom = m_blurTexture[1];

    // ---- 3. Composite to default framebuffer ----
    // Use the viewport that was active when BeginCapture() was called.
    // In the editor that is the game-panel rect (set by Editor::Render last frame).
    // In a standalone build it is the full letterboxed area — both are correct.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(m_savedVP[0], m_savedVP[1], m_savedVP[2], m_savedVP[3]);

    m_compositeShader.Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
    m_compositeShader.SetUniform("uScene", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, finalBloom);
    m_compositeShader.SetUniform("uBloom",          1);
    m_compositeShader.SetUniform("uBloomIntensity", m_bloomIntensity);

    RenderQuad();
    m_compositeShader.UnUse();

    // Restore blend state for text/UI/overlay rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clean up texture units
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================
//  Per-object glow capture & composite
// ============================================================

void PostProcessing::BeginGlowCapture()
{
    // Save current viewport so ApplyGlowBlur can restore it
    glGetIntegerv(GL_VIEWPORT, m_savedVP);

    glBindFramebuffer(GL_FRAMEBUFFER, m_glowFBO);
    glViewport(0, 0, m_w, m_h);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void PostProcessing::ApplyGlowBlur(float intensity)
{
    glDisable(GL_BLEND);

    // ---- Blur the captured glow silhouette (ping-pong, reuse bloom FBOs) ----
    // Pass 0: glow texture -> blurFBO[0] (horizontal)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO[0]);
        glViewport(0, 0, m_bw, m_bh);
        glClear(GL_COLOR_BUFFER_BIT);

        m_blurShader.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_glowTexture);
        m_blurShader.SetUniform("uTex",        0);
        m_blurShader.SetUniform("uHorizontal", (GLboolean)GL_TRUE);
        RenderQuad();
        m_blurShader.UnUse();
    }

    // Remaining 9 passes (alternating V/H)
    for (int i = 1; i < 10; ++i)
    {
        int dst = i & 1;
        int src = 1 - dst;

        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO[dst]);
        glViewport(0, 0, m_bw, m_bh);
        glClear(GL_COLOR_BUFFER_BIT);

        m_blurShader.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_blurTexture[src]);
        m_blurShader.SetUniform("uTex",        0);
        GLboolean isHoriz = (i % 2 == 0) ? GL_TRUE : GL_FALSE;
        m_blurShader.SetUniform("uHorizontal", isHoriz);
        RenderQuad();
        m_blurShader.UnUse();
    }
    // After 10 passes, final blur result is in m_blurTexture[1]
    GLuint finalGlow = m_blurTexture[1];

    // ---- Composite additively onto the default FBO ----
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(m_savedVP[0], m_savedVP[1], m_savedVP[2], m_savedVP[3]);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);  // additive blending

    m_glowCompositeShader.Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, finalGlow);
    m_glowCompositeShader.SetUniform("uGlowTex",       0);
    m_glowCompositeShader.SetUniform("uGlowIntensity", intensity);
    RenderQuad();
    m_glowCompositeShader.UnUse();

    // Restore normal alpha blending for text/UI/overlay rendering
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clean up texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================
//  Private helpers
// ============================================================

void PostProcessing::CreateFBOs()
{
    // --- Scene FBO (full virtual resolution) ---
    glCreateTextures(GL_TEXTURE_2D, 1, &m_sceneTexture);
    glTextureStorage2D(m_sceneTexture, 1, GL_RGBA8, m_w, m_h);
    glTextureParameteri(m_sceneTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_sceneTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_sceneTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_sceneTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &m_sceneFBO);
    glNamedFramebufferTexture(m_sceneFBO, GL_COLOR_ATTACHMENT0, m_sceneTexture, 0);

    if (glCheckNamedFramebufferStatus(m_sceneFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[PostProcessing] Scene FBO incomplete!\n";

    // --- Extract FBO (half resolution) ---
    glCreateTextures(GL_TEXTURE_2D, 1, &m_extractTexture);
    glTextureStorage2D(m_extractTexture, 1, GL_RGBA8, m_bw, m_bh);
    glTextureParameteri(m_extractTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_extractTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_extractTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_extractTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &m_extractFBO);
    glNamedFramebufferTexture(m_extractFBO, GL_COLOR_ATTACHMENT0, m_extractTexture, 0);

    if (glCheckNamedFramebufferStatus(m_extractFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[PostProcessing] Extract FBO incomplete!\n";

    // --- Glow capture FBO (full resolution) ---
    glCreateTextures(GL_TEXTURE_2D, 1, &m_glowTexture);
    glTextureStorage2D(m_glowTexture, 1, GL_RGBA8, m_w, m_h);
    glTextureParameteri(m_glowTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_glowTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_glowTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_glowTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &m_glowFBO);
    glNamedFramebufferTexture(m_glowFBO, GL_COLOR_ATTACHMENT0, m_glowTexture, 0);

    if (glCheckNamedFramebufferStatus(m_glowFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[PostProcessing] Glow FBO incomplete!\n";

    // --- Blur ping-pong FBOs (half resolution) ---
    glCreateTextures(GL_TEXTURE_2D, 2, m_blurTexture);
    glCreateFramebuffers(2, m_blurFBO);

    for (int i = 0; i < 2; ++i)
    {
        glTextureStorage2D(m_blurTexture[i], 1, GL_RGBA8, m_bw, m_bh);
        glTextureParameteri(m_blurTexture[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_blurTexture[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_blurTexture[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_blurTexture[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glNamedFramebufferTexture(m_blurFBO[i], GL_COLOR_ATTACHMENT0, m_blurTexture[i], 0);

        if (glCheckNamedFramebufferStatus(m_blurFBO[i], GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "[PostProcessing] Blur FBO[" << i << "] incomplete!\n";
    }
}

void PostProcessing::DeleteFBOs()
{
    if (m_sceneFBO)        { glDeleteFramebuffers(1, &m_sceneFBO);        m_sceneFBO        = 0; }
    if (m_sceneTexture)    { glDeleteTextures(1,    &m_sceneTexture);     m_sceneTexture    = 0; }
    if (m_extractFBO)      { glDeleteFramebuffers(1, &m_extractFBO);      m_extractFBO      = 0; }
    if (m_extractTexture)  { glDeleteTextures(1,    &m_extractTexture);   m_extractTexture  = 0; }
    if (m_glowFBO)         { glDeleteFramebuffers(1, &m_glowFBO);         m_glowFBO         = 0; }
    if (m_glowTexture)     { glDeleteTextures(1,    &m_glowTexture);      m_glowTexture     = 0; }

    glDeleteFramebuffers(2, m_blurFBO);
    glDeleteTextures(2,     m_blurTexture);
    m_blurFBO[0] = m_blurFBO[1] = 0;
    m_blurTexture[0] = m_blurTexture[1] = 0;
}

void PostProcessing::CreateQuad()
{
    // Fullscreen NDC quad with UVs: 2 triangles covering [-1,1]x[-1,1]
    // Layout: vec2 pos, vec2 uv  (interleaved, 4 floats per vertex)
    float verts[] = {
        // pos          uv
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,

        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f,
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    // location 0: vec2 pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // location 1: vec2 uv
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PostProcessing::LoadShaders(const std::string& shaderDir)
{
    namespace fs = std::filesystem;
    auto path = [&](const char* name) -> std::string {
        return (fs::path(shaderDir) / name).string();
    };

    std::string vsPath = path("postprocess_vs.vert");

    if (!m_extractShader.CompileLinkValidate({
            { GL_VERTEX_SHADER,   vsPath },
            { GL_FRAGMENT_SHADER, path("bloom_extract_fs.frag") }
        }))
    {
        std::cerr << "[PostProcessing] Extract shader failed: " << m_extractShader.GetLog() << "\n";
    }

    if (!m_blurShader.CompileLinkValidate({
            { GL_VERTEX_SHADER,   vsPath },
            { GL_FRAGMENT_SHADER, path("bloom_blur_fs.frag") }
        }))
    {
        std::cerr << "[PostProcessing] Blur shader failed: " << m_blurShader.GetLog() << "\n";
    }

    if (!m_compositeShader.CompileLinkValidate({
            { GL_VERTEX_SHADER,   vsPath },
            { GL_FRAGMENT_SHADER, path("bloom_composite_fs.frag") }
        }))
    {
        std::cerr << "[PostProcessing] Composite shader failed: " << m_compositeShader.GetLog() << "\n";
    }

    if (!m_glowCompositeShader.CompileLinkValidate({
            { GL_VERTEX_SHADER,   vsPath },
            { GL_FRAGMENT_SHADER, path("glow_composite_fs.frag") }
        }))
    {
        std::cerr << "[PostProcessing] Glow composite shader failed: " << m_glowCompositeShader.GetLog() << "\n";
    }
}

void PostProcessing::RenderQuad()
{
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
