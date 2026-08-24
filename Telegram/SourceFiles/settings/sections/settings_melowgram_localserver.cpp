#include "settings/sections/settings_melowgram_localserver.h"

#include "settings/settings_common_session.h"
#include "settings/settings_builder.h"
#include "settings/sections/settings_melowgram.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/buttons.h"
#include "ui/vertical_list.h"
#include "ui/ui_utility.h"
#include "styles/style_settings.h"
#include "styles/style_menu_icons.h"
#include "styles/style_boxes.h"
#include "window/window_session_controller.h"
#include "main/main_session.h"
#include "data/components/credits.h"
#include "core/credits_amount.h"
#include "melow/local_server.h"

namespace Settings {

LocalServerSection::LocalServerSection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent(controller);
}

rpl::producer<QString> LocalServerSection::title() {
	return rpl::single(u"Local Server"_q);
}

void LocalServerSection::setupContent(not_null<Window::SessionController*> controller) {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto &localServer = Melow::LocalServer::Instance();
	const auto &stButton = Settings::GetRoundedButtonStyle();
	const auto blockMargins = style::margins(26, 14, 26, 18);

	const auto paddedInputStyle = Ui::AttachAsChild(content, [=] {
		auto result = st::defaultInputField;
		result.textMargins = style::margins(16, 8, 16, 8);
		result.placeholderMargins = style::margins(16, 8, 16, 8);
		result.placeholderScale = 0.;
		result.heightMin = 36;
		return result;
	}());

	const auto nftItemStyle = Ui::AttachAsChild(content, [=] {
		auto result = st::defaultSettingsButton;
		result.height = 28;
		result.padding = style::margins(16, 4, 16, 4);
		return result;
	}());

	// 1. Local Server Main Card
	auto mainCard = Settings::AddRoundedBlock(content);

	const auto enableBtn = Settings::AddButtonWithSvgIcon(
		mainCard,
		rpl::single(u"Enable Local Server"_q),
		stButton,
		u":/gui/melow/melowgui/localserver/enable.svg"_q
	);
	enableBtn->toggleOn(rpl::single(localServer.isEnabled()));

	auto optionsWrap = mainCard->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			mainCard,
			object_ptr<Ui::VerticalLayout>(mainCard)
		)
	);
	optionsWrap->toggleOn(enableBtn->toggledValue());

	enableBtn->toggledValue()
		| rpl::filter([&localServer](bool val) { return val != localServer.isEnabled(); })
		| rpl::on_next([&localServer, controller](bool val) {
			localServer.setEnabled(val);
			if (val) {
				controller->session().credits().apply(
					CreditsAmount(localServer.stars()));
			} else {
				controller->session().credits().load(true);
			}
		}, optionsWrap->lifetime());

	auto optionsContent = optionsWrap->entity();

	auto inputWrap = optionsContent->add(
		object_ptr<Ui::VerticalLayout>(optionsContent),
		blockMargins
	);

	auto label = inputWrap->add(
		object_ptr<Ui::FlatLabel>(
			inputWrap,
			u"Stars Balance"_q,
			st::defaultFlatLabel)
	);
	label->setTextColorOverride(st::windowBoldFg->c);

	Ui::AddSkip(inputWrap, 6);

	auto field = inputWrap->add(
		object_ptr<Ui::InputField>(
			inputWrap,
			*paddedInputStyle,
			rpl::single(QString()),
			QString::number(localServer.stars()))
	);

	const auto applyValue = [&localServer, controller](const QString &text) {
		bool ok = false;
		auto val = text.trimmed().toLongLong(&ok);
		if (ok && val >= 0) {
			localServer.setStars(val);
			if (localServer.isEnabled()) {
				controller->session().credits().apply(
					CreditsAmount(val));
			}
		}
	};

	field->changes()
		| rpl::on_next([field, applyValue] {
			applyValue(field->getTextWithTags().text);
		}, optionsWrap->lifetime());

	// 2. Anonymous Number Card
	auto anonCard = Settings::AddRoundedBlock(content);

	const auto anonBtn = Settings::AddButtonWithSvgIcon(
		anonCard,
		rpl::single(u"Anonymous Number (+888)"_q),
		stButton,
		u":/gui/melow/melowgui/localserver/number.svg"_q
	);
	anonBtn->toggleOn(rpl::single(localServer.isAnonymousNumberEnabled()));

	auto anonWrap = anonCard->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			anonCard,
			object_ptr<Ui::VerticalLayout>(anonCard)
		)
	);
	anonWrap->toggleOn(anonBtn->toggledValue());

	anonBtn->toggledValue()
		| rpl::filter([&localServer](bool val) { return val != localServer.isAnonymousNumberEnabled(); })
		| rpl::on_next([&localServer](bool val) {
			localServer.setAnonymousNumberEnabled(val);
		}, anonWrap->lifetime());

	auto anonContent = anonWrap->entity();
	auto anonInputWrap = anonContent->add(
		object_ptr<Ui::VerticalLayout>(anonContent),
		blockMargins
	);

	auto anonLabel = anonInputWrap->add(
		object_ptr<Ui::FlatLabel>(
			anonInputWrap,
			u"Fake Anonymous Phone Number"_q,
			st::defaultFlatLabel)
	);
	anonLabel->setTextColorOverride(st::windowBoldFg->c);

	Ui::AddSkip(anonInputWrap, 6);

	auto anonField = anonInputWrap->add(
		object_ptr<Ui::InputField>(
			anonInputWrap,
			*paddedInputStyle,
			rpl::single(QString()),
			localServer.anonymousNumber())
	);

	anonField->changes()
		| rpl::on_next([anonField, &localServer] {
			localServer.setAnonymousNumber(anonField->getTextWithTags().text.trimmed());
		}, anonWrap->lifetime());

	// 3. NFT Usernames Card
	auto nftCard = Settings::AddRoundedBlock(content);

	const auto nftBtn = Settings::AddButtonWithSvgIcon(
		nftCard,
		rpl::single(u"NFT Usernames"_q),
		stButton,
		u":/gui/melow/melowgui/localserver/user.svg"_q
	);
	nftBtn->toggleOn(rpl::single(localServer.isNftUsernamesEnabled()));

	auto nftWrap = nftCard->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			nftCard,
			object_ptr<Ui::VerticalLayout>(nftCard)
		)
	);
	nftWrap->toggleOn(nftBtn->toggledValue());

	nftBtn->toggledValue()
		| rpl::filter([&localServer](bool val) { return val != localServer.isNftUsernamesEnabled(); })
		| rpl::on_next([&localServer](bool val) {
			localServer.setNftUsernamesEnabled(val);
		}, nftWrap->lifetime());

	auto nftContent = nftWrap->entity();
	auto nftInputWrap = nftContent->add(
		object_ptr<Ui::VerticalLayout>(nftContent),
		blockMargins
	);

	auto nftLabel = nftInputWrap->add(
		object_ptr<Ui::FlatLabel>(
			nftInputWrap,
			u"Add NFT Username"_q,
			st::defaultFlatLabel)
	);
	nftLabel->setTextColorOverride(st::windowBoldFg->c);

	Ui::AddSkip(nftInputWrap, 6);

	auto nftAddField = nftInputWrap->add(
		object_ptr<Ui::InputField>(
			nftInputWrap,
			*paddedInputStyle,
			rpl::single(QString()))
	);

	Ui::AddSkip(nftInputWrap, 8);

	auto nftListWrap = nftInputWrap->add(
		object_ptr<Ui::VerticalLayout>(nftInputWrap)
	);

	const auto refreshNftList = [nftListWrap, &localServer, nftItemStyle] {
		nftListWrap->clear();
		for (const auto &u : localServer.nftUsernames()) {
			auto row = nftListWrap->add(
				object_ptr<Ui::SettingsButton>(
					nftListWrap,
					rpl::single(u"@"_q + u + u"   ✕"_q),
					*nftItemStyle),
				style::margins(0, 2, 0, 2)
			);
			row->setClickedCallback([&localServer, u] {
				localServer.removeNftUsername(u);
			});
		}
	};

	localServer.profileChanged()
		| rpl::on_next([=] {
			refreshNftList();
		}, nftWrap->lifetime());

	nftAddField->submits()
		| rpl::on_next([nftAddField, &localServer](auto) {
			const auto text = nftAddField->getTextWithTags().text.trimmed();
			if (!text.isEmpty()) {
				localServer.addNftUsername(text);
				nftAddField->setText(QString());
			}
		}, nftWrap->lifetime());

	refreshNftList();

	Ui::AddSkip(content);
	Ui::AddSkip(content);
	Ui::ResizeFitChild(this, content);
}

Type LocalServerId() {
	return LocalServerSection::Id();
}

} // namespace Settings
