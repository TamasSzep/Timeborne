// Timeborne/Render/GameObjects/GameObjectRenderer.cpp

#include <Timeborne/Render/GameObjects/GameObjectRenderer.h>

#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>
#include <EngineBuildingBlocks/Graphics/Primitives/ModelLoader.h>
#include <EngineBuildingBlocks/Math/Intersection.h>

#include <Timeborne/MainApplication.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Math;
using namespace DirectXRender;
using namespace DirectX11Render;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<GameObjectRenderer::ModelConstData> GameObjectRenderer::GetAllModelConstData()
{
	std::vector<GameObjectRenderer::ModelConstData> result;
	
	auto addModel = [&result](GameObjectTypeIndex modelKey, const char* meshName) {
		assert((size_t)modelKey == result.size());
		result.push_back(GameObjectRenderer::ModelConstData{ modelKey, meshName });
	};
	
	addModel(GameObjectTypeIndex::TestInfantryUnit, "Test/TestInfantryUnit/TestInfantryUnit.fbx");
	addModel(GameObjectTypeIndex::TestCar, "Test/TestCar/TestCar.fbx" );
	
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr bool c_IsOptimizingMeshes = true;
constexpr bool c_IsUsingMaterialColors = false;
constexpr bool c_IsRenderingWireframe = false;

constexpr unsigned c_MaxCountObjects = 1024 * 64;
constexpr unsigned c_MaxCountMaterials = 1024;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The alpha value is a property of the object, not the material. This allows not only semi-transparent objects
// with a constant alpha, but also fade-out animations later.

struct ObjectCBType
{
	glm::mat4 ModelMatrix;
	glm::mat4 InverseModelMatrix;
	glm::mat4 ModelViewProjectionMatrix;
	unsigned ColorIndex;
	glm::vec3 _Padding0;
	float Alpha;
	glm::vec3 _Padding1;
};

struct MaterialCBType
{
	glm::vec3 DiffuseColor;
	float _Padding0;
	glm::vec3 SpecularColor;
	float _Padding1;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool GameObjectRenderer::Material::operator==(const Material& other) const
{
	NumericalEqualCompareBlock(PipelineStateIndex);
	NumericalEqualCompareBlock(ResourceStateIndex);
	NumericalEqualCompareBlock(MaterialCBIndex);
	return true;
}

bool GameObjectRenderer::Material::operator!=(const Material& other) const
{
	return !(*this == other);
}

bool GameObjectRenderer::Material::operator<(const Material& other) const
{
	NumericalLessCompareBlock(PipelineStateIndex);
	NumericalLessCompareBlock(ResourceStateIndex);
	NumericalLessCompareBlock(MaterialCBIndex);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GameObjectRenderer::RenderTaskGroup::Update(ConstantBuffer& objectCB, Camera& camera,
	const SceneNodeHandler& sceneNodeHandler, const std::map<GameObjectTypeIndex, ModelData>& models,
	const Core::SimpleTypeUnorderedVectorU<ObjectInternalData>& objectData,
	const Core::IndexVectorU& visibleObjectIndices)
{
	ProcessedRenderTasks.Clear();

	auto& viewProjectionMatrix = camera.GetViewProjectionMatrix();

	glm::mat4 worldMatrix;

	bool isOpaquePass = (Type == PassType::Opaque);
	bool isTransparentPass = (Type == PassType::Transparent);

	auto countVisibleObjects = visibleObjectIndices.GetSize();
	for (unsigned i = 0; i < countVisibleObjects; i++)
	{
		auto& cObjectData = objectData[visibleObjectIndices[i]];

		bool isObjectOpaque = (cObjectData.Alpha >= 1.0f);

		// Skipping transparent objects in the opaque render pass.
		// No meshes with transparent materials are added to the opaque render pass, it doesn't have to be checked.
		if (isOpaquePass && !isObjectOpaque)
		{
			continue;
		}

		auto modelKey = cObjectData.ModelKey;
		unsigned instanceIndex = cObjectData.InstanceIndex;
		auto& modelData = models.find(modelKey)->second;
		auto& renderedMeshes = RenderedMeshes[modelKey];

		bool transparentSkipPreconditions = (isTransparentPass && isObjectOpaque);

		auto countRenderedMeshes = renderedMeshes.GetSize();
		for (unsigned j = 0U; j < countRenderedMeshes; j++)
		{
			auto meshIndex = renderedMeshes[j];

			// Skipping opaque objects with opaque materials in the transparent render pass.
			bool materialIsOpaque = modelData.Meshes[meshIndex].MaterialIndex_Opaque != Core::c_InvalidIndexU;
			if (transparentSkipPreconditions && materialIsOpaque)
			{
				continue;
			}

			auto& instanceData = modelData.Instances[meshIndex][instanceIndex];

			unsigned sceneNodeIndex = instanceData.SceneNodeIndex;
			auto& objectCBData = objectCB.Access<ObjectCBType>(instanceData.ObjectCBIndex);

			worldMatrix = sceneNodeHandler.UnsafeGetScaledWorldTransformation(sceneNodeIndex).AsMatrix4();
			objectCBData.ModelMatrix = worldMatrix;
			objectCBData.InverseModelMatrix = sceneNodeHandler.UnsafeGetInverseScaledWorldTransformation(sceneNodeIndex).AsMatrix4();
			objectCBData.ModelViewProjectionMatrix = viewProjectionMatrix * worldMatrix;
			objectCBData.ColorIndex = cObjectData.ColorIndex;
			objectCBData.Alpha = cObjectData.Alpha;

			// Currently all models are rendered.
			InstanceRenderTask instanceRenderTask;
			instanceRenderTask.ModelKey = modelKey;
			instanceRenderTask.MeshIndex = meshIndex;
			instanceRenderTask.InstanceIndex = instanceIndex;
			ProcessedRenderTasks.PushBack(instanceRenderTask);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

GameObjectRenderer::GameObjectRenderer()
{
}

GameObjectRenderer::~GameObjectRenderer()
{
}

void GameObjectRenderer::InitializeRendering(const ComponentRenderContext& context,
	DirectX11Render::ConstantBuffer* lightingCB, DirectX11Render::ConstantBuffer* renderPassCB)
{
	m_ModelLoader = &context.Application->GetModelLoader();

	m_LightingCB = lightingCB;
	m_RenderPassCB = renderPassCB;

	m_ObjectCB.Initialize(context.Device, sizeof(ObjectCBType), c_MaxCountObjects);
	m_MaterialCB.Initialize(context.Device, sizeof(MaterialCBType), c_MaxCountMaterials);

	m_RenderTaskGroups.resize(2);
	for (size_t i = 0; i < m_RenderTaskGroups.size(); i++)
	{
		m_RenderTaskGroups[i].Type = (RenderTaskGroup::PassType)i;
	}

	// Loading models.
	for (auto& modelData : GetAllModelConstData())
	{
		LoadModel(context, modelData);
	}
}

void GameObjectRenderer::DestroyRendering()
{
}

void GameObjectRenderer::UpdateTransformations()
{
	m_SceneNodeHandler.UpdateTransformations();
}

void GameObjectRenderer::PreUpdate(const ComponentPreUpdateContext& context,
	EngineBuildingBlocks::Graphics::Camera& camera,
	const Core::IndexVectorU& visibleObjectIndices)
{
	m_ViewFrustumCuller.ViewFrustumCull(camera, *context.ThreadPool, m_SceneNodeHandler, m_ObjectData.GetArray(),
		visibleObjectIndices.GetArray(), m_OutputIndicesForVFC, visibleObjectIndices.GetSize());

	for (auto& renderTaskGroup : m_RenderTaskGroups)
	{
		renderTaskGroup.Update(m_ObjectCB, camera, m_SceneNodeHandler, m_Models, m_ObjectData, m_OutputIndicesForVFC);
	}
}

void GameObjectRenderer::RenderContent(const ComponentRenderContext& context)
{
	auto d3dContext = context.DeviceContext;

	unsigned prevMaterialIndex = Core::c_InvalidIndexU;
	unsigned prevPSIndex = Core::c_InvalidIndexU;
	unsigned prevRSIndex = Core::c_InvalidIndexU;
	unsigned prevMCBIndex = Core::c_InvalidIndexU;

	for (size_t i = 0; i < m_RenderTaskGroups.size(); i++)
	{
		auto& renderTaskGroup = m_RenderTaskGroups[i];
		auto& processedRenderTasks = renderTaskGroup.ProcessedRenderTasks;
		bool isOpaquePass = (renderTaskGroup.Type == RenderTaskGroup::PassType::Opaque);

		unsigned countRenderTasks = processedRenderTasks.GetSize();
		for (unsigned j = 0; j < countRenderTasks; j++)
		{
			auto& renderTask = processedRenderTasks[j];
			auto& modelData = m_Models[renderTask.ModelKey];
			auto& meshData = modelData.Meshes[renderTask.MeshIndex];
			auto& instanceData = modelData.Instances[renderTask.MeshIndex][renderTask.InstanceIndex];

			auto materialIndex = isOpaquePass ? meshData.MaterialIndex_Opaque : meshData.MaterialIndex_Transparent;

			// This is material level, but we don't sort by material now.
			if (materialIndex != prevMaterialIndex)
			{
				auto& material = m_Materials[materialIndex];
				auto psIndex = material.PipelineStateIndex;
				auto rsIndex = material.ResourceStateIndex;
				auto mcbIndex = material.MaterialCBIndex;

				if (prevPSIndex != psIndex)
				{
					context.DX11M->PipelineStateManager.GetPipelineState(psIndex).SetForContext(d3dContext);
					prevPSIndex = psIndex;
				}
				if (prevRSIndex != rsIndex)
				{
					context.DX11M->ResourceStateManager.GetResourceState(rsIndex).SetForContext(d3dContext);
					prevRSIndex = rsIndex;
				}
				if (prevMCBIndex != mcbIndex)
				{
					m_MaterialCB.Update(d3dContext, mcbIndex);
					prevMCBIndex = mcbIndex;
				}

				prevMaterialIndex = materialIndex;
			}

			m_ObjectCB.Update(d3dContext, instanceData.ObjectCBIndex);

			DrawPrimitive(meshData.Primitive, d3dContext);
		}
	}
}

void GameObjectRenderer::RenderGUI(const ComponentRenderContext& context)
{
}

unsigned GameObjectRenderer::AddObject(const ObjectData& obj)
{
	auto modelKey = obj.TypeIndex;
	auto& modelData = m_Models[modelKey];

	unsigned instanceIndex;
	unsigned rootSceneNodeIndex;
	if (!modelData.UnusedInstances.IsEmpty())
	{
		instanceIndex = modelData.UnusedInstances.PopBackReturn();
		rootSceneNodeIndex = modelData.InstanceRootSceneNodes[instanceIndex];
	}
	else
	{
		auto modelInstantiationResult = m_ModelLoader->Instatiate(modelData.ModelIndex, m_SceneNodeHandler);
		rootSceneNodeIndex = modelInstantiationResult.GlobalRootSceneNodeIndex;
		
		instanceIndex = modelData.InstanceRootSceneNodes.GetSize();
		modelData.InstanceRootSceneNodes.PushBack(rootSceneNodeIndex);

		auto countMeshes = (unsigned)modelData.Instances.size();
		for (unsigned meshIndex = 0U; meshIndex < countMeshes; meshIndex++)
		{
			auto localSceneNodeIndex = modelData.Meshes[meshIndex].LocalSceneNodeIndex;
			auto sceneNodeIndex = modelInstantiationResult.GlobalSceneNodeMapping[localSceneNodeIndex];
			auto objectCBIndex = m_CountObjectCBData++;
			modelData.Instances[meshIndex].PushBack({ sceneNodeIndex, objectCBIndex });

			assert(objectCBIndex < c_MaxCountObjects);
		}
	}

	auto scalerWorld = modelData.ModelScaler * obj.Size;

	m_SceneNodeHandler.SetLocalScaler(rootSceneNodeIndex, scalerWorld);
	m_SceneNodeHandler.SetPosition(rootSceneNodeIndex, obj.Position);
	m_SceneNodeHandler.SetDirectionUp(rootSceneNodeIndex, obj.Direction, obj.Up);

	unsigned objectIndex = m_ObjectData.Add();
	auto& objectData = m_ObjectData[objectIndex];
	objectData.ModelKey = modelKey;
	objectData.InstanceIndex = instanceIndex;
	objectData.SceneNodeIndex = rootSceneNodeIndex;
	objectData.BoundingBox = GetModelAABB(modelKey);
	objectData.ColorIndex = obj.ColorIndex;
	objectData.Alpha = obj.Alpha;

	return objectIndex;
}

void GameObjectRenderer::RemoveObject(unsigned objectIndex)
{
	auto& objectData = m_ObjectData[objectIndex];
	auto& modelData = m_Models[objectData.ModelKey];
	modelData.UnusedInstances.PushBack(objectData.InstanceIndex);
	m_ObjectData.Remove(objectIndex);
}

void GameObjectRenderer::ClearObjects()
{
	m_SceneNodeHandler.Clear();

	for (auto& model : m_Models)
	{
		auto& modelData = model.second;

		modelData.UnusedInstances.Clear();
		modelData.InstanceRootSceneNodes.Clear();

		for (auto& meshInstances : modelData.Instances)
		{
			meshInstances.Clear();
		}
	}

	m_ObjectData.Clear();

	m_CountObjectCBData = 0;
}

EngineBuildingBlocks::Math::AABoundingBox GameObjectRenderer::GetTransformedBox(unsigned objectIndex)
{
	auto& objectData = m_ObjectData[objectIndex];
	unsigned rootSceneNodeIndex = objectData.SceneNodeIndex;
	auto& box = objectData.BoundingBox;
	return box.Transform(m_SceneNodeHandler.GetScaledWorldTransformation(rootSceneNodeIndex).AsMatrix4x3());
}

void GameObjectRenderer::SetObjectPose(unsigned objectIndex, const glm::vec3& position,
	const glm::vec3& direction, const glm::vec3& up)
{
	auto rootSceneNodeIndex = m_ObjectData[objectIndex].SceneNodeIndex;
	m_SceneNodeHandler.SetPosition(rootSceneNodeIndex, position);
	m_SceneNodeHandler.SetDirectionUp(rootSceneNodeIndex, direction, up);
}

void GameObjectRenderer::SetObjectAlpha(unsigned objectIndex, float alpha)
{
	m_ObjectData[objectIndex].Alpha = alpha;
}

const Core::IndexVectorU& GameObjectRenderer::GetVisibleObjectIndices() const
{
	return m_OutputIndicesForVFC;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GameObjectRenderer::LoadModel(const ComponentRenderContext& context, const ModelConstData& modelConstData)
{
	auto& device = context.Device;

	Vertex_SOA_Data vertexData;
	IndexData indexData;
	ModelLoadingDescription modelDesc;
	modelDesc.BuildingDescription.FilePath = modelConstData.Name;
	modelDesc.BuildingDescription.GeometryOptions.IsOptimizingMeshes = c_IsOptimizingMeshes;
	modelDesc.BuildingDescription.GeometryOptions.IsOptimizingGraph = false;
	auto modelLoadingResult = m_ModelLoader->Load(modelDesc, vertexData, indexData);
	auto& model = m_ModelLoader->GetModel(modelLoadingResult.ModelIndex);

	auto vertexBuffer = m_VertexBuffers.Request();
	auto indexBuffer = m_IndexBuffers.Request();

	vertexBuffer->Initialize(device, vertexData);
	indexBuffer->Initialize(device, indexData);

	ModelData modelData;
	modelData.ConstData = modelConstData;
	modelData.ModelIndex = modelLoadingResult.ModelIndex;
	modelData.ModelScaler = glm::vec3(1.0) / model.NonAnimatedBox.GetSize();

	auto countObjects = modelLoadingResult.Meshes.GetSize();
	for (unsigned i = 0; i < countObjects; i++)
	{
		AddMesh(context, modelData, i, modelLoadingResult, vertexData, vertexBuffer, indexBuffer);
	}

	m_Models[modelConstData.Key] = modelData;
}

void GameObjectRenderer::AddMesh(const ComponentRenderContext& context, ModelData& modelData, unsigned meshIndex,
	const ModelLoadingResult& loadRes, const Vertex_SOA_Data& vertexData, DirectX11Render::VertexBuffer* vertexBuffer,
	DirectX11Render::IndexBuffer* indexBuffer)
{
	auto& model = m_ModelLoader->GetModel(loadRes.ModelIndex);
	auto& objectData = model.Objects[meshIndex];

	// Creating a scene node for the render task.
	auto& meshData = modelData.Meshes.PushBackPlaceHolder();

	meshData.LocalSceneNodeIndex = objectData.SceneNodeIndex;

	auto& geometryData = loadRes.Meshes[objectData.MeshIndex];
	auto& targetPrimitive = meshData.Primitive;
	targetPrimitive.PVertexBuffer = vertexBuffer;
	targetPrimitive.PIndexBuffer = indexBuffer;
	targetPrimitive.BaseVertex = geometryData.BaseVertex;
	targetPrimitive.BaseIndex = geometryData.BaseIndex;
	targetPrimitive.CountVertices = geometryData.CountVertices;
	targetPrimitive.CountIndices = geometryData.CountIndices;

	modelData.Instances.emplace_back();

	bool isMaterialOpaque = IsMaterialOpaque(loadRes.ModelIndex, meshIndex);

	auto createMeshDataForRenderPass = [this, &meshData, &modelData, &context, &loadRes, meshIndex](
		RenderTaskGroup::PassType passType)
	{	
		bool isPassOpaque = (passType == RenderTaskGroup::PassType::Opaque);
		auto& targetMaterialIndex = (isPassOpaque
			? meshData.MaterialIndex_Opaque
			: meshData.MaterialIndex_Transparent);
		targetMaterialIndex = GetMaterialIndex(context, loadRes.ModelIndex, meshIndex, isPassOpaque,
			modelData.ConstData.OpacityChannelIndex);
		
		m_RenderTaskGroups[(unsigned)passType].RenderedMeshes[modelData.ConstData.Key].PushBack(meshIndex);
	};

	// Every mesh must be transparent renderable. Opaque meshes must be additionnaly opaque renderable.
	createMeshDataForRenderPass(RenderTaskGroup::PassType::Transparent);
	if (isMaterialOpaque)
	{
		createMeshDataForRenderPass(RenderTaskGroup::PassType::Opaque);
	}
	else
	{
		meshData.MaterialIndex_Opaque = Core::c_InvalidIndexU;
	}
}

const EngineBuildingBlocks::Math::AABoundingBox& GameObjectRenderer::GetModelAABB(GameObjectTypeIndex modelKey) const
{
	return m_ModelLoader->GetModel(m_Models.find(modelKey)->second.ModelIndex).NonAnimatedBox;
}

bool GameObjectRenderer::IsMaterialOpaque(unsigned modelIndex, unsigned meshIndex) const
{
	auto& model = m_ModelLoader->GetModel(modelIndex);
	unsigned meshMaterialIndex = model.Objects[meshIndex].MaterialIndex;
	auto& meshMaterial = model.Materials[meshMaterialIndex];
	return (meshMaterial.Opacity == 1.0f);
}

unsigned GameObjectRenderer::GetMaterialIndex(const ComponentRenderContext& context,
	unsigned modelIndex, unsigned meshIndex, bool isPassOpaque, unsigned opacityChannelIndex)
{
	auto& device = context.Device;

	auto& model = m_ModelLoader->GetModel(modelIndex);
	unsigned meshMaterialIndex = model.Objects[meshIndex].MaterialIndex;
	auto& meshMaterial = model.Materials[meshMaterialIndex];

	unsigned materialCBIndex = Core::c_InvalidIndexU;
	{
		MaterialCBType cbData;
		cbData.DiffuseColor = meshMaterial.DiffuseColor;
		cbData.SpecularColor = meshMaterial.SpecularColor;

		// Note: this is slow for many materials.
		for (unsigned i = 0; i < m_CountMaterialCBData; i++)
		{
			auto& otherCBData = m_MaterialCB.Access<MaterialCBType>(i);
			if (cbData.DiffuseColor == otherCBData.DiffuseColor && cbData.SpecularColor == otherCBData.SpecularColor)
			{
				materialCBIndex = i;
				break;
			}
		}
		if (materialCBIndex == Core::c_InvalidIndexU)
		{
			materialCBIndex = m_CountMaterialCBData++;
			memcpy(&m_MaterialCB.Access<MaterialCBType>(materialCBIndex), &cbData, sizeof(MaterialCBType));
		}
	}

	Material material;
	material.PipelineStateIndex = GetPipelineStateIndex(context, modelIndex, isPassOpaque, meshMaterial);
	material.ResourceStateIndex = GetResourceStateIndex(context, meshMaterial, opacityChannelIndex);
	material.MaterialCBIndex = materialCBIndex;

	unsigned materialIndex;
	if (!m_Materials.IsContaining(material, materialIndex))
	{
		materialIndex = m_Materials.Add(material);
	}

	return materialIndex;
}

unsigned GameObjectRenderer::GetPipelineStateIndex(const ComponentRenderContext& context,
	unsigned modelIndex, bool isPassOpaque, const MaterialData& materialData)
{
	auto& device = context.Device;

	auto inputLayout = DirectX11Render::VertexInputLayout::Create(m_ModelLoader->GetModel(modelIndex).Vertices.InputLayout);

	std::vector<ShaderDefine> shaderDefines = {
		{ "IS_USING_MATERIAL_COLORS", c_IsUsingMaterialColors },
		{ "IS_USING_VERTEX_COLOR", inputLayout.InputLayout.HasVertexColors() }
	};
	auto vs = context.DX11M->ShaderManager.GetShader(device, { "GameObjects/GameObject.hlsl",
		"VSMain", ShaderType::Vertex, "5_0", 0, shaderDefines });
	auto ps = context.DX11M->ShaderManager.GetShader(device, { "GameObjects/GameObject.hlsl",
		"PSMain", ShaderType::Pixel, "5_0", 0, shaderDefines });

	SamplerStateDescription diffuseTextureSampler;
	diffuseTextureSampler.SetToWrap();
	diffuseTextureSampler.SetToAnisotropic();

	PipelineStateDescription description;
	description.Shaders = { vs, ps };
	description.InputLayout = std::move(inputLayout);
	description.SamplerStates = { { diffuseTextureSampler, ShaderFlagType::Pixel } };

	if (isPassOpaque)
	{
		// Opaque material.
		if (!materialData.OpacityTextureName.empty())
		{
			description.RasterizerState.DisableCulling();
			description.BlendState.EnableAlphaToCoverage();
		}
	}
	else
	{
		// Transparent material.
		description.BlendState.SetToNonpremultipliedAlphaBlending();
	}

	description.RasterizerState.EnableMultisampling();
	if (c_IsRenderingWireframe) description.RasterizerState.SetWireframeFillMode();

	return context.DX11M->PipelineStateManager.GetPipelineStateIndex(device, description);
}

unsigned GameObjectRenderer::GetResourceStateIndex(const ComponentRenderContext& context,
	const MaterialData& materialData, unsigned opacityChannelIndex)
{
	ResourceState resourceState(ShaderFlagType::Vertex | ShaderFlagType::Pixel, 4, 1);
	resourceState.AddConstantBuffer(m_RenderPassCB->GetBuffer(), 0, ShaderFlagType::Pixel);
	resourceState.AddConstantBuffer(m_ObjectCB.GetBuffer(), 1, ShaderFlagType::Vertex);
	resourceState.AddConstantBuffer(m_ObjectCB.GetBuffer(), 1, ShaderFlagType::Pixel);
	resourceState.AddConstantBuffer(m_MaterialCB.GetBuffer(), 2, ShaderFlagType::Pixel);
	resourceState.AddConstantBuffer(m_LightingCB->GetBuffer(), 3, ShaderFlagType::Pixel);

	if (materialData.OpacityTextureName.empty())
	{
		resourceState.AddShaderResourceView(
			GetTexture(context, materialData.DiffuseTextureName)->GetSRV(),
			0, ShaderFlagType::Pixel);
	}
	else
	{
		resourceState.AddShaderResourceView(
			GetTexture(context, materialData.DiffuseTextureName, materialData.OpacityTextureName, opacityChannelIndex)->GetSRV(),
			0, ShaderFlagType::Pixel);
	}

	return context.DX11M->ResourceStateManager.AddResourceState(resourceState);
}

Texture2D* GameObjectRenderer::GetTexture(const ComponentRenderContext& context, const std::string& textureName)
{
	return context.DX11M->ConstantTextureManager.GetTexture2DFromFile(context.Device, textureName).Texture;
}

Texture2D* GameObjectRenderer::GetTexture(const ComponentRenderContext& context, const std::string& diffuseTextureName,
	const std::string& opacityTextureName, unsigned opacityChannelIndex)
{
	return context.DX11M->ConstantTextureManager.GetDiffuseTexture2DWithOpacity(context.Device,
		diffuseTextureName, opacityTextureName, opacityChannelIndex).Texture;
}
