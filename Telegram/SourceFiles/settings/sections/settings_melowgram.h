// MelowGram main menu

#pragma once

#include "settings/settings_common.h"
#include "ui/wrap/vertical_layout.h"

namespace Settings {

[[nodiscard]] Type MelowGramId();

not_null<Ui::VerticalLayout*> AddRoundedBlock(not_null<Ui::VerticalLayout*> container);
const style::SettingsButton &GetRoundedButtonStyle();
const style::SettingsButton &GetRoundedButtonNoIconStyle();

void AttachMelowSvgIcon(
	not_null<Ui::SettingsButton*> button,
	const style::SettingsButton &st,
	const QString &svgResourcePath,
	int iconSize = 20);

not_null<Ui::SettingsButton*> AddButtonWithSvgIcon(
	not_null<Ui::VerticalLayout*> container,
	rpl::producer<QString> text,
	const style::SettingsButton &st,
	const QString &svgResourcePath,
	int iconSize = 20);

not_null<Ui::SettingsButton*> AddButtonWithSvgLabel(
	not_null<Ui::VerticalLayout*> container,
	rpl::producer<QString> text,
	rpl::producer<QString> label,
	const style::SettingsButton &st,
	const QString &svgResourcePath,
	int iconSize = 20);

} // namespace Settings
