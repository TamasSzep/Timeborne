// Timeborne/InGame/View/GameObjects/HealthPointBarView.cpp

#include <Timeborne/InGame/View/GameObjects/HealthPointBarView.h>

#include <Timeborne/InGame/GameState/LocalGameState.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectFightPrototype.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/Render/Hud/HudRectangleRenderer.h>
#include <Timeborne/ApplicationComponent.h>

#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>

using namespace EngineBuildingBlocks::Graphics;

HealthPointBarView::HealthPointBarView(GameObjectVisibilityProvider& visibilityProvider)
	: GameObjectVisibilityListener(visibilityProvider)
{
}

HealthPointBarView::~HealthPointBarView()
{
}

void HealthPointBarView::Initialize(const ComponentRenderContext& context)
{
	m_HealthPointRenderer = std::make_unique<HudRectangleRenderer>();
	m_HealthPointRenderer->InitializeRendering(context);
}

void HealthPointBarView::Destroy()
{
	m_HealthPointRenderer.reset();
}

void HealthPointBarView::OnLoading(const ComponentRenderContext& context)
{
	m_SourceGameObjectIds.Clear();

	m_HealthPointRenderer->ClearRectangles();
	m_HealtPointRectangleIndices.Clear();

	UpdateSourceGameObjects();
}

void HealthPointBarView::PreUpdate(const ComponentPreUpdateContext& context)
{
	assert(m_Camera != nullptr);

	UpdateSourceGameObjects();
	UpdateHealthPointRendering(context, *m_Camera);
}

void HealthPointBarView::RenderContent(const ComponentRenderContext& context)
{
	m_HealthPointRenderer->RenderContent(context);
}

void HealthPointBarView::UpdateSourceGameObjects()
{
	assert(m_LocalGameState != nullptr);
	const auto& stateSourceGameObjectIds = m_LocalGameState->GetControllerGameState().SourceGameObjectIds;

	if (m_SourceGameObjectIds != stateSourceGameObjectIds)
	{
		m_SourceGameObjectIds = stateSourceGameObjectIds;
	}
}

