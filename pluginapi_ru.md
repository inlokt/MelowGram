
# 🍃 MelowGram Plugin API

Study the API in English: [EN🇬🇧](https://github.com/inlokt/MelowGram/blob/dev/pluginapi_en.md)

> **JS API для плагинов MelowGram 2.0.0**  
> Управление чатами, сообщениями, реакциями, опросами, медиа, UI, событиями, приватностью и данными.

[![MelowGram](https://img.shields.io/badge/MelowGram-2.0.0-8cc84b?style=for-the-badge)](https://t.me/melowdesktop)
[![JavaScript](https://img.shields.io/badge/API-JavaScript-F7DF1E?style=for-the-badge&logo=javascript&logoColor=black)](https://developer.mozilla.org/docs/Web/JavaScript)

---

## 📚 Содержание

- [Установка плагинов](#-установка-плагинов)
- [Структура плагина](#-структура-плагина)
- [Lifecycle](#-lifecycle)
- [Polls](#-polls)
- [Chats & Contacts](#-chats--contacts)
- [Notifications](#-notifications)
- [Events](#-events)
- [Export](#-export)
- [UI & Render](#-ui--render)
- [Privacy](#-privacy)
- [Пример плагина](#-пример-плагина)

---

## 📦 Установка плагинов

Плагины размещаются в:

```text
plugins/
```

Поддерживаются:

- `.js` — файлы плагинов
- `.svg` — иконки
- `.png` — иконки

Размер иконки:

```text
20x20 px ( melowgram сам может задать ей такое разрешение )
```

Папку плагинов можно открыть через **Open Plugins Folder**.

Плагины активируются без перезапуска MelowGram и сохраняют состояние между сессиями.

---

## 🧩 Структура плагина

Минимальный плагин:

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

## 🏷️ Метаданные

| Переменная | Тип | Описание |
|---|---|---|
| `title` | `string` | Название плагина |
| `description` | `string` | Описание |
| `version` | `string` | Версия |
| `author` | `string` | Автор |
| `icon` | `string` | SVG/PNG иконка |

Пример:

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

Вызывается при включении плагина.

```js
function onEnable() {
    console.log("Plugin enabled");
}
```

### `onDisable()`

Вызывается при выключении плагина.

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

Добавляет реакцию.

```js
melow.reaction.add(peerId, messageId, "🔥");
```

### `remove(peerId, messageId)`

Удаляет реакцию.

```js
melow.reaction.remove(peerId, messageId);
```

### `getList(peerId, messageId)`

Получает список реакций.

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

Получает доступные реакции.

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

Создаёт опрос или викторину.

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

### Параметры

| Параметр | Тип | По умолчанию | Описание |
|---|---|---:|---|
| `question` | `string` | — | Вопрос |
| `options` | `string[]` | — | Варианты ответа, до 10 |
| `isAnonymous` | `boolean` | `true` | Анонимный опрос |
| `allowsMultipleAnswers` | `boolean` | `false` | Несколько вариантов |
| `isQuiz` | `boolean` | `false` | Режим викторины |
| `correctOptionId` | `number` | — | Индекс правильного ответа |
| `explanation` | `string` | — | Пояснение к ответу |

### `vote(peerId, messageId, optionIndexes)`

Голосует в опросе.

```js
melow.poll.vote(peerId, messageId, [0]);
```

Несколько вариантов:

```js
melow.poll.vote(peerId, messageId, [0, 2]);
```

### `retractVote(peerId, messageId)`

Отзывает голос.

```js
melow.poll.retractVote(peerId, messageId);
```

### `getResults(peerId, messageId)`

Получает результаты опроса.

```js
const results = melow.poll.getResults(peerId, messageId);
```

Возвращает статистику:

```text
totalVoters
options
percentages
votes
```

### `close(peerId, messageId)`

Закрывает опрос.

```js
melow.poll.close(peerId, messageId);
```

---

# 💬 Chats & Contacts

## `melow.chat`

### `list()`

Получает список чатов.

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

Получает информацию о чате.

```js
const chat = melow.chat.get(peerId);
```

### `sendMessage(peerId, text, options)`

Отправляет сообщение.

```js
melow.chat.sendMessage(peerId, "Hello!");
```

### `getHistory(peerId, options)`

Получает историю сообщений.

```js
const history = melow.chat.getHistory(peerId, {
    limit: 100,
    offset: 0
});
```

### `search(query, options)`

Ищет сообщения.

```js
const results = melow.chat.search("hello");
```

По конкретному чату:

```js
const results = melow.chat.search("hello", {
    peerId
});
```

### `delete(peerId)`

Удаляет чат / выходит из чата.

```js
melow.chat.delete(peerId);
```

---

## 👤 `melow.contact`

### `get(userId)`

Получает информацию о пользователе.

```js
const user = melow.contact.get(userId);
```

Доступные данные:

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

Отправляет фотографию.

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

Отправляет файл без сжатия.

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

Отправляет голосовое сообщение.

```js
melow.media.sendVoice(
    peerId,
    "/path/voice.ogg"
);
```

### `sendRoundVideo(peerId, videoFilePath)`

Отправляет видеосообщение-кружок.

```js
melow.media.sendRoundVideo(
    peerId,
    "/path/video.mp4"
);
```

### `sendSticker(peerId, stickerId)`

Отправляет стикер.

```js
melow.media.sendSticker(
    peerId,
    stickerId
);
```

### `download(messageId, targetPath)`

Скачивает медиафайл.

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

Отключает уведомления на время.

```js
melow.notifications.mute(
    peerId,
    3600
);
```

### `unmute(peerId)`

Включает уведомления.

```js
melow.notifications.unmute(peerId);
```

### `setCustomSound(peerId, soundPath)`

Устанавливает индивидуальный звук.

Поддерживаются:

```text
.wav
.mp3
```

Пример:

```js
melow.notifications.setCustomSound(
    peerId,
    "/path/notification.mp3"
);
```

### `send(options)`

Отправляет системное уведомление.

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

## Доступные события

| Event | Описание |
|---|---|
| `onMessageSend` | Отправка сообщения |
| `onMessageReceived` | Получение сообщения |
| `onMessageEdited` | Редактирование сообщения |
| `onMessageDeleted` | Удаление сообщения |
| `onReactionAdded` | Добавление реакции |
| `onUserTyping` | Пользователь печатает |
| `onChatOpened` | Открытие чата |
| `onCallEvent` | События звонков |

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

### Отключение обработчика

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

Экспортирует переписку.

Поддерживаются:

```text
json
html
txt
```

Пример:

```js
melow.export.chat(peerId, {
    format: "json",
    outputPath: "/path/chat.json"
});
```

### `media(peerId, options)`

Экспортирует медиа.

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

Настраивает скругление аватарки.

```text
0   — квадрат
100 — круг
```

Пример:

```js
melow.ui.setAvatarRounding(50);
```

### `setBlurEnabled(bool)`

Включает или выключает Aero Glass blur.

```js
melow.ui.setBlurEnabled(true);
```

### `setEffectsEnabled(bool)`

Включает анимационные эффекты.

```js
melow.ui.setEffectsEnabled(true);
```

### `showToast(options)`

Показывает уведомление.

```js
melow.ui.showToast({
    text: "Готово!",
    duration: 3000
});
```

### `showModal(title, text)`

Показывает диалоговое окно.

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

Скрывает галочки прочтения.

```js
melow.privacy.setGhostMode(true);
```

### `setSendTyping(bool)`

Управляет отправкой статуса набора текста.

```js
melow.privacy.setSendTyping(false);
```

### `setSendStoryViews(bool)`

Управляет отправкой просмотров историй.

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

Изменяет отображаемый номер.

```js
melow.data.setCustomPhone(
    "+1 234 567 890"
);
```

### `setCustomName(string)`

Изменяет отображаемое имя.

```js
melow.data.setCustomName(
    "Melow User"
);
```

### `setLocalPremium(bool)`

Разблокирует Premium-функции в UI локально.

```js
melow.data.setLocalPremium(true);
```

### `setAntiDelete(bool)`

Сохраняет удалённые сообщения.

```js
melow.data.setAntiDelete(true);
```

### `setSaveTtl(bool)`

Сохраняет самоуничтожающиеся фото и видео.

```js
melow.data.setSaveTtl(true);
```

---

# 🚀 Пример плагина

Файл:

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

# 📌 Полный API

| Namespace | Возможности |
|---|---|
| `melow.reaction` | Реакции |
| `melow.poll` | Опросы и викторины |
| `melow.chat` | Чаты и сообщения |
| `melow.contact` | Пользователи |
| `melow.media` | Фото, файлы, голосовые, видео, стикеры |
| `melow.notifications` | Уведомления и звуки |
| `melow.events` | События приложения |
| `melow.export` | Экспорт чатов и медиа |
| `melow.ui` | Интерфейс |
| `melow.render` | Рендеринг |
| `melow.privacy` | Ghost Mode и приватность |
| `melow.data` | Данные профиля и локальные функции |

---

## ⚠️ Important

**MelowGram** — неофициальный форк Telegram Desktop.

Проект не связан с Telegram Messenger LLP, не является его официальным продуктом и не спонсируется Telegram.

---

## 🔗 Links

- 🍃 [MelowGram](https://t.me/melowdesktop)
- 💻 [MelowGram Repository](https://github.com/inlokt/melowgram)

---

## 📄 License

Source code is distributed under **GPLv3 with OpenSSL exception**.

See [`LICENSE`](LICENSE).
