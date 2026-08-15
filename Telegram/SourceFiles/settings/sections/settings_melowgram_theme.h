// MelowGram Theme Settings

#pragma once

#include "settings/settings_common_session.h"

namespace Settings {

Type MelowGramThemeId();

class MelowGramTheme : public Section<MelowGramTheme> {
public:
	MelowGramTheme(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent(not_null<Window::SessionController*> controller);
};

} // namespace Settings
