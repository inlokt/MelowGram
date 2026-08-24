// MelowGram Plugins Settings

#include "settings/sections/settings_melowgram_plugins.h"

#include "settings/settings_common_session.h"
#include "settings/settings_builder.h"
#include "settings/sections/settings_melowgram.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/buttons.h"
#include "ui/vertical_list.h"
#include "ui/ui_utility.h"
#include "ui/style/style_core.h"
#include "ui/layers/box_content.h"
#include "ui/painter.h"
#include "ui/effects/ripple_animation.h"
#include "lang/lang_keys.h"
#include "styles/style_settings.h"
#include "styles/style_menu_icons.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "melow/plugin_engine.h"
#include "window/window_session_controller.h"
#include "core/file_utilities.h"
#include "settings.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QtSvg/QSvgRenderer>

namespace Settings {
namespace {

struct PluginMeta {
	QString fileName;
	QString filePath;
	QString title;
	QString description;
	QString author;
	QString authorLink;
	QString version;
	QString icon;
};

[[nodiscard]] QString GetPluginsDirectory() {
	auto dirPath = cWorkingDir() + u"plugins"_q;
	QDir().mkpath(dirPath);
	return dirPath;
}

[[nodiscard]] PluginMeta ParsePluginFile(const QFileInfo &fileInfo) {
	auto meta = PluginMeta();
	meta.fileName = fileInfo.fileName();
	meta.filePath = fileInfo.absoluteFilePath();
	meta.title = fileInfo.baseName();
	meta.version = u"unkown"_q;

	QFile file(meta.filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return meta;
	}

	const auto content = QString::fromUtf8(file.read(16384));
	file.close();

	const auto titleRegexes = {
		QRegularExpression(u"//\\s*@name\\s+([^\r\n]+)"_q),
		QRegularExpression(u"(?:const|let|var)?\\s*title\\s*[:=]\\s*[\"']?([^\"';\r\n]+)[\"']?"_q, QRegularExpression::CaseInsensitiveOption),
		QRegularExpression(u"(?:const|let|var)?\\s*name\\s*[:=]\\s*[\"']?([^\"';\r\n]+)[\"']?"_q, QRegularExpression::CaseInsensitiveOption),
	};
	for (const auto &regex : titleRegexes) {
		const auto match = regex.match(content);
		if (match.hasMatch()) {
			const auto parsed = match.captured(1).trimmed();
			if (!parsed.isEmpty()) {
				meta.title = parsed;
				break;
			}
		}
	}

	const auto versionRegexes = {
		QRegularExpression(u"//\\s*@version\\s+([^\r\n]+)"_q),
		QRegularExpression(u"(?:const|let|var)?\\s*version\\s*[:=]\\s*[\"']?([^\"';\r\n]+)[\"']?"_q, QRegularExpression::CaseInsensitiveOption),
	};
	for (const auto &regex : versionRegexes) {
		const auto match = regex.match(content);
		if (match.hasMatch()) {
			const auto parsed = match.captured(1).trimmed();
			if (!parsed.isEmpty()) {
				meta.version = parsed;
				break;
			}
		}
	}

	const auto authorRegexes = {
		QRegularExpression(u"//\\s*@author\\s+([^\r\n]+)"_q),
		QRegularExpression(u"(?:const|let|var)?\\s*author\\s*[:=]\\s*[\"']?([^\"';\r\n]+)[\"']?"_q, QRegularExpression::CaseInsensitiveOption),
	};
	for (const auto &regex : authorRegexes) {
		const auto match = regex.match(content);
		if (match.hasMatch()) {
			const auto parsed = match.captured(1).trimmed();
			if (!parsed.isEmpty()) {
				meta.author = parsed;
				break;
			}
		}
	}

	const auto linkRegexes = {
		QRegularExpression(u"//\\s*@(?:author_link|authorLink|link|channel)\\s+([^\r\n]+)"_q),
		QRegularExpression(u"(?:const|let|var)?\\s*(?:author_link|authorLink|link|channel)\\s*[:=]\\s*[\"']?([^\"';\r\n]+)[\"']?"_q, QRegularExpression::CaseInsensitiveOption),
	};
	for (const auto &regex : linkRegexes) {
		const auto match = regex.match(content);
		if (match.hasMatch()) {
			const auto parsed = match.captured(1).trimmed();
			if (!parsed.isEmpty()) {
				meta.authorLink = parsed;
				break;
			}
		}
	}

	const auto descRegexes = {
		QRegularExpression(u"//\\s*@description\\s+([^\r\n]+)"_q),
		QRegularExpression(u"(?:const|let|var)?\\s*description\\s*[:=]\\s*[\"']?([^\"';\r\n]+)[\"']?"_q, QRegularExpression::CaseInsensitiveOption),
	};
	for (const auto &regex : descRegexes) {
		const auto match = regex.match(content);
		if (match.hasMatch()) {
			const auto parsed = match.captured(1).trimmed();
			if (!parsed.isEmpty()) {
				meta.description = parsed;
				break;
			}
		}
	}

	const auto iconRegexes = {
		QRegularExpression(u"//\\s*@icon\\s+([^\r\n]+)"_q),
		QRegularExpression(u"(?:const|let|var)?\\s*icon\\s*[:=]\\s*[\"']?([a-zA-Z0-9_./\\\\-]+)[\"']?"_q, QRegularExpression::CaseInsensitiveOption),
	};
	for (const auto &regex : iconRegexes) {
		const auto match = regex.match(content);
		if (match.hasMatch()) {
			auto rawIcon = match.captured(1).trimmed();
			rawIcon.remove('\"').remove('\'').remove(';');
			meta.icon = rawIcon.trimmed();
			break;
		}
	}

	return meta;
}

[[nodiscard]] std::vector<PluginMeta> ScanPlugins() {
	auto result = std::vector<PluginMeta>();
	const auto dirPath = GetPluginsDirectory();
	const auto dir = QDir(dirPath);
	const auto entries = dir.entryInfoList({ u"*.js"_q }, QDir::Files, QDir::Name);

	result.reserve(entries.size());
	for (const auto &entry : entries) {
		result.push_back(ParsePluginFile(entry));
	}
	return result;
}

[[nodiscard]] QImage LoadPluginIconImage(const QString &iconName) {
	constexpr auto kIconSize = 20;
	const auto ratio = style::DevicePixelRatio();
	const auto pixelSize = kIconSize * ratio;

	auto result = QImage(pixelSize, pixelSize, QImage::Format_ARGB32_Premultiplied);
	result.setDevicePixelRatio(ratio);
	result.fill(Qt::transparent);

	QString customPath;
	if (!iconName.isEmpty()) {
		const auto cleanName = QString(iconName).remove('\"').remove('\'').remove(';').trimmed();
		const auto candidate = GetPluginsDirectory() + u"/"_q + cleanName;
		if (QFile::exists(candidate)) {
			customPath = candidate;
		}
	}

	if (!customPath.isEmpty()) {
		if (customPath.endsWith(u".svg"_q, Qt::CaseInsensitive)) {
			QSvgRenderer renderer(customPath);
			if (renderer.isValid()) {
				QPainter p(&result);
				p.setRenderHint(QPainter::Antialiasing, true);
				p.setRenderHint(QPainter::SmoothPixmapTransform, true);
				renderer.render(&p, QRectF(0, 0, kIconSize, kIconSize));
				return result;
			}
		} else {
			QImage img(customPath);
			if (!img.isNull()) {
				QPainter p(&result);
				p.setRenderHint(QPainter::Antialiasing, true);
				p.setRenderHint(QPainter::SmoothPixmapTransform, true);
				p.drawImage(QRect(0, 0, kIconSize, kIconSize), img);
				return result;
			}
		}
	}

	// Fallback to unknown.svg
	QFile svgFile(u":/gui/melow/badge_plugins_unknown.svg"_q);
	QByteArray svgData;
	if (svgFile.open(QIODevice::ReadOnly)) {
		svgData = svgFile.readAll();
		svgFile.close();
	}
	if (!svgData.isEmpty()) {
		QSvgRenderer renderer(svgData);
		if (renderer.isValid()) {
			QPainter p(&result);
			p.setRenderHint(QPainter::Antialiasing, true);
			p.setRenderHint(QPainter::SmoothPixmapTransform, true);
			renderer.render(&p, QRectF(0, 0, kIconSize, kIconSize));
			return result;
		}
	}

	return result;
}

void AttachCustomPluginIcon(
		not_null<Ui::SettingsButton*> button,
		const style::SettingsButton &st,
		const QImage &iconImage) {
	struct CustomIconWidget {
		CustomIconWidget(QWidget *parent, const QImage &img)
		: widget(parent), image(img) {
		}
		Ui::RpWidget widget;
		QImage image;
	};

	const auto icon = button->lifetime().make_state<CustomIconWidget>(
		button.get(),
		iconImage);
	icon->widget.setAttribute(Qt::WA_TransparentForMouseEvents);
	icon->widget.resize(20, 20);
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
			p.drawImage(0, 0, icon->image);
		}, icon->widget.lifetime());
}

