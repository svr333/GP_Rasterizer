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
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
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
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	auto v0 = Vector3{ 0, 0.5, 1 };
	auto v1 = Vector3{ 0.5, -0.5, 1 };
	auto v2 = Vector3{ -0.5, -0.5, 1 };

	auto v0ss = Vector2{ static_cast<float>((v0.x + 1) / 2 * m_Width),static_cast<float>((1 - v0.y) / 2 * m_Height) };
	auto v1ss = Vector2{ static_cast<float>((v1.x + 1) / 2 * m_Width),static_cast<float>((1 - v1.y) / 2 * m_Height) };
	auto v2ss = Vector2{ static_cast<float>((v2.x + 1) / 2 * m_Width),static_cast<float>((1 - v2.y) / 2 * m_Height) };

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			Vector2 pixel{ static_cast<float>(px),static_cast<float>(py) };
			Vector2 p0ToPixel = pixel - v0ss;

			Vector2 edge1 = { v1ss - v0ss };

			if (Vector2::Cross(edge1, p0ToPixel) < 0)
			{
				continue;
			}

			Vector2 p1ToPixel = pixel - v1ss;
			Vector2 edge2 = { v2ss - v1ss };

			if (Vector2::Cross(edge2, p1ToPixel) < 0)
			{
				continue;
			}

			Vector2 p2ToPixel = pixel - v2ss;
			Vector2 edge3 = { v0ss - v2ss };

			if (Vector2::Cross(edge3, p2ToPixel) < 0)
			{
				continue;
			}

			// if pixel is in triangle
			ColorRGB finalColor{ 1,1,1 };

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
	//Todo > W1 Projection Stage
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
