#pragma once
#include <JAGL.h>
#include "libs/cJSON.h"
#include <curl/curl.h>
#include "model.h"

extern char cURLErrBuff[CURL_ERROR_SIZE];

int GetThemesList(char *url, char *data, cJSON **response);
ShapeLinker_t *GenListItemsFromJson(cJSON *json);
int MakeJsonRequest(char *url, cJSON **response);
char *GenLink(RequestInfo_t *rI);
ShapeLinker_t *GenListItemList(RequestInfo_t *rI);
int GenThemeArray(RequestInfo_t *rI);
void SetDefaultsRequestInfo(RequestInfo_t *rI);
int DownloadThemeFromID(char *id, char *path);
int HandleDownloadQueue(Context_t *ctx);
int AddThemeImagesToDownloadQueue(RequestInfo_t *rI, bool thumb);
int CleanupTransferInfo(RequestInfo_t *rI);
void FreeThemes(RequestInfo_t *rI);