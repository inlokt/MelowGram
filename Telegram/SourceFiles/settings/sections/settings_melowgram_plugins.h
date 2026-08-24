// MelowGram Plugins Settings

#pragma once

#include "settings/settings_common_session.h"

namespace Settings {

Type MelowGramPluginsId();

class MelowGramPlugins : public Section<MelowGramPlugins> {
public:
	MelowGramPlugins(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent(not_null<Window::SessionController*> controller);

};

} // namespace Settings
