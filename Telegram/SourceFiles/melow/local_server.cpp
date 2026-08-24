#include "melow/local_server.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QCoreApplication>
#include "base/algorithm.h"
#include "base/unixtime.h"
#include "storage/localstorage.h"
#include "data/data_session.h"
#include "data/data_document.h"
#include "main/main_session.h"
#include "scheme.h"

namespace Melow {
namespace {

LocalDocumentInfo MakeDocumentInfo(DocumentData *doc, bool isCustomEmoji) {
	if (!doc) {
		return {};
	}
	uint64 accessHash = 0;
	const auto input = doc->mtpInput();
	if (input.type() == mtpc_inputDocument) {
		accessHash = input.c_inputDocument().vaccess_hash().v;
	}
	return LocalDocumentInfo{
		.id = doc->id,
		.accessHash = accessHash,
		.fileReference = doc->fileReference(),
		.dc = 1,
		.size = doc->size,
		.mime = doc->mimeString(),
		.width = doc->dimensions.width() ? doc->dimensions.width() : 512,
		.height = doc->dimensions.height() ? doc->dimensions.height() : 512,
		.alt = (doc->sticker() ? doc->sticker()->alt : QString()),
		.isCustomEmoji = isCustomEmoji,
	};
}

} // namespace

LocalServer &LocalServer::Instance() {
	static LocalServer instance;
	return instance;
}

LocalServer::LocalServer() {
	load();
}

QString LocalServer::filePath() const {
	return cWorkingDir() + u"tdata/local_server.json"_q;
}

bool LocalServer::isEnabled() const {
	return _enabled;
}

void LocalServer::setEnabled(bool enabled) {
	if (_enabled != enabled) {
		_enabled = enabled;
		save();
		_enabledChanges.fire_copy(_enabled);
		_giftsChanges.fire({});
		_profileChanges.fire({});
	}
}

int64 LocalServer::stars() const {
	return _stars;
}

void LocalServer::setStars(int64 stars) {
	if (_stars != stars) {
		_stars = std::max(int64(0), stars);
		save();
		_starsChanges.fire_copy(_stars);
	}
}

void LocalServer::addStars(int64 delta) {
	setStars(_stars + delta);
}

bool LocalServer::isAnonymousNumberEnabled() const {
	return _anonymousNumberEnabled;
}

void LocalServer::setAnonymousNumberEnabled(bool enabled) {
	if (_anonymousNumberEnabled != enabled) {
		_anonymousNumberEnabled = enabled;
		save();
		_profileChanges.fire({});
	}
}

QString LocalServer::anonymousNumber() const {
	return _anonymousNumber;
}

void LocalServer::setAnonymousNumber(const QString &number) {
	if (_anonymousNumber != number) {
		_anonymousNumber = number;
		save();
		_profileChanges.fire({});
	}
}

bool LocalServer::isNftUsernamesEnabled() const {
	return _nftUsernamesEnabled;
}

void LocalServer::setNftUsernamesEnabled(bool enabled) {
	if (_nftUsernamesEnabled != enabled) {
		_nftUsernamesEnabled = enabled;
		save();
		_profileChanges.fire({});
	}
}

const std::vector<QString> &LocalServer::nftUsernames() const {
	return _nftUsernames;
}

void LocalServer::addNftUsername(const QString &username) {
	const auto trimmed = username.trimmed().remove('@');
	if (!trimmed.isEmpty() && !ranges::contains(_nftUsernames, trimmed)) {
		_nftUsernames.push_back(trimmed);
		save();
		_profileChanges.fire({});
	}
}

void LocalServer::removeNftUsername(const QString &username) {
	const auto trimmed = username.trimmed().remove('@');
	if (ranges::contains(_nftUsernames, trimmed)) {
		_nftUsernames.erase(
			std::remove(_nftUsernames.begin(), _nftUsernames.end(), trimmed),
			_nftUsernames.end());
		save();
		_profileChanges.fire({});
	}
}

void LocalServer::setNftUsernames(const std::vector<QString> &list) {
	_nftUsernames = list;
	save();
	_profileChanges.fire({});
}

void LocalServer::setNftUsernamesFromRaw(const QString &raw) {
	const auto parts = raw.split(QRegExp(u"[,;\\s]+"_q), Qt::SkipEmptyParts);
	auto list = std::vector<QString>();
	list.reserve(parts.size());
	for (const auto &p : parts) {
		const auto cleaned = p.trimmed().remove('@');
		if (!cleaned.isEmpty() && !ranges::contains(list, cleaned)) {
			list.push_back(cleaned);
		}
	}
	setNftUsernames(list);
}

const std::vector<LocalGift> &LocalServer::gifts() const {
	return _gifts;
}

void LocalServer::addGift(LocalGift gift) {
	_gifts.push_back(std::move(gift));
	save();
	_giftsChanges.fire({});
}

void LocalServer::registerStarGift(
		uint64 giftId,
		DocumentData *doc,
		const QString &title,
		int64 price) {
	if (!giftId) {
		return;
	}
	auto docInfo = MakeDocumentInfo(doc, false);
	_knownGifts[giftId] = LocalGift{
		.id = giftId,
		.initialGiftId = giftId,
		.title = title,
		.stars = price,
		.modelDocId = docInfo.id,
		.modelDoc = std::move(docInfo),
	};
}

void LocalServer::registerStarGift(
		uint64 giftId,
		uint64 docId,
		const QString &title,
		int64 price,
		uint64 modelDocId,
		uint64 patternDocId,
		uint32 backdropId,
		int centerColor,
		int edgeColor,
		int patternColor) {
	if (!giftId) {
		return;
	}
	const auto mId = (modelDocId ? modelDocId : docId);
	_knownGifts[giftId] = LocalGift{
		.id = giftId,
		.initialGiftId = giftId,
		.title = title,
		.stars = price,
		.modelDocId = mId,
		.patternDocId = patternDocId,
		.backdropId = backdropId,
		.centerColor = centerColor,
		.edgeColor = edgeColor,
		.patternColor = patternColor,
		.modelDoc = { .id = mId, .width = 512, .height = 512 },
		.patternDoc = { .id = patternDocId, .width = 512, .height = 512, .isCustomEmoji = true },
	};
}

void LocalServer::registerUniqueGift(
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
		const QString &patternName,
		const QString &backdropName) {
	if (slug.isEmpty()) {
		return;
	}
	auto modelInfo = MakeDocumentInfo(modelDoc, false);
	auto patternInfo = MakeDocumentInfo(patternDoc, true);
	_knownUniqueGifts[slug] = LocalGift{
		.id = id,
		.initialGiftId = id,
		.title = title,
		.number = number,
		.isUnique = true,
		.modelDocId = modelInfo.id,
		.patternDocId = patternInfo.id,
		.backdropId = backdropId,
		.centerColor = centerColor,
		.edgeColor = edgeColor,
		.patternColor = patternColor,
		.slug = slug,
		.patternName = patternName.isEmpty() ? u"Cosmic Symbol"_q : patternName,
		.backdropName = backdropName.isEmpty() ? u"Celestial Velvet"_q : backdropName,
		.modelDoc = std::move(modelInfo),
		.patternDoc = std::move(patternInfo),
	};
}

void LocalServer::registerUniqueGift(
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
		const QString &patternName,
		const QString &backdropName) {
	if (slug.isEmpty()) {
		return;
	}
	_knownUniqueGifts[slug] = LocalGift{
		.id = id,
		.initialGiftId = id,
		.title = title,
		.number = number,
		.isUnique = true,
		.modelDocId = modelDocId,
		.patternDocId = patternDocId,
		.backdropId = backdropId,
		.centerColor = centerColor,
		.edgeColor = edgeColor,
		.patternColor = patternColor,
		.slug = slug,
		.patternName = patternName.isEmpty() ? u"Cosmic Symbol"_q : patternName,
		.backdropName = backdropName.isEmpty() ? u"Celestial Velvet"_q : backdropName,
		.modelDoc = { .id = modelDocId, .width = 512, .height = 512 },
		.patternDoc = { .id = patternDocId, .width = 512, .height = 512, .isCustomEmoji = true },
	};
}

bool LocalServer::buyGiftBySlug(
		const QString &slug,
		int price,
		PeerId toPeerId,
		const QString &msg) {
	if (_stars < price) {
		return false;
	}
	_stars -= price;
	_starsChanges.fire_copy(_stars);

	const auto it = _knownUniqueGifts.find(slug);
	if (it != _knownUniqueGifts.end()) {
		auto gift = it->second;
		gift.stars = price;
		gift.date = base::unixtime::now();
		gift.toId = toPeerId;
		gift.message = QString();
		gift.isUnique = true;
		addGift(std::move(gift));
		return true;
	}

	auto gift = LocalGift{
		.id = uint64(base::unixtime::now()),
		.title = slug,
		.number = int(1 + _gifts.size()),
		.stars = price,
		.date = base::unixtime::now(),
		.toId = toPeerId,
		.message = QString(),
		.isUnique = true,
		.slug = slug,
	};
	addGift(std::move(gift));
	return true;
}

bool LocalServer::buyGift(
		uint64 giftId,
		const QString &title,
		int number,
		int price,
		PeerId toPeerId,
		const QString &msg,
		uint64 modelDocId,
		uint64 patternDocId,
		uint32 backdropId,
		int centerColor,
		int edgeColor,
		int patternColor) {
	if (_stars < price) {
		return false;
	}
	_stars -= price;
	_starsChanges.fire_copy(_stars);

	auto realTitle = title;
	auto realNumber = number;
	auto realModelDocId = modelDocId;
	auto realPatternDocId = patternDocId;
	auto realBackdropId = backdropId;
	auto realCenterColor = centerColor;
	auto realEdgeColor = edgeColor;
	auto realPatternColor = patternColor;
	auto modelDoc = LocalDocumentInfo{ .id = modelDocId, .width = 512, .height = 512 };
	auto patternDoc = LocalDocumentInfo{ .id = patternDocId, .width = 512, .height = 512, .isCustomEmoji = true };

	const auto it = _knownGifts.find(giftId);
	if (it != _knownGifts.end()) {
		if (realTitle.isEmpty() || realTitle == u"Exclusive Gift"_q) {
			realTitle = it->second.title;
		}
		if (!realNumber && it->second.number) {
			realNumber = it->second.number;
		}
		if (!realModelDocId) {
			realModelDocId = it->second.modelDocId;
		}
		if (!realPatternDocId) {
			realPatternDocId = it->second.patternDocId;
		}
		if (!realBackdropId) {
			realBackdropId = it->second.backdropId;
		}
		if (!realCenterColor) {
			realCenterColor = it->second.centerColor;
		}
		if (!realEdgeColor) {
			realEdgeColor = it->second.edgeColor;
		}
		if (!realPatternColor) {
			realPatternColor = it->second.patternColor;
		}
		if (it->second.modelDoc.id) {
			modelDoc = it->second.modelDoc;
		}
		if (it->second.patternDoc.id) {
			patternDoc = it->second.patternDoc;
		}
	}

	auto gift = LocalGift{
		.id = (giftId ? giftId : uint64(base::unixtime::now())),
		.initialGiftId = giftId,
		.title = realTitle,
		.number = realNumber,
		.stars = price,
		.date = base::unixtime::now(),
		.toId = toPeerId,
		.message = QString(),
		.isUnique = (backdropId != 0 || centerColor != 0 || edgeColor != 0),
		.modelDocId = realModelDocId,
		.patternDocId = realPatternDocId,
		.backdropId = realBackdropId,
		.centerColor = realCenterColor,
		.edgeColor = realEdgeColor,
		.patternColor = realPatternColor,
		.modelDoc = std::move(modelDoc),
		.patternDoc = std::move(patternDoc),
	};
	addGift(std::move(gift));
	return true;
}

not_null<DocumentData*> LocalServer::ensureModelDocument(
		not_null<Main::Session*> session,
		const LocalGift &gift) const {
	const auto docId = gift.modelDoc.id ? gift.modelDoc.id : (gift.modelDocId ? gift.modelDocId : (0x7FFFFFFF00000000ULL | (gift.id ? gift.id : 1)));
	const auto doc = session->data().document(docId);
	if (!doc->dimensions.isEmpty() && doc->sticker()) {
		return doc;
	}
	auto attributes = QVector<MTPDocumentAttribute>();
	attributes.push_back(MTP_documentAttributeSticker(
		MTP_flags(0),
		MTP_string(gift.modelDoc.alt),
		MTP_inputStickerSetEmpty(),
		MTPMaskCoords()));
	attributes.push_back(MTP_documentAttributeImageSize(
		MTP_int(gift.modelDoc.width ? gift.modelDoc.width : 512),
		MTP_int(gift.modelDoc.height ? gift.modelDoc.height : 512)));

	doc->date = gift.date ? gift.date : base::unixtime::now();
	doc->setMimeString(gift.modelDoc.mime.isEmpty() ? u"application/x-tgsticker"_q : gift.modelDoc.mime);
	doc->size = gift.modelDoc.size ? gift.modelDoc.size : 1024;
	doc->dimensions = QSize(gift.modelDoc.width ? gift.modelDoc.width : 512, gift.modelDoc.height ? gift.modelDoc.height : 512);
	doc->setattributes(attributes);
	doc->recountIsImage();
	if (gift.modelDoc.accessHash) {
		doc->setRemoteLocation(gift.modelDoc.dc ? gift.modelDoc.dc : 1, gift.modelDoc.accessHash, gift.modelDoc.fileReference);
	}

	return doc;
}

not_null<DocumentData*> LocalServer::ensurePatternDocument(
		not_null<Main::Session*> session,
		const LocalGift &gift) const {
	const auto docId = gift.patternDoc.id ? gift.patternDoc.id : (gift.patternDocId ? gift.patternDocId : (gift.modelDocId ? gift.modelDocId : (0x7FFFFFFE00000000ULL | (gift.id ? gift.id : 1))));
	const auto doc = session->data().document(docId);
	if (!doc->dimensions.isEmpty() && doc->sticker()) {
		return doc;
	}
	using Flag = MTPDdocumentAttributeCustomEmoji::Flag;
	auto attributes = QVector<MTPDocumentAttribute>();
	attributes.push_back(MTP_documentAttributeCustomEmoji(
		MTP_flags(Flag::f_free | Flag::f_text_color),
		MTP_string(gift.patternDoc.alt),
		MTP_inputStickerSetEmpty()));
	attributes.push_back(MTP_documentAttributeSticker(
		MTP_flags(0),
		MTP_string(gift.patternDoc.alt),
		MTP_inputStickerSetEmpty(),
		MTPMaskCoords()));
	attributes.push_back(MTP_documentAttributeImageSize(
		MTP_int(gift.patternDoc.width ? gift.patternDoc.width : 512),
		MTP_int(gift.patternDoc.height ? gift.patternDoc.height : 512)));

	doc->date = gift.date ? gift.date : base::unixtime::now();
	doc->setMimeString(gift.patternDoc.mime.isEmpty() ? (gift.modelDoc.mime.isEmpty() ? u"application/x-tgsticker"_q : gift.modelDoc.mime) : gift.patternDoc.mime);
	doc->size = gift.patternDoc.size ? gift.patternDoc.size : 1024;
	doc->dimensions = QSize(gift.patternDoc.width ? gift.patternDoc.width : 512, gift.patternDoc.height ? gift.patternDoc.height : 512);
	doc->setattributes(attributes);
	doc->recountIsImage();
	doc->overrideEmojiUsesTextColor(true);
	if (gift.patternDoc.accessHash || gift.modelDoc.accessHash) {
		const auto access = gift.patternDoc.accessHash ? gift.patternDoc.accessHash : gift.modelDoc.accessHash;
		const auto fileRef = !gift.patternDoc.fileReference.isEmpty() ? gift.patternDoc.fileReference : gift.modelDoc.fileReference;
		const auto dc = gift.patternDoc.dc ? gift.patternDoc.dc : (gift.modelDoc.dc ? gift.modelDoc.dc : 1);
		doc->setRemoteLocation(dc, access, fileRef);
	}

	return doc;
}

rpl::producer<bool> LocalServer::enabledValue() const {
	return _enabledChanges.events_starting_with_copy(_enabled);
}

rpl::producer<int64> LocalServer::starsValue() const {
	return _starsChanges.events_starting_with_copy(_stars);
}

rpl::producer<> LocalServer::giftsChanged() const {
	return _giftsChanges.events();
}

rpl::producer<> LocalServer::profileChanged() const {
	return _profileChanges.events();
}

void LocalServer::save() {
	QJsonObject root;
	root[u"enabled"_q] = _enabled;
	root[u"stars"_q] = QString::number(_stars);

	root[u"anonymousNumberEnabled"_q] = _anonymousNumberEnabled;
	root[u"anonymousNumber"_q] = _anonymousNumber;

	root[u"nftUsernamesEnabled"_q] = _nftUsernamesEnabled;
	QJsonArray usernamesArr;
	for (const auto &u : _nftUsernames) {
		usernamesArr.append(u);
	}
	root[u"nftUsernames"_q] = usernamesArr;

	QJsonArray giftsArr;
	for (const auto &g : _gifts) {
		QJsonObject obj;
		obj[u"id"_q] = QString::number(g.id);
		obj[u"initialGiftId"_q] = QString::number(g.initialGiftId);
		obj[u"title"_q] = g.title;
		obj[u"number"_q] = g.number;
		obj[u"stars"_q] = QString::number(g.stars);
		obj[u"date"_q] = QString::number(g.date);
		obj[u"fromId"_q] = QString::number(g.fromId.value);
		obj[u"toId"_q] = QString::number(g.toId.value);
		obj[u"fromName"_q] = g.fromName;
		obj[u"message"_q] = g.message;
		obj[u"isUnique"_q] = g.isUnique;
		obj[u"modelDocId"_q] = QString::number(g.modelDoc.id ? g.modelDoc.id : g.modelDocId);
		obj[u"patternDocId"_q] = QString::number(g.patternDoc.id ? g.patternDoc.id : g.patternDocId);
		obj[u"backdropId"_q] = QString::number(g.backdropId);
		obj[u"centerColor"_q] = QString::number(uint32(g.centerColor), 16);
		obj[u"edgeColor"_q] = QString::number(uint32(g.edgeColor), 16);
		obj[u"patternColor"_q] = QString::number(uint32(g.patternColor), 16);
		obj[u"textColor"_q] = QString::number(uint32(g.textColor), 16);
		obj[u"slug"_q] = g.slug;
		obj[u"patternName"_q] = g.patternName;
		obj[u"backdropName"_q] = g.backdropName;

		obj[u"modelDocAccess"_q] = QString::number(g.modelDoc.accessHash);
		obj[u"modelDocFileRef"_q] = QString::fromLatin1(g.modelDoc.fileReference.toBase64());
		obj[u"modelDocDc"_q] = g.modelDoc.dc;
		obj[u"modelDocSize"_q] = QString::number(g.modelDoc.size);
		obj[u"modelDocMime"_q] = g.modelDoc.mime;
		obj[u"modelDocWidth"_q] = g.modelDoc.width;
		obj[u"modelDocHeight"_q] = g.modelDoc.height;
		obj[u"modelDocAlt"_q] = g.modelDoc.alt;

		obj[u"patternDocAccess"_q] = QString::number(g.patternDoc.accessHash);
		obj[u"patternDocFileRef"_q] = QString::fromLatin1(g.patternDoc.fileReference.toBase64());
		obj[u"patternDocDc"_q] = g.patternDoc.dc;
		obj[u"patternDocSize"_q] = QString::number(g.patternDoc.size);
		obj[u"patternDocMime"_q] = g.patternDoc.mime;
		obj[u"patternDocWidth"_q] = g.patternDoc.width;
		obj[u"patternDocHeight"_q] = g.patternDoc.height;
		obj[u"patternDocAlt"_q] = g.patternDoc.alt;

		giftsArr.append(obj);
	}
	root[u"gifts"_q] = giftsArr;

	QDir().mkpath(u"tdata"_q);
	const auto target = filePath();
	QDir().mkpath(QFileInfo(target).dir().absolutePath());
	const auto temp = target + u".tmp"_q;
	QFile file(temp);
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
		file.close();
		QFile::remove(target);
		QFile::rename(temp, target);
	}
}

