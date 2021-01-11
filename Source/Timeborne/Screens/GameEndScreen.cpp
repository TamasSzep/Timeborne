// Timeborne/Screens/GameEndScreen.cpp

#include <Timeborne/Screens/GameEndScreen.h>

#include <Timeborne/GameCreation/GameCreationData.h>
#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/GUI/TimeborneGUI.h>
#include <Timeborne/InGame/GameState/InGameStatistics.h>
#include <Timeborne/Render/PlayerColors.h>
#include <Timeborne/Misc/ScreenResolution.h>
#include <Timeborne/Screens/InGameScreen.h>
#include <Timeborne/MainApplication.h>

#include <numeric>

GameEndScreen::GameEndScreen()
{
	Reset();
}

GameEndScreen::~GameEndScreen()
{
}

void GameEndScreen::Reset()
{
	m_NextScreen = ApplicationScreens::GameEnd;
	m_HasConsole = false;
}

void GameEndScreen::Enter(const ComponentRenderContext& context)
{
	context.Application->SetAllowGUIActiveTracking(false);

	Reset();
}

void GameEndScreen::Exit()
{
}

void GameEndScreen::DerivedInitializeMain(const ComponentInitContext& context)
{
}

void GameEndScreen::InitializeRendering(const ComponentRenderContext& context)
{
}

void GameEndScreen::DestroyMain()
{
}

void GameEndScreen::DestroyRendering()
{
}

bool GameEndScreen::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_EscapeECI)
	{
		m_NextScreen = ApplicationScreens::MainMenu;
		return true;
	}
	return false;
}

void GameEndScreen::PreUpdate(const ComponentPreUpdateContext& context)
{
}

void GameEndScreen::PostUpdate(const ComponentPostUpdateContext& context)
{
}

void GameEndScreen::RenderFullscreenClear(const ComponentRenderContext& context)
{
	auto backgroundColor = ToFColor(nk_rgb(28, 48, 62));
	context.DeviceContext->ClearRenderTargetView(context.RTV, glm::value_ptr(backgroundColor));
}

void GameEndScreen::RenderContent(const ComponentRenderContext& context)
{
}

template <size_t Size>
void PrintToBuffer(char(&buffer)[Size], const Core::IndexVectorU& values)
{
	uint32_t countValues = values.GetSize();
	uint32_t sum = std::accumulate(values.GetArray(), values.GetEndPointer(), 0);
	int ctr = std::snprintf(buffer, Size, "%d (", sum);
	for (uint32_t i = 0; i < countValues; i++)
	{
		ctr += std::snprintf(buffer + ctr, Size - ctr, "%s%d", i > 0 ? ", " : "", values[i]);
	}
	std::snprintf(buffer + ctr, Size - ctr, ")");
}

