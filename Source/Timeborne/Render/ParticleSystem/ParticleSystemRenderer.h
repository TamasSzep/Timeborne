// Timeborne/Render/ParticleSystem/ParticleSystemRenderer.h

#pragma once

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/DataStructures/ResourceUnorderedVector.hpp>
#include <Core/Constants.h>
#include <EngineBuildingBlocks/Math/GLM.h>
#include <DirectX11Render/Primitive.h>
#include <DirectX11Render/Resources/VertexBuffer.h>

#include <Timeborne/Render/ParticleSystem/ParticleSystem.h>

#include <cstdint>

struct ComponentRenderContext;

class ParticleSystemRenderer
{
public:


private: // Particle system.

	Core::ResourceUnorderedVectorU<ParticleSystem> m_ParticleSystems;

	Core::SimpleTypeVectorU<float> m_ParticleData; // @todo: type.

private: // Rendering.

	DirectX11Render::IndexedPrimitive m_Primitive;
	DirectX11Render::VertexInputLayout m_MergedIL;
	DirectX11Render::VertexBuffer m_InstanceBuffer;
	uint32_t m_PipelineStateIndex = Core::c_InvalidIndexU;

	void CreatePrimitive(const ComponentRenderContext& context);
	void CreateInstanceBuffer(const ComponentRenderContext& context);
	void CreatePipelineState(const ComponentRenderContext& context);

	void UpdateParticleInstances(const ComponentRenderContext& context);

public:

	uint32_t AddParticleSystem(const ParticleSystem_SystemParameters& systemParameters, 
		const ParticleSystemParameters& parameters);
	void RemoveParticleSystem(uint32_t systemIndex);

	void InitializeRendering(const ComponentRenderContext& context);
	void RenderContent(const ComponentRenderContext& context);
};