static PluginMeta sCurrentPluginMeta;

class MelowGramPluginDetails : public Section<MelowGramPluginDetails> {
public:
	MelowGramPluginDetails(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent(not_null<Window::SessionController*> controller);

	PluginMeta _meta;
};

Type MelowGramPluginDetailsId(const PluginMeta &meta) {
	sCurrentPluginMeta = meta;
	return MelowGramPluginDetails::Id();
}

MelowGramPluginDetails::MelowGramPluginDetails(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller)
, _meta(sCurrentPluginMeta) {
	setupContent(controller);
}

rpl::producer<QString> MelowGramPluginDetails::title() {
	return rpl::single(_meta.title.isEmpty() ? _meta.fileName : _meta.title);
}

void MelowGramPluginDetails::setupContent(not_null<Window::SessionController*> controller) {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	const auto &stButton = Settings::GetRoundedButtonNoIconStyle();

	// 1. Version & Author Card
	auto mainCard = Settings::AddRoundedBlock(content);

	// Version
	const auto version = _meta.version.isEmpty() ? u"1.0.0"_q : _meta.version;
	auto versionBtn = Settings::AddButtonWithIcon(
		mainCard,
		rpl::single(u"Version: "_q + version),
		stButton,
		{});
	versionBtn->setAttribute(Qt::WA_TransparentForMouseEvents);

	// Author
	const auto authorName = _meta.author.isEmpty() ? u"Unknown"_q : _meta.author;
	const auto authorLink = _meta.authorLink.trimmed();

	if (!authorLink.isEmpty()) {
		auto authorBtn = Settings::AddButtonWithIcon(
			mainCard,
			rpl::single(u"Author: "_q + authorName + u" ("_q + authorLink + u")"_q),
			stButton,
			{});
		authorBtn->setClickedCallback([authorLink] {
			auto link = authorLink;
			if (link.startsWith('@')) {
				link = u"https://t.me/"_q + link.mid(1);
			} else if (!link.startsWith(u"http://"_q) && !link.startsWith(u"https://"_q) && !link.startsWith(u"tg://"_q)) {
				if (link.startsWith(u"t.me/"_q)) {
					link = u"https://"_q + link;
				} else {
					link = u"https://t.me/"_q + link;
				}
			}
			File::OpenUrl(link);
		});
	} else {
		auto authorBtn = Settings::AddButtonWithIcon(
			mainCard,
			rpl::single(u"Author: "_q + authorName),
			stButton,
			{});
		authorBtn->setAttribute(Qt::WA_TransparentForMouseEvents);
	}

	// 2. Description Card
	const auto descTextStr = _meta.description.isEmpty() ? u"No description provided."_q : _meta.description;
	auto descCard = Settings::AddRoundedBlock(content);
	auto descWrap = descCard->add(
		object_ptr<Ui::VerticalLayout>(descCard),
		style::margins(18, 14, 18, 14));
	
	auto descHeader = descWrap->add(
		object_ptr<Ui::FlatLabel>(
			descWrap,
			u"Description:"_q,
			st::defaultFlatLabel));
	descHeader->setTextColorOverride(st::windowActiveTextFg->c);

	Ui::AddSkip(descWrap, 8);

	auto descText = descWrap->add(
		object_ptr<Ui::FlatLabel>(
			descWrap,
			descTextStr,
			st::defaultFlatLabel));
	descText->setTextColorOverride(st::windowSubTextFg->c);

	Ui::AddSkip(content);
	Ui::AddSkip(content);

	Ui::ResizeFitChild(this, content);
}

class PluginInfoButton : public Ui::RippleButton {
public:
	PluginInfoButton(QWidget *parent)
	: Ui::RippleButton(parent, st::defaultRippleAnimation) {
		resize(28, 28);
		setCursor(style::cur_pointer);
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		paintRipple(p, 0, 0);

		const auto rect = this->rect();
		if (isOver()) {
			p.setRenderHint(QPainter::Antialiasing);
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgOver);
			p.drawEllipse(rect);
		}

