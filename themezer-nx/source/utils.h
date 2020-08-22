#pragma once
#include <switch.h>

#define NNFREE(x) if (x) { free(x); x = NULL;} // Not null free

char* showKeyboard(char* message, char* initialText, u64 size);
int isStringNullOrEmpty(const char *in);
char *SanitizeString(const char *name);
char *GetInstallArgs(const char *path);
void SetInstallSlot(int pos, char *path);
int CheckIfInstallSlotIsFree(int pos);
void AllocateInstalls(int len);
int CheckIfInstallsQueued();
const char* GetThemeInstallerPath();
char *GetThemePath(const char *creator, const char *themeName, const char *themeType);