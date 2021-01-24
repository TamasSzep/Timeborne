// Timeborne/Render/ParticleSystem/ParticleSystem.cpp

#include <Timeborne/Render/ParticleSystem/ParticleSystem.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Implementing the random number generation using the Wang hash.

uint32_t WangHash(uint32_t seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}

void ParticleSystem::InitializeRandom(uint32_t seed)
{
	m_CurrentRandomValue = seed;
}

uint32_t ParticleSystem::GetRandomUint32()
{
	m_CurrentRandomValue = WangHash(m_CurrentRandomValue);

	return m_CurrentRandomValue;
}

float ParticleSystem::GetRandomFloat()
{
	return (float)GetRandomUint32() / (float)0xffffffff;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ParticleSystem::Reset(const ParticleSystem_SystemParameters& systemParameters,
	const ParticleSystemParameters& parameters)
{
	InitializeRandom(parameters.Seed);

	m_SystemParameters = systemParameters;
	m_LastUpdateTickCount = parameters.StartTickCount;

	m_Parameters = parameters;
	m_CurrentUpdateStateIndex = 0;

	auto& currentState = m_UpdateStates[m_CurrentUpdateStateIndex];
	currentState.Particles.Clear();

	ComputeNextState();
}

void ParticleSystem::Update(uint32_t currentTickCount)
{
	for (; m_LastUpdateTickCount < currentTickCount; m_LastUpdateTickCount++)
	{
		m_CurrentUpdateStateIndex = 1 - m_CurrentUpdateStateIndex;

		RemoveDeadParticles();
		ComputeNextState();
	}
}

void ParticleSystem::RemoveDeadParticles()
{
	auto& currentState = m_UpdateStates[m_CurrentUpdateStateIndex];
	auto& particles = currentState.Particles;

	for (uint32_t i = 0; i < particles.GetSize();)
	{
		if (particles[i].LifeInMillis == 0) particles.RemoveWithLastElementCopy(i);
		else i++;
	}
}

void ParticleSystem::ComputeNextState()
{
	const auto& currentState = m_UpdateStates[m_CurrentUpdateStateIndex];
	auto& nextState = m_UpdateStates[1 - m_CurrentUpdateStateIndex];

	uint32_t updateIntervalInMillis = m_SystemParameters.UpdateIntervalInMillis;
	float dt = (float)updateIntervalInMillis * 1e-3f;
	auto globalAcceleration = m_Parameters.GlobalAccelleration;

	// Updating existing particles.
	uint32_t countParticles = currentState.Particles.GetSize();
	nextState.Particles.Resize(countParticles);
	for (uint32_t i = 0; i < countParticles; i++)
	{
		const auto& source = currentState.Particles[i];
		auto& target = nextState.Particles[i];

		// Not killing the particle yet, because it is used in the rendering.
		uint32_t lifeTimeInMillis = source.LifeInMillis;
		lifeTimeInMillis =
			(lifeTimeInMillis <= updateIntervalInMillis) ? 0 : (lifeTimeInMillis - updateIntervalInMillis);

		auto ownAcceleration = source.OwnAccelleration;
		auto acceleration = ownAcceleration + globalAcceleration;
		auto speed = source.Speed + acceleration * dt;

		target.LifeInMillis = lifeTimeInMillis;
		target.OwnAccelleration = ownAcceleration;
		target.Speed = speed;
		target.Position = source.Position + speed * dt;

		countParticles++;
	}

	// Emitting new particles.
	uint32_t countEmitters = m_Parameters.Emitters.GetSize();
	for (uint32_t i = 0; i < countEmitters; i++)
	{
		const auto& emitter = m_Parameters.Emitters[i];

		// @todo... call nextState.Particles.PushBack
	}
}

const ParticleSystem::State& ParticleSystem::GetRenderState(double gameTime)
{
	float dt = (float)m_SystemParameters.UpdateIntervalInMillis * 1e-3f;
	double tickCount = gameTime / dt;
	double nextUpdateTickCount = m_LastUpdateTickCount + 1.0;
	assert(tickCount >= (double)m_LastUpdateTickCount && tickCount <= (double)nextUpdateTickCount);
	double interpolationFactor = nextUpdateTickCount - tickCount;

	const auto& currentState = m_UpdateStates[m_CurrentUpdateStateIndex];
	auto& nextState = m_UpdateStates[1 - m_CurrentUpdateStateIndex];

	uint32_t countParticles = currentState.Particles.GetSize();
	m_RenderState.Particles.ClearAndReserve(countParticles);

	// ...

	return m_RenderState;
}

// Problems:
//
//   - local vs global particle systems
//   - the rendering of the objects must be synchronized first!