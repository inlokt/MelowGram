/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
/*
 * modified for melowgram 23.07.2026
 */
#include "ui/unread_badge.h"

#include "data/data_channel.h"
#include "data/data_emoji_statuses.h"
#include "data/data_peer.h"
#include "data/data_user.h"
#include "data/data_session.h"
#include "data/stickers/data_custom_emoji.h"
#include "main/main_session.h"
#include "lang/lang_keys.h"
#include "ui/painter.h"
#include "ui/rect.h"
#include "ui/power_saving.h"
#include "ui/text/text_custom_emoji.h"
#include "ui/unread_badge_paint.h"
#include "styles/style_dialogs.h"
#include <QtSvg/QSvgRenderer>

namespace MelowBadge {

bool IsChannel(const PeerData *peer) {
	if (!peer || !peer->isChannel()) return false;
	const auto bare = peerToChannel(peer->id).bare;
	return (bare == kChannelId1 || bare == kChannelId2);
}

bool IsUser(const PeerData *peer) {
	if (!peer || !peer->isUser()) return false;
	const auto bare = peerToUser(peer->id).bare;
	return (bare == kUserId1 || bare == kUserId2 || bare == kUserId3);
}

bool HasCatBadge(const PeerData *peer) {
	if (!peer || !peer->isUser()) return false;
	const auto bare = peerToUser(peer->id).bare;
	return (bare == kUserId2);
}

bool IsMelow(const PeerData *peer) {
	return IsChannel(peer) || IsUser(peer);
}

bool IsMelowId(uint64 id) {
	return (id == kChannelId1 || id == kChannelId2 || id == kUserId1 || id == kUserId2 || id == kUserId3);
}

[[nodiscard]] const QImage &MasterBadgeImage() {
	static const auto image = [] {
		constexpr auto kCanvasSize = 512;
		auto result = QImage(kCanvasSize, kCanvasSize, QImage::Format_ARGB32_Premultiplied);
		result.fill(Qt::transparent);

		QFile file(u":/gui/melow/badge_logotype2.svg"_q);
		if (!file.open(QIODevice::ReadOnly)) {
			return result;
		}
		const auto content = QString::fromUtf8(file.readAll());
		file.close();

		const auto pathIdx = content.indexOf(u"<path "_q);
		const auto pathEnd = (pathIdx != -1) ? content.indexOf(u"/>"_q, pathIdx) : -1;
		if (pathIdx != -1 && pathEnd != -1) {
			const auto pathElement = content.mid(pathIdx, pathEnd - pathIdx + 2);
			const auto miniSvg = u"<svg viewBox=\"0 0 300 300\" xmlns=\"http://www.w3.org/2000/svg\">"_q
				+ pathElement
				+ u"</svg>"_q;
			QSvgRenderer starRenderer(miniSvg.toUtf8());
			if (starRenderer.isValid()) {
				QPainter p(&result);
				p.setRenderHint(QPainter::Antialiasing, true);
				p.setRenderHint(QPainter::SmoothPixmapTransform, true);
				starRenderer.render(&p, QRectF(0, 0, kCanvasSize, kCanvasSize));
			}
		}

		const auto base64Prefix = u"xlink:href=\"data:image/png;base64,"_q;
		const auto base64Idx = content.indexOf(base64Prefix);
		if (base64Idx != -1) {
			const auto start = base64Idx + base64Prefix.size();
			const auto end = content.indexOf(u"\""_q, start);
			if (end != -1) {
				const auto base64Data = content.mid(start, end - start).toLatin1();
				const auto rawImage = QImage::fromData(QByteArray::fromBase64(base64Data));
				if (!rawImage.isNull()) {
					constexpr auto maskX = 12.1277;
					constexpr auto maskY = 24.2553;
					constexpr auto maskW = 275.745;
					constexpr auto maskH = 275.745;
					const auto fillColor = QColor(0x84, 0x8C, 0xB1);

					const auto scale = kCanvasSize / 300.0;
					const auto targetRect = QRectF(maskX * scale, maskY * scale, maskW * scale, maskH * scale);

					auto maskImage = QImage(kCanvasSize, kCanvasSize, QImage::Format_ARGB32_Premultiplied);
					maskImage.fill(Qt::transparent);
					{
						QPainter p(&maskImage);
						p.setRenderHint(QPainter::Antialiasing, true);
						p.setRenderHint(QPainter::SmoothPixmapTransform, true);
						p.drawImage(targetRect, rawImage);
						p.setCompositionMode(QPainter::CompositionMode_SourceIn);
						p.fillRect(QRect(0, 0, kCanvasSize, kCanvasSize), fillColor);
					}

					QPainter p(&result);
					p.setRenderHint(QPainter::Antialiasing, true);
					p.setRenderHint(QPainter::SmoothPixmapTransform, true);
					p.drawImage(0, 0, maskImage);
				}
			}
		} else {
			QSvgRenderer fullSvg(content.toUtf8());
			if (fullSvg.isValid()) {
				QPainter p(&result);
				p.setRenderHint(QPainter::Antialiasing, true);
				p.setRenderHint(QPainter::SmoothPixmapTransform, true);
				fullSvg.render(&p, QRectF(0, 0, kCanvasSize, kCanvasSize));
			}
		}

		return result;
	}();
	return image;
}

[[nodiscard]] const QImage &GetBadgeForSize(int width, int height) {
	static base::flat_map<int, QImage> cache;
	const auto key = (width << 16) | (height & 0xFFFF);
	auto it = cache.find(key);
	if (it != cache.end()) {
		return it->second;
	}
	const auto &master = MasterBadgeImage();
	auto scaled = master.scaled(
		width,
		height,
		Qt::IgnoreAspectRatio,
		Qt::SmoothTransformation);
	return cache.emplace(key, std::move(scaled)).first->second;
}

void Paint(QPainter &p, QRect targetRect, float64 rotationAngle) {
	p.save();
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);

