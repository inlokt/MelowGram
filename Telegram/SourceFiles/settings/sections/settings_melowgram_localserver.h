#pragma once

#include "settings/settings_common_session.h"

namespace Settings {

Type LocalServerId();

class LocalServerSection : public Section<LocalServerSection> {
public:
	LocalServerSection(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent(not_null<Window::SessionController*> controller);
};

} // namespace Settings
