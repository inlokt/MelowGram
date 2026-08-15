// MelowGram main menu

#pragma once

#include "settings/settings_common.h"
#include "ui/wrap/vertical_layout.h"

namespace Settings {

[[nodiscard]] Type MelowGramId();

not_null<Ui::VerticalLayout*> AddRoundedBlock(not_null<Ui::VerticalLayout*> container);
const style::SettingsButton &GetRoundedButtonStyle();
const style::SettingsButton &GetRoundedButtonNoIconStyle();

} // namespace Settings