void GameEndScreen::RenderGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	auto mwSize = glm::vec2(context.WindowSize.x, context.WindowSize.y);

	if (Nuklear_BeginWindow(ctx, "Game ended", glm::vec2(0.0f, 0.0f), glm::vec2(mwSize.x, mwSize.y)))
	{
		auto inGameScreen = GetScreen<InGameScreen>(ApplicationScreens::InGame, m_Application);
		const auto& statistics = inGameScreen->GetStatistics();
		const auto& sPlayerData = statistics.GetPlayerData();
		const auto& gameCreationData = inGameScreen->GetGameCreationData();

		uint32_t ownAlliance = gameCreationData.GetOwnPlayerData().AllianceIndex;
		uint32_t winnerAlliance = statistics.GetWinnerAlliance();
		auto mainText = (winnerAlliance == Core::c_InvalidIndexU)
			? "Game was canceled."
			: ((winnerAlliance == ownAlliance) ? "Victory!" : "Defeat!");

		nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1); // Empty row.

		nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
		nk_label(ctx, mainText, NK_TEXT_CENTERED);

		nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1); // Empty row.

		constexpr int c_CountColumns = 9;
		constexpr float c_ColumnWidth_Color = 40.0f;
		constexpr float c_ColumnWidth_Name = 200.0f;
		constexpr float c_ColumnWidth_Team = 40.0f;
		constexpr float c_ColumnWidth_Count = 200.0f;

		nk_layout_row_begin(ctx, NK_STATIC, c_ButtonSize.y, c_CountColumns);
		nk_layout_row_push(ctx, c_ColumnWidth_Color);
		nk_label(ctx, "Color", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, c_ColumnWidth_Name);
		nk_label(ctx, "Name", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, c_ColumnWidth_Team);
		nk_label(ctx, "Team", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, c_ColumnWidth_Count);
		nk_label(ctx, "Produced units", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, c_ColumnWidth_Count);
		nk_label(ctx, "Produced buildings", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, c_ColumnWidth_Count);
		nk_label(ctx, "Destroyed units", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, c_ColumnWidth_Count);
		nk_label(ctx, "Destroyed buildings", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, c_ColumnWidth_Count);
		nk_label(ctx, "Lost units", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, c_ColumnWidth_Count);
		nk_label(ctx, "Lost buildings", NK_TEXT_LEFT);
		nk_layout_row_end(ctx);

		char allianceBuffer[] = { '\0', '\0' };
		char buffer[256];

		uint32_t countPlayers = gameCreationData.Players.GetCountPlayers();
		for (uint32_t i = 0; i < countPlayers; i++)
		{
			const auto& playerData = gameCreationData.Players[i];

			auto playerColor = (playerData.LevelEditorIndex != Core::c_InvalidIndexU)
				? c_PlayerColors[playerData.LevelEditorIndex]
				: glm::vec3(0.5f, 0.5f, 0.5f);
			nk_color color = ToNKColor(glm::vec4(playerColor, 1.0f));

			allianceBuffer[0] = 'A' + (char)playerData.AllianceIndex;

			nk_layout_row_begin(ctx, NK_STATIC, c_ButtonSize.y, c_CountColumns);

			nk_layout_row_push(ctx, c_ColumnWidth_Color);
			nk_button_color(ctx, color);
			nk_layout_row_push(ctx, c_ColumnWidth_Name);
			nk_label(ctx, playerData.Name.c_str(), NK_TEXT_LEFT);
			nk_layout_row_push(ctx, c_ColumnWidth_Team);
			nk_label(ctx, allianceBuffer, NK_TEXT_LEFT);

			nk_layout_row_push(ctx, c_ColumnWidth_Count);
			sprintf_s(buffer, "%d", sPlayerData[i].ProducedUnits);
			nk_label(ctx, buffer, NK_TEXT_LEFT);

			nk_layout_row_push(ctx, c_ColumnWidth_Count);
			sprintf_s(buffer, "%d", sPlayerData[i].ProducedBuildings);
			nk_label(ctx, buffer, NK_TEXT_LEFT);

			nk_layout_row_push(ctx, c_ColumnWidth_Count);
			PrintToBuffer(buffer, sPlayerData[i].DestroyedUnits);
			nk_label(ctx, buffer, NK_TEXT_LEFT);

			nk_layout_row_push(ctx, c_ColumnWidth_Count);
			PrintToBuffer(buffer, sPlayerData[i].DestroyedBuildings);
			nk_label(ctx, buffer, NK_TEXT_LEFT);

			nk_layout_row_push(ctx, c_ColumnWidth_Count);
			sprintf_s(buffer, "%d", statistics.GetLostUnitCount(i));
			nk_label(ctx, buffer, NK_TEXT_LEFT);

			nk_layout_row_push(ctx, c_ColumnWidth_Count);
			sprintf_s(buffer, "%d", statistics.GetLostBuildingCount(i));
			nk_label(ctx, buffer, NK_TEXT_LEFT);

			nk_layout_row_end(ctx);
		}

		nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1); // Empty row.

		nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 1);
		if (TimeborneGUI_CreateExitButton(ctx)) m_NextScreen = ApplicationScreens::MainMenu;
	}
	nk_end(ctx);
}
