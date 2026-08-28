#pragma once
#include <JAGL.h>
#include "libs/cJSON.h"
#include <curl/curl.h>
#include "model.h"

extern char cURLErrBuff[CURL_ERROR_SIZE];

typedef struct {
	ShapeLinker_t *menu;
	TextCentered_t *message;
	ProgressBar_t *bar;
	bool cancelled;
	bool finalResponseReady;
} DownloadProgressContext_t;

typedef enum {
	QuickIdLookupNone = 0,
	QuickIdLookupTheme,
	QuickIdLookupPack,
	QuickIdLookupSplash,
	QuickIdLookupRemoteInstall,
} QuickIdLookupType_t;

int GetThemesList(char *url, char *data, cJSON **response);
int InitCurlSession(void);
void CleanupCurlSession(void);
ShapeLinker_t *GenListItemsFromJson(cJSON *json);
int MakeJsonRequest(char *url, cJSON **response);
int MakeJsonRequestCancelable(char *url, cJSON **response, volatile bool *cancelRequested);
char *GenLink(RequestInfo_t *rI);
ShapeLinker_t *GenListItemList(RequestInfo_t *rI);
int GenThemeArray(RequestInfo_t *rI);
void SetDefaultsRequestInfo(RequestInfo_t *rI);
int DownloadThemeFromUrl(char *url, char *path, DownloadProgressContext_t *progress);
int HandleDownloadQueue(Context_t *ctx);
int AddThemeImagesToDownloadQueue(RequestInfo_t *rI, bool thumb);
int CleanupTransferInfo(RequestInfo_t *rI);
void FreeRequestContent(RequestInfo_t *rI);
int LookupByQuickId(const char *quickId, RequestInfo_t *rI, QuickIdLookupType_t *lookupType);
SDL_Texture *CreateThumbHashTexture(const char *encodedThumbHash);
