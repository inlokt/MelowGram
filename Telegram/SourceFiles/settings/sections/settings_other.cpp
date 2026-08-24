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
#include "ui/widgets/continuous_sliders.h"
#include "ui/widgets/labels.h"
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
#include "window/window_session_controller.h"

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

	const auto toggle = Settings::AddButtonWithSvgIcon(
		block,
		rpl::single(u"Save deleted messages"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/other/save_del.svg"_q
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

	const auto ghostToggle = Settings::AddButtonWithSvgIcon(
		block,
		rpl::single(u"Ghost mode"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/other/ghost_mode.svg"_q
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

	const auto saveTtlToggle = Settings::AddButtonWithSvgIcon(
		block,
		rpl::single(u"Save self-destructing media"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/other/save_dest.svg"_q
	);
	saveTtlToggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramSaveTTLMedia", false)
	));
	
	std::move(
		saveTtlToggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramSaveTTLMedia", false);
	}) | rpl::on_next([](bool value) {
		Core::App().settings().writePref<bool>("MelowGramSaveTTLMedia", value);
		Core::App().saveSettingsDelayed();
	}, saveTtlToggle->lifetime());

	const auto editOthersToggle = Settings::AddButtonWithSvgIcon(
		block,
		rpl::single(u"Modifying other people's messages"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/other/edit.svg"_q
	);
	editOthersToggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramEditOthersMessages", false)
	));
	
	std::move(
		editOthersToggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramEditOthersMessages", false);
	}) | rpl::on_next([](bool value) {
		Core::App().settings().writePref<bool>("MelowGramEditOthersMessages", value);
		Core::App().saveSettingsDelayed();
	}, editOthersToggle->lifetime());

	const auto streamerToggle = Settings::AddButtonWithSvgIcon(
		block,
		rpl::single(u"Streamer Mode"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/other/streamer.svg"_q
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
		inner->add(object_ptr<Ui::Radiobutton>::fromRaw(radio), style::margins(22, 5, 22, 5));
	};
	addRadio(0, "Всех");
	addRadio(1, "Себя");

	streamerScopeGroup->setChangedCallback([=](int value) {
		Core::App().settings().writePref<int>("MelowGramStreamerModeScope", value);
		Core::App().saveSettingsDelayed();
	});
	
	scopeWrap->toggleOn(streamerToggle->toggledValue());

	const auto customSwitcherToggle = Settings::AddButtonWithSvgIcon(
		block,
		rpl::single(u"Custom Switcher"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/other/boolean.svg"_q
	);
	customSwitcherToggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramCustomSwitcher", false)
	));
	
	std::move(
		customSwitcherToggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramCustomSwitcher", false);
	}) | rpl::on_next([=](bool value) {
		Core::App().settings().writePref<bool>("MelowGramCustomSwitcher", value);
		Core::App().saveSettingsDelayed();
		update();
	}, customSwitcherToggle->lifetime());

	const auto hideCustomBgToggle = Settings::AddButtonWithSvgIcon(
		block,
		rpl::single(u"Hide custom backgrounds"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/other/hide_custom.svg"_q
	);
	hideCustomBgToggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramHideCustomBackgrounds", false)
	));
	
	std::move(
		hideCustomBgToggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramHideCustomBackgrounds", false);
	}) | rpl::on_next([=](bool value) {
		Core::App().settings().writePref<bool>("MelowGramHideCustomBackgrounds", value);
		Core::App().saveSettingsDelayed();
		update();
	}, hideCustomBgToggle->lifetime());

	const auto avatarRoundingToggle = Settings::AddButtonWithSvgIcon(
		block,
		rpl::single(u"Custom avatar rounding"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/other/avatar.svg"_q
	);
	avatarRoundingToggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramCustomAvatarRounding", false)
	));

	auto avatarRoundingWrap = block->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			block,
			object_ptr<Ui::VerticalLayout>(block)
		)
	);

	auto avatarRoundingInner = avatarRoundingWrap->entity();

	int initialRounding = std::clamp(
		Core::App().settings().readPref<int>("MelowGramAvatarRadius", 100),
		10,
		100
	);

	auto roundingSlider = Settings::MakeSliderWithLabel(
		avatarRoundingInner,
		st::settingsScale,
		st::settingsScaleLabel,
		15
	);

	auto roundingSliderLabel = roundingSlider.label;
	auto roundingSliderWidget = roundingSlider.slider;

	roundingSliderWidget->setChangeProgressCallback([=](float64 value) {
		const int val = std::clamp(int(std::round(10 + value * 90)), 10, 100);
		Core::App().settings().writePref<int>("MelowGramAvatarRadius", val);
		roundingSliderLabel->setText(QString::number(val) + "%");
		update();
	});
	roundingSliderWidget->setValue((initialRounding - 10) / 90.0);
	roundingSliderLabel->setText(QString::number(initialRounding) + "%");

	avatarRoundingInner->add(std::move(roundingSlider.widget), QMargins(22, 5, 22, 10));

	avatarRoundingWrap->toggleOn(avatarRoundingToggle->toggledValue());

	std::move(
		avatarRoundingToggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramCustomAvatarRounding", false);
	}) | rpl::on_next([=](bool value) {
		Core::App().settings().writePref<bool>("MelowGramCustomAvatarRounding", value);
		Core::App().saveSettingsDelayed();
		update();
	}, avatarRoundingToggle->lifetime());

	const auto alwaysShowLastVisitToggle = Settings::AddButtonWithSvgIcon(
		block,
		rpl::single(u"Always show last visit"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/other/show_last.svg"_q
	);
	alwaysShowLastVisitToggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramAlwaysShowLastVisit", false)
	));

	std::move(
		alwaysShowLastVisitToggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramAlwaysShowLastVisit", false);
	}) | rpl::on_next([=](bool value) {
		Core::App().settings().writePref<bool>("MelowGramAlwaysShowLastVisit", value);
		Core::App().saveSettingsDelayed();
		update();
	}, alwaysShowLastVisitToggle->lifetime());

	const auto displayRepostsToggle = Settings::AddButtonWithSvgIcon(
		block,
		rpl::single(u"Display reposts in channels"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/other/repost.svg"_q
	);
	displayRepostsToggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramDisplayRepostsInChannels", false)
	));

	std::move(
		displayRepostsToggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramDisplayRepostsInChannels", false);
	}) | rpl::on_next([=](bool value) {
		Core::App().settings().writePref<bool>("MelowGramDisplayRepostsInChannels", value);
		Core::App().saveSettingsDelayed();
		update();
	}, displayRepostsToggle->lifetime());

	Ui::ResizeFitChild(this, content);
}

} // namespace

Type OtherId() {
	return Other::Id();
}

} // namespace Settings

bool IsMelowGramSaveTTLMediaEnabled() {
	if (!Core::IsAppLaunched()) {
		return false;
	}
	return Core::App().settings().readPref<bool>("MelowGramSaveTTLMedia", false);
}

bool IsMelowGramEditOthersMessagesEnabled() {
	if (!Core::IsAppLaunched()) {
		return false;
	}
	return Core::App().settings().readPref<bool>("MelowGramEditOthersMessages", false);
}

bool IsMelowGramDisplayRepostsInChannelsEnabled() {
	if (!Core::IsAppLaunched()) {
		return false;
	}
	return Core::App().settings().readPref<bool>("MelowGramDisplayRepostsInChannels", false);
}

bool IsMelowGramAlwaysShowLastVisitEnabled() {
	if (!Core::IsAppLaunched()) {
		return false;
	}
	return Core::App().settings().readPref<bool>("MelowGramAlwaysShowLastVisit", false);
}
