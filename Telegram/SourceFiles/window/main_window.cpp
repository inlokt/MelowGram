#include "crl/crl_async.h"
#include "crl/crl_on_main.h"
#include <QtGui/QImageReader>
#include <QSvgRenderer>
#include <QPainter>
/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "window/main_window.h"

#include "api/api_updates.h"
#include "storage/localstorage.h"
#include "platform/platform_specific.h"
#include "ui/platform/ui_platform_window.h"
#include "platform/platform_window_title.h"
#include "history/history.h"
#include "info/media/info_media_widget.h" // SharedMediaTitle.
#include "window/window_separate_id.h"
#include "window/window_session_controller.h"
#include "window/window_lock_widgets.h"
#include "window/themes/window_theme.h"
#include "window/window_controller.h"
#include "main/main_account.h" // Account::sessionValue.
#include "main/main_domain.h"
#include "core/application.h"
#include "core/version.h"
#include "core/sandbox.h"
#include "core/shortcuts.h"
#include "lang/lang_keys.h"
#include "data/data_session.h"
#include "data/data_forum_topic.h"
#include "data/data_user.h"
#include "main/main_session.h"
#include "main/main_session_settings.h"
#include "base/options.h"
#include "base/crc32hash.h"
#include "ui/boxes/confirm_box.h"
#include "ui/toast/toast.h"
#include "ui/widgets/shadow.h"
#include "ui/controls/window_outdated_bar.h"
#include "ui/controls/window_screen_reader_bar.h"
#include "ui/painter.h"
#include "ui/screen_reader_mode.h"
#include "ui/ui_utility.h"
#include "apiwrap.h"
#include "mainwidget.h" // session->content()->windowShown().
#include "tray.h"
#include "styles/style_window.h"
#include "styles/style_dialogs.h" // ChildSkip().x() for new child windows.

#ifdef Q_OS_MAC
#include "platform/mac/global_menu_mac.h"
#endif // Q_OS_MAC

#include <QtCore/QMimeData>
#include <QtGui/QWindow>
#include <QtGui/QScreen>
#include <QtGui/QDrag>

#include <kurlmimedata.h>

