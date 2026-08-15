/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/launcher.h"

#include <cstdio>

static void StartupTrace(const char *msg) {
	FILE *f = fopen("C:\\Users\\1337\\AppData\\Local\\Temp\\melow_trace.txt", "a");
	if (f) {
		fprintf(f, "%s\n", msg);
		fflush(f);
		fclose(f);
	}
}

int main(int argc, char *argv[]) {
	StartupTrace("main: entry");
	const auto launcher = Core::Launcher::Create(argc, argv);
	StartupTrace("main: Launcher created");
	return launcher ? launcher->exec() : 1;
}
