// MelowGram Theme Settings


#include "settings/sections/settings_melowgram_theme.h"

#include "settings/settings_common_session.h"
#include "ui/vertical_list.h"
#include "settings/settings_common.h"
#include "settings/sections/settings_melowgram.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/widgets/continuous_sliders.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/buttons.h"
#include "ui/boxes/confirm_box.h"
#include "core/file_utilities.h"
#include "core/application.h"
#include "window/main_window.h"
#include "window/window_controller.h"
#include "core/core_settings.h"
#include "lang/lang_keys.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"
#include "styles/style_boxes.h"
#include "styles/style_menu_icons.h"
#include "window/themes/window_theme.h"
#include "window/window_session_controller.h"
#include "settings/settings_builder.h"

namespace Settings {

MelowGramTheme::MelowGramTheme(QWidget *parent, not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent(controller);
}

rpl::producer<QString> MelowGramTheme::title() {
	return rpl::single(u"Theme"_q);
}

void MelowGramTheme::setupContent(not_null<Window::SessionController*> controller) {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	
	auto block1 = Settings::AddRoundedBlock(content);

	const auto gifEnabled = Settings::AddButtonWithIcon(
		block1,
		rpl::single(u"GIF BackGround"_q),
		Settings::GetRoundedButtonStyle(),
		{ &st::menuIconPalette }
	);
	gifEnabled->toggleOn(rpl::single(Core::App().settings().readPref<bool>("MelowGramGifBackground", false)));

	auto optionsWrap = block1->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			block1,
			object_ptr<Ui::VerticalLayout>(block1)
		)
	);
	
	optionsWrap->toggleOn(gifEnabled->toggledValue());
	
	std::move(gifEnabled->toggledValue()) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramGifBackground", false);
	}) | rpl::on_next([=](bool value) {
		Core::App().settings().writePref<bool>("MelowGramGifBackground", value);
		Window::Theme::ApplyMelowGramModifiers();
		style::NotifyPaletteChanged();
	}, optionsWrap->lifetime());
	
	auto optionsContent = optionsWrap->entity();

	const auto gifButton = Settings::AddButtonWithIcon(
		optionsContent,
		rpl::single(u"Choose GIF file..."_q),
		Settings::GetRoundedButtonStyle(),
		{ &st::menuIconFile }
	);
	gifButton->setClickedCallback([=] {
		const auto filters = u"GIF files (*.gif);;All files (*.*)"_q;
		FileDialog::GetOpenPath(
			this, 
			u"Select Background GIF"_q, 
			filters, 
			[=](const FileDialog::OpenResult &result) {
				if (!result.paths.isEmpty()) {
					Core::App().settings().writePref<QString>(
						"MelowGramGifPath",
						result.paths.first());
				}
			});
	});
	
	auto block2 = Settings::AddRoundedBlock(content);
	
	const auto blurEnabled = Settings::AddButtonWithIcon(
		block2,
		rpl::single(u"Blur Behind Window (Windows 10+)"_q),
		Settings::GetRoundedButtonStyle(),
		{ &st::menuIconShowAll }
	);
	blurEnabled->toggleOn(rpl::single(Core::App().settings().readPref<bool>("MelowGramBlur", false)));
	
	std::move(blurEnabled->toggledValue()) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramBlur", false);
	}) | rpl::on_next([=](bool value) {
		Core::App().settings().writePref<bool>("MelowGramBlur", value);
		controller->window().updateIsActiveFocus();
	}, blurEnabled->lifetime());

	auto block3 = Settings::AddRoundedBlock(content);
	
	auto titleLabel = Ui::CreateChild<Ui::FlatLabel>(block3, u"Blackout"_q, st::defaultFlatLabel);
	block3->add(object_ptr<Ui::FlatLabel>::fromRaw(titleLabel), QMargins(22, 10, 22, 0));
	
	int initialValue = Core::App().settings().readPref<int>("MelowGramBlackout", 50);

	auto slider = Settings::MakeSliderWithLabel(
		block3,
		st::settingsScale,
		st::settingsScaleLabel,
		15
	);
	
	auto sliderLabel = slider.label;
	auto sliderWidget = slider.slider;
	
	sliderWidget->setChangeProgressCallback([=](float64 value) {
		const int pct = std::round(value * 100);
		Core::App().settings().writePref<int>(
			"MelowGramBlackout",
			pct);
		sliderLabel->setText(QString::number(pct) + "%");
		Window::Theme::ApplyMelowGramModifiers();
		style::NotifyPaletteChanged();
	});
	sliderWidget->setValue(initialValue / 100.0);
	sliderLabel->setText(QString::number(initialValue) + "%");

	block3->add(std::move(slider.widget), QMargins(22, 5, 22, 10));

	auto effectsBlock = Settings::AddRoundedBlock(content);

	const auto effectsToggle = Settings::AddButtonWithIcon(
		effectsBlock,
		rpl::single(u"Effects"_q),
		Settings::GetRoundedButtonStyle(),
		{ &st::menuIconPremium }
	);
	effectsToggle->toggleOn(rpl::single(
		Core::App().settings().readPref<bool>("MelowGramEffects", false)
	));

	auto effectsWrap = effectsBlock->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			effectsBlock,
			object_ptr<Ui::VerticalLayout>(effectsBlock)
		)
	);

	auto effectsInner = effectsWrap->entity();

	const auto effectsTypeGroup = std::make_shared<Ui::RadiobuttonGroup>(
		Core::App().settings().readPref<int>("MelowGramEffectsType", 0)
	);

	auto addEffectRadio = [&](int value, const QString &text) {
		auto radio = Ui::CreateChild<Ui::Radiobutton>(
			effectsInner,
			effectsTypeGroup,
			value,
			text,
			st::settingsCheckbox
		);
		effectsInner->add(object_ptr<Ui::Radiobutton>::fromRaw(radio), style::margins(22, 5, 22, 5));
	};
	addEffectRadio(0, "Snow");
	addEffectRadio(1, "Rain");

	effectsTypeGroup->setChangedCallback([=](int value) {
		Core::App().settings().writePref<int>("MelowGramEffectsType", value);
		Core::App().saveSettingsDelayed();
		controller->window().widget()->update();
	});

	auto speedTitle = Ui::CreateChild<Ui::FlatLabel>(effectsInner, u"Speed"_q, st::defaultFlatLabel);
	effectsInner->add(object_ptr<Ui::FlatLabel>::fromRaw(speedTitle), QMargins(22, 10, 22, 0));

	int initialSpeed = Core::App().settings().readPref<int>("MelowGramEffectsSpeed", 50);

	auto speedSlider = Settings::MakeSliderWithLabel(
		effectsInner,
		st::settingsScale,
		st::settingsScaleLabel,
		15
	);

	auto speedSliderLabel = speedSlider.label;
	auto speedSliderWidget = speedSlider.slider;

	speedSliderWidget->setChangeProgressCallback([=](float64 value) {
		const int val = std::clamp(int(std::round(value * 100)), 1, 100);
		Core::App().settings().writePref<int>("MelowGramEffectsSpeed", val);
		speedSliderLabel->setText(QString::number(val) + "%");
	});
	speedSliderWidget->setValue(initialSpeed / 100.0);
	speedSliderLabel->setText(QString::number(initialSpeed) + "%");

	effectsInner->add(std::move(speedSlider.widget), QMargins(22, 5, 22, 10));

	effectsWrap->toggleOn(effectsToggle->toggledValue());

	std::move(
		effectsToggle->toggledValue()
	) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramEffects", false);
	}) | rpl::on_next([=](bool value) {
		Core::App().settings().writePref<bool>("MelowGramEffects", value);
		Core::App().saveSettingsDelayed();
		controller->window().widget()->update();
	}, effectsToggle->lifetime());

	Ui::ResizeFitChild(this, content);
}

Type MelowGramThemeId() {
	return MelowGramTheme::Id();
}

} // namespace Settings
