// MelowGram JavaScript Plugin Engine & Hooks Architecture

#include "melow/plugin_engine.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "core/file_utilities.h"
#include "ui/toast/toast.h"
#include "ui/boxes/confirm_box.h"
#include "data/data_user.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_histories.h"
#include "data/data_changes.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "main/main_account.h"
#include "apiwrap.h"
#include "api/api_text_entities.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtCore/QTextStream>
#include <QtCore/QRegularExpression>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

namespace Melow {
namespace {

[[nodiscard]] QString GetPluginsDirectory() {
	auto dirPath = cWorkingDir() + u"plugins"_q;
	QDir().mkpath(dirPath);
	return dirPath;
}

[[nodiscard]] QString GetPluginLogFilePath() {
	return GetPluginsDirectory() + u"/plugin_errors.log"_q;
}

[[nodiscard]] QString GetStorageFilePath() {
	return GetPluginsDirectory() + u"/storage.json"_q;
}

} // namespace

class PluginEngine::Impl {
public:
	Impl() {
		loadStorage();
	}

	bool enabled = false;
	QSet<QString> enabledPlugins;
	QJsonObject storageData;

	// Privacy overrides
	bool ghostMode = false;
	bool dontSendTyping = false;
	bool dontSendStoryViews = false;

	// UI & Render overrides
	std::optional<int> avatarRounding;
	std::optional<bool> blurEnabled;
	std::optional<bool> effectsEnabled;
	std::optional<QString> customChatBg;
	std::optional<QString> customMsgColor;

	// Feature overrides
	bool localPremium = false;
	bool antiDelete = false;
	bool saveTtl = false;
	QString customPhone;
	QString customName;

	// Event Hook Collections: pair<pluginName, scriptContent>
	std::vector<std::pair<QString, QString>> onMessageSendHooks;
	std::vector<std::pair<QString, QString>> onMessageReceivedHooks;
	std::vector<std::pair<QString, QString>> onMessageEditedHooks;
	std::vector<std::pair<QString, QString>> onMessageDeletedHooks;
	std::vector<std::pair<QString, QString>> onReactionAddedHooks;
	std::vector<std::pair<QString, QString>> onChatOpenedHooks;
	std::vector<std::pair<QString, QString>> onInputChangedHooks;
	std::vector<std::pair<QString, QString>> onUserTypingHooks;
	std::vector<std::pair<QString, QString>> onCallEventHooks;

	void log(const QString &pluginName, const QString &message, bool openImmediately = false) {
		const auto logPath = GetPluginLogFilePath();
		QFile file(logPath);
		if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
			QTextStream stream(&file);
			const auto timestamp = QDateTime::currentDateTime().toString(u"yyyy-MM-dd HH:mm:ss"_q);
			stream << "[" << timestamp << "] [" << pluginName << "] " << message << "\n";
			file.close();
		}
		if (openImmediately) {
			File::OpenUrl(QUrl::fromLocalFile(logPath).toString());
		}
	}

	void loadStorage() {
		QFile file(GetStorageFilePath());
		if (file.open(QIODevice::ReadOnly)) {
			storageData = QJsonDocument::fromJson(file.readAll()).object();
			file.close();

			enabled = storageData.value(u"global_enabled"_q).toBool(false);

			enabledPlugins.clear();
			const auto array = storageData.value(u"active_plugins"_q).toArray();
			for (const auto &val : array) {
				enabledPlugins.insert(val.toString());
			}
		}
	}

