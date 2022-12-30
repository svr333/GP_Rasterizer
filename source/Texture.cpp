#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		return new Texture(IMG_Load(path.c_str()));
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		int x = uv.x * m_pSurface->w;
		int y = uv.y * m_pSurface->h;

		Uint8 r, g, b;
		SDL_GetRGB(m_pSurfacePixels[x + (y * m_pSurface->w)], m_pSurface->format, &r, &g, &b);

		return { r / 255.0f, g / 255.0f, b / 255.0f };
	}
}