		const auto iconSize = 22;
		const auto ratio = style::DevicePixelRatio();
		const auto pxSize = std::max(int(std::round(iconSize * ratio)), 1);

		static base::flat_map<int, QImage> cache;
		auto it = cache.find(pxSize);
		if (it == cache.end()) {
			QImage img(pxSize, pxSize, QImage::Format_ARGB32_Premultiplied);
			img.fill(Qt::transparent);
			QSvgRenderer renderer(u":/gui/melow/melowgui/plugins/info_plug.svg"_q);
			if (renderer.isValid()) {
				QPainter ip(&img);
				ip.setRenderHint(QPainter::Antialiasing, true);
				ip.setRenderHint(QPainter::SmoothPixmapTransform, true);
				renderer.render(&ip, QRectF(0, 0, pxSize, pxSize));
			}
			it = cache.emplace(pxSize, std::move(img)).first;
		}

		const auto &baseImg = it->second;
		if (!baseImg.isNull()) {
			QImage colored = baseImg;
			QPainter cp(&colored);
			cp.setCompositionMode(QPainter::CompositionMode_SourceIn);
			cp.fillRect(colored.rect(), isOver() ? st::menuIconFgOver->c : st::menuIconFg->c);
			cp.end();

			const auto x = (rect.width() - iconSize) / 2;
			const auto y = (rect.height() - iconSize) / 2;
			p.drawImage(QRect(x, y, iconSize, iconSize), colored);
		}
	}