void LocalServer::load() {
	auto path = filePath();
	if (!QFile::exists(path)) {
		if (QFile::exists(cWorkingDir() + u"local.json"_q)) {
			path = cWorkingDir() + u"local.json"_q;
		} else if (QFile::exists(u"tdata/local_server.json"_q)) {
			path = u"tdata/local_server.json"_q;
		} else if (QFile::exists(u"local.json"_q)) {
			path = u"local.json"_q;
		}
	}
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(file.readAll());
	file.close();
	if (!doc.isObject()) {
		return;
	}
	const auto root = doc.object();
	_enabled = root.value(u"enabled"_q).toBool(false);
	
	const auto starsVal = root.value(u"stars"_q);
	if (starsVal.isString()) {
		_stars = starsVal.toString().toLongLong();
	} else {
		_stars = int64(starsVal.toDouble(0));
	}

	_anonymousNumberEnabled = root.value(u"anonymousNumberEnabled"_q).toBool(false);
	_anonymousNumber = root.value(u"anonymousNumber"_q).toString(u"+888 8888 8888"_q);

	_nftUsernamesEnabled = root.value(u"nftUsernamesEnabled"_q).toBool(false);
	_nftUsernames.clear();
	const auto usernamesArr = root.value(u"nftUsernames"_q).toArray();
	for (const auto &u : usernamesArr) {
		const auto str = u.toString().trimmed().remove('@');
		if (!str.isEmpty() && !ranges::contains(_nftUsernames, str)) {
			_nftUsernames.push_back(str);
		}
	}

	auto parseUInt64 = [](const QJsonValue &v) -> uint64 {
		if (v.isString()) return v.toString().toULongLong();
		if (v.isDouble()) return uint64(v.toDouble());
		return 0;
	};
	auto parseInt64 = [](const QJsonValue &v) -> int64 {
		if (v.isString()) return v.toString().toLongLong();
		if (v.isDouble()) return int64(v.toDouble());
		return 0;
	};
	auto parseColor = [](const QJsonValue &v) -> int {
		if (v.isString()) {
			bool ok = false;
			auto val = v.toString().toUInt(&ok, 16);
			if (ok) return int(val);
			return int(v.toString().toUInt());
		}
		if (v.isDouble()) return int(uint32(v.toDouble()));
		return v.toInt();
	};

	_gifts.clear();
	const auto giftsArr = root.value(u"gifts"_q).toArray();
	for (const auto &val : giftsArr) {
		if (!val.isObject()) {
			continue;
		}
		const auto obj = val.toObject();
		auto patName = obj.value(u"patternName"_q).toString();
		auto backName = obj.value(u"backdropName"_q).toString();

		auto mDocId = parseUInt64(obj.value(u"modelDocId"_q));
		auto pDocId = parseUInt64(obj.value(u"patternDocId"_q));

		auto modelInfo = LocalDocumentInfo{
			.id = mDocId,
			.accessHash = parseUInt64(obj.value(u"modelDocAccess"_q)),
			.fileReference = QByteArray::fromBase64(obj.value(u"modelDocFileRef"_q).toString().toLatin1()),
			.dc = obj.value(u"modelDocDc"_q).toInt(1),
			.size = parseInt64(obj.value(u"modelDocSize"_q)),
			.mime = obj.value(u"modelDocMime"_q).toString(u"application/x-tgsticker"_q),
			.width = obj.value(u"modelDocWidth"_q).toInt(512),
			.height = obj.value(u"modelDocHeight"_q).toInt(512),
			.alt = obj.value(u"modelDocAlt"_q).toString(),
			.isCustomEmoji = false,
		};

		auto patternInfo = LocalDocumentInfo{
			.id = pDocId ? pDocId : mDocId,
			.accessHash = parseUInt64(obj.value(u"patternDocAccess"_q)),
			.fileReference = QByteArray::fromBase64(obj.value(u"patternDocFileRef"_q).toString().toLatin1()),
			.dc = obj.value(u"patternDocDc"_q).toInt(1),
			.size = parseInt64(obj.value(u"patternDocSize"_q)),
			.mime = obj.value(u"patternDocMime"_q).toString(u"application/x-tgsticker"_q),
			.width = obj.value(u"patternDocWidth"_q).toInt(512),
			.height = obj.value(u"patternDocHeight"_q).toInt(512),
			.alt = obj.value(u"patternDocAlt"_q).toString(),
			.isCustomEmoji = true,
		};

		auto g = LocalGift{
			.id = parseUInt64(obj.value(u"id"_q)),
			.initialGiftId = parseUInt64(obj.value(u"initialGiftId"_q)),
			.title = obj.value(u"title"_q).toString(),
			.number = obj.value(u"number"_q).toInt(1),
			.stars = parseInt64(obj.value(u"stars"_q)),
			.date = TimeId(parseUInt64(obj.value(u"date"_q))),
			.fromId = PeerId(parseUInt64(obj.value(u"fromId"_q))),
			.toId = PeerId(parseUInt64(obj.value(u"toId"_q))),
			.fromName = obj.value(u"fromName"_q).toString(),
			.message = obj.value(u"message"_q).toString(),
			.isUnique = obj.value(u"isUnique"_q).toBool(true),
			.modelDocId = mDocId,
			.patternDocId = pDocId,
			.backdropId = uint32(parseUInt64(obj.value(u"backdropId"_q))),
			.centerColor = parseColor(obj.value(u"centerColor"_q)),
			.edgeColor = parseColor(obj.value(u"edgeColor"_q)),
			.patternColor = parseColor(obj.value(u"patternColor"_q)),
			.textColor = parseColor(obj.value(u"textColor"_q)),
			.slug = obj.value(u"slug"_q).toString(),
			.patternName = patName.isEmpty() ? u"Cosmic Symbol"_q : patName,
			.backdropName = backName.isEmpty() ? u"Celestial Velvet"_q : backName,
			.modelDoc = std::move(modelInfo),
			.patternDoc = std::move(patternInfo),
		};
		if (g.title == u"Exclusive Gift"_q && (!g.modelDocId || g.number == 0)) {
			continue;
		}
		if (g.number == 0 && g.slug.isEmpty()) {
			continue;
		}
		if (!g.isUnique && !g.modelDocId) {
			continue;
		}
		_gifts.push_back(std::move(g));
	}
}

std::optional<LocalGift> LocalServer::findGiftBySlug(const QString &slug) const {
	const auto it = _knownUniqueGifts.find(slug);
	if (it != _knownUniqueGifts.end()) {
		return it->second;
	}
	for (const auto &g : _gifts) {
		if (g.slug == slug) {
			return g;
		}
	}
	return std::nullopt;
}

std::optional<LocalGift> LocalServer::findKnownGift(uint64 giftId) const {
	const auto it = _knownGifts.find(giftId);
	if (it != _knownGifts.end()) {
		return it->second;
	}
	for (const auto &g : _gifts) {
		if (g.id == giftId || g.initialGiftId == giftId) {
			return g;
		}
	}
	return std::nullopt;
}

} // namespace Melow

