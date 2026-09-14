/******************************************************************************/
/**
 * @file        PostProcessing.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 *
 * @brief       Bloom post-processing pipeline.
 *              Captures the scene into an off-screen FBO, runs a
 *              bright-pass extract + multi-pass Gaussian blur, then
 *              composites the result back to the default framebuffer.
 *
 *              Usage (GraphicsManager::Update):
 *                if (m_postProcess.IsEnabled()) m_postProcess.BeginCapture();
 *                glapp->draw(...);
 *                if (m_postProcess.IsEnabled()) m_postProcess.ApplyBloom();
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              the prior written consent of DigiPen Institute of Technology
 *              is prohibited.
 */
/******************************************************************************/
#pragma once

#include <string>
#include <GL/glew.h>
#include "glslshader.h"

class PostProcessing
{
public:
    // Call once after GL context is ready.
    // w, h       - virtual resolution (e.g. 1920 x 1080)
    // shaderDir  - absolute path to the folder that contains the .vert/.frag files
    void Init(int w, int h, const std::string& shaderDir);

    // Release all GPU resources.
    void Shutdown();

    // Bind the scene FBO so subsequent draw calls write into it.
    // Saves the current GL viewport (= game panel in editor, full window in standalone)
    // so the composite can restore it exactly. Clears the scene FBO.
    void BeginCapture();

    // Extract bright pixels -> blur -> composite scene+bloom to default FBO.
    // Viewport is left at the letterboxed game area so subsequent text/UI calls
    // still render in the right place.
    void ApplyBloom();

    void  SetEnabled(bool v)          { m_enabled        = v;     }
    void  SetBloomIntensity(float v)  { m_bloomIntensity = v;     }
    void  SetBloomThreshold(float v)  { m_bloomThreshold = v;     }
    bool  IsEnabled()          const  { return m_enabled;         }
    float GetBloomIntensity()  const  { return m_bloomIntensity;  }
    float GetBloomThreshold()  const  { return m_bloomThreshold;  }

    // --- Per-object glow pass ---
    // Bind the glow FBO so subsequent draw calls write the glowing-entity silhouettes.
    void BeginGlowCapture();
    // Blur the captured glow texture and composite it additively onto FBO 0.
    // intensity: final brightness multiplier (1.0 is normal; individual entity
    //            intensities are already baked into the silhouette color).
    void ApplyGlowBlur(float intensity = 1.0f);

private:
    // --- FBO / texture handles ---
    GLuint m_sceneFBO      = 0;  GLuint m_sceneTexture   = 0;
    GLuint m_extractFBO    = 0;  GLuint m_extractTexture = 0;
    GLuint m_blurFBO[2]    = {}; GLuint m_blurTexture[2] = {};

    // Glow pass (full resolution, separate from bloom)
    GLuint m_glowFBO     = 0;
    GLuint m_glowTexture = 0;

    // --- Fullscreen quad ---
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;

    // --- Shaders ---
    GLSLShader m_extractShader;
    GLSLShader m_blurShader;
    GLSLShader m_compositeShader;
    GLSLShader m_glowCompositeShader;

    // --- Parameters ---
    float m_bloomIntensity = 1.2f;
    float m_bloomThreshold = 0.5f;
    bool  m_enabled        = false; // off by default; enable via PostProcessAPI

    // --- Dimensions ---
    int m_w  = 0;  int m_h  = 0;   // virtual resolution
    int m_bw = 0;  int m_bh = 0;   // half-resolution for blur FBOs

    // Viewport saved by BeginCapture() and restored by ApplyBloom() composite
    GLint m_savedVP[4] = {};  // [x, y, width, height]

    // --- Helpers ---
    void CreateFBOs();
    void DeleteFBOs();
    void CreateQuad();
    void LoadShaders(const std::string& shaderDir);
    void RenderQuad();
};
