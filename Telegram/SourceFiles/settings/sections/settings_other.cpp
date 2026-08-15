/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
/*
 * modified for melowgram 23.07.2026
 */
#include "settings/sections/settings_other.h"

#include "ui/vertical_list.h"
#include "settings/settings_common.h"
#include "settings/settings_common_session.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/slide_wrap.h"
#include "lang/lang_keys.h"
#include "styles/style_settings.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "styles/style_widgets.h"
#include "styles/style_window.h"
#include "styles/style_menu_icons.h"
#include "ui/painter.h"
#include "core/local_url_handlers.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "storage/localstorage.h"
#include "settings/settings_builder.h"
#include "settings/sections/settings_melowgram.h"

namespace Settings {
namespace {

class Other : public Section<Other> {
public:
	Other(QWidget *parent, not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();
};

Other::Other(QWidget *parent, not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> Other::title() {
	return rpl::single(u"Other"_q);
}

void Other::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	auto block = Settings::AddRoundedBlock(content);

	const auto toggle = Settings::AddButtonWithIcon(
		block,
		rpl::single(u"Save deleted messages"_q),
		Settings::GetRoundedButtonStyle(),
		{ &st::menuIconChatBubble }
	);
	toggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramSaveDeleted", false)
	));
	
	std::move(
		toggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramSaveDeleted", false);
	}) | rpl::on_next([](bool value) {
		Core::App().settings().writePref<bool>("MelowGramSaveDeleted", value);
		Core::App().saveSettingsDelayed();
	}, toggle->lifetime());

	const auto ghostToggle = Settings::AddButtonWithIcon(
		block,
		rpl::single(u"Ghost mode"_q),
		Settings::GetRoundedButtonStyle(),
		{ &st::menuIconStealth }
	);
	ghostToggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramGhostMode", false)
	));
	
	std::move(
		ghostToggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramGhostMode", false);
	}) | rpl::on_next([](bool value) {
		Core::App().settings().writePref<bool>("MelowGramGhostMode", value);
		Core::App().saveSettingsDelayed();
	}, ghostToggle->lifetime());

	const auto streamerToggle = Settings::AddButtonWithIcon(
		block,
		rpl::single(u"Streamer Mode"_q),
		Settings::GetRoundedButtonStyle(),
		{ &st::menuIconShowInFolder }
	);
	streamerToggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramStreamerMode", false)
	));
	
	std::move(
		streamerToggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramStreamerMode", false);
	}) | rpl::on_next([](bool value) {
		Core::App().settings().writePref<bool>("MelowGramStreamerMode", value);
		Core::App().saveSettingsDelayed();
	}, streamerToggle->lifetime());

	const auto streamerScopeGroup = std::make_shared<Ui::RadiobuttonGroup>(
		Core::App().settings().readPref<int>("MelowGramStreamerModeScope", 0)
	);

	auto scopeWrap = block->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			block,
			object_ptr<Ui::VerticalLayout>(block)
		)
	);
	
	auto inner = scopeWrap->entity();
	
	auto addRadio = [&](int value, const QString &text) {
		auto radio = Ui::CreateChild<Ui::Radiobutton>(
			inner,
			streamerScopeGroup,
			value,
			text,
			st::settingsCheckbox
		);
		inner->add(object_ptr<Ui::Radiobutton>::fromRaw(radio), style::margins(54, 5, 0, 5));
	};
	addRadio(0, "Всех");
	addRadio(1, "Себя");

	streamerScopeGroup->setChangedCallback([=](int value) {
		Core::App().settings().writePref<int>("MelowGramStreamerModeScope", value);
		Core::App().saveSettingsDelayed();
	});
	
	scopeWrap->toggleOn(streamerToggle->toggledValue());

	Ui::ResizeFitChild(this, content);
}

} // namespace

Type OtherId() {
	return Other::Id();
}

} // namespace Settings

bool IsMelowGramSmoothScrollEnabled() {
	return Core::App().settings().readPref<bool>("MelowGramSmoothScroll", true);
}