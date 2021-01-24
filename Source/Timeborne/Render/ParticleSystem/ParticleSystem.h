// Timeborne/Render/ParticleSystem/ParticleSystem.h

#pragma once

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <EngineBuildingBlocks/Math/GLM.h>

#include <cstdint>

struct ParticleSystem_SystemParameters
{
	uint32_t UpdateIntervalInMillis;
};

struct ParticleSystemEmitter
{

};

struct ParticleSystemParameters
{
	uint32_t Seed;
	uint32_t StartTickCount;

	glm::vec3 GlobalAccelleration;

	Core::SimpleTypeVectorU<ParticleSystemEmitter> Emitters;
};

class ParticleSystem
{
private: // Random number generation.

	uint32_t m_CurrentRandomValue = 0;

	void InitializeRandom(uint32_t seed);
	uint32_t GetRandomUint32();
	float GetRandomFloat();

private: // Updates.

	ParticleSystem_SystemParameters m_SystemParameters;

	uint32_t m_LastUpdateTickCount = 0;

public: // Particles.

	struct Particle
	{
		uint32_t LifeInMillis;
		glm::vec3 Position;
		glm::vec3 Speed;
		glm::vec3 OwnAccelleration;
	};

	struct State
	{
		Core::SimpleTypeVectorU<Particle> Particles;
	};

private:

	ParticleSystemParameters m_Parameters;

	State m_UpdateStates[2];
	State m_RenderState;
	uint32_t m_CurrentUpdateStateIndex = 0;

	void RemoveDeadParticles();
	void ComputeNextState();

public:

	void Reset(const ParticleSystem_SystemParameters& systemParameters,
		const ParticleSystemParameters& parameters);
	void Update(uint32_t currentTickCount);
	
	const State& GetRenderState(double gameTime);
};