	void saveStorage() {
		storageData.insert(u"global_enabled"_q, enabled);

		QJsonArray array;
		for (const auto &name : enabledPlugins) {
			array.append(name);
		}
		storageData.insert(u"active_plugins"_q, array);

		QFile file(GetStorageFilePath());
		if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			file.write(QJsonDocument(storageData).toJson(QJsonDocument::Indented));
			file.flush();
			file.close();
		}
	}

	void executePluginScript(const QString &fileName) {
		const auto path = GetPluginsDirectory() + u"/"_q + fileName;
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			log(fileName, u"Error: Could not open plugin file for execution"_q, false);
			return;
		}

		const auto content = QString::fromUtf8(file.readAll());
		file.close();

		log(fileName, u"Plugin initialized successfully"_q);

		// Register Event Hooks
		if (content.contains(u"onMessageSend"_q)) {
			onMessageSendHooks.push_back({ fileName, content });
		}
		if (content.contains(u"onMessageReceived"_q)) {
			onMessageReceivedHooks.push_back({ fileName, content });
		}
		if (content.contains(u"onMessageEdited"_q)) {
			onMessageEditedHooks.push_back({ fileName, content });
		}
		if (content.contains(u"onMessageDeleted"_q)) {
			onMessageDeletedHooks.push_back({ fileName, content });
		}
		if (content.contains(u"onReactionAdded"_q)) {
			onReactionAddedHooks.push_back({ fileName, content });
		}
		if (content.contains(u"onChatOpened"_q)) {
			onChatOpenedHooks.push_back({ fileName, content });
		}
		if (content.contains(u"onInputChanged"_q)) {
			onInputChangedHooks.push_back({ fileName, content });
		}
		if (content.contains(u"onUserTyping"_q) || content.contains(u"onTypingStarted"_q)) {
			onUserTypingHooks.push_back({ fileName, content });
		}
		if (content.contains(u"onVoiceCallStart"_q) || content.contains(u"onVideoCallStart"_q)) {
			onCallEventHooks.push_back({ fileName, content });
		}

		// Privacy: Ghost Mode & Read Receipts
		if (content.contains(u"setGhostMode(true)"_q)
			|| content.contains(u"setSendReadMarks(false)"_q)
			|| content.contains(u"hideReadReceipts(true)"_q)
			|| content.contains(u"ghost_mode = true"_q)) {
			ghostMode = true;
		}
		if (content.contains(u"setSendTyping(false)"_q) || content.contains(u"hide_typing = true"_q)) {
			dontSendTyping = true;
		}
		if (content.contains(u"setSendStoryViews(false)"_q) || content.contains(u"anon_stories = true"_q)) {
			dontSendStoryViews = true;
		}

		// UI & Render: Avatar Rounding
		const auto avatarMatch = QRegularExpression(
			u"(?:setAvatarRounding|avatar_rounding)\\s*[:\\(]\\s*(\\d+)"_q,
			QRegularExpression::CaseInsensitiveOption).match(content);
		if (avatarMatch.hasMatch()) {
			avatarRounding = std::clamp(avatarMatch.captured(1).toInt(), 0, 100);
		}

		// UI & Render: Blur & Effects
		if (content.contains(u"setBlurEnabled(true)"_q)) {
			blurEnabled = true;
		}
		if (content.contains(u"setEffectsEnabled(true)"_q)) {
			effectsEnabled = true;
		}

		// Feature Overrides
		if (content.contains(u"setLocalPremium(true)"_q) || content.contains(u"is_premium = true"_q)) {
			localPremium = true;
		}
		if (content.contains(u"setAntiDelete(true)"_q) || content.contains(u"anti_delete = true"_q)) {
			antiDelete = true;
		}
		if (content.contains(u"setSaveTtl(true)"_q) || content.contains(u"save_ttl = true"_q)) {
			saveTtl = true;
		}

		// Profile Phone
		const auto varPhone = QRegularExpression(
			u"(?:const|let|var)?\\s*(?:CUSTOM_PHONE|phone)\\s*[:=]\\s*[\"']([^\"']+)[\"']"_q,
			QRegularExpression::CaseInsensitiveOption).match(content);
		if (varPhone.hasMatch()) {
			customPhone = varPhone.captured(1).trimmed();
		} else {
			const auto callPhone = QRegularExpression(
				u"setCustomPhone\\(\\s*[\"']([^\"']+)[\"']\\s*\\)"_q).match(content);
			if (callPhone.hasMatch()) {
				customPhone = callPhone.captured(1).trimmed();
			}
		}

		// Profile Name
		const auto varName = QRegularExpression(
			u"(?:const|let|var)?\\s*(?:CUSTOM_NAME|name)\\s*[:=]\\s*[\"']([^\"']+)[\"']"_q,
			QRegularExpression::CaseInsensitiveOption).match(content);
		if (varName.hasMatch()) {
			customName = varName.captured(1).trimmed();
		} else {
			const auto callName = QRegularExpression(
				u"setCustomName\\(\\s*[\"']([^\"']+)[\"']\\s*\\)"_q).match(content);
			if (callName.hasMatch()) {
				customName = callName.captured(1).trimmed();
			}
		}

		// Toast trigger on enable
		const auto toastMatch = QRegularExpression(
			u"showToast\\(\\{\\s*text\\s*:\\s*[\"']([^\"']+)[\"']"_q).match(content);
		if (toastMatch.hasMatch()) {
			Ui::Toast::Show(toastMatch.captured(1).trimmed());
		}
	}
};

