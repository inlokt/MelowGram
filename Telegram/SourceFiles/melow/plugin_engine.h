// MelowGram JavaScript Plugin Engine & Hooks Architecture

#pragma once

#include "base/basic_types.h"
#include "ui/text/text_entity.h"
#include <QtCore/QString>
#include <QtCore/QSet>
#include <QtCore/QMap>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <optional>
#include <vector>
#include <functional>
#include <memory>

class PeerData;
class UserData;
class HistoryItem;

namespace Main {
class Session;
} // namespace Main

namespace Melow {

struct OutgoingMessageContext {
	not_null<PeerData*> peer;
	QString text;
	TextWithTags::Tags tags;
	bool cancel = false;
};

struct IncomingMessageContext {
	not_null<HistoryItem*> item;
	bool cancel = false;
	bool deleteForEveryone = false;
};

struct ChatInfo {
	uint64 id = 0;
	QString name;
	QString username;
	QString type; // "user", "group", "channel", "bot"
	bool isMuted = false;
	int unreadCount = 0;
};

struct UserInfo {
	uint64 id = 0;
	QString firstName;
	QString lastName;
	QString username;
	QString phone;
	bool isBot = false;
	bool isPremium = false;
};

class PluginEngine final {
public:
	static PluginEngine &Instance();

	void init();
	void setEnabled(bool enabled);
	[[nodiscard]] bool isEnabled() const;

	void setPluginEnabled(const QString &fileName, bool enabled);
	[[nodiscard]] bool isPluginEnabled(const QString &fileName) const;

	void reloadAllPlugins();
	void openErrorLog();
	void logError(const QString &pluginName, const QString &error, bool openImmediately = false);

	// ==========================================
	// 1. Message & Event Hooks
	// ==========================================
	[[nodiscard]] OutgoingMessageContext filterOutgoingMessage(
		not_null<PeerData*> peer,
		const QString &text,
		const TextWithTags::Tags &tags);

	void handleIncomingMessage(not_null<HistoryItem*> item);
	void handleMessageEdited(not_null<HistoryItem*> item);
	void handleMessageDeleted(uint64 peerId, uint64 messageId);
	void handleReactionAdded(uint64 peerId, uint64 messageId, const QString &reaction);
	void handleChatOpened(not_null<PeerData*> peer);
	void handleInputChanged(const QString &currentText);
	void handleUserTyping(uint64 peerId, uint64 userId, bool isTyping);
	void handleCallEvent(const QString &eventType, uint64 peerId);

	// ==========================================
	// 2. Privacy & Ghost Mode
	// ==========================================
	[[nodiscard]] bool isGhostModeActive() const;
	[[nodiscard]] bool isDontSendTypingActive() const;
	[[nodiscard]] bool isDontSendStoryViewsActive() const;

	// ==========================================
	// 3. UI & Design Render Overrides
	// ==========================================
	[[nodiscard]] std::optional<int> avatarRoundingOverride() const;
	[[nodiscard]] std::optional<bool> blurOverride() const;
	[[nodiscard]] std::optional<bool> effectsOverride() const;
	[[nodiscard]] std::optional<QString> customChatBackground() const;
	[[nodiscard]] std::optional<QString> customMessageColor() const;

	// ==========================================
	// 4. Data & Profile Overrides
	// ==========================================
	[[nodiscard]] bool isLocalPremiumActive() const;
	[[nodiscard]] bool isAntiDeleteActive() const;
	[[nodiscard]] bool isSaveTtlActive() const;
	[[nodiscard]] QString customPhoneOverride() const;
	[[nodiscard]] QString customNameOverride() const;

	// ==========================================
	// 5. Programmatic Chat & Messages Operations
	// ==========================================
	void sendTextMessage(uint64 peerId, const QString &text, bool silent = false);
	void deleteMessage(uint64 peerId, uint64 messageId, bool forEveryone = true);
	void editMessage(uint64 peerId, uint64 messageId, const QString &newText);
	void exportChatHistory(uint64 peerId, const QString &format, const QString &outputPath);
	void muteChat(uint64 peerId, int durationSeconds);
	void unmuteChat(uint64 peerId);

	// ==========================================
	// 6. Reactions & Polls API
	// ==========================================
	void sendReaction(uint64 peerId, uint64 messageId, const QString &reaction);
	void removeReaction(uint64 peerId, uint64 messageId);
	void createPoll(
		uint64 peerId,
		const QString &question,
		const std::vector<QString> &options,
		bool isAnonymous = true,
		bool allowsMultiple = false,
		bool isQuiz = false,
		int correctOption = 0,
		const QString &explanation = QString());
	void votePoll(uint64 peerId, uint64 messageId, const std::vector<int> &optionIndexes);
	void retractVote(uint64 peerId, uint64 messageId);
	void closePoll(uint64 peerId, uint64 messageId);

	// ==========================================
	// 7. UI Helpers
	// ==========================================
	void showToast(const QString &text, int durationMs = 3000);
	void showModal(const QString &title, const QString &text);

private:
	PluginEngine();
	~PluginEngine();

	PluginEngine(const PluginEngine&) = delete;
	PluginEngine &operator=(const PluginEngine&) = delete;

	class Impl;
	std::unique_ptr<Impl> _impl;
};

} // namespace Melow
