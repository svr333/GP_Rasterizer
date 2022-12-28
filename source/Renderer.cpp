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
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
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

	std::vector<Vertex> vertices_world {
		{{ 0.f, 2.f, 0.f }, { 1.f, 0.f, 0.f }},
		{{ 1.5f, -1.f, 0.f }, { 1.f, 0.f, 0.f }},
		{{ -1.5f, -1.f, 0.f }, { 1.f, 0.f, 0.f }},

		{{ 0.f, 4.f, 2.f }, { 1.f, 0.f, 0.f }},
		{{ 3.f, -2.f, 2.f }, { 0.f, 1.f, 0.f }},
		{{ -3.f, -2.f, 2.f }, { 0.f, 0.f, 1.f }},
	};

	std::vector<Vertex> vertices;
	VertexTransformationFunction(vertices_world, vertices);

	for (size_t i = 0; i < vertices.size(); i += 3)
	{
		auto top = std::min(std::max(std::max(vertices[i].position.y, vertices[i + 1].position.y), vertices[i + 2].position.y), static_cast<float>(m_Height));
		auto bottom = std::max(std::min(std::min(vertices[i].position.y, vertices[i + 1].position.y), vertices[i + 2].position.y), 0.f);
		auto left = std::max(std::min(std::min(vertices[i].position.x, vertices[i + 1].position.x), vertices[i + 2].position.x), 0.f);
		auto right = std::min(std::max(std::max(vertices[i].position.x, vertices[i + 1].position.x), vertices[i + 2].position.x), static_cast<float>(m_Width));

		for (int px{ static_cast<int>(left) }; px < static_cast<int>(right); ++px)
		{
			for (int py{ static_cast<int>(bottom) }; py < static_cast<int>(top); ++py)
			{
				Vector2 pixel{ static_cast<float>(px), static_cast<float>(py) };
				Vector2 p0ToPixel = pixel - vertices[i].position.GetXY();

				Vector2 edge1 = { vertices[i + 1].position.GetXY() - vertices[i].position.GetXY() };
				auto cross0 = Vector2::Cross(edge1, p0ToPixel);

				if (cross0 < 0)
				{
					continue;
				}

				Vector2 p1ToPixel = pixel - vertices[i + 1].position.GetXY();
				Vector2 edge2 = { vertices[i + 2].position.GetXY() - vertices[i + 1].position.GetXY() };
				auto cross1 = Vector2::Cross(edge2, p1ToPixel);

				if (cross1 < 0)
				{
					continue;
				}

				Vector2 p2ToPixel = pixel - vertices[i + 2].position.GetXY();
				Vector2 edge3 = { vertices[i].position.GetXY() - vertices[i + 2].position.GetXY() };
				auto cross2 = Vector2::Cross(edge3, p2ToPixel);

				if (cross2 < 0)
				{
					continue;
				}

				// interpolate depth
				auto total = cross0 + cross1 + cross2;
				auto z = cross1 / total * vertices[i].position.z + cross2 / total * vertices[i + 1].position.z + cross0 / total * vertices[i + 2].position.z;

				// depth test
				if (z > m_pDepthBufferPixels[px + py * m_Width])
				{
					continue;
				}

				m_pDepthBufferPixels[px + py * m_Width] = z;

				// interpolate color
				ColorRGB finalColor = cross1 / total * vertices[i].color + cross2 / total * vertices[i + 1].color + cross0 / total * vertices[i + 2].color;

				//Update Color in Buffer
				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out = vertices_in;
	auto aspectRatio = m_Width / m_Height;

	for (auto& v : vertices_out)
	{
		v.position = m_Camera.viewMatrix.TransformPoint(v.position);
		v.position.x /= (m_Camera.fov * aspectRatio);
		v.position.y /= m_Camera.fov;
		v.position.x /= v.position.z;
		v.position.y /= v.position.z;

		v.position.x = static_cast<float>((v.position.x + 1) / 2 * m_Width);
		v.position.y = static_cast<float>((1 - v.position.y) / 2 * m_Height);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