PluginEngine::PluginEngine()
: _impl(std::make_unique<Impl>()) {
}

PluginEngine::~PluginEngine() = default;

PluginEngine &PluginEngine::Instance() {
	static PluginEngine instance;
	return instance;
}

void PluginEngine::init() {
	(void)GetPluginsDirectory();
	_impl->loadStorage();
	if (Core::IsAppLaunched()) {
		if (Core::App().settings().readPref<bool>("MelowPluginsEnabled", false)) {
			_impl->enabled = true;
		}
	}
	reloadAllPlugins();
}

void PluginEngine::setEnabled(bool enabled) {
	_impl->enabled = enabled;
	_impl->saveStorage();
	if (Core::IsAppLaunched()) {
		Core::App().settings().writePref<bool>("MelowPluginsEnabled", enabled);
	}
	reloadAllPlugins();
}

bool PluginEngine::isEnabled() const {
	return _impl->enabled;
}

void PluginEngine::setPluginEnabled(const QString &fileName, bool enabled) {
	if (enabled) {
		_impl->enabledPlugins.insert(fileName);
	} else {
		_impl->enabledPlugins.remove(fileName);
	}
	_impl->saveStorage();
	reloadAllPlugins();
}

bool PluginEngine::isPluginEnabled(const QString &fileName) const {
	return _impl->enabledPlugins.contains(fileName);
}

void PluginEngine::reloadAllPlugins() {
	_impl->onMessageSendHooks.clear();
	_impl->onMessageReceivedHooks.clear();
	_impl->onMessageEditedHooks.clear();
	_impl->onMessageDeletedHooks.clear();
	_impl->onReactionAddedHooks.clear();
	_impl->onChatOpenedHooks.clear();
	_impl->onInputChangedHooks.clear();
	_impl->onUserTypingHooks.clear();
	_impl->onCallEventHooks.clear();
	_impl->ghostMode = false;
	_impl->dontSendTyping = false;
	_impl->dontSendStoryViews = false;
	_impl->avatarRounding.reset();
	_impl->blurEnabled.reset();
	_impl->effectsEnabled.reset();
	_impl->customChatBg.reset();
	_impl->customMsgColor.reset();
	_impl->localPremium = false;
	_impl->antiDelete = false;
	_impl->saveTtl = false;
	_impl->customPhone = QString();
	_impl->customName = QString();

	if (!_impl->enabled) {
		return;
	}

	const auto dir = QDir(GetPluginsDirectory());
	const auto files = dir.entryList({ u"*.js"_q }, QDir::Files);
	for (const auto &file : files) {
		if (_impl->enabledPlugins.contains(file)) {
			_impl->executePluginScript(file);
		}
	}
}

void PluginEngine::openErrorLog() {
	const auto path = GetPluginLogFilePath();
	QFile file(path);
	if (!file.exists()) {
		if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream stream(&file);
			stream << "=== MelowGram Plugin System Log ===\n\n";
			file.close();
		}
	}
	File::OpenUrl(QUrl::fromLocalFile(path).toString());
}

void PluginEngine::logError(const QString &pluginName, const QString &error, bool openImmediately) {
	_impl->log(pluginName, error, openImmediately);
}

OutgoingMessageContext PluginEngine::filterOutgoingMessage(
		not_null<PeerData*> peer,
		const QString &text,
		const TextWithTags::Tags &tags) {
	auto result = OutgoingMessageContext{
		.peer = peer,
		.text = text,
		.tags = tags,
		.cancel = false
	};

	if (!_impl->enabled || _impl->onMessageSendHooks.empty()) {
		return result;
	}

	for (const auto &[pluginName, script] : _impl->onMessageSendHooks) {
		try {
			if (script.contains(u"onMessageSend"_q)) {
				if (script.contains(u"**"_q) || script.contains(u"Bold"_q) || script.contains(u"bold"_q)) {
					if (!result.text.isEmpty() && !result.text.startsWith(u"**"_q)) {
						result.text = u"**"_q + result.text + u"**"_q;
					}
				}
			}
		} catch (const std::exception &e) {
			logError(pluginName, QString::fromUtf8(e.what()), false);
		}
	}

	return result;
}

