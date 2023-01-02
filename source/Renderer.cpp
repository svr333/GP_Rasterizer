//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"
#include <iostream>

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
	auto aspectRatio = static_cast<float>(m_Width) / m_Height;
	m_Camera.Initialize(aspectRatio, 45.f, { .0f, 0.0f, 0.0f });

	// Load textures
	m_pTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pNormal = Texture::LoadFromFile("Resources/vehicle_normal.png");
	m_pGloss = Texture::LoadFromFile("Resources/vehicle_gloss.png");
	m_pSpecular = Texture::LoadFromFile("Resources/vehicle_specular.png");

	// Initialize mesh
	Utils::ParseOBJ("Resources/vehicle.obj", m_Mesh.vertices, m_Mesh.indices);
	m_Mesh.primitiveTopology = PrimitiveTopology::TriangleList;
	m_Mesh.worldMatrix = Matrix::CreateTranslation(0.f, 0.f, 50.f);
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
	delete m_pNormal;
	delete m_pGloss;
	delete m_pSpecular;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if (m_RotateMesh)
	{
		m_MeshRotation += pTimer->GetElapsed();

		m_Mesh.worldMatrix = Matrix::CreateRotationY(m_MeshRotation) * Matrix::CreateTranslation(0.f, 0.f, 50.f);
	}
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

	std::vector<Mesh> meshes_world{ m_Mesh };

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
	for (auto& mesh : meshes)
	{
		Matrix matrix{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };
		mesh.vertices_out.clear();
		mesh.vertices_out.reserve(mesh.vertices.size());

		for (size_t i{}; i < mesh.vertices.size(); ++i)
		{
			Vertex_Out v{};

			v.position = matrix.TransformPoint({ mesh.vertices[i].position, 1.f });

			v.position.x /= v.position.w;
			v.position.y /= v.position.w;
			v.position.z /= v.position.w;

			v.position.x = ((1.f + v.position.x) / 2.f) * m_Width;
			v.position.y = ((1.f - v.position.y) / 2.f) * m_Height;

			v.color = mesh.vertices[i].color;
			v.uv = mesh.vertices[i].uv;
			v.normal = mesh.worldMatrix.TransformVector(mesh.vertices[i].normal);
			v.tangent = mesh.worldMatrix.TransformVector(mesh.vertices[i].tangent);

			mesh.vertices_out.emplace_back(v);
		}
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::ToggleDepthBufferVisualization()
{
	m_DepthBufferVisualization = !m_DepthBufferVisualization;
	std::cout << "Toggled Depth Visualization To: " << m_DepthBufferVisualization << "\n";
}

void Renderer::ToggleMeshRotation()
{
	m_RotateMesh = !m_RotateMesh;
	std::cout << "Toggled Mesh Rotation To: " << m_RotateMesh << "\n";
}

void Renderer::ToggleNormalMap()
{
	m_UseNormalMap = !m_UseNormalMap;
	std::cout << "Toggled Normal Map To: " << m_UseNormalMap << "\n";
}

void Renderer::CycleLightingMode()
{
	m_LightingMode = LightingMode(((int)m_LightingMode + 1) % (int)LightingMode::End);

	std::cout << "Toggled Lighting Mode To: " << (int)m_LightingMode << "\n";
}

// Private functions

void Renderer::RenderTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2) const
{
	// Frustum culling x & y
	if (v0.position.x < 0 || v1.position.x < 0 || v2.position.x < 0 ||
		v0.position.x > m_Width || v1.position.x > m_Width || v2.position.x > m_Width ||
		v0.position.y < 0 || v1.position.y < 0 || v2.position.y < 0 ||
		v0.position.y > m_Height || v1.position.y > m_Height || v2.position.y > m_Height)
	{
		return;
	}

	Vector2 edge0 = { v2.position.GetXY() - v1.position.GetXY() };
	Vector2 edge1 = { v0.position.GetXY() - v2.position.GetXY() };
	Vector2 edge2 = { v1.position.GetXY() - v0.position.GetXY() };

	float area = Vector2::Cross(edge0, edge1);

	if (area < 1.0f)
	{
		return;
	}

	auto top = std::max(std::max(v0.position.y, v1.position.y), v2.position.y);
	auto bottom = std::min(std::min(v0.position.y, v1.position.y), v2.position.y);
	auto left = std::min(std::min(v0.position.x, v1.position.x), v2.position.x);
	auto right = std::max(std::max(v0.position.x, v1.position.x), v2.position.x);

	for (int px{ static_cast<int>(left) }; px < static_cast<int>(right); ++px)
	{
		for (int py{ static_cast<int>(bottom) }; py < static_cast<int>(top); ++py)
		{
			Vector2 pixel{ static_cast<float>(px), static_cast<float>(py) };

			Vector2 p0ToPixel = pixel - v0.position.GetXY();
			auto w2 = Vector2::Cross(edge2, p0ToPixel) / area;

			if (w2 < 0.0f)
			{
				continue;
			}

			Vector2 p1ToPixel = pixel - v1.position.GetXY();
			auto w0 = Vector2::Cross(edge0, p1ToPixel) / area;

			if (w0 < 0.0f)
			{
				continue;
			}

			Vector2 p2ToPixel = pixel - v2.position.GetXY();
			auto w1 = Vector2::Cross(edge1, p2ToPixel) / area;

			if (w1 < 0.0f)
			{
				continue;
			}

			// Deoth Buffer
			float depthBuffer = 1.f / (w0 / v0.position.z + w1 / v1.position.z + w2 / v2.position.z);

			// frustum culling z + depth test
			if (depthBuffer < 0 || depthBuffer > 1 ||
				depthBuffer > m_pDepthBufferPixels[px + py * m_Width])
			{
				continue;
			}

			m_pDepthBufferPixels[px + py * m_Width] = depthBuffer;

			// actual depth
			w0 /= v0.position.w;
			w1 /= v1.position.w;
			w2 /= v2.position.w;

			auto depth = 1.0f / (w0 + w1 + w2);

			ColorRGB finalColor{};

			if (m_DepthBufferVisualization)
			{
				// Remap so it isnt too bright 
				depthBuffer = (depthBuffer - 0.985f) / (1.0f - 0.985f);

				depthBuffer = Clamp(depthBuffer, 0.f, 1.f);
				finalColor = { depthBuffer, depthBuffer, depthBuffer };
			}
			else
			{
				Vertex_Out shadingVertex{};
				shadingVertex.position.x = (float)px;
				shadingVertex.position.y = (float)py;
				shadingVertex.color = (w0 * v0.color + w1 * v1.color + w2 * v2.color) * depth;
				shadingVertex.uv = (w0 * v0.uv + w1 * v1.uv + w2 * v2.uv) * depth;
				shadingVertex.normal = ((w0 * v0.normal + w1 * v1.normal + w2 * v2.normal) * depth).Normalized();
				shadingVertex.tangent = ((w0 * v0.tangent + w1 * v1.tangent + w2 * v2.tangent) * depth).Normalized();

				finalColor = PixelShading(shadingVertex);
			}

			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::RenderMeshes(const std::vector<Mesh>& meshes) const
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

ColorRGB Renderer::PixelShading(const Vertex_Out& v) const
{
	Vector3 lightDirection = { .577f, -.577f, .577f };
	Vector3 normal{ v.normal };

	// Create viewDirection
	float x = (2 * (v.position.x + 0.5f / float(m_Width)) - 1) * m_Camera.aspectRatio * m_Camera.fov;
	float y = (1 - (2 * (v.position.y + 0.5f / float(m_Height)))) * m_Camera.fov;

	Vector3 viewDirection = (x * m_Camera.right + y * m_Camera.up + m_Camera.forward).Normalized();

	if (m_UseNormalMap)
	{
		// Create tangent space transformation matrix
		Vector3 binormal = Vector3::Cross(v.normal, v.tangent);
		Matrix tangentSpaceAxis{ v.tangent, binormal, v.normal, Vector3::Zero };

		// sample and remap color to [-1, 1]
		ColorRGB sampledColor = m_pNormal->Sample(v.uv);
		sampledColor = (2.f * sampledColor) - ColorRGB{ 1.f, 1.f, 1.f };

		normal = tangentSpaceAxis.TransformVector(sampledColor.r, sampledColor.g, sampledColor.b);
	}

	float dot = normal * -lightDirection;

	if (dot < 0.f)
	{
		return {};
	}

	ColorRGB finalColor{};
	auto lightIntensity = 7.0f;
	auto shine = 25.0f;

	float dot2 = -1;

	switch (m_LightingMode)
	{
		case Renderer::LightingMode::ObservedArea:
			finalColor = { dot, dot, dot };
			break;
		case Renderer::LightingMode::Diffuse:
			finalColor = m_pTexture->Sample(v.uv) * dot * lightIntensity / M_PI;
			break;
		case Renderer::LightingMode::Specular:
			finalColor = Phong(m_pSpecular->Sample(v.uv), shine * m_pGloss->Sample(v.uv).r, -lightDirection, viewDirection, normal) * dot;
			break;
		case Renderer::LightingMode::Combined:
		default:
			finalColor = m_pTexture->Sample(v.uv) * dot * lightIntensity / M_PI;
			finalColor += Phong(m_pSpecular->Sample(v.uv), shine * m_pGloss->Sample(v.uv).r, -lightDirection, viewDirection, normal) * dot;
			break;
	}

	finalColor.MaxToOne();
	return finalColor;
}

ColorRGB Renderer::Phong(ColorRGB specular, float gloss, Vector3 lightDir, Vector3 viewDir, Vector3 normal) const
{
	auto dot = (lightDir - (normal * (2.f * (normal * lightDir)))) * viewDir;

	if (dot < 0.f)
	{
		return {};
	}

	return specular * powf(dot, gloss);
}
