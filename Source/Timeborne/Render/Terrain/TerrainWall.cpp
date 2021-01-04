// Timeborne/Render/Terrain/TerrainWall.cpp

#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/Render/Terrain/TerrainWall.h>

#include <DirectX11Render/Resources/PipelineState.h>
#include <DirectX11Render/Managers.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace DirectXRender;
using namespace DirectX11Render;

inline EngineBuildingBlocks::Graphics::VertexInputLayout GetTerrainWall_IL()
{
	EngineBuildingBlocks::Graphics::VertexInputLayout instanceBufferIL;
	instanceBufferIL.Elements =
	{
		{ "FieldIndices", VertexElementType::Uint32, sizeof(unsigned), 2 },
		{ "IsXWall", VertexElementType::Uint32, sizeof(unsigned), 1 }
	};
	return instanceBufferIL;
}

inline unsigned CreateTerrainWallPS(const ComponentRenderContext& context, bool isInGame, bool wireframe, bool showGrid)
{
	std::vector<DirectXRender::ShaderDefine> defines;
	defines.push_back({ "IS_SHOWING_GRID", showGrid ? "1" : "0" });
	defines.push_back({ "IS_INGAME", isInGame ? "1" : "0" });
	auto vs = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "TerrainWall.hlsl", "VSMain", ShaderType::Vertex, defines });
	auto hs = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "TerrainWall.hlsl", "HSMain", ShaderType::Hull, defines });
	auto ds = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "TerrainWall.hlsl", "DSMain", ShaderType::Domain, defines });
	auto ps = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "TerrainWall.hlsl", "PSMain", ShaderType::Pixel, defines });

	PipelineStateDescription description;
	description.Shaders = { vs, hs, ds, ps };
	description.InputLayout = DirectX11Render::VertexInputLayout::Create(GetTerrainWall_IL());
	description.SamplerStates = { { SamplerStateDescription(), DirectXRender::ShaderFlagType::Hull } };
	description.RasterizerState.EnableMultisampling();
	if (wireframe) description.RasterizerState.SetWireframeFillMode();

	return context.DX11M->PipelineStateManager.GetPipelineStateIndex(context.Device, description);
}

unsigned& TerrainWall::GetPSIndex(bool isInGame, bool wireframe, bool showGrid)
{
	return m_PSIndices[(int)isInGame * 4 + (int)wireframe * 2 + (int)showGrid];
}

void TerrainWall::InitializeRendering(const ComponentRenderContext& context)
{
	for (int i = 0; i < 8; i++)
	{
		bool isInGame = (bool)(i & 4);
		bool wireframe = (bool)(i & 2);
		bool showGrid = (bool)(i & 1);
		GetPSIndex(isInGame, wireframe, showGrid) = CreateTerrainWallPS(context, isInGame, wireframe, showGrid);
	}
}

void TerrainWall::CreateGROnSizeChange(const ComponentRenderContext& context, const Level* level)
{
	auto& terrain = level->GetTerrain();
	auto countFields = terrain.GetCountFields();

	assert(countFields.x % 16 == 0 && countFields.y % 16 == 0);
	m_ChunkSize = 16;
	auto countChunksX = countFields.x / m_ChunkSize;
	auto countChunksZ = countFields.y / m_ChunkSize;
	m_XWallVBData.resize(countChunksX * countChunksZ);
	m_ZWallVBData.resize(countChunksX * countChunksZ);

	unsigned maxCountWalls = (countFields.x + 1) * countFields.y + (countFields.y + 1) * countFields.x;
	unsigned maxCountVertices = maxCountWalls * 6;
	m_TerrainWallVB = VertexBuffer();
	m_TerrainWallVB.Initialize(context.Device, D3D11_USAGE_DYNAMIC, GetTerrainWall_IL(), maxCountVertices);
}