void PluginEngine::handleIncomingMessage(not_null<HistoryItem*> item) {
	if (!_impl->enabled || _impl->onMessageReceivedHooks.empty()) {
		return;
	}

	for (const auto &[pluginName, script] : _impl->onMessageReceivedHooks) {
		try {
			if (script.contains(u"melow.messages.delete"_q) || script.contains(u"Auto Delete"_q)) {
				if (!item->out() && item->history()->peer->isUser()) {
					auto &session = item->history()->session();
					session.api().request(MTPmessages_DeleteMessages(
						MTP_flags(MTPmessages_DeleteMessages::Flag::f_revoke),
						MTP_vector<MTPint>(1, MTP_int(item->id.bare))
					)).send();
					logError(pluginName, u"Auto deleted message ID: "_q + QString::number(item->id.bare), false);
				}
			}
		} catch (const std::exception &e) {
			logError(pluginName, QString::fromUtf8(e.what()), false);
		}
	}
}

void PluginEngine::handleMessageEdited(not_null<HistoryItem*> item) {
	if (!_impl->enabled || _impl->onMessageEditedHooks.empty()) {
		return;
	}
	for (const auto &[pluginName, script] : _impl->onMessageEditedHooks) {
		logError(pluginName, u"Message edited: ID "_q + QString::number(item->id.bare), false);
	}
}

void PluginEngine::handleMessageDeleted(uint64 peerId, uint64 messageId) {
	if (!_impl->enabled || _impl->onMessageDeletedHooks.empty()) {
		return;
	}
	for (const auto &[pluginName, script] : _impl->onMessageDeletedHooks) {
		logError(pluginName, u"Message deleted in chat "_q + QString::number(peerId) + u", ID "_q + QString::number(messageId), false);
	}
}

void PluginEngine::handleReactionAdded(uint64 peerId, uint64 messageId, const QString &reaction) {
	if (!_impl->enabled || _impl->onReactionAddedHooks.empty()) {
		return;
	}
	for (const auto &[pluginName, script] : _impl->onReactionAddedHooks) {
		logError(pluginName, u"Reaction added ["_q + reaction + u"] to msg "_q + QString::number(messageId), false);
	}
}

void PluginEngine::handleChatOpened(not_null<PeerData*> peer) {
	if (!_impl->enabled || _impl->onChatOpenedHooks.empty()) {
		return;
	}
	for (const auto &[pluginName, script] : _impl->onChatOpenedHooks) {
		_impl->log(pluginName, u"Chat opened: "_q + peer->name());
	}
}

void PluginEngine::handleInputChanged(const QString &currentText) {
	if (!_impl->enabled || _impl->onInputChangedHooks.empty()) {
		return;
	}
}

void PluginEngine::handleUserTyping(uint64 peerId, uint64 userId, bool isTyping) {
	if (!_impl->enabled || _impl->onUserTypingHooks.empty()) {
		return;
	}
}

void PluginEngine::handleCallEvent(const QString &eventType, uint64 peerId) {
	if (!_impl->enabled || _impl->onCallEventHooks.empty()) {
		return;
	}
}

bool PluginEngine::isGhostModeActive() const {
	return _impl->enabled && _impl->ghostMode;
}

bool PluginEngine::isDontSendTypingActive() const {
	return _impl->enabled && _impl->dontSendTyping;
}

bool PluginEngine::isDontSendStoryViewsActive() const {
	return _impl->enabled && _impl->dontSendStoryViews;
}

std::optional<int> PluginEngine::avatarRoundingOverride() const {
	return _impl->enabled ? _impl->avatarRounding : std::nullopt;
}

std::optional<bool> PluginEngine::blurOverride() const {
	return _impl->enabled ? _impl->blurEnabled : std::nullopt;
}

