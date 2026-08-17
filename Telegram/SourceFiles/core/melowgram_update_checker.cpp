/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/melowgram_update_checker.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QScopeGuard>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

#include "settings.h"

namespace Core {
namespace {

[[nodiscard]] std::vector<int> ParseVersionNumbers(QString tag) {
	tag = tag.trimmed();
	if (tag.startsWith('v', Qt::CaseInsensitive)) {
		tag = tag.mid(1).trimmed();
	}
	const auto dashIndex = tag.indexOf('-');
	if (dashIndex >= 0) {
		tag = tag.left(dashIndex);
	}
	const auto parts = tag.split('.');
	auto result = std::vector<int>();
	result.reserve(parts.size());
	for (const auto &part : parts) {
		result.push_back(part.toInt());
	}
	return result;
}

[[nodiscard]] bool IsVersionGreater(const QString &candidate, const QString &current) {
	const auto candidateParts = ParseVersionNumbers(candidate);
	const auto currentParts = ParseVersionNumbers(current);
	const auto maxParts = std::max(candidateParts.size(), currentParts.size());
	for (size_t i = 0; i < maxParts; ++i) {
		const auto c = (i < candidateParts.size()) ? candidateParts[i] : 0;
		const auto cur = (i < currentParts.size()) ? currentParts[i] : 0;
		if (c > cur) {
			return true;
		} else if (c < cur) {
			return false;
		}
	}
	return false;
}

[[nodiscard]] QString ReadLocalVersionTag() {
	const auto tryPath = [](const QString &dir) -> QString {
		const auto path = dir + u"version.json"_q;
		auto file = QFile(path);
		if (!file.open(QIODevice::ReadOnly)) {
			return QString();
		}
		const auto doc = QJsonDocument::fromJson(file.readAll());
		if (!doc.isObject()) {
			return QString();
		}
		return doc.object().value(u"tag_name"_q).toString().trimmed();
	};
	auto tag = tryPath(cExeDir());
	if (tag.isEmpty()) {
		tag = tryPath(cWorkingDir());
	}
	return tag;
}

void LaunchUpdater() {
	const auto tryPath = [](const QString &dir) -> bool {
		const auto path = dir + u"Updater.exe"_q;
		if (QFile::exists(path)) {
#ifdef Q_OS_WIN
			const auto nativePath = QDir::toNativeSeparators(path).toStdWString();
			const auto nativeDir = QDir::toNativeSeparators(dir).toStdWString();
			ShellExecuteW(
				nullptr,
				L"open",
				nativePath.c_str(),
				nullptr,
				nativeDir.c_str(),
				SW_SHOWNORMAL);
#else
			QProcess::startDetached(path, QStringList());
#endif
			return true;
		}
		return false;
	};
	if (!tryPath(cExeDir())) {
		tryPath(cWorkingDir());
	}
}

class Checker final : public QObject {
public:
	Checker() = default;

	void start() {
		const auto url = QUrl(
			u"https://api.github.com/repos/inlokt/MelowGram/releases/latest"_q);
		auto request = QNetworkRequest(url);
		request.setHeader(QNetworkRequest::UserAgentHeader, u"MelowGram"_q);
		request.setRawHeader("Accept", "application/vnd.github.v3+json");
		request.setAttribute(
			QNetworkRequest::RedirectPolicyAttribute,
			QNetworkRequest::NoLessSafeRedirectPolicy);
		request.setTransferTimeout(10000);

		const auto manager = new QNetworkAccessManager(this);
		const auto reply = manager->get(request);

		connect(reply, &QNetworkReply::finished, this, [=] {
			const auto deleteGuard = qScopeGuard([=] {
				reply->deleteLater();
				deleteLater();
			});

			if (reply->error() != QNetworkReply::NoError) {
				return;
			}
			const auto statusCode = reply->attribute(
				QNetworkRequest::HttpStatusCodeAttribute).toInt();
			if (statusCode != 200) {
				return;
			}

			const auto doc = QJsonDocument::fromJson(reply->readAll());
			if (!doc.isObject()) {
				return;
			}

			const auto githubTag = doc.object().value(
				u"tag_name"_q
			).toString().trimmed();
			if (githubTag.isEmpty()) {
				return;
			}

			const auto localTag = ReadLocalVersionTag();
			if (localTag.isEmpty()) {
				return;
			}

			if (IsVersionGreater(githubTag, localTag)) {
				LaunchUpdater();
			}
		});
	}
};

} // namespace

void CheckMelowGramUpdate() {
	QTimer::singleShot(3000, [] {
		const auto checker = new Checker();
		checker->start();
	});
}

} // namespace Core
