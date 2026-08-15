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
		auto ok = false;
		const auto val = part.toInt(&ok);
		result.push_back(ok ? val : 0);
	}
	return result;
}

[[nodiscard]] bool IsVersionGreater(
		const QString &remote,
		const QString &local) {
	if (remote.isEmpty() || local.isEmpty()) {
		return false;
	}
	const auto remoteParts = ParseVersionNumbers(remote);
	const auto localParts = ParseVersionNumbers(local);
	const auto count = std::max(remoteParts.size(), localParts.size());
	for (auto i = std::size_t(0); i < count; ++i) {
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
	const auto tryPath = [](const QString &dir) -> QString {
		const auto path = dir + u"version.json"_q;
		auto f = QFile(path);
		if (!f.open(QIODevice::ReadOnly)) {
			return QString();
		}
		const auto doc = QJsonDocument::fromJson(f.readAll());
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
			QProcess::startDetached(path, QStringList());
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