void TerrainWall::UpdateHeights(const ComponentRenderContext& context, const Level* level, const glm::ivec2& changeStart, const glm::ivec2& changeEnd)
{
	auto& terrain = level->GetTerrain();
	auto countFields = terrain.GetCountFields();
	auto fields = terrain.GetFields();

	FieldData zeroField{};

	auto countChunksX = countFields.x / m_ChunkSize;
	auto countChunksZ = countFields.y / m_ChunkSize;

	glm::ivec2 xWallStart(changeStart.x / m_ChunkSize, glm::clamp((changeStart.y - 1) / (int)m_ChunkSize, 0, (int)countChunksZ - 1));
	glm::ivec2 xWallEnd(changeEnd.x / m_ChunkSize, std::min(changeEnd.y / m_ChunkSize, countChunksZ - 1));

	for (int zChunk = xWallStart.y; zChunk <= xWallEnd.y; zChunk++)
	{
		int zStart = zChunk * m_ChunkSize - (zChunk == 0 ? 1 : 0);
		int zEnd = (zChunk + 1) * m_ChunkSize - 1;

		for (int xChunk = xWallStart.x; xChunk <= xWallEnd.x; xChunk++)
		{
			auto& chunk = m_XWallVBData[zChunk * countChunksZ + xChunk];
			chunk.Clear();

			int xStart = xChunk * m_ChunkSize;
			int xEnd = (xChunk + 1) * m_ChunkSize - 1;

			for (int z = zStart; z <= zEnd; z++)
			{
				for (int x = xStart; x <= xEnd; x++)
				{
					int index0 = (z >= 0 ? z * countFields.x + x : Core::c_InvalidIndexU);
					int index1 = (z < (int)countFields.y - 1 ? (z + 1) * countFields.x + x : Core::c_InvalidIndexU);
					auto& field0 = (index0 == Core::c_InvalidIndexU ? zeroField : fields[index0]);
					auto& field1 = (index1 == Core::c_InvalidIndexU ? zeroField : fields[index1]);

					if (field0.Heights[0] != field1.Heights[3] || field0.Heights[1] != field1.Heights[2])
					{
						WallVBType wall;
						wall.FieldIndices.x = index0;
						wall.FieldIndices.y = index1;
						wall.IsXWall = 1;
						chunk.PushBack(wall);
					}
				}
			}
		}
	}

	glm::ivec2 zWallStart(glm::clamp((changeStart.x - 1) / (int)m_ChunkSize, 0, (int)countChunksX - 1), changeStart.y / m_ChunkSize);
	glm::ivec2 zWallEnd(std::min(changeEnd.x / m_ChunkSize, countChunksX - 1), changeEnd.y / m_ChunkSize);

	for (int zChunk = zWallStart.y; zChunk <= zWallEnd.y; zChunk++)
	{
		int zStart = zChunk * m_ChunkSize;
		int zEnd = (zChunk + 1) * m_ChunkSize - 1;

		for (int xChunk = zWallStart.x; xChunk <= zWallEnd.x; xChunk++)
		{
			auto& chunk = m_ZWallVBData[zChunk * countChunksZ + xChunk];
			chunk.Clear();

			int xStart = xChunk * m_ChunkSize - (xChunk == 0 ? 1 : 0);
			int xEnd = (xChunk + 1) * m_ChunkSize - 1;

			for (int z = zStart; z <= zEnd; z++)
			{
				for (int x = xStart; x <= xEnd; x++)
				{
					int index0 = (x >= 0 ? z * countFields.x + x : Core::c_InvalidIndexU);
					int index1 = (x < (int)countFields.x - 1 ? z * countFields.x + x + 1 : Core::c_InvalidIndexU);
					auto& field0 = (index0 == Core::c_InvalidIndexU ? zeroField : fields[index0]);
					auto& field1 = (index1 == Core::c_InvalidIndexU ? zeroField : fields[index1]);

					if (field0.Heights[1] != field1.Heights[0] || field0.Heights[2] != field1.Heights[3])
					{
						WallVBType wall;
						wall.FieldIndices.x = index0;
						wall.FieldIndices.y = index1;
						wall.IsXWall = 0;
						chunk.PushBack(wall);
					}
				}
			}
		}
	}

	m_WallVBData.Clear();
	m_CountWalls = 0;
	for (auto& chunk : m_XWallVBData)
	{
		m_WallVBData.PushBack(chunk.GetArray(), chunk.GetSize());
		m_CountWalls += chunk.GetSize();
	}
	for (auto& chunk : m_ZWallVBData)
	{
		m_WallVBData.PushBack(chunk.GetArray(), chunk.GetSize());
		m_CountWalls += chunk.GetSize();
	}

	m_TerrainWallVB.SetData(context.DeviceContext, m_WallVBData.GetArray(), m_WallVBData.GetSizeInBytes());
}

void TerrainWall::Update(const ComponentRenderContext& context, const Level* level,
	bool isSizeChanged, const glm::ivec2& changeStart, const glm::ivec2& changeEnd)
{
	if (isSizeChanged)
	{
		CreateGROnSizeChange(context, level);
	}
	if (changeStart.x <= changeEnd.x && changeStart.y <= changeEnd.y)
	{
		UpdateHeights(context, level, changeStart, changeEnd);
	}
}

void TerrainWall::Render(const ComponentRenderContext& context, const Level* level, bool isInGame, bool wireframe,
	bool showGrid)
{
	auto& terrain = level->GetTerrain();
	auto countFields = terrain.GetCountFields();
	if (countFields.x == 0 || countFields.y == 0) return;

	if (m_CountWalls > 0)
	{
		context.DX11M->PipelineStateManager.GetPipelineState(GetPSIndex(isInGame, wireframe, showGrid)).SetForContext(context.DeviceContext);

		ID3D11Buffer* vbs[] = { m_TerrainWallVB.GetBuffer() };
		unsigned strides[] = { m_TerrainWallVB.GetVertexStride() };
		unsigned offsets[] = { 0U };
		context.DeviceContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);

		context.DeviceContext->Draw(m_CountWalls, 0);
	}
}