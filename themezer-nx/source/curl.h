#pragma once
#include <JAGL.h>
#include "libs/cJSON.h"

typedef struct {
    char *id;
    char *creator;
    char *name;
    char *description;
    char *lastUpdated;
    int dlCount;
    int likeCount;
    SDL_Texture *preview;
} ThemeInfo_t;

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
} RequestInfo_t;

int GetThemesList(char *url, char *data, cJSON **response);
ShapeLinker_t *GenListItemsFromJson(cJSON *json);
int MakeJsonRequest(char *url, cJSON **response);
char *GenLink(RequestInfo_t *rI);
ShapeLinker_t *GenListItemList(RequestInfo_t *rI);
int GenThemeArray(RequestInfo_t *rI);
int FillThemeArrayWithImg(RequestInfo_t *rI);
void SetDefaultsRequestInfo(RequestInfo_t *rI);
int DownloadThemeFromID(char *id, char *path);