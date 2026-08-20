
# 🍃 MelowGram Plugin API

Изучить апи на Русском: [RU🇷🇺](https://github.com/inlokt/MelowGram/blob/dev/pluginapi_ru.md)

> **JS API for MelowGram 2.0.0**  
> Manage chats, messages, reactions, polls, media, UI, events, privacy, and data.

[![MelowGram](https://img.shields.io/badge/MelowGram-2.0.0-8cc84b?style=for-the-badge)](https://t.me/melowdesktop)
[![JavaScript](https://img.shields.io/badge/API-JavaScript-F7DF1E?style=for-the-badge&logo=javascript&logoColor=black)](https://developer.mozilla.org/docs/Web/JavaScript)

---

## 📚 Table of Contents

- [Installing Plugins](#-установка-плагинов)
- [Plugin Structure](#-структура-плагина)
- [Lifecycle](#-lifecycle)
- [Polls](#-polls)
- [Chats & Contacts](#-chats--contacts)
- [Notifications](#-notifications)
- [Events](#-events)
- [Export](#-export)
- [UI & Render](#-ui--render)
- [Privacy](#-privacy)
- [Plugin Example](#-пример-плагина)

---

## 📦 Installing Plugins

Plugins are placed in:

```text
plugins/
```

Supported:

- `.js` — plugin files
- `.svg` — icons
- `.png` — icons

Размер icons:

```text
20x20 px ( melowgram сам может задать ей такое разрешение )
```

The plugins folder can be opened via **Open Plugins Folder**.

Plugins are activated without restarting MelowGram and retain their state between sessions.

---

## 🧩 Plugin Structure

Minimal plugin:

```js
const title = "My Plugin";
const description = "Plugin description";
const version = "1.0.0";
const author = "YourName";
const icon = "myicon.svg";

function onEnable() {
    // Plugin enabled
}

function onDisable() {
    // Plugin disabled
}
```

---

## 🏷️ Metadata

| Variable | Type | Description |
|---|---|---|
| `title` | `string` | Plugin name |
| `description` | `string` | Description |
| `version` | `string` | Version |
| `author` | `string` | Author |
| `icon` | `string` | SVG/PNG icon |

Example:

```js
const title = "Super Plugin";
const description = "Дополнительные функции MelowGram";
const version = "1.0.0";
const author = "YourName";
const icon = "myicon.svg";
```

---

## 🔄 Lifecycle

### `onEnable()`

Called when the plugin is enabled.

```js
function onEnable() {
    console.log("Plugin enabled");
}
```

### `onDisable()`

Called when the plugin is disabled.

```js
function onDisable() {
    console.log("Plugin disabled");
}
```

---

# ❤️ Reactions

Namespace:

```js
melow.reaction
```

### `add(peerId, messageId, emoji)`

Adds a reaction.

```js
melow.reaction.add(peerId, messageId, "🔥");
```

### `remove(peerId, messageId)`

Removes a reaction.

```js
melow.reaction.remove(peerId, messageId);
```

### `getList(peerId, messageId)`

Gets the list of reactions.

```js
const reactions = melow.reaction.getList(peerId, messageId);
```

Результат:

```js
[
    {
        userId: 123,
        emoji: "🔥",
        date: 1710000000
    }
]
```

### `getAvailable(peerId)`

Gets the available reactions.

```js
const reactions = melow.reaction.getAvailable(peerId);
```

---

# 📊 Polls

Namespace:

```js
melow.poll
```

### `create(peerId, options)`

Creates a poll or quiz.

```js
melow.poll.create(peerId, {
    question: "Какой язык программирования лучше?",
    options: ["C++", "JavaScript", "Rust", "Python"],
    isAnonymous: false,
    allowsMultipleAnswers: false,
    isQuiz: true,
    correctOptionId: 0,
    explanation: "C++ обеспечивает максимальную скорость работы MelowGram!"
});
```

### Parameters

| Параметр | Type | Default | Description |
|---|---|---:|---|
| `question` | `string` | — | Question |
| `options` | `string[]` | — | Answer options, up to 10 |
| `isAnonymous` | `boolean` | `true` | Anonymous poll |
| `allowsMultipleAnswers` | `boolean` | `false` | Multiple choices |
| `isQuiz` | `boolean` | `false` | Quiz mode |
| `correctOptionId` | `number` | — | Correct answer index |
| `explanation` | `string` | — | Answer explanation |

### `vote(peerId, messageId, optionIndexes)`

Votes in the poll.

```js
melow.poll.vote(peerId, messageId, [0]);
```

Multiple choices:

```js
melow.poll.vote(peerId, messageId, [0, 2]);
```

### `retractVote(peerId, messageId)`

Retracts the vote.

```js
melow.poll.retractVote(peerId, messageId);
```

### `getResults(peerId, messageId)`

Gets poll results.

```js
const results = melow.poll.getResults(peerId, messageId);
```

Returns statistics:

```text
totalVoters
options
percentages
votes
```

### `close(peerId, messageId)`

Closes the poll.

```js
melow.poll.close(peerId, messageId);
```

---

# 💬 Chats & Contacts

## `melow.chat`

### `list()`

Gets the list of chats.

```js
const chats = melow.chat.list();
```

Формат:

```js
[
    {
        id,
        name,
        username,
        type,
        unreadCount,
        isMuted
    }
]
```

### `get(peerId)`

Gets chat information.

```js
const chat = melow.chat.get(peerId);
```

### `sendMessage(peerId, text, options)`

Sends a message.

```js
melow.chat.sendMessage(peerId, "Hello!");
```

### `getHistory(peerId, options)`

Gets message history.

```js
const history = melow.chat.getHistory(peerId, {
    limit: 100,
    offset: 0
});
```

### `search(query, options)`

Searches messages.

```js
const results = melow.chat.search("hello");
```

For a specific chat:

```js
const results = melow.chat.search("hello", {
    peerId
});
```

### `delete(peerId)`

Deletes the chat / leaves the chat.

```js
melow.chat.delete(peerId);
```

---

## 👤 `melow.contact`

### `get(userId)`

Gets user information.

```js
const user = melow.contact.get(userId);
```

Available data:

```js
{
    id,
    firstName,
    lastName,
    username,
    phone,
    isBot,
    isPremium
}
```

---

# 🖼️ Media

Namespace:

```js
melow.media
```

### `sendPhoto(peerId, filePath, options)`

Sends a photo.

```js
melow.media.sendPhoto(
    peerId,
    "/path/image.jpg",
    {
        caption: "My photo"
    }
);
```

### `sendDocument(peerId, filePath, options)`

Sends a file without compression.

```js
melow.media.sendDocument(
    peerId,
    "/path/file.zip",
    {
        caption: "File"
    }
);
```

### `sendVoice(peerId, audioFilePath)`

Sends a voice message.

```js
melow.media.sendVoice(
    peerId,
    "/path/voice.ogg"
);
```

### `sendRoundVideo(peerId, videoFilePath)`

Sends a round video message.

```js
melow.media.sendRoundVideo(
    peerId,
    "/path/video.mp4"
);
```

### `sendSticker(peerId, stickerId)`

Sends a sticker.

```js
melow.media.sendSticker(
    peerId,
    stickerId
);
```

### `download(messageId, targetPath)`

Downloads a media file.

```js
melow.media.download(
    messageId,
    "/path/download"
);
```

---

# 🔔 Notifications

Namespace:

```js
melow.notifications
```

### `mute(peerId, durationSeconds)`

Disables notifications for a period of time.

```js
melow.notifications.mute(
    peerId,
    3600
);
```

### `unmute(peerId)`

Enables notifications.

```js
melow.notifications.unmute(peerId);
```

### `setCustomSound(peerId, soundPath)`

Sets a custom sound.

Supported:

```text
.wav
.mp3
```

Example:

```js
melow.notifications.setCustomSound(
    peerId,
    "/path/notification.mp3"
);
```

### `send(options)`

Sends a system notification.

```js
melow.notifications.send({
    title: "MelowGram",
    body: "Новое сообщение",
    icon: "icon.png",
    peerId
});
```

---

# ⚡ Events

Namespace:

```js
melow.events
```

## Available events

| Event | Description |
|---|---|
| `onMessageSend` | Message sent |
| `onMessageReceived` | Message received |
| `onMessageEdited` | Message edited |
| `onMessageDeleted` | Message deleted |
| `onReactionAdded` | Reaction added |
| `onUserTyping` | User is typing |
| `onChatOpened` | Chat opened |
| `onCallEvent` | Call events |

### `onMessageSend`

```js
melow.events.onMessageSend((msg) => {
    console.log(msg);
});
```

### `onMessageReceived`

```js
melow.events.onMessageReceived((msg) => {
    console.log(msg);
});
```

### `onMessageEdited`

```js
melow.events.onMessageEdited((msg) => {
    console.log(msg);
});
```

### `onMessageDeleted`

```js
melow.events.onMessageDeleted((msg) => {
    console.log(msg);
});
```

### `onReactionAdded`

```js
melow.events.onReactionAdded((reaction) => {
    console.log(reaction);
});
```

### `onUserTyping`

```js
melow.events.onUserTyping((user) => {
    console.log(user);
});
```

### `onChatOpened`

```js
melow.events.onChatOpened((chat) => {
    console.log(chat);
});
```

### `onCallEvent`

```js
melow.events.onCallEvent((event) => {
    console.log(event);
});
```

### Disable handler

```js
melow.events.offMessageReceived();
```

---

# 📤 Export

Namespace:

```js
melow.export
```

### `chat(peerId, options)`

Exports a chat conversation.

Supported:

```text
json
html
txt
```

Example:

```js
melow.export.chat(peerId, {
    format: "json",
    outputPath: "/path/chat.json"
});
```

### `media(peerId, options)`

Exports media.

```js
melow.export.media(peerId, {
    types: ["photos", "videos"],
    outputPath: "/path/media"
});
```

---

# 🎨 UI & Render

Namespaces:

```js
melow.ui
melow.render
```

### `setAvatarRounding(value)`

Configures avatar corner rounding.

```text
0   — квадрат
100 — круг
```

Example:

```js
melow.ui.setAvatarRounding(50);
```

### `setBlurEnabled(bool)`

Enables or disables Aero Glass blur.

```js
melow.ui.setBlurEnabled(true);
```

### `setEffectsEnabled(bool)`

Enables animation effects.

```js
melow.ui.setEffectsEnabled(true);
```

### `showToast(options)`

Shows a notification.

```js
melow.ui.showToast({
    text: "Готово!",
    duration: 3000
});
```

### `showModal(title, text)`

Shows a dialog window.

```js
melow.ui.showModal(
    "MelowGram",
    "Плагин успешно активирован!"
);
```

---

# 👻 Privacy

Namespace:

```js
melow.privacy
```

### `setGhostMode(bool)`

Hides read receipts.

```js
melow.privacy.setGhostMode(true);
```

### `setSendTyping(bool)`

Controls sending the typing status.

```js
melow.privacy.setSendTyping(false);
```

### `setSendStoryViews(bool)`

Controls sending story views.

```js
melow.privacy.setSendStoryViews(false);
```

---

# 🗃️ Data

Namespace:

```js
melow.data
```

### `setCustomPhone(string)`

Changes the displayed phone number.

```js
melow.data.setCustomPhone(
    "+1 234 567 890"
);
```

### `setCustomName(string)`

Changes the displayed name.

```js
melow.data.setCustomName(
    "Melow User"
);
```

### `setLocalPremium(bool)`

Unlocks Premium features locally in the UI.

```js
melow.data.setLocalPremium(true);
```

### `setAntiDelete(bool)`

Preserves deleted messages.

```js
melow.data.setAntiDelete(true);
```

### `setSaveTtl(bool)`

Preserves self-destructing photos and videos.

```js
melow.data.setSaveTtl(true);
```

---

# 🚀 Plugin Example

File:

```text
plugins/reactions_poll.js
```

```js
const title = "Реакции и Опросы";
const description = "Авто-реакции и создание викторин";
const version = "1.0.0";
const author = "MelowGram";
const icon = "reactions.svg";

function onEnable() {
    melow.events.onMessageReceived((msg) => {
        if (!msg.isOutgoing && msg.isPrivateChat) {
            melow.reaction.add(
                msg.peerId,
                msg.id,
                "❤️"
            );
        }
    });

    melow.ui.showToast({
        text: "Авто-реакции активны!"
    });
}

function onDisable() {
    melow.events.offMessageReceived();
}

function createQuiz(peerId) {
    melow.poll.create(peerId, {
        question: "Какой язык программирования лучше?",
        options: [
            "C++",
            "JavaScript",
            "Rust",
            "Python"
        ],
        isAnonymous: false,
        isQuiz: true,
        correctOptionId: 0,
        explanation:
            "C++ обеспечивает максимальную скорость работы MelowGram!"
    });
}
```

---

# 📌 Full API

| Namespace | Features |
|---|---|
| `melow.reaction` | Reactions |
| `melow.poll` | Polls and quizzes |
| `melow.chat` | Chats and messages |
| `melow.contact` | Users |
| `melow.media` | Photos, files, voice messages, videos, stickers |
| `melow.notifications` | Notifications and sounds |
| `melow.events` | Application events |
| `melow.export` | Chat and media export |
| `melow.ui` | Interface |
| `melow.render` | Rendering |
| `melow.privacy` | Ghost Mode and privacy |
| `melow.data` | Profile data and local features |

---

## ⚠️ Important

**MelowGram** — an unofficial fork of Telegram Desktop.

The project is not affiliated with Telegram Messenger LLP, is not an official Telegram product, and is not sponsored by Telegram.

---

## 🔗 Links

- 🍃 [MelowGram](https://t.me/melowdesktop)
- 💻 [MelowGram Repository](https://github.com/inlokt/melowgram)

---

## 📄 License

Source code is distributed under **GPLv3 with OpenSSL exception**.

See [`LICENSE`](LICENSE).

