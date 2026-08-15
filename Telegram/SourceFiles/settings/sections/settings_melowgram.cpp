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
#include "settings/sections/settings_other.h"
#include "core/click_handler_types.h"
#include "ui/widgets/checkbox.h"
#include "ui/vertical_list.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "window/window_session_controller.h"
#include "ui/rp_widget.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/buttons.h"
#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QtGui/QPainterPath>

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
		QImage img(":/gui/melow/avatar.jpg");
		if (!img.isNull()) {
			img = img.scaled(80, 80, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
			QPainterPath path;
			path.addEllipse(QRectF((avatarWrap->width() - 80) / 2.0, 10.0, 80.0, 80.0));
			p.setClipPath(path);
			p.drawImage(QRect((avatarWrap->width() - 80) / 2, 10, 80, 80), img);
		}
		p.setClipping(false);
		
		p.setFont(st::semiboldFont);
		p.setPen(st::windowFg);
		p.drawText(QRectF(0, 105, avatarWrap->width(), 20), "MelowGram", QTextOption(Qt::AlignCenter));
		
		p.setFont(st::normalFont);
		p.setPen(st::windowSubTextFg);
		p.drawText(QRectF(0, 130, avatarWrap->width(), 20), "1.0 Beta", QTextOption(Qt::AlignCenter));
	}, avatarWrap->lifetime());
	
	avatarWrap->widthValue() | rpl::on_next([avatarWrap](int w) {
		avatarWrap->resize(w, 160);
	}, avatarWrap->lifetime());

	// 2. Main functions card
	auto mainCard = AddRoundedBlock(content);
	
	const auto &stButton = GetRoundedButtonStyle();
	
	auto btnTheme = Settings::AddButtonWithIcon(mainCard, rpl::single(u"Theme"_q), stButton, { &st::menuIconChangeColors });
	btnTheme->setClickedCallback([=] { controller()->showSettings(MelowGramThemeId()); });
	
	auto btnParticles = Settings::AddButtonWithIcon(mainCard, rpl::single(u"Particles"_q), stButton, { &st::menuIconPremium });
	btnParticles->setClickedCallback([=] { controller()->showSettings(MelowGramParticlesId()); });
	
	auto btnOther = Settings::AddButtonWithIcon(mainCard, rpl::single(u"Other"_q), stButton, { &st::menuIconChatBubble });
	btnOther->setClickedCallback([=] { controller()->showSettings(OtherId()); });

	// 3. Socials card
	auto socialsCard = AddRoundedBlock(content);
	auto btnChannel = Settings::AddButtonWithLabel(socialsCard, rpl::single(u"Официальный канал"_q), rpl::single(u"@melowdesktop"_q), stButton, { &st::menuIconChannel });
	btnChannel->setClickedCallback([=] { UrlClickHandler::Open(u"https://t.me/melowdesktop"_q); });

	auto btnSource = Settings::AddButtonWithLabel(socialsCard, rpl::single(u"Исходный код"_q), rpl::single(u"GitHub"_q), stButton, { &st::menuIconLink });
	btnSource->setClickedCallback([=] { UrlClickHandler::Open(u"https://github.com/inlokr"_q); });

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
