/******************************************************************************/
/**
 * @file        glapp.h
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 * @brief		Declares OpenGL application utilities for initializing
				shaders, models, viewports, and rendering textured entities.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#ifndef GLAPP_H
#define GLAPP_H
#include "glhelper.h"
#include "pch/pch_temp.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Transform.h"
#include "Renderable.h"
#include "Animation.h"
#include "CoreEngine/Assets.h"
#include "Physics/collision.h" 
#include "DebugDraw.h"
#include "Layer.h"
#include "GameLogic/VFX/ParticleSystem.h"

// for .net
#include "CoreEngine/Core/ImportExport.h"
#pragma warning(push)
#pragma warning(disable: 4251) // stupid warning just saying to make sure we are using same compilier type and runtime MDd. WHICH WE ARE SO WHY WARNING STILL??????????

// External transforms list (used elsewhere)   
namespace Framework { class Transform; class Animation; class Renderable; }

class DLL_API GLApp {
public:
	static inline int VIRTUAL_W = 1920;
	static inline int VIRTUAL_H = 1080;
	void init(/*float inputEditorWidth, float inputEditorHeight*/);
	//void update(GLFWwindow* window);
	void update(GLFWwindow* window, std::set<Entity>);
	void draw(std::set<Entity> entities, LayerMask includeMask, float deltaTime);
	// Draw only glow-enabled entities into the currently bound FBO (glow capture pass).
	// The entity silhouettes are rendered using each entity's glow color * intensity.
	void drawGlowing(std::set<Entity>& entities, float dt);
	void cleanup();

	GLuint vaoid; // handle to VAO
	GLuint idx_elem_cnt;
	GLenum primitive_type;
	void drawTrans(Framework::Renderable& x, Framework::Animation& y, Framework::Transform& z, const glm::mat3& ZoomNDC, int drawOrder);
	void updateAnimation(Framework::Animation& x, float deltaTime);

	static inline int GameWidth = VIRTUAL_W;  // actual GL viewport width after letterbox
	static inline int GameHeight = VIRTUAL_H;  // actual GL viewport height after letterbox
	static inline int GameOffsetX = 0;         // viewport X in framebuffer
	static inline int GameOffsetY = 0;         // viewport Y in framebuffer
	static inline float PixelScale = 1.0f;     // uniform scale = min(fbw/VW, fbh/VH)
	static void ComputeLetterbox(int fbw, int fbh);
	static void OnResize(int width, int height);
	void DrawAABB_NDC(const glm::mat3& modelToNDC, const AABB& box, const glm::vec3& color);

	struct BatchRender {
		struct Vtx { glm::vec2 pos; glm::vec2 uv; glm::vec4 color; };

		// Add drawOrder to the Key
		struct Key {
			uint16_t shd;
			uint16_t mdl;
			GLuint   tex;
			int      drawOrder;  // NEW: preserves sort order

			bool operator==(const Key& o) const {
				return shd == o.shd && mdl == o.mdl && tex == o.tex && drawOrder == o.drawOrder;
			}
		};

		struct KeyHash {
			size_t operator()(const Key& k) const noexcept {
				size_t h = std::hash<uint32_t>{}((uint32_t(k.shd) << 16) ^ uint32_t(k.mdl));
				h ^= std::hash<GLuint>{}(k.tex) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
				h ^= std::hash<int>{}(k.drawOrder) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
				return h;
			}
		};
		// one bucket per texture -> one draw call per texture
		std::unordered_map<Key, std::vector<Vtx>, KeyHash> buckets;

		// GPU objects reused every frame
		GLuint vao = 0, vbo = 0, ebo = 0;
		size_t capacitySprites = 0;

		void Init(size_t maxSprites) {
			capacitySprites = maxSprites;
			glCreateVertexArrays(1, &vao);
			glCreateBuffers(1, &vbo);
			glCreateBuffers(1, &ebo);

			// 4 verts + 6 indices per sprite
			const size_t maxVerts = maxSprites * 4;
			const size_t maxIndices = maxSprites * 6;

			glNamedBufferStorage(vbo, maxVerts * sizeof(Vtx), nullptr, GL_DYNAMIC_STORAGE_BIT);
			std::vector<GLushort> ib(maxIndices);
			for (size_t i = 0; i < maxSprites; ++i) {
				GLushort base = static_cast<GLushort>(i * 4);
				size_t off = i * 6;
				ib[off + 0] = base + 0; ib[off + 1] = base + 1; ib[off + 2] = base + 2;
				ib[off + 3] = base + 2; ib[off + 4] = base + 3; ib[off + 5] = base + 0;
			}
			glNamedBufferStorage(ebo, ib.size() * sizeof(GLushort), ib.data(), 0);

			glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vtx));
			glVertexArrayElementBuffer(vao, ebo);

			// layout: location 0 = vec2 pos, location 2 = vec2 uv (matches your VAOs)
			glEnableVertexArrayAttrib(vao, 0);
			glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, offsetof(Vtx, pos));
			glVertexArrayAttribBinding(vao, 0, 0);

			glEnableVertexArrayAttrib(vao, 2);
			glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vtx, uv));
			glVertexArrayAttribBinding(vao, 2, 0);

			// layout: location 1 = vec4 tint color
			glEnableVertexArrayAttrib(vao, 1);
			glVertexArrayAttribFormat(vao, 1, 4, GL_FLOAT, GL_FALSE, offsetof(Vtx, color));
			glVertexArrayAttribBinding(vao, 1, 0);
		}

		void Begin() { buckets.clear(); }

		// shd_ref & mdl_ref come from Framework::Renderable
		void Push(uint16_t shd_ref, uint16_t mdl_ref, GLuint tex, int drawOrder,
			const glm::mat3& modelToNDC, float u0, float v0, float u1, float v1,
			const glm::vec4& color = glm::vec4(1.0f))
		{
			// Choose model-space corners based on model type.
			// mdl_ref==0 is your background (full-screen), others are sprite quads.
			glm::vec2 a, b, c, d;
			if (mdl_ref == 0) {                       // Background quad
				a = { -1.0f, -1.0f };
				b = { 1.0f, -1.0f };
				c = { 1.0f,  1.0f };
				d = { -1.0f,  1.0f };
			}
			else {                                   // Sprite quad
				a = { -0.5f, -0.5f };
				b = { 0.5f, -0.5f };
				c = { 0.5f,  0.5f };
				d = { -0.5f,  0.5f };
			}

			auto toNDC = [&](glm::vec2 p) {
				glm::vec3 h = modelToNDC * glm::vec3(p, 1.0f);
				return glm::vec2(h.x, h.y);
				};

			Key k{ shd_ref, mdl_ref, tex, drawOrder };  // Include drawOrder in key
			auto& buf = buckets[k];
			buf.reserve(buf.size() + 4);

			// UVs after v-fix (above)
			buf.push_back({ toNDC(a), glm::vec2(u0, v0), color }); // BL
			buf.push_back({ toNDC(b), glm::vec2(u1, v0), color }); // BR
			buf.push_back({ toNDC(c), glm::vec2(u1, v1), color }); // TR
			buf.push_back({ toNDC(d), glm::vec2(u0, v1), color }); // TL
		}

		void Flush(std::vector<GLSLShader>& shaderPrograms) {
			glBindVertexArray(vao);

			// Sort buckets by shader/model to ensure consistent draw order
			std::vector<std::pair<Key, std::vector<Vtx>>> sortedBuckets(buckets.begin(), buckets.end());
			std::sort(sortedBuckets.begin(), sortedBuckets.end(), [](const auto& a, const auto& b) {
				if (a.first.drawOrder != b.first.drawOrder)
					return a.first.drawOrder < b.first.drawOrder;  // DRAW ORDER IS PRIMARY
				if (a.first.shd != b.first.shd) return a.first.shd < b.first.shd;
				if (a.first.mdl != b.first.mdl) return a.first.mdl < b.first.mdl;
				return a.first.tex < b.first.tex;
				});

			for (const auto& bucket : sortedBuckets) {
				const Key& k = bucket.first;
				const auto& verts = bucket.second;
				const size_t spriteCount = verts.size() / 4;
				if (!spriteCount) continue;

				auto& shader = shaderPrograms[k.shd];
				shader.Use();

				if (GLint uModel = glGetUniformLocation(shader.GetHandle(), "uModel_to_NDC"); uModel >= 0) {
					glm::mat3 I(1.0f); glUniformMatrix3fv(uModel, 1, GL_FALSE, glm::value_ptr(I));
				}
				if (GLint uOff = glGetUniformLocation(shader.GetHandle(), "uOffset"); uOff >= 0) glUniform2f(uOff, 0, 0);
				if (GLint uScl = glGetUniformLocation(shader.GetHandle(), "uScale");  uScl >= 0) glUniform2f(uScl, 1, 1);

				glNamedBufferSubData(vbo, 0, verts.size() * sizeof(Vtx), verts.data());

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, k.tex);

				glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(spriteCount * 6), GL_UNSIGNED_SHORT, 0);
				shader.UnUse();
			}

			glBindVertexArray(0);
		}
	};

	struct Camera2D {
		glm::vec2 center{ 0.0f, 0.0f };
		float zoom = 0.6f; // 0.5f = normal, >0.5 = zoom in, <0.5 = zoom out
		float     minZoom{ 0.25f };
		float     maxZoom{ 2.0f };
	};

	static Camera2D gEditorCamera;   // used only in editor
	static Camera2D gGameCamera;     // used only during Play

	// Set each frame before draw() to render textured VFX particles through gBatch
	static void SetVFXParticles(const std::vector<Particle>* particles) { sVFXParticles = particles; }
	static const std::vector<Particle>* sVFXParticles;


	static glm::vec2 ScreenToNDC(double x, double y);
	static glm::mat3 BuildPanZoomNDC(const Camera2D& cam);

	static std::vector<GLSLShader> shdrpgms; // singleton in tutorial 3

	using VPSS = std::vector<std::pair<std::string, std::string>>;
	static void init_shdrpgms_cont(GLApp::VPSS const&);

	struct GLViewport {
		GLint x, y;
		GLsizei width, height;
	};
	static std::vector<GLViewport> vps; // container for viewports
};
#pragma warning(pop)
#endif /* GLAPP_H */
