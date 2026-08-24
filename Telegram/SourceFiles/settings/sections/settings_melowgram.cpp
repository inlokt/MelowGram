// MelowGram main menu

#include "settings/sections/settings_melowgram.h"

#include "settings/settings_common_session.h"
#include "settings/settings_builder.h"
#include "settings/sections/settings_main.h"
#include "ui/wrap/vertical_layout.h"
#include "lang/lang_keys.h"
#include "styles/style_settings.h"
#include "styles/style_menu_icons.h"
#include "styles/style_boxes.h"

#include "settings/sections/settings_melowgram_theme.h"
#include "settings/sections/settings_melowgram_particles.h"
#include "settings/sections/settings_melowgram_localserver.h"
#include "settings/sections/settings_melowgram_plugins.h"
#include "settings/sections/settings_other.h"
#include "core/click_handler_types.h"
#include "ui/widgets/checkbox.h"
#include "ui/vertical_list.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "window/window_session_controller.h"
#include "ui/rp_widget.h"
#include "ui/widgets/labels.h"
#include "ui/unread_badge.h"
#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QtGui/QPainterPath>
#include <QtSvg/QSvgRenderer>

namespace Settings {

not_null<Ui::VerticalLayout*> AddRoundedBlock(not_null<Ui::VerticalLayout*> container) {
	auto wrap = container->add(
		object_ptr<Ui::RpWidget>(container),
		style::margins(10, 10, 10, 10));
	
	auto inner = Ui::CreateChild<Ui::VerticalLayout>(wrap);
	wrap->paintRequest() | rpl::on_next([wrap] {
		QPainter p(wrap);
		p.setRenderHint(QPainter::Antialiasing);
		p.setPen(Qt::NoPen);
		QColor bg = st::windowBgOver->c;
		bg.setAlpha(255);
		p.setBrush(bg);
		p.drawRoundedRect(wrap->rect(), st::boxRadius, st::boxRadius);
	}, wrap->lifetime());
	
	wrap->widthValue() | rpl::on_next([inner](int w) {
		inner->resizeToWidth(w - 8);
		inner->moveToLeft(4, 4);
	}, wrap->lifetime());
	
	inner->heightValue() | rpl::on_next([wrap](int h) {
		wrap->resize(wrap->width(), h + 8);
	}, inner->lifetime());
	
	return inner;
}

const style::SettingsButton &GetRoundedButtonStyle() {
	static style::SettingsButton stButton;
	static bool stButtonInited = false;
	if (!stButtonInited) {
		stButton = st::settingsButton;
		stButton.textBg = st::transparent;
		stButton.textBgOver = st::windowBgRipple;
		stButtonInited = true;
	}
	return stButton;
}

const style::SettingsButton &GetRoundedButtonNoIconStyle() {
	static style::SettingsButton stButton;
	static bool stButtonInited = false;
	if (!stButtonInited) {
		stButton = st::settingsButtonNoIcon;
		stButton.textBg = st::transparent;
		stButton.textBgOver = st::windowBgRipple;
		stButtonInited = true;
	}
	return stButton;
}

void AttachMelowSvgIcon(
		not_null<Ui::SettingsButton*> button,
		const style::SettingsButton &st,
		const QString &svgResourcePath,
		int iconSize) {
	struct SvgIconWidget {
		SvgIconWidget(QWidget *parent, const QString &path, int size)
		: widget(parent), resourcePath(path), size(size) {
		}
		Ui::RpWidget widget;
		QString resourcePath;
		int size;
	};

	const auto icon = button->lifetime().make_state<SvgIconWidget>(
		button.get(),
		svgResourcePath,
		iconSize);
	icon->widget.setAttribute(Qt::WA_TransparentForMouseEvents);
	icon->widget.resize(iconSize, iconSize);
	icon->widget.show();

	button->sizeValue()
		| rpl::on_next([=, left = st.iconLeft](QSize size) {
			icon->widget.moveToLeft(
				left,
				(size.height() - icon->widget.height()) / 2,
				size.width());
		}, icon->widget.lifetime());

	icon->widget.paintRequest()
		| rpl::on_next([=] {
			auto p = QPainter(&icon->widget);
			p.setRenderHint(QPainter::Antialiasing, true);
			p.setRenderHint(QPainter::SmoothPixmapTransform, true);

			const auto ratio = style::DevicePixelRatio();
			const auto pxSize = std::max(int(std::round(iconSize * ratio)), 1);

			static base::flat_map<std::pair<QString, int>, QImage> cache;
			const auto key = std::make_pair(icon->resourcePath, pxSize);
			auto it = cache.find(key);
			if (it == cache.end()) {
				if (icon->resourcePath.contains(u"logotype2"_q)) {
					it = cache.emplace(key, MelowBadge::GetBadgeForSize(pxSize, pxSize)).first;
				} else {
					QImage img(pxSize, pxSize, QImage::Format_ARGB32_Premultiplied);
					img.fill(Qt::transparent);
					QSvgRenderer renderer(icon->resourcePath);
					if (renderer.isValid()) {
						QPainter ip(&img);
						ip.setRenderHint(QPainter::Antialiasing, true);
						ip.setRenderHint(QPainter::SmoothPixmapTransform, true);
						renderer.render(&ip, QRectF(0, 0, pxSize, pxSize));
					}
					it = cache.emplace(key, std::move(img)).first;
				}
			}

			const auto &baseImg = it->second;
			if (!baseImg.isNull()) {
				if (icon->resourcePath.contains(u"logotype2"_q)) {
					p.drawImage(QRect(0, 0, iconSize, iconSize), baseImg);
				} else {
					QImage colored = baseImg;
					QPainter cp(&colored);
					cp.setCompositionMode(QPainter::CompositionMode_SourceIn);
					cp.fillRect(colored.rect(), st::menuIconFg->c);
					cp.end();

					p.drawImage(QRect(0, 0, iconSize, iconSize), colored);
				}
			}
		}, icon->widget.lifetime());
}

not_null<Ui::SettingsButton*> AddButtonWithSvgIcon(
		not_null<Ui::VerticalLayout*> container,
		rpl::producer<QString> text,
		const style::SettingsButton &st,
		const QString &svgResourcePath,
		int iconSize) {
	const auto button = container->add(
		object_ptr<Ui::SettingsButton>(container, std::move(text), st));
	AttachMelowSvgIcon(button, st, svgResourcePath, iconSize);
	return button;
}

not_null<Ui::SettingsButton*> AddButtonWithSvgLabel(
		not_null<Ui::VerticalLayout*> container,
		rpl::producer<QString> text,
		rpl::producer<QString> label,
		const style::SettingsButton &st,
		const QString &svgResourcePath,
		int iconSize) {
	const auto button = AddButtonWithSvgIcon(
		container,
		rpl::duplicate(text),
		st,
		svgResourcePath,
		iconSize);
	CreateRightLabel(button, std::move(label), st, std::move(text));
	return button;
}

namespace {

class MelowGram : public Section<MelowGram> {
public:
	MelowGram(QWidget *parent, not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

MelowGram::MelowGram(QWidget *parent, not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> MelowGram::title() {
	return tr::lng_settings_section_melowgram();
}

void MelowGram::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	// 1. Avatar block
	auto avatarWrap = content->add(
		object_ptr<Ui::RpWidget>(content),
		style::margins(0, 20, 0, 10));
	avatarWrap->resize(content->width(), 160);
	
	avatarWrap->paintRequest() | rpl::on_next([avatarWrap] {
		QPainter p(avatarWrap);
		p.setRenderHint(QPainter::Antialiasing);
		const auto targetRect = QRect((avatarWrap->width() - 80) / 2, 10, 80, 80);
		MelowBadge::Paint(p, targetRect);
		
		p.setFont(st::semiboldFont);
		p.setPen(st::windowFg);
		p.drawText(QRectF(0, 105, avatarWrap->width(), 20), "MelowGram", QTextOption(Qt::AlignCenter));
		
		p.setFont(st::normalFont);
		p.setPen(st::windowSubTextFg);
		p.drawText(QRectF(0, 130, avatarWrap->width(), 20), "2.0.0 Release", QTextOption(Qt::AlignCenter));
	}, avatarWrap->lifetime());
	
	avatarWrap->widthValue() | rpl::on_next([avatarWrap](int w) {
		avatarWrap->resize(w, 160);
	}, avatarWrap->lifetime());

	// 2. Main functions card
	auto mainCard = AddRoundedBlock(content);
	
	const auto &stButton = GetRoundedButtonStyle();
	
	auto btnTheme = Settings::AddButtonWithSvgIcon(mainCard, rpl::single(u"Theme"_q), stButton, u":/gui/melow/melowgui/main/theme.svg"_q);
	btnTheme->setClickedCallback([=] { controller()->showSettings(MelowGramThemeId()); });
	
	auto btnParticles = Settings::AddButtonWithSvgIcon(mainCard, rpl::single(u"Particles"_q), stButton, u":/gui/melow/melowgui/main/particles.svg"_q);
	btnParticles->setClickedCallback([=] { controller()->showSettings(MelowGramParticlesId()); });
	
	auto btnLocalServer = Settings::AddButtonWithSvgIcon(mainCard, rpl::single(u"Local Server"_q), stButton, u":/gui/melow/melowgui/main/localserver.svg"_q);
	btnLocalServer->setClickedCallback([=] { controller()->showSettings(LocalServerId()); });

	auto btnPlugins = Settings::AddButtonWithSvgIcon(mainCard, rpl::single(u"Plugins"_q), stButton, u":/gui/melow/melowgui/main/plugin.svg"_q);
	btnPlugins->setClickedCallback([=] { controller()->showSettings(MelowGramPluginsId()); });

	auto btnOther = Settings::AddButtonWithSvgIcon(mainCard, rpl::single(u"Other"_q), stButton, u":/gui/melow/melowgui/main/other.svg"_q);
	btnOther->setClickedCallback([=] { controller()->showSettings(OtherId()); });

	// 3. Socials card
	auto socialsCard = AddRoundedBlock(content);
	auto btnChannel = Settings::AddButtonWithSvgLabel(socialsCard, rpl::single(u"Официальный канал"_q), rpl::single(u"@melowdesktop"_q), stButton, u":/gui/melow/melowgui/main/channel.svg"_q);
	btnChannel->setClickedCallback([=] { UrlClickHandler::Open(u"https://t.me/melowdesktop"_q); });

	auto btnSource = Settings::AddButtonWithSvgLabel(socialsCard, rpl::single(u"Исходный код"_q), rpl::single(u"GitHub"_q), stButton, u":/gui/melow/melowgui/main/github.svg"_q);
	btnSource->setClickedCallback([=] { UrlClickHandler::Open(u"https://github.com/inlokt/melowgram"_q); });

	// 4. Ads card
	auto adsCard = AddRoundedBlock(content);
	auto adsButton = new Ui::RippleButton(adsCard, st::defaultRippleAnimation);
	auto adsWrap = adsCard->add(object_ptr<Ui::RpWidget>::fromRaw(adsButton));
	
	auto adsTitle = Ui::CreateChild<Ui::FlatLabel>(adsWrap, u"MelowGram"_q, st::defaultFlatLabel);
	auto adsLabel = Ui::CreateChild<Ui::FlatLabel>(adsWrap, u"Када то тут будет реклама >.<"_q, st::defaultFlatLabel);
	adsLabel->setTextColorOverride(st::windowSubTextFg->c);
	
	adsWrap->widthValue() | rpl::on_next([adsWrap, adsTitle, adsLabel](int w) {
		adsTitle->moveToLeft(22, 12);
		adsLabel->moveToLeft(22, 12 + adsTitle->height() + 2);
		adsWrap->resize(w, 12 + adsTitle->height() + 2 + adsLabel->height() + 12);
	}, adsWrap->lifetime());
	
	Ui::AddSkip(content);
	Ui::AddSkip(content);

	Ui::ResizeFitChild(this, content);
}

} // namespace

Type MelowGramId() {
	return MelowGram::Id();
}

} // namespace Settings
