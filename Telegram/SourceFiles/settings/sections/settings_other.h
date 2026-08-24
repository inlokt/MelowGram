/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
/*
 * modified for melowgram 23.07.2026
 */
#pragma once

#include "settings/settings_common.h"

namespace Settings {

[[nodiscard]] Type OtherId();

} // namespace Settings

[[nodiscard]] bool IsMelowGramSaveTTLMediaEnabled();
[[nodiscard]] bool IsMelowGramEditOthersMessagesEnabled();
[[nodiscard]] bool IsMelowGramDisplayRepostsInChannelsEnabled();
[[nodiscard]] bool IsMelowGramAlwaysShowLastVisitEnabled();
