#pragma once

#include "base/basic_types.h"
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtGui/QColor>
#include <rpl/producer.h>
#include <rpl/event_stream.h>
#include <vector>
#include <map>
#include <optional>

class DocumentData;

namespace Main {
class Session;
} // namespace Main

namespace Melow {

struct LocalDocumentInfo {
	uint64 id = 0;
	uint64 accessHash = 0;
	QByteArray fileReference;
	int32 dc = 0;
	int64 size = 0;
	QString mime;
	int width = 512;
	int height = 512;
	QString alt;
	bool isCustomEmoji = false;
};

struct LocalGift {
	uint64 id = 0;
	uint64 initialGiftId = 0;
	QString title;
	int number = 1;
	int64 stars = 0;
	TimeId date = 0;
	PeerId fromId = 0;
	PeerId toId = 0;
	QString fromName;
	QString message;
	bool isUnique = true;
	uint64 modelDocId = 0;
	uint64 patternDocId = 0;
	uint32 backdropId = 0;
	int centerColor = 0;
	int edgeColor = 0;
	int patternColor = 0;
	int textColor = 0;
	QString slug;
	QString patternName;
	QString backdropName;
	LocalDocumentInfo modelDoc;
	LocalDocumentInfo patternDoc;
};

class LocalServer final {
public:
	static LocalServer &Instance();

	[[nodiscard]] bool isEnabled() const;
	void setEnabled(bool enabled);

	[[nodiscard]] int64 stars() const;
	void setStars(int64 stars);
	void addStars(int64 delta);

	[[nodiscard]] bool isAnonymousNumberEnabled() const;
	void setAnonymousNumberEnabled(bool enabled);
	[[nodiscard]] QString anonymousNumber() const;
	void setAnonymousNumber(const QString &number);

	[[nodiscard]] bool isNftUsernamesEnabled() const;
	void setNftUsernamesEnabled(bool enabled);
	[[nodiscard]] const std::vector<QString> &nftUsernames() const;
	void addNftUsername(const QString &username);
	void removeNftUsername(const QString &username);
	void setNftUsernames(const std::vector<QString> &list);
	void setNftUsernamesFromRaw(const QString &raw);

	void save();
	void load();

	[[nodiscard]] const std::vector<LocalGift> &gifts() const;
	void addGift(LocalGift gift);
	bool buyGift(
		uint64 giftId,
		const QString &title,
		int number,
		int price,
		PeerId toPeerId,
		const QString &msg = QString(),
		uint64 modelDocId = 0,
		uint64 patternDocId = 0,
		uint32 backdropId = 0,
		int centerColor = 0,
		int edgeColor = 0,
		int patternColor = 0);
	bool buyGiftBySlug(
		const QString &slug,
		int price,
		PeerId toPeerId,
		const QString &msg = QString());

	void registerStarGift(
		uint64 giftId,
		DocumentData *doc,
		const QString &title,
		int64 price);

	void registerStarGift(
		uint64 giftId,
		uint64 docId,
		const QString &title,
		int64 price,
		uint64 modelDocId = 0,
		uint64 patternDocId = 0,
		uint32 backdropId = 0,
		int centerColor = 0,
		int edgeColor = 0,
		int patternColor = 0);

	void registerUniqueGift(
		uint64 id,
		const QString &slug,
		const QString &title,
		int number,
		DocumentData *modelDoc,
		DocumentData *patternDoc,
		uint32 backdropId,
		int centerColor,
		int edgeColor,
		int patternColor,
		const QString &patternName = QString(),
		const QString &backdropName = QString());

	void registerUniqueGift(
		uint64 id,
		const QString &slug,
		const QString &title,
		int number,
		uint64 modelDocId,
		uint64 patternDocId,
		uint32 backdropId,
		int centerColor,
		int edgeColor,
		int patternColor,
		const QString &patternName = QString(),
		const QString &backdropName = QString());

	[[nodiscard]] not_null<DocumentData*> ensureModelDocument(
		not_null<Main::Session*> session,
		const LocalGift &gift) const;
	[[nodiscard]] not_null<DocumentData*> ensurePatternDocument(
		not_null<Main::Session*> session,
		const LocalGift &gift) const;

	[[nodiscard]] std::optional<LocalGift> findGiftBySlug(const QString &slug) const;
	[[nodiscard]] std::optional<LocalGift> findKnownGift(uint64 giftId) const;

	[[nodiscard]] rpl::producer<bool> enabledValue() const;
	[[nodiscard]] rpl::producer<int64> starsValue() const;
	[[nodiscard]] rpl::producer<> giftsChanged() const;
	[[nodiscard]] rpl::producer<> profileChanged() const;

private:
	LocalServer();
	~LocalServer() = default;

	[[nodiscard]] QString filePath() const;

	bool _enabled = false;
	int64 _stars = 0;
	std::vector<LocalGift> _gifts;

	bool _anonymousNumberEnabled = false;
	QString _anonymousNumber = u"+888 8888 8888"_q;

	bool _nftUsernamesEnabled = false;
	std::vector<QString> _nftUsernames;

	std::map<uint64, LocalGift> _knownGifts;
	std::map<QString, LocalGift> _knownUniqueGifts;

	rpl::event_stream<bool> _enabledChanges;
	rpl::event_stream<int64> _starsChanges;
	rpl::event_stream<> _giftsChanges;
	rpl::event_stream<> _profileChanges;
};

} // namespace Melow

