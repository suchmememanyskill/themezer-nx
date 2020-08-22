#pragma once
#include <JAGL.h>
#include "libs/cJSON.h"
#include <curl/curl.h>

typedef struct {
    unsigned char *buffer;
    size_t len;
    size_t buflen;
} get_request_t;

typedef struct {
    char *id;
    char *creator;
    char *name;
    char *description;
    char *lastUpdated;
    char *imgLink;
    char *thumbLink;
    int dlCount;
    int likeCount;
    int target;
    SDL_Texture *preview;
} ThemeInfo_t;

typedef struct {
    CURL *transfer;
    get_request_t data;
    int index;
} Transfer_t;

typedef struct {
    Transfer_t *transfers;
    int queueOffset;
    CURLM *transferer;
    bool finished;
} TransferInfo_t;

typedef struct {
    int target;
    int limit;
    int page;
    int sort;
    int order;
    char *search;
    int pageCount;
    int itemCount;
    int curPageItemCount;
    cJSON *response;
    ThemeInfo_t *themes;
    TransferInfo_t tInfo;
} RequestInfo_t;

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