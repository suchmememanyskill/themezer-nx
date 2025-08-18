#include "utils.h"
#include <switch.h>
#include <stdlib.h>
#include <JAGL.h>
#include <unistd.h>
#include <sys/stat.h> 
#include "model.h"

typedef struct {
    char **paths;
    u8 len;
} ThemeInstallerArgs_t;

const char *possibleThemeInstallerPaths[] = {
	"/switch/NXThemesInstaller.nro",
	"/switch/NXThemesInstaller/NXThemesInstaller.nro",
	"/switch/Switch_themes_Installer/NXThemesInstaller.nro"
};

const char* GetThemeInstallerPath(){
	for (int i = 0; i < ARRAY_SIZE(possibleThemeInstallerPaths); i++){
		if (access(possibleThemeInstallerPaths[i], F_OK) != -1){
			return possibleThemeInstallerPaths[i];
		}
	}

	return NULL;
}

char *GetThemePath(const char *creator, const char *themeName, const char *themeType){
	return CopyTextArgsUtil("/Themes/ThemezerNX/%s - %s - %s.nxtheme", themeType, creator, themeName);
}

char* showKeyboard(char* message, char* initialText, u64 size){
	SwkbdConfig	skp; 
	Result keyrc = swkbdCreate(&skp, 0);
	char* out = NULL;
	out = (char *)calloc(sizeof(char), size + 1);

	if (R_SUCCEEDED(keyrc) && out != NULL){
		swkbdConfigMakePresetDefault(&skp);
		swkbdConfigSetGuideText(&skp, message);
		if (initialText)
			swkbdConfigSetInitialText(&skp, initialText);
		swkbdConfigSetStringLenMax(&skp, size);
		keyrc = swkbdShow(&skp, out, size);
		swkbdClose(&skp);	
	}

	else {
	    free(out);
	    out = NULL;
	}

	return (out);
}

int isStringNullOrEmpty(const char *in){
	if (in == NULL){
		return 1;
	}

	for (; *in; ++in){
		if (*in != ' ')
			return 0;
	}

	return 1;
}

char *SanitizeString(const char *name)
{
	const char* forbiddenChars = "/?<>\\:*|\".,";

	char *src = calloc(strlen(name) + 1, 1);
	const char *c = name;
	char *src_temp = src;
	while (*c)
	{
		if (!strchr(forbiddenChars, *c) && *c >= 32 && *c <= 126){
			*src_temp = *c;
			src_temp++;
		}
			
		c++;
	}

	if (!src[0]){
		src[0] = '_';
	}

	return src;
}

ThemeInstallerArgs_t QueuedInstalls;

void AllocateInstalls(int len){
	QueuedInstalls.len = len;
	QueuedInstalls.paths = calloc(sizeof(char*), len);
}

int GetInstallSlotOffset(char *name){
	for (int i = 0; i < 7; i++){
		if (!strcmp(targetOptions[i], name))
			return i;
	}

	return -1;
}

int CheckIfInstallSlotIsFree(int pos){
	if (pos < 0)
		return 0;

	return (QueuedInstalls.paths[pos] == NULL);
}

void SetInstallSlot(int pos, char *path){
	if (path == NULL){
		if (QueuedInstalls.paths[pos] != NULL){
			free(QueuedInstalls.paths[pos]);
			QueuedInstalls.paths[pos] = NULL;
		}
		return;
	}
	
	int len = 0;

	char *c = path;
	while(*c){
		len++;
		if (*c == ' ')
			len += 2;
		c++;
	}

	char *out = calloc(1, len + 1);
	char *temp = out;
	c = path;

	while(*c){
		if (*c == ' '){
			*temp = '(';
			temp++;
			*temp = '_';
			temp++;
			*temp = ')';
		}
		else {
			*temp = *c; 
		}

		temp++;
		c++;
	}

	QueuedInstalls.paths[pos] = out;
}

char *GetInstallArgs(const char *path){
	int len = 15, pathAmount = 0, temp = 0;
	len += strlen(path);

	for (int i = 0; i < QueuedInstalls.len; i++){
		if (QueuedInstalls.paths[i] != NULL){
			len += strlen(QueuedInstalls.paths[i]) + 1;
			pathAmount++;
		}
	}

	char *out = calloc(1, len);
	strcpy(out, path);
	strcat(out, " installtheme=");
	for (int i = 0; i < QueuedInstalls.len; i++){
		if (QueuedInstalls.paths[i] != NULL){
			strcat(out, QueuedInstalls.paths[i]);
			temp++;
			if (pathAmount > temp)
				strcat(out, ",");
		}
	}

	return out;
}

int CheckIfInstallsQueued(){
	for (int i = 0; i < QueuedInstalls.len; i++){
		if (QueuedInstalls.paths[i] != NULL){
			return 1;
		}
	}

	return 0;
}
