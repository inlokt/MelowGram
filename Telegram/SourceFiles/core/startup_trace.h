#pragma once

#include <cstdio>
#include <cstdlib>
#include <windows.h>

inline void StartupTrace(const char *msg) {
	OutputDebugStringA(msg);
	OutputDebugStringA("\n");

	char exePath[MAX_PATH];
	if (GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
		char *lastSlash = strrchr(exePath, '\\');
		if (lastSlash) {
			strcpy(lastSlash + 1, "melow_trace.txt");
			if (FILE *f = fopen(exePath, "a")) {
				fprintf(f, "%s\n", msg);
				fflush(f);
				fclose(f);
			}
		}
	}

	char tempPath[MAX_PATH];
	if (GetTempPathA(MAX_PATH, tempPath)) {
		strcat(tempPath, "melow_trace.txt");
		if (FILE *f = fopen(tempPath, "a")) {
			fprintf(f, "%s\n", msg);
			fflush(f);
			fclose(f);
		}
	}
}
