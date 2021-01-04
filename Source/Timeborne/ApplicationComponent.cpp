// Timeborne/ApplicationComponent.cpp

#include <Timeborne/ApplicationComponent.h>

#include <Timeborne/MainApplication.h>

using namespace EngineBuildingBlocks::Input;

void ApplicationScreen::InitializeCommonInputs(const ComponentInitContext& context)
{
	auto application = context.Application;
	auto keyHandler = application->GetKeyHandler();

	// Registering state key events.
	m_EscapeECI = keyHandler->RegisterStateKeyEventListener("ApplicationScreen.Escape", application);

	// Binding the key events.
	keyHandler->BindEventToKey(m_EscapeECI, Keys::Escape);
}
