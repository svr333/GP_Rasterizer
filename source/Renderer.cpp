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

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f, .0f, -10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	std::vector<Vertex> vertices_world {
		{{ 0.f, 4.f, 2.f }, { 1.f, 0.f, 0.f }},
		{{ 3.f, -2.f, 2.f }, { 0.f, 1.f, 0.f }},
		{{ -3.f, -2.f, 2.f }, { 0.f, 0.f, 1.f }},
	};

	std::vector<Vertex> vertices;
	VertexTransformationFunction(vertices_world, vertices);

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			Vector2 pixel{ static_cast<float>(px),static_cast<float>(py) };
			Vector2 p0ToPixel = pixel - vertices[0].position.GetXY();

			Vector2 edge1 = { vertices[1].position.GetXY() - vertices[0].position.GetXY() };
			auto cross0 = Vector2::Cross(edge1, p0ToPixel);

			if (cross0 < 0)
			{
				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(0),
					static_cast<uint8_t>(0),
					static_cast<uint8_t>(0));
				continue;
			}

			Vector2 p1ToPixel = pixel - vertices[1].position.GetXY();
			Vector2 edge2 = { vertices[2].position.GetXY() - vertices[1].position.GetXY() };
			auto cross1 = Vector2::Cross(edge2, p1ToPixel);

			if (cross1 < 0)
			{
				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(0),
					static_cast<uint8_t>(0),
					static_cast<uint8_t>(0));
				continue;
			}

			Vector2 p2ToPixel = pixel - vertices[2].position.GetXY();
			Vector2 edge3 = { vertices[0].position.GetXY() - vertices[2].position.GetXY() };
			auto cross2 = Vector2::Cross(edge3, p2ToPixel);

			if (cross2 < 0)
			{
				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(0),
					static_cast<uint8_t>(0),
					static_cast<uint8_t>(0));
				continue;
			}

			auto total = cross0 + cross1 + cross2;

			// interpolate color
			ColorRGB finalColor = cross1 / total * vertices[0].color + cross2 / total * vertices[1].color + cross0 / total * vertices[2].color;

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
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