	const auto ratio = style::DevicePixelRatio();
	const auto pxW = std::max(int(std::round(targetRect.width() * ratio)), 1);
	const auto pxH = std::max(int(std::round(targetRect.height() * ratio)), 1);

	const auto &img = GetBadgeForSize(pxW, pxH);
	if (!img.isNull()) {
		p.drawImage(targetRect, img);
	}

	p.restore();
}

[[nodiscard]] const QImage &CatBadgeImage() {
	static const auto image = [] {
		constexpr auto kCanvasSize = 512;
		auto result = QImage(kCanvasSize, kCanvasSize, QImage::Format_ARGB32_Premultiplied);
		result.fill(Qt::transparent);

		QSvgRenderer renderer(u":/gui/melow/badge_cat.svg"_q);
		if (renderer.isValid()) {
			QPainter p(&result);
			p.setRenderHint(QPainter::Antialiasing, true);
			p.setRenderHint(QPainter::SmoothPixmapTransform, true);
			renderer.render(&p, QRectF(0, 0, kCanvasSize, kCanvasSize));
		}
		return result;
	}();
	return image;
}

[[nodiscard]] const QImage &GetCatBadgeForSize(int width, int height) {
	static base::flat_map<int, QImage> cache;
	const auto key = (width << 16) | (height & 0xFFFF);
	auto it = cache.find(key);
	if (it != cache.end()) {
		return it->second;
	}
	const auto &master = CatBadgeImage();
	auto scaled = master.scaled(
		width,
		height,
		Qt::IgnoreAspectRatio,
		Qt::SmoothTransformation);
	return cache.emplace(key, std::move(scaled)).first->second;
}

void PaintCat(QPainter &p, QRect targetRect) {
	p.save();
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);

	const auto ratio = style::DevicePixelRatio();
	const auto pxW = std::max(int(std::round(targetRect.width() * ratio)), 1);
	const auto pxH = std::max(int(std::round(targetRect.height() * ratio)), 1);

	const auto &img = GetCatBadgeForSize(pxW, pxH);
	if (!img.isNull()) {
		p.drawImage(targetRect, img);
	}

	p.restore();
}

[[nodiscard]] const QImage &TrashBadgeImage() {
	static const auto image = [] {
		constexpr auto kCanvasSize = 512;
		auto result = QImage(kCanvasSize, kCanvasSize, QImage::Format_ARGB32_Premultiplied);
		result.fill(Qt::transparent);

		QSvgRenderer renderer(u":/gui/melow/melowgui/trash.svg"_q);
		if (renderer.isValid()) {
			QPainter p(&result);
			p.setRenderHint(QPainter::Antialiasing, true);
			p.setRenderHint(QPainter::SmoothPixmapTransform, true);
			renderer.render(&p, QRectF(0, 0, kCanvasSize, kCanvasSize));
		}
		return result;
	}();
	return image;
}

