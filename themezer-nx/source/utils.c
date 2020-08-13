#include "utils.h"
#include <switch.h>
#include <stdlib.h>
#include <JAGL.h>

char* showKeyboard(char* message, u64 size){
	SwkbdConfig	skp; 
	Result keyrc = swkbdCreate(&skp, 0);
	char* out = NULL;
	out = (char *)calloc(sizeof(char), size + 1);

	if (R_SUCCEEDED(keyrc) && out != NULL){
		swkbdConfigMakePresetDefault(&skp);
		swkbdConfigSetGuideText(&skp, message);
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
	const char* forbiddenChars = "/?<>\\:*|\".";

	char *src = CopyTextUtil(name);
	char *c = src;
	while (*c)
	{
		if (strchr(forbiddenChars, *c))
			*c = '_';
		c++;
	}

	return src;
}