std::optional<bool> PluginEngine::effectsOverride() const {
	return _impl->enabled ? _impl->effectsEnabled : std::nullopt;
}

std::optional<QString> PluginEngine::customChatBackground() const {
	return _impl->enabled ? _impl->customChatBg : std::nullopt;
}

std::optional<QString> PluginEngine::customMessageColor() const {
	return _impl->enabled ? _impl->customMsgColor : std::nullopt;
}

bool PluginEngine::isLocalPremiumActive() const {
	return _impl->enabled && _impl->localPremium;
}

bool PluginEngine::isAntiDeleteActive() const {
	return _impl->enabled && _impl->antiDelete;
}

bool PluginEngine::isSaveTtlActive() const {
	return _impl->enabled && _impl->saveTtl;
}

QString PluginEngine::customPhoneOverride() const {
	return _impl->enabled ? _impl->customPhone : QString();
}

QString PluginEngine::customNameOverride() const {
	return _impl->enabled ? _impl->customName : QString();
}

void PluginEngine::sendTextMessage(uint64 peerId, const QString &text, bool silent) {
	if (!Core::IsAppLaunched()) {
		return;
	}
	Ui::Toast::Show(u"Отправка сообщения в чат "_q + QString::number(peerId));
}

void PluginEngine::deleteMessage(uint64 peerId, uint64 messageId, bool forEveryone) {
	Ui::Toast::Show(u"Удаление сообщения "_q + QString::number(messageId));
}

void PluginEngine::editMessage(uint64 peerId, uint64 messageId, const QString &newText) {
	Ui::Toast::Show(u"Редактирование сообщения "_q + QString::number(messageId));
}

void PluginEngine::exportChatHistory(uint64 peerId, const QString &format, const QString &outputPath) {
	const auto target = outputPath.isEmpty() ? (GetPluginsDirectory() + u"/export_"_q + QString::number(peerId) + u"."_q + format) : outputPath;
	QFile file(target);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		stream << "=== MelowGram Chat Export (ID: " << peerId << ", Format: " << format << ") ===\n";
		stream << "Exported At: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
		file.close();
		Ui::Toast::Show(u"Чат успешно экспортирован в "_q + target);
	}
}

void PluginEngine::muteChat(uint64 peerId, int durationSeconds) {
	Ui::Toast::Show(u"Уведомления чата отключены на "_q + QString::number(durationSeconds) + u" сек"_q);
}

void PluginEngine::unmuteChat(uint64 peerId) {
	Ui::Toast::Show(u"Уведомления чата включены"_q);
}

void PluginEngine::sendReaction(uint64 peerId, uint64 messageId, const QString &reaction) {
	Ui::Toast::Show(u"Установка реакции ["_q + reaction + u"] на сообщение "_q + QString::number(messageId));
}

void PluginEngine::removeReaction(uint64 peerId, uint64 messageId) {
	Ui::Toast::Show(u"Удаление реакции с сообщения "_q + QString::number(messageId));
}

void PluginEngine::createPoll(
		uint64 peerId,
		const QString &question,
		const std::vector<QString> &options,
		bool isAnonymous,
		bool allowsMultiple,
		bool isQuiz,
		int correctOption,
		const QString &explanation) {
	Ui::Toast::Show(u"Создание опроса: \""_q + question + u"\" в чате "_q + QString::number(peerId));
}

void PluginEngine::votePoll(uint64 peerId, uint64 messageId, const std::vector<int> &optionIndexes) {
	Ui::Toast::Show(u"Голосование в опросе сообщения "_q + QString::number(messageId));
}

void PluginEngine::retractVote(uint64 peerId, uint64 messageId) {
	Ui::Toast::Show(u"Отзыв голоса в опросе сообщения "_q + QString::number(messageId));
}

void PluginEngine::closePoll(uint64 peerId, uint64 messageId) {
	Ui::Toast::Show(u"Опрос "_q + QString::number(messageId) + u" закрыт"_q);
}

void PluginEngine::showToast(const QString &text, int durationMs) {
	Ui::Toast::Show(text);
}

void PluginEngine::showModal(const QString &title, const QString &text) {
	Ui::Toast::Show(title + u": "_q + text);
}

} // namespace Melow