void HealthPointBarView::UpdateHealthPointRendering(const ComponentPreUpdateContext& context, Camera& camera)
{
	constexpr uint32_t c_MaxHpUpperThreshold = 1000;
	constexpr float c_LowerThresholdBarCountLog2 = 3.2f;
	constexpr float c_UpperThresholdBarCountLog2 = 4.0f;

	constexpr float c_GreenHpRatio = 0.5f;
	constexpr float c_YellowHpRatio = 0.25f;

	assert(m_VisibilityProvider != nullptr && m_GameState != nullptr);

	auto snapToPixels = [&context](const glm::vec2& vInCs) {
		glm::dvec2 resolution(context.ContentSize);
		auto vInPixels = glm::dvec2(vInCs) * resolution / 2.0;
		vInPixels = glm::round(vInPixels);
		return glm::vec2(glm::dvec2(vInPixels) / resolution * 2.0);
	};

	// Snapping sizes to the content size.
	const auto c_MarginSize = snapToPixels({ 0.0015f, 0.003f });
	const auto c_HPLineSize = snapToPixels({ 0.003f, 0.015f });

	const glm::vec4 c_BigFrameColor(0.75f, 0.75f, 0.75f, 1.0f);
	const glm::vec4 c_SmallFrameColor(0.0f, 0.0f, 0.0f, 1.0f);

	m_HealthPointRenderer->RemoveRectangles(m_HealtPointRectangleIndices);
	m_HealtPointRectangleIndices.Clear();

	auto addRectangle = [this](const HudRectangleRenderer::Rectangle& rectangle) {
		auto index = m_HealthPointRenderer->AddRectangle(rectangle);
		m_HealtPointRectangleIndices.PushBack(index);
	};

	auto& prototypes = GameObjectPrototype::GetPrototypes();
	auto& gameObjects = m_GameState->GetGameObjects().Get();
	auto& fightList = m_GameState->GetFightList();

	auto& viewProj = camera.GetViewProjectionMatrix();

	unsigned countObjects = m_SourceGameObjectIds.GetSize();
	for (unsigned i = 0; i < countObjects; i++)
	{
		auto gameObjectId = m_SourceGameObjectIds[i];

		auto gIt = gameObjects.find(gameObjectId);
		assert(gIt != gameObjects.end());
		auto& gameObject = gIt->second;

		if (gameObject.FightIndex == Core::c_InvalidIndexU)
		{
			continue;
		}

		auto& fightData = fightList[gameObject.FightIndex];

		auto& prototype = *prototypes[(uint32_t)gameObject.Data.TypeIndex];
		auto maxHealthPoints = prototype.GetFight().MaxHealthPoints;

		auto healthPoints = fightData.HealthPoints;

		float hpRatio = (float)healthPoints / (float)maxHealthPoints;

		float maxHpFactor = (float)glm::clamp(maxHealthPoints, 0U, c_MaxHpUpperThreshold) / (float)c_MaxHpUpperThreshold;
		uint32_t maxHPLines = (uint32_t)std::round(std::pow(2.0f,
			glm::mix(c_LowerThresholdBarCountLog2, c_UpperThresholdBarCountLog2, maxHpFactor)));

		uint32_t countHPLines = (uint32_t)std::round(hpRatio * (float)maxHPLines);

		auto hpColor = (hpRatio >= c_GreenHpRatio) ? glm::vec3(0.0f, 1.0f, 0.0f)
			: ((hpRatio >= c_YellowHpRatio) ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(0.8f, 0.0f, 0.0f));

		auto box = m_VisibilityProvider->GetTransformedBox(gameObjectId);
		glm::vec4 attachPoint((box.Minimum.x + box.Maximum.x) * 0.5f, box.Maximum.y,
			(box.Minimum.z + box.Maximum.z) * 0.5f, 1.0f);
		glm::vec4 frameMiddle4 = viewProj * attachPoint;
		glm::vec2 frameMiddle = glm::vec2(frameMiddle4) / frameMiddle4.w;

		glm::vec2 smallFrameSize(c_HPLineSize.x * maxHPLines + c_MarginSize.x * (maxHPLines + 1),
			c_HPLineSize.y + 2 * c_MarginSize.y);

		// Snapping the frame start to a pixel.
		auto halfSmallFrameX = smallFrameSize * 0.5f;
		auto smallFrameStart = snapToPixels(frameMiddle - halfSmallFrameX);
		frameMiddle = smallFrameStart + halfSmallFrameX;

		HudRectangleRenderer::Rectangle bigFrame;
		bigFrame.middleInCs = frameMiddle;
		bigFrame.sizeInCs = smallFrameSize + c_MarginSize * 2.0f;
		bigFrame.color = c_BigFrameColor;
		bigFrame.z = 0;
		addRectangle(bigFrame);

		HudRectangleRenderer::Rectangle smallFrame;
		smallFrame.middleInCs = frameMiddle;
		smallFrame.sizeInCs = smallFrameSize;
		smallFrame.color = c_SmallFrameColor;
		smallFrame.z = 1;
		addRectangle(smallFrame);

		float hpLineStartX = smallFrameStart.x + c_MarginSize.x + c_HPLineSize.x * 0.5f;

		for (unsigned j = 0; j < countHPLines; j++)
		{
			HudRectangleRenderer::Rectangle hpLine;
			hpLine.middleInCs = glm::vec2(hpLineStartX + (c_HPLineSize.x + c_MarginSize.x) * j, frameMiddle.y);
			hpLine.sizeInCs = c_HPLineSize;
			hpLine.color = glm::vec4(hpColor.r, hpColor.g, hpColor.b, 1.0f);
			hpLine.z = 2;
			addRectangle(hpLine);
		}
	}
}
