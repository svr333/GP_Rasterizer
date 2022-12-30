//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f, .0f, -10.f });

	m_pTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	// Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// Clear BackBuffer
	// convert rgb to decimal
	auto decimalColor = (100 << 16) + (100 << 8) + 100;
	SDL_FillRect(m_pBackBuffer, NULL, decimalColor);

	// Initialize Depth buffer
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, std::numeric_limits<float>::max());

	std::vector<Mesh> meshes_world
	{
		Mesh
		{
			{
				Vertex{{-3, 3, -2}, colors::White, {0, 0}},
				Vertex{{0, 3, -2}, colors::White, {0.5, 0}},
				Vertex{{3, 3, -2}, colors::White, {1, 0}},
				Vertex{{-3, 0, -2}, colors::White, {0, 0.5}},
				Vertex{{0, 0, -2}, colors::White, {0.5, 0.5}},
				Vertex{{3, 0, -2}, colors::White, {1, 0.5}},
				Vertex{{-3, -3, -2}, colors::White, {0, 1}},
				Vertex{{0, -3, -2}, colors::White, {0.5, 1}},
				Vertex{{3, -3, -2}, colors::White, {1, 1}},
			},
			{
			3, 0, 4,    1, 5, 2,
			2, 6,
			6, 3, 7,    4, 8, 5
				//3, 0, 1,   1, 4, 3,   4, 1, 2,
				//2, 5, 4,   6, 3, 4,   4, 7, 6,
				//7, 4, 5,   5, 8, 7
			},
			PrimitiveTopology::TriangleStrip
		}
	};

	VertexTransformationFunction(meshes_world);
	RenderMeshes(meshes_world);

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(std::vector<Mesh>& meshes) const
{
	auto aspectRatio = m_Width / m_Height;

	for (auto& mesh : meshes)
	{
		mesh.vertices_out = mesh.vertices;

		for (auto& v : mesh.vertices_out)
		{
			v.position = m_Camera.viewMatrix.TransformPoint(v.position);
			v.position.x /= v.position.z * (m_Camera.fov * aspectRatio);
			v.position.y /= v.position.z * m_Camera.fov;

			v.position.x = static_cast<float>((v.position.x + 1) / 2 * m_Width);
			v.position.y = static_cast<float>((1 - v.position.y) / 2 * m_Height);
		}
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::RenderTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) const
{
	auto top = std::min(std::max(std::max(v0.position.y, v1.position.y), v2.position.y), static_cast<float>(m_Height));
	auto bottom = std::max(std::min(std::min(v0.position.y, v1.position.y), v2.position.y), 0.f);
	auto left = std::max(std::min(std::min(v0.position.x, v1.position.x), v2.position.x), 0.f);
	auto right = std::min(std::max(std::max(v0.position.x, v1.position.x), v2.position.x), static_cast<float>(m_Width));

	for (int px{ static_cast<int>(left) }; px < static_cast<int>(right); ++px)
	{
		for (int py{ static_cast<int>(bottom) }; py < static_cast<int>(top); ++py)
		{
			Vector2 pixel{ static_cast<float>(px), static_cast<float>(py) };
			Vector2 p0ToPixel = pixel - v0.position.GetXY();

			Vector2 edge1 = { v1.position.GetXY() - v0.position.GetXY() };
			auto cross0 = Vector2::Cross(edge1, p0ToPixel);

			if (cross0 < 0)
			{
				continue;
			}

			Vector2 p1ToPixel = pixel - v1.position.GetXY();
			Vector2 edge2 = { v2.position.GetXY() - v1.position.GetXY() };
			auto cross1 = Vector2::Cross(edge2, p1ToPixel);

			if (cross1 < 0)
			{
				continue;
			}

			Vector2 p2ToPixel = pixel - v2.position.GetXY();
			Vector2 edge3 = { v0.position.GetXY() - v2.position.GetXY() };
			auto cross2 = Vector2::Cross(edge3, p2ToPixel);

			if (cross2 < 0)
			{
				continue;
			}

			// interpolate depth
			auto total = std::max(cross0 + cross1 + cross2, FLT_EPSILON);
			auto z = cross1 / total * v0.position.z + cross2 / total * v1.position.z + cross0 / total * v2.position.z;

			// depth test
			if (z > m_pDepthBufferPixels[px + py * m_Width])
			{
				continue;
			}

			m_pDepthBufferPixels[px + py * m_Width] = z;

			// interpolate color
			//ColorRGB finalColor = cross1 / total * v0.color + cross2 / total * v1.color + cross0 / total * v2.color;
			ColorRGB finalColor = m_pTexture->Sample(cross1 / total * v0.uv + cross2 / total * v1.uv + cross0 / total * v2.uv);

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void dae::Renderer::RenderMeshes(const std::vector<Mesh>& meshes) const
{
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto mesh = meshes[i];

		if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
		{
			for (size_t i = 0; i < mesh.indices.size() - 2; i += 3)
			{
				RenderTriangle(mesh.vertices_out[mesh.indices[i]], mesh.vertices_out[mesh.indices[i + 1]], mesh.vertices_out[mesh.indices[i + 2]]);
			}
		}
		else if (meshes[i].primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			for (size_t i = 0; i < mesh.indices.size() - 2; ++i)
			{
				// try optimize without if statement, either 2 for loops or just adding/substracting the result of the modulo directly
				if (i % 2)
				{
					RenderTriangle(mesh.vertices_out[mesh.indices[i]], mesh.vertices_out[mesh.indices[i + 2]], mesh.vertices_out[mesh.indices[i + 1]]);
				}
				else
				{
					RenderTriangle(mesh.vertices_out[mesh.indices[i]], mesh.vertices_out[mesh.indices[i + 1]], mesh.vertices_out[mesh.indices[i + 2]]);
				}
			}
		}
	}
}
