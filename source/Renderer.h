#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	struct Vertex_Out;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;
		void ToggleDepthBufferVisualization();
		void ToggleMeshRotation();
		void ToggleNormalMap();
		void CycleLightingMode();

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};
		float m_MeshRotation = 0.0f;

		int m_Width{};
		int m_Height{};

		Mesh m_Mesh{};
		Texture* m_pTexture = nullptr;
		Texture* m_pNormal = nullptr;
		Texture* m_pGloss = nullptr;
		Texture* m_pSpecular = nullptr;

		enum class LightingMode
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined,
			End
		};

		LightingMode m_LightingMode{ LightingMode::Combined };
		bool m_DepthBufferVisualization = false;
		bool m_RotateMesh = false;
		bool m_UseNormalMap = true;

		void VertexTransformationFunction(std::vector<Mesh>& meshes) const;

		void RenderTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2) const;
		void RenderMeshes(const std::vector<Mesh>& meshes) const;

		ColorRGB PixelShading(const Vertex_Out& v) const;
		ColorRGB Phong(ColorRGB specular, float gloss, Vector3 lightDir, Vector3 viewDir, Vector3 normal) const;
	};
}
