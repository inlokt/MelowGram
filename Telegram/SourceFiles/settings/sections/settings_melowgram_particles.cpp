// MelowGram Particles Settings

#include "settings/sections/settings_melowgram_particles.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "settings/settings_common_session.h"
#include "settings/settings_common.h"
#include "ui/vertical_list.h"
#include "ui/widgets/continuous_sliders.h"
#include "ui/widgets/labels.h"
#include "styles/style_widgets.h"
#include "styles/style_window.h"
#include "ui/widgets/buttons.h"
#include "styles/style_settings.h"
#include <utility>
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "window/themes/window_theme.h"
#include "settings/settings_builder.h"
#include "settings/sections/settings_melowgram.h"
#include "lang/lang_keys.h"
#include "styles/style_menu_icons.h"

namespace Settings {
namespace {

class MelowGramParticles : public Section<MelowGramParticles> {
public:
	MelowGramParticles(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent(not_null<Window::SessionController*> controller);
};

MelowGramParticles::MelowGramParticles(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent(controller);
}

rpl::producer<QString> MelowGramParticles::title() {
	return rpl::single(u"Particles"_q);
}

void MelowGramParticles::setupContent(not_null<Window::SessionController*> controller) {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	auto block1 = Settings::AddRoundedBlock(content);

	const auto initialMove = Core::App().settings().readPref<bool>("MelowGramParticlesMove", false);
	auto moveToggle = Settings::AddButtonWithIcon(
		block1,
		rpl::single(u"While moving"_q),
		Settings::GetRoundedButtonNoIconStyle()
	);
	moveToggle->toggleOn(rpl::single(initialMove));
	
	std::move(moveToggle->toggledValue()) | rpl::on_next([=](bool checked) {
		Core::App().settings().writePref<bool>("MelowGramParticlesMove", checked);
		Core::App().saveSettingsDelayed();
	}, moveToggle->lifetime());

	const auto initialClick = Core::App().settings().readPref<bool>("MelowGramParticlesClick", false);
	auto clickToggle = Settings::AddButtonWithIcon(
		block1,
		rpl::single(u"On click"_q),
		Settings::GetRoundedButtonNoIconStyle()
	);
	clickToggle->toggleOn(rpl::single(initialClick));
	
	std::move(clickToggle->toggledValue()) | rpl::on_next([=](bool checked) {
		Core::App().settings().writePref<bool>("MelowGramParticlesClick", checked);
		Core::App().saveSettingsDelayed();
	}, clickToggle->lifetime());

	const auto initialMercury = Core::App().settings().readPref<bool>("MelowGramMercury", false);
	auto mercuryToggle = Settings::AddButtonWithIcon(
		block1,
		rpl::single(u"Mercury (Click ripple)"_q),
		Settings::GetRoundedButtonNoIconStyle()
	);
	mercuryToggle->toggleOn(rpl::single(initialMercury));
	
	std::move(mercuryToggle->toggledValue()) | rpl::on_next([=](bool checked) {
		Core::App().settings().writePref<bool>("MelowGramMercury", checked);
		Core::App().saveSettingsDelayed();
	}, mercuryToggle->lifetime());

	auto block2 = Settings::AddRoundedBlock(content);

	auto addSlider = [&](const QString &title, const char *key, int min, int max, int def) {
		auto titleLabel = Ui::CreateChild<Ui::FlatLabel>(block2, title, st::defaultFlatLabel);
		block2->add(object_ptr<Ui::FlatLabel>::fromRaw(titleLabel), QMargins(22, 10, 22, 0));

		auto sliderWithLabel = Settings::MakeSliderWithLabel(
			block2,
			st::settingsScale,
			st::settingsScaleLabel,
			15
		);
		auto sld = sliderWithLabel.slider;
		auto lbl = sliderWithLabel.label;
		
		int initial = Core::App().settings().readPref<int>(key, def);
		
		auto updateLabel = [lbl](int value) {
			lbl->setText(QString::number(value));
		};
		updateLabel(initial);
		
		int steps = max - min;
		sld->setPseudoDiscrete(
			steps,
			[min](int index) { return min + index; },
			initial,
			[=](int value) { updateLabel(value); },
			[=](int value) {
				Core::App().settings().writePref<int>(key, value);
				Window::Theme::ApplyMelowGramModifiers();
				style::NotifyPaletteChanged();
			});

		block2->add(std::move(sliderWithLabel.widget), QMargins(22, 5, 22, 10));
	};

	addSlider(u"Particles Density"_q, "MelowGramParticlesCount", 10, 500, 100);
	addSlider(u"Particles Speed"_q, "MelowGramParticlesSpeed", 1, 100, 30);
	addSlider(u"Particles Size"_q, "MelowGramParticlesSize", 1, 50, 15);
	addSlider(u"Particles Connect Distance"_q, "MelowGramParticlesDistance", 10, 200, 70);

	Ui::ResizeFitChild(this, content);
}

} // namespace

Type MelowGramParticlesId() {
	return MelowGramParticles::Id();
}

} // namespace Settings
