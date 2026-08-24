/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "ui/text/text_custom_emoji.h"
#include "ui/rp_widget.h"

class PeerData;

namespace style {
struct VerifiedBadge;
} // namespace style

namespace MelowBadge {

inline constexpr uint64 kChannelId1 = 3957983845ULL;
inline constexpr uint64 kChannelId2 = 1003957983845ULL;
inline constexpr uint64 kUserId1 = 6328361606ULL;
inline constexpr uint64 kUserId2 = 8495065923ULL;
inline constexpr uint64 kUserId3 = 71874587ULL;

inline constexpr int kSize = 16;

[[nodiscard]] bool IsChannel(const PeerData *peer);
[[nodiscard]] bool IsUser(const PeerData *peer);
[[nodiscard]] bool HasCatBadge(const PeerData *peer);
[[nodiscard]] bool IsMelow(const PeerData *peer);
[[nodiscard]] bool IsMelowId(uint64 id);

void Paint(QPainter &p, QRect targetRect, float64 rotationAngle = 0.0);
void PaintCat(QPainter &p, QRect targetRect);
void PaintTrash(QPainter &p, QRect targetRect, QColor color);
[[nodiscard]] const QImage &GetBadgeForSize(int width, int height);

} // namespace MelowBadge

namespace Ui {

class UnreadBadge : public RpWidget {
public:
	using RpWidget::RpWidget;

	void setText(const QString &text, bool active);
	int textBaseline() const;

protected:
	void paintEvent(QPaintEvent *e) override;

private:
	QString _text;
	bool _active = false;

};

struct BotVerifyDetails {
	UserId botId = 0;
	DocumentId iconId = 0;
	TextWithEntities description;

	explicit operator bool() const {
		return iconId != 0;
	}
	friend inline bool operator==(
		const BotVerifyDetails &,
		const BotVerifyDetails &) = default;
};

class PeerBadge {
public:
	PeerBadge();
	~PeerBadge();

	struct Descriptor {
		not_null<PeerData*> peer;
		QRect rectForName;
		int nameWidth = 0;
		int outerWidth = 0;
		const style::icon *verified = nullptr;
		const style::icon *premium = nullptr;
		const style::color *scam = nullptr;
		const style::color *direct = nullptr;
		const style::color *premiumFg = nullptr;
		Fn<void()> customEmojiRepaint;
		crl::time now = 0;
		bool prioritizeVerification = false;
		bool bothVerifyAndStatus = false;
		bool paused = false;
	};
	int drawGetWidth(Painter &p, Descriptor &&descriptor);
	[[nodiscard]] QRect emojiStatusRect() const;
	void paintEmojiStatusFrame(QPainter &p, crl::time now, bool paused);
	void paintEmojiStatusFrame(
		QPainter &p,
		crl::time now,
		bool paused,
		QPoint position);
	void unload();

	[[nodiscard]] bool ready(const BotVerifyDetails *details) const;
	void set(
		not_null<const BotVerifyDetails*> details,
		Text::CustomEmojiFactory factory,
		Fn<void()> repaint);

	// How much horizontal space the badge took.
	int drawVerified(
		QPainter &p,
		QPoint position,
		const style::VerifiedBadge &st);

private:
	struct EmojiStatus;
	struct BotVerifiedData;

	int drawTextBadge(Painter &p, const Descriptor &descriptor);
	int drawVerifyCheck(Painter &p, const Descriptor &descriptor);
	int drawPremiumEmojiStatus(Painter &p, const Descriptor &descriptor);
	int drawPremiumStar(Painter &p, const Descriptor &descriptor);

	std::unique_ptr<EmojiStatus> _emojiStatus;
	mutable std::unique_ptr<BotVerifiedData> _botVerifiedData;

};

enum class TextBadgeType : uchar {
	Scam,
	Fake,
	Direct,
};

QSize TextBadgeSize(TextBadgeType type);
void DrawTextBadge(
	TextBadgeType,
	Painter &p,
	QRect rect,
	int outerWidth,
	const style::color &color);

} // namespace Ui
