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
#include "ui/painter.h"
#include "core/file_utilities.h"
#include "core/application.h"
#include "storage/localstorage.h"
#include <QPainterPath>
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

#include "ui/ui_utility.h"

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

	const auto gifEnabled = Settings::AddButtonWithSvgIcon(
		block1,
		rpl::single(u"GIF BackGround"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/theme/gif_back.svg"_q
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
		Core::App().saveSettingsDelayed();
		controller->widget()->reloadMelowGramGif();
		Window::Theme::ApplyMelowGramModifiers();
		controller->widget()->updateWindowTransparency();
		Ui::ForceFullRepaint(controller->widget());
	}, optionsWrap->lifetime());
	
	auto optionsContent = optionsWrap->entity();

	const auto gifButton = Settings::AddButtonWithSvgIcon(
		optionsContent,
		rpl::single(u"Choose GIF file..."_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/theme/choose.svg"_q
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
					Core::App().saveSettingsDelayed();
					controller->widget()->reloadMelowGramGif();
					Window::Theme::ApplyMelowGramModifiers();
					controller->widget()->updateWindowTransparency();
					Ui::ForceFullRepaint(controller->widget());
				}
			});
	});
	
	auto block2 = Settings::AddRoundedBlock(content);
	
	const auto blurEnabled = Settings::AddButtonWithSvgIcon(
		block2,
		rpl::single(u"Blur Behind Window (Windows 10+)"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/theme/blur.svg"_q
	);
	blurEnabled->toggleOn(rpl::single(Core::App().settings().readPref<bool>("MelowGramBlur", false)));
	
	std::move(blurEnabled->toggledValue()) | rpl::filter([](bool value) {
		return value != Core::App().settings().readPref<bool>("MelowGramBlur", false);
	}) | rpl::on_next([=](bool value) {
		Core::App().settings().writePref<bool>("MelowGramBlur", value);
		Core::App().saveSettingsDelayed();
		Window::Theme::ApplyMelowGramModifiers();
		controller->widget()->updateWindowTransparency();
		Ui::ForceFullRepaint(controller->widget());
	}, blurEnabled->lifetime());

	auto blackoutWrap = content->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			content,
			object_ptr<Ui::VerticalLayout>(content)
		)
	);

	auto block3 = Settings::AddRoundedBlock(blackoutWrap->entity());
	
	auto titleLabel = Ui::CreateChild<Ui::FlatLabel>(block3, u"Blackout"_q, st::defaultFlatLabel);
	block3->add(object_ptr<Ui::FlatLabel>::fromRaw(titleLabel), QMargins(22, 10, 22, 0));
	
	int initialValue = std::clamp(Core::App().settings().readPref<int>("MelowGramBlackout", 50), 15, 100);

	auto slider = Settings::MakeSliderWithLabel(
		block3,
		st::settingsScale,
		st::settingsScaleLabel,
		15
	);
	
	auto sliderLabel = slider.label;
	auto sliderWidget = slider.slider;
	
	sliderWidget->setChangeProgressCallback([=](float64 value) {
		const int pct = std::clamp(int(std::round(value * 100)), 15, 100);
		Core::App().settings().writePref<int>(
			"MelowGramBlackout",
			pct);
		sliderLabel->setText(QString::number(pct) + "%");
		Window::Theme::ApplyMelowGramModifiers();
		controller->widget()->updateWindowTransparency();
		Ui::ForceFullRepaint(controller->widget());
	});
	sliderWidget->setChangeFinishedCallback([=](float64 value) {
		Core::App().saveSettingsDelayed();
	});
	sliderWidget->setValue(initialValue / 100.0);
	sliderLabel->setText(QString::number(initialValue) + "%");

	block3->add(std::move(slider.widget), QMargins(22, 5, 22, 10));

	blackoutWrap->toggleOn(
		rpl::combine(
			gifEnabled->toggledValue(),
			blurEnabled->toggledValue()
		) | rpl::map([](bool g, bool b) { return g || b; })
	);

	auto effectsBlock = Settings::AddRoundedBlock(content);

	const auto effectsToggle = Settings::AddButtonWithSvgIcon(
		effectsBlock,
		rpl::single(u"Effects"_q),
		Settings::GetRoundedButtonStyle(),
		u":/gui/melow/melowgui/theme/effect.svg"_q
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

	auto iconBlock = Settings::AddRoundedBlock(content);
	auto iconTitle = Ui::CreateChild<Ui::FlatLabel>(iconBlock, u"App Icon"_q, st::defaultFlatLabel);
	iconBlock->add(object_ptr<Ui::FlatLabel>::fromRaw(iconTitle), QMargins(22, 10, 22, 10));

	const auto currentAppIcon = std::make_shared<int>(
		Core::App().settings().readPref<int>("MelowGramAppIcon", 0)
	);

	struct IconOption {
		int id;
		QString title;
		QString imagePath;
	};

	const std::vector<IconOption> options = {
		{ 0, u"Default"_q, u":/gui/melow/avatar.png"_q },
		{ 1, u"Monochrome"_q, u":/gui/melow/monochrome.png"_q },
		{ 2, u"Anime"_q, u":/gui/melow/anime_avatar.png"_q },
	};

	auto cardsContainer = iconBlock->add(
		object_ptr<Ui::RpWidget>(iconBlock)
	);

	const auto cardHeight = 118;
	const auto cardSkip = 10;
	const auto padding = 16;

	cardsContainer->resize(cardsContainer->width(), cardHeight + 10);

	std::vector<Ui::RippleButton*> cardButtons;
	for (size_t i = 0; i < options.size(); ++i) {
		const auto opt = options[i];
		auto btn = Ui::CreateChild<Ui::RippleButton>(cardsContainer, st::defaultRippleAnimation);
		btn->resize(1, cardHeight);

		btn->paintRequest() | rpl::on_next([=] {
			Painter p(btn);
			p.setRenderHint(QPainter::Antialiasing, true);
			p.setRenderHint(QPainter::SmoothPixmapTransform, true);

			const auto cardWidth = btn->width();
			const auto selected = (*currentAppIcon == opt.id);
			const auto rect = QRect(2, 2, cardWidth - 4, cardHeight - 4);
			const auto radius = 10.0;

			p.setPen(Qt::NoPen);
			if (selected) {
				p.setBrush(st::windowBgOver);
			} else {
				p.setBrush(st::windowBg);
			}
			p.drawRoundedRect(rect, radius, radius);

			if (selected) {
				p.setPen(QPen(st::windowActiveTextFg, 2.0));
				p.setBrush(Qt::NoBrush);
				p.drawRoundedRect(rect, radius, radius);
			} else {
				p.setPen(QPen(st::windowBgOver, 1.0));
				p.setBrush(Qt::NoBrush);
				p.drawRoundedRect(rect, radius, radius);
			}

			static base::flat_map<QString, QImage> roundedImages;
			auto it = roundedImages.find(opt.imagePath);
			if (it == roundedImages.end()) {
				QImage img(opt.imagePath);
				if (!img.isNull()) {
					img = img.scaled(58 * style::DevicePixelRatio(), 58 * style::DevicePixelRatio(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
					QImage rounded(img.size(), QImage::Format_ARGB32_Premultiplied);
					rounded.fill(Qt::transparent);
					QPainter rp(&rounded);
					rp.setRenderHint(QPainter::Antialiasing, true);
					QPainterPath path;
					path.addRoundedRect(QRectF(0, 0, img.width(), img.height()), 12 * style::DevicePixelRatio(), 12 * style::DevicePixelRatio());
					rp.setClipPath(path);
					rp.drawImage(0, 0, img);
					rp.end();
					it = roundedImages.emplace(opt.imagePath, std::move(rounded)).first;
				}
			}

			const auto imageX = (cardWidth - 58) / 2;
			const auto imageY = 12;
			if (it != roundedImages.end() && !it->second.isNull()) {
				p.drawImage(QRect(imageX, imageY, 58, 58), it->second);
			}

			if (selected) {
				p.setPen(Qt::NoPen);
				p.setBrush(st::windowActiveTextFg);
				p.drawEllipse(QRect(imageX + 58 - 16, imageY + 58 - 16, 14, 14));
				p.setPen(QPen(st::windowBg, 1.5));
				p.drawLine(imageX + 58 - 13, imageY + 58 - 9, imageX + 58 - 10, imageY + 58 - 6);
				p.drawLine(imageX + 58 - 10, imageY + 58 - 6, imageX + 58 - 5, imageY + 58 - 11);
			}

			p.setFont(selected ? st::semiboldFont : st::normalFont);
			p.setPen(selected ? st::windowActiveTextFg : st::windowFg);
			p.drawText(QRect(0, imageY + 58 + 8, cardWidth, 22), Qt::AlignHCenter | Qt::AlignTop, opt.title);
		}, btn->lifetime());

		btn->setClickedCallback([=] {
			if (*currentAppIcon != opt.id) {
				*currentAppIcon = opt.id;
				Core::App().settings().writePref<int>("MelowGramAppIcon", opt.id);
				Core::App().saveSettingsDelayed();
				Local::writeSettings();
				cardsContainer->update();

				controller->show(Ui::MakeConfirmBox({
					.text = u"Для применения новой иконки приложения требуется перезапуск. Перезапустить сейчас?"_q,
					.confirmed = [=] { Core::Restart(); },
					.confirmText = u"Перезапустить"_q,
					.cancelText = u"Позже"_q,
				}));
			}
		});

		cardButtons.push_back(btn);
	}

	cardsContainer->widthValue(
	) | rpl::on_next([buttons = std::move(cardButtons), padding, cardSkip, cardHeight](int fullWidth) {
		if (buttons.empty() || fullWidth <= 0) {
			return;
		}
		const auto count = int(buttons.size());
		const auto avail = fullWidth - padding * 2 - (count - 1) * cardSkip;
		if (avail <= 0) {
			return;
		}
		const auto singleWidth = avail / count;
		auto left = padding;
		for (const auto btn : buttons) {
			btn->resize(singleWidth, cardHeight);
			btn->moveToLeft(left, 0);
			left += singleWidth + cardSkip;
		}
	}, cardsContainer->lifetime());

	Ui::ResizeFitChild(this, content);
}

Type MelowGramThemeId() {
	return MelowGramTheme::Id();
}

} // namespace Settings
