/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/launcher.h"

#include "core/startup_trace.h"

int main(int argc, char *argv[]) {
	StartupTrace("main: entry");
	const auto launcher = Core::Launcher::Create(argc, argv);
	StartupTrace("main: Launcher created");
	return launcher ? launcher->exec() : 1;
}
