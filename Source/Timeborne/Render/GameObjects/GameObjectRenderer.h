// Timeborne/Render/GameObjects/GameObjectRenderer.h

#pragma once

#include <Timeborne/Declarations/DirectX11RenderDeclarations.h>
#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectTypeIndex.h>
#include <Timeborne/ApplicationComponent.h>

#include <Core/DataStructures/SimpleTypeUnorderedVector.hpp>
#include <EngineBuildingBlocks/Graphics/ViewFrustumCuller.h>
#include <EngineBuildingBlocks/SceneNode.h>
#include <DirectX11Render/Resources/ConstantBuffer.h>
#include <DirectX11Render/Primitive.h>

#include <vector>

class GameObjectRenderer
{
public:

	struct ObjectData
	{
		GameObjectTypeIndex TypeIndex;

		glm::vec3 Size;
		glm::vec3 Position;
		glm::vec3 Direction;
		glm::vec3 Up;

		unsigned ColorIndex;

		float Alpha;
	};

private:

	EngineBuildingBlocks::SceneNodeHandler m_SceneNodeHandler;

	EngineBuildingBlocks::Graphics::ModelLoader* m_ModelLoader = nullptr;
	DirectX11Render::ConstantBuffer* m_LightingCB = nullptr;
	DirectX11Render::ConstantBuffer* m_RenderPassCB = nullptr;
	
	struct Material
	{
		unsigned PipelineStateIndex;
		unsigned ResourceStateIndex;

		unsigned MaterialCBIndex;

		bool operator==(const Material& other) const;
		bool operator!=(const Material& other) const;
		bool operator<(const Material& other) const;
	};

	Core::SimpleTypeSetU<Material> m_Materials;

	struct ModelConstData
	{
		GameObjectTypeIndex Key;
		std::string Name;

		unsigned OpacityChannelIndex;
	};

	struct MeshData
	{
		unsigned LocalSceneNodeIndex;
		unsigned MaterialIndex_Opaque;
		unsigned MaterialIndex_Transparent;
		DirectX11Render::IndexedPrimitive Primitive;
	};

	struct InstanceData
	{
		unsigned SceneNodeIndex;

		// @todo: this must be done more linearly.
		// Optimize after making the objects movable.
		unsigned ObjectCBIndex;
	};

	struct ModelData
	{
		ModelConstData ConstData;

		unsigned ModelIndex;

		Core::SimpleTypeVector<MeshData> Meshes;

		glm::vec3 ModelScaler;

		Core::IndexVectorU UnusedInstances;
		Core::IndexVectorU InstanceRootSceneNodes;

		// Indices: mesh, instance
		std::vector<Core::SimpleTypeVectorU<InstanceData>> Instances;
	};

	std::map<GameObjectTypeIndex, ModelData> m_Models;

	const EngineBuildingBlocks::Math::AABoundingBox& GetModelAABB(
		GameObjectTypeIndex modelKey) const;

private: // Object internal data.

	struct ObjectInternalData
	{
		GameObjectTypeIndex ModelKey;
		unsigned InstanceIndex;
		unsigned SceneNodeIndex; // The root scene node's index.
		EngineBuildingBlocks::Math::AABoundingBox BoundingBox;
		unsigned ColorIndex;
		float Alpha;
	};
	Core::SimpleTypeUnorderedVectorU<ObjectInternalData> m_ObjectData;

private: // Render task data.

	struct InstanceRenderTask
	{
		GameObjectTypeIndex ModelKey;
		unsigned MeshIndex;
		unsigned InstanceIndex;
	};

	struct RenderTaskGroup
	{
		enum class PassType
		{
			// The values also indicate the order of execution.
			Opaque, Transparent
		};

		std::map<GameObjectTypeIndex, Core::IndexVectorU> RenderedMeshes;
		Core::SimpleTypeVectorU<InstanceRenderTask> ProcessedRenderTasks;
		PassType Type;

