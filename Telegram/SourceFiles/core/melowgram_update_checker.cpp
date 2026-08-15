/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/melowgram_update_checker.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "core/core_settings.h"

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
		auto ok = false;
		const auto val = part.toInt(&ok);
		result.push_back(ok ? val : 0);
	}
	return result;
}

[[nodiscard]] bool IsVersionGreater(const QString &remote, const QString &local) {
	if (remote.isEmpty() || local.isEmpty()) {
		return false;
	}
	const auto remoteParts = ParseVersionNumbers(remote);
	const auto localParts = ParseVersionNumbers(local);
	const auto count = std::max(remoteParts.size(), localParts.size());
	for (auto i = 0; i < count; ++i) {
		const auto r = (i < remoteParts.size()) ? remoteParts[i] : 0;
		const auto l = (i < localParts.size()) ? localParts[i] : 0;
		if (r > l) {
			return true;
		} else if (r < l) {
			return false;
		}
	}
	return false;
}

[[nodiscard]] QString ReadLocalVersionTag() {
	const auto localPath = QFile::exists(cExeDir() + u"version.json"_q)
		? (cExeDir() + u"version.json"_q)
		: (cWorkingDir() + u"version.json"_q);
	auto localFile = QFile(localPath);
	if (!localFile.open(QIODevice::ReadOnly)) {
		return QString();
	}
	const auto doc = QJsonDocument::fromJson(localFile.readAll());
	if (!doc.isObject()) {
		return QString();
	}
	return doc.object().value(u"tag_name"_q).toString().trimmed();
}

void LaunchUpdater() {
	const auto updaterPath = QFile::exists(cExeDir() + u"Updater.exe"_q)
		? (cExeDir() + u"Updater.exe"_q)
		: (cWorkingDir() + u"Updater.exe"_q);
	if (QFile::exists(updaterPath)) {
		QProcess::startDetached(updaterPath, QStringList());
	}
}

class Checker final : public QObject {
public:
	Checker() = default;

	void start() {
		const auto url = QUrl(u"https://api.github.com/repos/inlokt/MelowGram/releases/latest"_q);
		auto request = QNetworkRequest(url);
		request.setHeader(QNetworkRequest::UserAgentHeader, u"MelowGram"_q);
		request.setRawHeader("Accept", "application/vnd.github.v3+json");
		request.setAttribute(
			QNetworkRequest::RedirectPolicyAttribute,
			QNetworkRequest::NoLessSafeRedirectPolicy);

		const auto manager = new QNetworkAccessManager(this);
		const auto reply = manager->get(request);

		connect(reply, &QNetworkReply::finished, this, [=] {
			const auto guard = gsl::finally([=] {
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
	const auto checker = new Checker();
	checker->start();
}

} // namespace Core