namespace Window {
namespace {

constexpr auto kSaveWindowPositionTimeout = crl::time(1000);

using Core::WindowPosition;

[[nodiscard]] QPoint ChildSkip() {
	const auto skipx = st::defaultDialogRow.padding.left()
		+ st::defaultDialogRow.photoSize
		+ st::defaultDialogRow.padding.left();
	const auto skipy = st::windowTitleHeight;
	return { skipx, skipy };
}

[[nodiscard]] QImage &OverridenIcon() {
	static auto result = QImage();
	return result;
}

base::options::toggle OptionNewWindowsSizeAsFirst({
	.id = kOptionNewWindowsSizeAsFirst,
	.name = "Adjust size of new chat windows",
	.description = "Open new windows with a size of the main window.",
});

base::options::toggle OptionDisableTouchbar({
	.id = kOptionDisableTouchbar,
	.name = "Disable Touch Bar (macOS only).",
	.scope = [] {
#ifdef Q_OS_MAC
		return true;
#else // !Q_OS_MAC
		return false;
#endif // !Q_OS_MAC
	},
	.restartRequired = true,
});

[[nodiscard]] QString TitleFromSeparateSharedMedia(
		const Core::WindowTitleContent &settings,
		const SeparateId &id) {
	if (id.type != SeparateType::SharedMedia) {
		return QString();
	}
	const auto type = id.sharedMediaType;
	const auto result = Info::Media::SharedMediaTitle(type)(tr::now);
	if (settings.hideChatName) {
		return result;
	}
	const auto thread = id.thread;
	const auto topic = thread->asTopic();
	const auto name = topic
		? topic->title()
		: thread->peer()->isSelf()
		? tr::lng_saved_messages(tr::now)
		: thread->peer()->name();
	const auto wrapped = st::wrap_rtl(name);
	return name + u" @ "_q + result;
}

} // namespace

const char kOptionNewWindowsSizeAsFirst[] = "new-windows-size-as-first";
const char kOptionDisableTouchbar[] = "touchbar-disabled";

const QImage &Logo() {
	static const auto result = QImage(u":/gui/art/logo_256.png"_q);
	return result;
}

const QImage &LogoNoMargin() {
	static const auto result = QImage(u":/gui/art/logo_256_no_margin.png"_q);
	return result;
}

void ConvertIconToBlack(QImage &image) {
	if (image.format() != QImage::Format_ARGB32_Premultiplied) {
		image = std::move(image).convertToFormat(
			QImage::Format_ARGB32_Premultiplied);
	}
	//const auto gray = red * 0.299 + green * 0.587 + blue * 0.114;
	//const auto result = (gray - 100 < 0) ? 0 : (gray - 100) * 255 / 155;
	constexpr auto scale = 255 / 155.;
	constexpr auto red = 0.299;
	constexpr auto green = 0.587;
	constexpr auto blue = 0.114;
	static constexpr auto shift = (1 << 24);
	auto shifter = [](double value) {
		return uint32(value * shift);
	};
	constexpr auto iscale = shifter(scale);
	constexpr auto ired = shifter(red);
	constexpr auto igreen = shifter(green);
	constexpr auto iblue = shifter(blue);
	constexpr auto threshold = 100;

	const auto width = image.width();
	const auto height = image.height();
	const auto data = reinterpret_cast<uint32*>(image.bits());
	const auto intsPerLine = image.bytesPerLine() / 4;
	const auto intsPerLineAdded = intsPerLine - width;

	auto pixel = data;
	for (auto j = 0; j != height; ++j) {
		for (auto i = 0; i != width; ++i) {
			const auto value = *pixel;
			const auto gray = (((value >> 16) & 0xFF) * ired
				+ ((value >> 8) & 0xFF) * igreen
				+ (value & 0xFF) * iblue) >> 24;
			const auto small = gray - threshold;
			const auto test = ~small;
			const auto result = (test >> 31) * small * iscale;
			const auto component = (result >> 24) & 0xFF;
			*pixel++ = (value & 0xFF000000U)
				| (component << 16)
				| (component << 8)
				| component;
		}
		pixel += intsPerLineAdded;
	}
}

void OverrideApplicationIcon(QImage image) {
	OverridenIcon() = std::move(image);
}

QIcon CreateSupportIcon(Main::Session *session) {
	const auto support = (session && session->supportMode());
	if (!support) {
		return QIcon();
	}
	auto overriden = OverridenIcon();
	auto image = overriden.isNull()
		? Platform::DefaultApplicationIcon()
		: overriden;
	ConvertIconToBlack(image);
	return QIcon(Ui::PixmapFromImage(std::move(image)));
}

QIcon CreateIcon(Main::Session *session, bool returnNullIfDefault) {
	const auto supportIcon = CreateSupportIcon(session);
	if (!supportIcon.isNull() || returnNullIfDefault) {
		return supportIcon;
	}

	const auto officialIcon = QIcon(
		Ui::PixmapFromImage(base::duplicate(Logo())));

	if constexpr (!Platform::IsLinux()) {
		return officialIcon;
	}

	const auto iconFromTheme = QIcon::fromTheme(
		Platform::ApplicationIconName(),
		officialIcon);

	if (!Platform::IsX11()) {
		return iconFromTheme;
	}

	QIcon result;

	static const auto iconSizes = {
		16,
		22,
		32,
		48,
		64,
		128,
		256,
	};

	// Qt's standard QIconLoaderEngine sets availableSizes
	// to XDG directories sizes, since svg icons are scalable,
	// they could be only in one XDG folder (like 48x48)
	// and Qt will set only a 48px icon to the window
	// even though the icon could be scaled to other sizes.
	// Thus, scale it manually to the most widespread sizes.
	for (const auto iconSize : iconSizes) {
		// We can't use QIcon::actualSize here
		// since it works incorrectly with svg icon themes
		const auto iconPixmap = iconFromTheme.pixmap(iconSize);

		const auto iconPixmapSize = iconPixmap.size()
			/ iconPixmap.devicePixelRatio();

		// Not a svg icon, don't scale it
		if (iconPixmapSize.width() != iconSize) {
			return iconFromTheme;
		}

		result.addPixmap(iconPixmap);
	}

	return result;
}

QImage GenerateCounterLayer(CounterLayerArgs &&args) {
	const auto count = args.count.value();
	const auto text = (count < 1000)
		? QString::number(count)
		: u"..%1"_q.arg(count % 100, 2, 10, QChar('0'));
	const auto textSize = text.size();

	struct Dimensions {
		int size = 0;
		int font = 0;
		int delta = 0;
		int radius = 0;
	};
	const auto d = [&]() -> Dimensions {
		switch (args.size.value()) {
		case 16:
			return {
				.size = 16,
				.font = ((textSize < 2) ? 11 : (textSize < 3) ? 11 : 8),
				.delta = ((textSize < 2) ? 5 : (textSize < 3) ? 2 : 1),
				.radius = ((textSize < 2) ? 8 : (textSize < 3) ? 7 : 3),
			};
		case 20:
			return {
				.size = 20,
				.font = ((textSize < 2) ? 14 : (textSize < 3) ? 13 : 10),
				.delta = ((textSize < 2) ? 6 : (textSize < 3) ? 2 : 1),
				.radius = ((textSize < 2) ? 10 : (textSize < 3) ? 9 : 5),
			};
		case 24:
			return {
				.size = 24,
				.font = ((textSize < 2) ? 17 : (textSize < 3) ? 16 : 12),
				.delta = ((textSize < 2) ? 7 : (textSize < 3) ? 3 : 1),
				.radius = ((textSize < 2) ? 12 : (textSize < 3) ? 11 : 6),
			};
		default:
			return {
				.size = 32,
				.font = ((textSize < 2) ? 22 : (textSize < 3) ? 20 : 16),
				.delta = ((textSize < 2) ? 9 : (textSize < 3) ? 4 : 2),
				.radius = ((textSize < 2) ? 16 : (textSize < 3) ? 14 : 8),
			};
		}
	}();

	auto result = QImage(
		QSize(d.size, d.size) * args.devicePixelRatio,
		QImage::Format_ARGB32);
	result.setDevicePixelRatio(args.devicePixelRatio);
	result.fill(Qt::transparent);

	auto p = QPainter(&result);
	auto hq = PainterHighQualityEnabler(p);
	const auto f = style::font{ d.font, 0, 0 };
	const auto w = f->width(text);

	p.setBrush(args.bg.value());
	p.setPen(Qt::NoPen);
	p.drawRoundedRect(
		QRect(
			d.size - w - d.delta * 2,
			d.size - f->height,
			w + d.delta * 2,
			f->height),
		d.radius,
		d.radius);

	p.setFont(f);
	p.setPen(args.fg.value());
	p.drawText(d.size - w - d.delta, d.size - f->height + f->ascent, text);
	p.end();

	return result;
}

QImage WithSmallCounter(QImage image, CounterLayerArgs &&args) {
	// platform/linux/tray_linux depends on count used the same
	// way for all the same (count % 100) values.
	const auto count = args.count.value();
	const auto text = (count < 100)
		? QString::number(count)
		: QString("..%1").arg(count % 10, 1, 10, QChar('0'));
	const auto textSize = text.size();

	struct Dimensions {
		int size = 0;
		int font = 0;
		int delta = 0;
		int radius = 0;
	};
	const auto d = Dimensions{
		.size = args.size.value(),
		.font = args.size.value() / 2,
		.delta = args.size.value() / ((textSize < 2) ? 8 : 16),
		.radius = args.size.value() / ((textSize < 2) ? 4 : 5),
	};

	auto p = QPainter(&image);
	auto hq = PainterHighQualityEnabler(p);
	const auto f = style::font{ d.font, 0, 0 };
	const auto w = f->width(text);

	p.setBrush(args.bg.value());
	p.setPen(Qt::NoPen);
	p.drawRoundedRect(
		QRect(
			d.size - w - d.delta * 2,
			d.size - f->height,
			w + d.delta * 2,
			f->height),
		d.radius,
		d.radius);

	p.setFont(f);
	p.setPen(args.fg.value());
	p.drawText(d.size - w - d.delta, d.size - f->height + f->ascent, text);
	p.end();

	return image;
}

MainWindow::MainWindow(not_null<Controller*> controller)
: _controller(controller)
, _positionUpdatedTimer([=] { savePosition(); })
, _outdated(Ui::CreateOutdatedBar(body(), cWorkingDir()))
, _screenReaderBar(Ui::CreateScreenReaderBar(body(), [=] {
	controller->show(Ui::MakeConfirmBox({
		.text = tr::lng_screen_reader_confirm_text(tr::now),
		.confirmed = [=](Fn<void()> close) {
			Core::App().settings().writePref<bool>(
				Core::kScreenReaderModeDisabledKey,
				true);
			Core::App().saveSettingsDelayed();
			Ui::SetScreenReaderModeDisabled(true);
			close();
		},
		.confirmText = tr::lng_screen_reader_confirm_disable(),
	}));
}))
, _body(body()) {
	window()->setAttribute(Qt::WA_NoSystemBackground, false);
	window()->setAttribute(Qt::WA_TranslucentBackground, true);

	style::PaletteChanged(
	) | rpl::on_next([=] {
		updatePalette();
	}, lifetime());

	Core::App().unreadBadgeChanges(
	) | rpl::on_next([=] {
		updateTitle();
		unreadCounterChangedHook();
		Core::App().tray().updateIconCounters();
	}, lifetime());

	Core::App().settings().workModeChanges(
	) | rpl::on_next([=](Core::Settings::WorkMode mode) {
		workmodeUpdated(mode);
	}, lifetime());

	if (isPrimary()) {
		Ui::Toast::SetDefaultParent(_body.data());
	}

	windowActiveValue(
	) | rpl::skip(1) | rpl::on_next([=](bool active) {
		InvokeQueued(this, [=] {
			handleActiveChanged(active);
		});
	}, lifetime());

	shownValue(
	) | rpl::skip(1) | rpl::on_next([=](bool visible) {
		InvokeQueued(this, [=] {
			handleVisibleChanged(visible);
		});
	}, lifetime());

	body()->sizeValue(
	) | rpl::on_next([=](QSize size) {
		updateControlsGeometry();
	}, lifetime());

	if (_outdated) {
		_outdated->heightValue(
		) | rpl::on_next([=](int height) {
			if (!height) {
				crl::on_main(this, [=] { _outdated.destroy(); });
			}
			updateControlsGeometry();
		}, _outdated->lifetime());
	}

	if (_screenReaderBar) {
		_screenReaderBar->heightValue(
		) | rpl::on_next([=](int height) {
			updateControlsGeometry();
		}, _screenReaderBar->lifetime());
	}

	Shortcuts::Listen(this);
	setupMelowGramParticles();
	setupMelowGramGif();
}

Main::Account &MainWindow::account() const {
	return _controller->account();
}

Window::SeparateId MainWindow::id() const {
	return _controller->id();
}

bool MainWindow::isPrimary() const {
	return _controller->isPrimary();
}

Window::SessionController *MainWindow::sessionController() const {
	return _controller->sessionController();
}

bool MainWindow::hideNoQuit() {
	if (Core::Quitting()) {
		return false;
	}
	const auto workMode = Core::App().settings().workMode();
	using Mode = Core::Settings::WorkMode;
	if (workMode == Mode::TrayOnly || workMode == Mode::WindowAndTray) {
		if (minimizeToTray()) {
			if (const auto controller = sessionController()) {
				controller->clearSectionStack();
			}
			return true;
		}
	}
	using Behavior = Core::Settings::CloseBehavior;
	const auto behavior = Platform::IsMac()
		? Behavior::RunInBackground
		: Core::App().settings().closeBehavior();
	if (behavior == Behavior::RunInBackground) {
		closeWithoutDestroy();
	} else if (behavior == Behavior::CloseToTaskbar) {
		setWindowState(window()->windowState() | Qt::WindowMinimized);
	} else {
		return false;
	}
	controller().updateIsActiveBlur();
	updateGlobalMenu();
	if (const auto controller = sessionController()) {
		controller->clearSectionStack();
	}
	return true;
}

void MainWindow::clearWidgets() {
	clearWidgetsHook();
	updateGlobalMenu();
}

void MainWindow::updateGlobalMenu() {
#ifdef Q_OS_MAC
	Platform::RequestUpdateGlobalMenu();
#else // Q_OS_MAC
	updateGlobalMenuHook();
#endif // Q_OS_MAC
}

void MainWindow::updateIsActive() {
	const auto isActive = computeIsActive();
	if (_isActive != isActive) {
		_isActive = isActive;
	}
}

bool MainWindow::computeIsActive() const {
	return isActiveWindow() && isVisible() && !(windowState() & Qt::WindowMinimized);
}

QRect MainWindow::desktopRect() const {
	const auto now = crl::now();
	if (!_monitorLastGot || now >= _monitorLastGot + crl::time(1000)) {
		_monitorLastGot = now;
		_monitorRect = computeDesktopRect();
	}
	return _monitorRect;
}

void MainWindow::init() {
	initHook();

	updatePalette();

	if (Ui::Platform::NativeWindowFrameSupported()) {
		Core::App().settings().nativeWindowFrameChanges(
		) | rpl::on_next([=](bool native) {
			refreshTitleWidget();
			recountGeometryConstraints();
		}, lifetime());
	}
	refreshTitleWidget();

	updateWindowTransparency();
	updateTitle();
	updateWindowIcon();
}

void MainWindow::updateWindowTransparency() {
	if (!Core::IsAppLaunched()) return;
	Window::Theme::ApplyMelowGramModifiers();
	
	if (_melowgramGifLabel) {
		_melowgramGifLabel->update();
	}
	if (body()) {
		body()->update();
	}
	window()->update();
}

void MainWindow::handleStateChanged(Qt::WindowState state) {
	stateChangedHook(state);
	updateControlsGeometry();
	if (state == Qt::WindowMinimized) {
		controller().updateIsActiveBlur();
	} else {
		controller().updateIsActiveFocus();
		updateWindowTransparency();
	}
	Core::App().updateNonIdle();
	using WorkMode = Core::Settings::WorkMode;
	if (state == Qt::WindowMinimized
		&& (Core::App().settings().workMode() == WorkMode::TrayOnly)) {
		minimizeToTray();
	}
	savePosition(state);
}

void MainWindow::handleActiveChanged(bool active) {
	checkActivation();
	if (active) {
		Core::App().windowActivated(&controller());
	}
	if (const auto controller = sessionController()) {
		controller->session().updates().updateOnline();
	}
}

void MainWindow::handleVisibleChanged(bool visible) {
	if (visible) {
		if (_maximizedBeforeHide) {
			DEBUG_LOG(("Window Pos: Window was maximized before hidding, setting maximized."));
			setWindowState(Qt::WindowMaximized);
		}
	} else {
		_maximizedBeforeHide = Core::App().settings().windowPosition().maximized;
	}

	handleVisibleChangedHook(visible);
}

void MainWindow::showFromTray() {
	InvokeQueued(this, [=] {
		updateGlobalMenu();
	});
	activate();
	unreadCounterChangedHook();
	Core::App().tray().updateIconCounters();
}

void MainWindow::quitFromTray() {
	Core::Quit();
}

void MainWindow::activate() {
	bool wasHidden = !isVisible();
	setWindowState(windowState() & ~Qt::WindowMinimized);
	setVisible(true);
	Platform::ActivateThisProcess();
	raise();
	activateWindow();
	controller().updateIsActiveFocus();
	if (wasHidden) {
		if (const auto session = sessionController()) {
			session->content()->windowShown();
		}
	}
}

void MainWindow::updatePalette() {
	Ui::ForceFullRepaint(this);

	reloadMelowGramGif();

	auto p = palette();
	p.setColor(QPalette::Window, st::windowBg->c);
	setPalette(p);
}

int MainWindow::computeMinWidth() const {
	auto result = st::windowMinWidth;
	if (_rightColumn) {
		result += _rightColumn->width();
	}
	return result;
}

int MainWindow::computeMinHeight() const {
	const auto outdated = [&] {
		if (!_outdated) {
			return 0;
		}
		_outdated->resizeToWidth(st::windowMinWidth);
		return _outdated->height();
	}();
	const auto screenReader = [&] {
		if (!_screenReaderBar) {
			return 0;
		}
		_screenReaderBar->resizeToWidth(st::windowMinWidth);
		return _screenReaderBar->height();
	}();
	return outdated + screenReader + st::windowMinHeight;
}

void MainWindow::refreshTitleWidget() {
	if (Ui::Platform::NativeWindowFrameSupported()
		&& Core::App().settings().nativeWindowFrame()) {
		setNativeFrame(true);
		if (Platform::NativeTitleRequiresShadow()) {
			_titleShadow.create(this);
			_titleShadow->show();
		}
	} else {
		setNativeFrame(false);
		_titleShadow.destroy();
	}
}

void MainWindow::updateMinimumSize() {
	setMinimumSize(QSize(computeMinWidth(), computeMinHeight()));
}

void MainWindow::recountGeometryConstraints() {
	updateMinimumSize();
	updateControlsGeometry();
	fixOrder();
}

WindowPosition MainWindow::initialPosition() const {
	const auto active = Core::App().activeWindow();
	return (!active || active == &controller())
		? Core::AdjustToScale(
			Core::App().settings().windowPosition(),
			u"Window"_q)
		: active->widget()->nextInitialChildPosition(id());
}

WindowPosition MainWindow::nextInitialChildPosition(SeparateId childId) {
	const auto rect = geometry().marginsRemoved(frameMargins());
	const auto position = rect.topLeft();
	const auto adjust = [&](int value) {
		return (value * 3 / 4);
	};
	const auto width = OptionNewWindowsSizeAsFirst.value()
		? Core::App().settings().windowPosition().w
		: childId.primary()
		? st::windowDefaultWidth
		: childId.hasChatsList()
		? (st::columnMinimalWidthLeft + adjust(st::windowDefaultWidth))
		: adjust(st::windowDefaultWidth);
	const auto height = OptionNewWindowsSizeAsFirst.value()
		? Core::App().settings().windowPosition().h
		: childId.primary()
		? st::windowDefaultHeight
		: adjust(st::windowDefaultHeight);
	const auto skip = ChildSkip();
	const auto delta = _lastChildIndex
		? (_lastMyChildCreatePosition - position)
		: skip;
	if (qAbs(delta.x()) >= skip.x() || qAbs(delta.y()) >= skip.y()) {
		_lastChildIndex = 1;
	} else {
		++_lastChildIndex;
	}

	_lastMyChildCreatePosition = position;
	const auto use = position + (skip * _lastChildIndex);
	return withScreenInPosition({
		.scale = cScale(),
		.x = use.x(),
		.y = use.y(),
		.w = width,
		.h = height,
	});
}

QRect MainWindow::countInitialGeometry(WindowPosition position) {
	const auto primaryScreen = QGuiApplication::primaryScreen();
	const auto primaryAvailable = primaryScreen
		? primaryScreen->availableGeometry()
		: QRect(0, 0, st::windowDefaultWidth, st::windowDefaultHeight);
	const auto initialWidth = Core::Settings::ThirdColumnByDefault()
		? st::windowBigDefaultWidth
		: st::windowDefaultWidth;
	const auto initialHeight = Core::Settings::ThirdColumnByDefault()
		? st::windowBigDefaultHeight
		: st::windowDefaultHeight;
	const auto initial = WindowPosition{
		.x = (primaryAvailable.x()
			+ std::max((primaryAvailable.width() - initialWidth) / 2, 0)),
		.y = (primaryAvailable.y()
			+ std::max((primaryAvailable.height() - initialHeight) / 2, 0)),
		.w = initialWidth,
		.h = initialHeight,
	};
	return CountInitialGeometry(
		this,
		position,
		initial,
		{ st::windowMinWidth, st::windowMinHeight },
		u"Window"_q);
}

void MainWindow::firstShow() {
	updateMinimumSize();
	if (initGeometryFromSystem()) {
		show();
		return;
	}
	const auto geometry = countInitialGeometry(initialPosition());
	DEBUG_LOG(("Window Pos: Setting first %1, %2, %3, %4"
		).arg(geometry.x()
		).arg(geometry.y()
		).arg(geometry.width()
		).arg(geometry.height()));
	setGeometry(geometry);
	show();
}

void MainWindow::positionUpdated() {
	_positionUpdatedTimer.callOnce(kSaveWindowPositionTimeout);
}

void MainWindow::setPositionInited() {
	_positionInited = true;
}

void MainWindow::imeCompositionStartReceived() {
	_imeCompositionStartReceived.fire({});
}

rpl::producer<> MainWindow::leaveEvents() const {
	return _leaveEvents.events();
}

rpl::producer<> MainWindow::imeCompositionStarts() const {
	return _imeCompositionStartReceived.events();
}

void MainWindow::leaveEventHook(QEvent *e) {
	_leaveEvents.fire({});
}

void MainWindow::updateControlsGeometry() {
	const auto inner = body()->rect();
	auto bodyLeft = inner.x();
	auto bodyTop = inner.y();
	auto bodyWidth = inner.width();
	if (_titleShadow) {
		_titleShadow->setGeometry(inner.x(), bodyTop, inner.width(), st::lineWidth);
	}
	if (_outdated) {
		Ui::SendPendingMoveResizeEvents(_outdated.data());
		_outdated->resizeToWidth(inner.width());
		_outdated->moveToLeft(inner.x(), bodyTop);
		bodyTop += _outdated->height();
	}
	if (_screenReaderBar) {
		Ui::SendPendingMoveResizeEvents(_screenReaderBar.data());
		_screenReaderBar->resizeToWidth(inner.width());
		_screenReaderBar->moveToLeft(inner.x(), bodyTop);
		bodyTop += _screenReaderBar->height();
	}
	if (_rightColumn) {
		bodyWidth -= _rightColumn->width();
		_rightColumn->setGeometry(bodyWidth, bodyTop, inner.width() - bodyWidth, inner.height() - (bodyTop - inner.y()));
	}
	_body->setGeometry(bodyLeft, bodyTop, bodyWidth, inner.height() - (bodyTop - inner.y()));
}

void MainWindow::updateTitle() {
	if (Core::Quitting()) {
		return;
	}

	const auto settings = Core::App().settings().windowTitleContent();
	const auto locked = Core::App().passcodeLocked();
	const auto counter = settings.hideTotalUnread
		? 0
		: Core::App().unreadBadge();
	const auto added = (counter > 0) ? u" (%1)"_q.arg(counter) : QString();
	const auto session = locked ? nullptr : _controller->sessionController();
	const auto user = (session
		&& !settings.hideAccountName
		&& Core::App().domain().accountsAuthedCount() > 1)
		? st::wrap_rtl(session->authedName())
		: QString();
	const auto separateSharedMediaTitle = session
		? TitleFromSeparateSharedMedia(settings, session->windowId())
		: QString();
	if (!separateSharedMediaTitle.isEmpty()) {
		setTitle(separateSharedMediaTitle);
		return;
	}
	const auto key = (session && !settings.hideChatName)
		? session->activeChatCurrent()
		: Dialogs::Key();
	const auto thread = key ? key.thread() : nullptr;
	if (!thread) {
		setTitle((user.isEmpty() ? AppName.utf16() : user) + added);
		return;
	}
	const auto history = thread->owningHistory();
	const auto topic = thread->asTopic();
	const auto name = topic
		? topic->title()
		: history->peer->isSelf()
		? tr::lng_saved_messages(tr::now)
		: history->peer->name();
	const auto wrapped = st::wrap_rtl(name);
	const auto threadCounter = thread->chatListBadgesState().unreadCounter;
	const auto primary = (threadCounter > 0)
		? u"(%1) %2"_q.arg(threadCounter).arg(wrapped)
		: wrapped;
	const auto middle = !user.isEmpty()
		? (u" @ "_q + user)
		: !added.isEmpty()
		? u" \u2013"_q
		: QString();
	setTitle(primary + middle + added);
}

QRect MainWindow::computeDesktopRect() const {
	return screen()->availableGeometry();
}

void MainWindow::savePosition(Qt::WindowState state) {
	if (state == Qt::WindowActive) {
		state = windowHandle()->windowState();
	}

	if (state == Qt::WindowMinimized
		|| !isVisible()
		|| !Core::App().savingPositionFor(&controller())
		|| !positionInited()) {
		return;
	}

	const auto &savedPosition = Core::App().settings().windowPosition();
	auto realPosition = savedPosition;

	if (state == Qt::WindowMaximized) {
		realPosition.maximized = 1;
		DEBUG_LOG(("Window Pos: Saving maximized position."));
	} else {
		auto r = body()->mapToGlobal(body()->rect());
		realPosition.x = r.x();
		realPosition.y = r.y();
		realPosition.w = r.width() - (_rightColumn ? _rightColumn->width() : 0);
		realPosition.h = r.height();
		realPosition.scale = cScale();
		realPosition.maximized = 0;
		realPosition.moncrc = 0;

		DEBUG_LOG(("Window Pos: Saving non-maximized position: %1, %2, %3, %4").arg(realPosition.x).arg(realPosition.y).arg(realPosition.w).arg(realPosition.h));
		realPosition = withScreenInPosition(realPosition);
	}
	if (realPosition.w >= st::windowMinWidth && realPosition.h >= st::windowMinHeight) {
		if (realPosition.x != savedPosition.x
			|| realPosition.y != savedPosition.y
			|| realPosition.w != savedPosition.w
			|| realPosition.h != savedPosition.h
			|| realPosition.scale != savedPosition.scale
			|| realPosition.moncrc != savedPosition.moncrc
			|| realPosition.maximized != savedPosition.maximized) {
			DEBUG_LOG(("Window Pos: Writing: %1, %2, %3, %4 (scale %5%, maximized %6)")
				.arg(realPosition.x)
				.arg(realPosition.y)
				.arg(realPosition.w)
				.arg(realPosition.h)
				.arg(realPosition.scale)
				.arg(Logs::b(realPosition.maximized)));
			Core::App().settings().setWindowPosition(realPosition);
			Core::App().saveSettingsDelayed();
		}
	}
}

WindowPosition MainWindow::withScreenInPosition(
		WindowPosition position) const {
	return PositionWithScreen(
		position,
		this,
		{ st::windowMinWidth, st::windowMinHeight },
		u"Window"_q);
}

bool MainWindow::minimizeToTray() {
	if (Core::Quitting() || !Core::App().tray().has()) {
		return false;
	}

	closeWithoutDestroy();
	controller().updateIsActiveBlur();
	updateGlobalMenu();
	return true;
}

void MainWindow::showRightColumn(object_ptr<Ui::RpWidget> widget) {
	const auto wasWidth = width();
	const auto wasRightWidth = _rightColumn ? _rightColumn->width() : 0;
	_rightColumn = std::move(widget);
	if (_rightColumn) {
		_rightColumn->setParent(body());
		_rightColumn->show();
		_rightColumn->setFocus();
	} else {
		setInnerFocus();
	}
	const auto nowRightWidth = _rightColumn ? _rightColumn->width() : 0;
	const auto wasMinimumWidth = minimumWidth();
	const auto nowMinimumWidth = computeMinWidth();
	const auto firstResize = (nowMinimumWidth < wasMinimumWidth);
	if (firstResize) {
		updateMinimumSize();
	}
	if (!isMaximized()) {
		tryToExtendWidthBy(wasWidth + nowRightWidth - wasRightWidth - width());
	} else {
		updateControlsGeometry();
	}
	if (!firstResize) {
		updateMinimumSize();
	}
}

int MainWindow::maximalExtendBy() const {
	auto desktop = screen()->availableGeometry();
	return std::max(desktop.width() - body()->width(), 0);
}

bool MainWindow::canExtendNoMove(int extendBy) const {
	auto desktop = screen()->availableGeometry();
	auto inner = body()->mapToGlobal(body()->rect());
	auto innerRight = (inner.x() + inner.width() + extendBy);
	auto desktopRight = (desktop.x() + desktop.width());
	return innerRight <= desktopRight;
}

int MainWindow::tryToExtendWidthBy(int addToWidth) {
	auto desktop = screen()->availableGeometry();
	auto inner = body()->mapToGlobal(body()->rect());
	accumulate_min(
		addToWidth,
		std::max(desktop.width() - inner.width(), 0));
	auto newWidth = inner.width() + addToWidth;
	auto newLeft = std::min(
		inner.x(),
		desktop.x() + desktop.width() - newWidth);
	if (inner.x() != newLeft || inner.width() != newWidth) {
		setGeometry(QRect(newLeft, inner.y(), newWidth, inner.height()));
	} else {
		updateControlsGeometry();
	}
	return addToWidth;
}

void MainWindow::launchDrag(
		std::unique_ptr<QMimeData> data,
		Fn<void()> &&callback,
		QPixmap pixmap) {
	// Qt destroys this QDrag automatically after the drag is finished
	// We must not delete this at the end of this function, as this breaks DnD on Linux
	auto drag = new QDrag(this);
	KUrlMimeData::exportUrlsToPortal(data.get());
	drag->setMimeData(data.release());
	if (!pixmap.isNull()) {
		drag->setPixmap(std::move(pixmap));
	}
	drag->exec(Qt::CopyAction);

	// We don't receive mouseReleaseEvent when drag is finished.
	ClickHandler::unpressed();
	callback();
}

MainWindow::~MainWindow() {
	if (_melowgramGifStop) {
		*_melowgramGifStop = true;
	}
	// Otherwise:
	// ~QWidget
	// QWidgetPrivate::close_helper
	// QWidgetPrivate::setVisible
	// QWidgetPrivate::hide_helper
	// QWidgetPrivate::hide_sys
	// QWindowPrivate::setVisible
	// QMetaObject::activate
	// Window::MainWindow::handleVisibleChanged on a destroyed MainWindow.
	hide();
}

int32 DefaultScreenNameChecksum(const QString &name) {
	const auto bytes = name.toUtf8();
	return base::crc32(bytes.constData(), bytes.size());
}

WindowPosition PositionWithScreen(
		WindowPosition position,
		const QScreen *chosen,
		QSize minimal,
		const QString &name) {
	if (!chosen) {
		return position;
	}
	const auto available = chosen->availableGeometry();
	if (available.width() < minimal.width()
		|| available.height() < minimal.height()) {
		return position;
	}
	accumulate_min(position.w, available.width());
	accumulate_min(position.h, available.height());
	if (position.x + position.w > available.x() + available.width()) {
		position.x = available.x() + available.width() - position.w;
	}
	if (position.y + position.h > available.y() + available.height()) {
		position.y = available.y() + available.height() - position.h;
	}
	const auto geometry = chosen->geometry();
	DEBUG_LOG(("%1 Pos: Screen found, geometry: %2, %3, %4, %5"
		).arg(name
		).arg(geometry.x()
		).arg(geometry.y()
		).arg(geometry.width()
		).arg(geometry.height()));
	return position;
}

WindowPosition PositionWithScreen(
		WindowPosition position,
		not_null<const QWidget*> widget,
		QSize minimal,
		const QString &name) {
	const auto screen = widget->screen();
	return PositionWithScreen(
		position,
		screen ? screen : QGuiApplication::primaryScreen(),
		minimal,
		name);
}

QRect CountInitialGeometry(
		not_null<const Ui::RpWindow*> widget,
		WindowPosition position,
		WindowPosition initial,
		QSize minSize,
		const QString &name) {
	if (!position.w || !position.h) {
		return initial.rect();
	}
	const auto screen = [&]() -> QScreen* {
		for (const auto screen : QGuiApplication::screens()) {
			const auto sum = Platform::ScreenNameChecksum(screen->name());
			if (position.moncrc == sum) {
				return screen;
			}
		}
		return QGuiApplication::screenAt(position.rect().center());
	}();
	if (!screen) {
		return initial.rect();
	}
	const auto frame = widget->frameMargins();
	const auto screenGeometry = screen->geometry();
	const auto availableGeometry = screen->availableGeometry();
	const auto spaceForInner = availableGeometry.marginsRemoved(frame);
	DEBUG_LOG(("%1 Pos: "
		"Screen found, screen geometry: %2, %3, %4, %5, "
		"available: %6, %7, %8, %9"
		).arg(name
		).arg(screenGeometry.x()
		).arg(screenGeometry.y()
		).arg(screenGeometry.width()
		).arg(screenGeometry.height()
		).arg(availableGeometry.x()
		).arg(availableGeometry.y()
		).arg(availableGeometry.width()
		).arg(availableGeometry.height()));
	DEBUG_LOG(("%1 Pos: "
		"Window frame margins: %2, %3, %4, %5, "
		"available space for inner geometry: %6, %7, %8, %9"
		).arg(name
		).arg(frame.left()
		).arg(frame.top()
		).arg(frame.right()
		).arg(frame.bottom()
		).arg(spaceForInner.x()
		).arg(spaceForInner.y()
		).arg(spaceForInner.width()
		).arg(spaceForInner.height()));

	const auto x = spaceForInner.x()
		- (position.moncrc ? screenGeometry.x() : 0);
	const auto y = spaceForInner.y()
		- (position.moncrc ? screenGeometry.y() : 0);
	const auto w = spaceForInner.width();
	const auto h = spaceForInner.height();
	if (w < st::windowMinWidth || h < st::windowMinHeight) {
		return initial.rect();
	}
	if (position.x < x) position.x = x;
	if (position.y < y) position.y = y;
	if (position.w > w) position.w = w;
	if (position.h > h) position.h = h;
	const auto rightPoint = position.x + position.w;
	const auto screenRightPoint = x + w;
	if (rightPoint > screenRightPoint) {
		const auto distance = rightPoint - screenRightPoint;
		const auto newXPos = position.x - distance;
		if (newXPos >= x) {
			position.x = newXPos;
		} else {
			position.x = x;
			const auto newRightPoint = position.x + position.w;
			const auto newDistance = newRightPoint - screenRightPoint;
			position.w -= newDistance;
		}
	}
	const auto bottomPoint = position.y + position.h;
	const auto screenBottomPoint = y + h;
	if (bottomPoint > screenBottomPoint) {
		const auto distance = bottomPoint - screenBottomPoint;
		const auto newYPos = position.y - distance;
		if (newYPos >= y) {
			position.y = newYPos;
		} else {
			position.y = y;
			const auto newBottomPoint = position.y + position.h;
			const auto newDistance = newBottomPoint - screenBottomPoint;
			position.h -= newDistance;
		}
	}
	if (position.moncrc) {
		position.x += screenGeometry.x();
		position.y += screenGeometry.y();
	}
	if ((position.x + st::windowMinWidth
		> screenGeometry.x() + screenGeometry.width())
		|| (position.y + st::windowMinHeight
			> screenGeometry.y() + screenGeometry.height())) {
		return initial.rect();
	}
	DEBUG_LOG(("%1 Pos: Resulting geometry is %2, %3, %4, %5"
		).arg(name
		).arg(position.x
		).arg(position.y
		).arg(position.w
		).arg(position.h));
	return position.rect();
}

void MainWindow::setupMelowGramGif() {
	if (!Core::IsAppLaunched()) return;
	if (Core::App().settings().readPref<bool>("MelowGramGifBackground", false)) {
		const auto path = Core::App().settings().readPref<QString>("MelowGramGifPath", QString());
		if (!path.isEmpty()) {
			_melowgramGifMovie = std::make_unique<QMovie>(path);
			if (_melowgramGifMovie->isValid()) {
				_melowgramGifLabel.create(body());
				_melowgramGifLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
				_melowgramGifLabel->show();
				_melowgramGifLabel->lower();
				
				_melowgramGifLabel->paintRequest() | rpl::on_next([=] {
					QPainter p(_melowgramGifLabel);
					QPixmap pixmap = _melowgramGifMovie->currentPixmap();
					if (!pixmap.isNull()) {
						QSize s = pixmap.size();
						QSize ws = _melowgramGifLabel->size();
						s.scale(ws, Qt::KeepAspectRatioByExpanding);
						int x = (ws.width() - s.width()) / 2;
						int y = (ws.height() - s.height()) / 2;
						
						int blackout = Core::App().settings().readPref<int>("MelowGramBlackout", 100);
						bool blur = Core::App().settings().readPref<bool>("MelowGramBlur", false);
						
						p.setCompositionMode(QPainter::CompositionMode_Source);
						if (blur && blackout < 100) {
							p.fillRect(_melowgramGifLabel->rect(), Qt::transparent);
						} else {
							p.fillRect(_melowgramGifLabel->rect(), Qt::black);
						}
						
						p.setCompositionMode(QPainter::CompositionMode_SourceOver);
						if (blur && blackout < 100) {
							p.setOpacity(blackout / 100.0);
						}
						
						p.drawPixmap(x, y, s.width(), s.height(), pixmap);
					}
				}, _melowgramGifLabel->lifetime());

				_melowgramGifUpdateTimer.setCallback([=] {
					if (_melowgramGifLabel && window()->windowState() != Qt::WindowMinimized) {
						_melowgramGifMovie->jumpToNextFrame();
						_melowgramGifLabel->update();
					}
				});
				_melowgramGifUpdateTimer.callEach(66); // ~15 FPS
				
				body()->sizeValue() | rpl::on_next([=](QSize size) {
					if (_melowgramGifLabel) {
						_melowgramGifLabel->setGeometry(QRect(QPoint(0, 0), size));
					}
				}, _melowgramGifLabel->lifetime());

				_melowgramGifMovie->start();
				_melowgramGifMovie->setPaused(true);
			}
		}
	}
}

void MainWindow::reloadMelowGramGif() {
	if (_melowgramGifLabel) {
		_melowgramGifLabel.destroy();
	}
	if (_melowgramGifMovie) {
		_melowgramGifMovie.reset();
	}
	_melowgramGifUpdateTimer.cancel();
	setupMelowGramGif();
	
	if (body()) {
		body()->update();
	}
	updateWindowTransparency();
}

void MainWindow::setupMelowGramParticles() {
	if (!body()) return;

	_melowgramBackgroundEffects.create(body());
	_melowgramBackgroundEffects->setAttribute(Qt::WA_TransparentForMouseEvents);
	_melowgramBackgroundEffects->show();
	_melowgramBackgroundEffects->lower();
	if (_melowgramGifLabel) {
		_melowgramGifLabel->lower();
	}

	_melowgramParticlesOverlay.create(body());
	_melowgramParticlesOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
	_melowgramParticlesOverlay->show();

	if (!_melowParticlesEventFilterInstalled) {
		_melowParticlesEventFilterInstalled = true;
		QCoreApplication::instance()->installEventFilter(this);
	}

	_melowgramBackgroundEffects->paintRequest() | rpl::on_next([=](QRect clip) {
		if (!Core::IsAppLaunched()) return;
		const bool effectsEnabled = Core::App().settings().readPref<bool>("MelowGramEffects", false);
		if (!effectsEnabled) return;

		Painter p(_melowgramBackgroundEffects.data());
		p.setRenderHint(QPainter::Antialiasing);

		const int effectsType = Core::App().settings().readPref<int>("MelowGramEffectsType", 0);
		if (effectsType == 0) { // Snow
			for (const auto &flake : _melowWeatherParticles) {
				p.setPen(Qt::NoPen);
				QColor c(255, 255, 255);
				c.setAlphaF(flake.alpha);
				p.setBrush(c);
				p.drawEllipse(flake.pos, flake.size, flake.size);
				if (flake.size > 2.8f) {
					c.setAlphaF(flake.alpha * 0.25f);
					p.setBrush(c);
					p.drawEllipse(flake.pos, flake.size * 1.6f, flake.size * 1.6f);
				}
			}
		} else { // Rain
			for (const auto &drop : _melowWeatherParticles) {
				QColor c(185, 215, 245);
				c.setAlphaF(drop.alpha);
				QPen pen(c);
				pen.setWidthF(drop.size);
				pen.setCapStyle(Qt::RoundCap);
				p.setPen(pen);
				p.drawLine(drop.pos, drop.pos + QPointF(drop.speedX * (drop.length / drop.speedY), drop.length));
			}
		}
	}, _melowgramBackgroundEffects->lifetime());

	_melowgramParticlesOverlay->paintRequest() | rpl::on_next([=](QRect clip) {
		if (!Core::IsAppLaunched()) return;
		Painter p(_melowgramParticlesOverlay.data());
		p.setRenderHint(QPainter::Antialiasing);

		float connectDist = Core::App().settings().readPref<int>("MelowGramParticlesDistance", 70);
		float connectDistSq = connectDist * connectDist;

		for (size_t i = 0; i < _melowParticles.size(); ++i) {
			if (_melowParticles[i].life <= 0) continue;
			for (size_t j = i + 1; j < _melowParticles.size(); ++j) {
				if (_melowParticles[j].life <= 0) continue;
				float dx = _melowParticles[i].pos.x() - _melowParticles[j].pos.x();
				float dy = _melowParticles[i].pos.y() - _melowParticles[j].pos.y();
				float distSq = dx*dx + dy*dy;
				if (distSq < connectDistSq) {
					float alpha = (1.0f - distSq / connectDistSq) * std::min(_melowParticles[i].life, _melowParticles[j].life) * 0.5f;
					QColor c = _melowParticles[i].color;
					c.setAlphaF(alpha);
					QPen pen(c);
					pen.setWidthF(1.0f);
					p.setPen(pen);
					p.drawLine(_melowParticles[i].pos, _melowParticles[j].pos);
				}
			}
		}

		for (const auto &particle : _melowParticles) {
			int alpha = particle.life * 255;
			if (alpha <= 0) continue;
			
			// A glowing effect: draw multiple circles
			QColor c = particle.color;
			
			// Outer glow
			float glowMul = Core::App().settings().readPref<int>("MelowGramParticlesGlowSize100", 200) / 100.0f;
			c.setAlphaF(particle.life * 0.3f);
			p.setPen(Qt::NoPen);
			p.setBrush(c);
			p.drawEllipse(particle.pos, particle.size * glowMul, particle.size * glowMul);
			
			// Core
			c.setAlphaF(particle.life);
			p.setBrush(c);
			p.drawEllipse(particle.pos, particle.size, particle.size);
		}

		for (const auto &r : _melowRipples) {
			if (r.life <= 0) continue;
			
			float progress = 1.0f - r.life; // 0.0 to 1.0
			float radius = 10.0f + progress * 80.0f; // expands
			
			QColor c = st::windowBgActive->c;
			
			// Outer wave
			c.setAlphaF(r.life * 0.8f);
			QPen pen(c);
			pen.setWidthF(1.0f + r.life * 4.0f);
			p.setPen(pen);
			p.setBrush(Qt::NoBrush);
			p.drawEllipse(r.pos, radius, radius);
			
			// Inner wave for water drop
			if (progress > 0.1f) {
				float innerRadius = radius * 0.5f;
				c.setAlphaF(r.life * 0.4f);
				pen.setColor(c);
				pen.setWidthF(1.0f + r.life * 2.0f);
				p.setPen(pen);
				p.drawEllipse(r.pos, innerRadius, innerRadius);
			}
		}
	}, _melowgramParticlesOverlay->lifetime());

	_melowParticlesTimer.setCallback([=] {
		if (!window() || !window()->isVisible() || isMinimized() || isHidden()) {
			return;
		}

		bool interactiveAlive = false;
		for (auto &p : _melowParticles) {
			if (p.life > 0.0f) {
				p.pos += p.velocity;
				p.life -= 0.02f; // Fade out speed
				p.size += 0.1f; // Expand slightly
				interactiveAlive = true;
			}
		}
		for (auto &r : _melowRipples) {
			if (r.life > 0.0f) {
				r.life -= 0.025f; // Fade speed
				interactiveAlive = true;
			}
		}

		bool weatherAlive = false;
		const bool effectsEnabled = Core::IsAppLaunched()
			&& Core::App().settings().readPref<bool>("MelowGramEffects", false);
		const int effectsType = Core::IsAppLaunched()
			? Core::App().settings().readPref<int>("MelowGramEffectsType", 0)
			: 0;
		const float speedFactor = Core::IsAppLaunched()
			? (std::max(1, Core::App().settings().readPref<int>("MelowGramEffectsSpeed", 50)) / 50.0f)
			: 1.0f;

		const auto bodyW = body() ? body()->width() : 0;
		const auto bodyH = body() ? body()->height() : 0;

		if (effectsEnabled && bodyW > 0 && bodyH > 0) {
			const size_t targetCount = (effectsType == 0) ? 90 : 120;
			if (_melowWeatherType != effectsType || _melowWeatherParticles.size() != targetCount) {
				_melowWeatherType = effectsType;
				_melowWeatherParticles.clear();
				_melowWeatherParticles.reserve(targetCount);
				for (size_t i = 0; i < targetCount; ++i) {
					MelowWeatherParticle wp;
					wp.pos = QPointF(
						(std::rand() % (bodyW + 200)) - 100,
						std::rand() % bodyH);
					if (effectsType == 0) { // Snow
						wp.speedY = 1.0f + (std::rand() % 20) / 10.0f;
						wp.speedX = 0.4f + (std::rand() % 10) / 10.0f;
						wp.size = 1.5f + (std::rand() % 25) / 10.0f;
						wp.alpha = 0.35f + (std::rand() % 50) / 100.0f;
						wp.phase = (std::rand() % 628) / 100.0f;
					} else { // Rain
						wp.speedY = 14.0f + (std::rand() % 100) / 10.0f;
						wp.speedX = -2.5f - (std::rand() % 15) / 10.0f;
						wp.length = 12.0f + (std::rand() % 160) / 10.0f;
						wp.size = 1.0f + (std::rand() % 10) / 10.0f;
						wp.alpha = 0.25f + (std::rand() % 45) / 100.0f;
					}
					_melowWeatherParticles.push_back(wp);
				}
			}

			for (auto &wp : _melowWeatherParticles) {
				wp.pos.ry() += wp.speedY * speedFactor;
				if (effectsType == 0) { // Snow
					wp.phase += 0.03f * speedFactor;
					wp.pos.rx() += std::sin(wp.phase) * (wp.speedX * speedFactor);
					if (wp.pos.y() > bodyH + 5) {
						wp.pos.setY(-5);
						wp.pos.setX(std::rand() % bodyW);
					}
					if (wp.pos.x() > bodyW + 10) {
						wp.pos.setX(-5);
					} else if (wp.pos.x() < -10) {
						wp.pos.setX(bodyW + 5);
					}
				} else { // Rain
					wp.pos.rx() += wp.speedX * speedFactor;
					if (wp.pos.y() > bodyH + wp.length || wp.pos.x() < -60) {
						wp.pos.setY(-wp.length);
						wp.pos.setX((std::rand() % (bodyW + 200)) - 50);
					}
				}
			}
			weatherAlive = true;
		} else if (!_melowWeatherParticles.empty()) {
			_melowWeatherParticles.clear();
			_melowWeatherType = -1;
		}

		if (weatherAlive) {
			_melowgramBackgroundEffects->update();
		} else if (_melowgramBackgroundEffects) {
			_melowgramBackgroundEffects->update();
		}

		if (interactiveAlive) {
			_melowgramParticlesOverlay->update();
		} else {
			if (!_melowParticles.empty()) _melowParticles.clear();
			if (!_melowRipples.empty()) _melowRipples.clear();
			_melowgramParticlesOverlay->update();
		}

		if (!weatherAlive && !interactiveAlive) {
			_melowParticlesTimer.cancel();
		}
	});

	body()->sizeValue() | rpl::on_next([=](QSize size) {
		if (_melowgramBackgroundEffects) {
			_melowgramBackgroundEffects->setGeometry(QRect(QPoint(0, 0), size));
			_melowgramBackgroundEffects->lower();
			if (_melowgramGifLabel) {
				_melowgramGifLabel->lower();
			}
		}
		_melowgramParticlesOverlay->resize(size);
		_melowgramParticlesOverlay->raise();
	}, _melowgramParticlesOverlay->lifetime());

	if (Core::IsAppLaunched() && Core::App().settings().readPref<bool>("MelowGramEffects", false)) {
		_melowParticlesTimer.callEach(16);
	}
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e) {
	if (!Core::IsAppLaunched() || !body()) {
		return Ui::RpWindow::eventFilter(obj, e);
	}
	if (Core::App().settings().readPref<bool>("MelowGramEffects", false) && !_melowParticlesTimer.isActive()) {
		_melowParticlesTimer.callEach(16);
	}
	if (e->type() == QEvent::MouseMove || e->type() == QEvent::MouseButtonPress) {
		bool onMove = Core::App().settings().readPref<bool>("MelowGramParticlesMove", false);
		bool onClick = Core::App().settings().readPref<bool>("MelowGramParticlesClick", false);
		bool onMercury = Core::App().settings().readPref<bool>("MelowGramMercury", false);

		if ((onMove && e->type() == QEvent::MouseMove) || ((onClick || onMercury) && e->type() == QEvent::MouseButtonPress)) {
			QMouseEvent *me = static_cast<QMouseEvent*>(e);
			
			// Only spawn if within our window
			if (window() && window()->windowHandle() && body()) {
				QPoint globalPos = me->globalPos();
				if (window()->geometry().contains(globalPos)) {
					QPoint localPos = body()->mapFromGlobal(globalPos);

					if (e->type() == QEvent::MouseButtonPress && onMercury) {
						MelowRipple r;
						r.pos = localPos;
						r.life = 1.0f;
						_melowRipples.push_back(r);
						if (_melowRipples.size() > 10) {
							_melowRipples.erase(_melowRipples.begin());
						}
						if (!_melowParticlesTimer.isActive()) {
							_melowParticlesTimer.callEach(16);
						}
					}

					if ((onMove && e->type() == QEvent::MouseMove) || (onClick && e->type() == QEvent::MouseButtonPress)) {
						MelowParticle p;
						p.pos = localPos;
						
						// Random velocity
						float speedMul = Core::App().settings().readPref<int>("MelowGramParticlesSpeed", 30) / 30.0f;
						float vx = ((std::rand() % 100 - 50) / 25.0f) * speedMul;
						float vy = ((std::rand() % 100 - 50) / 25.0f) * speedMul;
						p.velocity = QPointF(vx, vy);
						
						float sizeMul = Core::App().settings().readPref<int>("MelowGramParticlesSize", 15) / 15.0f;
						p.size = (2.0f + (std::rand() % 20) / 10.0f) * sizeMul;
						
						// Theme color
						p.color = st::windowBgActive->c;

						_melowParticles.push_back(p);
						int maxCount = Core::App().settings().readPref<int>("MelowGramParticlesCount", 100);
						if (_melowParticles.size() > maxCount) {
							_melowParticles.erase(_melowParticles.begin());
						}

						if (!_melowParticlesTimer.isActive()) {
							_melowParticlesTimer.callEach(16); // ~60fps
						}
					}
				}
			}
		}
	}
	return Ui::RpWindow::eventFilter(obj, e);
}

} // namespace Window