	QImage prepareRippleMask() const override {
		return Ui::RippleAnimation::EllipseMask(size());
	}
};

void AttachPluginInfoButton(
		not_null<Ui::SettingsButton*> button,
		not_null<Window::SessionController*> controller,
		const PluginMeta &plugin) {
	const auto infoBtn = Ui::CreateChild<PluginInfoButton>(button.get());
	infoBtn->show();

	button->sizeValue()
		| rpl::on_next([=](QSize size) {
			infoBtn->moveToRight(
				56,
				(size.height() - infoBtn->height()) / 2,
				size.width());
		}, infoBtn->lifetime());

	infoBtn->setClickedCallback([controller, plugin] {
		controller->showSettings(MelowGramPluginDetailsId(plugin));
	});
}

} // namespace

MelowGramPlugins::MelowGramPlugins(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent(controller);
}

rpl::producer<QString> MelowGramPlugins::title() {
	return rpl::single(u"Plugins"_q);
}

void MelowGramPlugins::setupContent(not_null<Window::SessionController*> controller) {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	const auto &stButton = Settings::GetRoundedButtonStyle();

	auto mainCard = Settings::AddRoundedBlock(content);

	const auto isEnabled = Melow::PluginEngine::Instance().isEnabled();

	const auto enableBtn = Settings::AddButtonWithSvgIcon(
		mainCard,
		rpl::single(u"Enable Plugins"_q),
		stButton,
		u":/gui/melow/melowgui/plugins/plugin.svg"_q);
	enableBtn->toggleOn(rpl::single(isEnabled));

	auto optionsWrap = mainCard->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			mainCard,
			object_ptr<Ui::VerticalLayout>(mainCard)));
	optionsWrap->toggleOn(enableBtn->toggledValue());

	enableBtn->toggledValue()
		| rpl::on_next([](bool val) {
			Melow::PluginEngine::Instance().setEnabled(val);
		}, optionsWrap->lifetime());

	auto optionsContent = optionsWrap->entity();

	auto openFolderBtn = Settings::AddButtonWithSvgIcon(
		optionsContent,
		rpl::single(u"Open Plugins Folder"_q),
		stButton,
		u":/gui/melow/melowgui/plugins/open.svg"_q);

	openFolderBtn->setClickedCallback([] {
		const auto dirPath = GetPluginsDirectory();
		File::OpenUrl(QUrl::fromLocalFile(dirPath).toString());
	});

	auto pluginsListCard = Settings::AddRoundedBlock(optionsContent);

	const auto populateList = [pluginsListCard, &stButton, controller] {
		while (pluginsListCard->count() > 0) {
			delete pluginsListCard->widgetAt(0);
		}

		const auto plugins = ScanPlugins();
		if (plugins.empty()) {
			auto emptyWrap = pluginsListCard->add(
				object_ptr<Ui::VerticalLayout>(pluginsListCard),
				style::margins(26, 16, 26, 16));
			auto label = emptyWrap->add(
				object_ptr<Ui::FlatLabel>(
					emptyWrap,
					u"No .js plugins found in the plugins directory"_q,
					st::defaultFlatLabel));
			label->setTextColorOverride(st::windowSubTextFg->c);
			return;
		}

		for (const auto &plugin : plugins) {
			const auto isPluginActive = Melow::PluginEngine::Instance().isPluginEnabled(plugin.fileName);
			
			Ui::SettingsButton *pluginBtn = Settings::AddButtonWithIcon(
				pluginsListCard,
				rpl::single(plugin.title),
				stButton,
				{});

			AttachCustomPluginIcon(pluginBtn, stButton, LoadPluginIconImage(plugin.icon));
			AttachPluginInfoButton(pluginBtn, controller, plugin);

			pluginBtn->toggleOn(rpl::single(isPluginActive));

			const auto fileName = plugin.fileName;
			pluginBtn->toggledValue()
				| rpl::on_next([fileName](bool val) {
					Melow::PluginEngine::Instance().setPluginEnabled(fileName, val);
				}, pluginBtn->lifetime());
		}
	};

	auto refreshBtn = Settings::AddButtonWithSvgIcon(
		optionsContent,
		rpl::single(u"Refresh Plugins List"_q),
		stButton,
		u":/gui/melow/melowgui/plugins/reload.svg"_q);

	refreshBtn->setClickedCallback(populateList);

	populateList();

	Ui::AddSkip(content);
	Ui::AddSkip(content);

	Ui::ResizeFitChild(this, content);
}

Type MelowGramPluginsId() {
	return MelowGramPlugins::Id();
}

} // namespace Settings