[[nodiscard]] const QImage &GetTrashBadgeForSize(int width, int height) {
	static base::flat_map<int, QImage> cache;
	const auto key = (width << 16) | (height & 0xFFFF);
	auto it = cache.find(key);
	if (it != cache.end()) {
		return it->second;
	}
	const auto &master = TrashBadgeImage();
	auto scaled = master.scaled(
		width,
		height,
		Qt::IgnoreAspectRatio,
		Qt::SmoothTransformation);
	return cache.emplace(key, std::move(scaled)).first->second;
}

void PaintTrash(QPainter &p, QRect targetRect, QColor color) {
	p.save();
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);

	const auto ratio = style::DevicePixelRatio();
	const auto pxW = std::max(int(std::round(targetRect.width() * ratio)), 1);
	const auto pxH = std::max(int(std::round(targetRect.height() * ratio)), 1);

	const auto &baseImg = GetTrashBadgeForSize(pxW, pxH);
	if (!baseImg.isNull()) {
		auto colored = baseImg;
		{
			QPainter cp(&colored);
			cp.setCompositionMode(QPainter::CompositionMode_SourceIn);
			cp.fillRect(colored.rect(), color);
		}
		p.drawImage(targetRect, colored);
	}

	p.restore();
}

} // namespace MelowBadge

namespace Ui {
namespace {

constexpr auto kPlayStatusLimit = 2;
constexpr auto kBotVerifiedScale = 0.88;

class ScaledBotVerifiedEmoji final : public Ui::Text::CustomEmoji {
public:
	ScaledBotVerifiedEmoji(
		std::unique_ptr<Ui::Text::CustomEmoji> wrapped,
		int innerSize,
		int outerSize);