		void Update(DirectX11Render::ConstantBuffer& objectCB,
			EngineBuildingBlocks::Graphics::Camera& camera,
			const EngineBuildingBlocks::SceneNodeHandler& sceneNodeHandler,
			const std::map<GameObjectTypeIndex, ModelData>& models,
			const Core::SimpleTypeUnorderedVectorU<ObjectInternalData>& objectData,
			const Core::IndexVectorU& visibleObjectIndices);
	};

	std::vector<RenderTaskGroup> m_RenderTaskGroups;

	static std::vector<GameObjectRenderer::ModelConstData> GetAllModelConstData();

private: // Graphics resources.

	Core::ResourcePoolU<DirectX11Render::VertexBuffer> m_VertexBuffers;
	Core::ResourcePoolU<DirectX11Render::IndexBuffer> m_IndexBuffers;
	DirectX11Render::ConstantBuffer m_MaterialCB;
	DirectX11Render::ConstantBuffer m_ObjectCB;

	unsigned m_CountMaterialCBData = 0U;
	unsigned m_CountObjectCBData = 0U;

	void LoadModel(const ComponentRenderContext& context,
		const ModelConstData& modelData);
	void AddMesh(const ComponentRenderContext& context,
		ModelData& modelData, unsigned meshIndex,
		const EngineBuildingBlocks::Graphics::ModelLoadingResult& loadRes,
		const EngineBuildingBlocks::Graphics::Vertex_SOA_Data& vertexData,
		DirectX11Render::VertexBuffer* vertexBuffer,
		DirectX11Render::IndexBuffer* indexBuffer);
	bool IsMaterialOpaque(unsigned modelIndex, unsigned meshIndex) const;
	unsigned GetMaterialIndex(const ComponentRenderContext& context,
		unsigned modelIndex, unsigned meshIndex, bool isPassOpaque,
		unsigned opacityChannelIndex);
	unsigned GetPipelineStateIndex(const ComponentRenderContext& context,
		unsigned modelIndex, bool isPassOpaque,
		const EngineBuildingBlocks::Graphics::MaterialData& materialData);
	unsigned GetResourceStateIndex(const ComponentRenderContext& context,
		const EngineBuildingBlocks::Graphics::MaterialData& materialData,
		unsigned opacityChannelIndex);
	DirectX11Render::Texture2D* GetTexture(const ComponentRenderContext& context, const std::string& textureName);
	DirectX11Render::Texture2D* GetTexture(const ComponentRenderContext& context, const std::string& diffuseTextureName,
		const std::string& opacityTextureName, unsigned opacityChannelIndex);

private: // View frustum culling.

	EngineBuildingBlocks::Graphics::ViewFrustumCuller m_ViewFrustumCuller;
	Core::IndexVectorU m_OutputIndicesForVFC;

public:

	GameObjectRenderer();
	~GameObjectRenderer();

	unsigned AddObject(const ObjectData& obj);
	void RemoveObject(unsigned objectIndex);
	void ClearObjects();
	
	EngineBuildingBlocks::Math::AABoundingBox GetTransformedBox(unsigned objectIndex);
	const Core::IndexVectorU& GetVisibleObjectIndices() const;
	
	void SetObjectPose(unsigned objectIndex, const glm::vec3& position,
		const glm::vec3& direction, const glm::vec3& up);
	void SetObjectAlpha(unsigned objectIndex, float alpha);

	void InitializeRendering(const ComponentRenderContext& context,
		DirectX11Render::ConstantBuffer* lightingCB,
		DirectX11Render::ConstantBuffer* renderPassCB);
	void DestroyRendering();

	void UpdateTransformations();
	void PreUpdate(const ComponentPreUpdateContext& context,
		EngineBuildingBlocks::Graphics::Camera& camera,
		const Core::IndexVectorU& visibleObjectIndices);

	void RenderContent(const ComponentRenderContext& context);
	void RenderGUI(const ComponentRenderContext& context);
};