	int width() override;
	QString entityData() override;
	void paint(QPainter &p, const Context &context) override;
	void unload() override;
	bool ready() override;
	bool readyInDefaultState() override;

private:
	const std::unique_ptr<Ui::Text::CustomEmoji> _wrapped;
	const int _innerSize = 0;
	const int _outerSize = 0;
	QImage _frame;
	QColor _frameColor;

};

ScaledBotVerifiedEmoji::ScaledBotVerifiedEmoji(
	std::unique_ptr<Ui::Text::CustomEmoji> wrapped,
	int innerSize,
	int outerSize)
: _wrapped(std::move(wrapped))
, _innerSize(innerSize)
, _outerSize(outerSize) {
}

int ScaledBotVerifiedEmoji::width() {
	return _outerSize;
}

QString ScaledBotVerifiedEmoji::entityData() {
	return _wrapped->entityData();
}

void ScaledBotVerifiedEmoji::paint(QPainter &p, const Context &context) {
	if (_frame.isNull() || _frameColor != context.textColor) {
		if (!_wrapped->ready()) {
			return;
		}
		const auto ratio = style::DevicePixelRatio();
		const auto sourcePx = Data::FrameSizeFromTag(
			Data::CustomEmojiSizeTag::Isolated);
		_frame = QImage(
			QSize(sourcePx, sourcePx),
			QImage::Format_ARGB32_Premultiplied);
		_frame.setDevicePixelRatio(ratio);
		_frame.fill(Qt::transparent);

		auto painter = QPainter(&_frame);
		painter.translate(-context.position);
		const auto was = context.internal.forceFirstFrame;
		context.internal.forceFirstFrame = true;
		_wrapped->paint(painter, context);
		context.internal.forceFirstFrame = was;
		painter.end();

		_frame = _frame.scaled(
			QSize(_innerSize, _innerSize) * ratio,
			Qt::IgnoreAspectRatio,
			Qt::SmoothTransformation);
		_frameColor = context.textColor;
	}
	const auto skip = (_outerSize - _innerSize) / 2;
	p.drawImage(context.position + QPoint(skip, skip), _frame);
}

void ScaledBotVerifiedEmoji::unload() {
	_wrapped->unload();
}

bool ScaledBotVerifiedEmoji::ready() {
	return !_frame.isNull() || _wrapped->ready();
}

bool ScaledBotVerifiedEmoji::readyInDefaultState() {
	return !_frame.isNull() || _wrapped->ready();
}

} // namespace

struct PeerBadge::EmojiStatus {
	EmojiStatusId id;
	std::unique_ptr<Ui::Text::CustomEmoji> emoji;
	QPoint lastPosition;
	QColor lastColor;
	int skip = 0;
};

struct PeerBadge::BotVerifiedData {
	QImage cache;
	std::unique_ptr<Text::CustomEmoji> icon;
};

void UnreadBadge::setText(const QString &text, bool active) {
	_text = text;
	_active = active;
	const auto st = Dialogs::Ui::UnreadBadgeStyle();
	resize(
		std::max(st.font->width(text) + 2 * st.padding, st.size),
		st.size);
	update();
}

int UnreadBadge::textBaseline() const {
	const auto st = Dialogs::Ui::UnreadBadgeStyle();
	return ((st.size - st.font->height) / 2) + st.font->ascent;
}

void UnreadBadge::paintEvent(QPaintEvent *e) {
	if (_text.isEmpty()) {
		return;
	}

	auto p = QPainter(this);

	UnreadBadgeStyle unreadSt;
	unreadSt.muted = !_active;
	auto unreadRight = width();
	auto unreadTop = 0;
	PaintUnreadBadge(
		p,
		_text,
		unreadRight,
		unreadTop,
		unreadSt);
}

QString TextBadgeText(TextBadgeType type) {
	switch (type) {
	case TextBadgeType::Fake: return tr::lng_fake_badge(tr::now);
	case TextBadgeType::Scam: return tr::lng_scam_badge(tr::now);
	case TextBadgeType::Direct: return tr::lng_direct_badge(tr::now);
	}
	Unexpected("Type in TextBadgeText.");
}

QSize TextBadgeSize(TextBadgeType type) {
	const auto phrase = TextBadgeText(type);
	const auto phraseWidth = st::dialogsScamFont->width(phrase);
	const auto width = st::dialogsScamPadding.left()
		+ phraseWidth
		+ st::dialogsScamPadding.right();
	const auto height = st::dialogsScamPadding.top()
		+ st::dialogsScamFont->height
		+ st::dialogsScamPadding.bottom();
	return { width, height };
}

void DrawTextBadge(
		Painter &p,
		QRect rect,
		int outerWidth,
		const style::color &color,
		const QString &phrase,
		int phraseWidth) {
	PainterHighQualityEnabler hq(p);
	auto pen = color->p;
	pen.setWidth(st::lineWidth);
	p.setPen(pen);
	p.setBrush(Qt::NoBrush);
	p.drawRoundedRect(rect, st::dialogsScamRadius, st::dialogsScamRadius);
	p.setFont(st::dialogsScamFont);
	if (style::DevicePixelRatio() > 1) {
		p.drawText(
			QRect(
				rect.x() + st::dialogsScamPadding.left(),
				rect.y() + st::dialogsScamPadding.top(),
				rect.width() - rect::m::sum::h(st::dialogsScamPadding),
				rect.height() - rect::m::sum::v(st::dialogsScamPadding)),
			Qt::AlignCenter,
			phrase);
	} else {
		p.drawTextLeft(
			rect.x() + st::dialogsScamPadding.left(),
			rect.y() + st::dialogsScamPadding.top(),
			outerWidth,
			phrase,
			phraseWidth);
	}
}

void DrawTextBadge(
		TextBadgeType type,
		Painter &p,
		QRect rect,
		int outerWidth,
		const style::color &color) {
	const auto phrase = TextBadgeText(type);
	DrawTextBadge(
		p,
		rect,
		outerWidth,
		color,
		phrase,
		st::dialogsScamFont->width(phrase));
}

PeerBadge::PeerBadge() = default;

PeerBadge::~PeerBadge() = default;

int PeerBadge::drawGetWidth(Painter &p, Descriptor &&descriptor) {
	Expects(descriptor.customEmojiRepaint != nullptr);

	const auto peer = descriptor.peer;
	if ((descriptor.scam && (peer->isScam() || peer->isFake()))
		|| (descriptor.direct && peer->isMonoforum())) {
		return drawTextBadge(p, descriptor);
	}

	const auto isMelowUser = MelowBadge::IsUser(peer);
	const auto isMelowChannel = MelowBadge::IsChannel(peer);
	auto melowWidth = 0;

	if (isMelowUser) {
		const auto rectForName = descriptor.rectForName;
		const auto s = MelowBadge::kSize;
		const auto iconx = rectForName.x();
		const auto icony = rectForName.y() + (rectForName.height() - s) / 2 + 1;
		MelowBadge::Paint(p, QRect(iconx, icony, s, s));
		melowWidth = s + 6;
	}

	const auto verifyCheck = descriptor.verified && peer->isVerified();
	const auto premiumMark = descriptor.premium
		&& peer->session().premiumBadgesShown();
	const auto emojiStatus = premiumMark
		&& peer->emojiStatusId()
		&& (peer->isPremium() || peer->isChannel());
	const auto premiumStar = premiumMark
		&& !emojiStatus
		&& peer->isPremium();

	const auto paintVerify = verifyCheck
		&& (descriptor.prioritizeVerification
			|| descriptor.bothVerifyAndStatus
			|| !emojiStatus);
	const auto paintEmoji = emojiStatus
		&& (!paintVerify || descriptor.bothVerifyAndStatus);
	const auto paintStar = premiumStar && !paintVerify;

	auto rightDescriptor = descriptor;
	if (isMelowUser) {
		rightDescriptor.rectForName.setLeft(descriptor.rectForName.left() + melowWidth);
		rightDescriptor.rectForName.setWidth(descriptor.rectForName.width() - melowWidth);
	}

	auto result = 0;
	if (paintEmoji) {
		auto &rectForName = rightDescriptor.rectForName;
		const auto verifyWidth = rightDescriptor.verified->width();
		if (paintVerify) {
			rectForName.setWidth(rectForName.width() - verifyWidth);
		}
		result += drawPremiumEmojiStatus(p, rightDescriptor);
		if (!paintVerify) {
			// Done with verify
		} else {
			rectForName.setWidth(rectForName.width() + verifyWidth);
			rightDescriptor.nameWidth += result;
		}
	}
	if (paintVerify) {
		result += drawVerifyCheck(p, rightDescriptor);
	} else if (paintStar) {
		result += drawPremiumStar(p, rightDescriptor);
	}

	if (isMelowChannel) {
		const auto rectForName = rightDescriptor.rectForName;
		const auto s = MelowBadge::kSize;
		const auto iconx = rectForName.x() + qMin(rightDescriptor.nameWidth, rectForName.width() - s - result) + result + 6;
		const auto icony = rectForName.y() + (rectForName.height() - s) / 2 + 1;
		MelowBadge::Paint(p, QRect(iconx, icony, s, s));
		melowWidth = s + 6;
	}

	return melowWidth + result;
}

int PeerBadge::drawTextBadge(Painter &p, const Descriptor &descriptor) {
	const auto type = [&] {
		if (descriptor.peer->isScam()) {
			return TextBadgeType::Scam;
		} else if (descriptor.peer->isFake()) {
			return TextBadgeType::Fake;
		}
		return TextBadgeType::Direct;
	}();
	const auto phrase = TextBadgeText(type);
	const auto phraseWidth = st::dialogsScamFont->width(phrase);
	const auto width = st::dialogsScamPadding.left()
		+ phraseWidth
		+ st::dialogsScamPadding.right();
	const auto height = st::dialogsScamPadding.top()
		+ st::dialogsScamFont->height
		+ st::dialogsScamPadding.bottom();
	const auto rectForName = descriptor.rectForName;
	const auto rect = QRect(
		(rectForName.x()
			+ qMin(
				descriptor.nameWidth + st::dialogsScamSkip,
				rectForName.width() - width)),
		rectForName.y() + (rectForName.height() - height) / 2,
		width,
		height);
	DrawTextBadge(
		p,
		rect,
		descriptor.outerWidth,
		*((type == TextBadgeType::Direct)
			? descriptor.direct
			: descriptor.scam),
		phrase,
		phraseWidth);
	return st::dialogsScamSkip + width;
}

int PeerBadge::drawVerifyCheck(Painter &p, const Descriptor &descriptor) {
	const auto iconw = descriptor.verified->width();
	const auto rectForName = descriptor.rectForName;
	const auto nameWidth = descriptor.nameWidth;
	descriptor.verified->paint(
		p,
		rectForName.x() + qMin(nameWidth, rectForName.width() - iconw),
		rectForName.y(),
		descriptor.outerWidth);
	return iconw;
}

int PeerBadge::drawPremiumEmojiStatus(
		Painter &p,
		const Descriptor &descriptor) {
	const auto peer = descriptor.peer;
	const auto id = peer->emojiStatusId();
	const auto rectForName = descriptor.rectForName;
	const auto iconw = descriptor.premium->width();
	const auto iconx = rectForName.x()
		+ qMin(descriptor.nameWidth, rectForName.width() - iconw);
	const auto icony = rectForName.y();
	if (!_emojiStatus) {
		_emojiStatus = std::make_unique<EmojiStatus>();
		const auto size = st::emojiSize;
		const auto emoji = Ui::Text::AdjustCustomEmojiSize(size);
		_emojiStatus->skip = (size - emoji) / 2;
	}
	if (_emojiStatus->id != id) {
		using namespace Ui::Text;
		auto &manager = peer->session().data().customEmojiManager();
		_emojiStatus->id = id;
		_emojiStatus->emoji = MakeWrappedEmoji<LimitedLoopsEmoji>(
			manager.create(
				Data::EmojiStatusCustomId(id),
				descriptor.customEmojiRepaint),
			kPlayStatusLimit);
	}
	if (!_emojiStatus->emoji) {
		return 0;
	}
	_emojiStatus->lastPosition = QPoint(
		iconx - 2 * _emojiStatus->skip,
		icony + _emojiStatus->skip);
	_emojiStatus->lastColor = (*descriptor.premiumFg)->c;
	_emojiStatus->emoji->paint(p, {
		.textColor = _emojiStatus->lastColor,
		.now = descriptor.now,
		.position = _emojiStatus->lastPosition,
		.paused = descriptor.paused || On(PowerSaving::kEmojiStatus),
	});
	return iconw - 4 * _emojiStatus->skip;
}

int PeerBadge::drawPremiumStar(Painter &p, const Descriptor &descriptor) {
	const auto rectForName = descriptor.rectForName;
	const auto iconw = descriptor.premium->width();
	const auto iconx = rectForName.x()
		+ qMin(descriptor.nameWidth, rectForName.width() - iconw);
	const auto icony = rectForName.y();
	_emojiStatus = nullptr;
	descriptor.premium->paint(p, iconx, icony, descriptor.outerWidth);
	return iconw;
}

QRect PeerBadge::emojiStatusRect() const {
	if (!_emojiStatus || !_emojiStatus->emoji) {
		return QRect();
	}
	return QRect(
		_emojiStatus->lastPosition,
		Size(st::emojiSize - 2 * _emojiStatus->skip));
}

void PeerBadge::paintEmojiStatusFrame(
		QPainter &p,
		crl::time now,
		bool paused) {
	if (!_emojiStatus || !_emojiStatus->emoji) {
		return;
	}
	paintEmojiStatusFrame(p, now, paused, _emojiStatus->lastPosition);
}

void PeerBadge::paintEmojiStatusFrame(
		QPainter &p,
		crl::time now,
		bool paused,
		QPoint position) {
	if (!_emojiStatus || !_emojiStatus->emoji) {
		return;
	}
	_emojiStatus->emoji->paint(p, {
		.textColor = _emojiStatus->lastColor,
		.now = now,
		.position = position,
		.paused = paused || On(PowerSaving::kEmojiStatus),
	});
}

void PeerBadge::unload() {
	_emojiStatus = nullptr;
}

bool PeerBadge::ready(const BotVerifyDetails *details) const {
	if (!details || !*details) {
		_botVerifiedData = nullptr;
		return true;
	} else if (!_botVerifiedData) {
		return false;
	}
	if (!details->iconId) {
		_botVerifiedData->icon = nullptr;
	} else if (!_botVerifiedData->icon
		|| (_botVerifiedData->icon->entityData()
			!= Data::SerializeCustomEmojiId(details->iconId))) {
		return false;
	}
	return true;
}

void PeerBadge::set(
		not_null<const BotVerifyDetails*> details,
		Ui::Text::CustomEmojiFactory factory,
		Fn<void()> repaint) {
	if (!_botVerifiedData) {
		_botVerifiedData = std::make_unique<BotVerifiedData>();
	}
	if (details->iconId) {
		const auto outer = st::emojiSize;
		const auto inner = int(base::SafeRound(
			st::emojiSize * kBotVerifiedScale));
		_botVerifiedData->icon = MakeWrappedEmoji<ScaledBotVerifiedEmoji>(
			factory(
				Data::SerializeCustomEmojiId(details->iconId),
				{ .repaint = repaint }),
			inner,
			outer);
	}
}

int PeerBadge::drawVerified(
		QPainter &p,
		QPoint position,
		const style::VerifiedBadge &st) {
	const auto data = _botVerifiedData.get();
	if (!data) {
		return 0;
	}
	if (const auto icon = data->icon.get()) {
		icon->paint(p, {
			.textColor = st.color->c,
			.now = crl::now(),
			.position = position + st.position,
		});
		return icon->width();
	}
	return 0;
}

} // namespace Ui
